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

#ifndef MYSQLSHDK_LIBS_TEXTUI_TEXTUI_H_
#define MYSQLSHDK_LIBS_TEXTUI_TEXTUI_H_

#include <map>
#include <string>
#include <utility>
#include <vector>
#include "mysqlshdk/libs/textui/term_vt100.h"
#include "mysqlshdk_export.h"  // NOLINT

namespace mysqlshdk {
namespace textui {
namespace internal {
/**
 * Structure to hold color highlights, including:
 * - Position where coloring starts
 * - Length where coloring ends
 *
 * NOTE: just highlights (bold) are being used atm so nothing else
 * is required for now.
 */
enum class Text_style { Bold, Warning };

typedef std::tuple<size_t, size_t, Text_style> Highlight;
typedef std::vector<Highlight> Highlights;

std::string preprocess_markup(const std::string &line, Highlights *highlights);

void postprocess_markup(std::vector<std::string> *lines,
                        const Highlights &highlights);

std::vector<std::string> get_sized_strings(const std::string &input,
                                           size_t size);
}  // namespace internal

enum Color_capability { No_color, Color_16, Color_256, Color_rgb };

void SHCORE_PUBLIC set_color_capability(Color_capability cap);

inline Color_capability get_color_capability() {
  extern Color_capability g_color_capability;
  return g_color_capability;
}

inline bool has_color() {
  extern Color_capability g_color_capability;
  return g_color_capability != No_color;
}

/**
 * Checks if operations which control screen are supported (i.e erasing the
 * screen contents).
 *
 * @returns true if screen operations are supported.
 */
bool SHCORE_PUBLIC supports_screen_control();

/**
 * Clears the whole screen maintaining previous output. Positions the cursor in
 * the upper left corner.
 */
void SHCORE_PUBLIC scroll_screen();

/**
 * Clears the whole screen erasing previous output. Positions the cursor in
 * the upper left corner.
 */
void SHCORE_PUBLIC clear_screen();

bool SHCORE_PUBLIC parse_rgb(const std::string &color, uint8_t rgb[3]);
int SHCORE_PUBLIC parse_color_set(const std::string &color_spec,
                                  uint8_t *color_16, uint8_t *color_256,
                                  uint8_t color_rgb[3],
                                  bool convert_missing = true);

struct SHCORE_PUBLIC Style {
  static const int Attributes_set = 1;
  static const int Color_16_fg_set = 2;
  static const int Color_256_fg_set = 4;
  static const int Color_rgb_fg_set = 8;
  static const int Color_16_bg_set = 16;
  static const int Color_256_bg_set = 32;
  static const int Color_rgb_bg_set = 64;

  static const int Color_fg_mask =
      (Color_16_fg_set | Color_256_fg_set | Color_rgb_fg_set);
  static const int Color_bg_mask =
      (Color_16_bg_set | Color_256_bg_set | Color_rgb_bg_set);

  struct Color {
    uint8_t color_16 = 0;
    uint8_t color_256 = 0;
    uint8_t color_rgb[3] = {0, 0, 0};

    Color() {}

    Color(const Color &c) : color_16(c.color_16), color_256(c.color_256) {
      color_rgb[0] = c.color_rgb[0];
      color_rgb[1] = c.color_rgb[1];
      color_rgb[2] = c.color_rgb[2];
    }

    Color(uint8_t c16, uint8_t c256, const uint8_t rgb[3])
        : color_16(c16), color_256(c256) {
      color_rgb[0] = rgb[0];
      color_rgb[1] = rgb[1];
      color_rgb[2] = rgb[2];
    }

    bool operator==(const Color &c) const {
      return color_16 == c.color_16 && color_256 == c.color_256 &&
             color_rgb[0] == c.color_rgb[0] && color_rgb[1] == c.color_rgb[1] &&
             color_rgb[2] == c.color_rgb[2];
    }

    bool operator!=(const Color &c) const { return !(*this == c); }
  };

  Style() {}

  Style(const Color &afg, const Color &abg) : fg(afg), bg(abg) {
    field_mask = Color_fg_mask | Color_bg_mask;
  }

  explicit Style(const Color &afg) : fg(afg) { field_mask = Color_fg_mask; }

  int field_mask = 0;

  Color fg;
  Color bg;

  bool bold = false;
  bool underline = false;

  static Style parse(const std::map<std::string, std::string> &attributes);

  static Style clear() {
    Style style;
    style.field_mask = Attributes_set;
    return style;
  }

  bool compare(const Style &style,
               int mask = Attributes_set | Color_fg_mask | Color_bg_mask) const;

  operator std::string() const;

  operator bool() const { return field_mask != 0; }

  void merge(const Style &style);
};

inline std::string strip_colors(const std::string &text) {
  std::string s;
  size_t p = 0;
  auto pos = text.find("\033[");
  while (pos != std::string::npos) {
    s.append(text.substr(p, pos - p));
    auto end = text.find("m", pos);
    if (end == std::string::npos) break;
    p = end + 1;
    pos = text.find("\033[", p);
  }
  s.append(text.substr(p));
  return s;
}

inline size_t strip_color_length(const std::string &text) {
  size_t len = 0;
  size_t p = 0;
  auto pos = text.find("\033[");
  while (pos != std::string::npos) {
    len += pos - p;
    auto end = text.find("m", pos);
    if (end == std::string::npos) break;
    p = end + 1;
    pos = text.find("\033[", p);
  }
  len += text.size() - p;
  return len;
}

inline std::string bold(const std::string &text) {
  if (!has_color()) return text;
  return vt100::attr(-1, -1, vt100::Bright) + text + vt100::attr();
}

inline std::string error(const std::string &text) {
  if (!has_color()) return text;
  return vt100::attr(1, -1) + text + vt100::attr();
}

inline std::string alert(const std::string &text) {
  if (!has_color()) return text;
  return vt100::attr(1, -1) + text + vt100::attr();
}

inline std::string warning(const std::string &text) {
  if (!has_color()) return text;
  return vt100::attr(3, -1) + text + vt100::attr();
}

inline std::string notice(const std::string &text) {
  if (!has_color()) return text;
  return vt100::attr(6, -1) + text + vt100::attr();
}

inline std::string green(const std::string &text) {
  if (!has_color()) return text;
  return vt100::attr(2, -1) + text + vt100::attr();
}

inline std::string yellow(const std::string &text) {
  if (!has_color()) return text;
  return vt100::attr(3, -1) + text + vt100::attr();
}

inline std::string red(const std::string &text) {
  if (!has_color()) return text;
  return vt100::attr(1, -1) + text + vt100::attr();
}

/**
 * Creates a remark.
 *
 * If terminal does not support color output, text is going to be enclosed in
 * single quotes, unless it contains a single quote, then it's going to be
 * enclosed in double quotes. If terminal supports color output, text is going
 * to be bold.
 *
 * @param text A text to be formatted.
 * @return Formatted text.
 */
inline std::string remark(const std::string &text) {
  const char quote = text.find('\'') == std::string::npos ? '\'' : '"';
  if (!has_color()) return quote + text + quote;
  return bold(text);
}

/**
 * Multiparagraph markup formatting function.
 *
 * Each received line will create a paragraph which could be separated by the
 * others either by a new line or a new line + a blank line.
 *
 * It also performs the correct formatting of bulleted lists: consecutive lines
 * starting with @li will create a bulleted list.
 *
 * This function depends on the single string version of format_markup_text to
 * achieve it's objective.
 */
std::string format_markup_text(const std::vector<std::string> &lines,
                               size_t width, size_t left_padding,
                               bool paragraph_per_line = true);

/**
 * This function allows applying format to a string for proper display in the
 * console. At the moment supported formatting includes:
 *
 * - Removal of @code and @endcode tags.
 * - Use of <b> and </b> tags to create highlighted text.
 * - Use of @li to create a bulleted list
 * - Replacement of @< by "<"
 * - Replacement of @> by ">"
 * - Replacement of &nbsp; by " "
 * - Replacement of @li by "-" (To create bullet list)
 *
 * These formatting options are required to use the same documentation for
 * online help and the doxygen generated documentation.
 *
 * @IMPORTANT: Any change or addition to the formatting options MUST be
 * compliant with the Doxygen formatting options.
 *
 * In addition to the formatting the function allows returning strings of a
 * specific size and has the option for padding.
 *
 * @param line is the line to be formatted.
 * @param width is the max width per line of the returned string.
 * @param left_padding is the number of whitespaces to be added at the beggining
 * of every returned line.
 *
 * This function depends on the following functions to achieve it's purpose:
 * - preprocess_markup: does tag replacement and extracts highlights for <b></b>
 * - get_sized_strings: splits the input string as required to guarantee output
 *   lines of complete words of max width size.
 * - postprocess_markup: uses th highlights extracted on preprocess_markup to
 *   format the text with the corresponding escape sequences
 */
std::string format_markup_text(const std::string &line, size_t width,
                               size_t left_padding);
}  // namespace textui
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_TEXTUI_TEXTUI_H_
