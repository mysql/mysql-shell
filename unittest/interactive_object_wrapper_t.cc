/*
* Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "gtest/gtest_prod.h"
#include "modules/interactive_object_wrapper.h"
#include "test_utils.h"

namespace shcore {
class Interactive_object_wrapper_modified : public Interactive_object_wrapper {
public:
  Interactive_object_wrapper_modified(Shell_core& shell_core) :
  Interactive_object_wrapper("test", shell_core) {   };

private:
  FRIEND_TEST(Interactive_object_wrapper_test, prompt_answer);

};

class Interactive_object_wrapper_test : public Shell_core_test_wrapper {
  public:
  virtual void SetUp() {
    Shell_core_test_wrapper::SetUp();
  }
};

TEST_F(Interactive_object_wrapper_test, prompt_answer) {
  Interactive_object_wrapper_modified wrap(*(_interactive_shell->shell_context()));

  output_handler.prompts.push_back({"*", ""});
  Prompt_answer ans = wrap.prompt("ques?");
  MY_EXPECT_STDOUT_CONTAINS("ques? [Y|n]");
  EXPECT_EQ(ans, Prompt_answer::YES);

  output_handler.prompts.push_back({"*", "y"});
  ans = wrap.prompt("ques?");
  MY_EXPECT_STDOUT_CONTAINS("ques? [Y|n]");
  EXPECT_EQ(ans, Prompt_answer::YES);

  output_handler.prompts.push_back({"*", "Y"});
  ans = wrap.prompt("ques?", Prompt_answer::YES);
  MY_EXPECT_STDOUT_CONTAINS("ques? [Y|n]");
  EXPECT_EQ(ans, Prompt_answer::YES);

  output_handler.prompts.push_back({"*", "yes"});
  ans = wrap.prompt("ques?");
  MY_EXPECT_STDOUT_CONTAINS("ques? [Y|n]");
  EXPECT_EQ(ans, Prompt_answer::YES);

  output_handler.prompts.push_back({"*", "YES"});
  ans = wrap.prompt("ques?");
  MY_EXPECT_STDOUT_CONTAINS("ques? [Y|n]");
  EXPECT_EQ(ans, Prompt_answer::YES);

  output_handler.prompts.push_back({"*", "Yes"});
  ans = wrap.prompt("ques?");
  MY_EXPECT_STDOUT_CONTAINS("ques? [Y|n]");
  EXPECT_EQ(ans, Prompt_answer::YES);

  output_handler.prompts.push_back({"*", "yEs"});
  ans = wrap.prompt("ques?");
  MY_EXPECT_STDOUT_CONTAINS("ques? [Y|n]");
  EXPECT_EQ(ans, Prompt_answer::YES);

  output_handler.prompts.push_back({"*", ""});
  ans = wrap.prompt("ques?", Prompt_answer::NO);
  MY_EXPECT_STDOUT_CONTAINS("ques? [y|N]");
  EXPECT_EQ(ans, Prompt_answer::NO);

  output_handler.prompts.push_back({"*", "n"});
  ans = wrap.prompt("ques?", Prompt_answer::NO);
  MY_EXPECT_STDOUT_CONTAINS("ques? [y|N]");
  EXPECT_EQ(ans, Prompt_answer::NO);

  output_handler.prompts.push_back({"*", "N"});
  ans = wrap.prompt("ques?", Prompt_answer::NO);
  MY_EXPECT_STDOUT_CONTAINS("ques? [y|N]");
  EXPECT_EQ(ans, Prompt_answer::NO);

  output_handler.prompts.push_back({"*", "NO"});
  ans = wrap.prompt("ques?", Prompt_answer::NO);
  MY_EXPECT_STDOUT_CONTAINS("ques? [y|N]");
  EXPECT_EQ(ans, Prompt_answer::NO);

  output_handler.prompts.push_back({"*", "No"});
  ans = wrap.prompt("ques?", Prompt_answer::NO);
  MY_EXPECT_STDOUT_CONTAINS("ques? [y|N]");
  EXPECT_EQ(ans, Prompt_answer::NO);

  output_handler.prompts.push_back({"*", "nO"});
  ans = wrap.prompt("ques?", Prompt_answer::NO);
  MY_EXPECT_STDOUT_CONTAINS("ques? [y|N]");
  EXPECT_EQ(ans, Prompt_answer::NO);

  output_handler.prompts.push_back({"*", "sfd"});
  output_handler.prompts.push_back({"*", ""});
  ans = wrap.prompt("ques?", Prompt_answer::NO);
  MY_EXPECT_STDOUT_CONTAINS("ques? [y|N]");
  MY_EXPECT_STDOUT_CONTAINS("Invalid answer!");

  output_handler.prompts.push_back({"*", "      "});
  output_handler.prompts.push_back({"*", ""});
  ans = wrap.prompt("ques?", Prompt_answer::YES);
  MY_EXPECT_STDOUT_CONTAINS("ques? [Y|n]");
  MY_EXPECT_STDOUT_CONTAINS("Invalid answer!");

  output_handler.prompts.push_back({"*", ""});
  ans = wrap.prompt("ques?", Prompt_answer::NO);
  EXPECT_EQ(ans, Prompt_answer::NO);
}
}

