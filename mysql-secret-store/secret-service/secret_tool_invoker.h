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

#ifndef MYSQL_SECRET_STORE_SECRET_SERVICE_SECRET_TOOL_INVOKER_H_
#define MYSQL_SECRET_STORE_SECRET_SERVICE_SECRET_TOOL_INVOKER_H_

#include <string>
#include <unordered_map>
#include <vector>

namespace mysql {
namespace secret_store {
namespace secret_service {

class Secret_tool_invoker {
 public:
  using Attributes = std::unordered_map<std::string, std::string>;

  void store(const std::string &, const Attributes &, const std::string &);

  std::string get(const Attributes &);

  void erase(const Attributes &);

  std::string list(const Attributes &);

 private:
  std::string invoke(const std::vector<std::string> &args,
                     bool has_password = false,
                     const std::string &password = "") const;
};

}  // namespace secret_service
}  // namespace secret_store
}  // namespace mysql

#endif  // MYSQL_SECRET_STORE_SECRET_SERVICE_SECRET_TOOL_INVOKER_H_
