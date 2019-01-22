/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/libs/rest/authentication.h"

#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlshdk {
namespace rest {

Authentication::Authentication(const std::string &username,
                               const std::string &password)
    : m_username(username), m_password(password) {}

Authentication::~Authentication() {
  shcore::clear_buffer(&m_username[0], m_username.length());
  shcore::clear_buffer(&m_password[0], m_password.length());
}

const std::string &Authentication::username() const noexcept {
  return m_username;
}

const std::string &Authentication::password() const noexcept {
  return m_password;
}

Basic_authentication::Basic_authentication(const std::string &username,
                                           const std::string &password)
    : Authentication(username, password) {}

}  // namespace rest
}  // namespace mysqlshdk
