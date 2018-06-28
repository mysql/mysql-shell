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

#ifndef MYSQL_SECRET_STORE_LOGIN_PATH_LOGIN_PATH_HELPER_H_
#define MYSQL_SECRET_STORE_LOGIN_PATH_LOGIN_PATH_HELPER_H_

#include <string>
#include <vector>

#include "mysql-secret-store/include/helper.h"

#include "mysql-secret-store/login-path/config_editor_invoker.h"
#include "mysql-secret-store/login-path/entry.h"

namespace mysql {
namespace secret_store {
namespace login_path {

class Login_path_helper : public common::Helper {
 public:
  Login_path_helper();

  void check_requirements() override;

  void store(const common::Secret &) override;

  void get(const common::Secret_id &, std::string *) override;

  void erase(const common::Secret_id &) override;

  void list(std::vector<common::Secret_id> *) override;

 private:
  std::vector<Entry> list();

  Entry find(const common::Secret_id &);

  Entry to_entry(const common::Secret_id &) const;

  std::string load(const Entry &);

  Config_editor_invoker m_invoker;
};

}  // namespace login_path
}  // namespace secret_store
}  // namespace mysql

#endif  // MYSQL_SECRET_STORE_LOGIN_PATH_LOGIN_PATH_HELPER_H_
