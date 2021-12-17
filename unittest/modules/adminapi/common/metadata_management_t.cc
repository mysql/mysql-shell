/*
 * Copyright (c) 2019, 2021, Oracle and/or its affiliates.
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

#include <string>

#include "modules/adminapi/common/metadata_management_mysql.h"
#include "unittest/test_utils.h"

using mysqlshdk::mysql::Instance;
using mysqlshdk::mysql::Var_qualifier;
using mysqlshdk::utils::Version;

namespace testing {

using MDS = mysqlsh::dba::metadata::State;

class Metadata_management_test : public Shell_core_test_wrapper {
 public:
  void SetUp() override {
    Shell_core_test_wrapper::SetUp();
    m_session = get_session();
    m_instance = std::make_shared<mysqlsh::dba::Instance>(m_session);

    // The tests on this test case require the metadata schema to not exist
    m_instance->execute("DROP SCHEMA IF EXISTS mysql_innodb_cluster_metadata");
  }

  std::shared_ptr<mysqlshdk::db::ISession> get_session() {
    auto session = mysqlshdk::db::mysql::Session::create();

    mysqlshdk::db::Connection_options connection_options;
    connection_options.set_host(_host);
    connection_options.set_port(std::stoi(_mysql_port));
    connection_options.set_user(_user);
    connection_options.set_password(_pwd);

    session->connect(connection_options);

    return session;
  }

  void TearDown() override {
    if (m_session) {
      m_session->execute("DROP SCHEMA IF EXISTS mysql_innodb_cluster_metadata");
      m_session->execute(
          "DROP SCHEMA IF EXISTS mysql_innodb_cluster_metadata_bkp");
      m_session->close();
    }

    Shell_core_test_wrapper::TearDown();
  }

 protected:
  std::shared_ptr<mysqlshdk::db::ISession> m_session;
  std::shared_ptr<mysqlsh::dba::Instance> m_instance;

  void drop_metadata_schema_backup() {
    m_instance->execute(
        "DROP SCHEMA IF EXISTS mysql_innodb_cluster_metadata_bkp");
  }

  void create_metadata_schema() {
    m_instance->execute("CREATE SCHEMA mysql_innodb_cluster_metadata");
  }

  void create_metadata_schema_backup() {
    m_instance->execute("CREATE SCHEMA mysql_innodb_cluster_metadata_bkp");
    m_instance->execute(
        "CREATE SQL SECURITY INVOKER VIEW "
        "mysql_innodb_cluster_metadata_bkp.backup_stage (stage) AS SELECT "
        "'UPGRADING'");
    m_instance->execute(
        "CREATE VIEW mysql_innodb_cluster_metadata_bkp.schema_version (major, "
        "minor, patch) AS SELECT 1, 0, 1");
  }

  void set_metadata_version(int major, int minor, int patch,
                            const std::string &view = "schema_version") {
    m_instance->executef("DROP VIEW IF EXISTS !.!",
                         "mysql_innodb_cluster_metadata", view.c_str());

    m_instance->executef(
        "CREATE VIEW !.! (major, minor, patch) AS SELECT ?, ?, ?",
        "mysql_innodb_cluster_metadata", view.c_str(), major, minor, patch);
  }
};

TEST_F(Metadata_management_test, installed_version) {
  // The version will be -1.-1.-1 if the metadata schema does not exist
  auto installed = mysqlsh::dba::metadata::installed_version(m_instance);
  EXPECT_TRUE(installed == mysqlsh::dba::metadata::kNotInstalled);

  // The function will raise an exception if the metadata schema exists but the
  // schema_version view is missing
  create_metadata_schema();
  EXPECT_THROW_LIKE(mysqlsh::dba::metadata::installed_version(m_instance),
                    std::runtime_error,
                    "Error verifying metadata version, the metadata schema "
                    "mysql_innodb_cluster_metadata is inconsistent: the "
                    "schema_version view is missing.");

  // The version will be the one in the schema_version view when it exists
  set_metadata_version(1, 0, 5);
  installed = mysqlsh::dba::metadata::installed_version(m_instance);
  EXPECT_TRUE(installed == mysqlshdk::utils::Version(1, 0, 5));
}

TEST_F(Metadata_management_test, check_installed_schema_version) {
  auto current_version = mysqlsh::dba::metadata::current_version();
  mysqlshdk::utils::Version installed;
  mysqlshdk::utils::Version real_md_version;
  std::string md_version_schema;

  // No metadata schema
  auto compatibility = mysqlsh::dba::metadata::check_installed_schema_version(
      m_instance, &installed, &real_md_version, &md_version_schema);
  EXPECT_EQ(compatibility, MDS::NONEXISTING);
  EXPECT_EQ(mysqlsh::dba::metadata::kNotInstalled, installed);

  // The same scenario with a backup indicates a Restore Process that failed
  // after dropping the original metadata schema, is handled as a failed upgrade
  create_metadata_schema_backup();
  compatibility = mysqlsh::dba::metadata::check_installed_schema_version(
      m_instance, &installed, &real_md_version, &md_version_schema);
  EXPECT_EQ(compatibility, MDS::FAILED_UPGRADE);
  EXPECT_EQ(mysqlsh::dba::metadata::kUpgradingVersion, installed);

  // There's a metadata schema and a backup, this is the default scenario for a
  // failed upgrade
  create_metadata_schema();
  set_metadata_version(1, 0, 1);
  compatibility = mysqlsh::dba::metadata::check_installed_schema_version(
      m_instance, &installed, &real_md_version, &md_version_schema);
  EXPECT_EQ(compatibility, MDS::FAILED_UPGRADE);
  EXPECT_EQ(mysqlsh::dba::metadata::kUpgradingVersion, installed);
  drop_metadata_schema_backup();

  compatibility = mysqlsh::dba::metadata::check_installed_schema_version(
      m_instance, &installed, &real_md_version, &md_version_schema);

  EXPECT_TRUE(mysqlsh::dba::metadata::kIncompatible.is_set(compatibility));

  // Metadata upgrading version 0.0.0, no metadata backup is a restore scenario
  // when the data was already restored but setting the original version failed,
  // it is handled as a failed upgrade
  set_metadata_version(0, 0, 0);
  compatibility = mysqlsh::dba::metadata::check_installed_schema_version(
      m_instance, &installed, &real_md_version, &md_version_schema);
  EXPECT_EQ(compatibility, MDS::FAILED_UPGRADE);
  EXPECT_EQ(mysqlsh::dba::metadata::kUpgradingVersion, installed);

  // Metadata upgrading version 0.0.0, but lock available indicates
  // a failed upgrade state
  create_metadata_schema_backup();
  set_metadata_version(0, 0, 0);
  compatibility = mysqlsh::dba::metadata::check_installed_schema_version(
      m_instance, &installed, &real_md_version, &md_version_schema);
  EXPECT_EQ(compatibility, MDS::FAILED_UPGRADE);
  EXPECT_EQ(mysqlsh::dba::metadata::kUpgradingVersion, installed);

  // Metadata upgrading version 0.0.0, but lock is held by other session must
  // indicate an active upgrading process
  auto other_session = get_session();
  other_session->execute(
      "SELECT GET_LOCK('mysql_innodb_cluster_metadata.upgrade_in_progress', "
      "1)");
  compatibility = mysqlsh::dba::metadata::check_installed_schema_version(
      m_instance, &installed, &real_md_version, &md_version_schema);
  EXPECT_EQ(compatibility, MDS::UPGRADING);
  EXPECT_EQ(mysqlsh::dba::metadata::kUpgradingVersion, installed);
  other_session->close();
  drop_metadata_schema_backup();

  // The rest of the test are with the schema_version table existing, testing
  // The different combinations of version, no upgrade/restore in progress or
  // failed

  struct Version_check {
    int major;
    int minor;
    int patch;
    MDS expected_state;
  };

  int major = current_version.get_major();
  int minor = current_version.get_minor();
  int patch = current_version.get_patch();

  std::vector<Version_check> compatibility_checks{
      {major, minor, patch, MDS::EQUAL},
      {major, minor, patch - 1, MDS::PATCH_LOWER},
      {major, minor, patch + 1, MDS::PATCH_HIGHER},
      {major, minor - 1, patch, MDS::MINOR_LOWER},
      {major, minor + 1, patch, MDS::MINOR_HIGHER},
      {major - 1, minor, patch, MDS::MAJOR_LOWER},
      {major + 1, minor, patch, MDS::MAJOR_HIGHER}};

  for (const auto &check : compatibility_checks) {
    set_metadata_version(check.major, check.minor, check.patch);
    Version version;
    auto state = mysqlsh::dba::metadata::check_installed_schema_version(
        m_instance, &version, &real_md_version, &md_version_schema);
    auto text = shcore::str_format("Major: %d, Minor: %d, Patch: %d",
                                   check.major, check.minor, check.patch);
    SCOPED_TRACE(text.c_str());
    EXPECT_EQ(check.expected_state, state);
  }
}

}  // namespace testing
