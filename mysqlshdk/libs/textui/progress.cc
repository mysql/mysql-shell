/*
 * Copyright (c) 2018, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/libs/textui/progress.h"
#include "mysqlshdk/libs/textui/term_vt100.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace textui {

const int k_max_width = 60;

Progress_vt100::Progress_vt100(int num_secondary_bars) {
  m_secondary_bars.resize(num_secondary_bars);
  m_screen_width = 0;
  m_top_row = 0;
}

void Progress_vt100::set_style(const Style &text_style, const Style &fg_style,
                               const Style &bg_style, const std::string &fg,
                               const std::string &bg) {
  m_text_style = text_style;
  m_fg_style = fg_style;
  m_bg_style = bg_style;
  m_fg = fg;
  m_bg = bg;
}

void Progress_vt100::set_secondary_style(const Style &text_style,
                                         const Style &fg_style,
                                         const Style &bg_style) {
  m_text2_style = text_style;
  m_fg2_style = fg_style;
  m_bg2_style = bg_style;
}

void Progress_vt100::set_label(const std::string &label) {
  m_bar.label = label;
}

void Progress_vt100::set_right_label(const std::string &label) {
  m_bar.rlabel = label;
}

void Progress_vt100::set_total(int64_t total) { m_bar.total = total; }

void Progress_vt100::set_current(int64_t current) { m_bar.current = current; }

void Progress_vt100::set_secondary_label(int i, const std::string &label) {
  m_secondary_bars[i].label = label;
}

void Progress_vt100::set_secondary_right_label(int i,
                                               const std::string &label) {
  m_secondary_bars[i].rlabel = label;
}

void Progress_vt100::set_secondary_total(int i, int64_t total) {
  m_secondary_bars[i].total = total;
}

void Progress_vt100::set_secondary_current(int i, int64_t current) {
  m_secondary_bars[i].current = current;
}

bool Progress_vt100::start() {
  vt100::set_echo(false);

  int rows;
  // get the width of the screen and the cursor position
  if (!vt100::get_screen_size(&rows, &m_screen_width)) return false;

  for (size_t i = 0; i < 1 + m_secondary_bars.size(); i++) printf("\n");
  int column;
  vt100::query_cursor_position(&m_top_row, &column);
  m_top_row -= 1 + m_secondary_bars.size();

  update();

  return true;
}

void Progress_vt100::end() {
  vt100::set_echo(true);
  vt100::cursor_home(m_top_row + 1 + m_secondary_bars.size(), 0);
}

void Progress_vt100::update() {
  vt100::cursor_home(m_top_row, 0);
  draw_bar(m_bar, 0, m_text_style, m_bg_style, m_fg_style);

  int i = 1;
  for (const auto &bar : m_secondary_bars) {
    vt100::cursor_home(m_top_row + i++, 0);
    draw_bar(bar, 4, m_text2_style ? m_text2_style : m_text_style,
             m_bg2_style ? m_bg2_style : m_bg_style,
             m_fg2_style ? m_fg2_style : m_fg_style);
  }
}

void styled(const Style &style, const std::string &text) {
  if (style)
    printf("%s%s%s", style.str().c_str(), text.c_str(),
           Style::clear().str().c_str());
  else
    printf("%s", text.c_str());
}

void Progress_vt100::draw_bar(const Item &item, int indent,
                              const Style &text_style, const Style &bg_style,
                              const Style &fg_style) {
  styled(text_style, std::string(indent, ' ') + item.label + "  ");

  if (item.total >= 0) {
    std::string rtext = shcore::str_format(
        "%3i%% ", item.total == 0
                      ? 0
                      : static_cast<int>(item.current * 100 / item.total));
    if (!item.rlabel.empty()) rtext += " " + item.rlabel;

    int width =
        m_screen_width - (item.label.length() + 2) - (rtext.length() + 2);
    if (width > k_max_width) width = k_max_width;

    std::string bar;
    int progress = item.total == 0 ? 0 : item.current * width / item.total;
    for (int i = 0; i < progress; i++) {
      bar += m_fg;
    }
    styled(fg_style, bar);
    bar.clear();
    for (int i = 0; i < width - progress; i++) {
      bar += m_bg;
    }
    styled(bg_style, bar);

    styled(text_style, "  " + rtext);
  }
  fflush(stdout);

  vt100::erase_end_of_line();
}

constexpr const char k_spinny_sticks[] = "\\|/-";

void Spinny_stick::update() {
  printf("%s %c\r", m_label.c_str(), k_spinny_sticks[m_step]);
  fflush(stdout);
  m_step++;
  if (m_step >= int(sizeof(k_spinny_sticks) - 1)) m_step = 0;
}

void Spinny_stick::done(const std::string &text) {
  printf("%s %s\n", m_label.c_str(), text.c_str());
}

}  // namespace textui
}  // namespace mysqlshdk
