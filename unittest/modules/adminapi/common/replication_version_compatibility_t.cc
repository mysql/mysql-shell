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

#include <string>

#include "modules/adminapi/common/base_cluster_impl.h"
#include "unittest/gprod_clean.h"
#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"
#include "unittest/test_utils/mocks/gmock_clean.h"

#include "mysqlshdk/libs/mysql/version_compatibility.h"

namespace mysqlsh::dba {

class Admin_api_replication_version_compat_test
    : public Shell_core_test_wrapper {
 public:
  static std::shared_ptr<mysqlshdk::db::ISession> create_session(
      int port, std::string user = "root") {
    auto session = mysqlshdk::db::mysql::Session::create();

    auto connection_options =
        Connection_options(user + ":root@localhost:" + std::to_string(port));
    session->connect(connection_options);

    return session;
  }
};

TEST_F(Admin_api_replication_version_compat_test,
       diagnostics_include_version_update) {
  using mysqlshdk::utils::Version;

  EXPECT_THROW_LIKE(
      mysqlshdk::mysql::verify_compatible_replication_versions(
          Version("27.0.0.2-debug"), Version(9, 7, 0)),
      std::runtime_error,
      "Unsupported MySQL Server version at source instance: 27.0.0.2-debug");
  EXPECT_THROW_LIKE(
      mysqlshdk::mysql::verify_compatible_replication_versions(
          Version(9, 7, 0), Version("27.0.0.2-debug")),
      std::runtime_error,
      "Unsupported MySQL Server version at replica instance: 27.0.0.2-debug");

  for (const std::string source_full : {"9.7.0.0", "9.7.0.12-debug"}) {
    SCOPED_TRACE(source_full);
    const Version source_version(source_full);
    auto issues = shcore::make_array();
    Base_cluster_impl::check_replication_version_compatibility(
        "source", "replica", source_version, Version("9.7.0.1"), false, false,
        &issues);
    EXPECT_TRUE(issues->empty());

    Base_cluster_impl::check_replication_version_compatibility(
        "source", "replica", source_version, Version("8.4.0.2"), false, false,
        &issues);
    ASSERT_EQ(1, issues->size());
    EXPECT_THAT(
        issues->front().as_string(),
        ::testing::HasSubstr("WARNING: The replication source ('source') "
                             "is running version " +
                             source_full));
    EXPECT_THAT(issues->front().as_string(),
                ::testing::HasSubstr("version 8.4.0.2."));
    EXPECT_THAT(
        issues->front().as_string(),
        ::testing::HasSubstr("new functionality from version 9.7.0 is"));

    issues->clear();
    Base_cluster_impl::check_replication_version_compatibility(
        "source", "replica", source_version, Version(8, 3, 0), false, false,
        &issues);
    ASSERT_EQ(1, issues->size());
    EXPECT_THAT(issues->front().as_string(),
                ::testing::HasSubstr("ERROR: The replication source ('source') "
                                     "is running version " +
                                     source_full));
    EXPECT_THAT(
        issues->front().as_string(),
        ::testing::HasSubstr("target replica's ('replica') version 8.3.0"));
  }
}

TEST_F(Admin_api_replication_version_compat_test,
       verify_compatible_replication_versions) {
  using mysqlshdk::mysql::Replication_version_compatibility;
  using mysqlshdk::mysql::verify_compatible_replication_versions;
  using mysqlshdk::utils::Version;

  // Anything < 8.0.0 is unsupported
  EXPECT_THROW_LIKE(
      verify_compatible_replication_versions(Version(5, 7, 4),
                                             Version(8, 1, 4)),
      std::runtime_error,
      "Unsupported MySQL Server version at source instance: 5.7.4");

  EXPECT_THROW_LIKE(
      verify_compatible_replication_versions(Version(8, 4, 1),
                                             Version(5, 7, 11)),
      std::runtime_error,
      "Unsupported MySQL Server version at replica instance: 5.7.11");

  EXPECT_THROW_LIKE(
      verify_compatible_replication_versions(Version(10, 0, 0),
                                             Version(26, 7, 0)),
      std::runtime_error,
      "Unsupported MySQL Server version at source instance: 10.0.0");

  EXPECT_THROW_LIKE(
      verify_compatible_replication_versions(Version(26, 7, 0),
                                             Version(10, 0, 0)),
      std::runtime_error,
      "Unsupported MySQL Server version at replica instance: 10.0.0");

  EXPECT_THROW_LIKE(
      verify_compatible_replication_versions(Version(26, 6, 0),
                                             Version(26, 7, 0)),
      std::runtime_error,
      "Unsupported MySQL Server version at source instance: 26.6.0");

  EXPECT_THROW_LIKE(
      verify_compatible_replication_versions(Version(26, 7, 0),
                                             Version(26, 6, 0)),
      std::runtime_error,
      "Unsupported MySQL Server version at replica instance: 26.6.0");

  EXPECT_THROW_LIKE(
      verify_compatible_replication_versions(Version(26, 5, 0),
                                             Version(27, 1, 0)),
      std::runtime_error,
      "Unsupported MySQL Server version at source instance: 26.5.0");

  EXPECT_THROW_LIKE(
      verify_compatible_replication_versions(Version(27, 1, 0),
                                             Version(26, 5, 0)),
      std::runtime_error,
      "Unsupported MySQL Server version at replica instance: 26.5.0");

  EXPECT_THROW_LIKE(
      verify_compatible_replication_versions(Version(27, 0, 0),
                                             Version(27, 1, 0)),
      std::runtime_error,
      "Unsupported MySQL Server version at source instance: 27.0.0");

  EXPECT_THROW_LIKE(
      verify_compatible_replication_versions(Version(27, 1, 0),
                                             Version(27, 0, 0)),
      std::runtime_error,
      "Unsupported MySQL Server version at replica instance: 27.0.0");

  // Easy path: if {major.minor.patch} are equal they're compatible
  EXPECT_EQ(Replication_version_compatibility::COMPATIBLE,
            verify_compatible_replication_versions(Version(9, 1, 4),
                                                   Version(9, 1, 4)));
  EXPECT_EQ(Replication_version_compatibility::COMPATIBLE,
            verify_compatible_replication_versions(Version("8.0.28-debug"),
                                                   Version("8.0.28-test")));

  // If both source and replica are withing the 8.0.x releases, they'll be
  // compatible only if both have a version >= 8.0.34. Otherwise, they have
  // limited compatibility
  EXPECT_EQ(Replication_version_compatibility::COMPATIBLE,
            verify_compatible_replication_versions(Version(8, 0, 34),
                                                   Version(8, 0, 35)));
  EXPECT_EQ(Replication_version_compatibility::COMPATIBLE,
            verify_compatible_replication_versions(Version(8, 0, 35),
                                                   Version(8, 0, 34)));

  EXPECT_EQ(Replication_version_compatibility::COMPATIBLE,
            verify_compatible_replication_versions(Version(8, 0, 33),
                                                   Version(8, 0, 34)));
  EXPECT_EQ(Replication_version_compatibility::COMPATIBLE,
            verify_compatible_replication_versions(Version(8, 0, 32),
                                                   Version(8, 0, 33)));

  // If both are on 8.0.x but the replica is not running an LTS version, it's
  // incompatible
  EXPECT_EQ(Replication_version_compatibility::INCOMPATIBLE,
            verify_compatible_replication_versions(Version(8, 0, 35),
                                                   Version(8, 0, 22)));
  EXPECT_EQ(Replication_version_compatibility::INCOMPATIBLE,
            verify_compatible_replication_versions(Version(8, 0, 34),
                                                   Version(8, 0, 33)));
  EXPECT_EQ(Replication_version_compatibility::INCOMPATIBLE,
            verify_compatible_replication_versions(Version(8, 0, 33),
                                                   Version(8, 0, 32)));

  // If source is lower than replica for 2 major versions it's incompatible
  EXPECT_EQ(Replication_version_compatibility::INCOMPATIBLE,
            verify_compatible_replication_versions(Version(8, 4, 0),
                                                   Version(28, 4, 0)));
  EXPECT_EQ(Replication_version_compatibility::INCOMPATIBLE,
            verify_compatible_replication_versions(Version(9, 7, 0),
                                                   Version(28, 7, 0)));

  // If source and replica are both an innovation release and the source has 1
  // major version lower, it's incompatible
  EXPECT_EQ(Replication_version_compatibility::INCOMPATIBLE,
            verify_compatible_replication_versions(Version(8, 3, 0),
                                                   Version(9, 0, 0)));
  EXPECT_EQ(Replication_version_compatibility::INCOMPATIBLE,
            verify_compatible_replication_versions(Version(8, 1, 0),
                                                   Version(9, 2, 0)));

  // If source is 8.0.x and replica is 9.0 or above it's incompatible
  EXPECT_EQ(Replication_version_compatibility::INCOMPATIBLE,
            verify_compatible_replication_versions(Version(8, 0, 38),
                                                   Version(9, 0, 0)));
  EXPECT_EQ(Replication_version_compatibility::INCOMPATIBLE,
            verify_compatible_replication_versions(Version(8, 0, 32),
                                                   Version(9, 0, 0)));

  // If source is innovation and replica is LTS in a previous release cycle,
  // it's incompatible.
  EXPECT_EQ(Replication_version_compatibility::INCOMPATIBLE,
            verify_compatible_replication_versions(Version(9, 1, 0),
                                                   Version(8, 4, 0)));

  // If source has a lower version than the replica, it's compatible (if not
  // major version is 2 or more)
  EXPECT_EQ(Replication_version_compatibility::COMPATIBLE,
            verify_compatible_replication_versions(Version(8, 0, 30),
                                                   Version(8, 0, 38)));
  EXPECT_EQ(Replication_version_compatibility::COMPATIBLE,
            verify_compatible_replication_versions(Version(8, 4, 0),
                                                   Version(8, 4, 1)));
  EXPECT_EQ(Replication_version_compatibility::COMPATIBLE,
            verify_compatible_replication_versions(Version(8, 4, 0),
                                                   Version(9, 0, 0)));
  EXPECT_EQ(Replication_version_compatibility::COMPATIBLE,
            verify_compatible_replication_versions(Version(8, 0, 37),
                                                   Version(8, 4, 0)));
  EXPECT_EQ(Replication_version_compatibility::COMPATIBLE,
            verify_compatible_replication_versions(Version(8, 4, 0),
                                                   Version(8, 4, 4)));
  EXPECT_EQ(Replication_version_compatibility::COMPATIBLE,
            verify_compatible_replication_versions(Version(8, 0, 37),
                                                   Version(8, 4, 1)));
  EXPECT_EQ(Replication_version_compatibility::COMPATIBLE,
            verify_compatible_replication_versions(Version(8, 0, 34),
                                                   Version(8, 3, 0)));
  EXPECT_EQ(Replication_version_compatibility::COMPATIBLE,
            verify_compatible_replication_versions(Version(8, 3, 0),
                                                   Version(8, 4, 0)));
  EXPECT_EQ(Replication_version_compatibility::COMPATIBLE,
            verify_compatible_replication_versions(Version(8, 1, 0),
                                                   Version(8, 3, 0)));
  EXPECT_EQ(Replication_version_compatibility::COMPATIBLE,
            verify_compatible_replication_versions(Version(9, 7, 1),
                                                   Version(26, 7, 0)));
  EXPECT_EQ(Replication_version_compatibility::COMPATIBLE,
            verify_compatible_replication_versions(Version(26, 7, 1),
                                                   Version(28, 4, 0)));
  EXPECT_EQ(Replication_version_compatibility::COMPATIBLE,
            verify_compatible_replication_versions(Version(28, 4, 0),
                                                   Version(28, 7, 0)));

  // If source is higher than replica it's compatible if both are LTS and
  // only the patch version differs, otherwise, it's limited
  EXPECT_EQ(Replication_version_compatibility::COMPATIBLE,
            verify_compatible_replication_versions(Version(8, 4, 1),
                                                   Version(8, 4, 0)));
  EXPECT_EQ(Replication_version_compatibility::COMPATIBLE,
            verify_compatible_replication_versions(Version(9, 7, 0),
                                                   Version(9, 7, 2)));
  EXPECT_EQ(Replication_version_compatibility::COMPATIBLE,
            verify_compatible_replication_versions(Version(28, 4, 1),
                                                   Version(28, 4, 0)));
  EXPECT_EQ(Replication_version_compatibility::INCOMPATIBLE,
            verify_compatible_replication_versions(Version(28, 7, 0),
                                                   Version(28, 4, 0)));
  EXPECT_EQ(Replication_version_compatibility::DOWNGRADE_ONLY,
            verify_compatible_replication_versions(Version(9, 7, 0),
                                                   Version(8, 4, 0)));
  EXPECT_EQ(Replication_version_compatibility::DOWNGRADE_ONLY,
            verify_compatible_replication_versions(Version(9, 7, 0),
                                                   Version(9, 6, 0)));
  EXPECT_EQ(Replication_version_compatibility::DOWNGRADE_ONLY,
            verify_compatible_replication_versions(Version(9, 7, 0),
                                                   Version(9, 5, 0)));
  EXPECT_EQ(Replication_version_compatibility::INCOMPATIBLE,
            verify_compatible_replication_versions(Version(9, 7, 0),
                                                   Version(8, 3, 0)));
  EXPECT_EQ(Replication_version_compatibility::DOWNGRADE_ONLY,
            verify_compatible_replication_versions(Version(8, 4, 0),
                                                   Version(8, 0, 34)));
  EXPECT_EQ(Replication_version_compatibility::DOWNGRADE_ONLY,
            verify_compatible_replication_versions(Version(8, 4, 0),
                                                   Version(8, 3, 0)));
  EXPECT_EQ(Replication_version_compatibility::DOWNGRADE_ONLY,
            verify_compatible_replication_versions(Version(8, 4, 0),
                                                   Version(8, 2, 0)));
  EXPECT_EQ(Replication_version_compatibility::DOWNGRADE_ONLY,
            verify_compatible_replication_versions(Version(28, 4, 0),
                                                   Version(26, 10, 0)));
}

TEST_F(Admin_api_replication_version_compat_test,
       calendar_version_support_ends_at_next_lts) {
  using mysqlshdk::mysql::Replication_version_compatibility;
  using mysqlshdk::mysql::verify_compatible_replication_versions;
  using mysqlshdk::utils::Version;

  EXPECT_EQ(Replication_version_compatibility::COMPATIBLE,
            verify_compatible_replication_versions(Version(9, 7, 0),
                                                   Version(28, 4, 0)));
  EXPECT_EQ(Replication_version_compatibility::INCOMPATIBLE,
            verify_compatible_replication_versions(Version(9, 7, 0),
                                                   Version(28, 7, 0)));
  EXPECT_EQ(Replication_version_compatibility::INCOMPATIBLE,
            verify_compatible_replication_versions(Version(26, 7, 0),
                                                   Version(28, 7, 0)));
}

TEST_F(Admin_api_replication_version_compat_test,
       calendar_version_compatibility_uses_legacy_overlap_semantics) {
  using mysqlshdk::mysql::Replication_version_compatibility;
  using mysqlshdk::mysql::verify_compatible_replication_versions;
  using mysqlshdk::utils::Version;

  EXPECT_EQ(Replication_version_compatibility::COMPATIBLE,
            verify_compatible_replication_versions(Version(58, 7, 0),
                                                   Version(60, 4, 0)));
  EXPECT_EQ(Replication_version_compatibility::INCOMPATIBLE,
            verify_compatible_replication_versions(Version(58, 7, 0),
                                                   Version(60, 7, 0)));
  EXPECT_EQ(Replication_version_compatibility::INCOMPATIBLE,
            verify_compatible_replication_versions(Version(60, 7, 0),
                                                   Version(60, 4, 0)));
  EXPECT_EQ(Replication_version_compatibility::DOWNGRADE_ONLY,
            verify_compatible_replication_versions(Version(63, 1, 1),
                                                   Version(63, 1, 0)));
}

}  // namespace mysqlsh::dba
