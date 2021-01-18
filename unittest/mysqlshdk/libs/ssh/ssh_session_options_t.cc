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

#include <iostream>
#include "mysqlshdk/libs/ssh/ssh_session_options.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "unittest/gtest_clean.h"
#include "unittest/test_utils/shell_base_test.h"

using mysqlshdk::ssh::Ssh_session_options;
extern "C" const char *g_test_home;

namespace testing {

class Ssh_session_options_t : public tests::Shell_base_test {
  void SetUp() override {
    m_tmpdir = getenv("TMPDIR");
    m_ssh_files = shcore::path::join_path(m_tmpdir, "ssh_files");
    if (shcore::is_folder(m_ssh_files)) {
      shcore::remove_directory(m_ssh_files, true);
    }
    shcore::create_directory(m_ssh_files, true, 0700);

    m_test_config = shcore::path::join_path(m_ssh_files, "test_config");

    auto input_config = shcore::path::join_path(g_test_home, "data", "docker",
                                                "sshd_pw_login", "test_config");

    std::ifstream input(input_config.c_str());
    std::ofstream output(m_test_config, std::ofstream::out);
    if (!input) {
      throw std::runtime_error(
          "I/O Error when trying to open for reading test_config");
    }
    if (!output) {
      throw std::runtime_error(
          "I/O Error when trying to open for writing test_config");
    }

    m_key_file = shcore::path::join_path(g_test_home, "data", "docker", "keys",
                                         "no_password");

    const std::string identity_line = "<<IdentityFile>>";
    const auto len = m_key_file.length();
    std::string line;
    while (std::getline(input, line)) {
      auto pos = line.find(identity_line);
      if (pos != std::string::npos) {
        line.replace(pos, len, m_key_file);
      }
      output << line << std::endl;
    }
  }

 protected:
  std::string m_ssh_files;
  std::string m_tmpdir;
  std::string m_test_config;
  std::string m_key_file;
};

TEST_F(Ssh_session_options_t, default_initialization) {
  {
    Ssh_session_options opts(m_test_config, "example");
    EXPECT_STREQ("example.com", opts.get_host().c_str());
    EXPECT_STREQ(m_key_file.c_str(), opts.get_identity_file().c_str());
    EXPECT_STREQ("exampleUser", opts.get_user().c_str());
    EXPECT_STREQ("/tmp/example/known_hosts", opts.get_known_hosts().c_str());
    EXPECT_EQ(1234, opts.get_port());
  }

  {
    Ssh_session_options opts(m_test_config, "exampleNoKey");
    EXPECT_STREQ("example-no-key.com", opts.get_host().c_str());
    EXPECT_STREQ("", opts.get_identity_file().c_str());
    EXPECT_STREQ("exampleUserNoKey", opts.get_user().c_str());
    EXPECT_STREQ("/tmp/example/known_hosts_no_key",
                 opts.get_known_hosts().c_str());
    EXPECT_EQ(22, opts.get_port());
  }
}
}  // namespace testing
