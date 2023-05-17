/*
 * Copyright (c) 2021, 2023, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/storage/backend/oci_par_directory.h"

#include <algorithm>
#include <filesystem>

#include "mysqlshdk/libs/db/uri_encoder.h"
#include "mysqlshdk/libs/oci/oci_par.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"

namespace mysqlshdk {
namespace storage {
namespace backend {
namespace oci {

using mysqlshdk::db::uri::pctencode_query_value;

size_t Oci_par_file::file_size() const {
  if (Mode::WRITE != m_open_mode) {
    return Http_object::file_size();
  }

  return m_file_size;
}

void Oci_par_file::open(Mode m) {
  Http_object::open(m);

  if (Mode::WRITE == m_open_mode) {
    auto config = std::dynamic_pointer_cast<const mysqlshdk::oci::IPAR_config>(
        m_parent_config);

    m_file_path = shcore::path::join_path(config->temp_folder(), m_path);

    m_file = shcore::create_private_file(m_file_path);
  }
}

void Oci_par_file::set_write_data(Http_request *request) {
  request->file = this;
}

void Oci_par_file::close() {
  assert(is_open());

  if (Mode::WRITE == *m_open_mode) {
    // Resets the file position so it is read from the beginning by the HTTP Put
    // request
    fseek(m_file, 0, SEEK_SET);
  }

  Http_object::close();

  do_close();
}

void Oci_par_file::do_close() {
  if (m_file) {
    std::fclose(m_file);
    m_file = nullptr;
    shcore::delete_file(m_file_path);
    m_file_path.clear();
  }
}

ssize_t Oci_par_file::read(void *buffer, size_t length) {
  assert(is_open());
  if (Mode::READ == *m_open_mode) {
    return Http_object::read(buffer, length);
  } else {
    if (!(length > 0)) return 0;
    const auto fsize = file_size();
    if (m_offset >= static_cast<off64_t>(fsize)) return 0;

    size_t count = fread(buffer, 1, length, m_file);

    m_offset += count;

    return count;
  }
}

ssize_t Oci_par_file::write(const void *buffer, size_t length) {
  assert(is_open() && Mode::WRITE == *m_open_mode);

  size_t written = fwrite(buffer, 1, length, m_file);
  if (length != written) {
    throw std::runtime_error(shcore::str_format(
        "Unexpected error writing %zi bytes to file %s, written: %zi", length,
        this->full_path().masked().c_str(), written));
  }

  m_file_size += length;

  return length;
}

std::unique_ptr<IFile> Oci_par_directory::file(
    const std::string &name, const File_options & /*options*/) const {
  // file may outlive the directory, need to copy the values
  const auto full = full_path();
  auto real = full.real();
  auto masked = full.masked();
  Masked_string copy = {std::move(real), std::move(masked)};
  auto file = std::make_unique<Oci_par_file>(
      copy, db::uri::pctencode_path(name), m_use_retry);
  file->set_parent_config(m_config);
  return file;
}

Oci_par_directory::Oci_par_directory(const Oci_par_directory_config_ptr &config)
    : Http_directory(config, true), m_config(config) {
  init_rest();
}

Masked_string Oci_par_directory::full_path() const {
  return ::mysqlshdk::oci::anonymize_par(m_config->par().full_url());
}

bool Oci_par_directory::exists() const {
  // If list files succeeds, we can say the target exists even if it's a prefix
  // PAR and there are no objects with that prefix as the result will be an
  // empty list, otherwise, an exception describing the issue would be raised
  if (!m_exists.has_value()) {
    list_files();
    m_exists = true;
  }

  return m_exists.value_or(false);
}

void Oci_par_directory::create() { /*NOOP*/
}

std::string Oci_par_directory::get_list_url() const {
  // use delimiter to limit the number of results, we're just interested in the
  // files in the current directory
  std::string url = "?fields=name,size&delimiter=/";

  if (!m_config->par().object_prefix.empty()) {
    url += "&prefix=" + pctencode_query_value(m_config->par().object_prefix);
  }

  if (!m_next_start_with.empty()) {
    url += "&start=" + pctencode_query_value(m_next_start_with);
  }

  return url;
}

std::unordered_set<IDirectory::File_info> Oci_par_directory::parse_file_list(
    const std::string &data, const std::string &pattern) const {
  std::unordered_set<IDirectory::File_info> list;

  const auto response = shcore::Value::parse(data.data(), data.size()).as_map();
  const auto objects = response->get_array("objects");
  m_next_start_with = response->get_string("nextStartWith", "");
  const auto prefix_length = m_config->par().object_prefix.length();

  list.reserve(objects->size());

  for (const auto &object : *objects) {
    const auto file = object.as_map();
    auto name = file->get_string("name").substr(prefix_length);

    // The OCI Console UI allows creating an empty folder in a bucket. A prefix
    // PAR for such folder should be valid as long as the folder does not
    // contain files. On a scenario like this, using the prefix PAR to list the
    // objects will return 1 object named as <prefix>/.
    // Since we are taking as name, everything after the <prefix>/, that means
    // in a case like this, an empty named object will be returned.
    // Such object should be ignored from the listing.
    if (!name.empty() &&
        (pattern.empty() || shcore::match_glob(pattern, name))) {
      list.emplace(std::move(name),
                   static_cast<size_t>(file->get_uint("size")));
    }
  }

  return list;
}

bool Oci_par_directory::is_list_files_complete() const {
  return m_next_start_with.empty();
}

void Oci_par_directory::init_rest() {
  Http_directory::init_rest(
      ::mysqlshdk::oci::anonymize_par(m_config->par().par_url()));
}

}  // namespace oci
}  // namespace backend
}  // namespace storage
}  // namespace mysqlshdk
