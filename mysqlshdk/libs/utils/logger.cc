/*
 * Copyright (c) 2015, 2019, Oracle and/or its affiliates. All rights reserved.
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
#include "mysqlshdk/libs/utils/utils_string.h"

namespace shcore {

namespace {

std::string to_string(Logger::LOG_LEVEL level) {
  switch (level) {
    case Logger::LOG_NONE:
      return "None";
    case Logger::LOG_INTERNAL_ERROR:
      return "INTERNAL";
    case Logger::LOG_ERROR:
      return "Error";
    case Logger::LOG_WARNING:
      return "Warning";
    case Logger::LOG_INFO:
      return "Info";
    case Logger::LOG_DEBUG:
      return "Debug";
    case Logger::LOG_DEBUG2:
      return "Debug2";
    case Logger::LOG_DEBUG3:
      return "Debug3";
    default:
      return "";
  }
}

Logger::LOG_LEVEL get_level_by_name(const std::string &name) {
  if (strcasecmp(name.c_str(), "none") == 0)
    return Logger::LOG_NONE;
  else if (strcasecmp(name.c_str(), "internal") == 0)
    return Logger::LOG_INTERNAL_ERROR;
  else if (strcasecmp(name.c_str(), "error") == 0)
    return Logger::LOG_ERROR;
  else if (strcasecmp(name.c_str(), "warning") == 0)
    return Logger::LOG_WARNING;
  else if (strcasecmp(name.c_str(), "info") == 0)
    return Logger::LOG_INFO;
  else if (strcasecmp(name.c_str(), "debug") == 0)
    return Logger::LOG_DEBUG;
  else if (strcasecmp(name.c_str(), "debug2") == 0)
    return Logger::LOG_DEBUG2;
  else if (strcasecmp(name.c_str(), "debug3") == 0)
    return Logger::LOG_DEBUG3;
  else
    throw std::invalid_argument("invalid level");
}

}  // namespace

std::unique_ptr<Logger> Logger::s_instance;
std::string Logger::s_output_format;

Logger::Log_entry::Log_entry()
    : Log_entry(nullptr, nullptr, LOG_LEVEL::LOG_NONE) {}

Logger::Log_entry::Log_entry(const char *domain_, const char *message_,
                             LOG_LEVEL level_)
    : timestamp{time(nullptr)},
      domain{domain_},
      message{message_},
      level{level_} {}

void Logger::attach_log_hook(Log_hook hook, void *user_data, bool catch_all) {
  if (hook) {
    m_hook_list.emplace_back(hook, user_data, catch_all);
  } else {
    throw std::invalid_argument("Logger::attach_log_hook: Null hook pointer");
  }
}

void Logger::detach_log_hook(Log_hook hook) {
  if (hook) {
    m_hook_list.remove_if([hook](const std::tuple<Log_hook, void *, bool> &i) {
      return std::get<0>(i) == hook;
    });
  } else {
    throw std::invalid_argument("Logger::detach_log_hook: Null hook pointer");
  }
}

void Logger::set_log_level(LOG_LEVEL log_level) { m_log_level = log_level; }

void Logger::assert_logger_initialized() {
  if (s_instance.get() == nullptr) {
    static constexpr auto msg_noinit =
        "Logger: Tried to log to an uninitialized logger.\n";

#ifdef _WIN32
    ::_write(fileno(stderr), msg_noinit, strlen(msg_noinit));
#else   // !_WIN32
    const auto ignore = ::write(STDERR_FILENO, msg_noinit, strlen(msg_noinit));
    (void)ignore;
#endif  // !_WIN32
  }
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

void Logger::log(LOG_LEVEL level, const char *formats, ...) {
  assert_logger_initialized();

  if (s_instance.get() != nullptr) {
    va_list args;
    va_start(args, formats);
    const auto msg = format(formats, args);
    va_end(args);

    do_log({s_instance->context(), msg.c_str(), level});
  }
}

void Logger::do_log(const Log_entry &entry) {
  std::lock_guard<std::recursive_mutex> lock(s_instance->m_mutex);

  if (s_instance->m_log_file.is_open() &&
      entry.level <= s_instance->m_log_level) {
    const auto s = format_message(entry);
    s_instance->m_log_file.write(s.c_str(), s.length());
    s_instance->m_log_file.flush();
  }

  for (const auto &f : s_instance->m_hook_list) {
    if (std::get<2>(f) || entry.level <= s_instance->m_log_level)
      std::get<0>(f)(entry, std::get<1>(f));
  }
}

Logger *Logger::singleton() {
  if (s_instance.get() != nullptr) {
    return s_instance.get();
  } else {
    throw std::logic_error("shcore::Logger not initialized");
  }
}

void Logger::out_to_stderr(const Log_entry &entry, void *) {
  std::string msg;

  if (std::string::npos != s_output_format.find("json")) {
    rapidjson::Document doc{rapidjson::Type::kObjectType};
    auto &allocator = doc.GetAllocator();

    const auto timestamp = std::to_string(entry.timestamp);
    doc.AddMember(rapidjson::StringRef("timestamp"),
                  rapidjson::StringRef(timestamp.c_str(), timestamp.length()),
                  allocator);

    const auto level = to_string(entry.level);
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
  const auto ignore = ::write(STDERR_FILENO, msg.c_str(), msg.length());
  (void)ignore;
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
    result += to_string(entry.level);
    result += ':';
    result += ' ';
    if (entry.domain && *entry.domain) {
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

    s_instance->detach_log_hook(&Logger::out_to_stderr);

    if (use_stderr) {
      s_instance->attach_log_hook(&Logger::out_to_stderr);
    }
  } else {
    s_instance.reset(new Logger(filename, use_stderr));
  }

  s_instance->set_log_level(log_level);
}

Logger::Logger(const char *filename, bool use_stderr) {
  if (filename != nullptr) {
    m_log_file_name = filename;
#ifdef _WIN32
    const auto wide_filename = shcore::utf8_to_wide(filename);
    m_log_file.open(wide_filename, std::ios_base::app);
#else
    m_log_file.open(filename, std::ios_base::app);
#endif
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

Logger::LOG_LEVEL Logger::parse_log_level(const std::string &tag) {
  try {
    try {
      return get_level_by_name(tag);
    } catch (...) {
      int level = std::stoi(tag);
      if (level >= 1 && level <= LOG_MAX_LEVEL)
        return static_cast<LOG_LEVEL>(level);

      throw std::invalid_argument(get_level_range_info());
    }
  } catch (...) {
    throw std::invalid_argument(get_level_range_info());
  }
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
  return m_hook_list.end() !=
         std::find_if(m_hook_list.begin(), m_hook_list.end(),
                      [](const std::tuple<Log_hook, void *, bool> &h) {
                        return std::get<0>(h) == &Logger::out_to_stderr;
                      });
}

}  // namespace shcore
