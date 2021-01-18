/*
 * Copyright (c) 2021, Oracle and/or its affiliates.
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

#include "mysqlshdk/include/shellcore/scoped_contexts.h"
#include "mysqlshdk/libs/ssh/ssh_connection_options.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "unittest/gtest_clean.h"
#include "unittest/test_utils/shell_base_test.h"

using mysqlshdk::ssh::Ssh_connection_options;
extern "C" const char *g_test_home;

namespace testing {

class Ssh_connection_options_t : public tests::Shell_base_test {
  void SetUp() override {
    m_tmpdir = getenv("TMPDIR");
    m_ssh_files = shcore::path::join_path(m_tmpdir, "ssh_files");
    if (shcore::is_folder(m_ssh_files)) {
      shcore::remove_directory(m_ssh_files, true);
    }
    shcore::create_directory(m_ssh_files, true, 0700);

    m_key_with_pw = shcore::path::join_path(m_ssh_files, "with_password");
    m_key_without_pw = shcore::path::join_path(m_ssh_files, "no_password");
    m_config_file = shcore::path::join_path(m_ssh_files, "ssh_config");

    shcore::copy_file(shcore::path::join_path(g_test_home, "data", "docker",
                                              "keys", "no_password"),
                      m_key_without_pw);

    shcore::copy_file(shcore::path::join_path(g_test_home, "data", "docker",
                                              "keys", "with_password"),
                      m_key_with_pw);

    shcore::copy_file(shcore::path::join_path(g_test_home, "data", "docker",
                                              "sshd_pw_login", "ssh_config"),
                      m_config_file);
  }

 protected:
  std::string m_ssh_files;
  std::string m_tmpdir;
  std::string m_key_with_pw;
  std::string m_key_without_pw;
  std::string m_config_file;
};

TEST_F(Ssh_connection_options_t, default_initialization) {
  Ssh_connection_options ssh_config;

  ASSERT_TRUE(ssh_config.has_scheme());
  ASSERT_FALSE(ssh_config.has_user());
  ASSERT_FALSE(ssh_config.has_password());
  ASSERT_FALSE(ssh_config.has_host());
  ASSERT_FALSE(ssh_config.has_port());

  ASSERT_FALSE(ssh_config.has_key_file());
  ASSERT_FALSE(ssh_config.has_keyfile_password());
  ASSERT_FALSE(ssh_config.has_remote_host());
  ASSERT_FALSE(ssh_config.has_remote_port());
  ASSERT_FALSE(ssh_config.has_local_port());
  ASSERT_FALSE(ssh_config.has_config_file());

  // has verifies the existence of the option
  EXPECT_TRUE(ssh_config.has(mysqlshdk::db::kScheme));
  EXPECT_TRUE(ssh_config.has(mysqlshdk::db::kUser));
  EXPECT_TRUE(ssh_config.has(mysqlshdk::db::kPassword));
  EXPECT_TRUE(ssh_config.has(mysqlshdk::db::kHost));

  EXPECT_TRUE(ssh_config.has(mysqlshdk::db::kSshIdentityFile));
  EXPECT_TRUE(ssh_config.has(mysqlshdk::db::kSshRemoteHost));
  EXPECT_TRUE(ssh_config.has(mysqlshdk::db::kSshConfigFile));

  // has_value verifies the existence of a value for the option
  EXPECT_FALSE(ssh_config.has_value(mysqlshdk::db::kSchema));
  EXPECT_FALSE(ssh_config.has_value(mysqlshdk::db::kUser));
  EXPECT_FALSE(ssh_config.has_value(mysqlshdk::db::kPassword));
  EXPECT_FALSE(ssh_config.has_value(mysqlshdk::db::kHost));
  EXPECT_FALSE(ssh_config.has_value(mysqlshdk::db::kPort));

  EXPECT_FALSE(ssh_config.has_value(mysqlshdk::db::kSshIdentityFile));
  EXPECT_FALSE(ssh_config.has_value(mysqlshdk::db::kSshRemoteHost));
  EXPECT_FALSE(ssh_config.has_value(mysqlshdk::db::kSshConfigFile));
}

TEST_F(Ssh_connection_options_t, scheme_functions) {
  Ssh_connection_options ssh_configs;
  std::string msg;
  msg = "The ssh_connection option '";
  msg.append(mysqlshdk::db::kScheme);
  msg.append("' is already defined as 'ssh'.");
  EXPECT_THROW_LIKE(ssh_configs.set_scheme("mysqlx"), std::invalid_argument,
                    msg.c_str());
  EXPECT_TRUE(ssh_configs.has_scheme());
  EXPECT_STREQ("ssh", ssh_configs.get_scheme().c_str());
  EXPECT_NO_THROW(ssh_configs.clear_scheme());
  EXPECT_FALSE(ssh_configs.has_scheme());
  msg = "The ssh_connection option '";
  msg.append(mysqlshdk::db::kScheme).append("' has no value.");
  EXPECT_THROW_LIKE(ssh_configs.get_scheme(), std::invalid_argument,
                    msg.c_str());
}

TEST_F(Ssh_connection_options_t, user_functions) {
  Ssh_connection_options ssh_config;
  std::string msg;

  EXPECT_NO_THROW(ssh_config.set_user("value"));
  msg = "The ssh_connection option '";
  msg.append(mysqlshdk::db::kUser);
  msg.append("' is already defined as 'value'.");
  EXPECT_THROW_LIKE(ssh_config.set_user("value1"), std::invalid_argument,
                    msg.c_str());
  EXPECT_TRUE(ssh_config.has_user());
  EXPECT_STREQ("value", ssh_config.get_user().c_str());
  EXPECT_NO_THROW(ssh_config.clear_user());
  EXPECT_FALSE(ssh_config.has_user());
  msg = "The ssh_connection option '";
  msg.append(mysqlshdk::db::kUser).append("' has no value.");
  EXPECT_THROW_LIKE(ssh_config.get_user(), std::invalid_argument, msg.c_str());
}

TEST_F(Ssh_connection_options_t, password_functions) {
  Ssh_connection_options ssh_config;
  std::string msg;

  EXPECT_NO_THROW(ssh_config.set_password("value"));
  msg = "The ssh_connection option '";
  msg.append(mysqlshdk::db::kPassword);
  msg.append("' is already defined as 'value'.");
  EXPECT_THROW_LIKE(ssh_config.set_password("value1"), std::invalid_argument,
                    msg.c_str());

  EXPECT_TRUE(ssh_config.has_password());
  EXPECT_STREQ("value", ssh_config.get_password().c_str());
  EXPECT_NO_THROW(ssh_config.clear_password());
  EXPECT_FALSE(ssh_config.has_password());
  msg = "The ssh_connection option '";
  msg.append(mysqlshdk::db::kPassword).append("' has no value.");
  EXPECT_THROW_LIKE(ssh_config.get_password(), std::invalid_argument,
                    msg.c_str());
}

TEST_F(Ssh_connection_options_t, host_functions) {
  Ssh_connection_options ssh_config;
  std::string msg;

  EXPECT_NO_THROW(ssh_config.set_host("value"));
  msg = "The ssh_connection option '";
  msg.append(mysqlshdk::db::kHost);
  msg.append("' is already defined as 'value'.");
  EXPECT_THROW_LIKE(ssh_config.set_host("value1"), std::invalid_argument,
                    msg.c_str());
  EXPECT_TRUE(ssh_config.has_host());
  EXPECT_STREQ("value", ssh_config.get_host().c_str());
  EXPECT_NO_THROW(ssh_config.clear_host());
  EXPECT_FALSE(ssh_config.has_host());
  msg = "The ssh_connection option '";
  msg.append(mysqlshdk::db::kHost).append("' has no value.");
  EXPECT_THROW_LIKE(ssh_config.get_host(), std::invalid_argument, msg.c_str());
}

TEST_F(Ssh_connection_options_t, port_functions) {
  Ssh_connection_options ssh_config;
  std::string msg;

  EXPECT_NO_THROW(ssh_config.set_port(22));

  msg = "The option port '22' is already defined.";
  EXPECT_THROW_LIKE(ssh_config.set_port(22), std::invalid_argument,
                    msg.c_str());
  EXPECT_TRUE(ssh_config.has_port());
  EXPECT_EQ(22, ssh_config.get_port());
  EXPECT_NO_THROW(ssh_config.clear_port());
  EXPECT_FALSE(ssh_config.has_port());
  msg = "The option '";
  msg.append(mysqlshdk::db::kPort).append("' has no value.");
  EXPECT_THROW_LIKE(ssh_config.get_port(), std::invalid_argument, msg.c_str());
}

TEST_F(Ssh_connection_options_t, key_file_functions) {
  Ssh_connection_options ssh_config;
  std::string msg;

  auto missing_file = shcore::path::join_path(
      shcore::path::dirname(m_key_with_pw), "missing_file");
  msg = "The path '" + missing_file + "' doesn't exist";
  EXPECT_THROW_LIKE(ssh_config.set_key_file(missing_file),
                    std::invalid_argument, msg.c_str());

#ifndef _WIN32
  // We cannot test this on Windows since we can't create a file with such wild
  // permissions.
  shcore::ch_mod(m_key_with_pw, 0666);
  msg = "Invalid SSH Identity file: '" + m_key_with_pw +
        "' has insecure permissions. Expected u+rw only: Permission "
        "denied";
  EXPECT_THROW_LIKE(ssh_config.set_key_file(m_key_with_pw),
                    std::invalid_argument, msg.c_str());

  shcore::ch_mod(m_key_with_pw, 0600);
#endif
  EXPECT_NO_THROW(ssh_config.set_key_file(m_key_with_pw));

  EXPECT_TRUE(ssh_config.get_key_encrypted());
  EXPECT_TRUE(ssh_config.has_key_file());
  EXPECT_NO_THROW(ssh_config.clear_key_file());

  msg = "Invalid SSH Identity file: Path '" + shcore::get_user_config_path() +
        "' is not a file";
  EXPECT_THROW_LIKE(ssh_config.set_key_file(shcore::get_user_config_path()),
                    std::invalid_argument, msg.c_str());

#ifndef _WIN32
  // it's not possible to test this on Windows since one cannot create a file
  // that user has no access to as we will be unable to remove this file without
  // hacking test environment.
  shcore::ch_mod(m_key_with_pw, 0077);
  msg = "Invalid SSH Identity file: Unable to open file '" + m_key_with_pw +
        "': Permission denied";
  EXPECT_THROW_LIKE(ssh_config.set_key_file(m_key_with_pw),
                    std::invalid_argument, msg.c_str());
#endif
  msg = "The ssh_connection option 'ssh-identity-file' has no value.";
  EXPECT_THROW_LIKE(ssh_config.get_key_file(), std::invalid_argument,
                    msg.c_str());

  auto relative_path = shcore::path::join_path("..", "sample", "file");
  msg =
      "Invalid SSH Identity file: Only absolute paths are accepted, the path "
      "'" +
      relative_path + "' looks like relative one.";
  EXPECT_THROW_LIKE(ssh_config.set_key_file(relative_path),
                    std::invalid_argument, msg.c_str());

  shcore::ch_mod(m_key_without_pw, 0600);
  EXPECT_NO_THROW(ssh_config.set_key_file(m_key_without_pw));
  EXPECT_FALSE(ssh_config.get_key_encrypted());

  // Empty value is accepted and clears the option
  EXPECT_NO_THROW(ssh_config.set_key_file(""));
  EXPECT_FALSE(ssh_config.has_key_file());
}

TEST_F(Ssh_connection_options_t, remote_host_functions) {
  Ssh_connection_options ssh_config;
  std::string msg;

  EXPECT_NO_THROW(ssh_config.set_remote_host("value"));
  msg = "The ssh_connection option '";
  msg.append(mysqlshdk::db::kSshRemoteHost);
  msg.append("' is already defined as 'value'.");
  EXPECT_THROW_LIKE(ssh_config.set_remote_host("value1"), std::invalid_argument,
                    msg.c_str());

  EXPECT_TRUE(ssh_config.has_remote_host());
  EXPECT_STREQ("value", ssh_config.get_remote_host().c_str());
  EXPECT_NO_THROW(ssh_config.clear_remote_host());
  EXPECT_FALSE(ssh_config.has_remote_host());
  msg = "The ssh_connection option '";
  msg.append(mysqlshdk::db::kSshRemoteHost).append("' has no value.");
  EXPECT_THROW_LIKE(ssh_config.get_remote_host(), std::invalid_argument,
                    msg.c_str());
}

TEST_F(Ssh_connection_options_t, remote_port_functions) {
  Ssh_connection_options ssh_config;
  std::string msg;

  EXPECT_NO_THROW(ssh_config.set_remote_port(3306));
  msg = "The option ssh-remote-port '3306' is already defined.";
  EXPECT_THROW_LIKE(ssh_config.set_remote_port(3307), std::invalid_argument,
                    msg.c_str());
  EXPECT_TRUE(ssh_config.has_remote_port());
  EXPECT_EQ(3306, ssh_config.get_remote_port());
  EXPECT_NO_THROW(ssh_config.clear_remote_port());
  EXPECT_FALSE(ssh_config.has_remote_port());
  msg = "Internal option 'ssh-remote-port' has no value.";
  EXPECT_THROW_LIKE(ssh_config.get_remote_port(), std::runtime_error,
                    msg.c_str());
}

TEST_F(Ssh_connection_options_t, local_port_functions) {
  Ssh_connection_options ssh_config;
  std::string msg;

  EXPECT_NO_THROW(ssh_config.set_local_port(3306));
  msg = "The option ssh-local-port '3306' is already defined.";
  EXPECT_THROW_LIKE(ssh_config.set_local_port(3307), std::invalid_argument,
                    msg.c_str());
  EXPECT_TRUE(ssh_config.has_local_port());
  EXPECT_EQ(3306, ssh_config.get_local_port());
  EXPECT_NO_THROW(ssh_config.clear_local_port());
  EXPECT_FALSE(ssh_config.has_local_port());
  msg = "Internal option 'ssh-local-port' has no value.";
  EXPECT_THROW_LIKE(ssh_config.get_local_port(), std::runtime_error,
                    msg.c_str());
}

TEST_F(Ssh_connection_options_t, config_file_functions) {
  Ssh_connection_options ssh_config;
  std::string msg;

  auto missing_file = shcore::path::join_path(
      shcore::path::dirname(m_config_file), "missing_file");
  msg = "Invalid SSH configuration file: The path '" + missing_file +
        "' doesn't exist";
  EXPECT_THROW_LIKE(ssh_config.set_config_file(missing_file),
                    std::invalid_argument, msg.c_str());

#ifndef _WIN32
  // We cannot test this on Windows since we can't create a file with such wild
  // permissions.
  shcore::ch_mod(m_config_file, 0666);
  msg = "Invalid SSH configuration file: '" + m_config_file +
        "' has insecure permissions. Expected u+rw only: Permission "
        "denied";
  EXPECT_THROW_LIKE(ssh_config.set_config_file(m_config_file),
                    std::invalid_argument, msg.c_str());

  shcore::ch_mod(m_config_file, 0600);
#endif
  EXPECT_NO_THROW(ssh_config.set_config_file(m_config_file));

  EXPECT_TRUE(ssh_config.has_config_file());
  EXPECT_NO_THROW(ssh_config.clear_config_file());

#ifndef _WIN32
  // it's not possible to test this on Windows since one cannot create a file
  // that user has no access to as we will be unable to remove this file without
  // hacking test environment.
  shcore::ch_mod(m_config_file, 0077);
  msg = "Invalid SSH configuration file: Unable to open file '" +
        m_config_file + "': Permission denied";
  EXPECT_THROW_LIKE(ssh_config.set_config_file(m_config_file),
                    std::invalid_argument, msg.c_str());
#endif
  msg = "The ssh_connection option 'ssh-config-file' has no value.";
  EXPECT_THROW_LIKE(ssh_config.get_config_file(), std::invalid_argument,
                    msg.c_str());

  auto relative_path = shcore::path::join_path("..", "sample", "file");
  msg =
      "Invalid SSH configuration file: Only absolute paths are accepted, the "
      "path '" +
      relative_path + "' looks like relative one.";
  EXPECT_THROW_LIKE(ssh_config.set_config_file(relative_path),
                    std::invalid_argument, msg.c_str());

  // Empty value is accepted and clears the option
  EXPECT_NO_THROW(ssh_config.set_config_file(""));
  EXPECT_FALSE(ssh_config.has_config_file());
}
}  // namespace testing
