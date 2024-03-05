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

#ifndef MYSQLSHDK_LIBS_UTILS_VERSION_H_
#define MYSQLSHDK_LIBS_UTILS_VERSION_H_

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mysqlshdk {
namespace utils {
/**
 * Class to handle versions following the MySQL Standard which is
 * \<major\>.\<minor\>.\<patch\>[-extra]
 */
class Version {
 public:
  constexpr Version() = default;
  explicit Version(std::string_view version);
  explicit constexpr Version(int major, int minor) noexcept
      : _major(major), _minor(minor) {}
  explicit constexpr Version(int major, int minor, int patch) noexcept
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

  /**
   * Returns true if this is an MDS server.
   */
  bool is_mds() const;

  /**
   * Returns the numeric major.minor version series (MMmm)
   */
  uint32_t numeric_version_series() const { return numeric() / 100; }

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

  int parse_token(std::string_view data);
};

inline const Version k_shell_version = Version(MYSH_VERSION);

/**
 * Provides the difference between major versions of source and target. This
 * difference is positive if target has greater version than source.
 *
 * NOTE: for sake of simplicity, the difference between 5.7 and 8.0 is one.
 *
 * @param source The source version.
 * @param target The target version.
 *
 * @returns difference between major versions
 */
int major_version_difference(const Version &source, const Version &target);

/**
 * Returns all versions which were current when the given version was released,
 * sorted in ascending order.
 */
std::vector<Version> corresponding_versions(Version version);

/**
 * Get first LTS version in the same major version as the given version
 */
Version get_first_lts_version(Version version);

/**
 * Get first innovation version in the same major version as the given version
 */
Version get_first_innovation_version(Version version);

}  // namespace utils
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_UTILS_VERSION_H_
