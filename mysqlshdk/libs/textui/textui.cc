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

#include "mysqlshdk/libs/textui/textui.h"

#include <vector>

#include "mysqlshdk/libs/textui/term_vt100.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace textui {

Color_capability g_color_capability = Color_256;

void set_color_capability(Color_capability cap) {
  g_color_capability = cap;
}

bool parse_rgb(const std::string &color, uint8_t rgb[3]) {
  if (color.empty() || color[0] != '#' ||
      (color.length() != 7 && color.length() != 4))
    return false;
  if (color.length() == 7 && sscanf(color.c_str(), "#%2hhx%2hhx%2hhx", rgb + 0,
                                    rgb + 1, rgb + 2) == 3) {
    return true;
  } else if (color.length() == 4 && sscanf(color.c_str(), "#%1hhx%1hhx%1hhx",
                                           rgb + 0, rgb + 1, rgb + 2) == 3) {
    rgb[0] = rgb[0] << 4;
    rgb[1] = rgb[1] << 4;
    rgb[2] = rgb[2] << 4;
    return true;
  }
  return false;
}

/** Clear the screen */
void cls() {
  vt100::erase_screen();
}

int parse_color_set(const std::string &color_spec, uint8_t *color_16,
                    uint8_t *color_256, uint8_t color_rgb[3],
                    bool convert_missing) {
  static const char *color_names[] = {"black", "red",     "green", "yellow",
                                      "blue",  "magenta", "cyan",  "white"};
  int mask = 0;
  std::vector<std::string> values(shcore::split_string(color_spec, ";"));
  for (auto &color : values) {
    if (color.empty())
      continue;
    if (color[0] == '#') {
      if (!parse_rgb(color, color_rgb))
        throw std::invalid_argument("Invalid rgb color " + color);
      mask |= Style::Color_rgb_fg_set;
    } else if (isdigit(color[0])) {
      int tmp = std::atoi(color.c_str());
      if (tmp < 0 || tmp > 255)
        throw std::invalid_argument("Invalid color value " + color);
      *color_256 = tmp;
      mask |= Style::Color_256_fg_set;
    } else {
      for (size_t i = 0; i < sizeof(color_names) / sizeof(char *); i++) {
        if (shcore::str_caseeq(color_names[i], color.c_str())) {
          *color_16 = i;
          mask |= Style::Color_16_fg_set;
          break;
        }
      }
      if (mask == 0)
        throw std::invalid_argument("Invalid color value " + color);
    }
  }
  if (convert_missing) {
    if ((mask & Style::Color_16_fg_set) && !(mask & Style::Color_256_fg_set)) {
      mask |= Style::Color_256_fg_set;
      *color_256 = *color_16;
    }
    // Not much point in converting from 256 to rgb since most (all?)
    // terminals that support RGB will support 256 too
    // if ((mask & Style::Color_256_fg_set) &&
    //      !(mask & Style::Color_rgb_fg_set)) {
    //   mask |= Style::Color_rgb_fg_set;
    //   parse_rgb(k_rgb_values[*color_256], color_rgb);
    // }
  }
  return mask;
}

const int Style::Attributes_set;
const int Style::Color_16_fg_set;
const int Style::Color_256_fg_set;
const int Style::Color_rgb_fg_set;
const int Style::Color_16_bg_set;
const int Style::Color_256_bg_set;
const int Style::Color_rgb_bg_set;

const int Style::Color_fg_mask;
const int Style::Color_bg_mask;

/** Parse a text style map

  fg: <color>
  bg: <color>
  bold: true/false
  underline: true/false

  <color> requires from 1 to 3 color values, separated by ;

  <color> : <color_name>[;<256_color>[;<rgb_color>]]
  <color_name>: black|red|green|yellow|blue|magenta|cyan|white
  <256_color>: 0..256
  <rgb_color>: #rrggbb
*/
Style Style::parse(const std::map<std::string, std::string> &attributes) {
  Style style;

  for (auto &iter : attributes) {
    if (iter.first == "fg") {
      style.field_mask |=
          parse_color_set(iter.second, &style.fg.color_16, &style.fg.color_256,
                          style.fg.color_rgb);
    } else if (iter.first == "bg") {
      style.field_mask |=
          (parse_color_set(iter.second, &style.bg.color_16, &style.bg.color_256,
                           style.bg.color_rgb)
           << 3);
    } else if (iter.first == "bold") {
      style.field_mask |= Style::Attributes_set;
      style.bold = iter.second != "false" && iter.second != "0";
    } else if (iter.first == "underline") {
      style.field_mask |= Style::Attributes_set;
      style.underline = iter.second != "false" && iter.second != "0";
    }
  }
  return style;
}

Style::operator std::string() const {
  bool fg_set = false;
  bool bg_set = false;
  std::string s;
  switch (g_color_capability) {
    case No_color:
      break;
    case Color_rgb:
      if (field_mask & Style::Color_rgb_bg_set) {
        s += vt100::color_bg_rgb(bg.color_rgb);
        bg_set = true;
      }
      if (field_mask & Style::Color_rgb_fg_set) {
        s += vt100::color_fg_rgb(fg.color_rgb);
        fg_set = true;
      }
      // fallthrough
    case Color_256:
      if ((field_mask & Style::Color_256_bg_set) && !bg_set) {
        s += vt100::color_bg_256(bg.color_256);
        bg_set = true;
      }
      if ((field_mask & Style::Color_256_fg_set) && !fg_set) {
        s += vt100::color_fg_256(fg.color_256);
        fg_set = true;
      }
      // fallthrough
    case Color_16:
      if (((field_mask & Style::Color_16_fg_set) && !fg_set) ||
          ((field_mask & Style::Color_16_bg_set) && !bg_set) ||
          (field_mask & Style::Attributes_set))
        s += vt100::attr(
            (field_mask & Style::Color_16_fg_set) && !fg_set ? fg.color_16 : -1,
            (field_mask & Style::Color_16_bg_set) && !bg_set ? bg.color_16 : -1,
            (field_mask & Style::Attributes_set)
                ? ((bold ? vt100::Bright : 0) |
                   (underline ? vt100::Underline : 0))
                : 0);
      break;
  }
  return s;
}

bool Style::compare(const Style &style, int mask) const {
  if ((field_mask & mask) != (style.field_mask & mask))
    return false;

  if ((field_mask & mask) & Style::Color_16_fg_set) {
    if (fg.color_16 != style.fg.color_16)
      return false;
  }
  if ((field_mask & mask) & Style::Color_256_fg_set) {
    if (fg.color_256 != style.fg.color_256)
      return false;
  }
  if ((field_mask & mask) & Style::Color_rgb_fg_set) {
    if (fg.color_rgb[0] != style.fg.color_rgb[0] ||
        fg.color_rgb[1] != style.fg.color_rgb[1] ||
        fg.color_rgb[2] != style.fg.color_rgb[2])
      return false;
  }
  if ((field_mask & mask) & Style::Color_16_bg_set) {
    if (bg.color_16 != style.bg.color_16)
      return false;
  }
  if ((field_mask & mask) & Style::Color_256_bg_set) {
    if (bg.color_256 != style.bg.color_256)
      return false;
  }
  if ((field_mask & mask) & Style::Color_rgb_bg_set) {
    if (bg.color_rgb[0] != style.bg.color_rgb[0] ||
        bg.color_rgb[1] != style.bg.color_rgb[1] ||
        bg.color_rgb[2] != style.bg.color_rgb[2])
      return false;
  }
  if ((field_mask & mask) & Style::Attributes_set) {
    if (bold != style.bold || underline != style.underline)
      return false;
  }
  return true;
}

void Style::merge(const Style &style) {
  if (style.field_mask & Style::Color_16_fg_set) {
    field_mask |= Style::Color_16_fg_set;
    fg.color_16 = style.fg.color_16;
  }
  if (style.field_mask & Style::Color_256_fg_set) {
    field_mask |= Style::Color_256_fg_set;
    fg.color_256 = style.fg.color_256;
  }
  if (style.field_mask & Style::Color_rgb_fg_set) {
    field_mask |= Style::Color_rgb_fg_set;
    fg.color_rgb[0] = style.fg.color_rgb[0];
    fg.color_rgb[1] = style.fg.color_rgb[1];
    fg.color_rgb[2] = style.fg.color_rgb[2];
  }
  if (style.field_mask & Style::Color_16_bg_set) {
    field_mask |= Style::Color_16_bg_set;
    bg.color_16 = style.bg.color_16;
  }
  if (style.field_mask & Style::Color_256_bg_set) {
    field_mask |= Style::Color_256_bg_set;
    bg.color_256 = style.bg.color_256;
  }
  if (style.field_mask & Style::Color_rgb_bg_set) {
    field_mask |= Style::Color_rgb_bg_set;
    bg.color_rgb[0] = style.bg.color_rgb[0];
    bg.color_rgb[1] = style.bg.color_rgb[1];
    bg.color_rgb[2] = style.bg.color_rgb[2];
  }
  if (style.field_mask & Style::Attributes_set) {
    field_mask |= Style::Attributes_set;
    bold = style.bold;
    underline = style.underline;
  }
}

}  // namespace textui
}  // namespace mysqlshdk
