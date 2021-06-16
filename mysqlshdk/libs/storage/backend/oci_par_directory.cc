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

namespace mysqlshdk {
namespace storage {
namespace backend {
namespace oci {

Oci_par_directory::Oci_par_directory(const std::string &url)
    : Http_directory(true) {
  if (parse_par(url, &m_par_data) != Par_type::PREFIX) {
    throw std::runtime_error("Invalid prefix PAR");
  }

  init_rest(m_par_data.get_par_url(), true);
}

Oci_par_directory::Oci_par_directory(const Par_structure &par)
    : m_par_data(par) {
  init_rest(m_par_data.get_par_url(), true);
}

std::string Oci_par_directory::full_path() const { return m_par_data.full_url; }

std::string Oci_par_directory::get_list_url() const {
  return "?fields=name,size";
}

std::vector<IDirectory::File_info> Oci_par_directory::parse_file_list(
    const std::string &data, const std::string &pattern) const {
  std::vector<IDirectory::File_info> list;

  shcore::Dictionary_t response =
      shcore::Value::parse(data.data(), data.size()).as_map();
  shcore::Array_t objects = response->get_array("objects");

  for (const auto &object : *objects) {
    auto file = object.as_map();
    auto name =
        file->get_string("name").substr(m_par_data.object_prefix.size());
    if (pattern.empty() || shcore::match_glob(pattern, name)) {
      list.push_back({name, static_cast<size_t>(file->get_uint("size"))});
    }
  }

  return list;
}

}  // namespace oci
}  // namespace backend
}  // namespace storage
}  // namespace mysqlshdk
