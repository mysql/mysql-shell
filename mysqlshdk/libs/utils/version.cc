/*
 * Copyright (c) 2014, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "mysqlshdk/libs/utils/version.h"
#include <cctype>
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace utils {

Version::Version() : _major(0) {
}

// Minimal implementation of version parsing, no need for something more complex
// for now
Version::Version(const std::string& version) : _major(0) {
  auto tokens = shcore::str_split(version, "-", 1);

  if (tokens.size() == 2)
    _extra = tokens[1];
  auto base_tokens = shcore::str_split(tokens[0], ".");

  switch (base_tokens.size()) {
    case 3:
      _patch = parse_token(base_tokens[2]);
    case 2:
      _minor = parse_token(base_tokens[1]);
    case 1:
      _major = parse_token(base_tokens[0]);
      break;
    default:
      throw std::invalid_argument(
          "Invalid version format: 1 to 3 version "
          "numbers expected");
  }
}

int Version::parse_token(const std::string& data) {
  size_t idx = 0;
  int value = 0;
  std::string error;
  try {
    value = std::stoi(data, &idx);

    if (idx < data.length())
      error = "Only digits allowed for version numbers";
  } catch (const std::invalid_argument& e) {
    error = e.what();
  } catch (const std::out_of_range& e) {
    error = e.what();
  }

  if (!error.empty())
    throw std::invalid_argument("Error parsing version: " + error);

  return value;
}

std::string Version::get_base() const {
  std::string ret_val = std::to_string(_major);

  if (_minor) {
    ret_val.append(".").append(std::to_string(*_minor));

    if (_patch)
      ret_val.append(".").append(std::to_string(*_patch));
  }

  return ret_val;
}

std::string Version::get_full() const {
  std::string ret_val = get_base();

  if (_extra)
    ret_val.append("-" + *_extra);

  return ret_val;
}

bool Version::operator<(const Version& other) {
  return _major < other._major ||
         (_major == other._major &&
          (get_minor() < other.get_minor() ||
           (get_minor() == other.get_minor() &&
            get_patch() < other.get_patch())));
}

bool Version::operator<=(const Version& other) {
  return *this < other || (_major == other._major &&
                           get_minor() == other.get_minor() &&
                           get_patch() == other.get_patch());
}
bool Version::operator>(const Version& other) {
  return !(*this <= other);
}

bool Version::operator>=(const Version& other) {
  return !(*this < other);
}

bool Version::operator==(const Version& other) {
  return _major == other._major &&
         get_minor() == other.get_minor() &&
         get_patch() == other.get_patch();
}

bool Version::operator!=(const Version& other) {
  return !(*this == other);
}

}  // namespace utils
}  // namespace mysqlshdk
