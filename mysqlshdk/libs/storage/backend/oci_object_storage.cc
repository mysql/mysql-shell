/*
 * Copyright (c) 2019, 2020, Oracle and/or its affiliates. All rights reserved.
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
#include "mysqlshdk/include/shellcore/console.h"
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

using Status_code = mysqlshdk::rest::Response::Status_code;

using mysqlshdk::oci::Oci_error;
using mysqlshdk::oci::Oci_options;
using mysqlshdk::oci::Oci_rest_service;
using mysqlshdk::oci::Oci_service;

const size_t KIB = 1024;
const size_t MIB = KIB * 1024;
const size_t MAX_PART_SIZE = MIB * 10;
const size_t MAX_FILE_NAME_SIZE = 1024;

Directory::Directory(const Oci_options &options, const std::string &name)
    : m_name(name),
      m_bucket(std::make_unique<Bucket>(options)),
      m_created(false) {}

bool Directory::exists() const {
  if (m_name.empty() || m_created) {
    // An empty name represents the root directory, since the bucket was created
    // OK then it exists. If it has been "created" it exists as well.
    return true;
  } else {
    // If it has a name then we need to make sure it is a directory, it will be
    // verified by listing 1 object using as prefix as '<m_name>/'
    auto objects = m_bucket->list_objects(m_name + "/", "", "", 1, false);
    return !objects.empty();
  }
}

void Directory::create() { m_created = true; }

std::vector<IDirectory::File_info> Directory::list_files() const {
  std::vector<IDirectory::File_info> files;
  auto objects = m_bucket->list_objects(m_name.empty() ? "" : m_name + "/", "",
                                        "", 0, false);

  if (m_name.empty()) {
    for (const auto &object : objects) {
      files.push_back({object.name, object.size});
    }
  } else {
    size_t name_offset = m_name.size() + 1;
    for (const auto &object : objects) {
      files.push_back({object.name.substr(name_offset), object.size});
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

  if (file_name.size() > MAX_FILE_NAME_SIZE) {
    throw std::runtime_error(shcore::str_format(
        "Unable to create file '%s', it exceeds the allowed name length: 1024",
        file_name.c_str()));
  }

  return std::make_unique<Object>(m_bucket->get_options(), name);
}

Object::Object(const Oci_options &options, const std::string &name)
    : m_name(name),
      m_bucket(std::make_unique<Bucket>(options)),
      m_max_part_size(MAX_PART_SIZE),
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
      auto uploads = m_bucket->list_multipart_uploads();
      auto object = std::find_if(
          uploads.begin(), uploads.end(),
          [this](const Multipart_object &o) { return o.name == m_name; });

      if (object == uploads.end()) {
        throw std::runtime_error(
            "Object Storage only supports APPEND mode for in-progress "
            "multipart uploads.");
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
  assert(is_open());

  size_t ret_val = 0;
  if (m_reader)
    ret_val = m_reader->size();
  else if (m_writer)
    ret_val = m_writer->size();

  return ret_val;
}

std::string Object::filename() const { return m_name; }

bool Object::exists() const {
  bool ret_val = true;
  try {
    m_bucket->head_object(m_name);
  } catch (const Oci_error &error) {
    if (error.code() == Status_code::NOT_FOUND)
      ret_val = false;
    else
      throw;
  }

  return ret_val;
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
  m_bucket->rename_object(m_name, new_name);
  m_name = new_name;
}

Object::Writer::Writer(Object *owner, Multipart_object *object)
    : File_handler(owner), m_size(0), m_is_multipart(false) {
  // This is the writer for an already started multipart object
  if (object) {
    m_multipart = *object;
    m_is_multipart = true;

    m_parts = m_object->m_bucket->list_multipart_upload_parts(m_multipart);
    for (const auto &part : m_parts) {
      m_size += part.size;
    }
  }
}

off64_t Object::Writer::seek(off64_t /*offset*/) { return 0; }

ssize_t Object::Writer::write(const void *buffer, size_t length) {
  const size_t MY_MAX_PART_SIZE = m_object->m_max_part_size;
  size_t to_send = m_buffer.size() + length;
  size_t incoming_offset = 0;
  char *incoming = reinterpret_cast<char *>(const_cast<void *>(buffer));

  // Initializes the multipart as soon as FILE_PART_SIZE data is provided
  if (!m_is_multipart && to_send > MY_MAX_PART_SIZE) {
    m_multipart = m_object->m_bucket->create_multipart_upload(m_object->m_name);
    m_is_multipart = true;
  }

  // This loops handles the upload of N number of chunks of size
  // MY_MAX_PART_SIZE including the buffered data and the incoming data
  while (to_send > MY_MAX_PART_SIZE) {
    if (!m_buffer.empty()) {
      // BUFFERED DATA: fills the buffer and sends it
      size_t buffer_space = MY_MAX_PART_SIZE - m_buffer.size();
      m_buffer.append(incoming + incoming_offset, buffer_space);

      m_parts.push_back(m_object->m_bucket->upload_part(
          m_multipart, m_parts.size() + 1, m_buffer.data(), MY_MAX_PART_SIZE));
      m_buffer.clear();
      incoming_offset += buffer_space;
      to_send -= MY_MAX_PART_SIZE;
    } else {
      // NO BUFFERED DATA: sends the data directly from the incoming buffer
      m_parts.push_back(m_object->m_bucket->upload_part(
          m_multipart, m_parts.size() + 1, incoming + incoming_offset,
          MY_MAX_PART_SIZE));
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
    if (!m_buffer.empty()) {
      m_parts.push_back(m_object->m_bucket->upload_part(
          m_multipart, m_parts.size() + 1, m_buffer.data(), m_buffer.size()));
    }

    m_object->m_bucket->commit_multipart_upload(m_multipart, m_parts);
  } else {
    // NO UPLOAD STARTED: Sends whatever buffered data in a single PUT
    m_object->m_bucket->put_object(m_object->m_name, m_buffer.data(),
                                   m_buffer.size());
  }
}

Object::Reader::Reader(Object *owner) : File_handler(owner), m_offset(0) {
  m_size = m_object->m_bucket->head_object(m_object->m_name);
}

off64_t Object::Reader::seek(off64_t offset) {
  const off64_t fsize = m_size;
  m_offset = std::min(offset, fsize);
  return m_offset;
}

ssize_t Object::Reader::read(void *buffer, size_t length) {
  const size_t first = m_offset;
  const size_t last_unbounded = m_offset + length - 1;

  const off64_t fsize = m_size;
  if (m_offset >= fsize) return 0;

  const size_t last = std::min(m_size - 1, last_unbounded);

  size_t read =
      m_object->m_bucket->get_object(m_object->m_name, buffer, first, last);

  m_offset += read;

  return read;
}

}  // namespace oci
}  // namespace backend
}  // namespace storage
}  // namespace mysqlshdk
