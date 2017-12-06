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

#include "mysqlshdk/libs/utils/logger.h"
#include <utils/utils_general.h>

#include <stdarg.h>
#include <time.h>
#include <utility>
#include <vector>

#if defined __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wshadow"
#ifndef __has_warning
#define __has_warning(x) 0
#endif
#if __has_warning("-Wshorten-64-to-32")
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#endif
#if __has_warning("-Wunused-local-typedef")
#pragma GCC diagnostic ignored "-Wunused-local-typedef"
#endif
#elif defined _MSC_VER
#pragma warning(push)
#pragma warning(disable :)  // TODO(?): add MSVC code for pedantic and shadow
#endif

#ifdef __GNUC__
#pragma GCC diagnostic pop
#elif defined _MSC_VER
#pragma warning(pop)
#endif

#ifdef WIN32
#include <io.h>
#include <windows.h>
#define strcasecmp _stricmp
#pragma warning(disable : 4996)  // disable warnings stating that write() and
                                 // fileno() (POSIX) should be replaced by
                                 // _write() and _fileno() (allegedly ISO C++)
#else
#include <strings.h>
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace ngcommon {

std::unique_ptr<Logger> Logger::instance(nullptr);

Logger::Logger_levels_table Logger::log_levels_table;

void Logger::attach_log_hook(Log_hook hook) {
  if (hook) {
    hook_list.push_back(hook);
  } else {
    throw std::logic_error("Logger::attach_log_hook: Null hook pointer");
  }
}

void Logger::detach_log_hook(Log_hook hook) {
  if (hook) {
    hook_list.remove(hook);
  } else {
    throw std::logic_error("Logger::detach_log_hook: Null hook pointer");
  }
}

void Logger::set_log_level(LOG_LEVEL log_level_) {
  this->log_level = log_level_;
}

Logger::LOG_LEVEL Logger::get_log_level() {
  return log_level;
}

void Logger::assert_logger_initialized() {
  if (instance.get() == nullptr) {
    const char* msg_noinit =
        "Logger: Tried to log to an uninitialized logger.\n";

#ifdef WIN32
    ::write(fileno(stderr), msg_noinit, strlen(msg_noinit));
#else
    ssize_t n = ::write(STDERR_FILENO, msg_noinit, strlen(msg_noinit));
    (void)n;  // silent unused variable warning
#endif
  }
}

std::string Logger::format(const char* formats, ...) {
  va_list args;
  va_start(args, formats);
#ifdef WIN32
  int n = _vscprintf(formats, args);
#else
  size_t n = (size_t)vsnprintf(NULL, 0, formats, args);
#endif
  va_end(args);

  std::string mybuf;
  mybuf.resize(n + 1);

  va_start(args, formats);
  vsnprintf(&mybuf[0], n + 1, formats, args);
  va_end(args);

  return mybuf;
}

void Logger::log_text(LOG_LEVEL level, const char* domain, const char* text) {
  assert_logger_initialized();

  if (instance.get() != nullptr && level <= instance->log_level) {
    std::string s = instance->format_message(domain, text, level);
    s += "\n";
    const char* buf = s.c_str();

    instance->out.write(buf, (std::streamsize)strlen(buf));
    instance->out.flush();

    if (instance->use_stderr) {
      instance->out_to_stderr(buf);
    }

    std::list<Log_hook>::const_iterator myend = instance->hook_list.end();
    for (std::list<Log_hook>::const_iterator it = instance->hook_list.begin();
         it != myend; it++)
      (*it)(buf, level, domain);
  }
}

void Logger::log(LOG_LEVEL level, const char* domain, const char* formats,
                 ...) {
  assert_logger_initialized();

  if (instance.get() != nullptr && level <= instance->log_level) {
    char* mybuf;
    va_list args;
    va_start(args, formats);
#ifdef WIN32
    int n = _vscprintf(formats, args);
#else
    size_t n = (size_t)vsnprintf(NULL, 0, formats, args);
#endif
    va_end(args);

    mybuf = static_cast<char*>(malloc(n + 1));
    va_start(args, formats);
    vsnprintf(mybuf, n + 1, formats, args);
    va_end(args);
    std::string s = instance->format_message(domain, mybuf, level);
    s += "\n";
    const char* buf = s.c_str();

    instance->out.write(buf, (std::streamsize)strlen(buf));
    instance->out.flush();

    if (instance->use_stderr) {
      instance->out_to_stderr(buf);
    }

    std::list<Log_hook>::const_iterator myend = instance->hook_list.end();
    for (std::list<Log_hook>::const_iterator it = instance->hook_list.begin();
         it != myend; it++)
      (*it)(buf, level, domain);

    free(mybuf);
  }
}

void Logger::log_exc(const char* domain, const char* message,
                     const std::exception& exc) {
  assert_logger_initialized();
  if (instance.get() != nullptr) {
    Logger::LOG_LEVEL level = Logger::LOG_ERROR;
    std::string s = instance->format_message(domain, message, exc);
    const char* buf = s.c_str();

    if (!instance->out.is_open()) {
      instance->out.write(buf, (std::streamsize)strlen(buf));
      instance->out.flush();
    }

    if (instance->use_stderr) {
      instance->out_to_stderr(buf);
    }

    std::list<Log_hook>::const_iterator myend = instance->hook_list.end();
    for (std::list<Log_hook>::const_iterator it = instance->hook_list.begin();
         it != myend; it++)
      (*it)(buf, level, domain);
  }
}

Logger* Logger::singleton() {
  if (instance.get() != nullptr) {
    return instance.get();
  } else {
    throw std::logic_error("ngcommon::Logger not initialized");
  }
}

void Logger::out_to_stderr(const char* msg) {
#ifdef WIN32
  OutputDebugString(msg);
#else
  ssize_t n = ::write(STDERR_FILENO, msg, strlen(msg));
  (void)n;  // silent unused variable warning
#endif
}

/*static*/ std::string Logger::format_message(const char* domain,
                                              const char* message,
                                              Logger::LOG_LEVEL log_level_) {
  return format_message_common(domain, message, log_level_);
}

/*static*/ std::string Logger::format_message(const char* domain, const char*,
                                              const std::exception& exc) {
  return format_message_common(domain, exc.what(), Logger::LOG_ERROR);
}

/*static*/ std::string Logger::format_message_common(
    const char* domain, const char* message, Logger::LOG_LEVEL log_level_) {
  // get GMT time as string
  char timestamp[32];
  {
    time_t t = time(NULL);
#ifdef _WIN32
    struct tm tm;
    gmtime_s(&tm, &t);
#else
    struct tm tm;
    gmtime_r(&t, &tm);
#endif

    char date_format[] = "%Y-%m-%d %H:%M:%S: ";
    strftime(timestamp, sizeof(timestamp), date_format, &tm);
  }

  // build return string
  std::string result;
  {
    result.reserve(512);
    result += timestamp;
    result += get_log_level_desc(log_level_);
    result += ':';
    result += ' ';
    if (domain) {
      result += domain;
      result += ':';
      result += ' ';
    }
    result += message;
  }

  return result;
}

void Logger::setup_instance(const char* filename, bool use_stderr,
                            Logger::LOG_LEVEL log_level) {
  if (instance.get() != nullptr) {
    if (filename && instance->out_name.compare(filename) != 0) {
      instance->out.close();
      instance->out.open(filename, std::ios_base::app);
      if (instance->out.fail())
        throw std::logic_error(
            std::string("Error in Logger::Logger when opening file '") +
            filename + "' for writing");
      instance->out_name = filename;
    }
    instance->set_log_level(log_level);
    instance->use_stderr = use_stderr;
  } else {
    instance.reset(new Logger(filename, use_stderr, log_level));
  }
}

/*static*/ const char* Logger::get_log_level_desc(
    Logger::LOG_LEVEL log_level_) {
  return Logger::log_levels_table.get_level_name_by_enum(log_level_).c_str();
}

Logger::Logger(const char* filename, bool use_stderr_,
               Logger::LOG_LEVEL log_level_) {
  this->use_stderr = use_stderr_;
  this->log_level = log_level_;
  if (filename != NULL) {
    out_name = filename;
    out.open(filename, std::ios_base::app);
    if (out.fail())
      throw std::logic_error(
          std::string("Error in Logger::Logger when opening file '") +
          filename + "' for writing");
  }
}

Logger::~Logger() {
  if (out.is_open())
    out.close();
}

Logger::LOG_LEVEL Logger::get_level_by_name(const std::string& level_name) {
  return log_levels_table.get_level_by_name(level_name);
}

Logger::LOG_LEVEL Logger::get_log_level(const std::string& tag) {
  LOG_LEVEL level = log_levels_table.get_level_by_name(tag);

  if (level != LOG_NONE || is_level_none(tag))
    return level;

  if (tag.size() == 1) {
    int nlevel = tag.c_str()[0] - '0';
    if (nlevel >= 1 && nlevel <= 8)
      return static_cast<LOG_LEVEL>(nlevel);
  }

  return LOG_NONE;
}

bool Logger::is_level_none(const std::string& tag) {
  if (strcasecmp("none", tag.c_str()) == 0 || (tag == "1"))
    return true;
  else
    return false;
}

std::string Logger::get_level_range_info()
{
  std::vector<std::string> info = { "Value must be an integer between 1 and 8 "
                                    "any of [none, internal, error, warning, "
                                    "info, debug, debug2, debug3]."};
  return shcore::format_markup_text(info, 80, 27);
}

Logger::LOG_LEVEL Logger::Logger_levels_table::get_level_by_name(
    const std::string& level_name) {
  std::map<std::string, LOG_LEVEL, Case_insensitive_comp>::const_iterator it =
      descr_to_level.find(level_name);
  if (it != descr_to_level.end())
    return (*it).second;
  else
    return LOG_NONE;
}

bool Logger::Case_insensitive_comp::operator()(const std::string& lhs,
                                               const std::string& rhs) const {
  return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
}

}  // namespace ngcommon
