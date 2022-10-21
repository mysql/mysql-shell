/*
 * Copyright (c) 2022, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/aws/aws_credentials.h"

namespace mysqlshdk {
namespace aws {

Aws_credentials::Aws_credentials(const std::string &access_key_id,
                                 const std::string &secret_access_key,
                                 const std::string &session_token,
                                 Time_point expiration)
    : m_access_key_id(access_key_id),
      m_secret_access_key(secret_access_key),
      m_session_token(session_token),
      m_expiration(expiration) {}

bool Aws_credentials::operator==(const Aws_credentials &creds) const {
  return m_access_key_id == creds.m_access_key_id &&
         m_secret_access_key == creds.m_secret_access_key &&
         m_session_token == creds.m_session_token &&
         m_expiration == creds.m_expiration;
}

bool Aws_credentials::operator!=(const Aws_credentials &creds) const {
  return !(*this == creds);
}

}  // namespace aws
}  // namespace mysqlshdk
