/*
 * Copyright (c) 2015, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <fstream>
#include <list>
#include <memory>
#include <stdexcept>
#include <string>

#ifdef _MSC_VER
#include <sal.h>
#endif  // _MSC_VER

#include "mysqlshdk/include/mysqlshdk_export.h"

namespace ngcommon {

#define DOMAIN_DEFAULT nullptr
#ifndef LOG_DOMAIN
#define LOG_DOMAIN DOMAIN_DEFAULT
#endif

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
    Log_entry(const char *domain, const char *message, LOG_LEVEL level);

    time_t timestamp;
    const char *domain;
    const char *message;
    LOG_LEVEL level;
  };

  using Log_hook = void (*)(const Log_entry &entry);

  Logger(const Logger &) = delete;
  Logger(Logger &&) = delete;

  ~Logger();

  Logger &operator=(const Logger &) = delete;
  Logger &operator=(Logger &&) = delete;

  void attach_log_hook(Log_hook hook);
  void detach_log_hook(Log_hook hook);

  void set_log_level(LOG_LEVEL log_level);
  LOG_LEVEL get_log_level();

#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
  static void log(LOG_LEVEL level, const char *domain, const char *format, ...)
      __attribute__((__format__(__printf__, 3, 4)));
  static void log(const std::exception &exc, const char *domain,
                  const char *format, ...)
      __attribute__((__format__(__printf__, 3, 4)));
#elif _MSC_VER
  static void log(LOG_LEVEL level, const char *domain,
                  _In_z_ _Printf_format_string_ const char *format, ...);
  static void log(const std::exception &exc, const char *domain,
                  _In_z_ _Printf_format_string_ const char *format, ...);
#else
  static void log(LOG_LEVEL level, const char *domain, const char *format, ...);
  static void log(const std::exception &exc, const char *domain,
                  const char *format, ...);
#endif

  static Logger *singleton();

  // Creates singleton instance with proper parameters
  static void setup_instance(const char *filename, bool use_stderr = false,
                             LOG_LEVEL level = LOG_INFO);

  static LOG_LEVEL get_level_by_name(const std::string &level_name);

  static LOG_LEVEL get_log_level(const std::string &tag);

  static bool is_level_none(const std::string &tag);

  static const char *get_level_range_info();

  static void set_stderr_output_format(const std::string &format);

  static std::string stderr_output_format();

  const std::string &logfile_name() const { return m_log_file_name; }

  bool use_stderr() const;

 private:
  Logger(const char *filename, bool use_stderr = false,
         LOG_LEVEL log_level = LOG_INFO);

  static void out_to_stderr(const Log_entry &entry);

  static std::string format_message(const Log_entry &entry);

  static void assert_logger_initialized();

#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
  static std::string format(const char *formats, ...)
      __attribute__((__format__(__printf__, 1, 2)));
#elif _MSC_VER
  static std::string format(_In_z_ _Printf_format_string_ const char *format,
                            ...);
#else
  static std::string format(const char *formats, ...);
#endif

  static std::string format(const char *formats, va_list args);

  static void do_log(const Log_entry &entry);

  static std::unique_ptr<Logger> s_instance;
  static std::string s_output_format;

  LOG_LEVEL m_log_level;
  std::ofstream m_log_file;
  std::string m_log_file_name;
  std::list<Log_hook> m_hook_list;
};

#define log_internal_error(...)                                           \
  ngcommon::Logger::log(ngcommon::Logger::LOG_INTERNAL_ERROR, LOG_DOMAIN, \
                        __VA_ARGS__)
#define log_unexpected(...)                                               \
  ngcommon::Logger::log(ngcommon::Logger::LOG_INTERNAL_ERROR, LOG_DOMAIN, \
                        __VA_ARGS__)

#define log_exception(exc, ...) \
  ngcommon::Logger::log(exc, LOG_DOMAIN, __VA_ARGS__)
#define log_error(...) \
  ngcommon::Logger::log(ngcommon::Logger::LOG_ERROR, LOG_DOMAIN, __VA_ARGS__)
#define log_warning(...) \
  ngcommon::Logger::log(ngcommon::Logger::LOG_WARNING, LOG_DOMAIN, __VA_ARGS__)
#define log_info(...) \
  ngcommon::Logger::log(ngcommon::Logger::LOG_INFO, LOG_DOMAIN, __VA_ARGS__)
#define log_debug(...) \
  ngcommon::Logger::log(ngcommon::Logger::LOG_DEBUG, LOG_DOMAIN, __VA_ARGS__)

#ifndef NDEBUG
#define log_debug2(...) \
  ngcommon::Logger::log(ngcommon::Logger::LOG_DEBUG2, LOG_DOMAIN, __VA_ARGS__)
#define log_debug3(...) \
  ngcommon::Logger::log(ngcommon::Logger::LOG_DEBUG3, LOG_DOMAIN, __VA_ARGS__)

#define log_secret(...) \
  ngcommon::Logger::log(ngcommon::Logger::LOG_DEBUG, LOG_DOMAIN, __VA_ARGS__)
#else
#define log_debug2(...) \
  do {                  \
  } while (0)
#define log_debug3(...) \
  do {                  \
  } while (0)

#define log_secret(...) \
  do {                  \
  } while (0)
#endif
}  // namespace ngcommon

#endif  // MYSQLSHDK_LIBS_UTILS_LOGGER_H_
