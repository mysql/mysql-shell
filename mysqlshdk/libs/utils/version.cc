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

#include "mysqlshdk/libs/utils/version.h"

#include <algorithm>
#include <cassert>
#include <charconv>
#include <map>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <vector>

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
    ret_val.append(".");
    ret_val.append(std::to_string(*_minor));
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

namespace version {

namespace {

const Version k_min_supported_mysql_server_version(8, 0, 0);
const Version k_first_unsupported_legacy_mysql_server_version(10, 0, 0);
const Version k_min_calendar_mysql_server_version(26, 7, 0);
constexpr int k_july_2026_lts_anchor_patch_8_4 = 11;
constexpr int k_july_2026_lts_anchor_patch_9_7 = 2;

Version max_supported_mysql_server_version() {
  return Version(k_shell_version.get_major(), k_shell_version.get_minor(),
                 9999);
}

}  // namespace

bool is_supported_server(const Version &v) {
  if (v < k_min_supported_mysql_server_version) {
    return false;
  }

  if (v < k_first_unsupported_legacy_mysql_server_version) {
    return true;
  }

  return calendar::is_calendar_version(v) &&
         v <= max_supported_mysql_server_version();
}

std::string supported_servers() {
  return shcore::str_format(
      "%s <= version < %s or %s <= version <= %s.*",
      k_min_supported_mysql_server_version.get_short().c_str(),
      k_first_unsupported_legacy_mysql_server_version.get_short().c_str(),
      k_min_calendar_mysql_server_version.get_short().c_str(),
      k_shell_version.get_short().c_str());
}

namespace calendar {

constexpr int k_anchor_year = 26;
constexpr int k_anchor_month = 7;
constexpr int k_lts_start_year = 28;
constexpr int k_lts_month = 4;
constexpr int k_cycle_releases = 8;
constexpr int k_legacy_anchor_major = 10;
constexpr int k_legacy_lts_minor = 7;

namespace {

inline bool is_release_month(int month) {
  return month == 1 || month == 4 || month == 7 || month == 10;
}

inline bool is_valid_month(int month) { return month >= 1 && month <= 12; }

inline int release_index(int month) {
  assert(is_valid_month(month));

  return (month - 1) / 3;
}

inline bool is_lts_year(int year) {
  return year >= k_lts_start_year && year % 2 == 0;
}

inline int release_offset(const Version &v) {
  assert(is_calendar_version(v));

  auto index = release_index(v.get_minor());

  if (is_lts_year(v.get_major()) && v.get_minor() > k_lts_month &&
      index == release_index(k_lts_month)) {
    ++index;
  }

  return (v.get_major() - k_anchor_year) * 4 +
         (index - release_index(k_anchor_month));
}

inline int cycle(const Version &v) {
  return release_offset(v) / k_cycle_releases;
}

inline int lts_offset(int cycle) {
  return (k_lts_start_year + cycle * 2 - k_anchor_year) * 4 +
         (release_index(k_lts_month) - release_index(k_anchor_month));
}

inline Version lts_for_cycle(int cycle, int patch = 0) {
  return Version(k_lts_start_year + cycle * 2, k_lts_month, patch);
}

[[maybe_unused]] inline bool is_legacy_lts_slot(const Version &v) {
  return v.get_major() >= k_legacy_anchor_major &&
         v.get_minor() == k_legacy_lts_minor;
}

}  // namespace

bool is_calendar_version(const Version &v) {
  return is_valid_month(v.get_minor()) &&
         v >= Version(k_anchor_year, k_anchor_month, 0);
}

bool is_release_cadence_version(const Version &v) {
  return is_calendar_version(v) && is_release_month(v.get_minor());
}

Version to_legacy(const Version &v) {
  assert(is_calendar_version(v));

  if (!is_calendar_version(v)) {
    return v;
  }

  // 26.7.0 replaced the old 10.0.0 slot in the release sequence.
  // Non-cadence calendar months are grouped into the matching quarterly slot.
  const auto offset = release_offset(v);

  return Version(k_legacy_anchor_major + offset / k_cycle_releases,
                 offset % k_cycle_releases, v.get_patch());
}

Version lts_from_legacy(const Version &v) {
  assert(is_legacy_lts_slot(v));

  return lts_for_cycle(v.get_major() - k_legacy_anchor_major, v.get_patch());
}

Version first_lts(const Version &v) { return lts_for_cycle(cycle(v)); }

Version next_lts(const Version &v) {
  if (!is_lts(v)) {
    return first_lts(v);
  }

  const auto current_lts = first_lts(v);
  return Version(current_lts.get_major() + 2, k_lts_month, 0);
}

Version first_innovation(const Version &v) {
  return Version(k_anchor_year + cycle(v) * 2, k_anchor_month, 0);
}

bool is_lts(const Version &v) {
  return v.get_major() >= k_lts_start_year && v.get_major() % 2 == 0 &&
         v.get_minor() == k_lts_month;
}

}  // namespace calendar

namespace legacy {

using off_cycle_releases_t = std::set<int>;

// Bugfix release minor version for 8 series
constexpr int k_minor_version_8_0 = 0;
// LTS release minor version for 8 series
constexpr int k_minor_version_8_4 = 4;
// LTS release minor version for series > 8
constexpr int k_minor_version = 7;

namespace detail {

// ID of an 8.0 LTS release
constexpr int k_release_8_0_id = 7;

bool off_cycle_releases(int version_id, const off_cycle_releases_t **releases) {
  assert(releases);

  // LTS off-cycle releases
  static const std::unordered_map<int, off_cycle_releases_t>
      k_off_cycle_releases{
          {
              k_release_8_0_id,  // 8.0.x
              {
                  39,
              },
          },
          {
              8,  // 8.4.x
              {
                  2,
              },
          },
      };

  if (const auto found = k_off_cycle_releases.find(version_id);
      k_off_cycle_releases.end() != found) {
    *releases = &found->second;
    return true;
  } else {
    return false;
  }
}

}  // namespace detail

bool is_lts(const Version &v) {
  if (8 == v.get_major()) {
    return k_minor_version_8_0 == v.get_minor() ||
           k_minor_version_8_4 == v.get_minor();
  }

  return k_minor_version == v.get_minor();
}

inline bool uses_july_2026_lts_anchor_cadence(const Version &v) {
  return (v.get_major() == 8 && v.get_minor() == k_minor_version_8_4 &&
          v.get_patch() >= k_july_2026_lts_anchor_patch_8_4) ||
         (v.get_major() == 9 && v.get_minor() == k_minor_version &&
          v.get_patch() >= k_july_2026_lts_anchor_patch_9_7);
}

inline Version corresponding_8_4_lts_version(const Version &version) {
  assert(version.get_major() == 9 && version.get_minor() == k_minor_version);

  return Version(8, k_minor_version_8_4,
                 k_july_2026_lts_anchor_patch_8_4 + version.get_patch() -
                     k_july_2026_lts_anchor_patch_9_7);
}

/**
 * Patch versions of the off-cycle releases of the given LTS release series.
 *
 * @returns false, if there are no off-cycle releases
 */
inline bool off_cycle_releases(const Version &v,
                               const off_cycle_releases_t **releases) {
  assert(is_lts(v));

  int major = v.get_major();

  if (8 == major && k_minor_version_8_0 == v.get_minor()) {
    major = detail::k_release_8_0_id;
  }

  return detail::off_cycle_releases(major, releases);
}

/**
 * Off-cycle release is an innovation release with non-zero patch version, or
 * one of the LTS versions listed above.
 */
inline bool is_off_cycle_release(const Version &v) {
  if (!is_lts(v)) {
    return v.get_patch();
  }

  if (const off_cycle_releases_t * patch_versions;
      off_cycle_releases(v, &patch_versions)) {
    return std::binary_search(patch_versions->begin(), patch_versions->end(),
                              v.get_patch());
  }

  return false;
}

/**
 * Counts the number of planned releases in this series up to and including the
 * given major.minor version.
 */
inline int count_planned_releases(int major, int minor) {
  assert(major > 8 || minor > 0);  // major.minor >= 8.1
  int count = minor;

  if (8 != major) {
    // first innovation release is x.0.0, need to add 1 accommodate for this
    ++count;
  }

  return count;
}

inline int count_planned_releases(const Version &v) {
  return count_planned_releases(v.get_major(), v.get_minor());
}

/**
 * Counts the number of off-cycle releases in this series up to and including
 * the given major.minor.patch version.
 */
inline int count_off_cycle_releases(const Version &v) {
  assert(is_lts(v));

  if (const off_cycle_releases_t * patch_versions;
      off_cycle_releases(v, &patch_versions)) {
    int count = patch_versions->size();

    // starting with the latest off-cycle release, count the number of these
    // releases which are less than or equal to the requested version
    for (auto it = patch_versions->rbegin(); it != patch_versions->rend();
         ++it) {
      if (*it > v.get_patch()) {
        --count;
      } else {
        break;
      }
    }

    return count;
  } else {
    // no off-cycle releases
    return 0;
  }
}

int major_difference(const Version &source, const Version &target) {
  const auto major_version = [](const Version &v) {
    // we pretend that version 5.7 is 7 to simplify the code
    const auto m = v.get_major();

    return 5 == m && 7 == v.get_minor() ? 7 : m;
  };

  return major_version(target) - major_version(source);
}

std::vector<Version> corresponding_versions(const Version &version) {
  // first innovation release
  assert(version >= Version(8, 1, 0));
  // LTS releases are 8.4.0, then x.7.0
  assert((version.get_major() == 8 &&
          version.get_minor() <= k_minor_version_8_4) ||
         (version.get_major() > 8 && version.get_minor() <= k_minor_version));

  if (uses_july_2026_lts_anchor_cadence(version)) {
    std::vector<Version> result;

    if (version.get_major() > 8) {
      result.emplace_back(corresponding_8_4_lts_version(version));
    }

    result.emplace_back(version);
    return result;
  }

  // this holds number of planned releases released so far
  int planned_releases = count_planned_releases(version);

  if (is_lts(version)) {
    // this is an LTS release, count number of releases in the LTS series
    planned_releases += version.get_patch();
    // but don't count off-cycle releases
    planned_releases -= count_off_cycle_releases(version);
  }

  static const auto lts_release_patch = [](int major, int releases,
                                           bool allow_off_cycle) {
    // we have the number of planned releases, but we need to adjust this
    // number by the number of off-cycle releases that happened in between
    if (const off_cycle_releases_t * patch_versions;
        detail::off_cycle_releases(major, &patch_versions)) {
      // add all off-cycle releases
      releases += patch_versions->size();

      // starting from the end, find the off-cycle release that's smaller than
      // the requested number of releases
      for (auto it = patch_versions->rbegin(); it != patch_versions->rend();
           ++it) {
        if (*it < releases) {
          // patch version of the off-cycle release is smaller than the
          // requested number, stop here
          break;
        } else if (*it == releases) {
          // patch version is equal to the requested number, return this version
          // when off-cycle versions are not allowed, return previous version
          // NOTE: off-cycle versions are allowed only when we're looking for a
          // version corresponding to an off-cycle version
          if (!allow_off_cycle) {
            --releases;
          }

          break;
        } else {
          --releases;
        }
      }
    }

    return releases;
  };

  const auto is_off_cycle = is_off_cycle_release(version);

  // start with the requested version
  std::vector<Version> result;
  result.emplace_back(version);

  while (true) {
    // get the major version of the previous LTS release
    const auto lts_release_major = result.back().get_major() - 1;

    if (detail::k_release_8_0_id == lts_release_major) {
      // 8.0 series
      result.emplace_back(
          8, k_minor_version_8_0,
          lts_release_patch(detail::k_release_8_0_id, 33 + planned_releases,
                            is_off_cycle));
      break;
    } else {
      // minor version of the previous LTS release
      const auto lts_release_minor =
          (8 == lts_release_major) ? k_minor_version_8_4 : k_minor_version;

      result.emplace_back(
          lts_release_major, lts_release_minor,
          lts_release_patch(lts_release_major, planned_releases, is_off_cycle));
      planned_releases +=
          count_planned_releases(lts_release_major, lts_release_minor);
    }
  }

  std::reverse(result.begin(), result.end());
  return result;
}

Version first_lts(const Version &version) {
  // This function should not be called with versions lower than 8.0.0
  assert(version >= Version(8, 0, 0));

  auto major = version.get_major();
  return major == 8 ? Version(8, 4, 0) : Version(major, 7, 0);
}

Version next_lts(const Version &version) {
  // This function should not be called with versions lower than 8.0.0
  assert(version >= Version(8, 0, 0));

  if (!is_lts(version)) {
    return first_lts(version);
  }

  if (version.get_major() == 8 && version.get_minor() == k_minor_version_8_0) {
    return Version(8, k_minor_version_8_4, 0);
  }

  return first_lts(Version(version.get_major() + 1, 0, 0));
}

Version first_innovation(const Version &version) {
  // This function should not be called with versions lower than 8.0.0
  assert(version >= Version(8, 0, 0));

  auto major = version.get_major();
  return major == 8 ? Version(8, 1, 0) : Version(major, 0, 0);
}

}  // namespace legacy

namespace calendar {

std::vector<Version> corresponding_versions(const Version &version) {
  const auto offset = release_offset(version);
  const auto legacy_lts_patch = offset + version.get_patch();

  std::vector<Version> result;
  result.emplace_back(8, 4,
                      k_july_2026_lts_anchor_patch_8_4 + legacy_lts_patch);
  result.emplace_back(9, 7,
                      k_july_2026_lts_anchor_patch_9_7 + legacy_lts_patch);

  for (int i = 0; i <= offset / k_cycle_releases; ++i) {
    const auto cycle_lts_offset = lts_offset(i);

    if (offset >= cycle_lts_offset) {
      result.emplace_back(
          lts_for_cycle(i, offset - cycle_lts_offset + version.get_patch()));
    }
  }

  if (result.back() != version) {
    result.emplace_back(version);
  }

  return result;
}

}  // namespace calendar

int major_difference(const Version &source, const Version &target) {
  return legacy::major_difference(
      calendar::is_calendar_version(source) ? calendar::to_legacy(source)
                                            : source,
      calendar::is_calendar_version(target) ? calendar::to_legacy(target)
                                            : target);
}

std::vector<Version> corresponding_versions(const Version &version) {
  if (calendar::is_calendar_version(version)) {
    return calendar::corresponding_versions(version);
  }

  return legacy::corresponding_versions(version);
}

Version first_lts(const Version &version) {
  // This function should not be called with versions lower than 8.0.0
  assert(version >= Version(8, 0, 0));

  if (calendar::is_calendar_version(version)) {
    return calendar::first_lts(version);
  }

  return legacy::first_lts(version);
}

Version next_lts(const Version &version) {
  // This function should not be called with versions lower than 8.0.0
  assert(version >= Version(8, 0, 0));

  if (!is_lts(version)) {
    return first_lts(version);
  }

  if (calendar::is_calendar_version(version)) {
    return calendar::next_lts(version);
  }

  const auto next_legacy_lts = legacy::next_lts(version);

  if (next_legacy_lts.get_major() >= calendar::k_legacy_anchor_major) {
    return calendar::lts_from_legacy(next_legacy_lts);
  }

  return next_legacy_lts;
}

Version first_innovation(const Version &version) {
  // This function should not be called with versions lower than 8.0.0
  assert(version >= Version(8, 0, 0));

  if (calendar::is_calendar_version(version)) {
    return calendar::first_innovation(version);
  }

  return legacy::first_innovation(version);
}

bool is_lts(const Version &v) {
  if (calendar::is_calendar_version(v)) {
    return calendar::is_lts(v);
  }

  return legacy::is_lts(v);
}

}  // namespace version

}  // namespace utils
}  // namespace mysqlshdk
