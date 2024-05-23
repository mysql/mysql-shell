/*
 * Copyright (c) 2021, 2024, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_UTILS_LOGSQL_H_
#define MYSQLSHDK_LIBS_UTILS_LOGSQL_H_

#include <atomic>
#include <mutex>
#include <stack>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "mysqlshdk/include/shellcore/shell_notifications.h"
#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/libs/utils/logger.h"

namespace shcore {

class Log_sql : public NotificationObserver {
 public:
  enum class Log_level {
    OFF,
    ERROR,
    ON,
    ALL,
    UNFILTERED,
    MAX_VALUE = UNFILTERED
  };

  Log_sql();
  explicit Log_sql(const mysqlsh::Shell_options &opts);
  explicit Log_sql(const mysqlsh::Shell_options::Storage &opts);

  Log_sql(const Log_sql &) = delete;
  Log_sql(Log_sql &&) = delete;

  Log_sql &operator=(const Log_sql &) = delete;
  Log_sql &operator=(Log_sql &&) = delete;

  ~Log_sql() override;

  void handle_notification(const std::string &name,
                           const Object_bridge_ref &sender,
                           Value::Map_type_ref data) override;

  void push(std::string_view context);
  void pop();

  void log(uint64_t thread_id, std::string_view sql);
  void log(uint64_t thread_id, std::string_view sql,
           const shcore::Error &error);
  void log_connect(std::string_view endpoint_uri, uint64_t thread_id);

  bool is_off() const;

  static Log_level parse_log_level(std::string_view tag);

 private:
  void init(const mysqlsh::Shell_options::Storage &opts);
  std::pair<bool, std::string> will_log(std::string_view, bool has_error);
  void do_log(std::string_view msg) const;
  std::string is_filtered(const std::string_view sql) const;
  std::string is_filtered_unsafe(const std::string_view sql) const;

  mutable std::mutex m_mutex;

  std::atomic<int> m_dba_log_sql{0};
  std::atomic<Log_level> m_log_sql_level{Log_level::OFF};
  std::vector<std::string> m_ignore_patterns;
  std::vector<std::string> m_ignore_patterns_all;
  Logger::LOG_LEVEL m_log_level = Logger::LOG_LEVEL::LOG_INFO;
  std::stack<std::string> m_context_stack;
  size_t m_num_dba_ctx{0};
};

// implemented in scoped_contexts.cc
std::shared_ptr<shcore::Log_sql> current_log_sql(bool allow_empty = false);

class Log_sql_guard final {
 public:
  explicit Log_sql_guard(const char *context);
  Log_sql_guard();

  Log_sql_guard(const Log_sql_guard &) = delete;
  Log_sql_guard(Log_sql_guard &&) = delete;

  Log_sql_guard &operator=(const Log_sql_guard &) = delete;
  Log_sql_guard &operator=(Log_sql_guard &&) = delete;

  ~Log_sql_guard();

 private:
  std::shared_ptr<Log_sql> m_obj;
};

}  // namespace shcore

#endif  // MYSQLSHDK_LIBS_UTILS_LOGSQL_H_
