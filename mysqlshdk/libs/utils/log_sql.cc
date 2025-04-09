/*
 * Copyright (c) 2021, 2025, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "mysqlshdk/libs/utils/log_sql.h"

#include <cassert>
#include <string_view>
#include <utility>

#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/libs/oci/oci_par.h"
#include "mysqlshdk/libs/utils/logger.h"

namespace shcore {

namespace {
Log_sql::Log_level get_level_by_name(std::string_view name) {
  if (str_caseeq(name, "off")) return Log_sql::Log_level::OFF;
  if (str_caseeq(name, "error")) return Log_sql::Log_level::ERROR;
  if (str_caseeq(name, "on")) return Log_sql::Log_level::ON;
  if (str_caseeq(name, "all")) return Log_sql::Log_level::ALL;
  if (str_caseeq(name, "unfiltered")) return Log_sql::Log_level::UNFILTERED;

  throw std::invalid_argument("invalid level");
}

std::string mask_sql_query(std::string_view sql, bool *out_masked = nullptr) {
  return mysqlshdk::oci::mask_any_par(shcore::str_subvars(
      sql,
      [out_masked](std::string_view) {
        if (out_masked) {
          *out_masked = true;
        }

        return "****";
      },
      "/*((*/", "/*))*/"));
}
}  // namespace

Log_sql::Log_sql() {
  const auto options = mysqlsh::current_shell_options(true);
  if (options) {
    init(options->get());
  } else {
    auto default_options = mysqlsh::Shell_options();
    init(default_options.get());
  }
}

Log_sql::Log_sql(const mysqlsh::Shell_options &opts) { init(opts.get()); }

Log_sql::Log_sql(const mysqlsh::Shell_options::Storage &opts) { init(opts); }

Log_sql::~Log_sql() = default;

void Log_sql::init(const mysqlsh::Shell_options::Storage &opts) {
  // only called on ctor: doesn't need protection
  m_log_sql_level = get_level_by_name(opts.log_sql);
  m_ignore_patterns = shcore::split_string(opts.log_sql_ignore, ":");
  m_ignore_patterns_all = shcore::split_string(opts.log_sql_ignore_unsafe, ":");

  observe_notification(SN_SHELL_OPTION_CHANGED);
}

void Log_sql::handle_notification(const std::string &name,
                                  const Object_bridge_ref &,
                                  Value::Map_type_ref data) {
  if (name != SN_SHELL_OPTION_CHANGED) return;

  auto option = data->get_string("option");
  if (option == SHCORE_LOG_SQL) {
    m_log_sql_level = get_level_by_name(data->get_string("value"));
  } else if (option == SHCORE_LOG_SQL_IGNORE) {
    std::lock_guard lock(m_mutex);
    auto v = data->get_string("value");
    m_ignore_patterns = shcore::split_string(v, ":");
  } else if (option == SHCORE_LOG_SQL_IGNORE_UNSAFE) {
    std::lock_guard lock(m_mutex);
    auto v = data->get_string("value");
    m_ignore_patterns_all = shcore::split_string(v, ":");
  }
}

void Log_sql::do_log(std::string_view msg) const {
  // this is private and called with a lock on m_mutex
  Logger::Log_entry entry(m_context_stack.top(), msg, m_log_level);
  Logger::do_log(entry);
}

void Log_sql::log(uint64_t thread_id, std::string_view sql) {
  std::lock_guard lock(m_mutex);

  if (m_context_stack.empty()) return;

  const auto [log_flag, filtered_flag] = will_log(sql, false);
  if (!log_flag) return;

  std::string log_msg;
  log_msg.reserve(16 + sql.size());
  log_msg.append("tid=");
  log_msg.append(std::to_string(thread_id));
  log_msg.append(": SQL: ");
  log_msg.append(mask_sql_query(sql));

  do_log(log_msg);
}

void Log_sql::log(uint64_t thread_id, std::string_view sql,
                  const shcore::Error &error) {
  std::lock_guard lock(m_mutex);

  if (m_context_stack.empty()) return;

  auto [log_flag, filtered_flag] = will_log(sql, true);
  if (!log_flag) return;

  std::string log_msg;
  log_msg.reserve(256);
  log_msg.append("tid=");
  log_msg.append(std::to_string(thread_id));
  log_msg.append(": ");
  log_msg.append(mysqlshdk::oci::mask_any_par(error.format()));

  if (m_log_sql_level == Log_level::ERROR) {
    // SQL query string isn't logged when Log_level is error. Therefore we need
    // to append SQL query string to logged error message received from MySQL
    // Server, to have information which query caused error.
    bool masked = false;
    auto masked_sql = mask_sql_query(sql, &masked);

    if (masked || filtered_flag.empty()) {
      log_msg.append(", SQL: ").append(masked_sql);

      // If the statement is masked, clear filter flag if set
      filtered_flag.clear();
    }
  }

  if (!filtered_flag.empty()) {
    log_msg.append(", SQL: <filtered: ").append(filtered_flag).append(">");
  }

  do_log(log_msg);
}

void Log_sql::log_connect(std::string_view endpoint_uri, uint64_t thread_id) {
  if (is_off()) return;

  std::lock_guard lock(m_mutex);

  if (m_context_stack.empty()) return;

  std::string log_msg;
  log_msg.reserve(32 + endpoint_uri.size());
  log_msg.append("tid=");
  log_msg.append(std::to_string(thread_id));
  log_msg.append(": CONNECTED: ");
  log_msg.append(endpoint_uri);

  do_log(log_msg);
}

bool Log_sql::is_off() const { return m_log_sql_level == Log_level::OFF; }

std::pair<bool, std::string> Log_sql::will_log(std::string_view sql,
                                               bool has_error) {
  switch (m_log_sql_level) {
    case Log_level::OFF:
      return std::make_pair(false, "");
    case Log_level::ERROR: {
      return std::make_pair(has_error, is_filtered_unsafe(sql));
    }
    case Log_level::ON: {
      auto filter_flag = is_filtered(sql);
      if (filter_flag.empty()) {
        filter_flag = is_filtered_unsafe(sql);
      }
      return std::make_pair((has_error || filter_flag.empty()), filter_flag);
    }
    case Log_level::ALL: {
      auto filter_flag = is_filtered_unsafe(sql);
      return std::make_pair((has_error || filter_flag.empty()), filter_flag);
    }
    case Log_level::UNFILTERED:
      return std::make_pair(true, "");
  }

  return std::make_pair(false, "");
}

void Log_sql::push(std::string_view context) {
  std::lock_guard lock(m_mutex);

  m_context_stack.emplace(std::string{context});
}

void Log_sql::pop() {
  std::lock_guard lock(m_mutex);

  assert(!m_context_stack.empty());
  if (m_context_stack.empty()) return;

  m_context_stack.pop();
}

Log_sql::Log_level Log_sql::parse_log_level(std::string_view tag) {
  try {
    return get_level_by_name(tag);
  } catch (...) {
    throw std::invalid_argument(
        "The log level value must be any of {off, error, on, all, "
        "unfiltered}.");
  }
}

std::string Log_sql::is_filtered(std::string_view sql) const {
  for (auto &pattern : m_ignore_patterns) {
    if (shcore::match_glob(pattern, sql)) {
      return pattern;
    }
  }
  return "";
}

std::string Log_sql::is_filtered_unsafe(std::string_view sql) const {
  for (auto &pattern : m_ignore_patterns_all) {
    if (shcore::match_glob(pattern, sql)) {
      return pattern;
    }
  }
  return "";
}

Log_sql_guard::Log_sql_guard(const char *context) {
  m_obj = current_log_sql(true);
  if (m_obj) {
    m_obj->push(context);
  }
}

Log_sql_guard::Log_sql_guard() : Log_sql_guard("") {}

Log_sql_guard::~Log_sql_guard() {
  if (m_obj) {
    m_obj->pop();
  }
}
}  // namespace shcore
