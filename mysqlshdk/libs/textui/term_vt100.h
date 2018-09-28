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

#ifndef MYSQLSHDK_LIBS_TEXTUI_TERM_VT100_H_
#define MYSQLSHDK_LIBS_TEXTUI_TERM_VT100_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <string>
#if defined(WIN32) && _MSC_VER < 1900
#define snprintf _snprintf  // Microsoft headers use underscores in some names
#endif

namespace mysqlshdk {
namespace vt100 {

bool is_available();

bool get_screen_size(int *rows, int *columns);

void send_escape(const char *s);
#if 0
bool read_escape(const char *terminator, char *buffer, size_t buflen);

// Requests a Report Device Code response from the device.
inline std::string query_device_code() {
  send_escape("\033[c");
  char buffer[32];
  read_escape("0c", buffer, sizeof(buffer));
  return buffer;
}

// Requests a Report Device Status response from the device.
inline bool query_device_status() {
  send_escape("\033[5n");
  char buffer[4];
  read_escape("n", buffer, sizeof(buffer));
  return strcmp(buffer + 1, "0") == 0;
}

// Requests a Report Cursor Position response from the device.
inline void query_cursor_position(int *row, int *column) {
  send_escape("\033[6n");
  char buffer[100];
  read_escape("R", buffer, sizeof(buffer));
  sscanf(buffer, "%i;%i", row, column);
}
#endif

// Reset all terminal settings to default.
inline void reset_device() { send_escape("\033c"); }

// Text wraps to next line if longer than the length of the display area.
inline void enable_line_wrap() { send_escape("\033[7h"); }

// Disables line wrapping.
inline void disable_line_wrap() { send_escape("\033[7l"); }

// Set default font.
inline void font_set_g0() { send_escape("\033("); }

// Set alternate font.
inline void font_set_g1() { send_escape("\033)"); }

// Moves cursor to the upper left corner of the screen.
inline void cursor_home() { send_escape("\033[H"); }

// Sets the cursor position where subsequent text will begin. If no row/column
// parameters are provided (ie. <ESC>[H), the cursor will move to the home
// position, at the upper left of the screen.
inline void cursor_home(int row, int column) {
  char buffer[20];
  snprintf(buffer, sizeof(buffer), "\033[%i;%iH", row, column);
  send_escape(buffer);
}

// Moves the cursor up by COUNT rows; the default count is 1.
inline void cursor_up(int count) {
  char buffer[20];
  snprintf(buffer, sizeof(buffer), "\033[%iA", count);
  send_escape(buffer);
}

// Moves the cursor down by COUNT rows; the default count is 1.
inline void cursor_down(int count) {
  char buffer[20];
  snprintf(buffer, sizeof(buffer), "\033[%iB", count);
  send_escape(buffer);
}

// Moves the cursor forward by COUNT columns; the default count is 1.
inline void cursor_forward(int count) {
  char buffer[20];
  snprintf(buffer, sizeof(buffer), "\033[%iC", count);
  send_escape(buffer);
}

// Moves the cursor backward by COUNT columns; the default count is 1.
inline void cursor_backward(int count) {
  char buffer[20];
  snprintf(buffer, sizeof(buffer), "\033[%iD", count);
  send_escape(buffer);
}

// Identical to Cursor Home.
inline void force_cursor_position(int row, int column) {
  char buffer[20];
  snprintf(buffer, sizeof(buffer), "\033[%i;%if", row, column);
  send_escape(buffer);
}

// Save current cursor position.
inline void save_cursor() { send_escape("\033[s"); }

// Restores cursor position after a Save Cursor.
inline void unsave_cursor() { send_escape("\033[u"); }

// Save current cursor position.
inline void save_cursor_and_attrs() { send_escape("\0337"); }

// Restores cursor position after a Save Cursor.
inline void restore_cursor_and_attrs() { send_escape("\0338"); }

// Enable scrolling for entire display.
inline void scroll_screen() { send_escape("\033[r"); }

// Enable scrolling from row {start} to row {end}.
inline void scroll_screen(int start, int end) {
  char buffer[20];
  snprintf(buffer, sizeof(buffer), "\033[%i;%ir", start, end);
  send_escape(buffer);
}

// Scroll viewport down one line, text moves up.
inline void scroll_down() { send_escape("\033D"); }

// Scroll viewport up one line, text moves down.
inline void scroll_up() { send_escape("\033M"); }

// Sets a tab at the current position.
inline void set_tab() { send_escape("\033H"); }

// Clears tab at the current position.
inline void clear_tab() { send_escape("\033[g"); }

// Clears all tabs.
inline void clear_all_tabs() { send_escape("\033[3g"); }

// Erases from the current cursor position to the end of the current line.
inline void erase_end_of_line() { send_escape("\033[K"); }

// Erases from the current cursor position to the start of the current line.
inline void erase_start_of_line() { send_escape("\033[1K"); }

// Erases the entire current line.
inline void erase_line() { send_escape("\033[2K"); }

// Erases the screen from the current line down to the bottom of the screen.
inline void erase_down() { send_escape("\033[J"); }

// Erases the screen from the current line up to the top of the screen.
inline void erase_up() { send_escape("\033[1J"); }

// Erases the screen with the background colour.
inline void erase_screen() { send_escape("\033[2J"); }

const int ResetAllAttributes = 0;
const int Bright = 1;
const int Dim = 2;
const int Underline = 4;
const int Blink = 8;
const int Reverse = 16;
const int Hidden = 32;

inline std::string attr(int fgcolor = -1, int bgcolor = -1, int attrs = 0) {
  if (attrs == 0 && fgcolor < 0 && bgcolor < 0) {
    return "\033[0m";
  } else {
    std::string buffer;
    buffer = "\033[";
    if (attrs & Bright) buffer.append("1;");
    if (attrs & Dim) buffer.append("2;");
    if (attrs & Underline) buffer.append("4;");
    if (attrs & Blink) buffer.append("5;");
    if (attrs & Reverse) buffer.append("7;");
    if (attrs & Hidden) buffer.append("8;");
    if (fgcolor >= 0) buffer.append(std::to_string(30 + fgcolor)).append(";");
    if (bgcolor >= 0) buffer.append(std::to_string(40 + bgcolor)).append(";");
    if (buffer.back() == ';') buffer.pop_back();
    buffer.append("m");
    return buffer;
  }
}

inline std::string color_fg_256(int fgcolor = 15) {
  if (fgcolor >= 0) {
    char tmp[32];
    snprintf(tmp, sizeof(tmp), "\033[38;5;%im", fgcolor);
    return tmp;
  }
  return "";
}

inline std::string color_bg_256(int bgcolor = 0) {
  if (bgcolor >= 0) {
    char tmp[32];
    snprintf(tmp, sizeof(tmp), "\033[48;5;%im", bgcolor);
    return tmp;
  }
  return "";
}

inline std::string color_fg_rgb(const uint8_t rgb[3]) {
  char tmp[32];
  snprintf(tmp, sizeof(tmp), "\033[38;2;%i;%i;%im", rgb[0], rgb[1], rgb[2]);
  return tmp;
}

inline std::string color_bg_rgb(const uint8_t rgb[3]) {
  char tmp[32];
  snprintf(tmp, sizeof(tmp), "\033[48;2;%i;%i;%im", rgb[0], rgb[1], rgb[2]);
  return tmp;
}

}  // namespace vt100
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_TEXTUI_TERM_VT100_H_
