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
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <utility>
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

namespace {
namespace lts {

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

/**
 * Patch versions of the off-cycle releases of the given LTS release series.
 *
 * @returns false, if there are no off-cycle releases
 */
inline bool off_cycle_releases(const Version &v,
                               const off_cycle_releases_t **releases) {
  assert(is_lts_release(v));

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
  if (!is_lts_release(v)) {
    return v.get_patch();
  }

  if (const lts::off_cycle_releases_t * patch_versions;
      lts::off_cycle_releases(v, &patch_versions)) {
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
  assert(is_lts_release(v));

  if (const lts::off_cycle_releases_t * patch_versions;
      lts::off_cycle_releases(v, &patch_versions)) {
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

}  // namespace lts
}  // namespace

std::vector<Version> corresponding_versions(Version version) {
  // first innovation release
  assert(version >= Version(8, 1, 0));
  // LTS releases are 8.4.0, then x.7.0
  assert(
      (version.get_major() == 8 &&
       version.get_minor() <= lts::k_minor_version_8_4) ||
      (version.get_major() > 8 && version.get_minor() <= lts::k_minor_version));

  // this holds number of planned releases released so far
  int planned_releases = lts::count_planned_releases(version);

  if (is_lts_release(version)) {
    // this is an LTS release, count number of releases in the LTS series
    planned_releases += version.get_patch();
    // but don't count off-cycle releases
    planned_releases -= lts::count_off_cycle_releases(version);
  }

  static const auto lts_release_patch = [](int major, int releases,
                                           bool allow_off_cycle) {
    // we have the number of planned releases, but we need to adjust this
    // number by the number of off-cycle releases that happened in between
    if (const lts::off_cycle_releases_t * patch_versions;
        lts::detail::off_cycle_releases(major, &patch_versions)) {
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

  const auto is_off_cycle = lts::is_off_cycle_release(version);

  // start with the requested version
  std::vector<Version> result;
  result.emplace_back(std::move(version));

  while (true) {
    // get the major version of the previous LTS release
    const auto lts_release_major = result.back().get_major() - 1;

    if (lts::detail::k_release_8_0_id == lts_release_major) {
      // 8.0 series
      result.emplace_back(
          8, lts::k_minor_version_8_0,
          lts_release_patch(lts::detail::k_release_8_0_id,
                            33 + planned_releases, is_off_cycle));
      break;
    } else {
      // minor version of the previous LTS release
      const auto lts_release_minor = (8 == lts_release_major)
                                         ? lts::k_minor_version_8_4
                                         : lts::k_minor_version;

      result.emplace_back(
          lts_release_major, lts_release_minor,
          lts_release_patch(lts_release_major, planned_releases, is_off_cycle));
      planned_releases +=
          lts::count_planned_releases(lts_release_major, lts_release_minor);
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

/**
 * Checks if this is an LTS release.
 */
bool is_lts_release(const Version &v) {
  if (8 == v.get_major()) {
    return lts::k_minor_version_8_0 == v.get_minor() ||
           lts::k_minor_version_8_4 == v.get_minor();
  } else {
    return lts::k_minor_version == v.get_minor();
  }
}

}  // namespace utils
}  // namespace mysqlshdk
