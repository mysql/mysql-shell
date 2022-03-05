/*
 * Copyright (c) 2018, 2022, Oracle and/or its affiliates.
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

#include <list>
#include <string>

#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"

#include "mysqlshdk/shellcore/shell_console.h"
#include "src/mysqlsh/prompt_handler.h"

namespace mysqlsh {

struct Test_data {
  std::string output;
  std::list<std::string> input;
};

namespace {
bool print(void *user_data, const char *text) {
  Test_data *data = static_cast<Test_data *>(user_data);
  if (!data->output.empty()) data->output.append("||");
  data->output.append(text);
  return true;
}

shcore::Prompt_result prompt(void *user_data, const char *prompt,
                             const shcore::prompt::Prompt_options &options,
                             std::string *ret) {
  Test_data *data = static_cast<Test_data *>(user_data);

  return mysqlsh::Prompt_handler(
             [data](bool /*is_password*/, const char *prompt_text,
                    std::string *answer) -> shcore::Prompt_result {
               if (data->input.empty()) {
                 ADD_FAILURE() << "Unexpected prompt: " << prompt_text;
                 return shcore::Prompt_result::CTRL_D;
               }
               if (!data->output.empty()) data->output.append("||");
               data->output.append(prompt_text);
               *answer = data->input.front();
               data->input.pop_front();
               return shcore::Prompt_result::Ok;
             })
      .handle_prompt(prompt, options, ret);

  // Implementation handles the default value
  if (ret->empty() && options.default_value) {
    *ret = options.default_value.as_string();
  }
}

}  // namespace

class Shell_console_test : public Shell_core_test_wrapper {};

TEST_F(Shell_console_test, prompt) {
  auto data = std::make_unique<Test_data>();
  shcore::Interpreter_delegate deleg(data.get(), print, prompt, nullptr,
                                     nullptr);

  mysqlsh::Scoped_console scoped_console{
      std::make_shared<Shell_console>(&deleg)};
  const Shell_console &console(
      *std::dynamic_pointer_cast<Shell_console>(scoped_console.get()));
  Prompt_answer answer;

  data->input.push_back("");
  answer = console.confirm("Really?");
  EXPECT_EQ(Prompt_answer::NO, answer);
  EXPECT_EQ("Really? [y/N]: ", data->output);
  data->output.clear();

  data->input.push_back("");
  answer = console.confirm("Really?", Prompt_answer::YES);
  EXPECT_EQ(Prompt_answer::YES, answer);
  EXPECT_EQ("Really? [Y/n]: ", data->output);
  data->output.clear();

  data->input.push_back("");
  data->input.push_back("y");
  answer = console.confirm("Really?", Prompt_answer::NONE);
  EXPECT_EQ(Prompt_answer::YES, answer);
  EXPECT_EQ(
      "Really? [y/n]: ||Please pick an option out of [y/n]: ||Really? [y/n]: ",
      data->output);
  data->output.clear();

  data->input.push_back("");
  answer = console.confirm("Really?", Prompt_answer::YES, "&Yes", "&No");
  EXPECT_EQ(Prompt_answer::YES, answer);
  EXPECT_EQ("Really? [Y/n]: ", data->output);
  data->output.clear();

  data->input.push_back("");
  answer = console.confirm("Really?", Prompt_answer::NO, "&Yes", "&No");
  EXPECT_EQ(Prompt_answer::NO, answer);
  EXPECT_EQ("Really? [y/N]: ", data->output);
  data->output.clear();

  data->input.push_back("");
  answer = console.confirm("Really?", Prompt_answer::NO, "Yes", "No");
  EXPECT_EQ(Prompt_answer::NO, answer);
  EXPECT_EQ("Really? Yes/No (default No): ", data->output);
  data->output.clear();

  data->input.push_back("");
  answer =
      console.confirm("Really?", Prompt_answer::YES, "&Yes", "&No", "&Cancel");
  EXPECT_EQ(Prompt_answer::YES, answer);
  EXPECT_EQ("Really? [Y]es/[N]o/[C]ancel (default Yes): ", data->output);
  data->output.clear();

  data->input.push_back("");
  answer =
      console.confirm("Really?", Prompt_answer::ALT, "&Yes", "&No", "&Cancel");
  EXPECT_EQ(Prompt_answer::ALT, answer);
  EXPECT_EQ("Really? [Y]es/[N]o/[C]ancel (default Cancel): ", data->output);
  data->output.clear();

  data->input.push_back("");
  answer = console.confirm("Really?", Prompt_answer::YES, "&Continue", "&Edit",
                           "C&ancel");
  EXPECT_EQ(Prompt_answer::YES, answer);
  EXPECT_EQ("Really? [C]ontinue/[E]dit/C[a]ncel (default Continue): ",
            data->output);
  data->output.clear();

  data->input.push_back("cancel");
  answer = console.confirm("Really?", Prompt_answer::YES, "&Continue", "&Edit",
                           "C&ancel");
  EXPECT_EQ(Prompt_answer::ALT, answer);
  EXPECT_EQ("Really? [C]ontinue/[E]dit/C[a]ncel (default Continue): ",
            data->output);
  data->output.clear();

  data->input.push_back("CANCEL");
  answer = console.confirm("Really?", Prompt_answer::YES, "&Continue", "&Edit",
                           "C&ancel");
  EXPECT_EQ(Prompt_answer::ALT, answer);
  EXPECT_EQ("Really? [C]ontinue/[E]dit/C[a]ncel (default Continue): ",
            data->output);
  data->output.clear();

  data->input.push_back("a");
  answer = console.confirm("Really?", Prompt_answer::YES, "&Continue", "&Edit",
                           "C&ancel");
  EXPECT_EQ(Prompt_answer::ALT, answer);
  EXPECT_EQ("Really? [C]ontinue/[E]dit/C[a]ncel (default Continue): ",
            data->output);
  data->output.clear();

  data->input.push_back("A");
  answer = console.confirm("Really?", Prompt_answer::YES, "&Continue", "&Edit",
                           "C&ancel");
  EXPECT_EQ(Prompt_answer::ALT, answer);
  EXPECT_EQ("Really? [C]ontinue/[E]dit/C[a]ncel (default Continue): ",
            data->output);
  data->output.clear();

  data->input.push_back("x");
  data->input.push_back("e");
  answer = console.confirm("Really?", Prompt_answer::YES, "&Continue", "&Edit",
                           "C&ancel");
  EXPECT_EQ(Prompt_answer::NO, answer);
  EXPECT_EQ(
      "Really? [C]ontinue/[E]dit/C[a]ncel (default Continue): ||Please pick an "
      "option out of [C]ontinue/[E]dit/C[a]ncel (default Continue): ||Really? "
      "[C]ontinue/[E]dit/C[a]ncel (default Continue): ",
      data->output);
  data->output.clear();
}

}  // namespace mysqlsh
