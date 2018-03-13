/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_LIBS_SECRET_STORE_API_HELPER_INVOKER_H_
#define MYSQLSHDK_LIBS_SECRET_STORE_API_HELPER_INVOKER_H_

#include <string>

#include "mysql-secret-store/include/mysql-secret-store/api.h"

namespace mysql {
namespace secret_store {
namespace api {

class Helper_invoker {
 public:
  explicit Helper_invoker(const Helper_name &name);

  Helper_name name() const noexcept { return m_name; }

  bool store(const std::string &input) const;

  bool store(const std::string &input, std::string *output) const;

  bool get(const std::string &input, std::string *output) const;

  bool erase(const std::string &input) const;

  bool erase(const std::string &input, std::string *output) const;

  bool list(std::string *output) const;

  bool version() const;

  bool version(std::string *output) const;

 private:
  bool invoke(const char *command, const std::string &input,
              std::string *output) const;
  Helper_name m_name;
};

}  // namespace api
}  // namespace secret_store
}  // namespace mysql

#endif  // MYSQLSHDK_LIBS_SECRET_STORE_API_HELPER_INVOKER_H_
