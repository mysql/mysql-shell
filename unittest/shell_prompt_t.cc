/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlsh/prompt_manager.h"
#include "mysqlsh/prompt_renderer.h"
#include "mysqlshdk/libs/textui/term_vt100.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "unittest/test_utils/command_line_test.h"

namespace mysqlsh {

class Shell_prompt : public ::testing::Test {
 public:
  static void TearDownTestCase() {
    mysqlshdk::textui::set_color_capability(mysqlshdk::textui::No_color);
  }
};

TEST_F(Shell_prompt, prompt) {
  Prompt_renderer prompt;

  prompt.set_prompt("> ", "-> ", mysqlshdk::textui::Style());

  EXPECT_EQ("> ", prompt.render());

  prompt.set_is_continuing(true);
  EXPECT_EQ("-> ", prompt.render());
}

TEST_F(Shell_prompt, prompt_static) {
  {
    Prompt_renderer prompt;
    prompt.set_width(100);
    prompt.set_prompt("> ", "-> ", mysqlshdk::textui::Style());

    prompt.add_segment("mysql");
    EXPECT_EQ("mysql> ", prompt.render());

    prompt.clear();
    prompt.add_segment("mysql");
    prompt.add_segment("js");
    EXPECT_EQ("mysql js> ", prompt.render());

    prompt.clear();
    prompt.add_segment("mysql");
    prompt.add_segment("py");
    EXPECT_EQ("mysql py> ", prompt.render());

    prompt.set_is_continuing(true);
    EXPECT_EQ("       -> ", prompt.render());
  }
  {
    Prompt_renderer prompt;
    prompt.set_prompt("> ", "-> ", mysqlshdk::textui::Style());
    prompt.add_segment("mysql", mysqlshdk::textui::Style(), 0, 1, 3);
    EXPECT_EQ("   mysql   > ", prompt.render());
  }
}

static mysqlshdk::textui::Style mkstyle(int fg, int bg) {
  mysqlshdk::textui::Style style1;
  if (fg >= 0) style1.field_mask |= mysqlshdk::textui::Style::Color_16_fg_set;
  if (bg >= 0) style1.field_mask |= mysqlshdk::textui::Style::Color_16_bg_set;
  style1.fg.color_16 = fg;
  style1.bg.color_16 = bg;
  return style1;
}

TEST_F(Shell_prompt, separator_styling) {
  mysqlshdk::textui::set_color_capability(mysqlshdk::textui::Color_256);

  {
    Prompt_renderer prompt;
    prompt.add_segment("F", mkstyle(4, 5));
    prompt.add_segment("B", mkstyle(3, 5));
    EXPECT_EQ("\x1B[34;45mF \x1B[0m\x1B[33;45mB\x1B[0m", prompt.render());
  }

  // == As a separator

  // triangle with same bg color (use hollow)
  {
    Prompt_renderer prompt;
    prompt.set_separator(Prompt_renderer::k_symbol_sep_right_pl,
                         Prompt_renderer::k_symbol_sep_right_hollow_pl);
    prompt.add_segment("F", mkstyle(4, 5));
    prompt.add_segment("B", mkstyle(3, 5));
    EXPECT_EQ("\x1B[34;45mF\x1B[34;45m\xEE\x82\xB1\x1B[0m\x1B[33;45mB\x1B[0m",
              prompt.render());
  }
  // triangle with diff bg color (use previous bg as fg)
  {
    Prompt_renderer prompt;
    prompt.set_separator(Prompt_renderer::k_symbol_sep_right_pl,
                         Prompt_renderer::k_symbol_sep_right_hollow_pl);
    prompt.add_segment("F", mkstyle(4, 5));
    prompt.add_segment("B", mkstyle(3, 6));
    EXPECT_EQ("\x1B[34;45mF\x1B[35;46m\xEE\x82\xB0\x1B[0m\x1B[33;46mB\x1B[0m",
              prompt.render());
  }
  // no-sep
  {
    Prompt_renderer prompt;
    prompt.add_segment("F", mkstyle(4, 5));
    prompt.add_segment("B", mkstyle(3, 5));
    prompt.set_separator("", "");
    EXPECT_EQ("\x1B[34;45mF\x1B[0m\x1B[33;45mB\x1B[0m", prompt.render());
  }
  // space
  {
    Prompt_renderer prompt;
    prompt.add_segment("F", mkstyle(4, 5));
    prompt.add_segment("B", mkstyle(3, 5));
    prompt.set_separator(" ", " ");
    EXPECT_EQ("\x1B[34;45mF \x1B[0m\x1B[33;45mB\x1B[0m", prompt.render());
  }
  // some char
  {
    Prompt_renderer prompt;
    prompt.add_segment("F", mkstyle(4, 5));
    prompt.add_segment("B", mkstyle(3, 5));
    prompt.set_separator("#", "#");
    EXPECT_EQ("\x1B[34;45mF\x1B[33;45m#\x1B[0m\x1B[33;45mB\x1B[0m",
              prompt.render());
  }

  // == As the prompt itself
  // triangle default color
  {
    Prompt_renderer prompt;
    prompt.set_prompt(Prompt_renderer::k_symbol_sep_right_pl,
                      Prompt_renderer::k_symbol_sep_right_pl,
                      mysqlshdk::textui::Style());
    prompt.add_segment("F", mkstyle(4, 5));
    EXPECT_EQ("\x1B[34;45mF\x1B[0m\x1B[35m\xEE\x82\xB0\x1B[0m",
              prompt.render());
  }
  {
    Prompt_renderer prompt;
    prompt.set_prompt(Prompt_renderer::k_symbol_sep_right_pl,
                      Prompt_renderer::k_symbol_sep_right_pl, mkstyle(-1, 0));
    prompt.add_segment("F", mkstyle(4, 5));
    EXPECT_EQ("\x1B[34;45mF\x1B[0m\x1B[35;40m\xEE\x82\xB0\x1B[0m",
              prompt.render());
  }
  // triangle with specific fg color
  {
    Prompt_renderer prompt;
    prompt.set_prompt(Prompt_renderer::k_symbol_sep_right_pl,
                      Prompt_renderer::k_symbol_sep_right_pl, mkstyle(1, -1));
    prompt.add_segment("F", mkstyle(4, 5));
    EXPECT_EQ("\x1B[34;45mF\x1B[0m\x1B[31m\xEE\x82\xB0\x1B[0m",
              prompt.render());
  }
  // some char default color
  {
    Prompt_renderer prompt;
    prompt.set_prompt("> ", "-> ", mysqlshdk::textui::Style());
    prompt.add_segment("F", mkstyle(4, 5));
    EXPECT_EQ("\x1B[34;45mF\x1B[0m> ", prompt.render());
  }
  // some char specific fg color
  {
    Prompt_renderer prompt;
    prompt.set_prompt("> ", "-> ", mkstyle(1, -1));
    prompt.add_segment("F", mkstyle(4, 5));
    EXPECT_EQ("\x1B[34;45mF\x1B[0m\x1B[31m> \x1B[0m", prompt.render());
  }
  // some char specific colors
  {
    Prompt_renderer prompt;
    prompt.set_prompt("> ", "-> ", mkstyle(4, 5));
    prompt.add_segment("F", mkstyle(4, 5));
    EXPECT_EQ("\x1B[34;45mF\x1B[0m\x1B[34;45m> \x1B[0m", prompt.render());
  }
}

TEST_F(Shell_prompt, prompt_line_shrink) {
  // Truncate_on_dot
  {
    Prompt_renderer prompt(5);
    prompt.set_prompt("> ", "-> ", mysqlshdk::textui::Style());
    prompt.set_width(20);

    prompt.add_segment(
        "my.host.com", mysqlshdk::textui::Style(), 5, 2, 0,
        Prompt_renderer::Shrinker_type::Truncate_on_dot_from_right);

    prompt.set_width(20);
    EXPECT_EQ("my.host.com> ", prompt.render());
    prompt.set_width(19);
    EXPECT_EQ("my.host.com> ", prompt.render());
    prompt.set_width(18);
    EXPECT_EQ("my.host.com> ", prompt.render());
    prompt.set_width(17);
    EXPECT_EQ("my.host> ", prompt.render());
    prompt.set_width(16);
    EXPECT_EQ("my.host> ", prompt.render());
    prompt.set_width(15);
    EXPECT_EQ("my.host> ", prompt.render());
    prompt.set_width(14);
    EXPECT_EQ("my.host> ", prompt.render());
    prompt.set_width(13);
    EXPECT_EQ("my> ", prompt.render());
    prompt.set_width(12);
    EXPECT_EQ("my> ", prompt.render());
    prompt.set_width(11);
    EXPECT_EQ("my> ", prompt.render());
    prompt.set_width(10);
    EXPECT_EQ("my> ", prompt.render());
    prompt.set_width(9);
    EXPECT_EQ("my> ", prompt.render());
    prompt.set_width(8);
    EXPECT_EQ("> ", prompt.render());
    prompt.set_width(7);
    EXPECT_EQ("> ", prompt.render());
    prompt.set_width(6);
    EXPECT_EQ("> ", prompt.render());
    prompt.set_width(5);
    EXPECT_EQ("> ", prompt.render());
  }
  // Ellipsis
  {
    Prompt_renderer prompt(5);
    prompt.set_prompt("> ", "-> ", mysqlshdk::textui::Style());
    prompt.add_segment("my.host.com", mysqlshdk::textui::Style(), 7, 0, 0,
                       Prompt_renderer::Shrinker_type::Ellipsize_on_char);

    prompt.set_width(20);
    EXPECT_EQ("my.host.com> ", prompt.render());
    prompt.set_width(19);
    EXPECT_EQ("my.host.com> ", prompt.render());
    prompt.set_width(18);
    EXPECT_EQ("my.host.com> ", prompt.render());
    prompt.set_width(17);
    EXPECT_EQ("my.host.c-> ", prompt.render());
    prompt.set_width(16);
    EXPECT_EQ("my.host.-> ", prompt.render());
    prompt.set_width(15);
    EXPECT_EQ("my.host-> ", prompt.render());
    prompt.set_width(14);
    EXPECT_EQ("my.hos-> ", prompt.render());
    prompt.set_width(13);
    EXPECT_EQ("my.ho-> ", prompt.render());
    prompt.set_width(12);
    EXPECT_EQ("my.h-> ", prompt.render());
    prompt.set_width(11);
    EXPECT_EQ("my.-> ", prompt.render());
    prompt.set_width(10);
    EXPECT_EQ("my-> ", prompt.render());
    prompt.set_width(9);
    EXPECT_EQ("m-> ", prompt.render());
    prompt.set_width(8);
    EXPECT_EQ("-> ", prompt.render());
    prompt.set_width(7);
    EXPECT_EQ("> ", prompt.render());
  }

  // Combined with priority/weight
  {
    Prompt_renderer prompt(20);
    prompt.set_prompt("> ", "-> ", mysqlshdk::textui::Style());
    prompt.set_width(100);

    prompt.add_segment("mysql-js", mysqlshdk::textui::Style(), 100, 7);
    prompt.add_segment(
        "my.host.com", mysqlshdk::textui::Style(), 70, 2, 0,
        Prompt_renderer::Shrinker_type::Truncate_on_dot_from_right);
    prompt.add_segment("sakila", mysqlshdk::textui::Style(), 50, 4);

    EXPECT_EQ("mysql-js my.host.com sakila> ", prompt.render());

    prompt.set_width(45);
    EXPECT_EQ("mysql-js my.host sakila> ", prompt.render());

    prompt.set_width(40);
    EXPECT_EQ("mysql-js my sakila> ", prompt.render());
  }

  // Double line
  {
    Prompt_renderer prompt(20);
    prompt.set_prompt("> ", "-> ", mysqlshdk::textui::Style());
    prompt.set_width(100);

    prompt.add_segment("mysql-js", mysqlshdk::textui::Style(), 100, 7);
    prompt.add_segment(
        "my.host.com", mysqlshdk::textui::Style(), 70, 2, 0,
        Prompt_renderer::Shrinker_type::Truncate_on_dot_from_right);
    prompt.add_segment("sakila", mysqlshdk::textui::Style(), 50, 4);
    prompt.add_break();

    EXPECT_EQ("mysql-js my.host.com sakila\n> ", prompt.render());

    prompt.set_width(45);
    EXPECT_EQ("mysql-js my.host.com sakila\n> ", prompt.render());

    prompt.set_width(40);
    EXPECT_EQ("mysql-js my.host.com sakila\n> ", prompt.render());

    prompt.set_width(20);
    EXPECT_EQ("mysql-js my sakila\n> ", prompt.render());
  }

  {
    Prompt_renderer prompt1(20);
    Prompt_renderer prompt2(20);
    prompt1.set_prompt("> ", "-> ", mysqlshdk::textui::Style());
    prompt2.set_prompt("> ", "-> ", mysqlshdk::textui::Style());

    prompt1.add_segment("a.a.a.a", mysqlshdk::textui::Style(), 20, 2, 0,
                        Prompt_renderer::Shrinker_type::No_shrink);
    prompt1.add_segment(
        "b.b.b.b", mysqlshdk::textui::Style(), 10, 2, 0,
        Prompt_renderer::Shrinker_type::Truncate_on_dot_from_right);
    prompt1.add_segment(
        "c.c.c.c", mysqlshdk::textui::Style(), 30, 2, 0,
        Prompt_renderer::Shrinker_type::Truncate_on_dot_from_right);

    prompt2.add_segment("a.a.a.a", mysqlshdk::textui::Style(), 30, 2, 0,
                        Prompt_renderer::Shrinker_type::No_shrink);
    prompt2.add_segment(
        "b.b.b.b", mysqlshdk::textui::Style(), 20, 2, 0,
        Prompt_renderer::Shrinker_type::Truncate_on_dot_from_right);
    prompt2.add_segment(
        "c.c.c.c", mysqlshdk::textui::Style(), 10, 2, 0,
        Prompt_renderer::Shrinker_type::Truncate_on_dot_from_right);

    prompt1.set_width(45);
    EXPECT_EQ("a.a.a.a b.b.b.b c.c.c.c> ", prompt1.render());
    prompt2.set_width(45);
    EXPECT_EQ("a.a.a.a b.b.b.b c.c.c.c> ", prompt2.render());

    prompt1.set_width(44);
    EXPECT_EQ("a.a.a.a b.b.b c.c.c.c> ", prompt1.render());
    prompt2.set_width(44);
    EXPECT_EQ("a.a.a.a b.b.b.b c.c.c> ", prompt2.render());

    prompt1.set_width(43);
    EXPECT_EQ("a.a.a.a b.b.b c.c.c.c> ", prompt1.render());
    prompt2.set_width(43);
    EXPECT_EQ("a.a.a.a b.b.b.b c.c.c> ", prompt2.render());

    prompt1.set_width(42);
    EXPECT_EQ("a.a.a.a b.b c.c.c.c> ", prompt1.render());
    prompt2.set_width(42);
    EXPECT_EQ("a.a.a.a b.b.b.b c.c> ", prompt2.render());

    prompt1.set_width(41);
    EXPECT_EQ("a.a.a.a b.b c.c.c.c> ", prompt1.render());
    prompt2.set_width(41);
    EXPECT_EQ("a.a.a.a b.b.b.b c.c> ", prompt2.render());

    prompt1.set_width(40);
    EXPECT_EQ("a.a.a.a b c.c.c.c> ", prompt1.render());
    prompt2.set_width(40);
    EXPECT_EQ("a.a.a.a b.b.b.b c> ", prompt2.render());

    prompt1.set_width(39);
    EXPECT_EQ("a.a.a.a b c.c.c.c> ", prompt1.render());
    prompt2.set_width(39);
    EXPECT_EQ("a.a.a.a b.b.b.b c> ", prompt2.render());

    prompt1.set_width(38);
    EXPECT_EQ("a.a.a.a b c.c.c> ", prompt1.render());
    prompt2.set_width(38);
    EXPECT_EQ("a.a.a.a b.b.b c> ", prompt2.render());

    prompt1.set_width(37);
    EXPECT_EQ("a.a.a.a b c.c.c> ", prompt1.render());
    prompt2.set_width(37);
    EXPECT_EQ("a.a.a.a b.b.b c> ", prompt2.render());

    prompt1.set_width(36);
    EXPECT_EQ("a.a.a.a b c.c> ", prompt1.render());
    prompt2.set_width(36);
    EXPECT_EQ("a.a.a.a b.b c> ", prompt2.render());

    prompt1.set_width(35);
    EXPECT_EQ("a.a.a.a b c.c> ", prompt1.render());
    prompt2.set_width(35);
    EXPECT_EQ("a.a.a.a b.b c> ", prompt2.render());

    prompt1.set_width(34);
    EXPECT_EQ("a.a.a.a b c> ", prompt1.render());
    prompt2.set_width(34);
    EXPECT_EQ("a.a.a.a b c> ", prompt2.render());

    prompt1.set_width(33);
    EXPECT_EQ("a.a.a.a b c> ", prompt1.render());
    prompt2.set_width(33);
    EXPECT_EQ("a.a.a.a b c> ", prompt2.render());

    prompt1.set_width(32);
    EXPECT_EQ("a.a.a.a c> ", prompt1.render());
    prompt2.set_width(32);
    EXPECT_EQ("a.a.a.a b> ", prompt2.render());

    prompt1.set_width(31);
    EXPECT_EQ("a.a.a.a c> ", prompt1.render());
    prompt2.set_width(31);
    EXPECT_EQ("a.a.a.a b> ", prompt2.render());

    prompt1.set_width(29);
    EXPECT_EQ("c.c.c.c> ", prompt1.render());
    prompt2.set_width(29);
    EXPECT_EQ("a.a.a.a> ", prompt2.render());

    prompt1.set_width(20);
    EXPECT_EQ("> ", prompt1.render());
    prompt2.set_width(20);
    EXPECT_EQ("> ", prompt2.render());
  }
}

TEST_F(Shell_prompt, color_16) {
  mysqlshdk::textui::set_color_capability(mysqlshdk::textui::Color_256);
}

TEST_F(Shell_prompt, color_24bit) {
  mysqlshdk::textui::set_color_capability(mysqlshdk::textui::Color_256);
}

TEST_F(Shell_prompt, color_i256) {
  mysqlshdk::textui::set_color_capability(mysqlshdk::textui::Color_256);
}

TEST_F(Shell_prompt, color_i256_unicode) {
  mysqlshdk::textui::set_color_capability(mysqlshdk::textui::Color_256);
  Prompt_renderer prompt;
  uint8_t rgb0[3] = {0, 0, 0};
  prompt.set_width(60);
  prompt.set_prompt("> ", "-> ", mysqlshdk::textui::Style());
  mysqlshdk::textui::Style red({0, 196, rgb0}, {0, 15, rgb0});
  mysqlshdk::textui::Style blue({0, 21, rgb0}, {0, 15, rgb0});

  prompt.add_segment("hello", red);
  EXPECT_EQ("\x1B[48;5;15m\x1B[38;5;196mhello\x1B[0m> ", prompt.render());

  prompt.add_segment("world", blue);
  EXPECT_EQ(
      "\x1B[48;5;15m\x1B[38;5;196mhello "
      "\x1B[0m\x1B[48;5;15m\x1B[38;5;21mworld\x1B[0m> ",
      prompt.render());

  prompt.clear();
  prompt.set_prompt("> ", "-> ", mysqlshdk::textui::Style());

  mysqlshdk::textui::Style style;
  style.field_mask = mysqlshdk::textui::Style::Attributes_set;
  style.bold = true;
  style.underline = false;
  prompt.add_segment("aa", style);
  style.bold = true;
  style.underline = true;
  prompt.add_segment("bb", style);
  style.bold = false;
  style.underline = true;
  prompt.add_segment("cc", style);
  style.bold = false;
  style.underline = false;
  prompt.add_segment("dd", style);
  EXPECT_EQ(
      "\x1B[1maa \x1B[0m\x1B[1;4mbb \x1B[0m\x1B[4mcc \x1B[0m\x1B[0mdd\x1B[0m> ",
      prompt.render());

  prompt.clear();
  prompt.set_prompt(shcore::Value::parse("'\\uF37A'").get_string(), "",
                    mysqlshdk::textui::Style());
  EXPECT_EQ(u8"\uF37A", prompt.render());
}

TEST(Shell_prompt_manager, load) {
  mysqlshdk::textui::set_color_capability(mysqlshdk::textui::Color_256);
  Prompt_manager prompt;
  shcore::Value theme;
  std::map<std::string, std::string> vars;
  auto getvar =
      [](const std::string &var,
         mysqlsh::Prompt_manager::Dynamic_variable_type type) -> std::string {
    return "";
  };

  theme = shcore::Value::parse(
      "{ 'segments' : [{'text':'abab', 'padding':1}, {'text':'cd', "
      "'padding':0}], "
      "'symbols': {'separator':'|', 'separator2':'|', 'prompt':'>', "
      "'prompt2':'-', 'ellipsis':'-'}}");
  prompt.set_theme(theme);
  EXPECT_EQ(" abab |cd> ", prompt.get_prompt(&vars, getvar));

  theme = shcore::Value::parse(
      "{ 'segments' : [{'text':'abab', 'bg':'255', 'fg':'232', 'padding':1}]}");
  prompt.set_theme(theme);
  EXPECT_EQ("\x1B[48;5;255m\x1B[38;5;232m abab \x1B[0m> ",
            prompt.get_prompt(&vars, getvar));

  theme = shcore::Value::parse(
      "{ 'segments' : [{'text':'abab', 'bg':'1', 'fg':'2', 'padding':1}, "
      "{'bg':'3','fg':'4','text':'cd', 'padding':0}], "
      "'symbols': {'separator':'|', 'separator2':'|', 'prompt':'>', "
      "'prompt2':'-', 'ellipsis':'-'}}");
  prompt.set_theme(theme);
  EXPECT_EQ(
      "\x1B[48;5;1m\x1B[38;5;2m abab "
      "\x1B[48;5;3m\x1B[38;5;4m|\x1B[0m\x1B[48;5;3m\x1B[38;5;4mcd\x1B[0m> ",
      prompt.get_prompt(&vars, getvar));

  theme = shcore::Value::parse(
      "{ 'segments' : [{'text':'%var1%%var2%%', 'padding':1}, "
      "{'text':'%var3%', 'padding':0}], "
      "'symbols': {'separator':'|', 'separator2':'|', 'prompt':'>', "
      "'prompt2':'-', 'ellipsis':'-'}}");
  prompt.set_theme(theme);
  vars["var1"] = "AAA";
  vars["var2"] = "BBB";
  vars["var3"] = "CCC";
  EXPECT_EQ(" AAABBB% |CCC> ", prompt.get_prompt(&vars, getvar));

  // check attributes
  theme = shcore::Value::parse(
      "{'segments': [{'text': 'aa', 'fg':'red', 'bg':'yellow', 'bold':true}]}");
  prompt.set_theme(theme);
  EXPECT_EQ("\x1B[48;5;3m\x1B[38;5;1m\x1B[1maa\x1B[0m> ",
            prompt.get_prompt(&vars, getvar));

  theme = shcore::Value::parse(
      "{'segments': [{'text': 'aa', 'fg':'red', 'bg':'yellow', 'bold':true}, "
      "{'text': 'bb'}]}");
  prompt.set_theme(theme);
  EXPECT_EQ("\x1B[48;5;3m\x1B[38;5;1m\x1B[1maa \x1B[0mbb> ",
            prompt.get_prompt(&vars, getvar));

  theme = shcore::Value::parse(
      "{'segments': [{'text': 'aa', 'fg':'red', 'bg':'yellow', 'bold':true}, "
      "{'text': 'bb', 'fg': ';123', 'bg':';65', 'underline':true}]}");
  prompt.set_theme(theme);
  EXPECT_EQ(
      "\x1B[48;5;3m\x1B[38;5;1m\x1B[1maa "
      "\x1B[0m\x1B[48;5;65m\x1B[38;5;123m\x1B[4mbb\x1B[0m> ",
      prompt.get_prompt(&vars, getvar));

  // with a prompt
  theme =
      shcore::Value::parse("{'prompt': {'text': '$$ ', 'cont_text': '@@ '}}");
  prompt.set_theme(theme);
  EXPECT_EQ("$$ ", prompt.get_prompt(&vars, getvar));
  prompt.set_is_continuing(true);
  EXPECT_EQ("@@ ", prompt.get_prompt(&vars, getvar));
}

TEST(Shell_prompt_manager, classes) {
  mysqlshdk::textui::set_color_capability(mysqlshdk::textui::Color_256);
  Prompt_manager prompt;
  shcore::Value theme;
  std::map<std::string, std::string> vars;
  auto getvar =
      [](const std::string &var,
         mysqlsh::Prompt_manager::Dynamic_variable_type type) -> std::string {
    return "";
  };

  theme = shcore::Value::parse(
      "{ 'classes': {'c1':{'bg':'4','fg':'5','padding':2}}, 'segments' : "
      "[{'text':'aaa', 'classes':['c1']}]}");
  prompt.set_theme(theme);
  EXPECT_EQ("\x1B[48;5;4m\x1B[38;5;5m  aaa  \x1B[0m> ",
            prompt.get_prompt(&vars, getvar));

  theme = shcore::Value::parse(
      "{ 'classes': {'c1':{'bg':'4'},'c2':{'padding':5,'fg':'5'}}, "
      "'segments' : "
      "[{'text':'aaa', 'classes':['c1','c2']}]}");
  prompt.set_theme(theme);
  EXPECT_EQ("\x1B[48;5;4maaa\x1B[0m> ", prompt.get_prompt(&vars, getvar));

  theme = shcore::Value::parse(
      "{ 'classes': {'c1':{'bg':'4', 'match_tags': "
      "['tag']},'c2':{'padding':5,'fg':'5'}}, 'segments' : [{'text':'aaa', "
      "'tag':'tag', 'classes':['c1','c2']}]}");
  prompt.set_theme(theme);
  EXPECT_EQ("\x1B[48;5;4maaa\x1B[0m> ", prompt.get_prompt(&vars, getvar));

  // Bug#26501553 SHELL PRINTS: ERROR IN PROMPT THEME: BASIC_STRING::APPEND
  theme = shcore::Value::parse(
      "{ 'classes': {'c':{'padding':-1,'fg':'5'}}, "
      "'segments' : [{'text':'aaa', "
      "'classes':['c']}]}");
  prompt.set_theme(theme);
  EXPECT_EQ(
      "(Error in prompt theme: Error in class c of prompt theme: padding value "
      "must be >= 0)>",
      prompt.get_prompt(&vars, getvar));
  theme = shcore::Value::parse(
      "{ 'classes': {'c':{'min_width':-1,'fg':'5'}}, "
      "'segments' : [{'text':'aaa', "
      "'classes':['c']}]}");
  prompt.set_theme(theme);
  EXPECT_EQ(
      "(Error in prompt theme: Error in class c of prompt theme: min_width "
      "value must be >= 0)>",
      prompt.get_prompt(&vars, getvar));
  theme = shcore::Value::parse(
      "{ 'classes': {"
      "'c':{'min_width':1,'fg':'5'}}, 'segments' : [{'text':'aaa', "
      "'min_width': -1, 'classes':['c']}]}");
  prompt.set_theme(theme);
  EXPECT_EQ("(Error in prompt theme: min_width value must be >= 0)>",
            prompt.get_prompt(&vars, getvar));
  theme = shcore::Value::parse(
      "{ 'classes': {"
      "'c':{'min_width':1,'fg':'5'}}, 'segments' : [{'text':'aaa', "
      "'padding': -1, 'classes':['c']}]}");
  prompt.set_theme(theme);
  EXPECT_EQ("(Error in prompt theme: padding value must be >= 0)>",
            prompt.get_prompt(&vars, getvar));
}

TEST(Shell_prompt_manager, variables) {
  Prompt_manager promptm;
  Prompt_manager::Variables_map vars;

  EXPECT_NE("",
            promptm.do_apply_vars("%date%", &vars,
                                  [](const std::string &var,
                                     Prompt_manager::Dynamic_variable_type type)
                                      -> std::string { return ""; }));
  EXPECT_NE("",
            promptm.do_apply_vars("%time%", &vars,
                                  [](const std::string &var,
                                     Prompt_manager::Dynamic_variable_type type)
                                      -> std::string { return ""; }));
  EXPECT_EQ("%invalid%",
            promptm.do_apply_vars("%invalid%", &vars,
                                  [](const std::string &var,
                                     Prompt_manager::Dynamic_variable_type type)
                                      -> std::string { return ""; }));

  EXPECT_EQ(std::string(getenv("PATH")),
            promptm.do_apply_vars("%env:PATH%", &vars,
                                  [](const std::string &var,
                                     Prompt_manager::Dynamic_variable_type type)
                                      -> std::string { return ""; }));

  EXPECT_EQ("FOO",
            promptm.do_apply_vars(
                "%Sysvar:foo%", &vars,
                [](const std::string &var,
                   Prompt_manager::Dynamic_variable_type type) -> std::string {
                  if (Prompt_manager::Mysql_system_variable != type)
                    throw std::logic_error("mismatch");
                  return shcore::str_upper(var);
                }));
  EXPECT_EQ(0, vars.size());
  EXPECT_EQ("FOO",
            promptm.do_apply_vars(
                "%sysvar:foo%", &vars,
                [](const std::string &var,
                   Prompt_manager::Dynamic_variable_type type) -> std::string {
                  if (Prompt_manager::Mysql_system_variable != type)
                    throw std::logic_error("mismatch");
                  return shcore::str_upper(var);
                }));
  EXPECT_EQ(1, vars.size());

  EXPECT_EQ("BAR",
            promptm.do_apply_vars(
                "%sessvar:bar%", &vars,
                [](const std::string &var,
                   Prompt_manager::Dynamic_variable_type type) -> std::string {
                  if (Prompt_manager::Mysql_session_variable != type)
                    throw std::logic_error("mismatch");
                  return shcore::str_upper(var);
                }));
  EXPECT_EQ(2, vars.size());

  EXPECT_EQ("BLA",
            promptm.do_apply_vars(
                "%Status:bla%", &vars,
                [](const std::string &var,
                   Prompt_manager::Dynamic_variable_type type) -> std::string {
                  if (Prompt_manager::Mysql_status != type)
                    throw std::logic_error("mismatch");
                  return shcore::str_upper(var);
                }));
  EXPECT_EQ(2, vars.size());

  EXPECT_EQ("BLA",
            promptm.do_apply_vars(
                "%status:bla%", &vars,
                [](const std::string &var,
                   Prompt_manager::Dynamic_variable_type type) -> std::string {
                  if (Prompt_manager::Mysql_status != type)
                    throw std::logic_error("mismatch");
                  return shcore::str_upper(var);
                }));
  EXPECT_EQ(3, vars.size());

  EXPECT_EQ("ZZZ",
            promptm.do_apply_vars(
                "%sessstatus:zzz%", &vars,
                [](const std::string &var,
                   Prompt_manager::Dynamic_variable_type type) -> std::string {
                  if (Prompt_manager::Mysql_session_status != type)
                    throw std::logic_error("mismatch");
                  return shcore::str_upper(var);
                }));
  EXPECT_EQ(4, vars.size());
}

TEST(Shell_prompt_manager, custom_variable) {
  auto theme = shcore::Value::parse(
      "{'variables' : "
      "  {'test' : {'match' : {'value':'%value%', 'pattern':'%pattern%'}, "
      "          'if_true': '%true%', "
      "          'if_false':'%false%'}}, "
      "'segments':[{'text':'%test%'}]}");

  Prompt_manager prompt;
  prompt.set_theme(theme);

  std::map<std::string, std::string> vars;
  vars["true"] = "IS_TRUE";
  vars["false"] = "IS_FALSE";
  vars["value"] = "FOOOOOOOBAR";
  vars["pattern"] = "FOO*BAR";
  auto getvar =
      [](const std::string &var,
         mysqlsh::Prompt_manager::Dynamic_variable_type type) -> std::string {
    return "";
  };
  EXPECT_EQ("IS_TRUE> ", prompt.get_prompt(&vars, getvar));

  vars["value"] = "FOOOOOOOBAR";
  vars["pattern"] = "FOOBAR";
  EXPECT_EQ("IS_FALSE> ", prompt.get_prompt(&vars, getvar));

  vars["value"] = "XBAR";
  vars["pattern"] = "?BAR";
  EXPECT_EQ("IS_TRUE> ", prompt.get_prompt(&vars, getvar));
}

TEST(Shell_prompt_manager, custom_variables_bug26502508) {
  auto theme = shcore::Value::parse(R"##({
    'variables' :
      {'test' :
          {'match' : {'value': '1', 'pattern': '1'},
              'if_true': '%test%',
              'if_false':'%test%'
          }
      },
      'segments':[{'text':'%test%'}]
    })##");
  auto theme2 = shcore::Value::parse(R"##({'variables' :
      {'test' :
          {'match' : {'value': '1', 'pattern': '1'},
              'if_true': '%test2%',
              'if_false':'%test2%'
          },
        'test2' :
          {'match' : {'value': '1', 'pattern': '1'},
              'if_true': '%test%',
              'if_false':'%test%'
          }
      },
      'segments':[{'text':'%test%'}]
    })##");

  Prompt_manager prompt;
  std::map<std::string, std::string> vars;
  vars["value"] = "FOOOOOOOBAR";
  vars["pattern"] = "FOO*BAR";
  auto getvar =
      [](const std::string &var,
         mysqlsh::Prompt_manager::Dynamic_variable_type type) -> std::string {
    return "";
  };

  prompt.set_theme(theme);
  EXPECT_EQ("<<Recursion detected during variable evaluation>>> ",
            prompt.get_prompt(&vars, getvar));
  prompt.set_theme(theme2);
  EXPECT_EQ("<<Recursion detected during variable evaluation>>> ",
            prompt.get_prompt(&vars, getvar));
}

TEST(Shell_prompt_manager, misc) {
  // check default style
  Prompt_manager promptm;
  Prompt_manager::Variables_map vmap;
  vmap["host"] = "localhost";
  vmap["schema"] = "";
  vmap["mode"] = "js";
  EXPECT_EQ(
      "mysql-js []> ",
      promptm.get_prompt(&vmap, Prompt_manager::Dynamic_variable_callback()));

  vmap["schema"] = "sakila";
  vmap["mode"] = "js";
  EXPECT_EQ(
      "mysql-js [sakila]> ",
      promptm.get_prompt(&vmap, Prompt_manager::Dynamic_variable_callback()));

  // check whether errors during render are properly caught
  vmap["schema"] = "sakila";
  vmap["mode"] = "js";
  EXPECT_NO_THROW(promptm.get_prompt(
      &vmap,
      [](const std::string &, Prompt_manager::Dynamic_variable_type)
          -> std::string { throw std::runtime_error("catchme"); }));

  // bad theme
  promptm.set_theme(shcore::Value::parse("{'segments': 'bla'}"));
  EXPECT_EQ("> ", promptm.get_prompt(
                      &vmap, Prompt_manager::Dynamic_variable_callback()));
}

TEST(Shell_prompt_manager, attributes_other) {
  mysqlshdk::textui::set_color_capability(mysqlshdk::textui::Color_256);
  shcore::Value theme;

  {
    Prompt_manager::Attributes attr;
    theme = shcore::Value::parse("{'text':'a'}");
    EXPECT_NO_THROW(attr.load(theme.as_map()));
    EXPECT_EQ("a", attr.text);
    EXPECT_FALSE(attr.sep);
    EXPECT_EQ(2, attr.min_width);
    EXPECT_EQ(0, attr.padding);
    EXPECT_EQ(Prompt_renderer::Shrinker_type::No_shrink, attr.shrink);
  }
  {
    Prompt_manager::Attributes attr;
    theme = shcore::Value::parse("{'separator':'#'}");
    EXPECT_NO_THROW(attr.load(theme.as_map()));
    EXPECT_EQ("", attr.text);
    EXPECT_EQ("#", *attr.sep);
    EXPECT_EQ(2, attr.min_width);
    EXPECT_EQ(0, attr.padding);
    EXPECT_EQ(Prompt_renderer::Shrinker_type::No_shrink, attr.shrink);
  }
  {
    Prompt_manager::Attributes attr;
    theme = shcore::Value::parse("{'padding':4}");
    EXPECT_NO_THROW(attr.load(theme.as_map()));
    EXPECT_EQ("", attr.text);
    EXPECT_FALSE(attr.sep);
    EXPECT_EQ(2, attr.min_width);
    EXPECT_EQ(4, attr.padding);
    EXPECT_EQ(Prompt_renderer::Shrinker_type::No_shrink, attr.shrink);
  }
  {
    Prompt_manager::Attributes attr;
    theme = shcore::Value::parse("{'min_width':40}");
    EXPECT_NO_THROW(attr.load(theme.as_map()));
    EXPECT_EQ("", attr.text);
    EXPECT_FALSE(attr.sep);
    EXPECT_EQ(40, attr.min_width);
    EXPECT_EQ(0, attr.padding);
    EXPECT_EQ(Prompt_renderer::Shrinker_type::No_shrink, attr.shrink);
  }
  {
    Prompt_manager::Attributes attr;
    theme = shcore::Value::parse("{'shrink':'none'}");
    EXPECT_NO_THROW(attr.load(theme.as_map()));
    EXPECT_EQ("", attr.text);
    EXPECT_FALSE(attr.sep);
    EXPECT_EQ(2, attr.min_width);
    EXPECT_EQ(0, attr.padding);
    EXPECT_EQ(Prompt_renderer::Shrinker_type::No_shrink, attr.shrink);
  }
  {
    Prompt_manager::Attributes attr;
    theme = shcore::Value::parse("{'shrink':'ellipsize'}");
    EXPECT_NO_THROW(attr.load(theme.as_map()));
    EXPECT_EQ("", attr.text);
    EXPECT_FALSE(attr.sep);
    EXPECT_EQ(2, attr.min_width);
    EXPECT_EQ(0, attr.padding);
    EXPECT_EQ(Prompt_renderer::Shrinker_type::Ellipsize_on_char, attr.shrink);
  }
  {
    Prompt_manager::Attributes attr;
    theme = shcore::Value::parse("{'shrink':'truncate_on_dot'}");
    EXPECT_NO_THROW(attr.load(theme.as_map()));
    EXPECT_EQ("", attr.text);
    EXPECT_FALSE(attr.sep);
    EXPECT_EQ(2, attr.min_width);
    EXPECT_EQ(0, attr.padding);
    EXPECT_EQ(Prompt_renderer::Shrinker_type::Truncate_on_dot_from_right,
              attr.shrink);
  }
  {
    Prompt_manager::Attributes attr;
    theme = shcore::Value::parse("{'shrink':'eqwewq'}");
    EXPECT_THROW(attr.load(theme.as_map()), std::invalid_argument);
  }
  {
    Prompt_manager::Attributes attr;
    theme = shcore::Value::parse("{'shrink':123}");
    EXPECT_THROW(attr.load(theme.as_map()), std::exception);
  }
}

// ----------------------------------------------------------------------------

class Shell_prompt_exe : public tests::Command_line_test {
 public:
  static void SetUpTestCase() {
    shcore::create_file(shcore::get_user_config_path() + "/prompt.json",
                        "{\"segments\":[{\"text\":\"TEST_PROMPT\"}]}");
    shcore::create_file("altprompt.json",
                        "{\"segments\":[{\"text\":\"ALT_PROMPT\"}]}");
    shcore::create_file("badprompt.json",
                        "\"segments\":[{\"text\":\"ALT_PROMPT\"}]}");
  }

  static void TearDownTestCase() {
    shcore::delete_file(shcore::get_user_config_path() + "/prompt.json");
    shcore::delete_file("altprompt.json");
    shcore::delete_file("badprompt.json");
  }
};

TEST_F(Shell_prompt_exe, environment) {
#ifdef HAVE_V8
  const auto expect_prompt = "mysql-js>";
#else
  const auto expect_prompt = "mysql-py>";
#endif
  std::string no_theme_prompt = expect_prompt;

  // by default this prompt is not necessarily the same as expect_prompt
  shcore::unsetenv("MYSQLSH_PROMPT_THEME");
  execute({_mysqlsh, "--interactive=full", "-e", "1", nullptr});
  const auto position = _output.rfind("\n");
  if (position != std::string::npos) {
    no_theme_prompt = _output.substr(position + 1);
  }

  // TS_CP#1 set MYSQLSH_PROMPT_THEME environment variable to a file name , it
  // configures prompt correctly accordingly to file
  shcore::setenv("MYSQLSH_PROMPT_THEME", "altprompt.json");
  execute({_mysqlsh, "--interactive=full", "-e", "1", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("ALT_PROMPT> 1\n1\nALT_PROMPT> ");
  shcore::unsetenv("MYSQLSH_PROMPT_THEME");

  // TS_CP#6 On startup, if an error is found in the prompt file, an error
  // message will be printed and a default prompt will be used.

  // TS_EV#1 invalid values on MYSQLSH_PROMPT_THEME= will force the shell to use
  // a default prompt with no colors and log an error to the log file and also

  // to the terminal, during startup.
  shcore::setenv("MYSQLSH_PROMPT_THEME", "badprompt.json");
  execute({_mysqlsh, "--interactive=full", "-e", "1", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Error loading prompt theme 'badprompt.json'");
  MY_EXPECT_CMD_OUTPUT_CONTAINS(expect_prompt);
  shcore::unsetenv("MYSQLSH_PROMPT_THEME");

  // no prompt theme
  execute({_mysqlsh, "--interactive=full", "-e", "1", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS(no_theme_prompt);

  // TS_EV#3 MYSQLSH_TERM_COLOR_MODE= with invalid value will force the shell to
  // use a default prompt with no colors and log an error to the log file and
  // also to the terminal, during startup.
  shcore::setenv("MYSQLSH_TERM_COLOR_MODE", "bla");
  execute({_mysqlsh, "--interactive=full", "-e", "1", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "MYSQLSH_TERM_COLOR_MODE environment variable set to invalid value. ");
  MY_EXPECT_CMD_OUTPUT_CONTAINS(expect_prompt);
  shcore::unsetenv("MYSQLSH_TERM_COLOR_MODE");
}

TEST_F(Shell_prompt_exe, histignore) {
  // TS_CLO#1  (combined with other tests in Cmdline_shell)
  // TS_CLO#2
  execute({_mysqlsh, "--histignore=foobar", "-e",
           "print(shell.options['history.sql.ignorePattern']);", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("foobar");
}

// the last line output is the prompt we will check
#define EXPECT_PROMPT(prompt)                            \
  ASSERT_TRUE(_output.rfind('\n') != std::string::npos); \
  EXPECT_EQ(prompt, _output.substr(_output.rfind('\n') + 1));

namespace {
size_t second_to_last_line(const std::string &s) {
  auto p = s.rfind('\n');
  if (p != std::string::npos) {
    p = s.substr(0, p).rfind('\n');
  }
  return p;
}
}  // namespace

#define EXPECT_DBL_PROMPT(prompt)                                 \
  ASSERT_TRUE(second_to_last_line(_output) != std::string::npos); \
  EXPECT_EQ(prompt, _output.substr(second_to_last_line(_output) + 1));

TEST_F(Shell_prompt_exe, prompt_variables) {
  std::string segs;
  for (const char *s :
       {"host", "port", "mode", "Mode", "uri", "user", "schema", "ssl", "date",
        "env:MYSQLSH_PROMPT_THEME", "sysvar:autocommit", "sessvar:autocommit",
        "sessstatus:Mysqlx_ssl_active"}) {
    segs.append(shcore::str_format(
        "{\"text\": \"%s=%%%s%%\", \"separator\":\"  \"}, ", s, s));
  }
  segs.append(shcore::str_format("{\"text\": \"END\"}"));
  shcore::create_file("allvars.json", "{\"segments\": [" + segs + "]}");

  shcore::setenv("MYSQLSH_PROMPT_THEME", "allvars.json");

  using mysqlshdk::utils::fmttime;
  int rc;
  std::string uri = _user + "@" + _host;

  wipe_out();
  rc = execute({_mysqlsh, "--interactive=full", "--sql", "-e", "1;", nullptr});
  EXPECT_EQ(0, rc);
  EXPECT_PROMPT(
      "host=  port=  mode=sql  Mode=SQL  uri=  user=  schema=  ssl=  date=" +
      fmttime("%F") +
      "  env:MYSQLSH_PROMPT_THEME=allvars.json  "
      "sysvar:autocommit=  sessvar:autocommit=  "
      "sessstatus:Mysqlx_ssl_active= END> ");

  wipe_out();
  rc = execute({_mysqlsh, "--interactive=full", "--py", "-e", "1", nullptr});
  EXPECT_EQ(0, rc);
  EXPECT_PROMPT(
      "host=  port=  mode=py  Mode=Py  uri=  user=  schema=  ssl=  date=" +
      fmttime("%F") +
      "  env:MYSQLSH_PROMPT_THEME=allvars.json  "
      "sysvar:autocommit=  sessvar:autocommit=  "
      "sessstatus:Mysqlx_ssl_active= END> ");

  wipe_out();
  rc = execute({_mysqlsh, "--interactive=full", _mysql_uri.c_str(),
                "--ssl-mode=REQUIRED", "--sql", "-e", "set autocommit=0;",
                nullptr});
  EXPECT_EQ(0, rc);
  EXPECT_PROMPT(
      "host=" + _host + "  port=" + _mysql_port + "  mode=sql" +
      "  Mode=SQL  uri=" + uri + ":" + _mysql_port +
      "  user=root  schema=  ssl=SSL  date=" + fmttime("%F") +
      "  env:MYSQLSH_PROMPT_THEME=allvars.json  sysvar:autocommit=ON  "
      "sessvar:autocommit=OFF  "
      "sessstatus:Mysqlx_ssl_active= END> ");

  wipe_out();
  rc = execute({_mysqlsh, "--interactive=full", _mysql_uri.c_str(),
                "--ssl-mode=DISABLED", "--sql", "-e", "set autocommit=0;",
                nullptr});
  EXPECT_EQ(0, rc);
  EXPECT_PROMPT(
      "host=" + _host + "  port=" + _mysql_port + "  mode=sql" +
      "  Mode=SQL  uri=" + uri + ":" + _mysql_port +
      "  user=root  schema=  ssl=  date=" + fmttime("%F") +
      "  env:MYSQLSH_PROMPT_THEME=allvars.json  sysvar:autocommit=ON  "
      "sessvar:autocommit=OFF  "
      "sessstatus:Mysqlx_ssl_active= END> ");

  // This test only works if the test server is listening on default socket path
  if (0) {
    wipe_out();
    rc = execute({_mysqlsh, "--interactive=full", uri.c_str(),
                  "--password=", "--mysql", "--ssl-mode=DISABLED", "--sql",
                  "-e", "set autocommit=0;", nullptr});
    EXPECT_EQ(0, rc);
    EXPECT_PROMPT(
        "host=" + _host + "  port=" + "  mode=sql" + "  Mode=SQL  uri=" + uri +
        "  user=root  schema=  ssl=  date=" + fmttime("%F") +
        "  env:MYSQLSH_PROMPT_THEME=allvars.json  sysvar:autocommit=ON  "
        "sessvar:autocommit=OFF  "
        "sessstatus:Mysqlx_ssl_active= END> ");
  }

  wipe_out();
  rc = execute({_mysqlsh, "--interactive=full", "--sql", _uri.c_str(),
                "--schema=mysql", "--ssl-mode=REQUIRED", "-e",
                "set autocommit=0;", nullptr});
  EXPECT_EQ(0, rc);
  EXPECT_PROMPT("host=" + _host + "  port=" + _port +
                "  mode=sql  Mode=SQL  uri=" + uri + ":" + _port +
                "  user=root  schema=mysql  ssl=SSL" +
                "  date=" + fmttime("%F") +
                "  env:MYSQLSH_PROMPT_THEME=allvars.json  "
                "sysvar:autocommit=ON  sessvar:autocommit=OFF  "
                "sessstatus:Mysqlx_ssl_active=ON END> ");

  if (_port == "33060") {
    wipe_out();
    rc = execute({_mysqlsh, "--interactive=full", "--sql", uri.c_str(),
                  "--password=", "--mysqlx", "--schema=mysql",
                  "--ssl-mode=REQUIRED", "-e", "set autocommit=0;", nullptr});
    EXPECT_EQ(0, rc);
    EXPECT_PROMPT("host=" + _host + "  port=" + _port +
                  "  mode=sql  Mode=SQL  uri=" + uri + ":" + _port +
                  "  user=root  schema=mysql  ssl=SSL" +
                  "  date=" + fmttime("%F") +
                  "  env:MYSQLSH_PROMPT_THEME=allvars.json  "
                  "sysvar:autocommit=ON  sessvar:autocommit=OFF  "
                  "sessstatus:Mysqlx_ssl_active=ON END> ");
  }
  shcore::delete_file("allvars.json");

  shcore::unsetenv("MYSQLSH_PROMPT_THEME");
}

// Test sample prompts (which in turn would test the whole thing)
TEST_F(Shell_prompt_exe, sample_prompt_theme_nocolor) {
  shcore::setenv("MYSQLSH_PROMPT_THEME",
                 shcore::get_binary_folder() + "/prompt_nocolor.json");
  shcore::setenv("MYSQLSH_TERM_COLOR_MODE", "nocolor");

  int rc =
      execute({_mysqlsh, "--interactive=full", _uri.c_str(), "--schema=mysql",
               "--ssl-mode=REQUIRED", "-e", "1", nullptr});
  EXPECT_EQ(0, rc);
  std::cout << _output << "\n";
#ifdef HAVE_V8
  EXPECT_PROMPT("MySQL [" + _host + "+ ssl/mysql] JS> ");
#else
  EXPECT_PROMPT("MySQL [" + _host + "+ ssl/mysql] Py> ");
#endif

  shcore::unsetenv("MYSQLSH_PROMPT_THEME");
  shcore::unsetenv("MYSQLSH_TERM_COLOR_MODE");
}

TEST_F(Shell_prompt_exe, sample_prompt_theme_16) {
  shcore::setenv("MYSQLSH_PROMPT_THEME",
                 shcore::get_binary_folder() + "/prompt_16.json");
  shcore::setenv("MYSQLSH_TERM_COLOR_MODE", "16");

  int rc =
      execute({_mysqlsh, "--interactive=full", _uri.c_str(), "--schema=mysql",
               "--ssl-mode=REQUIRED", "-e", "1", nullptr});
  EXPECT_EQ(0, rc);
  std::cout << _output << "\n";

#ifdef HAVE_V8
  EXPECT_PROMPT("MySQL \x1B[1m[" + _host + "+ ssl/mysql] \x1B[0mJS> ");
#else
  EXPECT_PROMPT("MySQL \x1B[1m[" + _host + "+ ssl/mysql] \x1B[0mPy> ");
#endif

  shcore::unsetenv("MYSQLSH_PROMPT_THEME");
  shcore::unsetenv("MYSQLSH_TERM_COLOR_MODE");
}

TEST_F(Shell_prompt_exe, sample_prompt_theme_256) {
  shcore::setenv("MYSQLSH_PROMPT_THEME",
                 shcore::get_binary_folder() + "/prompt_256.json");
  shcore::setenv("MYSQLSH_TERM_COLOR_MODE", "256");

  int rc =
      execute({_mysqlsh, "--interactive=full", _uri.c_str(), "--schema=mysql",
               "--ssl-mode=REQUIRED", "-e", "1", nullptr});
  EXPECT_EQ(0, rc);
  std::cout << _output << "\n";

#ifdef HAVE_V8
  EXPECT_PROMPT(
      "\x1B[48;5;254m\x1B[38;5;23m My\x1B[0m\x1B[48;5;254m\x1B[38;5;166mSQL "
      "\x1B[0m\x1B[48;5;237m\x1B[38;5;15m " +
      _host + ":" + _port +
      "+ ssl "
      "\x1B[0m\x1B[48;5;242m\x1B[38;5;15m mysql "
      "\x1B[0m\x1B[48;5;221m\x1B[38;5;0m JS \x1B[0m\x1B[48;5;0m> \x1B[0m");
#else
  EXPECT_PROMPT(
      "\x1B[48;5;254m\x1B[38;5;23m My\x1B[0m\x1B[48;5;254m\x1B[38;5;166mSQL "
      "\x1B[0m\x1B[48;5;237m\x1B[38;5;15m " +
      _host + ":" + _port +
      "+ ssl "
      "\x1B[0m\x1B[48;5;242m\x1B[38;5;15m mysql "
      "\x1B[0m\x1B[48;5;25m\x1B[38;5;15m Py \x1B[0m\x1B[48;5;0m> \x1B[0m");
#endif

  shcore::unsetenv("MYSQLSH_PROMPT_THEME");
  shcore::unsetenv("MYSQLSH_TERM_COLOR_MODE");
}

TEST_F(Shell_prompt_exe, sample_prompt_theme_dbl_256) {
  shcore::setenv("MYSQLSH_PROMPT_THEME",
                 shcore::get_binary_folder() + "/prompt_dbl_256.json");
  shcore::setenv("MYSQLSH_TERM_COLOR_MODE", "256");

  int rc =
      execute({_mysqlsh, "--interactive=full", _uri.c_str(), "--schema=mysql",
               "--ssl-mode=REQUIRED", "-e", "1", nullptr});
  EXPECT_EQ(0, rc);
  std::cout << _output << "\n";

#ifdef HAVE_V8
  EXPECT_DBL_PROMPT(
      "\x1B[48;5;254m\x1B[38;5;23m My\x1B[0m\x1B[48;5;254m\x1B[38;5;166mSQL "
      "\x1B[0m\x1B[48;5;237m\x1B[38;5;15m " +
      _host + ":" + _port +
      "+ ssl "
      "\x1B[0m\x1B[48;5;242m\x1B[38;5;15m mysql "
      "\x1B[0m\x1B[48;5;221m\x1B[38;5;0m JS \x1B[0m\n\x1B[48;5;0m  > \x1B[0m");
#else
  EXPECT_DBL_PROMPT(
      "\x1B[48;5;254m\x1B[38;5;23m My\x1B[0m\x1B[48;5;254m\x1B[38;5;166mSQL "
      "\x1B[0m\x1B[48;5;237m\x1B[38;5;15m " +
      _host + ":" + _port +
      "+ ssl "
      "\x1B[0m\x1B[48;5;242m\x1B[38;5;15m mysql "
      "\x1B[0m\x1B[48;5;25m\x1B[38;5;15m Py \x1B[0m\n\x1B[48;5;0m  > \x1B[0m");
#endif

  shcore::unsetenv("MYSQLSH_PROMPT_THEME");
  shcore::unsetenv("MYSQLSH_TERM_COLOR_MODE");
}

TEST_F(Shell_prompt_exe, sample_prompt_theme_256pl) {
  shcore::setenv("MYSQLSH_PROMPT_THEME",
                 shcore::get_binary_folder() + "/prompt_256pl.json");
  shcore::setenv("MYSQLSH_TERM_COLOR_MODE", "256");

  int rc =
      execute({_mysqlsh, "--interactive=full", _uri.c_str(), "--schema=mysql",
               "--ssl-mode=REQUIRED", "-e", "1", nullptr});
  EXPECT_EQ(0, rc);
  std::cout << _output << "\n";

#ifdef HAVE_V8
  EXPECT_PROMPT(
      "\x1B[48;5;254m\x1B[38;5;23m My\x1B[0m\x1B[48;5;254m\x1B[38;5;166mSQL "
      "\x1B[48;5;237m\x1B[38;5;254m\xEE\x82\xB0\x1B[0m\x1B[48;5;237m\x1B[38;5;"
      "15m " +
      _host + ":" + _port +
      "+ \xEE\x82\xA2 "
      "\x1B[48;5;242m\x1B[38;5;237m\xEE\x82\xB0\x1B[0m\x1B[48;5;242m\x1B[38;5;"
      "15m mysql "
      "\x1B[48;5;221m\x1B[38;5;242m\xEE\x82\xB0\x1B[0m\x1B[48;5;221m\x1B[38;5;"
      "0m JS \x1B[0m\x1B[48;5;0m\x1B[38;5;221m\xEE\x82\xB0 \x1B[0m");
#else
  EXPECT_PROMPT(
      "\x1B[48;5;254m\x1B[38;5;23m My\x1B[0m\x1B[48;5;254m\x1B[38;5;166mSQL "
      "\x1B[48;5;237m\x1B[38;5;254m\xEE\x82\xB0\x1B[0m\x1B[48;5;237m\x1B[38;5;"
      "15m " +
      _host + ":" + _port +
      "+ \xEE\x82\xA2 "
      "\x1B[48;5;242m\x1B[38;5;237m\xEE\x82\xB0\x1B[0m\x1B[48;5;242m\x1B[38;5;"
      "15m mysql "
      "\x1B[48;5;25m\x1B[38;5;242m\xEE\x82\xB0\x1B[0m\x1B[48;5;25m\x1B[38;5;"
      "15m Py \x1B[0m\x1B[48;5;0m\x1B[38;5;25m\xEE\x82\xB0 \x1B[0m");
#endif

  shcore::unsetenv("MYSQLSH_PROMPT_THEME");
  shcore::unsetenv("MYSQLSH_TERM_COLOR_MODE");
}

#ifdef HAVE_V8
TEST_F(Shell_prompt_exe, bug28314383_js) {
  static constexpr auto k_file = "close.js";
  shcore::setenv("MYSQLSH_PROMPT_THEME",
                 shcore::get_binary_folder() + "/prompt_nocolor.json");
  shcore::setenv("MYSQLSH_TERM_COLOR_MODE", "nocolor");
  shcore::create_file(k_file,
                      "session.close();\n"
                      "\\connect " +
                          _uri +
                          "?ssl-mode=REQUIRED\n"
                          "session.close();\n");

  int rc = execute({_mysqlsh, "--interactive=full", _uri.c_str(),
                    "--schema=mysql", "--ssl-mode=REQUIRED", "--js", nullptr},
                   nullptr, k_file);
  EXPECT_EQ(0, rc);
  std::cout << _output << "\n";

  MY_EXPECT_CMD_OUTPUT_CONTAINS("MySQL [" + _host +
                                "+ ssl/mysql] JS> session.close();\n"
                                "MySQL JS> \\connect " +
                                _uri + "?ssl-mode=REQUIRED\n");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("MySQL [" + _host +
                                "+ ssl] JS> session.close();\n"
                                "MySQL JS> Bye!");

  shcore::unsetenv("MYSQLSH_PROMPT_THEME");
  shcore::unsetenv("MYSQLSH_TERM_COLOR_MODE");
  shcore::delete_file(k_file);
}
#endif  // HAVE_V8

#ifdef HAVE_PYTHON
TEST_F(Shell_prompt_exe, bug28314383_py) {
  static constexpr auto k_file = "close.py";
  shcore::setenv("MYSQLSH_PROMPT_THEME",
                 shcore::get_binary_folder() + "/prompt_nocolor.json");
  shcore::setenv("MYSQLSH_TERM_COLOR_MODE", "nocolor");
  shcore::create_file(k_file,
                      "session.close();\n"
                      "\\connect " +
                          _uri +
                          "?ssl-mode=REQUIRED\n"
                          "session.close();\n");

  int rc = execute({_mysqlsh, "--interactive=full", _uri.c_str(),
                    "--schema=mysql", "--ssl-mode=REQUIRED", "--py", nullptr},
                   nullptr, k_file);
  EXPECT_EQ(0, rc);
  std::cout << _output << "\n";

  MY_EXPECT_CMD_OUTPUT_CONTAINS("MySQL [" + _host +
                                "+ ssl/mysql] Py> session.close();\n"
                                "MySQL Py> \\connect " +
                                _uri + "?ssl-mode=REQUIRED\n");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("MySQL [" + _host +
                                "+ ssl] Py> session.close();\n"
                                "MySQL Py> Bye!");

  shcore::unsetenv("MYSQLSH_PROMPT_THEME");
  shcore::unsetenv("MYSQLSH_TERM_COLOR_MODE");
  shcore::delete_file(k_file);
}
#endif  // HAVE_PYTHON

#undef EXPECT_PROMPT

}  // namespace mysqlsh
