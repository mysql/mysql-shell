/*
 * Copyright (c) 2022, 2024, Oracle and/or its affiliates.
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
#include "mysqlshdk/libs/db/generic_uri.h"
#include "mysqlshdk/libs/db/uri_encoder.h"
#include "mysqlshdk/libs/db/uri_parser.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace db {
namespace uri {

Generic_uri::Generic_uri(const std::string &uri) : Uri_serializable({}) {
  if (!uri.empty()) {
    Uri_parser parser(Type::Generic);
    parser.parse(uri, this);
  }
}

void Generic_uri::set(const std::string &name, const std::string &value) {
  if (name == db::kScheme) {
    scheme = value;
  } else if (name == db::kUser) {
    user = value;
  } else if (name == db::kPassword) {
    password = value;
  } else if (name == db::kHost) {
    host = value;
  } else if (name == db::kPath) {
    path = value;
  } else {
    query.push_back({name, value});
  }
}

void Generic_uri::set(const std::string &name, int value) {
  if (name == db::kPort) {
    port = value;
  } else {
    set(name, std::to_string(value));
  }
}

void Generic_uri::set(const std::string &name,
                      const std::vector<std::string> &values) {
  query.push_back({name, shcore::str_join(values, ",")});
}

bool Generic_uri::has_value(const std::string &name) const {
  if (name == db::kScheme) return !scheme.empty();
  if (name == db::kUser) return !user.empty();
  if (name == db::kPassword) return password.has_value();
  if (name == db::kHost) return !host.empty();
  if (name == db::kPort) return port.has_value();
  if (name == db::kPath) return !path.empty();

  const auto &item = std::find_if(
      query.begin(), query.end(),
      [&name](
          const std::pair<std::string, std::optional<std::string>> &member) {
        return member.first == name;
      });

  return item != query.end();
}

const std::string &Generic_uri::get(const std::string &name) const {
  if (name == db::kScheme) return scheme;
  if (name == db::kUser) return user;
  if (name == db::kPassword) return *password;
  if (name == db::kHost) return host;
  if (name == db::kPath) return path;

  const auto &item = std::find_if(
      query.begin(), query.end(),
      [&name](
          const std::pair<std::string, std::optional<std::string>> &member) {
        return member.first == name;
      });

  if (item == query.end())
    throw std::invalid_argument(
        shcore::str_format("Invalid URI property: %s", name.c_str()));

  return *(item->second);
}

int Generic_uri::get_numeric(const std::string &name) const {
  if (name == db::kPort) {
    if (port.has_value()) return port.value();

    throw std::runtime_error("Port is not defined.");
  }

  throw std::invalid_argument(
      shcore::str_format("Invalid URI property: %s", name.c_str()));
}

std::vector<std::pair<std::string, std::optional<std::string>>>
Generic_uri::query_attributes() const {
  std::vector<std::pair<std::string, std::optional<std::string>>> attributes;
  for (const auto &att : query) {
    if (att.first != db::kSocket) {
      attributes.push_back(att);
    }
  }
  return attributes;
}

}  // namespace uri
}  // namespace db
}  // namespace mysqlshdk