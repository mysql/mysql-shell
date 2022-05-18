/*
 * Copyright (c) 2021, 2022, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/utils/log_sql.h"
#include <cassert>
#include <string_view>
#include <utility>

#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/libs/utils/logger.h"

namespace shcore {

namespace {
Log_sql::Log_level get_level_by_name(std::string_view name) {
  if (str_caseeq(name, "off")) return Log_sql::Log_level::OFF;
  if (str_caseeq(name, "error")) return Log_sql::Log_level::ERROR;
  if (str_caseeq(name, "on")) return Log_sql::Log_level::ON;
  if (str_caseeq(name, "unfiltered")) return Log_sql::Log_level::UNFILTERED;

  throw std::invalid_argument("invalid level");
}

const char *get_level_range_info() {
  return "The log level value must be any of {off, error, on, unfiltered}.";
}

std::string mask_sql_query(const std::string &sql) {
  return shcore::str_subvars(
      sql, [](const std::string &) { return "****"; }, "/*((*/", "/*))*/");
}

// We need to know when we are in Dba.* context to be (backward) compatible
// with --dba-log-sql option.
// Dba.* context is when we are invoking one of the following objects: Dba.*,
// ClusterSet.*, Cluster.*, ReplicaSet.*
bool is_dba_context(std::string_view context) {
  using namespace std::literals;

  // todo(kg): transform to state machine or use (compile-time) regex
  return (shcore::str_ibeginswith(context, "dba."sv) ||
          shcore::str_ibeginswith(context, "cluster."sv) ||
          shcore::str_ibeginswith(context, "clusterset."sv) ||
          shcore::str_ibeginswith(context, "replicaset."sv));
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

Log_sql::Log_sql(const mysqlsh::Shell_options &opts) {
  const auto &storage = opts.get();
  init(storage);
}

Log_sql::Log_sql(const mysqlsh::Shell_options::Storage &opts) { init(opts); }

Log_sql::~Log_sql() = default;

void Log_sql::init(const mysqlsh::Shell_options::Storage &opts) {
  // only called on ctor: doesn't need protection
  m_dba_log_sql = opts.dba_log_sql;
  m_log_sql_level = get_level_by_name(opts.log_sql);
  m_ignore_patterns = shcore::split_string(opts.log_sql_ignore, ":");

  observe_notification(SN_SHELL_OPTION_CHANGED);
}

void Log_sql::handle_notification(const std::string &name,
                                  const Object_bridge_ref &,
                                  Value::Map_type_ref data) {
  if (name != SN_SHELL_OPTION_CHANGED) return;

  auto option = data->get_string("option");
  if (option == SHCORE_LOG_SQL) {
    m_log_sql_level = get_level_by_name(data->get_string("value"));
  } else if (option == SHCORE_DBA_LOG_SQL) {
    m_dba_log_sql = data->get_int("value");
  } else if (option == SHCORE_LOG_SQL_IGNORE) {
    std::lock_guard lock(m_mutex);
    auto v = data->get_string("value");
    m_ignore_patterns = shcore::split_string(v, ":");
  }
}

void Log_sql::do_log(const std::string &msg) const {
  // this is private and called with a lock on m_mutex
  const auto &e = m_context_stack.top();
  Logger::Log_entry entry(e.c_str(), msg.c_str(), msg.size(), m_log_level);
  Logger::do_log(entry);
}

void Log_sql::log(uint64_t thread_id, const char *sql, size_t len) {
  std::lock_guard lock(m_mutex);

  if (m_context_stack.empty()) return;

  const auto [log_flag, filtered_flag] = will_log(sql, len, false);
  if (!log_flag) return;

  const std::string log_msg = "tid=" + std::to_string(thread_id) +
                              ": SQL: " + mask_sql_query(std::string(sql, len));
  do_log(log_msg);
}

void Log_sql::log(uint64_t thread_id, const char *sql, size_t len,
                  const shcore::Error &error) {
  std::lock_guard lock(m_mutex);

  if (m_context_stack.empty()) return;

  const auto [log_flag, filtered_flag] = will_log(sql, len, true);
  if (!log_flag) return;

  std::string log_msg =
      "tid=" + std::to_string(thread_id) + ": " + error.format();

  if (m_log_sql_level == Log_level::ERROR) {
    // SQL query string isn't logged when Log_level is error. Therefore we need
    // to append SQL query string to logged error message received from MySQL
    // Server, to have information which query caused error.
    log_msg += ", SQL: " + mask_sql_query(std::string(sql, len));
  } else if (filtered_flag) {
    log_msg += ", SQL: <filtered>";
  }
  do_log(log_msg);
}

void Log_sql::log_connect(const std::string &endpoint_uri, uint64_t thread_id) {
  if (is_off()) return;

  std::lock_guard lock(m_mutex);

  if (m_context_stack.empty()) return;

  std::string log_msg{"tid=" + std::to_string(thread_id) +
                      ": CONNECTED: " + endpoint_uri};
  do_log(log_msg);
}

bool Log_sql::is_off() const { return m_log_sql_level == Log_level::OFF; }

std::pair<bool, bool> Log_sql::will_log(const char *sql, size_t len,
                                        bool has_error) {
  // If --dba-log-sql != 0, then it has priority in Dba.* context
  if (m_in_dba_context) {
    if (m_dba_log_sql >= 2) {
      return std::make_pair(true, false);
    }

    if (m_dba_log_sql == 1) {
      if (std::string_view sqlv{sql, len};
          shcore::str_ibeginswith(sqlv, "select") ||
          shcore::str_ibeginswith(sqlv, "show")) {
        return std::make_pair(false, true);
      } else {
        return std::make_pair(true, false);
      }
    }
  }

  switch (m_log_sql_level) {
    case Log_level::OFF:
      return std::make_pair(false, false);
    case Log_level::ERROR:
      return std::make_pair(has_error, false);
    case Log_level::ON: {
      auto filter_flag = is_filtered(std::string_view(sql, len));
      return std::make_pair((has_error || !filter_flag), filter_flag);
    }
    case Log_level::UNFILTERED:
      return std::make_pair(true, false);
  }

  return std::make_pair(false, false);
}

void Log_sql::push(const char *context) {
  assert(context);
  std::lock_guard lock(m_mutex);

  m_context_stack.emplace(std::string{context});

  // Track Dba.* context
  if (is_dba_context(m_context_stack.top())) {
    m_in_dba_context = true;
  }
}

void Log_sql::pop() {
  std::lock_guard lock(m_mutex);

  assert(!m_context_stack.empty());
  if (!m_context_stack.empty()) {
    m_context_stack.pop();
  }

  // Track Dba.* context
  if (m_context_stack.empty() || !is_dba_context(m_context_stack.top())) {
    m_in_dba_context = false;
  }
}

Log_sql::Log_level Log_sql::parse_log_level(const std::string &tag) {
  try {
    return get_level_by_name(tag);
  } catch (...) {
    throw std::invalid_argument(get_level_range_info());
  }
}

bool Log_sql::is_filtered(const std::string_view sql) const {
  for (auto &pattern : m_ignore_patterns) {
    if (shcore::match_glob(pattern, sql)) {
      return true;
    }
  }
  return false;
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
