/*
 * Copyright (c) 2017, 2022, Oracle and/or its affiliates.
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
#include "mysqlshdk/libs/db/uri_common.h"
#include "mysqlshdk/libs/utils/nullable_options.h"

#ifndef MYSQLSHDK_LIBS_DB_GENERIC_URI_H_
#define MYSQLSHDK_LIBS_DB_GENERIC_URI_H_

#include <optional>
#include <string>
#include <vector>

namespace mysqlshdk {
namespace db {
namespace uri {

/**
 * @brief Container for the components of a URI (excluding fragment)
 */
struct Generic_uri : public Uri_serializable {
  explicit Generic_uri(const std::string &uri = "");

  Generic_uri(const Generic_uri &) = default;
  Generic_uri(Generic_uri &&) = default;

  Generic_uri &operator=(const Generic_uri &) = default;
  Generic_uri &operator=(Generic_uri &&) = default;

  ~Generic_uri() override = default;

  mysqlshdk::db::uri::Type get_type() const override {
    return mysqlshdk::db::uri::Type::Generic;
  }

  void set(const std::string &name, const std::string &value) override;
  void set(const std::string &name, int value) override;
  void set(const std::string &name,
           const std::vector<std::string> &values) override;

  bool has_value(const std::string &name) const override;
  const std::string &get(const std::string &name) const override;
  int get_numeric(const std::string &name) const override;
  std::vector<std::pair<std::string, mysqlshdk::null_string>> query_attributes()
      const override;

  std::string scheme;
  std::string user;
  std::optional<std::string> password;
  std::string host;
  std::optional<int> port;
  std::string path;
  std::vector<std::pair<std::string, mysqlshdk::null_string>> query;
};

}  // namespace uri
}  // namespace db
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_DB_GENERIC_URI_H_
