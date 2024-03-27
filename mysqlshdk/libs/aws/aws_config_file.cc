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

std::optional<Profiles> Aws_config_file::load() const {
  const auto &path =
#ifdef _WIN32
      shcore::utf8_to_wide
#endif  // _WIN32
      (m_path);
  std::ifstream config(path);

  if (!config.is_open()) {
    return {};
  }

  Profiles profiles;
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

        current_profile = profiles.get_or_create(profile);
      } else {
        fail("missing closing square bracket");
      }
    } else if (current_profile) {
      const auto equals = line.find('=');

      if (std::string::npos != equals) {
        auto key = shcore::str_strip(line.substr(0, equals));
        auto value = shcore::str_strip(line.substr(equals + 1));

        current_profile->set(std::move(key), std::move(value));
      } else {
        fail("expected setting-name=value");
      }
    } else {
      fail("setting without an associated profile");
    }
  }

  config.close();

  return profiles;
}

}  // namespace aws
}  // namespace mysqlshdk
