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
#ifndef MYSQLSHDK_LIBS_DB_FILE_URI_H_
#define MYSQLSHDK_LIBS_DB_FILE_URI_H_

#include "mysqlshdk/libs/db/uri_common.h"

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace db {
namespace uri {

struct File_uri : public Uri_serializable {
  explicit File_uri(const std::string &uri = "");

  File_uri(const File_uri &) = default;
  File_uri(File_uri &&) = default;

  File_uri &operator=(const File_uri &) = default;
  File_uri &operator=(File_uri &&) = default;

  ~File_uri() override = default;

  mysqlshdk::db::uri::Type get_type() const override {
    return mysqlshdk::db::uri::Type::File;
  }

  void set(const std::string &name, const std::string &value) override;
  void set(const std::string &name, int value) override;
  void set(const std::string &name,
           const std::vector<std::string> &values) override;

  bool has_value(const std::string &name) const override;
  const std::string &get(const std::string &name) const override;
  int get_numeric(const std::string &name) const override {
    throw std::invalid_argument(
        shcore::str_format("Invalid component in file URI: %s", name.c_str()));
  };

  std::vector<std::pair<std::string, std::optional<std::string>>>
  query_attributes() const override {
    return {};
  }

  std::string scheme;
  std::string host;
  std::string path;
};

}  // namespace uri
}  // namespace db
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_DB_FILE_URI_H_
