/*
 * Copyright (c) 2024, 2026, Oracle and/or its affiliates.
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

#include <cstdlib>
#include <stdexcept>

#include "mysqlshdk/libs/mysql/version_compatibility.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk::mysql {
namespace {

constexpr mysqlshdk::utils::Version k_first_rollback_supported_8_0_version(8, 0,
                                                                           34);

utils::Version compatibility_version(const utils::Version &version) {
  if (utils::version::calendar::is_calendar_version(version)) {
    return utils::version::calendar::to_legacy(version);
  }

  return version;
}

bool is_unsupported_version(const utils::Version &version) {
  if (version.numeric_version_series() < 800) {
    return true;
  }

  return version >= utils::Version(10, 0, 0) &&
         !utils::version::calendar::is_calendar_version(version);
}

bool is_lts_or_bugfix_release(const utils::Version &version) {
  return version.numeric_version_series() == 800 ||
         utils::version::is_lts(version);
}

utils::Version first_lts_or_bugfix_rollback_target(
    const utils::Version &source_series) {
  if (source_series.get_major() == 8 && source_series.get_minor() == 4) {
    return k_first_rollback_supported_8_0_version;
  }

  return utils::version::legacy::first_lts(
      utils::Version(source_series.get_major() - 1, 0, 0));
}

bool can_rollback_from_lts_or_bugfix(const utils::Version &source_series,
                                     const utils::Version &replica_series) {
  if (source_series.numeric_version_series() == 800) {
    return false;
  }

  const auto first_rollback_target =
      first_lts_or_bugfix_rollback_target(source_series);
  return replica_series >= first_rollback_target &&
         replica_series < source_series;
}

bool can_rollback_within_innovation_series(
    const utils::Version &source, const utils::Version &replica,
    const utils::Version &source_series, const utils::Version &replica_series) {
  return !is_lts_or_bugfix_release(source) &&
         !is_lts_or_bugfix_release(replica) &&
         source_series.get_major() == replica_series.get_major();
}

}  // namespace

Replication_version_compatibility verify_compatible_replication_versions(
    const utils::Version &source, const utils::Version &replica) {
  // In general, replication is only fully supported from one release series
  // to the next higher release series. For example, 8.0.36 to 8.0.37, or
  // 8.2.0 to 8.3.0.
  //
  // With the LTS release model, the rule is that it is possible to
  // replicate from an LTS or Innovation release to:
  //
  //   - The next LTS release
  //   - Any future Innovation release up until the next LTS release:
  //     LTS 8.4 -> LTS 9.7, but not LTS 8.4 -> LTS 10.8
  //
  // 1. Supported upgrade paths:
  //
  //   - Within an LTS or Bugfix series
  //   - From an LTS or Bugfix series to the next LTS series
  //   - From an LTS or Bugfix release to an Innovation release before the
  //     next LTS series
  //   - From an Innovation series to the next LTS series
  //   - From within an Innovation series
  //
  // 2. Downgrade support:
  //
  // For downgrade purposes, it's only fully supported to replicate from a
  // higher release series to a lower release series both are within the
  // same LTS release and only patch versions differ. For example:
  // 8.4.20 -> 8.4.11.
  //
  // Finally, it's possible, although limited to rollback operations, to
  // replicate:
  //
  //   - From an LTS or Bugfix series to the previous LTS or Bugfix series
  //   - From an LTS or Bugfix series to an Innovation series after the
  //     previous LTS series
  //   - From within an Innovation series

  if (is_unsupported_version(source)) {
    throw std::runtime_error(shcore::str_format(
        "Unsupported MySQL Server version at source instance: %s",
        source.get_full_with_update().c_str()));
  }

  if (is_unsupported_version(replica)) {
    throw std::runtime_error(shcore::str_format(
        "Unsupported MySQL Server version at replica instance: %s",
        replica.get_full_with_update().c_str()));
  }

  // If {major.minor.patch.build/extra} are equal, it's compatible
  if (source == replica) {
    return Replication_version_compatibility::COMPATIBLE;
  }

  const auto source_series = compatibility_version(source);
  const auto replica_series = compatibility_version(replica);
  const auto first_lts_version =
      utils::version::legacy::first_lts(source_series);

  if (source_series.numeric_version_series() == 800 &&
      replica_series.numeric_version_series() == 800) {
    if ((source >= k_first_rollback_supported_8_0_version &&
         replica >= k_first_rollback_supported_8_0_version) ||
        source < replica) {
      return Replication_version_compatibility::COMPATIBLE;
    }

    if (replica < k_first_rollback_supported_8_0_version) {
      return Replication_version_compatibility::INCOMPATIBLE;
    }

    return Replication_version_compatibility::DOWNGRADE_ONLY;
  }

  // Incompatible if source is 8.0.x and replica is 9.0 or above
  if (source_series.numeric_version_series() == 800 &&
      replica_series.numeric_version_series() >= 900) {
    return Replication_version_compatibility::INCOMPATIBLE;
  }

  // Incompatible if major version difference is 2 or more
  if (std::abs(replica_series.get_major() - source_series.get_major()) >= 2) {
    return Replication_version_compatibility::INCOMPATIBLE;
  }

  // Incompatible if the source is within an innovation release and the replica
  // is in the next release cycle.
  if (source_series < replica_series && source_series < first_lts_version &&
      std::abs(replica_series.get_major() - source_series.get_major()) >= 1) {
    return Replication_version_compatibility::INCOMPATIBLE;
  }

  // If source is higher than replica it's compatible if both are LTS or Bugfix
  // and only the patch version differs. Limited rollback is allowed only for
  // the explicit rollback paths from the WL.
  if (source > replica) {
    const auto source_is_lts_or_bugfix = is_lts_or_bugfix_release(source);
    const auto replica_is_lts_or_bugfix = is_lts_or_bugfix_release(replica);

    if (source_series.numeric_version_series() ==
            replica_series.numeric_version_series() &&
        source_is_lts_or_bugfix && replica_is_lts_or_bugfix) {
      return Replication_version_compatibility::COMPATIBLE;
    }

    if (source_is_lts_or_bugfix &&
        can_rollback_from_lts_or_bugfix(source_series, replica_series)) {
      return Replication_version_compatibility::DOWNGRADE_ONLY;
    }

    if (can_rollback_within_innovation_series(source, replica, source_series,
                                              replica_series)) {
      return Replication_version_compatibility::DOWNGRADE_ONLY;
    }

    return Replication_version_compatibility::INCOMPATIBLE;
  }

  // Default to compatible for all other scenarios
  return Replication_version_compatibility::COMPATIBLE;
}

}  // namespace mysqlshdk::mysql
