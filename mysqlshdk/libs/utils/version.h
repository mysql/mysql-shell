/*
 * Copyright (c) 2017, 2022, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_UTILS_VERSION_H_
#define MYSQLSHDK_LIBS_UTILS_VERSION_H_

#include <cstdint>
#include <optional>
#include <string>

namespace mysqlshdk {
namespace utils {
/**
 * Class to handle versions following the MySQL Standard which is
 * \<major\>.\<minor\>.\<patch\>[-extra]
 */
class Version {
 public:
  constexpr Version() = default;
  explicit Version(const std::string &version);
  constexpr Version(int major, int minor, int patch)
      : _major(major), _minor(minor), _patch(patch) {}

  /**
   * Returns the major version.
   */
  int get_major() const { return _major; }

  /**
   * Returns the minor version, or 0 if not present.
   */
  int get_minor() const { return _minor.value_or(0); }

  /**
   * Returns the patch version, or 0 if not present.
   */
  int get_patch() const { return _patch.value_or(0); }

  /**
   * Returns the extra part of the version string, or an empty string if not
   * present.
   */
  std::string get_extra() const { return _extra.value_or(std::string{}); }

  /**
   * Returns the version string in format \<major\>.\<minor\>.
   */
  std::string get_short() const;

  /**
   * Returns the version string in format \<major\>.\<minor\>.\<patch\>.
   */
  std::string get_base() const;

  /**
   * Returns the version string in format \<major\>.\<minor\>.\<patch\>[-extra].
   */
  std::string get_full() const;

  /**
   * Returns the numeric version: MMmmpp, where M - major, m - minor, p - patch.
   */
  uint32_t numeric() const;

  bool operator<(const Version &other) const;
  bool operator<=(const Version &other) const;
  bool operator>(const Version &other) const;
  bool operator>=(const Version &other) const;
  bool operator==(const Version &other) const;
  bool operator!=(const Version &other) const;
  explicit operator bool() const;

 private:
  int _major = 0;
  std::optional<int> _minor;
  std::optional<int> _patch;
  std::optional<std::string> _extra;

  int parse_token(const std::string &data);
};

}  // namespace utils
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_UTILS_VERSION_H_
