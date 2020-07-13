/*
 * Copyright (c) 2019, 2020, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "mysqlshdk/libs/storage/backend/oci_object_storage.h"

#include <openssl/err.h>
#include <openssl/pem.h>

#include <algorithm>
#include <memory>
#include <vector>

#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/storage/utils.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"

using Rest_service = mysqlshdk::rest::Rest_service;
using Response = mysqlshdk::rest::Response;
using BIO_ptr = std::unique_ptr<BIO, decltype(&::BIO_free)>;

namespace mysqlshdk {
namespace storage {
namespace backend {

namespace oci {

using Status_code = Response::Status_code;

using mysqlshdk::oci::Oci_options;
using mysqlshdk::oci::Oci_rest_service;
using mysqlshdk::oci::Oci_service;
using mysqlshdk::oci::Response_error;

Directory::Directory(const Oci_options &options, const std::string &name)
    : m_name(utils::scheme_matches(utils::get_scheme(name), "oci+os")
                 ? utils::strip_scheme(name, "oci+os")
                 : name),
      m_bucket(std::make_unique<Bucket>(options)),
      m_created(false) {}

bool Directory::exists() const {
  if (m_name.empty() || m_created) {
    // An empty name represents the root directory
    // check if connection can be established
    try {
      m_bucket->list_objects("", "", "", 1, false);
    } catch (const Response_error &error) {
      throw shcore::Exception::runtime_error(error.format());
    }
    return true;
  } else {
    // If it has a name then we need to make sure it is a directory, it will be
    // verified by listing 1 object using as prefix as '<m_name>/'
    auto files = list_files(true);
    return !files.empty();
  }
}

void Directory::create() { m_created = true; }

std::vector<IDirectory::File_info> Directory::list_files(
    bool hidden_files) const {
  std::vector<IDirectory::File_info> files;
  std::string prefix = m_name.empty() ? "" : m_name + "/";

  std::vector<mysqlshdk::oci::Object_details> objects;
  try {
    objects = m_bucket->list_objects(prefix, "", "", 0, false);
  } catch (const Response_error &error) {
    throw shcore::Exception::runtime_error(error.format());
  }

  if (prefix.empty()) {
    for (const auto &object : objects) {
      files.push_back({object.name, object.size});
    }
  } else {
    for (const auto &object : objects) {
      files.push_back({object.name.substr(prefix.size()), object.size});
    }
  }

  if (hidden_files) {
    // Active multipart uploads to the target path should be considered files
    std::vector<mysqlshdk::oci::Multipart_object> uploads;

    try {
      uploads = m_bucket->list_multipart_uploads();
    } catch (const Response_error &error) {
      throw shcore::Exception::runtime_error(error.format());
    }

    if (prefix.empty()) {
      for (const auto &upload : uploads) {
        // Only uploads not having '/' in the name belong to the root folder
        if (upload.name.find("/") == std::string::npos) {
          files.push_back({upload.name, 0});
        }
      }
    } else {
      for (const auto &upload : uploads) {
        if (shcore::str_beginswith(upload.name, prefix)) {
          files.push_back({upload.name.substr(prefix.size()), 0});
        }
      }
    }
  }

  return files;
}

std::string Directory::join_path(const std::string &a,
                                 const std::string &b) const {
  return a.empty() ? b : a + "/" + b;
}

std::unique_ptr<IFile> Directory::file(const std::string &name) const {
  std::string file_name = join_path(m_name, name);

  return std::make_unique<Object>(m_bucket->get_options(), name,
                                  m_name.empty() ? "" : m_name + "/");
}

Object::Object(const Oci_options &options, const std::string &name,
               const std::string &prefix)
    : m_name(name),
      m_prefix(prefix),
      m_bucket(std::make_unique<Bucket>(options)),
      m_max_part_size(*options.part_size),
      m_writer{},
      m_reader{} {}

void Object::set_max_part_size(size_t new_size) {
  assert(!is_open());
  m_max_part_size = new_size;
}

void Object::open(mysqlshdk::storage::Mode mode) {
  switch (mode) {
    case Mode::READ:
      m_reader = std::make_unique<Reader>(this);
      break;
    case Mode::APPEND: {
      std::vector<mysqlshdk::oci::Multipart_object> uploads;
      try {
        uploads = m_bucket->list_multipart_uploads();
      } catch (const Response_error &error) {
        throw shcore::Exception::runtime_error(error.format());
      }

      auto object = std::find_if(
          uploads.begin(), uploads.end(),
          [this](const Multipart_object &o) { return o.name == full_path(); });

      // If there no active upload, the APPEND operation will be allowed if the
      // file does not exist
      if (object == uploads.end()) {
        try {
          // Verifies if the file exists, if not, an error will be thrown by
          // head_object
          m_bucket->head_object(full_path());

          throw std::invalid_argument(
              "Object Storage only supports APPEND mode for in-progress "
              "multipart uploads or new files.");
        } catch (const mysqlshdk::rest::Response_error &error) {
          // If the file did not exist then OK to continue as a new file
          mode = Mode::WRITE;
        }
      }

      m_writer = std::make_unique<Writer>(this, &(*object));
      break;
    }
    case Mode::WRITE:
      m_writer = std::make_unique<Writer>(this);
      break;
  }

  m_open_mode = mode;
}

bool Object::is_open() const { return !m_open_mode.is_null(); }

void Object::close() {
  if (m_writer) m_writer->close();
  m_open_mode.reset();
  m_writer.reset();
  m_reader.reset();
}

size_t Object::file_size() const {
  if (!m_open_mode.is_null() && *m_open_mode != Mode::READ) {
    return m_writer->size();
  } else {
    return m_bucket->head_object(full_path());
  }
}

std::string Object::filename() const { return m_name; }

bool Object::exists() const {
  bool ret_val = true;
  try {
    file_size();
  } catch (const Response_error &error) {
    if (error.code() == Status_code::NOT_FOUND)
      ret_val = false;
    else
      throw shcore::Exception::runtime_error(error.format());
  }

  return ret_val;
}

std::unique_ptr<IDirectory> Object::parent() const {
  const auto path = full_path();
  const auto pos = path.find_last_of('/');

  // if the full path does not contain a backslash then the parent directory is
  // the root directory
  return make_directory(std::string::npos == pos ? "" : path.substr(0, pos),
                        m_bucket->get_options());
}

off64_t Object::seek(off64_t offset) {
  assert(is_open());

  off64_t ret_val = 0;

  if (m_reader)
    ret_val = m_reader->seek(offset);
  else if (m_writer)
    ret_val = m_writer->seek(offset);

  return ret_val;
}

off64_t Object::tell() const {
  assert(is_open());

  off64_t ret_val = 0;

  if (m_reader)
    ret_val = m_reader->tell();
  else if (m_writer)
    ret_val = m_writer->tell();

  return ret_val;
}

ssize_t Object::read(void *buffer, size_t length) {
  assert(is_open());

  if (length <= 0) return 0;

  return m_reader->read(buffer, length);
}

ssize_t Object::write(const void *buffer, size_t length) {
  assert(is_open());

  return m_writer->write(buffer, length);
}

void Object::rename(const std::string &new_name) {
  try {
    m_bucket->rename_object(m_prefix + m_name, m_prefix + new_name);
  } catch (const Response_error &error) {
    throw shcore::Exception::runtime_error(error.format());
  }
  m_name = new_name;
}

void Object::remove() { m_bucket->delete_object(full_path()); }

Object::Writer::Writer(Object *owner, Multipart_object *object)
    : File_handler(owner), m_is_multipart(false) {
  // This is the writer for an already started multipart object
  if (object) {
    m_multipart = *object;
    m_is_multipart = true;

    try {
      m_parts = m_object->m_bucket->list_multipart_upload_parts(m_multipart);
    } catch (const Response_error &error) {
      throw shcore::Exception::runtime_error(error.format());
    }

    for (const auto &part : m_parts) {
      m_size += part.size;
    }
  }
}

off64_t Object::Writer::seek(off64_t /*offset*/) { return 0; }

off64_t Object::Writer::tell() const { return size(); }

ssize_t Object::Writer::write(const void *buffer, size_t length) {
  const size_t MY_MAX_PART_SIZE = m_object->m_max_part_size;
  size_t to_send = m_buffer.size() + length;
  size_t incoming_offset = 0;
  char *incoming = reinterpret_cast<char *>(const_cast<void *>(buffer));

  // Initializes the multipart as soon as FILE_PART_SIZE data is provided
  if (!m_is_multipart && to_send > MY_MAX_PART_SIZE) {
    try {
      m_multipart =
          m_object->m_bucket->create_multipart_upload(m_object->full_path());
    } catch (const Response_error &error) {
      throw shcore::Exception::runtime_error(error.format());
    }

    m_is_multipart = true;
  }

  // This loops handles the upload of N number of chunks of size
  // MY_MAX_PART_SIZE including the buffered data and the incoming data
  while (to_send > MY_MAX_PART_SIZE) {
    if (!m_buffer.empty()) {
      // BUFFERED DATA: fills the buffer and sends it
      size_t buffer_space = MY_MAX_PART_SIZE - m_buffer.size();
      m_buffer.append(incoming + incoming_offset, buffer_space);

      try {
        m_parts.push_back(
            m_object->m_bucket->upload_part(m_multipart, m_parts.size() + 1,
                                            m_buffer.data(), MY_MAX_PART_SIZE));
      } catch (const mysqlshdk::rest::Response_error &error) {
        try {
          log_info(
              "Cancelling multipart upload after failure uploading part, "
              "error %s\nobject: %s\n upload id: %s",
              error.format().c_str(), m_multipart.name.c_str(),
              m_multipart.upload_id.c_str());

          m_object->m_bucket->abort_multipart_upload(m_multipart);
        } catch (const mysqlshdk::rest::Response_error &inner_error) {
          log_error(
              "Error cancelling multipart upload after failure uploading "
              "part, "
              "error %s\nobject: %s\n upload id: %s",
              inner_error.format().c_str(), m_multipart.name.c_str(),
              m_multipart.upload_id.c_str());
        }

        throw shcore::Exception::runtime_error(error.format());
      }
      m_buffer.clear();
      incoming_offset += buffer_space;
      to_send -= MY_MAX_PART_SIZE;
    } else {
      // NO BUFFERED DATA: sends the data directly from the incoming buffer
      try {
        m_parts.push_back(m_object->m_bucket->upload_part(
            m_multipart, m_parts.size() + 1, incoming + incoming_offset,
            MY_MAX_PART_SIZE));
      } catch (const mysqlshdk::rest::Response_error &error) {
        try {
          log_info(
              "Cancelling multipart upload after failure uploading part, "
              "error %s\nobject: %s\n upload id: %s",
              error.format().c_str(), m_multipart.name.c_str(),
              m_multipart.upload_id.c_str());

          m_object->m_bucket->abort_multipart_upload(m_multipart);
        } catch (const mysqlshdk::rest::Response_error &inner_error) {
          log_error(
              "Error cancelling multipart upload after failure uploading "
              "part, "
              "error %s\nobject: %s\n upload id: %s",
              inner_error.format().c_str(), m_multipart.name.c_str(),
              m_multipart.upload_id.c_str());
        }

        throw shcore::Exception::runtime_error(error.format());
      }

      incoming_offset += MY_MAX_PART_SIZE;
      to_send -= MY_MAX_PART_SIZE;
    }
  }

  // REMAINING DATA: gets buffered again
  size_t remaining_input = length - incoming_offset;
  if (remaining_input)
    m_buffer.append(incoming + incoming_offset, remaining_input);

  m_size += length;

  return length;
}

void Object::Writer::close() {
  if (m_is_multipart) {
    // MULTIPART UPLOAD STARTED: Sends last part if any and commits the upload
    try {
      if (!m_buffer.empty()) {
        m_parts.push_back(m_object->m_bucket->upload_part(
            m_multipart, m_parts.size() + 1, m_buffer.data(), m_buffer.size()));
      }

      m_object->m_bucket->commit_multipart_upload(m_multipart, m_parts);
    } catch (const mysqlshdk::rest::Response_error &error) {
      try {
        log_info(
            "Cancelling multipart upload after failure completing the "
            "upload, error %s\nobject: %s\n upload id: %s",
            error.format().c_str(), m_multipart.name.c_str(),
            m_multipart.upload_id.c_str());

        m_object->m_bucket->abort_multipart_upload(m_multipart);
      } catch (const mysqlshdk::rest::Response_error &inner_error) {
        log_error(
            "Error cancelling multipart upload after failure completing the "
            "upload, error %s\nobject: %s\n upload id: %s",
            inner_error.format().c_str(), m_multipart.name.c_str(),
            m_multipart.upload_id.c_str());
      }

      throw shcore::Exception::runtime_error(error.format());
    }

  } else {
    // NO UPLOAD STARTED: Sends whatever buffered data in a single PUT
    try {
      m_object->m_bucket->put_object(m_object->full_path(), m_buffer.data(),
                                     m_buffer.size());
    } catch (const Response_error &error) {
      throw shcore::Exception::runtime_error(error.format());
    }
  }
}

Object::Reader::Reader(Object *owner) : File_handler(owner), m_offset(0) {
  try {
    m_size = m_object->m_bucket->head_object(m_object->full_path());
  } catch (const Response_error &error) {
    if (error.code() == Response::Status_code::NOT_FOUND) {
      // For Not Found generates a custom message as the one for the get()
      // doesn't make much sense
      throw shcore::Exception::runtime_error(
          "Failed opening object '" + m_object->m_name +
          "' in READ mode: " + Response_error(error.code()).format());
    } else {
      throw shcore::Exception::runtime_error(error.format());
    }
  }
}

off64_t Object::Reader::seek(off64_t offset) {
  const off64_t fsize = m_size;
  m_offset = std::min(offset, fsize);
  return m_offset;
}

off64_t Object::Reader::tell() const {
  throw std::logic_error("Object::Reader::tell() - not implemented");
}

ssize_t Object::Reader::read(void *buffer, size_t length) {
  const size_t first = m_offset;
  const size_t last_unbounded = m_offset + length - 1;
  const off64_t fsize = m_size;

  if (m_offset >= fsize) return 0;

  const size_t last = std::min(m_size - 1, last_unbounded);

  // Creates a response buffer that writes data directly to buffer
  mysqlshdk::rest::Static_char_ref_buffer rbuffer(
      reinterpret_cast<char *>(buffer), length);

  size_t read = 0;
  try {
    read = m_object->m_bucket->get_object(m_object->full_path(), &rbuffer,
                                          first, last);
  } catch (const Response_error &error) {
    throw shcore::Exception::runtime_error(error.format());
  }

  m_offset += read;

  return read;
}

}  // namespace oci
}  // namespace backend
}  // namespace storage
}  // namespace mysqlshdk
