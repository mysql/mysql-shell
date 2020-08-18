/*
 * Copyright (c) 2018, 2020, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/textui/text_progress.h"

#ifdef _WIN32
#include <io.h>
#define fileno _fileno
#define write _write
#else
#include <unistd.h>
#endif

#include <chrono>

#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/textui/term_vt100.h"
#include "mysqlshdk/libs/utils/strformat.h"

namespace mysqlshdk {
namespace textui {

Throughput::Throughput() {
  auto current_time = std::chrono::steady_clock::now();
  for (auto &s : m_history) {
    s.bytes = 0;
    s.time = current_time;
  }
}

uint64_t Throughput::rate() const {
  const auto current_point = m_history[(m_index + k_moving_average_points - 1) %
                                       k_moving_average_points];
  const auto past_point = m_history[(m_index + 1) % k_moving_average_points];

  const auto bytes_diff = current_point.bytes - past_point.bytes;
  const auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(
                             current_point.time - past_point.time)
                             .count();
  if (time_diff == 0) {
    return bytes_diff * 1000;
  }
  return bytes_diff * 1000 / time_diff;  // bytes per second
}

void Text_progress::set_left_label(const std::string &label) {
  m_left_label = label;
}

void Text_progress::set_right_label(const std::string &label) {
  m_right_label = label;
}

void Text_progress::show_status(bool force) {
  if (m_hide != 0) return;

  const auto current_time = std::chrono::steady_clock::now();
  const auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(
                             current_time - m_refresh_clock)
                             .count();
  const bool refresh_timeout = time_diff >= 200;  // update status every 200ms
  if ((refresh_timeout && m_changed) || force) {
    m_prev_status = m_status;

    render_status();
    m_changed = false;
    const auto status_printable_size =
        m_status.size() - 1;  // -1 because of leading '\r'
    if (status_printable_size < m_last_status_size) {
      clear_status();
    }
    {
      if (force || m_prev_status != m_status) {
        const auto ignore =
            write(fileno(stderr), &m_status[0], m_status.size());
        (void)ignore;
      }
      was_shown = true;
    }
    m_refresh_clock = current_time;
    m_last_status_size = status_printable_size;
  }
}

void Text_progress::hide(bool flag) {
  if (flag) {
    if (m_hide == 0 && was_shown) clear_status();
    m_hide++;
  } else {
    m_hide--;
    if (m_hide == 0 && was_shown) show_status(false);
  }
  assert(m_hide >= 0);
}

void Text_progress::clear_status() {
  std::string space(m_last_status_size + 2, ' ');
  space[0] = '\r';
  space[m_last_status_size + 1] = '\r';
  {
    const auto ignore = write(fileno(stderr), &space[0], space.size());
    (void)ignore;
  }
}

void Text_progress::shutdown() {
  const auto ignore = write(fileno(stderr), "\n", 1);
  (void)ignore;
}

void Text_progress::render_status() {
  // 100% (1024.00 MB / 1024.00 MB), 1024.00 MB/s
  m_status.clear();
  if (m_total == 0)
    m_status += "\r" + m_left_label + "?% (" +
                mysqlshdk::utils::format_items(m_items_full, m_items_abbrev,
                                               m_current, m_space_before_item) +
                " / ?), " +
                mysqlshdk::utils::format_throughput_items(
                    m_item_singular, m_item_plural, m_throughput.rate(), 1.0,
                    m_space_before_item);
  else
    m_status += "\r" + m_left_label + std::to_string(percent()) + "% (" +
                mysqlshdk::utils::format_items(m_items_full, m_items_abbrev,
                                               m_current, m_space_before_item) +
                (m_total_is_approx ? " / ~" : " / ") +
                mysqlshdk::utils::format_items(m_items_full, m_items_abbrev,
                                               m_total, m_space_before_item) +
                "), " +
                mysqlshdk::utils::format_throughput_items(
                    m_item_singular, m_item_plural, m_throughput.rate(), 1.0,
                    m_space_before_item);

  m_status += m_right_label;
}

void Json_progress::show_status(bool force) {
  const auto current_time = std::chrono::steady_clock::now();
  const auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(
                             current_time - m_refresh_clock)
                             .count();
  const bool refresh_timeout = time_diff >= 1000;  // update status every 1s
  if ((refresh_timeout && m_changed) || force) {
    render_status();
    m_changed = false;
    mysqlsh::current_console()->raw_print(m_status,
                                          mysqlsh::Output_stream::STDOUT, true);
    m_refresh_clock = current_time;
  }
}

void Json_progress::render_status() {
  // 100% (1024.00 MB / 1024.00 MB), 1024.00 MB/s
  // todo(kg): build proper json doc
  m_status.clear();
  m_status += m_left_label;
  m_status += std::to_string(percent()) + "% (" +
              mysqlshdk::utils::format_items(m_items_full, m_items_abbrev,
                                             m_current, m_space_before_item) +
              " / " +
              mysqlshdk::utils::format_items(m_items_full, m_items_abbrev,
                                             m_total, m_space_before_item) +
              "), " +
              mysqlshdk::utils::format_throughput_items(
                  m_item_singular, m_item_plural, m_throughput.rate(), 1.0,
                  m_space_before_item);
  m_status += m_right_label;
}

}  // namespace textui
}  // namespace mysqlshdk
