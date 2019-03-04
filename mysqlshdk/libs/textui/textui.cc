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

#include "mysqlshdk/libs/textui/textui.h"

#include <vector>

#include "mysqlshdk/libs/textui/term_vt100.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace textui {
namespace internal {
Highlight find_highlight(const std::string &where) {
  static const std::map<std::string, std::string> tags = {{"<b>", "</b>"},
                                                          {"<w>", "</w>"}};
  static const std::map<std::string, Text_style> types = {
      {"<b>", Text_style::Bold}, {"<w>", Text_style::Warning}};

  size_t new_start = std::string::npos;
  size_t new_end = std::string::npos;
  Text_style new_style = Text_style::Bold;

  for (const auto &item : tags) {
    size_t opening = where.find(item.first);
    if (opening != std::string::npos &&
        (new_start == std::string::npos || opening < new_start)) {
      // We need to ensure the closing tag is found as well, otherwise
      // an orphan opening tag mayinvalidate other valid tags
      size_t closing = where.find(item.second, opening);
      if (closing != std::string::npos) {
        new_start = opening;
        new_end = closing;
        new_style = types.at(item.first);
      }
    }
  }

  return std::make_tuple(new_start, new_end, new_style);
}

/**
 * Removes all the formatting tags by either applying final format to be used in
 * the console version of the hepl or extracting data to allow posprocessing
 * once the text is properly splitted in lines to be printed on the console.
 *
 * The preprocessing consist on:
 * - Eliminating tags that should not be on the console
 * - Replacing tags with console version of the intended format
 * - Identifying tags that must be processed after the whole formatting is done
 *
 * @param line The string to be formatted for console output.
 * @param color_data An array to hold the position/length pairs of text that
 * will be colored.
 * @returns A string with NO formatting tags.
 */
std::string preprocess_markup(const std::string &line, Highlights *highlights) {
  std::string ret_val = line;

  // @code tags are only used in Doxygen, in the console they are removed.
  size_t start = ret_val.find("@code");
  size_t end = ret_val.find("@endcode", start);
  while (start != std::string::npos && end != std::string::npos) {
    std::string tmp_line = ret_val.substr(0, start);
    size_t skip = 5;  // The length of @code

    tmp_line += ret_val.substr(start + skip, end - start - 5);
    tmp_line += ret_val.substr(end + 8);

    ret_val = tmp_line;
    start = ret_val.find("@code");
    end = ret_val.find("@endcode", start);
  }

  // Some characters need to be specified using special doxygen format to
  // to prevent generating doxygen warnings.
  std::vector<std::pair<const char *, const char *>> replacements = {
      {"@<", "<"}, {"@>", ">"},    {"&nbsp;", " "}, {"@li", "-"},
      {"@%", "%"}, {"\\\\", "\\"}, {"<br>", "\n"}};

  for (const auto &rpl : replacements) {
    ret_val = shcore::str_replace(ret_val, rpl.first, rpl.second);
  }

  Highlight next = find_highlight(ret_val);
  while (std::get<0>(next) != std::string::npos) {
    size_t start = std::get<0>(next);
    size_t end = std::get<1>(next);
    Text_style type = std::get<2>(next);

    std::string tmp_line = ret_val.substr(0, start);

    if (highlights)
      highlights->push_back(std::make_tuple(start, end - start - 3, type));

    tmp_line += ret_val.substr(start + 3, end - start - 3);
    tmp_line += ret_val.substr(end + 4);

    ret_val = tmp_line;
    next = find_highlight(ret_val);
  }

  return ret_val;
}

/**
 * Applies proper formatting to the lines based on the received highlights.
 */
void postprocess_markup(std::vector<std::string> *lines,
                        const Highlights &highlights) {
  std::vector<Highlights> line_highlights;

  // Copy of the highlights to be able to 'consume' them.
  Highlights process_highlights(highlights);

  size_t offset = 0;
  size_t highlight_index = 0;

  if (highlights.empty()) return;

  for (size_t index = 0; index < lines->size(); index++) {
    // Array of highlights for this line
    if (line_highlights.size() < (index + 1)) line_highlights.push_back({});

    size_t end_offset = offset + lines->at(index).length();

    while (highlight_index < process_highlights.size()) {
      size_t start = std::get<0>(process_highlights[highlight_index]);
      size_t length = std::get<1>(process_highlights[highlight_index]);
      Text_style type = std::get<2>(process_highlights[highlight_index]);

      if (start <= end_offset) {
        if ((start + length) <= end_offset) {
          // Full highlight fits in this line
          line_highlights[index].push_back(
              std::make_tuple(start - offset, length, type));
          highlight_index++;
        } else {
          // Part of the highlight is in this line, part is on the next
          size_t part_length = end_offset - start;

          // Consumes from the original highlight the part_length that will be
          // used in this line
          process_highlights[highlight_index] =
              std::make_tuple(end_offset, length - part_length, type);

          // Adjusts line highlight to exclude trailing whitespaces
          while (part_length &&
                 lines->at(index).at(start - offset + part_length - 1) == ' ')
            part_length--;

          // If the highlight is still valid adds it, otherwise ignores it
          // An invalid highlight would be one enclosing just an empty string
          // like <b>   </b>
          if (part_length)
            line_highlights[index].push_back(
                std::make_tuple(start - offset, part_length, type));

          // Remaining of the highlight is beyond this line, we are done
          break;
        }
      } else {
        // The highlight is beyond this line, we are done
        break;
      }
    }

    if (highlight_index >= process_highlights.size()) break;

    // Offset includes this line width
    offset += lines->at(index).length();
  }

  for (size_t index = 0; index < lines->size(); index++) {
    std::string line = lines->at(index);

    if (line_highlights.size() > index) {
      auto highlight = line_highlights[index].rbegin();
      while (highlight != line_highlights[index].rend()) {
        size_t start = std::get<0>(*highlight);
        size_t length = std::get<1>(*highlight);
        Text_style type = std::get<2>(*highlight);

        std::string fline = line.substr(0, start);
        switch (type) {
          case Text_style::Bold:
            fline += bold(line.substr(start, length));
            break;
          case Text_style::Warning:
            fline += warning(line.substr(start, length));
            break;
        }

        fline += line.substr(start + length);
        line = fline;
        highlight++;
      }
      (*lines)[index] = line;
    }
  }
}

/**
 * Splits a string into substrings of complete words of maximum @size length.
 *
 * @param input The string to be split.
 * @param size The character limit the strings will have.
 * @returns The list of strings having the required lengths,
 *
 * NOTE: An important aspect of this function is to NOT lose any character
 * from the input string, it is intended for use even when parsing formatted
 * markup data, losing characters would break any formatting highlights
 * extracted on string pre-processing.
 *
 * Trailing spaces ignored on @size: once a string is found, any subsequent
 * space is added to the end of the string no matter it exceeds the @size;
 * this is done to ensure all the returned lines start in non space
 * characters.
 *
 * It is expected that the caller trims trailing characters if needed, in such
 * case, if post_processing is done it must be done before trimming such
 * characters.
 */
std::vector<std::string> get_sized_strings(const std::string &input,
                                           size_t size) {
  std::vector<std::string> chunks;
  size_t consumed = 0;
  const size_t total_size = input.size();

  while (consumed < total_size) {
    size_t end = consumed + size;

    if ((total_size - consumed) <= size) {
      chunks.push_back(input.substr(consumed));
      consumed = total_size;
    } else {
      if (input[end] == ' ') {
        // Includes any subsequent space on the string
        while (end < total_size && input[end + 1] == ' ') end++;
      } else {
        // Removes any non space character until the last space is found
        while (end > consumed && input[end] != ' ') end--;
      }

      // If end was consumed a substring with the required length was found
      if (end > consumed) {
        // At this point end represents the position of the last character
        // So be included on the substring, in the following line end
        // represents string size so we add 1
        end++;
      } else {
        // If unable to find a string with the required length we need to
        // advance so the only available string is selected no matter it
        // exceeds the required size
        size_t first_space = input.find_first_of(' ', consumed);
        end = input.find_first_not_of(' ', first_space);
      }

      chunks.push_back(input.substr(consumed, end - consumed));
      consumed = end;
    }
  }

  return chunks;
}

}  // namespace internal

Color_capability g_color_capability = Color_256;

void set_color_capability(Color_capability cap) { g_color_capability = cap; }

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

bool SHCORE_PUBLIC supports_screen_control() {
  return mysqlshdk::vt100::is_available();
}

void scroll_screen() {
  int w = 0;
  int h = 0;

  if (mysqlshdk::vt100::get_screen_size(&h, &w)) {
    // scroll the screen, making the current line the first invisible one
    for (int i = 0; i < h; ++i) {
      // mysqlshdk::vt100::scroll_down() is not supported on Windows,
      // simply print some lines
      mysqlshdk::vt100::send_escape("\n");
    }
  } else {
    // effectively scrolls the screen, the whole area will be empty
    // may leave some empty areas
    mysqlshdk::vt100::erase_screen();
  }
  // move cursor to the upper left corner
  mysqlshdk::vt100::cursor_home();
}

void clear_screen() {
  // erase from the current cursor position to the beginning of screen
  mysqlshdk::vt100::erase_up();
  // move cursor to the upper left corner
  mysqlshdk::vt100::cursor_home();
}

int parse_color_set(const std::string &color_spec, uint8_t *color_16,
                    uint8_t *color_256, uint8_t color_rgb[3],
                    bool convert_missing) {
  static const char *color_names[] = {"black", "red",     "green", "yellow",
                                      "blue",  "magenta", "cyan",  "white"};
  int mask = 0;
  std::vector<std::string> values(shcore::split_string(color_spec, ";"));
  for (auto &color : values) {
    if (color.empty()) continue;
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
  if ((field_mask & mask) != (style.field_mask & mask)) return false;

  if ((field_mask & mask) & Style::Color_16_fg_set) {
    if (fg.color_16 != style.fg.color_16) return false;
  }
  if ((field_mask & mask) & Style::Color_256_fg_set) {
    if (fg.color_256 != style.fg.color_256) return false;
  }
  if ((field_mask & mask) & Style::Color_rgb_fg_set) {
    if (fg.color_rgb[0] != style.fg.color_rgb[0] ||
        fg.color_rgb[1] != style.fg.color_rgb[1] ||
        fg.color_rgb[2] != style.fg.color_rgb[2])
      return false;
  }
  if ((field_mask & mask) & Style::Color_16_bg_set) {
    if (bg.color_16 != style.bg.color_16) return false;
  }
  if ((field_mask & mask) & Style::Color_256_bg_set) {
    if (bg.color_256 != style.bg.color_256) return false;
  }
  if ((field_mask & mask) & Style::Color_rgb_bg_set) {
    if (bg.color_rgb[0] != style.bg.color_rgb[0] ||
        bg.color_rgb[1] != style.bg.color_rgb[1] ||
        bg.color_rgb[2] != style.bg.color_rgb[2])
      return false;
  }
  if ((field_mask & mask) & Style::Attributes_set) {
    if (bold != style.bold || underline != style.underline) return false;
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

std::string format_markup_text(const std::string &line, size_t width,
                               size_t left_padding) {
  std::string ret_val;

  // Protect against invalid padding value
  assert(left_padding < width);

  // Aproximate number of resulting lines
  size_t lines = (line.size() / (width - left_padding));

  // Aproximate final string size
  ret_val.reserve(lines * width);

  // Processes markup and gets final output line (pure text)
  internal::Highlights highlights;
  std::string prep_line = internal::preprocess_markup(line, &highlights);

  std::string padding(left_padding, ' ');

  // Splits the line for post processing
  size_t size = width - (left_padding + 1);
  std::vector<std::string> sublines =
      internal::get_sized_strings(prep_line, size);

  // Now applies the markup
  internal::postprocess_markup(&sublines, highlights);

  for (auto &subline : sublines)
    subline = padding + shcore::str_rstrip(subline, " \t");

  return shcore::str_join(sublines, "\n");
}

std::string format_markup_text(const std::vector<std::string> &lines,
                               size_t width, size_t left_padding,
                               bool paragraph_per_line) {
  std::string ret_val;

  if (!lines.empty()) {
    // Determine if we start with a list of items or not
    bool in_list = (0 == lines[0].find("@li"));

    std::string new_line;
    for (auto line : lines) {
      if (!ret_val.empty()) ret_val += "\n";

      if (shcore::str_beginswith(line.c_str(), "@code") &&
          shcore::str_endswith(line.c_str(), "@endcode")) {
        size_t code_len = 5;    // The length of @code
        size_t encode_len = 8;  // The length of @endcode
        size_t skip = code_len;
        // Handles Code Items
        if (line[code_len] == '{' && line[code_len + 1] == '.') {
          size_t new_start = line.find("}");
          if (new_start != std::string::npos) {
            skip = new_start + 1;
          }
        }

        std::string code = line.substr(skip, line.size() - (skip + encode_len));
        code = shcore::str_strip(code, "\n");
        std::vector<std::string> code_lines = shcore::str_split(code, "\n");
        ret_val += format_markup_text(code_lines, width, left_padding, false);
      } else if (0 == line.find("@li ")) {
        // handles list items:
        // - Padding increases in 2 to let the item bullet alone.
        // - Blank line is inserted before the first item.
        std::string formatted =
            format_markup_text(line.substr(4), width, left_padding + 2);
        formatted[left_padding] = '-';

        if (!in_list) {
          ret_val += "\n";
          in_list = true;
        }

        ret_val += formatted;
      } else {
        if ((!ret_val.empty() && paragraph_per_line) || in_list) {
          ret_val += "\n";
          in_list = false;
        }

        if (0 == line.find("@warning ")) {
          std::string warning =
              format_markup_text(line.substr(9), width, left_padding + 9);

          warning.replace(left_padding, 8, textui::warning("WARNING:"));
          ret_val += warning;
        } else if (0 == line.find("@note ")) {
          std::string warning =
              format_markup_text(line.substr(6), width, left_padding + 6);

          warning.replace(left_padding, 5, textui::notice("NOTE:"));
          ret_val += warning;
        } else if (0 == line.find("@attention ")) {
          std::string warning =
              format_markup_text(line.substr(11), width, left_padding + 11);

          warning.replace(left_padding, 10, textui::warning("ATTENTION:"));
          ret_val += warning;
        } else {
          ret_val += format_markup_text(line, width, left_padding);
        }
      }
    }
  }

  return ret_val;
}

}  // namespace textui
}  // namespace mysqlshdk
