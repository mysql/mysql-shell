/*
 * Copyright (c) 2019, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "mysqlshdk/libs/storage/backend/object_storage.h"

#include <iterator>

#include "mysqlshdk/libs/rest/error_codes.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlshdk {
namespace storage {
namespace backend {
namespace object_storage {

Directory::Directory(const Config_ptr &config, const std::string &name)
    : m_name(name),
      m_prefix(m_name.empty() ? "" : m_name + "/"),
      m_container(config->container()),
      m_created(false) {}

bool Directory::exists() const {
  try {
    const auto objects = m_container->list_objects(m_prefix, 1, false);

    // an empty prefix represents the root directory, in that case we've just
    // checked that connection can be established
    if (!objects.empty() || m_prefix.empty() || m_created) {
      return true;
    }
  } catch (const rest::Response_error &error) {
    throw rest::to_exception(error);
  }

  // use multipart uploads as the last resort
  return !list_multipart_uploads().empty();
}

void Directory::create() { m_created = true; }

std::unordered_set<IDirectory::File_info> Directory::list_files(
    bool hidden_files) const {
  std::unordered_set<IDirectory::File_info> files;
  std::vector<Object_details> objects;

  try {
    objects = m_container->list_objects(m_prefix, 0, false);
  } catch (const rest::Response_error &error) {
    throw rest::to_exception(error);
  }

  if (m_prefix.empty()) {
    for (auto &object : objects) {
      files.emplace(std::move(object.name), object.size);
    }
  } else {
    for (const auto &object : objects) {
      files.emplace(object.name.substr(m_prefix.size()), object.size);
    }
  }

  if (hidden_files) {
    // Active multipart uploads to the target path should be considered files
    auto uploads = list_multipart_uploads();
    files.insert(std::make_move_iterator(uploads.begin()),
                 std::make_move_iterator(uploads.end()));
  }

  return files;
}

std::unordered_set<IDirectory::File_info> Directory::list_multipart_uploads()
    const {
  std::unordered_set<IDirectory::File_info> files;
  std::vector<Multipart_object> uploads;

  try {
    uploads = m_container->list_multipart_uploads();
  } catch (const rest::Response_error &error) {
    throw rest::to_exception(error);
  }

  if (m_prefix.empty()) {
    for (auto &upload : uploads) {
      // Only uploads not having '/' in the name belong to the root folder
      if (upload.name.find("/") == std::string::npos) {
        files.emplace(std::move(upload.name), 0);
      }
    }
  } else {
    for (const auto &upload : uploads) {
      if (shcore::str_beginswith(upload.name, m_prefix)) {
        files.emplace(upload.name.substr(m_prefix.size()), 0);
      }
    }
  }

  return files;
}

std::unordered_set<IDirectory::File_info> Directory::filter_files(
    const std::string &pattern) const {
  std::unordered_set<IDirectory::File_info> files;
  std::vector<Object_details> objects;

  try {
    objects = m_container->list_objects(m_prefix, 0, false);
  } catch (const rest::Response_error &error) {
    throw rest::to_exception(error);
  }

  if (m_prefix.empty()) {
    for (auto &object : objects) {
      if (shcore::match_glob(pattern, object.name)) {
        files.emplace(std::move(object.name), object.size);
      }
    }
  } else {
    for (const auto &object : objects) {
      auto file_name = object.name.substr(m_prefix.size());
      if (!file_name.empty() && shcore::match_glob(pattern, file_name)) {
        files.emplace(std::move(file_name), object.size);
      }
    }
  }

  return files;
}

std::string Directory::join_path(const std::string &a,
                                 const std::string &b) const {
  return a.empty() ? b : a + "/" + b;
}

std::unique_ptr<IFile> Directory::file(const std::string &name,
                                       const File_options &) const {
  return std::make_unique<Object>(m_container->config(), name,
                                  join_path(m_name, ""));
}

Object::Object(const Config_ptr &config, const std::string &name,
               const std::string &prefix)
    : m_name(name),
      m_prefix(prefix),
      m_container(config->container()),
      m_max_part_size(config->part_size()),
      m_writer{},
      m_reader{} {}

void Object::set_max_part_size(size_t new_size) {
  assert(!is_open());
  m_max_part_size = new_size;
}

void Object::open(storage::Mode mode) {
  switch (mode) {
    case Mode::READ:
      m_reader = std::make_unique<Reader>(this);
      break;
    case Mode::APPEND: {
      std::vector<Multipart_object> uploads;

      try {
        uploads = m_container->list_multipart_uploads();
      } catch (const rest::Response_error &error) {
        throw rest::to_exception(error);
      }

      auto object = std::find_if(
          uploads.begin(), uploads.end(),
          [this](const Multipart_object &o) { return o.name == full_path(); });

      // If there no active upload, the APPEND operation will be allowed if
      // the file does not exist
      if (object == uploads.end()) {
        try {
          // Verifies if the file exists, if not, an error will be thrown by
          // head_object()
          m_container->head_object(full_path().real());

          throw std::invalid_argument(
              "Object Storage only supports APPEND mode for in-progress "
              "multipart uploads or new files.");
        } catch (const rest::Response_error &) {
          // If the file did not exist then OK to continue as a new file
          mode = Mode::WRITE;
        }
      }

      const auto object_ptr = uploads.end() == object ? nullptr : &(*object);

      m_writer = std::make_unique<Writer>(this, object_ptr);
      break;
    }
    case Mode::WRITE:
      m_writer = std::make_unique<Writer>(this);
      break;
  }

  m_open_mode = mode;
}

bool Object::is_open() const { return m_open_mode.has_value(); }

void Object::close() {
  if (m_writer) m_writer->close();
  m_open_mode.reset();
  m_writer.reset();
  m_reader.reset();
}

size_t Object::file_size() const {
  if (m_reader) {
    return m_reader->size();
  } else if (m_writer) {
    return m_writer->size();
  } else {
    return m_container->head_object(full_path().real());
  }
}

std::string Object::filename() const { return m_name; }

bool Object::exists() const {
  bool ret_val = true;
  try {
    file_size();
  } catch (const rest::Response_error &error) {
    if (error.status_code() == rest::Response::Status_code::NOT_FOUND)
      ret_val = false;
    else
      throw rest::to_exception(error);
  }

  return ret_val;
}

std::unique_ptr<IDirectory> Object::parent() const {
  // copy the path, otherwise we'll hold a reference to temporary
  const auto path = full_path().real();
  const auto pos = path.find_last_of('/');

  // if the full path does not contain a backslash then the parent directory
  // is the root directory
  return std::make_unique<Directory>(
      m_container->config(),
      std::string::npos == pos ? "" : path.substr(0, pos));
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
    m_container->rename_object(m_prefix + m_name, m_prefix + new_name);
  } catch (const rest::Response_error &error) {
    throw rest::to_exception(error);
  }
  m_name = new_name;
}

void Object::remove() { m_container->delete_object(full_path().real()); }

Object::Writer::Writer(Object *owner, Multipart_object *object)
    : File_handler(owner), m_is_multipart(false) {
  // This is the writer for an already started multipart object
  if (object) {
    m_multipart = *object;
    m_is_multipart = true;

    try {
      m_parts =
          m_object->m_container->list_multipart_uploaded_parts(m_multipart);
    } catch (const rest::Response_error &error) {
      throw rest::to_exception(error);
    }

    for (const auto &part : m_parts) {
      m_size += part.size;
    }
  }
}

Object::Writer::~Writer() {
  // if there's a pending multipart upload (in other words multipart upload was
  // started, but close() was not called before writer has been destroyed),
  // attempt to cancel it
  abort_multipart_upload("unexpected inner state");
}

off64_t Object::Writer::seek(off64_t /*offset*/) { return 0; }

off64_t Object::Writer::tell() const { return size(); }

ssize_t Object::Writer::write(const void *buffer, size_t length) {
  const size_t MY_MAX_PART_SIZE = m_object->m_max_part_size;
  size_t to_send = m_buffer.size() + length;

  // Initializes the multipart as soon as FILE_PART_SIZE data is provided
  if (!m_is_multipart && to_send > MY_MAX_PART_SIZE) {
    try {
      m_multipart = m_object->m_container->create_multipart_upload(
          m_object->full_path().real());
    } catch (const rest::Response_error &error) {
      throw rest::to_exception(error);
    }

    m_is_multipart = true;
  }

  size_t incoming_offset = 0;
  auto incoming = reinterpret_cast<char *>(const_cast<void *>(buffer));

  // This loops handles the upload of N number of chunks of size
  // MY_MAX_PART_SIZE including the buffered data and the incoming data
  while (to_send > MY_MAX_PART_SIZE) {
    const char *part = nullptr;

    if (!m_buffer.empty()) {
      // BUFFERED DATA: fills the buffer and sends it
      const auto buffer_space = MY_MAX_PART_SIZE - m_buffer.size();
      m_buffer.append(incoming + incoming_offset, buffer_space);

      part = m_buffer.data();
      incoming_offset += buffer_space;
    } else {
      // NO BUFFERED DATA: sends the data directly from the incoming buffer
      part = incoming + incoming_offset;
      incoming_offset += MY_MAX_PART_SIZE;
    }

    try {
      m_parts.push_back(m_object->m_container->upload_part(
          m_multipart, m_parts.size() + 1, part, MY_MAX_PART_SIZE));
    } catch (const rest::Response_error &error) {
      abort_multipart_upload("failure uploading part", error.format());
      throw rest::to_exception(error);
    }

    m_buffer.clear();
    to_send -= MY_MAX_PART_SIZE;
  }

  // REMAINING DATA: gets buffered again
  const auto remaining_input = length - incoming_offset;
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
        m_parts.push_back(m_object->m_container->upload_part(
            m_multipart, m_parts.size() + 1, m_buffer.data(), m_buffer.size()));
      }

      m_object->m_container->commit_multipart_upload(m_multipart, m_parts);
    } catch (const rest::Response_error &error) {
      abort_multipart_upload("failure completing the upload", error.format());
      throw rest::to_exception(error);
    } catch (const rest::Connection_error &error) {
      throw shcore::Exception::runtime_error(error.what());
    }

  } else {
    // NO UPLOAD STARTED: Sends whatever buffered data in a single PUT
    try {
      if (!m_buffer.empty()) {
        m_object->m_container->put_object(m_object->full_path().real(),
                                          m_buffer.data(), m_buffer.size());
      }
    } catch (const rest::Response_error &error) {
      throw rest::to_exception(error);
    } catch (const rest::Connection_error &error) {
      throw shcore::Exception::runtime_error(error.what());
    }
  }

  reset();
}

void Object::Writer::reset() {
  // clean up
  m_is_multipart = false;
  m_buffer.clear();
  m_parts.clear();
}

void Object::Writer::abort_multipart_upload(const char *context,
                                            const std::string &error) {
  if (m_is_multipart) {
    std::string maybe_error;

    if (!error.empty()) {
      maybe_error = ", error: ";
      maybe_error += error;
    }

    log_info(
        "Cancelling multipart upload after %s%s\nobject: %s\nupload id: %s",
        context, maybe_error.c_str(), m_multipart.name.c_str(),
        m_multipart.upload_id.c_str());

    // call reset() before aborting the upload, if abort throws it's not going
    // to be attempted again
    reset();

    try {
      m_object->m_container->abort_multipart_upload(m_multipart);
    } catch (const rest::Response_error &inner_error) {
      log_error(
          "Error cancelling multipart upload after %s, error: %s\nobject: "
          "%s\nupload id: %s",
          context, inner_error.format().c_str(), m_multipart.name.c_str(),
          m_multipart.upload_id.c_str());
    }
  }
}

Object::Reader::Reader(Object *owner) : File_handler(owner), m_offset(0) {
  try {
    m_size = m_object->m_container->head_object(m_object->full_path().real());
  } catch (const rest::Response_error &error) {
    std::string prefix;

    if (error.status_code() == rest::Response::Status_code::NOT_FOUND) {
      // For Not Found generates a custom message as the one for the get()
      // doesn't make much sense
      prefix = "Failed opening object '" + m_object->full_path().masked() +
               "' in READ mode: ";
    }

    throw rest::to_exception(error, prefix);
  } catch (const rest::Connection_error &error) {
    throw shcore::Exception::runtime_error(error.what());
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
  rest::Static_char_ref_buffer rbuffer(reinterpret_cast<char *>(buffer),
                                       length);

  size_t read = 0;
  try {
    read = m_object->m_container->get_object(m_object->full_path().real(),
                                             &rbuffer, first, last);
  } catch (const rest::Response_error &error) {
    throw rest::to_exception(error);
  }

  m_offset += read;

  return read;
}

}  // namespace object_storage
}  // namespace backend
}  // namespace storage
}  // namespace mysqlshdk
