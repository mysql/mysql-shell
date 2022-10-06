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

#include "mysqlshdk/libs/aws/env_credentials_provider.h"

#include <cstdlib>

namespace mysqlshdk {
namespace aws {

Env_credentials_provider::Env_credentials_provider()
    : Aws_credentials_provider({"environment variables", "AWS_ACCESS_KEY_ID",
                                "AWS_SECRET_ACCESS_KEY"}) {}

Aws_credentials_provider::Credentials
Env_credentials_provider::fetch_credentials() {
  Credentials creds;

  if (const auto value = ::getenv(access_key_id())) {
    creds.access_key_id = value;
  }

  if (const auto value = ::getenv(secret_access_key())) {
    creds.secret_access_key = value;
  }

  if (const auto value = ::getenv("AWS_SESSION_TOKEN")) {
    creds.session_token = value;
  }

  return creds;
}

}  // namespace aws
}  // namespace mysqlshdk
