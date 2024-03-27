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

#include "mysqlshdk/libs/aws/config_credentials_provider.h"

#include "mysqlshdk/libs/utils/logger.h"

namespace mysqlshdk {
namespace aws {

namespace {

inline bool is_config(Config_credentials_provider::Type type) {
  return type == Config_credentials_provider::Type::CONFIG;
}

const char *context(Config_credentials_provider::Type type) {
  return is_config(type) ? "config" : "credentials";
}

const std::string &path(const Settings &settings,
                        Config_credentials_provider::Type type) {
  return settings.get_string(is_config(type) ? Setting::CONFIG_FILE
                                             : Setting::CREDENTIALS_FILE);
}

std::string format_name(const Settings &settings, const std::string &profile,
                        Config_credentials_provider::Type type) {
  std::string name;

  name += context(type);
  name += " file (" + path(settings, type);
  name += ", profile: " + profile;
  name += ")";

  return name;
}

}  // namespace

Config_credentials_provider::Config_credentials_provider(
    const Settings &settings, const std::string &profile, Type type)
    : Aws_credentials_provider({format_name(settings, profile, type),
                                "aws_access_key_id", "aws_secret_access_key"}) {
  m_profile = is_config(type) ? settings.config_profile(profile)
                              : settings.credentials_profile(profile);

  if (!m_profile) {
    log_warning("The %s file at '%s' does not contain a profile named: '%s'.",
                context(type), path(settings, type).c_str(), profile.c_str());
  }
}

bool Config_credentials_provider::available() const noexcept {
  return m_profile;
}

Aws_credentials_provider::Credentials
Config_credentials_provider::fetch_credentials() {
  Credentials creds;

  creds.access_key_id = m_profile->access_key_id();
  creds.secret_access_key = m_profile->secret_access_key();
  creds.session_token = m_profile->session_token();

  return creds;
}

}  // namespace aws
}  // namespace mysqlshdk
