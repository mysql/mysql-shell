/*
 * Copyright (c) 2017, 2026, Oracle and/or its affiliates.
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
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace mysqlshdk {
namespace utils {
/**
 * Class to handle versions following the MySQL Standard which is
 * \<major\>.\<minor\>.\<patch\>[.update][-extra].
 * Comparisons and numeric conversions ignore the update and extra parts.
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
   * Returns true if the patch version is present.
   */
  bool has_patch() const { return _patch.has_value(); }

  /**
   * Returns the update version, or 0 if not present.
   */
  int get_update() const { return _update.value_or(0); }

  /**
   * Returns true if the update version is present.
   */
  bool has_update() const { return _update.has_value(); }

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
   * Returns the version string including the update part when present:
   * \<major\>.\<minor\>.\<patch\>[.update].
   */
  std::string get_base_with_update() const;

  /**
   * Returns the version string in format \<major\>.\<minor\>.\<patch\>[-extra].
   */
  std::string get_full() const;

  /**
   * Returns the version string including the update part when present:
   * \<major\>.\<minor\>.\<patch\>[.update][-extra].
   */
  std::string get_full_with_update() const;

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

  // helpers used by gtest
  friend std::ostream &operator<<(::std::ostream &os, const Version &v) {
    return os << v.get_full();
  }

  friend void PrintTo(const Version &v, ::std::ostream *os) { *os << v; }

 private:
  int _major = 0;
  std::optional<int> _minor;
  std::optional<int> _patch;
  std::optional<int> _update;
  std::optional<std::string> _extra;

  int parse_token(std::string_view data);
};

inline const Version k_shell_version = Version(MYSH_VERSION EXTRA_NAME_SUFFIX);

namespace version {

namespace legacy {

/**
 * Provides the difference between legacy release-major versions of source and
 * target. This difference is positive if target has greater version than
 * source.
 */
int major_difference(const Version &source, const Version &target);

/**
 * Returns all legacy versions which were current when the given version was
 * released, sorted in ascending order.
 */
std::vector<Version> corresponding_versions(const Version &version);

/**
 * Get first LTS version in the same legacy major version as the given version.
 */
Version first_lts(const Version &version);

/**
 * Get the next LTS version for the given legacy version.
 */
Version next_lts(const Version &version);

/**
 * Get first innovation version in the same legacy major version as the given
 * version.
 */
Version first_innovation(const Version &version);

/**
 * Checks if this is a legacy LTS release.
 */
bool is_lts(const Version &v);

}  // namespace legacy

namespace calendar {

/**
 * Checks if this version belongs to the calendar versioning scheme.
 *
 * Calendar versions use month numbers 1..12, starting with 26.7.0.
 */
bool is_calendar_version(const Version &v);

/**
 * Checks if this calendar version belongs to the Shell release cadence.
 *
 * Shell releases use January, April, July, and October.
 */
bool is_release_cadence_version(const Version &v);

/**
 * Converts a calendar version to the matching legacy release slot.
 */
Version to_legacy(const Version &version);

/**
 * Converts a legacy LTS release slot to the matching calendar LTS version.
 */
Version lts_from_legacy(const Version &version);

/**
 * Get first LTS version in the same calendar release cycle as the given
 * version.
 */
Version first_lts(const Version &version);

/**
 * Get the next LTS version for the given calendar version.
 */
Version next_lts(const Version &version);

/**
 * Get first innovation version in the same calendar release cycle as the given
 * version.
 */
Version first_innovation(const Version &version);

/**
 * Checks if this is a calendar LTS release.
 */
bool is_lts(const Version &v);

}  // namespace calendar

/**
 * Provides the difference between release-major versions of source and target.
 * This difference is positive if target has greater version than source.
 *
 * NOTE: for sake of simplicity, the difference between 5.7 and 8.0 is one.
 * NOTE: calendar versions map 26.7 to the release slot after 9.x.
 *
 * @param source The source version.
 * @param target The target version.
 *
 * @returns difference between major versions
 */
int major_difference(const Version &source, const Version &target);

/**
 * Returns all versions which were current when the given version was released,
 * sorted in ascending order.
 */
std::vector<Version> corresponding_versions(const Version &version);

/**
 * Get first LTS version in the same major version as the given version
 */
Version first_lts(const Version &version);

/**
 * Get the next LTS version for the given version. Innovation releases return
 * the LTS version in their release cycle, while LTS releases return the
 * following LTS anchor.
 */
Version next_lts(const Version &version);

/**
 * Get first innovation version in the same major version as the given version
 */
Version first_innovation(const Version &version);

/**
 * Checks if this is an LTS release.
 */
bool is_lts(const Version &v);

/**
 * Checks if this version belongs to a MySQL Server release range supported by
 * this Shell build: legacy 8.x/9.x releases, or calendar releases from 26.7
 * up to the current Shell series.
 */
bool is_supported_server(const Version &v);

/**
 * Returns a human-readable description of supported MySQL Server release
 * ranges.
 */
std::string supported_servers();

}  // namespace version

}  // namespace utils
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_UTILS_VERSION_H_
