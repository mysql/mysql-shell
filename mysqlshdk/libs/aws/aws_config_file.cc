/*
 * Copyright (c) 2022, 2025, Oracle and/or its affiliates.
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
#include <string_view>
#include <utility>

#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace aws {

namespace {

constexpr std::string_view k_profile_prefix = "profile ";
constexpr std::string_view k_default_profile = "default";

}  // namespace

Aws_config_file::Aws_config_file(const std::string &path) : m_path(path) {}

std::optional<Profiles> Aws_config_file::load(
    bool require_profile_prefix) const {
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
  // profile to hold settings of sections which are ignored (not prefixed with
  // k_profile_prefix when require_profile_prefix is true)
  Profile ignored_section{"ignored-section"};

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
        auto section = shcore::str_strip(line.substr(1, end - 1));

        if (!require_profile_prefix || k_default_profile == section ||
            shcore::str_beginswith(section, k_profile_prefix)) {
          // if we require a profile prefix, then section name is either
          // 'default' or is prefixed with 'profile '; given that the former is
          // shorter than the latter, if section name is longer than 'default',
          // then it's prefixed, and prefix has to be removed
          static_assert(k_profile_prefix.length() > k_default_profile.length());

          if (require_profile_prefix &&
              section.length() > k_default_profile.length()) {
            section = section.substr(k_profile_prefix.length());
          }

          current_profile = profiles.get_or_create(section);
        } else {
          current_profile = &ignored_section;
        }
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
