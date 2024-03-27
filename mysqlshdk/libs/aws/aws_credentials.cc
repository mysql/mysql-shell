/*
 * Copyright (c) 2022, 2024, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/aws/aws_credentials.h"

#include <utility>

namespace mysqlshdk {
namespace aws {

Aws_credentials::Aws_credentials(std::string access_key_id,
                                 std::string secret_access_key,
                                 std::string session_token,
                                 Time_point expiration)
    : Credentials(expiration),
      m_access_key_id(std::move(access_key_id)),
      m_secret_access_key(std::move(secret_access_key)),
      m_session_token(std::move(session_token)) {}

}  // namespace aws
}  // namespace mysqlshdk
