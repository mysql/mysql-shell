/*
 * Copyright (c) 2017, 2024, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/utils/version.h"

#include <algorithm>
#include <cassert>
#include <charconv>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>

#include <iostream>

#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace utils {

// Minimal implementation of version parsing, no need for something more complex
// for now
Version::Version(std::string_view version) {
  if (version.empty()) return;

  auto tokens = shcore::str_split(version, "-", 1);
  if (tokens.size() == 1 && version.size() == 5) {
    // check if format is digits only:
    bool invalid_format = false;
    for (const auto &it : version) {
      if (!isdigit(it)) {
        invalid_format = true;
        break;
      }
    }
    if (!invalid_format) {
      // it seems the format is digits only, we need to split by hand
      _major = parse_token(version.substr(0, 1));
      _minor = parse_token(version.substr(1, 2));
      _patch = parse_token(version.substr(3, 2));
      return;
    }
  }

  if (tokens.size() == 2) _extra = tokens[1];
  auto base_tokens = shcore::str_split(tokens[0], ".");

  switch (base_tokens.size()) {
    case 3:
      _patch = parse_token(base_tokens[2]);
      [[fallthrough]];
    case 2:
      _minor = parse_token(base_tokens[1]);
      [[fallthrough]];
    case 1:
      _major = parse_token(base_tokens[0]);
      break;
    default:
      throw std::invalid_argument(
          "Invalid version format: 1 to 3 version "
          "numbers expected");
  }
}

int Version::parse_token(std::string_view data) {
  int value = 0;

  const auto end = data.data() + data.length();
  const auto result = std::from_chars(data.data(), end, value);

  std::string error;

  if (std::errc() == result.ec) {
    // success
    if (end != result.ptr) {
      error = "Only digits allowed for version numbers";
    }
  } else {
    switch (result.ec) {
      case std::errc::invalid_argument:
        error = "Not an integer";
        break;

      case std::errc::result_out_of_range:
        error = "Out of integer range";
        break;

      default:
        // should not happen, std::from_chars() returns two errors handled above
        error = "Unknown error";
        break;
    }
  }

  if (!error.empty())
    throw std::invalid_argument("Error parsing version: " + error);

  return value;
}

std::string Version::get_short() const {
  std::string ret_val = std::to_string(_major);

  if (_minor) {
    ret_val.append(".").append(std::to_string(*_minor));
  }

  return ret_val;
}

std::string Version::get_base() const {
  std::string ret_val = get_short();

  if (_minor && _patch) {
    ret_val.append(".").append(std::to_string(*_patch));
  }

  return ret_val;
}

std::string Version::get_full() const {
  std::string ret_val = get_base();

  if (_extra) ret_val.append("-").append(*_extra);

  return ret_val;
}

uint32_t Version::numeric() const {
  return get_major() * 10000 + get_minor() * 100 + get_patch();
}

bool Version::is_mds() const {
  if (!_extra.has_value()) {
    return false;
  }

  return shcore::str_endswith(*_extra, "cloud");
}

bool Version::operator<(const Version &other) const {
  return _major < other._major ||
         (_major == other._major && (get_minor() < other.get_minor() ||
                                     (get_minor() == other.get_minor() &&
                                      get_patch() < other.get_patch())));
}

bool Version::operator<=(const Version &other) const {
  return *this < other ||
         (_major == other._major && get_minor() == other.get_minor() &&
          get_patch() == other.get_patch());
}
bool Version::operator>(const Version &other) const {
  return !(*this <= other);
}

bool Version::operator>=(const Version &other) const {
  return !(*this < other);
}

bool Version::operator==(const Version &other) const {
  return _major == other._major && get_minor() == other.get_minor() &&
         get_patch() == other.get_patch();
}

bool Version::operator!=(const Version &other) const {
  return !(*this == other);
}

Version::operator bool() const {
  return !(get_major() == 0 && get_minor() == 0 && get_patch() == 0);
}

int major_version_difference(const Version &source, const Version &target) {
  const auto major_version = [](const Version &v) {
    // we pretend that version 5.7 is 7 to simplify the code
    const auto m = v.get_major();
    return 5 == m && 7 == v.get_minor() ? 7 : m;
  };

  return major_version(target) - major_version(source);
}

std::vector<Version> corresponding_versions(Version version) {
  // first innovation release
  assert(version >= Version(8, 1, 0));
  // LTS releases are 8.4.0, then X.7.0
  assert((version.get_major() == 8 && version.get_minor() <= 4) ||
         (version.get_major() > 8 && version.get_minor() <= 7));

  // number of releases so far
  int releases = (version.get_major() == 8)
                     ? version.get_minor() +
                           (version.get_minor() < 4 ? 0 : version.get_patch())
                     : 1 + version.get_minor() +
                           (version.get_minor() < 7 ? 0 : version.get_patch());

  std::vector<Version> result;
  result.emplace_back(std::move(version));

  while (true) {
    const auto &prev = result.back();

    if (prev.get_major() == 8) {
      result.emplace_back(8, 0, 33 + releases);
      break;
    } else if (prev.get_major() == 9) {
      result.emplace_back(8, 4, releases);
      // three innovation releases + LTS
      releases += 4;
    } else {
      result.emplace_back(prev.get_major() - 1, 7, releases);
      // seven innovation releases + LTS
      releases += 8;
    }
  }

  std::reverse(result.begin(), result.end());
  return result;
}

Version get_first_lts_version(Version version) {
  // This function should not be called with versions lower than 8.0.0
  assert(version >= Version(8, 0, 0));

  auto major = version.get_major();
  return major == 8 ? Version(8, 4, 0) : Version(major, 7, 0);
}

Version get_first_innovation_version(Version version) {
  // This function should not be called with versions lower than 8.0.0
  assert(version >= Version(8, 0, 0));

  auto major = version.get_major();
  return major == 8 ? Version(8, 1, 0) : Version(major, 0, 0);
}

}  // namespace utils
}  // namespace mysqlshdk
