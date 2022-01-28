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

#include "mysqlshdk/libs/aws/aws_config_file.h"

#include <fstream>
#include <stdexcept>
#include <utility>

#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace aws {

namespace {

const std::string k_profile_prefix = "profile ";

}  // namespace

Aws_config_file::Aws_config_file(const std::string &path) : m_path(path) {}

bool Aws_config_file::load() {
  const auto &path =
#ifdef _WIN32
      shcore::utf8_to_wide
#endif  // _WIN32
      (m_path);
  std::ifstream config(path);

  if (!config.is_open()) {
    return false;
  }

  m_profiles.clear();
  Profile *current_profile = nullptr;

  // lines which are not a profile name, a key-value pair or a comment result in
  // a failure
  std::string line;
  uint64_t line_number = 0;

  const auto fail = [this, &line, &line_number](const char *reason) {
    throw std::runtime_error("Failed to parse config file '" + m_path +
                             "': " + reason + ", in line " +
                             std::to_string(line_number) + ": " + line);
  };

  while (std::getline(config, line)) {
    ++line_number;

    line = shcore::str_strip(line);

    if (line.empty()) {
      continue;
    }

    // comments
    if ('#' == line[0] || ';' == line[0]) {
      continue;
    }

    if ('[' == line[0]) {
      const auto end = line.find(']');

      if (end != std::string::npos) {
        auto profile = shcore::str_strip(line.substr(1, end - 1));

        // Profiles in the config file have a "profile " prefix (except for the
        // "default" profile). Profiles in the credentials file do not have a
        // prefix, but we do not enforce this (similarly to the AWS SDK).
        if (shcore::str_beginswith(profile, k_profile_prefix)) {
          profile = profile.substr(k_profile_prefix.length());
        }

        current_profile = &m_profiles[profile];
      } else {
        fail("missing closing square bracket");
      }
    } else if (current_profile) {
      const auto equals = line.find('=');

      if (std::string::npos != equals) {
        auto key = shcore::str_strip(line.substr(0, equals));
        auto value = shcore::str_strip(line.substr(equals + 1));

        if ("aws_access_key_id" == key) {
          current_profile->access_key_id = value;
        } else if ("aws_secret_access_key" == key) {
          current_profile->secret_access_key = value;
        } else if ("aws_session_token" == key) {
          current_profile->session_token = value;
        } else {
          current_profile->settings.emplace(std::move(key), std::move(value));
        }
      } else {
        fail("expected setting-name=value");
      }
    } else {
      fail("setting without an associated profile");
    }
  }

  config.close();

  return true;
}

bool Aws_config_file::has_profile(const std::string &name) const {
  return m_profiles.end() != m_profiles.find(name);
}

const Aws_config_file::Profile *Aws_config_file::get_profile(
    const std::string &name) const {
  const auto profile = m_profiles.find(name);
  return m_profiles.end() == profile ? nullptr : &profile->second;
}

}  // namespace aws
}  // namespace mysqlshdk
