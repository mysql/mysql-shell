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

#include "mysqlshdk/libs/aws/env_credentials_provider.h"

#include <utility>

#include "mysqlshdk/libs/utils/utils_general.h"

#include "mysqlshdk/libs/aws/aws_settings.h"

namespace mysqlshdk {
namespace aws {

Env_credentials_provider::Env_credentials_provider()
    : Aws_credentials_provider({"environment variables", "AWS_ACCESS_KEY_ID",
                                "AWS_SECRET_ACCESS_KEY"}) {}

Aws_credentials_provider::Credentials
Env_credentials_provider::fetch_credentials() {
  Credentials creds;

  if (auto value = shcore::get_env(access_key_id()); value.has_value()) {
    creds.access_key_id = std::move(value);
  }

  if (auto value = shcore::get_env(secret_access_key()); value.has_value()) {
    creds.secret_access_key = std::move(value);
  }

  if (auto value = shcore::get_env("AWS_SESSION_TOKEN"); value.has_value()) {
    creds.session_token = std::move(value);
  }

  return creds;
}

}  // namespace aws
}  // namespace mysqlshdk
