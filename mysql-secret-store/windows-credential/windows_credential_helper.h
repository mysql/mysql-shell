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

#ifndef MYSQL_SECRET_STORE_WINDOWS_CREDENTIAL_WINDOWS_CREDENTIAL_HELPER_H_
#define MYSQL_SECRET_STORE_WINDOWS_CREDENTIAL_WINDOWS_CREDENTIAL_HELPER_H_

#include <Windows.h>

#include <string>
#include <vector>

#include "mysql-secret-store/include/helper.h"

namespace mysql {
namespace secret_store {
namespace windows_credential {

class Windows_credential_helper : public common::Helper {
 public:
  Windows_credential_helper();

  void check_requirements() override;

  void store(const common::Secret &) override;

  void get(const common::Secret_id &, std::string *) override;

  void erase(const common::Secret_id &) override;

  void list(std::vector<common::Secret_id> *) override;

 private:
  std::string get_name_prefix() const;

  std::string get_name(const common::Secret_id &) const;

  std::string get_search_pattern() const;

  std::string get_keyword_prefix() const;

  std::string get_url_keyword() const;

  std::string get_secret_type_keyword() const;

  DWORD m_persist_mode;
};

}  // namespace windows_credential
}  // namespace secret_store
}  // namespace mysql

#endif  // MYSQL_SECRET_STORE_WINDOWS_CREDENTIAL_WINDOWS_CREDENTIAL_HELPER_H_
