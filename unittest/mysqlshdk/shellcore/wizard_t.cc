/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "unittest/gprod_clean.h"
#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"

#include "mysqlshdk/shellcore/shell_console.h"
#include "mysqlshdk/shellcore/wizard.h"
#include "unittest/test_utils/shell_test_env.h"

namespace shcore {
namespace wizard {

class Wizard_test : public ::testing::Test {
 public:
  Wizard_test()
      : ::testing::Test(),
        m_options{std::make_shared<mysqlsh::Shell_options>(0, nullptr)},
        m_console{
            std::make_shared<mysqlsh::Shell_console>(&output_handler.deleg)} {}

  virtual void SetUp() {}

 protected:
  Shell_test_output_handler output_handler;
  mysqlsh::Scoped_shell_options m_options;
  mysqlsh::Scoped_console m_console;
};

TEST_F(Wizard_test, step_addition) {
  Wizard wizard;

  {
    wizard.add_prompt("prompt");
    auto step = wizard.m_steps["prompt"].get();
    EXPECT_EQ(Prompt_type::PROMPT, step->type);
    auto prompt = dynamic_cast<Wizard::Wizard_prompt *>(step);
    EXPECT_NE(prompt, nullptr);
    auto confirm = dynamic_cast<Wizard::Wizard_confirm *>(step);
    EXPECT_EQ(confirm, nullptr);
    auto select = dynamic_cast<Wizard::Wizard_select *>(step);
    EXPECT_EQ(select, nullptr);
  }
  {
    wizard.add_confirm("confirm");
    auto step = wizard.m_steps["confirm"].get();
    EXPECT_EQ(Prompt_type::CONFIRM, step->type);
    auto prompt = dynamic_cast<Wizard::Wizard_prompt *>(step);
    EXPECT_EQ(prompt, nullptr);
    auto confirm = dynamic_cast<Wizard::Wizard_confirm *>(step);
    EXPECT_NE(confirm, nullptr);
    auto select = dynamic_cast<Wizard::Wizard_select *>(step);
    EXPECT_EQ(select, nullptr);
  }
  {
    wizard.add_select("selection");
    auto step = wizard.m_steps["selection"].get();
    EXPECT_EQ(Prompt_type::SELECTION, step->type);
    auto prompt = dynamic_cast<Wizard::Wizard_prompt *>(step);
    EXPECT_EQ(prompt, nullptr);
    auto confirm = dynamic_cast<Wizard::Wizard_confirm *>(step);
    EXPECT_EQ(confirm, nullptr);
    auto select = dynamic_cast<Wizard::Wizard_select *>(step);
    EXPECT_NE(select, nullptr);
  }

  // Attempt to add a duplicated step
  EXPECT_THROW_LIKE(wizard.add_prompt("selection"), std::runtime_error,
                    "Step 'selection' already exists.");
}

TEST_F(Wizard_test, prompt_step) {
  Wizard wizard;
  bool validation_called = false;
  bool leave_cb_called = false;
  std::string answer = "my_answer";

  auto &step =
      wizard.add_prompt("step_id")
          .prompt("This is the sample prompt.")
          .description(
              {"These details give an explanation about the following prompt."})
          .validator(
              [&validation_called](const std::string &value) -> std::string {
                validation_called = true;
                return "";
              })
          .on_leave([&leave_cb_called, answer](const std::string &value,
                                               Wizard *wizard) {
            leave_cb_called = true;
            EXPECT_STREQ(answer.c_str(), value.c_str());
          });

  EXPECT_THROW_LIKE(wizard.verify("step_id"), std::logic_error,
                    "Missing link 'K_NEXT' for step 'step_id'");
  step.link(K_NEXT, K_DONE);
  EXPECT_THROW_LIKE(step.link(K_NEXT, "Whatever");
                    , std::logic_error,
                    "Link 'K_NEXT' for step 'step_id' is already defined.");

  EXPECT_NO_THROW(wizard.verify("step_id"));

  output_handler.feed_to_prompt(answer);
  wizard.execute("step_id");

  MY_EXPECT_STDOUT_CONTAINS(
      "These details give an explanation about the following prompt.");
  MY_EXPECT_STDOUT_CONTAINS("This is the sample prompt.");
  EXPECT_TRUE(validation_called);
  EXPECT_TRUE(leave_cb_called);
  EXPECT_STREQ(answer.c_str(), wizard["step_id"].c_str());
}

TEST_F(Wizard_test, confirm_step) {
  Wizard wizard;
  bool leave_cb_called = false;

  // Testing with default answer which is NO
  auto &step =
      wizard.add_confirm("step_id")
          .prompt("This is the sample prompt.")
          .description(
              {"These details give an explanation about the following prompt."})
          .on_leave(
              [&leave_cb_called](const std::string &value, Wizard *wizard) {
                leave_cb_called = true;
                EXPECT_STREQ(K_NO, value.c_str());
              });

  EXPECT_THROW_LIKE(wizard.verify("step_id"), std::logic_error,
                    "Missing link, either 'K_YES' or 'K_NEXT' must be defined "
                    "for step 'step_id'.");

  step.link(K_YES, K_DONE);
  EXPECT_THROW_LIKE(wizard.verify("step_id"), std::logic_error,
                    "Missing link, either 'K_NO' or 'K_NEXT' must be defined "
                    "for step 'step_id'.");

  step.link(K_NO, K_DONE);
  EXPECT_NO_THROW(wizard.verify("step_id"));

  step.unlink(K_YES);
  step.unlink(K_NO);
  step.link(K_NEXT, K_DONE);
  EXPECT_NO_THROW(wizard.verify("step_id"));

  output_handler.feed_to_prompt("");
  wizard.execute("step_id");

  MY_EXPECT_STDOUT_CONTAINS(
      "These details give an explanation about the following prompt.");
  MY_EXPECT_STDOUT_CONTAINS("This is the sample prompt. [y/N]: ");
  EXPECT_TRUE(leave_cb_called);
  EXPECT_STREQ(K_NO, wizard["step_id"].c_str());

  // Test user overriding the default answer of NO to YES
  wizard.reset();
  leave_cb_called = false;
  step.on_leave([&leave_cb_called](const std::string &value, Wizard *wizard) {
    leave_cb_called = true;
    EXPECT_STREQ(K_YES, value.c_str());
  });

  output_handler.feed_to_prompt("y");
  wizard.execute("step_id");

  MY_EXPECT_STDOUT_CONTAINS(
      "These details give an explanation about the following prompt.");
  MY_EXPECT_STDOUT_CONTAINS("This is the sample prompt. [y/N]: ");
  EXPECT_TRUE(leave_cb_called);
  EXPECT_STREQ(K_YES, wizard["step_id"].c_str());

  // Test specifying a different default answer to YES
  wizard.reset();
  leave_cb_called = false;
  step.default_answer(Prompt_answer::YES);

  output_handler.feed_to_prompt("");
  wizard.execute("step_id");

  MY_EXPECT_STDOUT_CONTAINS(
      "These details give an explanation about the following prompt.");
  MY_EXPECT_STDOUT_CONTAINS("This is the sample prompt. [Y/n]: ");
  EXPECT_TRUE(leave_cb_called);
  EXPECT_STREQ(K_YES, wizard["step_id"].c_str());

  // Now tests overriding the default answer of YES to NO
  wizard.reset();
  leave_cb_called = false;
  step.on_leave([&leave_cb_called](const std::string &value, Wizard *wizard) {
    leave_cb_called = true;
    EXPECT_STREQ(K_NO, value.c_str());
  });

  output_handler.feed_to_prompt("n");
  wizard.execute("step_id");

  MY_EXPECT_STDOUT_CONTAINS(
      "These details give an explanation about the following prompt.");
  MY_EXPECT_STDOUT_CONTAINS("This is the sample prompt. [Y/n]: ");
  EXPECT_TRUE(leave_cb_called);
  EXPECT_STREQ(K_NO, wizard["step_id"].c_str());
}

TEST_F(Wizard_test, select_step) {
  Wizard wizard;
  bool validation_called = false;
  bool leave_cb_called = false;

  // Testing with default answer which is NO
  auto &step =
      wizard.add_select("step_id")
          .prompt("This is the sample prompt.")
          .description(
              {"These details give an explanation about the following prompt."})
          .validator(
              [&validation_called](const std::string &value) -> std::string {
                validation_called = true;
                EXPECT_STREQ("TWO", value.c_str());
                return "";
              })
          .on_leave(
              [&leave_cb_called](const std::string &value, Wizard *wizard) {
                leave_cb_called = true;
                EXPECT_STREQ("TWO", value.c_str());
              });

  EXPECT_THROW_LIKE(wizard.verify("step_id"), std::logic_error,
                    "No items defined for select step 'step_id'.");
  step.items({"ONE", "TWO", "THREE"});
  step.default_option(4);
  EXPECT_THROW_LIKE(wizard.verify("step_id"), std::logic_error,
                    "Default option for select step 'step_id' is not valid.");

  step.default_option(0);
  EXPECT_THROW_LIKE(wizard.verify("step_id"), std::logic_error,
                    "Missing link, either 'K_NEXT' or a link for every option "
                    "must be defined for step 'step_id'.");
  step.link("ONE", K_DONE);
  step.link("TWO", K_DONE);
  step.link("THREE", K_DONE);
  EXPECT_NO_THROW(wizard.verify("step_id"));

  step.unlink("THREE");
  EXPECT_THROW_LIKE(wizard.verify("step_id"), std::logic_error,
                    "Missing link, either 'K_NEXT' or a link for every option "
                    "must be defined for step 'step_id'.");

  step.link(K_NEXT, K_DONE);
  EXPECT_NO_THROW(wizard.verify("step_id"));

  output_handler.feed_to_prompt("2");
  wizard.execute("step_id");

  MY_EXPECT_STDOUT_CONTAINS(
      "These details give an explanation about the following prompt.");
  MY_EXPECT_STDOUT_CONTAINS("This is the sample prompt.");
  EXPECT_TRUE(validation_called);
  EXPECT_TRUE(leave_cb_called);
  EXPECT_STREQ("TWO", wizard["step_id"].c_str());

  validation_called = leave_cb_called = false;
  step.default_option(2);
  output_handler.feed_to_prompt("");
  wizard.execute("step_id");

  MY_EXPECT_STDOUT_CONTAINS(
      "These details give an explanation about the following prompt.");
  MY_EXPECT_STDOUT_CONTAINS("This is the sample prompt. [2]: ");
  EXPECT_EQ(1, leave_cb_called);
  EXPECT_EQ(1, validation_called);
  EXPECT_STREQ("TWO", wizard["step_id"].c_str());

  validation_called = leave_cb_called = false;
  output_handler.feed_to_prompt("something else");
  output_handler.feed_to_prompt("2");
  wizard.execute("step_id");
  MY_EXPECT_STDOUT_CONTAINS("WARNING: Invalid option selected.");
  EXPECT_TRUE(validation_called);
  EXPECT_TRUE(leave_cb_called);
  EXPECT_STREQ("TWO", wizard["step_id"].c_str());

  step.allow_custom(true);
  step.validator([&validation_called](const std::string &value) -> std::string {
        validation_called = true;
        EXPECT_STREQ("custom answer", value.c_str());
        return "";
      })
      .on_leave([&leave_cb_called](const std::string &value, Wizard *wizard) {
        leave_cb_called = true;
        EXPECT_STREQ("custom answer", value.c_str());
      });

  validation_called = leave_cb_called = false;
  output_handler.feed_to_prompt("5");
  output_handler.feed_to_prompt("custom answer");
  wizard.execute("step_id");
  MY_EXPECT_STDOUT_CONTAINS("WARNING: Invalid option selected.");
  EXPECT_TRUE(validation_called);
  EXPECT_TRUE(leave_cb_called);
  EXPECT_STREQ("custom answer", wizard["step_id"].c_str());
}
}  // namespace wizard
}  // namespace shcore
