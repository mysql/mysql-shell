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

#include "mysqlshdk/libs/utils/logger.h"

#include <rapidjson/document.h>
#include <stdarg.h>
#include <time.h>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else  // !_WIN32
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#endif  // !_WIN32

#include <algorithm>
#include <filesystem>
#include <ios>
#include <map>
#include <utility>

#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace fs = std::filesystem;

namespace shcore {

namespace {

/**
 * There are 3 mutexes that controll access to the Logger:
 *    - m_mutex_log_ctx (per instance): protects the acess to m_log_context
 *    - m_mutex_hooks (per instance): protects access to the hooks features:
 *        m_hook_list and m_level_hook_list
 *    - g_mutex (global): protects access to the (also global) variable
 *        g_output_format but also to the file system
 *
 * NOTE: m_mutex_hooks is locked while g_mutex is locked (check do_log)
 */
std::recursive_mutex g_mutex;
std::string g_output_format;

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

Logger::LOG_LEVEL get_level_by_name(std::string_view name) {
  if (str_caseeq(name, "none")) return Logger::LOG_NONE;
  if (str_caseeq(name, "internal")) return Logger::LOG_INTERNAL_ERROR;
  if (str_caseeq(name, "error")) return Logger::LOG_ERROR;
  if (str_caseeq(name, "warning")) return Logger::LOG_WARNING;
  if (str_caseeq(name, "info")) return Logger::LOG_INFO;
  if (str_caseeq(name, "debug")) return Logger::LOG_DEBUG;
  if (str_caseeq(name, "debug2")) return Logger::LOG_DEBUG2;
  if (str_caseeq(name, "debug3")) return Logger::LOG_DEBUG3;
  throw std::invalid_argument("invalid level");
}

std::string check_logfile_permissions(const std::string &filepath) {
  std::string ret_val;
  auto s = fs::status(filepath);

  if (fs::is_regular_file(s)) {
    auto perms = s.permissions();
    auto mask = perms & ~(fs::perms::owner_read | fs::perms::owner_write);
    if (mask != fs::perms::none) {
      std::stringstream warning;
      warning << "Permissions 0" << std::oct << static_cast<int>(perms)
              << " for log file \"" << filepath << "\" are too open.";

      ret_val = warning.str();
    }
  }
  return ret_val;
}

}  // namespace

Logger::Log_entry::Log_entry()
    : Log_entry(nullptr, nullptr, 0, LOG_LEVEL::LOG_NONE) {}

Logger::Log_entry::Log_entry(const char *domain_, const char *message_,
                             size_t length_, LOG_LEVEL level_)
    : timestamp{time(nullptr)},
      domain{domain_},
      message{message_},
      length{length_},
      level{level_} {}

void Logger::attach_log_hook(Log_hook hook, void *user_data, bool catch_all) {
  if (hook) {
    std::lock_guard l{m_mutex_hooks};
    m_hook_list.emplace_back(hook, user_data, catch_all);
  } else {
    throw std::invalid_argument("Logger::attach_log_hook: Null hook pointer");
  }
}

void Logger::detach_log_hook(Log_hook hook) {
  if (hook) {
    std::lock_guard l{m_mutex_hooks};
    m_hook_list.remove_if([hook](const std::tuple<Log_hook, void *, bool> &i) {
      return std::get<0>(i) == hook;
    });
  } else {
    throw std::invalid_argument("Logger::detach_log_hook: Null hook pointer");
  }
}

void Logger::attach_log_level_hook(Log_level_hook hook, void *user_data) {
  if (hook) {
    std::lock_guard l{m_mutex_hooks};
    m_level_hook_list.emplace_back(hook, user_data);
  } else {
    throw std::invalid_argument(
        "Logger::attach_log_level_hook: Null hook pointer");
  }
}

void Logger::detach_log_level_hook(Log_level_hook hook) {
  if (hook) {
    std::lock_guard l{m_mutex_hooks};
    m_level_hook_list.remove_if(
        [hook](const std::tuple<Log_level_hook, void *> &i) {
          return std::get<0>(i) == hook;
        });
  } else {
    throw std::invalid_argument(
        "Logger::detach_log_level_hook: Null hook pointer");
  }
}

bool Logger::log_allowed() const { return m_dont_log == 0; }

void Logger::set_log_level(LOG_LEVEL log_level) {
  m_log_level = log_level;

  std::lock_guard l{m_mutex_hooks};

  for (const auto &f : m_level_hook_list) {
    std::get<0>(f)(m_log_level, std::get<1>(f));
  }
}

void Logger::assert_logger_initialized() {
  if (current_logger().get() == nullptr) {
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
  mybuf.pop_back();  // remove '\0' appended by vsnprintf

  return mybuf;
}

void Logger::log(LOG_LEVEL level, const char *formats, ...) {
  if (auto logger = current_logger(); logger->will_log(level)) {
    va_list args;
    va_start(args, formats);
    const auto msg = format(formats, args);
    va_end(args);

    auto ctx = logger->context();
    Logger::do_log(*logger, {ctx.c_str(), msg.c_str(), msg.size(), level});
  }
}

void Logger::do_log(const Log_entry &entry) {
  Logger::do_log(*current_logger(), entry);
}

void Logger::do_log(const Logger &logger, const Log_entry &entry) {
  std::lock_guard lg{g_mutex};

  if (entry.level <= logger.m_log_level) {
#ifdef _WIN32
    if (logger.m_log_file.is_open()) {
      const auto s = format_message(entry);
      logger.m_log_file.write(s.c_str(), s.length());
      logger.m_log_file.flush();
    }
#else
    if (logger.m_log_file) {
      const auto s = format_message(entry);
      fwrite(s.c_str(), s.length(), 1, logger.m_log_file);
      fflush(logger.m_log_file);
    }
#endif
  }

  std::lock_guard lh{logger.m_mutex_hooks};

  for (const auto &f : logger.m_hook_list) {
    if (std::get<2>(f) || entry.level <= logger.m_log_level)
      std::get<0>(f)(entry, std::get<1>(f));
  }
}

bool Logger::will_log(LOG_LEVEL level) const {
  if (level <= m_log_level) return true;

  std::lock_guard l{m_mutex_hooks};

  for (const auto &hook : m_hook_list) {
    if (std::get<2>(hook)) return true;
  }

  return false;
}

void Logger::out_to_stderr(const Log_entry &entry, void *) {
  std::string msg;

  decltype(g_output_format) output_format;
  {
    std::lock_guard l{g_mutex};
    output_format = g_output_format;
  }

  if (std::string::npos != output_format.find("json")) {
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
                  rapidjson::StringRef(entry.message, entry.length), allocator);

    rapidjson::StringBuffer buffer;

    if ("json" == output_format) {
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

    static constexpr auto date_format = "%Y-%m-%d %H:%M:%S";
    strftime(timestamp, sizeof(timestamp), date_format, &tm);
  }

  // build return string
  std::string result;
  {
    result.reserve(512);
    result += timestamp;
#ifdef MYSQLSH_PRECISE_TIMESTAMP
    {
      timeval now;
      gettimeofday(&now, nullptr);
      result += "." + shcore::str_rjust(std::to_string(now.tv_usec), 6, '0');
    }
#endif  // MYSQLSH_PRECISE_TIMESTAMP
    result.append(": ");
    result += to_string(entry.level);
    result.append(": ");
    if (entry.domain && *entry.domain) {
      result += entry.domain;
      result.append(": ");
    }
    result += std::string(entry.message, entry.length);
    result += "\n";
  }

  return result;
}

std::shared_ptr<Logger> Logger::create_instance(const char *filename,
                                                bool use_stderr,
                                                LOG_LEVEL level) {
  std::shared_ptr<Logger> log(new Logger(filename, use_stderr));
  log->set_log_level(level);
  return log;
}

void Logger::log_to_stderr() {
  // attach hook only if not already logging to stderr
  if (!use_stderr()) {
    attach_log_hook(&Logger::out_to_stderr);
  }
}

void Logger::stop_log_to_stderr() { detach_log_hook(&Logger::out_to_stderr); }

Logger::Logger(const char *filename, bool use_stderr) : m_dont_log(0) {
  if (filename != nullptr) {
    m_log_file_name = filename;
#ifdef _WIN32
    const auto wide_filename = shcore::utf8_to_wide(filename);
    m_log_file.open(wide_filename, std::ios_base::app);
    if (m_log_file.fail()) {
      throw std::runtime_error(
          std::string("Error opening log file '") + filename +
          "' for writing: " + shcore::errno_to_string(errno));
    }
#else
    m_initialization_warning = check_logfile_permissions(m_log_file_name);
    if (!m_initialization_warning.empty()) {
      int rc = shcore::set_user_only_permissions(m_log_file_name);
      if (rc != 0) {
        m_initialization_warning +=
            " Failed adjusting permissions for user only access.";
      } else {
        m_initialization_warning +=
            " Permissions have been adjusted for user only access.";
      }
    }

    int fd = open(filename, O_CREAT | O_APPEND | O_WRONLY, S_IRUSR | S_IWUSR);
    if (fd < 0) {
      throw std::runtime_error(
          std::string("Error opening log file '") + filename +
          "' for writing: " + shcore::errno_to_string(errno));
    }
    m_log_file = fdopen(fd, "at");
    if (m_log_file == nullptr) {
      throw std::runtime_error(
          std::string("Error opening log file '") + filename +
          "' for writing: " + shcore::errno_to_string(errno));
    }
#endif
  }

  if (use_stderr) {
    attach_log_hook(&Logger::out_to_stderr);
  }
}

Logger::~Logger() {
#ifdef _WIN32
  if (m_log_file.is_open()) {
    m_log_file.close();
  }
#else
  if (m_log_file) {
    fclose(m_log_file);
  }
#endif
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
  std::lock_guard l{g_mutex};
  g_output_format = format;
}

std::string Logger::stderr_output_format() {
  std::lock_guard l{g_mutex};
  return g_output_format;
}

bool Logger::use_stderr() const {
  std::lock_guard l{m_mutex_hooks};
  return m_hook_list.end() !=
         std::find_if(m_hook_list.begin(), m_hook_list.end(),
                      [](const std::tuple<Log_hook, void *, bool> &h) {
                        return std::get<0>(h) == &Logger::out_to_stderr;
                      });
}

}  // namespace shcore
