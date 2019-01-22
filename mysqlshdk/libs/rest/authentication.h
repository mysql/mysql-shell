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

#ifndef MYSQLSHDK_LIBS_REST_AUTHENTICATION_H_
#define MYSQLSHDK_LIBS_REST_AUTHENTICATION_H_

#include <string>

namespace mysqlshdk {
namespace rest {

/**
 * Base class for credentials management, stores authentication details.
 */
class Authentication {
 public:
  virtual ~Authentication();

  /**
   * Provides access to the username used for authentication.
   *
   * @returns The username.
   */
  const std::string &username() const noexcept;

  /**
   * Provides to the password used for authentication.
   *
   * @returns The password.
   */
  const std::string &password() const noexcept;

 protected:
  /**
   * Stores username and password used to authenticate HTTP connection.
   * @param username Name of the user.
   * @param password Password of the specified user.
   */
  Authentication(const std::string &username, const std::string &password);

 private:
  std::string m_username;
  std::string m_password;
};

/**
 * Holds credentials of HTTP Basic authentication.
 */
class Basic_authentication : public Authentication {
 public:
  Basic_authentication(const std::string &username,
                       const std::string &password);
};

}  // namespace rest
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_REST_AUTHENTICATION_H_
