/*
 * Copyright (c) 2019, 2024, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/rest/authentication.h"

#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlshdk {
namespace rest {

Authentication::Authentication(std::string username,
                               std::string password) noexcept
    : m_username(std::move(username)), m_password(std::move(password)) {}

Authentication::~Authentication() {
  shcore::clear_buffer(m_username);
  shcore::clear_buffer(m_password);
}

const std::string &Authentication::username() const noexcept {
  return m_username;
}

const std::string &Authentication::password() const noexcept {
  return m_password;
}

Basic_authentication::Basic_authentication(std::string username,
                                           std::string password) noexcept
    : Authentication(std::move(username), std::move(password)) {}

}  // namespace rest
}  // namespace mysqlshdk
