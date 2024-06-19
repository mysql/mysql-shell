/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
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

#include <stdexcept>

#include "mysqlshdk/libs/mysql/version_compatibility.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk::mysql {
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

  const mysqlshdk::utils::Version min_lts_version_80(8, 0, 34);
  const mysqlshdk::utils::Version first_lts_version(8, 4, 0);

  // Anything < 8.0.0 is unsupported
  if (source.numeric_version_series() < 800) {
    throw std::runtime_error(shcore::str_format(
        "Unsupported MySQL Server version at source instance: %s",
        source.get_full().c_str()));
  }

  if (replica.numeric_version_series() < 800) {
    throw std::runtime_error(shcore::str_format(
        "Unsupported MySQL Server version at replica instance: %s",
        replica.get_full().c_str()));
  }

  // If {major.minor.patch.build/extra} are equal, it's compatible
  if (source == replica) {
    return Replication_version_compatibility::COMPATIBLE;
  }

  if (source.numeric_version_series() == 800 &&
      replica.numeric_version_series() == 800) {
    if ((source >= min_lts_version_80 && replica >= min_lts_version_80) ||
        source < replica) {
      return Replication_version_compatibility::COMPATIBLE;
    }

    if (replica < min_lts_version_80) {
      return Replication_version_compatibility::INCOMPATIBLE;
    }

    return Replication_version_compatibility::DOWNGRADE_ONLY;
  }

  // Incompatible if source is 8.0.x and replica is 9.0 or above
  if (source.numeric_version_series() == 800 &&
      replica.numeric_version_series() >= 900) {
    return Replication_version_compatibility::INCOMPATIBLE;
  }

  // Incompatible if major version difference is 2 or more
  if (abs(replica.get_major() - source.get_major()) >= 2) {
    return Replication_version_compatibility::INCOMPATIBLE;
  }

  // Incompatible if both are within an innovation release and the difference of
  // the major version is 1
  if (source < first_lts_version &&
      abs(replica.get_major() - source.get_major()) >= 1) {
    return Replication_version_compatibility::INCOMPATIBLE;
  }

  // If source is higher than replica it's compatible if both are LTS and
  // only the patch version differs, otherwise, it's limited
  if (source > replica) {
    if (source.numeric_version_series() == replica.numeric_version_series() &&
        source >= first_lts_version && replica >= first_lts_version) {
      return Replication_version_compatibility::COMPATIBLE;
    }
    return Replication_version_compatibility::DOWNGRADE_ONLY;
  }

  // Default to compatible for all other scenarios
  return Replication_version_compatibility::COMPATIBLE;
}

}  // namespace mysqlshdk::mysql
