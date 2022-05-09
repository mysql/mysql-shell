/*
 * Copyright (c) 2015, 2022, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_UTILS_LOGGER_H_
#define MYSQLSHDK_LIBS_UTILS_LOGGER_H_

#include <atomic>
#include <cstdarg>
#include <fstream>
#include <list>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <tuple>

#ifndef _WIN32
#include <cstdio>
#endif

#ifdef _MSC_VER
#include <sal.h>
#endif  // _MSC_VER

#include "mysqlshdk/include/mysqlshdk_export.h"

namespace shcore {

class SHCORE_PUBLIC Logger final {
 public:
  enum LOG_LEVEL {
    LOG_NONE = 1,
    LOG_INTERNAL_ERROR = 2,
    LOG_ERROR = 3,
    LOG_WARNING = 4,
    LOG_INFO = 5,
    LOG_DEBUG = 6,
    LOG_DEBUG2 = 7,
    LOG_DEBUG3 = 8,
    LOG_MAX_LEVEL = 8
  };

  struct Log_entry {
    Log_entry();
    Log_entry(const char *domain, const char *message, size_t length,
              LOG_LEVEL level);

    time_t timestamp;
    const char *domain;
    const char *message;
    size_t length;  //< message length
    LOG_LEVEL level;
  };

  using Log_hook = void (*)(const Log_entry &entry, void *);
  using Log_level_hook = void (*)(LOG_LEVEL level, void *);

  Logger(const Logger &) = delete;
  Logger(Logger &&) = delete;

  ~Logger();

  Logger &operator=(const Logger &) = delete;
  Logger &operator=(Logger &&) = delete;

  void attach_log_hook(Log_hook hook, void *hook_data = nullptr,
                       bool catch_all = false);
  void detach_log_hook(Log_hook hook);

  void attach_log_level_hook(Log_level_hook hook, void *hook_data = nullptr);
  void detach_log_level_hook(Log_level_hook hook);

  bool log_allowed() const;

  void set_log_level(LOG_LEVEL log_level);
  LOG_LEVEL get_log_level() const { return m_log_level; }

  void log_to_stderr();

  void stop_log_to_stderr();

#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
  static void log(LOG_LEVEL level, const char *format, ...)
      __attribute__((__format__(__printf__, 2, 3)));
#elif _MSC_VER
  static void log(LOG_LEVEL level,
                  _In_z_ _Printf_format_string_ const char *format, ...);
#else
  static void log(LOG_LEVEL level, const char *format, ...);
#endif

  static std::shared_ptr<Logger> create_instance(const char *filename,
                                                 bool use_stderr = false,
                                                 LOG_LEVEL level = LOG_INFO);

  static LOG_LEVEL parse_log_level(const std::string &tag);

  static const char *get_level_range_info();

  static void set_stderr_output_format(const std::string &format);

  static std::string stderr_output_format();

  const std::string &logfile_name() const { return m_log_file_name; }

  bool use_stderr() const;

  void push_context(std::string context) {
    std::lock_guard l{m_mutex_log_ctx};
    m_log_context.push_back(std::move(context));
  }

  void pop_context() {
    std::lock_guard l{m_mutex_log_ctx};
    m_log_context.pop_back();
  }

  std::string context() const {
    std::lock_guard l{m_mutex_log_ctx};
    return m_log_context.empty() ? "" : m_log_context.back();
  }

  inline std::string get_initialization_warning() const {
    return m_initialization_warning;
  }

 private:
  Logger(const char *filename, bool use_stderr);

  static void out_to_stderr(const Log_entry &entry, void *);

  static std::string format_message(const Log_entry &entry);

  static void assert_logger_initialized();

  static std::string format(const char *formats, va_list args);

  static void do_log(const Log_entry &entry);
  static void do_log(const Logger &logger, const Log_entry &entry);

  bool will_log(LOG_LEVEL level) const;

  std::atomic<LOG_LEVEL> m_log_level{LOG_NONE};

#ifdef _WIN32
  std::ofstream m_log_file;
#else  // !_WIN32
  FILE *m_log_file = nullptr;
#endif
  std::string m_log_file_name;

  mutable std::mutex m_mutex_hooks;
  std::list<std::tuple<Log_hook, void *, bool>> m_hook_list;
  std::list<std::tuple<Log_level_hook, void *>> m_level_hook_list;

  mutable std::mutex m_mutex_log_ctx;
  std::list<std::string> m_log_context;

  std::atomic<int> m_dont_log;

  std::string m_initialization_warning;

  friend class Log_reentrant_protector;
  friend class Log_sql;
};

// implemented in scoped_contexts.cc
std::shared_ptr<shcore::Logger> current_logger(bool allow_empty = false);

/** The Shell_console::print* functions may log some information.
 *  when --verbose is enable, those are also printed which may cause a cyclic
 * call. This Log_protector is used to break the cycle, hence works as a shield
 * for the Shell_console::print functions.
 */
class Log_reentrant_protector {
 public:
  Log_reentrant_protector() { current_logger()->m_dont_log++; }
  ~Log_reentrant_protector() { current_logger()->m_dont_log--; }
};

struct Log_context {
  explicit Log_context(const std::string &context) {
    current_logger()->push_context(context);
  }

  ~Log_context() { current_logger()->pop_context(); }
};

#define log_error(...) \
  shcore::Logger::log(shcore::Logger::LOG_ERROR, __VA_ARGS__)

#define log_warning(...) \
  shcore::Logger::log(shcore::Logger::LOG_WARNING, __VA_ARGS__)

#define log_info(...) shcore::Logger::log(shcore::Logger::LOG_INFO, __VA_ARGS__)

#define log_debug(...) \
  shcore::Logger::log(shcore::Logger::LOG_DEBUG, __VA_ARGS__)

#define log_debug2(...) \
  shcore::Logger::log(shcore::Logger::LOG_DEBUG2, __VA_ARGS__)

#define log_debug3(...) \
  shcore::Logger::log(shcore::Logger::LOG_DEBUG3, __VA_ARGS__)

}  // namespace shcore

#endif  // MYSQLSHDK_LIBS_UTILS_LOGGER_H_
