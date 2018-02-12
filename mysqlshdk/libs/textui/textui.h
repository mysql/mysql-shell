/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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
#include "mysqlshdk_export.h"  // NOLINT
#include "mysqlshdk/libs/textui/term_vt100.h"

namespace mysqlshdk {
namespace textui {

enum Color_capability { No_color, Color_16, Color_256, Color_rgb };

void SHCORE_PUBLIC set_color_capability(Color_capability cap);

inline bool has_color() {
  extern Color_capability g_color_capability;
  return g_color_capability != No_color;
}

void SHCORE_PUBLIC cls();

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
    if (end == std::string::npos)
      break;
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
    if (end == std::string::npos)
      break;
    p = end + 1;
    pos = text.find("\033[", p);
  }
  len += text.size() - p;
  return len;
}

inline std::string bold(const std::string &text) {
  if (!has_color())
    return text;
  return vt100::attr(-1, -1, vt100::Bright) + text + vt100::attr();
}

inline std::string error(const std::string &text) {
  if (!has_color())
    return text;
  return vt100::attr(1, -1, vt100::Bright) + text + vt100::attr();
}

inline std::string alert(const std::string &text) {
  if (!has_color())
    return text;
  return vt100::attr(1, -1, vt100::Bright) + text + vt100::attr();
}

inline std::string warning(const std::string &text) {
  if (!has_color())
    return text;
  return vt100::attr(3, -1, vt100::Bright) + text + vt100::attr();
}

inline std::string notice(const std::string &text) {
  if (!has_color())
    return text;
  return vt100::attr(4, -1, vt100::Bright) + text + vt100::attr();
}

}  // namespace textui
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_TEXTUI_TEXTUI_H_
