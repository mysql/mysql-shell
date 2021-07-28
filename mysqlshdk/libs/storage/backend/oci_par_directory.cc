/*
 * Copyright (c) 2021, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/db/uri_encoder.h"

namespace mysqlshdk {
namespace storage {
namespace backend {
namespace oci {

namespace {

std::string encode_query(const std::string &data) {
  mysqlshdk::db::uri::Uri_encoder encoder;
  return encoder.encode_query(data);
}

}  // namespace

Oci_par_directory::Oci_par_directory(const std::string &url)
    : Http_directory(true) {
  if (parse_par(url, &m_par_data) != Par_type::PREFIX) {
    throw std::runtime_error("Invalid prefix PAR");
  }

  init_rest();
}

Oci_par_directory::Oci_par_directory(const Par_structure &par)
    : m_par_data(par) {
  init_rest();
}

Masked_string Oci_par_directory::full_path() const {
  return ::mysqlshdk::oci::anonymize_par(m_par_data.full_url);
}

std::string Oci_par_directory::get_list_url() const {
  // use delimiter to limit the number of results, we're just interested in the
  // files in the current directory
  std::string url = "?fields=name,size&delimiter=/";

  if (!m_par_data.object_prefix.empty()) {
    url += "&prefix=" + encode_query(m_par_data.object_prefix);
  }

  if (!m_next_start_with.empty()) {
    url += "&start=" + encode_query(m_next_start_with);
  }

  return url;
}

std::vector<IDirectory::File_info> Oci_par_directory::parse_file_list(
    const std::string &data, const std::string &pattern) const {
  std::vector<IDirectory::File_info> list;

  const auto response = shcore::Value::parse(data.data(), data.size()).as_map();
  const auto objects = response->get_array("objects");
  m_next_start_with = response->get_string("nextStartWith", "");
  const auto prefix_length = m_par_data.object_prefix.length();

  list.reserve(objects->size());

  for (const auto &object : *objects) {
    const auto file = object.as_map();
    auto name = file->get_string("name").substr(prefix_length);

    if (pattern.empty() || shcore::match_glob(pattern, name)) {
      list.emplace_back(IDirectory::File_info{
          std::move(name), static_cast<size_t>(file->get_uint("size"))});
    }
  }

  return list;
}

bool Oci_par_directory::is_list_files_complete() const {
  return m_next_start_with.empty();
}

void Oci_par_directory::init_rest() {
  Http_directory::init_rest(
      ::mysqlshdk::oci::anonymize_par(m_par_data.get_par_url()));
}

}  // namespace oci
}  // namespace backend
}  // namespace storage
}  // namespace mysqlshdk
