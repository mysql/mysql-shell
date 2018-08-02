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

#include "mysqlshdk/libs/utils/logger.h"

#include <rapidjson/document.h>
#include <stdarg.h>
#include <time.h>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else  // !_WIN32
#include <unistd.h>
#endif  // !_WIN32

#include <algorithm>
#include <map>
#include <utility>

#include "mysqlshdk/libs/utils/utils_general.h"

namespace ngcommon {

namespace {

struct Logger_levels_table {
 public:
  Logger_levels_table() {
    m_names[0] = "";
    m_names[1] = "None";
    m_names[2] = "INTERNAL";
    m_names[3] = "Error";
    m_names[4] = "Warning";
    m_names[5] = "Info";
    m_names[6] = "Debug";
    m_names[7] = "Debug2";
    m_names[8] = "Debug3";

    for (int i = Logger::LOG_NONE; i <= Logger::LOG_MAX_LEVEL; ++i) {
      m_levels.insert(
          std::make_pair(m_names[i], static_cast<Logger::LOG_LEVEL>(i)));
    }
  }

  Logger::LOG_LEVEL to_log_level(const std::string &level_name) {
    const auto it = m_levels.find(level_name);
    if (m_levels.end() != it)
      return it->second;
    else
      return Logger::LOG_NONE;
  }

  std::string &to_string(Logger::LOG_LEVEL level) { return m_names[level]; }

 private:
  struct Case_insensitive_comp {
    bool operator()(const std::string &lhs, const std::string &rhs) const {
      return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
    }
  };

  std::string m_names[Logger::LOG_MAX_LEVEL + 1];
  std::map<std::string, Logger::LOG_LEVEL, Case_insensitive_comp> m_levels;
};

Logger_levels_table g_level_converter;

}  // namespace

std::unique_ptr<Logger> Logger::s_instance;
std::string Logger::s_output_format;

Logger::Log_entry::Log_entry()
    : Log_entry(nullptr, nullptr, LOG_LEVEL::LOG_NONE) {}

Logger::Log_entry::Log_entry(const char *domain, const char *message,
                             LOG_LEVEL level)
    : timestamp{time(nullptr)},
      domain{domain},
      message{message},
      level{level} {}

void Logger::attach_log_hook(Log_hook hook) {
  if (hook) {
    m_hook_list.push_back(hook);
  } else {
    throw std::logic_error("Logger::attach_log_hook: Null hook pointer");
  }
}

void Logger::detach_log_hook(Log_hook hook) {
  if (hook) {
    m_hook_list.remove(hook);
  } else {
    throw std::logic_error("Logger::detach_log_hook: Null hook pointer");
  }
}

void Logger::set_log_level(LOG_LEVEL log_level) {
  this->m_log_level = log_level;
}

Logger::LOG_LEVEL Logger::get_log_level() { return m_log_level; }

void Logger::assert_logger_initialized() {
  if (s_instance.get() == nullptr) {
    static constexpr auto msg_noinit =
        "Logger: Tried to log to an uninitialized logger.\n";

#ifdef _WIN32
    ::_write(fileno(stderr), msg_noinit, strlen(msg_noinit));
#else   // !_WIN32
    ::write(STDERR_FILENO, msg_noinit, strlen(msg_noinit));
#endif  // !_WIN32
  }
}

std::string Logger::format(const char *formats, ...) {
  va_list args;
  va_start(args, formats);
  const auto msg = format(formats, args);
  va_end(args);

  return msg;
}

std::string Logger::format(const char *formats, va_list args) {
  va_list args_copy;
  va_copy(args_copy, args);
#ifdef _WIN32
  int n = _vscprintf(formats, args_copy);
#else   // !_WIN32
  size_t n = (size_t)vsnprintf(nullptr, 0, formats, args_copy);
#endif  // !_WIN32
  va_end(args_copy);

  std::string mybuf;
  mybuf.resize(n + 1);

  va_copy(args_copy, args);
  vsnprintf(&mybuf[0], n + 1, formats, args_copy);
  va_end(args_copy);

  return mybuf;
}

void Logger::log(LOG_LEVEL level, const char *domain, const char *formats,
                 ...) {
  assert_logger_initialized();

  if (s_instance.get() != nullptr && level <= s_instance->m_log_level) {
    va_list args;
    va_start(args, formats);
    const auto msg = format(formats, args);
    va_end(args);

    do_log({domain, msg.c_str(), level});
  }
}

void Logger::log(const std::exception &exc, const char *domain,
                 const char *formats, ...) {
  assert_logger_initialized();

  if (s_instance.get() != nullptr) {
    va_list args;
    va_start(args, formats);
    const auto msg =
        format("%s: %s", format(formats, args).c_str(), exc.what());
    va_end(args);

    do_log({domain, msg.c_str(), Logger::LOG_ERROR});
  }
}

void Logger::do_log(const Log_entry &entry) {
  const auto s = format_message(entry);

  if (s_instance->m_log_file.is_open()) {
    s_instance->m_log_file.write(s.c_str(), s.length());
    s_instance->m_log_file.flush();
  }

  for (const auto &f : s_instance->m_hook_list) {
    f(entry);
  }
}

Logger *Logger::singleton() {
  if (s_instance.get() != nullptr) {
    return s_instance.get();
  } else {
    throw std::logic_error("ngcommon::Logger not initialized");
  }
}

void Logger::out_to_stderr(const Log_entry &entry) {
  std::string msg;

  if (std::string::npos != s_output_format.find("json")) {
    rapidjson::Document doc{rapidjson::Type::kObjectType};
    auto &allocator = doc.GetAllocator();

    const auto timestamp = std::to_string(entry.timestamp);
    doc.AddMember(rapidjson::StringRef("timestamp"),
                  rapidjson::StringRef(timestamp.c_str(), timestamp.length()),
                  allocator);

    const auto level = g_level_converter.to_string(entry.level);
    doc.AddMember(rapidjson::StringRef("level"),
                  rapidjson::StringRef(level.c_str(), level.length()),
                  allocator);

    if (entry.domain) {
      doc.AddMember(rapidjson::StringRef("domain"),
                    rapidjson::StringRef(entry.domain), allocator);
    }

    doc.AddMember(rapidjson::StringRef("message"),
                  rapidjson::StringRef(entry.message), allocator);

    rapidjson::StringBuffer buffer;

    if ("json" == s_output_format) {
      rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
      doc.Accept(writer);
    } else {
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      doc.Accept(writer);
    }

    msg = std::string{buffer.GetString(), buffer.GetSize()} + "\n";
  } else {
    msg = format_message(entry);
  }

#ifdef _WIN32
  OutputDebugString(msg.c_str());
#else   // !_WIN32
  ::write(STDERR_FILENO, msg.c_str(), msg.length());
#endif  // !_WIN32
}

std::string Logger::format_message(const Log_entry &entry) {
  // get GMT time as string
  char timestamp[32];
  {
    struct tm tm;
#ifdef _WIN32
    gmtime_s(&tm, &entry.timestamp);
#else   // !_WIN32
    gmtime_r(&entry.timestamp, &tm);
#endif  // !_WIN32

    static constexpr auto date_format = "%Y-%m-%d %H:%M:%S: ";
    strftime(timestamp, sizeof(timestamp), date_format, &tm);
  }

  // build return string
  std::string result;
  {
    result.reserve(512);
    result += timestamp;
    result += g_level_converter.to_string(entry.level);
    result += ':';
    result += ' ';
    if (entry.domain) {
      result += entry.domain;
      result += ':';
      result += ' ';
    }
    result += entry.message;
    result += "\n";
  }

  return result;
}

void Logger::setup_instance(const char *filename, bool use_stderr,
                            Logger::LOG_LEVEL log_level) {
  if (s_instance) {
    if (filename) {
      if (filename != s_instance->m_log_file_name) {
        if (s_instance->m_log_file.is_open()) s_instance->m_log_file.close();
        s_instance->m_log_file_name = filename;

        s_instance->m_log_file.open(filename, std::ios_base::app);
        if (s_instance->m_log_file.fail())
          throw std::logic_error(
              std::string("Error in Logger::Logger when opening file '") +
              filename + "' for writing");
      }
    } else {
      if (s_instance->m_log_file.is_open()) s_instance->m_log_file.close();
      s_instance->m_log_file_name.clear();
    }

    s_instance->set_log_level(log_level);
    s_instance->detach_log_hook(&Logger::out_to_stderr);

    if (use_stderr) {
      s_instance->attach_log_hook(&Logger::out_to_stderr);
    }
  } else {
    s_instance.reset(new Logger(filename, use_stderr, log_level));
  }
}

Logger::Logger(const char *filename, bool use_stderr,
               Logger::LOG_LEVEL log_level)
    : m_log_level{log_level} {
  if (filename != nullptr) {
    m_log_file_name = filename;
    m_log_file.open(filename, std::ios_base::app);
    if (m_log_file.fail())
      throw std::logic_error(
          std::string("Error in Logger::Logger when opening file '") +
          filename + "' for writing");
  }

  if (use_stderr) {
    attach_log_hook(&Logger::out_to_stderr);
  }
}

Logger::~Logger() {
  if (m_log_file.is_open()) m_log_file.close();
}

Logger::LOG_LEVEL Logger::get_level_by_name(const std::string &level_name) {
  return g_level_converter.to_log_level(level_name);
}

Logger::LOG_LEVEL Logger::get_log_level(const std::string &tag) {
  LOG_LEVEL level = get_level_by_name(tag);

  if (level != LOG_NONE || is_level_none(tag)) return level;

  if (tag.size() == 1) {
    int nlevel = tag.c_str()[0] - '0';
    if (nlevel >= 1 && nlevel <= 8) return static_cast<LOG_LEVEL>(nlevel);
  }

  return LOG_NONE;
}

bool Logger::is_level_none(const std::string &tag) {
  if (strcasecmp("none", tag.c_str()) == 0 || (tag == "1"))
    return true;
  else
    return false;
}

const char *Logger::get_level_range_info() {
  return "The log level value must be an integer between 1 and 8 or any of "
         "[none, internal, error, warning, info, debug, debug2, debug3] "
         "respectively.";
}

void Logger::set_stderr_output_format(const std::string &format) {
  s_output_format = format;
}

std::string Logger::stderr_output_format() { return s_output_format; }

bool Logger::use_stderr() const {
  return m_hook_list.end() != std::find(m_hook_list.begin(), m_hook_list.end(),
                                        &Logger::out_to_stderr);
}

}  // namespace ngcommon
