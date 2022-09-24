/*
 * Copyright (c) 2022, Oracle and/or its affiliates.
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
#include "mysqlshdk/libs/db/file_uri.h"
#include "mysqlshdk/libs/db/uri_encoder.h"
#include "mysqlshdk/libs/db/uri_parser.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace db {
namespace uri {

File_uri::File_uri(const std::string &uri) : Uri_serializable({"file"}) {
  if (!uri.empty()) {
    Uri_parser parser(Type::File);
    parser.parse(uri, this);
  }
}

void File_uri::set(const std::string &name, const std::string &value) {
  if (name == db::kScheme) {
    scheme = value;
  } else if (name == db::kHost) {
    host = value;
  } else if (name == db::kPath) {
    path = value;
  } else {
    throw std::invalid_argument(
        shcore::str_format("Invalid component in file URI: %s", name.c_str()));
  }
}

void File_uri::set(const std::string &name, int /*value*/) {
  throw std::invalid_argument(
      shcore::str_format("Invalid component in file URI: %s", name.c_str()));
}

void File_uri::set(const std::string &name,
                   const std::vector<std::string> & /*values*/) {
  throw std::invalid_argument(
      shcore::str_format("Invalid component in file URI: %s", name.c_str()));
}

bool File_uri::has_value(const std::string &name) const {
  if (name == db::kScheme) return !scheme.empty();
  if (name == db::kHost) return !host.empty();
  if (name == db::kPath) return !path.empty();

  return false;
}

const std::string &File_uri::get(const std::string &name) const {
  if (name == db::kScheme) return scheme;
  if (name == db::kHost) return host;
  if (name == db::kPath) return path;

  throw std::invalid_argument(
      shcore::str_format("Invalid component in file URI: %s", name.c_str()));
}

}  // namespace uri
}  // namespace db
}  // namespace mysqlshdk