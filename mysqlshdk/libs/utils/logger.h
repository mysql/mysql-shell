/*
* Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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


#ifndef _LOGGER_H_
#define _LOGGER_H_


#include <fstream>
#include <string>
#include <stdexcept>
#include <list>
#include <map>
#include <string>
#include <memory>

#ifdef _MSC_VER
#  include <sal.h>
#endif

#if defined(WIN32) && defined(_WINDLL)
#  ifdef NGCOMMON_EXPORTS
#    define NGCOMMON_API __declspec(dllexport)
#  else
#    define NGCOMMON_API __declspec(dllimport)
#  endif
#else
#  define NGCOMMON_API
#endif

namespace ngcommon
{

#define DOMAIN_DEFAULT NULL
#ifndef LOG_DOMAIN
#  define LOG_DOMAIN DOMAIN_DEFAULT
#endif

namespace tests { class NGCOMMON_API LoggerTestProxy; }

class NGCOMMON_API Logger
{
public:
  enum LOG_LEVEL
  {
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

  // receives a message formatted by the log infrastructure
  typedef void(*Log_hook)(const char* message, LOG_LEVEL level, const char* domain);

  // can be multicast, set as a linked list
  void attach_log_hook(Log_hook hook);
  void detach_log_hook(Log_hook hook);

  void set_log_level(LOG_LEVEL log_level);
  LOG_LEVEL get_log_level();

#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
  static void log(LOG_LEVEL level, const char* domain, const char* format, ...) __attribute__((__format__(__printf__, 3, 4)));
  static std::string format(const char* formats, ...) __attribute__((__format__(__printf__, 1, 2)));
#elif _MSC_VER
  static void log(LOG_LEVEL level, const char* domain, _In_z_ _Printf_format_string_ const char* format, ...);
  static std::string format(_In_z_ _Printf_format_string_ const char* format, ...);
#else
  static void log(LOG_LEVEL level, const char* domain, const char* format, ...);
  static std::string format(const char* formats, ...);
#endif
  static void log_text(LOG_LEVEL level, const char* domain, const char* text);
  static void log_text(LOG_LEVEL level, const char* domain, const std::string &text)
  {
    log_text(level, domain, text.c_str());
  }

  static void log_exc(const char *domain, const char* message, const std::exception &exc);

  static Logger* singleton();

  // Creates singleton instance with proper parameters
  static void setup_instance(const char* filename, bool use_stderr = false,
                             LOG_LEVEL level = LOG_INFO);

  static LOG_LEVEL get_level_by_name(const std::string& level_name);

  static LOG_LEVEL get_log_level(const std::string& tag);

  static bool is_level_none(const std::string& tag);

  static std::string get_level_range_info();

  ~Logger();

  const std::string &logfile_name() const {
    return out_name;
  }

private:
  struct Case_insensitive_comp
  {
    bool operator() (const std::string& lhs, const std::string& rhs) const;
  };

  struct Logger_levels_table
  {
  private:
    std::string descrs[LOG_MAX_LEVEL + 1];
    std::map<std::string, LOG_LEVEL, Case_insensitive_comp>descr_to_level;
  public:
    Logger_levels_table()
    {
      descrs[0] = "";
      descrs[1] = "None";
      descrs[2] = "INTERNAL";
      descrs[3] = "Error";
      descrs[4] = "Warning";
      descrs[5] = "Info";
      descrs[6] = "Debug";
      descrs[7] = "Debug2";
      descrs[8] = "Debug3";

      for (int i = 1; i <= LOG_MAX_LEVEL; i++)
      {
        descr_to_level.insert(std::map<std::string, LOG_LEVEL>::value_type(descrs[i], static_cast<LOG_LEVEL>(i)));
      }
    }

    LOG_LEVEL get_level_by_name(const std::string& level_name);

    std::string& get_level_name_by_enum(LOG_LEVEL level)
    {
      return descrs[level];
    }
  };

  // Outputs to stderr or OutputDebugStringA in Windows
  Logger(const char *filename, bool use_stderr = false, LOG_LEVEL log_level = LOG_INFO);

  void out_to_stderr(const char* msg);
  static std::string format_message(const char* domain, const char* message, LOG_LEVEL log_level);
  static std::string format_message(const char* domain, const char*,         const std::exception& exc);
  static std::string format_message_common(const char* domain, const char* message, Logger::LOG_LEVEL log_level);
  static const char* get_log_level_desc(LOG_LEVEL log_level);
  static void assert_logger_initialized();

  static std::unique_ptr<Logger> instance;
  static struct Logger_levels_table log_levels_table;

  LOG_LEVEL log_level;
  bool use_stderr;
  std::ofstream out;
  std::string out_name;
  std::list<Log_hook> hook_list;

  friend class tests::LoggerTestProxy;
};

#define log_internal_error(...) ngcommon::Logger::log(ngcommon::Logger::LOG_INTERNAL_ERROR, LOG_DOMAIN, __VA_ARGS__)
#define log_unexpected(...)     ngcommon::Logger::log(ngcommon::Logger::LOG_INTERNAL_ERROR, LOG_DOMAIN, __VA_ARGS__)

#define log_exception(msg, exc) ngcommon::Logger::log_exc(LOG_DOMAIN, msg, exc)
#define log_error(...)          ngcommon::Logger::log(ngcommon::Logger::LOG_ERROR,   LOG_DOMAIN, __VA_ARGS__)
#define log_warning(...)        ngcommon::Logger::log(ngcommon::Logger::LOG_WARNING, LOG_DOMAIN, __VA_ARGS__)
#define log_info(...)           ngcommon::Logger::log(ngcommon::Logger::LOG_INFO,    LOG_DOMAIN, __VA_ARGS__)
#define log_debug(...)          ngcommon::Logger::log(ngcommon::Logger::LOG_DEBUG,   LOG_DOMAIN, __VA_ARGS__)

#ifndef NDEBUG
  #define log_debug2(args) ngcommon::Logger::log_text(ngcommon::Logger::LOG_DEBUG2, LOG_DOMAIN, ngcommon::Logger::format args)
  #define log_debug3(args) ngcommon::Logger::log_text(ngcommon::Logger::LOG_DEBUG3, LOG_DOMAIN, ngcommon::Logger::format args)

  #define log_secret(...)  ngcommon::Logger::log(ngcommon::Logger::LOG_DEBUG, LOG_DOMAIN, __VA_ARGS__)
#else
  #define log_debug2(args) do {} while(0)
  #define log_debug3(args) do {} while(0)

  #define log_secret(...) do {} while(0)
#endif

}

#endif
