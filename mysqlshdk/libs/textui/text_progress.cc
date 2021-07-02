/*
 * Copyright (c) 2018, 2021, Oracle and/or its affiliates.
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
  const auto &current_point =
      m_history[(m_index + k_moving_average_points - 1) %
                k_moving_average_points];
  const auto &past_point = m_history[(m_index + 1) % k_moving_average_points];

  const auto bytes_diff = current_point.bytes - past_point.bytes;
  const auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(
                             current_point.time - past_point.time)
                             .count();
  if (time_diff == 0) {
    return bytes_diff * 1000;
  }
  return bytes_diff * 1000 / time_diff;  // bytes per second
}

void Throughput::reset() {
  m_index = 0;
  m_index_prev = 0;

  for (unsigned int i = 0; i < k_moving_average_points; ++i) {
    m_history[i].bytes = 0;
    m_history[i].time = {};
  }
}

void Base_progress::reset(const char *items_full, const char *items_abbrev,
                          const char *item_singular, const char *item_plural,
                          bool space_before_item, bool total_is_approx) {
  m_initial = 0;
  m_current = 0;
  m_total = 0;
  m_changed = true;
  m_throughput.reset();
  m_status = std::string(80, ' ');
  m_refresh_clock = {};
  m_items_full = items_full;
  m_items_abbrev = items_abbrev;
  m_item_singular = item_singular;
  m_item_plural = item_plural;
  m_left_label.clear();
  m_right_label.clear();
  m_space_before_item = space_before_item;
  m_total_is_approx = total_is_approx;
}

std::string Base_progress::progress() const {
  return (0 == m_total ? "?" : std::to_string(percent())) + "%";
}

std::string Base_progress::current() const { return format_value(m_current); }

std::string Base_progress::total() const {
  return 0 == m_total ? "?"
                      : (m_total_is_approx ? "~" : "") + format_value(m_total);
}

std::string Base_progress::throughput() const {
  return mysqlshdk::utils::format_throughput_items(
      m_item_singular, m_item_plural, m_throughput.rate(), 1.0,
      m_space_before_item);
}

std::string Base_progress::format_value(uint64_t value) const {
  return mysqlshdk::utils::format_items(m_items_full, m_items_abbrev, value,
                                        m_space_before_item);
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

void Text_progress::reset(const char *items_full, const char *items_abbrev,
                          const char *item_singular, const char *item_plural,
                          bool space_before_item, bool total_is_approx) {
  Base_progress::reset(items_full, items_abbrev, item_singular, item_plural,
                       space_before_item, total_is_approx);

  m_last_status_size = 0;
  m_prev_status.clear();
  m_hide = 0;
  was_shown = false;
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
  m_status += "\r";
  m_status += m_left_label;
  m_status +=
      progress() + " (" + current() + " / " + total() + "), " + throughput();
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
  m_status +=
      progress() + " (" + current() + " / " + total() + "), " + throughput();
  m_status += m_right_label;
}

}  // namespace textui
}  // namespace mysqlshdk
