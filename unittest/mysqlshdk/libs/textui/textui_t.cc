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

#include <gtest/gtest.h>
#include <initializer_list>
#include "mysqlshdk/libs/textui/term_vt100.h"
#include "mysqlshdk/libs/textui/textui.h"

namespace mysqlshdk {
namespace textui {
namespace internal {
TEST(Textui, get_sized_strings) {
  std::string input = "123 456 789 012 345";
  auto substrings = get_sized_strings(input, 19);
  EXPECT_EQ(1U, substrings.size());
  EXPECT_EQ(input, substrings[0]);

  substrings = get_sized_strings(input, 20);
  EXPECT_EQ(1U, substrings.size());
  EXPECT_EQ(input, substrings[0]);

  substrings = get_sized_strings(input, 18);
  EXPECT_EQ(2U, substrings.size());
  EXPECT_EQ("123 456 789 012 ", substrings[0]);
  EXPECT_EQ("345", substrings[1]);

  substrings = get_sized_strings(input, 8);
  EXPECT_EQ(3U, substrings.size());
  EXPECT_EQ("123 456 ", substrings[0]);
  EXPECT_EQ("789 012 ", substrings[1]);
  EXPECT_EQ("345", substrings[2]);

  substrings = get_sized_strings(input, 7);
  EXPECT_EQ(3U, substrings.size());
  EXPECT_EQ("123 456 ", substrings[0]);
  EXPECT_EQ("789 012 ", substrings[1]);
  EXPECT_EQ("345", substrings[2]);

  substrings = get_sized_strings(input, 6);
  EXPECT_EQ(5U, substrings.size());
  EXPECT_EQ("123 ", substrings[0]);
  EXPECT_EQ("456 ", substrings[1]);
  EXPECT_EQ("789 ", substrings[2]);
  EXPECT_EQ("012 ", substrings[3]);
  EXPECT_EQ("345", substrings[4]);

  substrings = get_sized_strings("123   456   789", 4);
  EXPECT_EQ(3U, substrings.size());
  EXPECT_EQ("123   ", substrings[0]);
  EXPECT_EQ("456   ", substrings[1]);
  EXPECT_EQ("789", substrings[2]);

  substrings = get_sized_strings("123 456 789", 6);
  EXPECT_EQ(3U, substrings.size());
  EXPECT_EQ("123 ", substrings[0]);
  EXPECT_EQ("456 ", substrings[1]);
  EXPECT_EQ("789", substrings[2]);

  substrings = get_sized_strings("12345 6789 01234 563 1234", 4);
  EXPECT_EQ(5U, substrings.size());
  EXPECT_EQ("12345 ", substrings[0]);
  EXPECT_EQ("6789 ", substrings[1]);
  EXPECT_EQ("01234 ", substrings[2]);
  EXPECT_EQ("563 ", substrings[3]);
  EXPECT_EQ("1234", substrings[4]);
}

TEST(Textui, preprocess_markup) {
  {
    Highlights highlights;
    auto result = preprocess_markup(
        "@code"
        "var sample = 3;"
        "@endcode",
        &highlights);
    EXPECT_TRUE(highlights.empty());
    EXPECT_EQ("var sample = 3;", result);

    result = preprocess_markup(
        "Some text @code"
        "var sample = 3;"
        "@endcode",
        &highlights);
    EXPECT_TRUE(highlights.empty());
    EXPECT_EQ("Some text var sample = 3;", result);

    result = preprocess_markup(
        "@code"
        "var sample = 3;"
        "@endcode "
        "Some text",
        &highlights);
    EXPECT_TRUE(highlights.empty());
    EXPECT_EQ("var sample = 3; Some text", result);

    result = preprocess_markup(
        "Some @code"
        "var sample = 3;"
        "@endcode text",
        &highlights);
    EXPECT_TRUE(highlights.empty());
    EXPECT_EQ("Some var sample = 3; text", result);

    result = preprocess_markup(
        "@code"
        "var sample = 3;"
        "@endcode"
        "Some text "
        "@code"
        "var other = 3;"
        "@endcode",
        &highlights);
    EXPECT_TRUE(highlights.empty());
    EXPECT_EQ("var sample = 3;Some text var other = 3;", result);
  }

  {
    Highlights highlights;
    auto result = preprocess_markup("@<@>&nbsp;@li@<@>&nbsp;@li", &highlights);
    EXPECT_TRUE(highlights.empty());
    EXPECT_EQ("<> -<> -", result);
  }

  {
    Highlights highlights;
    auto result = preprocess_markup("<b>12345</b>", &highlights);
    EXPECT_EQ(1U, highlights.size());
    EXPECT_EQ(0U, std::get<0>(highlights[0]));
    EXPECT_EQ(5U, std::get<1>(highlights[0]));
    EXPECT_EQ(Text_style::Bold, std::get<2>(highlights[0]));
    EXPECT_EQ("12345", result);
    highlights.clear();

    result = preprocess_markup("Some text <w>12345</w>", &highlights);
    EXPECT_EQ(1U, highlights.size());
    EXPECT_EQ(10U, std::get<0>(highlights[0]));
    EXPECT_EQ(5U, std::get<1>(highlights[0]));
    EXPECT_EQ(Text_style::Warning, std::get<2>(highlights[0]));
    EXPECT_EQ("Some text 12345", result);
    highlights.clear();

    result = preprocess_markup("<b>123</b> Some text", &highlights);
    EXPECT_EQ(1U, highlights.size());
    EXPECT_EQ(0U, std::get<0>(highlights[0]));
    EXPECT_EQ(3U, std::get<1>(highlights[0]));
    EXPECT_EQ("123 Some text", result);
    highlights.clear();

    result = preprocess_markup("Some <b>123456</b> text", &highlights);
    EXPECT_EQ(1U, highlights.size());
    EXPECT_EQ(5U, std::get<0>(highlights[0]));
    EXPECT_EQ(6U, std::get<1>(highlights[0]));
    EXPECT_EQ("Some 123456 text", result);
    highlights.clear();

    result = preprocess_markup(
        "<b>12</b>"
        "Some text "
        "<w>123456789</w>",
        &highlights);
    EXPECT_EQ(2U, highlights.size());
    EXPECT_EQ(0U, std::get<0>(highlights[0]));
    EXPECT_EQ(2U, std::get<1>(highlights[0]));
    EXPECT_EQ(Text_style::Bold, std::get<2>(highlights[0]));
    EXPECT_EQ(12U, std::get<0>(highlights[1]));
    EXPECT_EQ(9U, std::get<1>(highlights[1]));
    EXPECT_EQ(Text_style::Warning, std::get<2>(highlights[1]));
    EXPECT_EQ("12Some text 123456789", result);
    highlights.clear();
  }
}

TEST(Textui, postprocess_markup_bold) {
  set_color_capability(Color_16);
  std::string B = "\x1B[1m";  // sequence for <b>
  std::string b = "\x1B[0m";  // sequence for </b>

  // Single post markup
  {
    std::vector<std::string> lines = {"1234567890", "1234567890"};
    postprocess_markup(&lines, {std::make_tuple(0, 3, Text_style::Bold)});

    EXPECT_EQ(B + "123" + b + "4567890", lines[0]);
    EXPECT_EQ("1234567890", lines[1]);
  }

  // Double markup on the same line
  {
    std::vector<std::string> lines = {"1234567890", "1234567890"};
    postprocess_markup(&lines, {std::make_tuple(0, 3, Text_style::Bold),
                                std::make_tuple(5, 2, Text_style::Bold)});

    EXPECT_EQ(B + "123" + b + "45" + B + "67" + b + "890", lines[0]);
    EXPECT_EQ("1234567890", lines[1]);
  }

  // Double markup, end of line beggining of another
  {
    std::vector<std::string> lines = {"1234567890", "1234567890"};
    postprocess_markup(&lines, {std::make_tuple(2, 2, Text_style::Bold),
                                std::make_tuple(9, 2, Text_style::Bold)});

    EXPECT_EQ("12" + B + "34" + b + "56789" + B + "0" + b, lines[0]);
    EXPECT_EQ(B + "1" + b + "234567890", lines[1]);
  }

  // Double markup, different lines
  {
    std::vector<std::string> lines = {"1234567890", "1234567890"};
    postprocess_markup(&lines, {std::make_tuple(2, 2, Text_style::Bold),
                                std::make_tuple(12, 3, Text_style::Bold)});

    EXPECT_EQ("12" + B + "34" + b + "567890", lines[0]);
    EXPECT_EQ("12" + B + "345" + b + "67890", lines[1]);
  }

  // markup, covers even 3 lines (1 complete)
  {
    std::vector<std::string> lines = {"12345", "67890", "12345", "67890"};
    postprocess_markup(&lines, {std::make_tuple(2, 10, Text_style::Bold)});

    EXPECT_EQ("12" + B + "345" + b, lines[0]);
    EXPECT_EQ(B + "67890" + b, lines[1]);
    EXPECT_EQ(B + "12" + b + "345", lines[2]);
    EXPECT_EQ("67890", lines[3]);
  }
}

TEST(Textui, postprocess_markup_warning) {
  set_color_capability(Color_16);
  std::string W = "\x1B[33m";
  std::string w = "\x1B[0m";

  // Single post markup
  {
    std::vector<std::string> lines = {"1234567890", "1234567890"};
    postprocess_markup(&lines, {std::make_tuple(0, 3, Text_style::Warning)});

    EXPECT_EQ(W + "123" + w + "4567890", lines[0]);
    EXPECT_EQ("1234567890", lines[1]);
  }

  // Double markup on the same line
  {
    std::vector<std::string> lines = {"1234567890", "1234567890"};
    postprocess_markup(&lines, {std::make_tuple(0, 3, Text_style::Warning),
                                std::make_tuple(5, 2, Text_style::Warning)});

    EXPECT_EQ(W + "123" + w + "45" + W + "67" + w + "890", lines[0]);
    EXPECT_EQ("1234567890", lines[1]);
  }

  // Double markup, end of line beggining of another
  {
    std::vector<std::string> lines = {"1234567890", "1234567890"};
    postprocess_markup(&lines, {std::make_tuple(2, 2, Text_style::Warning),
                                std::make_tuple(9, 2, Text_style::Warning)});

    EXPECT_EQ("12" + W + "34" + w + "56789" + W + "0" + w, lines[0]);
    EXPECT_EQ(W + "1" + w + "234567890", lines[1]);
  }

  // Double markup, different lines
  {
    std::vector<std::string> lines = {"1234567890", "1234567890"};
    postprocess_markup(&lines, {std::make_tuple(2, 2, Text_style::Warning),
                                std::make_tuple(12, 3, Text_style::Warning)});

    EXPECT_EQ("12" + W + "34" + w + "567890", lines[0]);
    EXPECT_EQ("12" + W + "345" + w + "67890", lines[1]);
  }

  // markup, covers even 3 lines (1 complete)
  {
    std::vector<std::string> lines = {"12345", "67890", "12345", "67890"};
    postprocess_markup(&lines, {std::make_tuple(2, 10, Text_style::Warning)});

    EXPECT_EQ("12" + W + "345" + w, lines[0]);
    EXPECT_EQ(W + "67890" + w, lines[1]);
    EXPECT_EQ(W + "12" + w + "345", lines[2]);
    EXPECT_EQ("67890", lines[3]);
  }
}

TEST(Textui, postprocess_markup_combined) {
  set_color_capability(Color_16);
  std::string W = "\x1B[33m";  // sequence for <w>
  std::string w = "\x1B[0m";   // sequence for </w>
  std::string B = "\x1B[1m";   // sequence for <b>
  std::string b = "\x1B[0m";   // sequence for </b>

  // Double markup on the same line
  {
    std::vector<std::string> lines = {"1234567890", "1234567890"};
    postprocess_markup(&lines, {std::make_tuple(0, 3, Text_style::Bold),
                                std::make_tuple(5, 2, Text_style::Warning)});

    EXPECT_EQ(B + "123" + b + "45" + W + "67" + w + "890", lines[0]);
    EXPECT_EQ("1234567890", lines[1]);
  }

  // Double markup, end of line beggining of another
  {
    std::vector<std::string> lines = {"1234567890", "1234567890"};
    postprocess_markup(&lines, {std::make_tuple(2, 2, Text_style::Warning),
                                std::make_tuple(9, 2, Text_style::Bold)});

    EXPECT_EQ("12" + W + "34" + w + "56789" + B + "0" + b, lines[0]);
    EXPECT_EQ(B + "1" + b + "234567890", lines[1]);
  }

  // Double markup, different lines
  {
    std::vector<std::string> lines = {"1234567890", "1234567890"};
    postprocess_markup(&lines, {std::make_tuple(2, 2, Text_style::Bold),
                                std::make_tuple(12, 3, Text_style::Warning)});

    EXPECT_EQ("12" + B + "34" + b + "567890", lines[0]);
    EXPECT_EQ("12" + W + "345" + w + "67890", lines[1]);
  }
}
}  // namespace internal

TEST(Textui, parse_color_set) {
  uint8_t color_16 = 0;
  uint8_t color_256 = 0;
  uint8_t color_rgb[3] = {0, 0, 0};
  EXPECT_EQ(
      Style::Color_rgb_fg_set,
      parse_color_set("#112233", &color_16, &color_256, color_rgb, false));
  EXPECT_EQ(0, color_16);
  EXPECT_EQ(0, color_256);
  EXPECT_EQ(0x11, color_rgb[0]);
  EXPECT_EQ(0x22, color_rgb[1]);
  EXPECT_EQ(0x33, color_rgb[2]);

  color_16 = 0;
  color_256 = 0;
  color_rgb[0] = 0;
  color_rgb[1] = 0;
  color_rgb[2] = 0;
  EXPECT_EQ(Style::Color_rgb_fg_set,
            parse_color_set("#abc", &color_16, &color_256, color_rgb, false));
  EXPECT_EQ(0, color_16);
  EXPECT_EQ(0, color_256);
  EXPECT_EQ(0xa0, color_rgb[0]);
  EXPECT_EQ(0xb0, color_rgb[1]);
  EXPECT_EQ(0xc0, color_rgb[2]);

  color_16 = 0;
  color_256 = 0;
  color_rgb[0] = 0;
  color_rgb[1] = 0;
  color_rgb[2] = 0;
  EXPECT_EQ(Style::Color_256_fg_set,
            parse_color_set("123", &color_16, &color_256, color_rgb, false));
  EXPECT_EQ(0, color_16);
  EXPECT_EQ(123, color_256);
  EXPECT_EQ(0x0, color_rgb[0]);
  EXPECT_EQ(0x0, color_rgb[1]);
  EXPECT_EQ(0x0, color_rgb[2]);

  color_16 = 0;
  color_256 = 0;
  color_rgb[0] = 0;
  color_rgb[1] = 0;
  color_rgb[2] = 0;
  EXPECT_EQ(Style::Color_16_fg_set,
            parse_color_set("red", &color_16, &color_256, color_rgb, false));
  EXPECT_EQ(1, color_16);
  EXPECT_EQ(0, color_256);
  EXPECT_EQ(0x0, color_rgb[0]);
  EXPECT_EQ(0x0, color_rgb[1]);
  EXPECT_EQ(0x0, color_rgb[2]);

  color_16 = 0;
  color_256 = 0;
  color_rgb[0] = 0;
  color_rgb[1] = 0;
  color_rgb[2] = 0;
  EXPECT_EQ(Style::Color_16_fg_set,
            parse_color_set("green", &color_16, &color_256, color_rgb, false));
  EXPECT_EQ(2, color_16);
  EXPECT_EQ(0, color_256);
  EXPECT_EQ(0x0, color_rgb[0]);
  EXPECT_EQ(0x0, color_rgb[1]);
  EXPECT_EQ(0x0, color_rgb[2]);

  color_16 = 0;
  color_256 = 0;
  color_rgb[0] = 0;
  color_rgb[1] = 0;
  color_rgb[2] = 0;
  EXPECT_EQ(Style::Color_16_fg_set,
            parse_color_set("blue", &color_16, &color_256, color_rgb, false));
  EXPECT_EQ(4, color_16);
  EXPECT_EQ(0, color_256);
  EXPECT_EQ(0x0, color_rgb[0]);
  EXPECT_EQ(0x0, color_rgb[1]);
  EXPECT_EQ(0x0, color_rgb[2]);

  color_16 = 0;
  color_256 = 0;
  color_rgb[0] = 0;
  color_rgb[1] = 0;
  color_rgb[2] = 0;
  EXPECT_EQ(Style::Color_16_fg_set,
            parse_color_set("yellow", &color_16, &color_256, color_rgb, false));
  EXPECT_EQ(3, color_16);
  EXPECT_EQ(0, color_256);
  EXPECT_EQ(0x0, color_rgb[0]);
  EXPECT_EQ(0x0, color_rgb[1]);
  EXPECT_EQ(0x0, color_rgb[2]);

  color_16 = 0;
  color_256 = 0;
  color_rgb[0] = 0;
  color_rgb[1] = 0;
  color_rgb[2] = 0;
  EXPECT_EQ(Style::Color_16_fg_set,
            parse_color_set("black", &color_16, &color_256, color_rgb, false));
  EXPECT_EQ(0, color_16);
  EXPECT_EQ(0, color_256);
  EXPECT_EQ(0x0, color_rgb[0]);
  EXPECT_EQ(0x0, color_rgb[1]);
  EXPECT_EQ(0x0, color_rgb[2]);

  color_16 = 0;
  color_256 = 0;
  color_rgb[0] = 0;
  color_rgb[1] = 0;
  color_rgb[2] = 0;
  EXPECT_EQ(Style::Color_16_fg_set,
            parse_color_set("white", &color_16, &color_256, color_rgb, false));
  EXPECT_EQ(7, color_16);
  EXPECT_EQ(0, color_256);
  EXPECT_EQ(0x0, color_rgb[0]);
  EXPECT_EQ(0x0, color_rgb[1]);
  EXPECT_EQ(0x0, color_rgb[2]);

  color_16 = 0;
  color_256 = 0;
  color_rgb[0] = 0;
  color_rgb[1] = 0;
  color_rgb[2] = 0;
  EXPECT_EQ(
      Style::Color_16_fg_set | Style::Color_256_fg_set,
      parse_color_set("white;47", &color_16, &color_256, color_rgb, false));
  EXPECT_EQ(7, color_16);
  EXPECT_EQ(47, color_256);
  EXPECT_EQ(0x0, color_rgb[0]);
  EXPECT_EQ(0x0, color_rgb[1]);
  EXPECT_EQ(0x0, color_rgb[2]);

  color_16 = 0;
  color_256 = 0;
  color_rgb[0] = 0;
  color_rgb[1] = 0;
  color_rgb[2] = 0;
  EXPECT_EQ(Style::Color_16_fg_set | Style::Color_rgb_fg_set,
            parse_color_set("#5566aa;white", &color_16, &color_256, color_rgb,
                            false));
  EXPECT_EQ(7, color_16);
  EXPECT_EQ(0, color_256);
  EXPECT_EQ(0x55, color_rgb[0]);
  EXPECT_EQ(0x66, color_rgb[1]);
  EXPECT_EQ(0xaa, color_rgb[2]);

  color_16 = 0;
  color_256 = 0;
  color_rgb[0] = 0;
  color_rgb[1] = 0;
  color_rgb[2] = 0;
  EXPECT_EQ(Style::Color_16_fg_set | Style::Color_rgb_fg_set |
                Style::Color_256_fg_set,
            parse_color_set("78;#5566aa;white", &color_16, &color_256,
                            color_rgb, false));
  EXPECT_EQ(7, color_16);
  EXPECT_EQ(78, color_256);
  EXPECT_EQ(0x55, color_rgb[0]);
  EXPECT_EQ(0x66, color_rgb[1]);
  EXPECT_EQ(0xaa, color_rgb[2]);
}

TEST(Textui, parse_color_set_bad) {
  uint8_t color_16 = 0;
  uint8_t color_256 = 0;
  uint8_t color_rgb[3] = {0, 0, 0};

  color_16 = 0;
  color_256 = 0;
  color_rgb[0] = 0;
  color_rgb[1] = 0;
  color_rgb[2] = 0;
  EXPECT_EQ(
      Style::Color_16_fg_set,
      parse_color_set("white;red", &color_16, &color_256, color_rgb, false));
  EXPECT_EQ(1, color_16);
  EXPECT_EQ(0, color_256);
  EXPECT_EQ(0x0, color_rgb[0]);
  EXPECT_EQ(0x0, color_rgb[1]);
  EXPECT_EQ(0x0, color_rgb[2]);

  EXPECT_THROW(parse_color_set("white/red", &color_16, &color_256, color_rgb),
               std::invalid_argument);

  EXPECT_THROW(parse_color_set("pink", &color_16, &color_256, color_rgb),
               std::invalid_argument);

  EXPECT_THROW(parse_color_set("-23", &color_16, &color_256, color_rgb),
               std::invalid_argument);

  EXPECT_THROW(parse_color_set("300", &color_16, &color_256, color_rgb),
               std::invalid_argument);

  EXPECT_THROW(parse_color_set("300x", &color_16, &color_256, color_rgb),
               std::invalid_argument);

  EXPECT_THROW(parse_color_set("#7788", &color_16, &color_256, color_rgb),
               std::invalid_argument);

  EXPECT_THROW(parse_color_set("#rrggbb", &color_16, &color_256, color_rgb),
               std::invalid_argument);
}

TEST(Textui, parse_style) {
  Style style;

  style = Style::parse(std::map<std::string, std::string>{
      std::initializer_list<std::pair<std::string const, std::string>>{
          {"fg", "red"}, {"bg", "blue"}, {"bold", "true"}}});
  EXPECT_TRUE(style.field_mask & Style::Color_16_fg_set);
  EXPECT_TRUE(style.field_mask & Style::Color_16_bg_set);
  EXPECT_TRUE(style.field_mask & Style::Attributes_set);
  EXPECT_EQ(1, style.fg.color_16);
  EXPECT_EQ(4, style.bg.color_16);
  EXPECT_TRUE(style.bold);
  set_color_capability(Color_16);
  EXPECT_EQ("\x1B[1;31;44m", (std::string)style);

  style = Style::parse(std::map<std::string, std::string>{
      std::initializer_list<std::pair<std::string const, std::string>>{
          {"fg", "red;245"}, {"bg", "123"}}});
  EXPECT_TRUE(style.field_mask & Style::Color_16_fg_set);
  EXPECT_TRUE(style.field_mask & Style::Color_256_fg_set);
  EXPECT_TRUE(style.field_mask & Style::Color_256_bg_set);
  EXPECT_EQ(1, style.fg.color_16);
  EXPECT_EQ(245, style.fg.color_256);
  EXPECT_EQ(123, style.bg.color_256);
  set_color_capability(Color_256);
  EXPECT_EQ("\x1B[48;5;123m\x1B[38;5;245m", (std::string)style);

  style = Style::parse(std::map<std::string, std::string>{
      std::initializer_list<std::pair<std::string const, std::string>>{
          {"underline", "true"}, {"bold", "true"}}});
  EXPECT_EQ(Style::Attributes_set, style.field_mask);
  EXPECT_TRUE(style.bold);
  EXPECT_TRUE(style.underline);
}

TEST(Textui, compare_style) {
  Style src, dst;

  src.field_mask =
      Style::Color_fg_mask | Style::Color_bg_mask | Style::Attributes_set;
  dst.field_mask = Style::Attributes_set;

  EXPECT_FALSE(src.compare(dst));
  EXPECT_FALSE(src.compare(dst, Style::Color_fg_mask));
  EXPECT_TRUE(src.compare(dst, Style::Attributes_set));

  src.bold = true;
  EXPECT_FALSE(src.compare(dst, Style::Attributes_set));

  dst.fg.color_16 = 32;
  EXPECT_FALSE(src.compare(dst));
  EXPECT_FALSE(src.compare(dst, Style::Color_fg_mask));

  src.fg.color_16 = 2;
  src.fg.color_256 = 32;
  dst = src;

  EXPECT_TRUE(src.compare(dst));
  EXPECT_TRUE(src.compare(dst, Style::Color_fg_mask));

  src.fg.color_rgb[0] = 32;
  src.fg.color_rgb[1] = 32;
  src.fg.color_rgb[2] = 32;
  EXPECT_FALSE(src.compare(dst, Style::Color_fg_mask));

  EXPECT_TRUE(src.compare(dst, Style::Color_bg_mask));
  src.bg.color_rgb[0] = 32;
  dst.bg.color_rgb[0] = 32;
  EXPECT_TRUE(src.compare(dst, Style::Color_bg_mask));
  dst.bg.color_rgb[1] = 32;
  EXPECT_FALSE(src.compare(dst, Style::Color_bg_mask));
  dst.fg.color_rgb[0] = 32;
  dst.fg.color_rgb[1] = 32;
  dst.fg.color_rgb[2] = 32;
  EXPECT_FALSE(src.compare(dst, Style::Color_bg_mask));
  EXPECT_TRUE(src.compare(dst, Style::Color_fg_mask));

  dst = Style::clear();
  src = Style::clear();

  dst.field_mask = Style::Color_fg_mask;
  dst.fg.color_16 = 5;
  dst.fg.color_256 = 5;
  dst.fg.color_rgb[0] = 5;
  dst.fg.color_rgb[1] = 5;
  dst.fg.color_rgb[2] = 5;
  src.field_mask = Style::Color_fg_mask;
  src.fg.color_16 = 5;
  src.fg.color_256 = 5;
  src.fg.color_rgb[0] = 5;
  src.fg.color_rgb[1] = 5;
  src.fg.color_rgb[2] = 5;
  EXPECT_TRUE(src.compare(dst));
  src.fg.color_16 = 6;
  src.fg.color_256 = 5;
  src.fg.color_rgb[0] = 5;
  src.fg.color_rgb[1] = 5;
  src.fg.color_rgb[2] = 5;
  EXPECT_FALSE(src.compare(dst));
  src.fg.color_16 = 5;
  src.fg.color_256 = 6;
  src.fg.color_rgb[0] = 5;
  src.fg.color_rgb[1] = 5;
  src.fg.color_rgb[2] = 5;
  EXPECT_FALSE(src.compare(dst));
  src.fg.color_16 = 5;
  src.fg.color_256 = 5;
  src.fg.color_rgb[0] = 5;
  src.fg.color_rgb[1] = 5;
  src.fg.color_rgb[2] = 6;
  EXPECT_FALSE(src.compare(dst));
  src.fg.color_16 = 6;
  src.fg.color_256 = 6;
  src.fg.color_rgb[0] = 5;
  src.fg.color_rgb[1] = 5;
  src.fg.color_rgb[2] = 5;
  EXPECT_FALSE(src.compare(dst));
}

TEST(Textui, merge_style) {
  Style dst, src;

  src.field_mask =
      Style::Color_16_fg_set | Style::Color_rgb_bg_set | Style::Attributes_set;
  src.fg.color_16 = 3;
  src.fg.color_256 = 31;
  src.bg.color_rgb[0] = 32;
  src.bg.color_rgb[1] = 33;
  src.bg.color_rgb[2] = 34;
  src.underline = true;
  src.bold = false;

  dst.field_mask = Style::Color_16_fg_set | Style::Color_256_fg_set;
  dst.fg.color_16 = 42;
  dst.fg.color_256 = 111;
  dst.underline = false;
  dst.bold = true;

  dst.merge(src);

  EXPECT_EQ(Style::Color_16_fg_set | Style::Color_256_fg_set |
                Style::Color_rgb_bg_set | Style::Attributes_set,
            dst.field_mask);
  EXPECT_EQ(3, dst.fg.color_16);
  EXPECT_EQ(111, dst.fg.color_256);
  EXPECT_EQ(32, dst.bg.color_rgb[0]);
  EXPECT_EQ(33, dst.bg.color_rgb[1]);
  EXPECT_EQ(34, dst.bg.color_rgb[2]);
  EXPECT_TRUE(dst.underline);
  EXPECT_FALSE(dst.bold);

  src.field_mask = Style::Color_rgb_fg_set;
  src.fg.color_rgb[0] = 132;
  src.fg.color_rgb[1] = 133;
  src.fg.color_rgb[2] = 134;

  dst.field_mask = 0;
  dst.merge(src);
  EXPECT_EQ(Style::Color_rgb_fg_set, dst.field_mask);
  EXPECT_EQ(132, dst.fg.color_rgb[0]);
  EXPECT_EQ(133, dst.fg.color_rgb[1]);
  EXPECT_EQ(134, dst.fg.color_rgb[2]);
}

TEST(Textui, color_conversion) {
  Style style;

  style = Style::parse(std::map<std::string, std::string>{
      std::initializer_list<std::pair<std::string const, std::string>>{
          {"fg", "red"}, {"bg", "blue"}}});

  EXPECT_TRUE(style.field_mask & Style::Color_16_fg_set);
  EXPECT_TRUE(style.field_mask & Style::Color_16_bg_set);
  EXPECT_TRUE(style.field_mask & Style::Color_256_fg_set);
  EXPECT_TRUE(style.field_mask & Style::Color_256_bg_set);
  EXPECT_EQ(1, style.fg.color_16);
  EXPECT_EQ(4, style.bg.color_16);
  EXPECT_EQ(1, style.fg.color_256);
  EXPECT_EQ(4, style.bg.color_256);
}

TEST(Textui, color_capability) {
  Style style;

  style = Style::parse(std::map<std::string, std::string>{
      std::initializer_list<std::pair<std::string const, std::string>>{
          {"fg", "red;42;#112233"}, {"bg", "blue;100;#332211"}}});
  set_color_capability(Color_256);
  EXPECT_EQ("\x1B[48;5;100m\x1B[38;5;42m", (std::string)style);
  set_color_capability(Color_16);
  EXPECT_EQ("\x1B[31;44m", (std::string)style);
  set_color_capability(Color_rgb);
  EXPECT_EQ("\x1B[48;2;51;34;17m\x1B[38;2;17;34;51m", (std::string)style);
  set_color_capability(No_color);
  EXPECT_EQ("", (std::string)style);
}

TEST(Textui, term_vt100) {
  EXPECT_EQ("\x1B[2;31;41m", vt100::attr(1, 1, vt100::Dim));
  EXPECT_EQ("\x1B[5;31;41m", vt100::attr(1, 1, vt100::Blink));
  EXPECT_EQ("\x1B[7;31;41m", vt100::attr(1, 1, vt100::Reverse));
  EXPECT_EQ("\x1B[8;31;41m", vt100::attr(1, 1, vt100::Hidden));
  EXPECT_EQ("\x1B[1;5;31;41m", vt100::attr(1, 1, vt100::Bright | vt100::Blink));
}

TEST(Textui, format_markup_text_single) {
  set_color_capability(Color_16);
  std::string B = "\x1B[1m";  // sequence for <b>
  std::string b = "\x1B[0m";  // sequence for </b>

  std::string text1 = "@li <b>Sample</b> text @codewith code@endcode";

  // Single post markup
  {
    std::string formatted = format_markup_text(text1, 100, 0);
    EXPECT_EQ("- " + B + "Sample" + b + " text with code", formatted);
  }
  {
    std::string formatted = format_markup_text(text1, 100, 5);
    EXPECT_EQ("     - " + B + "Sample" + b + " text with code", formatted);
  }
  {
    std::string formatted = format_markup_text(text1, 10, 0);
    EXPECT_EQ("- " + B + "Sample" + b + "\ntext with\ncode", formatted);
  }

  std::string text2 = "@li <b>Sample text with no code</b> and formatted";
  {
    std::string formatted = format_markup_text(text2, 10, 0);
    EXPECT_EQ("- " + B + "Sample" + b + "\n" + B + "text with" + b + "\n" + B +
                  "no code" + b + "\nand\nformatted",
              formatted);
  }
}

TEST(Textui, format_markup_text_multiple) {
  set_color_capability(Color_16);
  std::string B = "\x1B[1m";  // sequence for <b>
  std::string b = "\x1B[0m";  // sequence for </b>

  {
    std::vector<std::string> lines = {"This is a <b>list</b>:", "@li one",
                                      "@li two", "and that is", "all about it"};

    std::string output = format_markup_text(lines, 60, 0, false);
    EXPECT_EQ("This is a " + B + "list" + b +
                  ":\n"
                  "\n"
                  "- one\n"
                  "- two\n"
                  "\n"
                  "and that is\n"
                  "all about it",
              output);

    output = format_markup_text(lines, 60, 0, true);
    EXPECT_EQ("This is a " + B + "list" + b +
                  ":\n"
                  "\n"
                  "- one\n"
                  "- two\n"
                  "\n"
                  "and that is\n"
                  "\n"
                  "all about it",
              output);
  }
}

TEST(Textui, remark_color) {
  set_color_capability(Color_16);

  EXPECT_EQ("\x1B[1m\x1B[0m", remark(""));
  EXPECT_EQ("\x1B[1mword\x1B[0m", remark("word"));
  EXPECT_EQ("\x1B[1mwhat's up\x1B[0m", remark("what's up"));
  EXPECT_EQ("\x1B[1m\"two words\"\x1B[0m", remark("\"two words\""));
}

TEST(Textui, remark_no_color) {
  set_color_capability(No_color);

  EXPECT_EQ("''", remark(""));
  EXPECT_EQ("'word'", remark("word"));
  EXPECT_EQ("\"what's up\"", remark("what's up"));
  EXPECT_EQ("'\"two words\"'", remark("\"two words\""));
}

}  // namespace textui
}  // namespace mysqlshdk
