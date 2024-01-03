/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/yparser/parser.h"

#ifdef _WIN32
#include <Windows.h>
#else  // !_WIN32
#include <dlfcn.h>
#endif  // !_WIN32

#include <cassert>
#include <limits>
#include <map>
#include <regex>
#include <unordered_map>
#include <utility>

#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "mysqlshdk/libs/yparser/mysql/parser.h"

namespace mysqlshdk {
namespace yacc {

namespace {

using Version = utils::Version;

namespace api {

#ifdef _WIN32

using Handle = HMODULE;

Handle dl_open(const std::string &path) {
  const auto wpath = shcore::utf8_to_wide(path);
  return LoadLibraryExW(wpath.c_str(), nullptr,
                        LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
}

auto dlsym(Handle handle, const char *symbol) {
  return GetProcAddress(handle, symbol);
}

bool dl_close(Handle handle) { return FALSE != FreeLibrary(handle); }

std::string dl_error() { return shcore::last_error_to_string(GetLastError()); }

#else  // !_WIN32

using Handle = void *;

Handle dl_open(const std::string &path) {
  return dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
}

bool dl_close(Handle handle) { return 0 == dlclose(handle); }

std::string dl_error() {
  const auto error = dlerror();
  return error ? error : "the dlerror() function returned no error diagnostic";
}

#endif  // !_WIN32

#define PARSER_SYMBOLS         \
  X(parser_version)            \
  X(parser_create)             \
  X(parser_destroy)            \
  X(parser_set_sql_mode)       \
  X(parser_check_syntax)       \
  X(parser_pending_errors)     \
  X(parser_next_error)         \
  X(parser_error_message)      \
  X(parser_error_token_offset) \
  X(parser_error_token_length) \
  X(parser_error_line)         \
  X(parser_error_line_offset)

class Lib final {
 public:
  Lib() = delete;

  explicit Lib(const std::string &path) {
    m_handle = dl_open(path);

    if (!m_handle) {
      throw std::runtime_error("Failed to load parser plugin '" + path +
                               "': " + dl_error());
    }

#define X(symbol)                                                              \
  symbol = reinterpret_cast<decltype(&::symbol)>(dlsym(m_handle, #symbol));    \
  if (!symbol) {                                                               \
    throw std::runtime_error("Failed to load symbol '" +                       \
                             std::string(#symbol) + "' from parser plugin '" + \
                             path + "': " + dl_error());                       \
  }
    PARSER_SYMBOLS
#undef X
  }

  Lib(const Lib &) = delete;
  Lib(Lib &&) = default;

  Lib &operator=(const Lib &) = delete;
  Lib &operator=(Lib &&) = default;

  ~Lib() {
    if (m_handle) {
      if (!dl_close(m_handle)) {
        log_error("Failed to unload parser plugin: %s", dl_error().c_str());
      }
    }
  }

#define X(symbol) decltype(&::symbol) symbol;
  PARSER_SYMBOLS
#undef X

 private:
  Handle m_handle;
};

#undef PARSER_SYMBOLS

}  // namespace api

class Loader final {
 public:
  Loader() {
#ifdef _WIN32
#define PLUGIN_EXT "dll"
#else  // !_WIN32
#define PLUGIN_EXT "so"
#endif  // !_WIN32
    const std::regex pattern{R"(mysql_(\d+\.\d+)\.)" PLUGIN_EXT};
#undef PLUGIN_EXT
    const auto dir =
        shcore::path::join_path(shcore::get_library_folder(), "yparser");

#ifdef _WIN32
    AddDllDirectory(shcore::utf8_to_wide(dir).c_str());
#endif  // _WIN32

    std::smatch match_result;

    shcore::iterdir(dir, [&, this](const std::string &name) {
      if (std::regex_match(name, match_result, pattern)) {
        const auto &match = match_result[1];
        // std::string_view(Iter, Iter) constructor requires GCC11+, this is
        // essentially the same
        m_plugins.emplace(
            Version{std::string_view{
                std::to_address(match.first),
                static_cast<std::size_t>(match.second - match.first)}},
            shcore::path::join_path(dir, name));
      }

      return true;
    });

    if (m_plugins.empty()) {
      throw std::runtime_error{"No parser plugins found in '" + dir + "'"};
    }
  }

  Loader(const Loader &) = delete;
  Loader(Loader &&) = delete;

  Loader &operator=(const Loader &) = delete;
  Loader &operator=(Loader &&) = delete;

  ~Loader() = default;

  std::unique_ptr<api::Lib> api(const Version &version) {
    // we're relying on the OS to keep track of the reference count
    return std::make_unique<api::Lib>(select_plugin(version));
  }

 private:
  const std::string &select_plugin(const Version &version) const {
    const Version major_minor{version.get_major(), version.get_minor()};

    for (const auto &plugin : m_plugins) {
      if (major_minor <= plugin.first) {
        return plugin.second;
      }
    }

    // requested version is greater than any that we support, use the newest one
    return m_plugins.crbegin()->second;
  }

  std::map<Version, std::string> m_plugins;
};

Loader &loader() {
  static Loader s_loader;
  return s_loader;
}

}  // namespace

Syntax_error::Syntax_error(const char *what, std::size_t token_offset,
                           std::size_t token_length, std::size_t line,
                           std::size_t line_offset)
    : Parser_error(what),
      m_token_offset(token_offset),
      m_token_length(token_length),
      m_line(line),
      m_line_offset(line_offset) {}

[[nodiscard]] std::string Syntax_error::format() const {
  return shcore::str_format("%zu:%zu: %s", m_line + 1, m_line_offset + 1,
                            what());
}

class Parser::Impl final {
 public:
  Impl() = delete;

  explicit Impl(const Version &version) {
    m_api = loader().api(version);

    if (Parser_result::PARSER_RESULT_OK != m_api->parser_create(&m_parser)) {
      throw std::runtime_error("Failed to create " + version.get_base() +
                               " parser");
    }

    m_version = Version{m_api->parser_version()};
  }

  Impl(const Impl &) = delete;
  Impl(Impl &&) = default;

  Impl &operator=(const Impl &) = delete;
  Impl &operator=(Impl &&) = default;

  ~Impl() { m_api->parser_destroy(m_parser); }

  [[nodiscard]] const Version &version() const noexcept { return m_version; }

  void set_sql_mode(std::string_view sql_mode) {
    if (Parser_result::PARSER_RESULT_OK !=
        m_api->parser_set_sql_mode(m_parser, sql_mode.data(),
                                   sql_mode.length())) {
      throw parser_error("Failed to set sql_mode");
    }
  }

  [[nodiscard]] std::vector<Syntax_error> check_syntax(
      std::string_view statement) const {
    if (Parser_result::PARSER_RESULT_OK ==
        m_api->parser_check_syntax(m_parser, statement.data(),
                                   statement.length())) {
      return {};
    }

    auto errors = fetch_errors();
    assert(!errors.empty());

    if (k_invalid_line == errors.front().line()) {
      throw parser_error("Failed to check syntax", std::move(errors));
    }

    return errors;
  }

 private:
  [[nodiscard]] std::vector<Syntax_error> fetch_errors() const {
    std::vector<Syntax_error> errors;

    if (const auto pending = m_api->parser_pending_errors(m_parser)) {
      errors.reserve(pending);

      Parser_error_h handle = PARSER_INVALID_HANDLE;

      while (PARSER_INVALID_HANDLE !=
             (handle = m_api->parser_next_error(m_parser))) {
        errors.emplace_back(m_api->parser_error_message(handle),
                            m_api->parser_error_token_offset(handle),
                            m_api->parser_error_token_length(handle),
                            m_api->parser_error_line(handle) - 1,
                            m_api->parser_error_line_offset(handle) - 1);
      }
    } else {
      errors.emplace_back("unknown error", 0, 0, k_invalid_line,
                          k_invalid_line);
    }

    return errors;
  }

  [[nodiscard]] Parser_error parser_error(
      std::string msg, std::vector<Syntax_error> errors = {}) const {
    if (errors.empty()) {
      errors = fetch_errors();
    }

    msg += ':';

    for (const auto &error : errors) {
      msg += ' ';
      msg += error.what();
      msg += ',';
    }

    // remove last comma
    msg.pop_back();

    return Parser_error{msg};
  }

  static constexpr auto k_invalid_line =
      std::numeric_limits<std::size_t>::max();

  std::unique_ptr<api::Lib> m_api;
  Parser_h m_parser = PARSER_INVALID_HANDLE;
  Version m_version;
};

Parser::~Parser() = default;

Parser::Parser(const Version &version)
    : m_impl(std::make_unique<Impl>(version)) {}

[[nodiscard]] const Version &Parser::version() const noexcept {
  return m_impl->version();
}

void Parser::set_sql_mode(std::string_view sql_mode) {
  m_impl->set_sql_mode(sql_mode);
}

[[nodiscard]] std::vector<Syntax_error> Parser::check_syntax(
    std::string_view statement) const {
  return m_impl->check_syntax(statement);
}

}  // namespace yacc
}  // namespace mysqlshdk
