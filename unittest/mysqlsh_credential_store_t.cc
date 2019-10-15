/*
 * Copyright (c) 2018, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "unittest/gtest_clean.h"

#include "unittest/test_utils.h"
#include "unittest/test_utils/command_line_test.h"

namespace tests {

class Mysqlsh_credential_store : public tests::Command_line_test {
 protected:
#ifdef HAVE_V8
  const std::string mode = "--js";
#else
  const std::string mode = "--py";
#endif
 public:
  void SetUp() override {
    shcore::unsetenv("MYSQLSH_CREDENTIAL_STORE_HELPER");
    shcore::unsetenv("MYSQLSH_CREDENTIAL_STORE_SAVE_PASSWORDS");

    Command_line_test::SetUp();
  }

  void TearDown() override {
    Command_line_test::TearDown();

    shcore::setenv("MYSQLSH_CREDENTIAL_STORE_HELPER", "<disabled>");
    shcore::unsetenv("MYSQLSH_CREDENTIAL_STORE_SAVE_PASSWORDS");
    shcore::unsetenv("MYSQLSH_TERM_COLOR_MODE");
  }
};

TEST_F(Mysqlsh_credential_store, default_helper) {
  execute({_mysqlsh, mode.c_str(), "-e",
           "print(shell.options['credentialStore.helper'])", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("default");
  wipe_out();
}

TEST_F(Mysqlsh_credential_store, cmdline_plaintext_helper) {
  execute({_mysqlsh, "--credential-store-helper=plaintext", mode.c_str(), "-e",
           "print(shell.options['credentialStore.helper'])", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("plaintext");
  wipe_out();
}

TEST_F(Mysqlsh_credential_store, cmdline_disabled_helper) {
  execute({_mysqlsh, "--credential-store-helper=<disabled>", mode.c_str(), "-e",
           "print(shell.options['credentialStore.helper'])", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("<disabled>");
  MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS(
      "Credential store mechanism is going to be disabled.");
  wipe_out();
}

TEST_F(Mysqlsh_credential_store, cmdline_invalid_helper) {
  execute({_mysqlsh, "--credential-store-helper=unknown", mode.c_str(), "-e",
           "print(shell.options['credentialStore.helper'])", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "Failed to initialize the user-specified helper \"unknown\": Credential "
      "helper named \"unknown\" could not be found or is invalid. "
      "See logs for more information.");
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "Credential store mechanism is going to be disabled.");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("<invalid>");
  wipe_out();
}

TEST_F(Mysqlsh_credential_store, env_plaintext_helper) {
  shcore::setenv("MYSQLSH_CREDENTIAL_STORE_HELPER", "plaintext");

  execute({_mysqlsh, mode.c_str(), "-e",
           "print(shell.options['credentialStore.helper'])", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("plaintext");
  wipe_out();
}

TEST_F(Mysqlsh_credential_store, env_disabled_helper) {
  shcore::setenv("MYSQLSH_CREDENTIAL_STORE_HELPER", "<disabled>");

  execute({_mysqlsh, mode.c_str(), "-e",
           "print(shell.options['credentialStore.helper'])", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("<disabled>");
  MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS(
      "Credential store mechanism is going to be disabled.");
  wipe_out();
}

TEST_F(Mysqlsh_credential_store, env_invalid_helper) {
  shcore::setenv("MYSQLSH_CREDENTIAL_STORE_HELPER", "unknown");

  execute({_mysqlsh, mode.c_str(), "-e",
           "print(shell.options['credentialStore.helper'])", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "Failed to initialize the user-specified helper \"unknown\": Credential "
      "helper named \"unknown\" could not be found or is invalid. "
      "See logs for more information.");
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "Credential store mechanism is going to be disabled.");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("<invalid>");
  wipe_out();
}

TEST_F(Mysqlsh_credential_store, default_save_passwords) {
  execute({_mysqlsh, mode.c_str(), "-e",
           "print(shell.options['credentialStore.savePasswords'])", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("prompt");
  wipe_out();
}

TEST_F(Mysqlsh_credential_store, cmdline_never_save_passwords) {
  execute({_mysqlsh, "--save-passwords=never", mode.c_str(), "-e",
           "print(shell.options['credentialStore.savePasswords'])", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("never");
  MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS("must be one of");
  wipe_out();
}

TEST_F(Mysqlsh_credential_store, cmdline_always_save_passwords) {
  execute({_mysqlsh, "--save-passwords=always", mode.c_str(), "-e",
           "print(shell.options['credentialStore.savePasswords'])", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("always");
  MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS("must be one of");
  wipe_out();
}

TEST_F(Mysqlsh_credential_store, cmdline_prompt_save_passwords) {
  execute({_mysqlsh, "--save-passwords=prompt", mode.c_str(), "-e",
           "print(shell.options['credentialStore.savePasswords'])", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("prompt");
  MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS("must be one of");
  wipe_out();
}

TEST_F(Mysqlsh_credential_store, cmdline_invalid_save_passwords) {
  execute({_mysqlsh, "--save-passwords=unknown", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "The option --save-passwords must be one of: always, prompt, never.");
  wipe_out();
}

TEST_F(Mysqlsh_credential_store, env_never_save_passwords) {
  shcore::setenv("MYSQLSH_CREDENTIAL_STORE_SAVE_PASSWORDS", "never");

  execute({_mysqlsh, mode.c_str(), "-e",
           "print(shell.options['credentialStore.savePasswords'])", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("never");
  MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS("must be one of");
  wipe_out();
}

TEST_F(Mysqlsh_credential_store, env_always_save_passwords) {
  shcore::setenv("MYSQLSH_CREDENTIAL_STORE_SAVE_PASSWORDS", "always");

  execute({_mysqlsh, mode.c_str(), "-e",
           "print(shell.options['credentialStore.savePasswords'])", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("always");
  MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS("must be one of");
  wipe_out();
}

TEST_F(Mysqlsh_credential_store, env_prompt_save_passwords) {
  shcore::setenv("MYSQLSH_CREDENTIAL_STORE_SAVE_PASSWORDS", "prompt");

  execute({_mysqlsh, mode.c_str(), "-e",
           "print(shell.options['credentialStore.savePasswords'])", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("prompt");
  MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS("must be one of");
  wipe_out();
}

TEST_F(Mysqlsh_credential_store, env_invalid_save_passwords) {
  shcore::setenv("MYSQLSH_CREDENTIAL_STORE_SAVE_PASSWORDS", "unknown");

  execute({_mysqlsh, nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "The option credentialStore.savePasswords must be one of: always, "
      "prompt, never.");
  wipe_out();
}

TEST_F(Mysqlsh_credential_store, bug_28216485) {
  shcore::setenv("MYSQLSH_CREDENTIAL_STORE_HELPER", "unknown");
  shcore::setenv("MYSQLSH_TERM_COLOR_MODE", "nocolor");

  // If shell is running in "nocolor" mode output should not contain VTERM
  // escape characters.
  // This test ensures that color capability is detected before any other
  // output is printed.

  execute({_mysqlsh, mode.c_str(), "-e",
           "print(shell.options['credentialStore.helper'])", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "ERROR: Failed to initialize the user-specified helper \"unknown\": "
      "Credential helper named \"unknown\" could not be found or is invalid. "
      "See logs for more information.");
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "Credential store mechanism is going to be disabled.");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("<invalid>");
  wipe_out();
}

}  // namespace tests
