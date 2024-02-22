/*
 * Copyright (c) 2014, 2024, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_UTILS_UTILS_GENERAL_H_
#define MYSQLSHDK_LIBS_UTILS_UTILS_GENERAL_H_

#include <charconv>
#include <chrono>
#include <functional>
#include <list>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>
#ifdef _WIN32
#include <io.h>
#include <windows.h>
#undef DELETE
#undef ERROR
#else
#include <unistd.h>
#endif

#include "mysqlshdk/include/scripting/naming_style.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/include/shellcore/ishell_core.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/ssh/ssh_connection_options.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace shcore {

/**
 * Calculate in compile time length/size of an array.
 *
 * @param array
 * @return Return size of an array.
 */
template <class T, std::size_t N>
constexpr static std::size_t array_size(const T (&)[N]) noexcept {
  return N;
}

class Scoped_callback {
 public:
  explicit Scoped_callback(std::function<void()> c) noexcept
      : m_callback{std::move(c)} {}

  Scoped_callback() = default;
  Scoped_callback(const Scoped_callback &) = delete;
  Scoped_callback &operator=(const Scoped_callback &) = delete;

  Scoped_callback(Scoped_callback &&o) noexcept { *this = std::move(o); }
  Scoped_callback &operator=(Scoped_callback &&o) noexcept {
    if (this != &o) std::swap(m_callback, o.m_callback);
    return *this;
  }

  ~Scoped_callback() noexcept {
    try {
      call();
    } catch (const std::exception &e) {
      log_error("Unexpected exception: %s", e.what());
    }
  }

  void call() {
    if (!m_callback) return;
    std::exchange(m_callback, nullptr)();
  }

  void cancel() { m_callback = nullptr; }

 private:
  std::function<void()> m_callback;
};

class Scoped_callback_list {
 public:
  Scoped_callback_list() = default;
  Scoped_callback_list(const Scoped_callback_list &) = delete;

  Scoped_callback_list(Scoped_callback_list &&o) noexcept {
    *this = std::move(o);
  }
  Scoped_callback_list &operator=(Scoped_callback_list &&o) noexcept {
    if (this != &o) {
      std::swap(m_callbacks, o.m_callbacks);
      std::swap(m_cancelled, o.m_cancelled);
    }
    return *this;
  }

  ~Scoped_callback_list() noexcept {
    try {
      call();
    } catch (const std::exception &e) {
      log_error("Unexpected exception: %s", e.what());
    }
  }

  void push_back(std::function<void()> c) noexcept {
    m_callbacks.push_back(std::move(c));
  }

  void push_front(std::function<void()> c) noexcept {
    m_callbacks.push_front(std::move(c));
  }

  void call() {
    if (m_cancelled) return;
    m_cancelled = true;  // can only call once (assume cancelled)

    try {
      for (const auto &cb : m_callbacks) cb();
    } catch (const std::exception &e) {
      log_error("Unexpected exception: %s", e.what());
      throw;
    }

    m_callbacks.clear();
  }

  void cancel() {
    m_cancelled = true;
    m_callbacks.clear();
  }

  bool empty() const { return m_callbacks.empty(); }

 private:
  std::list<std::function<void()>> m_callbacks;
  bool m_cancelled = false;
};

using on_leave_scope = Scoped_callback;

enum class OperatingSystem {
  UNKNOWN,
  DEBIAN,
  REDHAT,
  LINUX,
  WINDOWS,
  MACOS,
  SOLARIS
};
std::string SHCORE_PUBLIC to_string(OperatingSystem os_type);

bool SHCORE_PUBLIC is_valid_identifier(const std::string &name);
mysqlshdk::db::Connection_options SHCORE_PUBLIC
get_connection_options(const std::string &uri, bool set_defaults = true);
mysqlshdk::ssh::Ssh_connection_options SHCORE_PUBLIC
get_ssh_connection_options(const std::string &uri, bool set_defaults = true,
                           const std::string &config_path = "");

std::string SHCORE_PUBLIC get_system_user();

std::string SHCORE_PUBLIC strip_password(const std::string &connstring);

std::string SHCORE_PUBLIC strip_ssl_args(const std::string &connstring);

char SHCORE_PUBLIC *mysh_get_stdin_password(const char *prompt);

std::vector<std::string> SHCORE_PUBLIC split_string(std::string_view input,
                                                    std::string_view separator,
                                                    bool compress = false);
std::vector<std::string> SHCORE_PUBLIC
split_string_chars(std::string_view input, std::string_view separator_chars,
                   bool compress = false);

bool SHCORE_PUBLIC match_glob(const std::string_view pattern,
                              const std::string_view s,
                              bool case_sensitive = false);

std::string SHCORE_PUBLIC to_camel_case(const std::string &name);
std::string SHCORE_PUBLIC from_camel_case(const std::string &name);
std::string SHCORE_PUBLIC from_camel_case_to_dashes(const std::string &name);

std::string SHCORE_PUBLIC errno_to_string(int err);

struct Account {
  enum class Auto_quote {
    /**
     * No auto-quotes, string must be a valid account name.
     */
    NO,
    /**
     * Host will be auto-quoted, multiple unqouted '@' characters are NOT
     * allowed.
     */
    HOST,
    /**
     * String is a result of i.e. CURRENT_USER() function and is not quoted at
     * all. Host will be auto-quoted, multiple unqouted '@' characters are
     * allowed, the last one marks the beginning of the host name.
     */
    USER_AND_HOST,
  };

  std::string user;
  std::string host;

  bool operator<(const Account &a) const {
    return std::tie(user, host) < std::tie(a.user, a.host);
  }

  bool operator==(const Account &a) const {
    return user == a.user && host == a.host;
  }
};

void SHCORE_PUBLIC split_account(
    const std::string &account, std::string *out_user, std::string *out_host,
    Account::Auto_quote auto_quote = Account::Auto_quote::NO);
Account SHCORE_PUBLIC
split_account(const std::string &account,
              Account::Auto_quote auto_quote = Account::Auto_quote::NO);

template <typename C>
std::vector<Account> to_accounts(
    const C &c, Account::Auto_quote auto_quote = Account::Auto_quote::NO) {
  std::vector<Account> result;

  for (const auto &i : c) {
    result.emplace_back(split_account(i, auto_quote));
  }

  return result;
}

std::string SHCORE_PUBLIC make_account(const std::string &user,
                                       const std::string &host);
std::string SHCORE_PUBLIC make_account(const Account &account);

std::string SHCORE_PUBLIC get_member_name(std::string_view name,
                                          shcore::NamingStyle style);

/**
 * Ensures at most 2 identifiers are found on the string:
 *  - if 2 identifiers are found then they are set to schema and table
 *  - If 1 identifier is found it is set to table
 * Ensures the table name is not empty.
 */
void SHCORE_PUBLIC split_schema_and_table(const std::string &str,
                                          std::string *out_schema,
                                          std::string *out_table,
                                          bool allow_ansi_quotes = false);

/**
 * Ensures at most 3 identifiers are found on the string:
 *  - if 3 identifiers are found then they are set to schema, table and object
 *  - if 2 identifiers are found then they are set to table and object
 *  - if 1 identifier is found it is set to object
 * Ensures the object name is not empty.
 */
void SHCORE_PUBLIC split_schema_table_and_object(
    const std::string &str, std::string *out_schema, std::string *out_table,
    std::string *out_object, bool allow_ansi_quotes = false);

void SHCORE_PUBLIC split_priv_level(const std::string &str,
                                    std::string *out_schema,
                                    std::string *out_object,
                                    size_t *out_leftover = nullptr);

std::string SHCORE_PUBLIC unquote_identifier(const std::string &str,
                                             bool allow_ansi_quotes = false);

std::string SHCORE_PUBLIC unquote_sql_string(const std::string &str);

/** Substitute variables in string.
 *
 * str_subvar("hello ${foo}",
 *        [](const std::string&) { return "world"; },
 *        "${", "}");
 *    --> "hello world";
 *
 * str_subvar("hello $foo!",
 *        [](const std::string&) { return "world"; },
 *        "$", "");
 *    --> "hello world!";
 *
 * If var_end is "", then the variable name will span until the
 */
std::string SHCORE_PUBLIC str_subvars(
    std::string_view s,
    const std::function<std::string(std::string_view)> &subvar =
        [](std::string_view var) {
          return shcore::get_member_name(var, shcore::current_naming_style());
        },
    std::string_view var_begin = "<<<", std::string_view var_end = ">>>");

void SHCORE_PUBLIC sleep_ms(uint32_t ms);
void SHCORE_PUBLIC sleep(std::chrono::milliseconds duration);

OperatingSystem SHCORE_PUBLIC get_os_type();

/**
 * Provides host CPU type, i.e. aarch64, x86_64, etc.
 *
 * @return machine type
 */
std::string SHCORE_PUBLIC get_machine_type();

/**
 * Provides long version of mysqlsh, including version number, OS type, MySQL
 * version number and build type.
 *
 * @return version string
 */
const char *SHCORE_PUBLIC get_long_version();

#ifdef _WIN32

std::string SHCORE_PUBLIC last_error_to_string(DWORD code);

#endif  // _WIN32

template <class T>
auto lexical_cast(T &&data) noexcept {
  return std::forward<T>(data);
}

template <class T>
auto lexical_cast(std::string_view str) {
  // this version converts from string to something else

  // same behavior as boost: doesn't make sense so convert back to these types
  static_assert(!std::is_same_v<std::remove_cv_t<T>, char *> &&
                    !std::is_same_v<std::remove_cv_t<T>, char> &&
                    !std::is_same_v<std::remove_cv_t<T>, std::string_view>,
                "Invalid type to convert to.");

  if constexpr (std::is_same_v<T, std::string>) {
    // simple copies are allowed
    return T{str};

  } else if constexpr (std::is_same<T, bool>::value) {
    // conversion to bool
    if (shcore::str_caseeq(str, "true")) return true;
    if (shcore::str_caseeq(str, "false")) return false;

    std::stringstream ss;
    ss << str;
    if (std::is_unsigned<T>::value && ss.peek() == '-')
      throw std::invalid_argument("Unable to perform conversion.");
    T t;
    ss >> t;
    if (ss.fail()) throw std::invalid_argument("Unable to perform conversion.");
    if (!ss.eof())
      throw std::invalid_argument("Conversion did not consume whole input.");

    return t;

  } else if constexpr (std::is_integral_v<T>) {
    // some compilers don't have FP implementations of std::from_chars (GCC and
    // Clang) so it can only be used with integrals
    T value;
    auto [ptr, ec] =
        std::from_chars(str.data(), str.data() + str.size(), value);

    if (ec != std::errc())
      throw std::invalid_argument("Unable to perform conversion.");
    if (ptr != (str.data() + str.size()))
      throw std::invalid_argument("Conversion did not consume whole input.");
    return value;

  } else {
    // all other cases
    std::stringstream ss;
    ss << str;
    if (std::is_unsigned<T>::value && ss.peek() == '-')
      throw std::invalid_argument("Unable to perform conversion.");
    T t;
    ss >> t;
    if (ss.fail()) throw std::invalid_argument("Unable to perform conversion.");
    if (!ss.eof())
      throw std::invalid_argument("Conversion did not consume whole input.");

    return t;
  }
}

template <class T, class S>
auto lexical_cast(S &&data) {
  // lexical_cast doesn't allow to convert to "char*" or std::string_view
  static_assert(!std::is_same_v<T, char *> &&
                !std::is_same_v<T, const char *> &&
                !std::is_same_v<T, std::string_view>);

  // lexical_cast should be to / from strings, so at least T or S must be one
  static_assert(std::is_same_v<std::decay_t<S>, char *> ||
                std::is_same_v<std::decay_t<S>, const char *> ||
                std::is_same_v<std::decay_t<S>, std::string> ||
                std::is_same_v<T, std::string>);
  /*
   - if T and S are the same, forward S
   - if S is a string, use the specialization for strings (above this one)
   - if T is a string, convert it here
   - every other combination is "translated" through a std::stringstream
  */
  if constexpr (std::is_same_v<std::decay_t<S>, T>) {
    // if T and S match, just return the parameter
    return std::forward<T>(data);
  } else if constexpr (std::is_same_v<std::decay_t<S>, char *> ||
                       std::is_same_v<std::decay_t<S>, const char *> ||
                       std::is_same_v<std::decay_t<S>, std::string>) {
    // if S is char* or std::string (const or non-const) then use
    // std::string_view version
    return lexical_cast<T>(std::string_view{data});

  } else {
    // convert from S (which is not char*, std::string or std::string_view) to
    // std::string
    if constexpr (std::is_same_v<std::decay_t<S>, bool>) {
      return std::string{data ? "true" : "false"};

    } else if constexpr (std::is_integral_v<std::decay_t<S>>) {
      std::array<char, 64> str;

      auto [ptr, ec] = std::to_chars(str.data(), str.data() + str.size(), data);
      if (ec == std::errc()) return std::string{str.data(), ptr};

      throw std::invalid_argument("Unable to perform conversion.");

    } else {
      std::stringstream ss;
      ss << data;
      return ss.str();
    }
  }
}

template <class T, class S>
auto lexical_cast(S &&data, T default_value) noexcept {
  try {
    return lexical_cast<T>(std::forward<S>(data));
  } catch (...) {
  }
  return default_value;
}

/**
 * Wrapper for the std::getline() function which removes the last character
 * if it's carriage return.
 */
std::istream &getline(std::istream &in, std::string &out);

/**
 * Verifies the status code of an application.
 *
 * @param status - status code to be checked
 * @param error - if execution was not successful, contains details on
 *                corresponding error
 *
 * @returns true if status code corresponds to a successful execution of an
 * application.
 */
bool verify_status_code(int status, std::string *error);

/**
 * Sets the environment variable 'name' to the value of 'value'.
 * If value is null or empty, variable is removed instead.
 *
 * @param name - name of the environment variable to set
 * @param value - value of the environment variable
 *
 * @returns true if the environment variable was set successfully.
 */
bool setenv(const char *name, const char *value);
bool setenv(const char *name, const std::string &value);
bool setenv(const std::string &name, const std::string &value);

/**
 * Sets the environment variable, string must be in form: name=value.
 * If value is empty, variable is removed instead.
 *
 * @param name_value - name and the new value of the environment variable
 *
 * @returns true if the environment variable was set successfully.
 */
bool setenv(const std::string &name_value);

/**
 * Clears the environment variable called 'name'.
 *
 * @param name - name of the environment variable to clear
 *
 * @returns true if the environment variable was cleared successfully.
 */
bool unsetenv(const char *name);
bool unsetenv(const std::string &name);

#ifdef _WIN32
#define STDIN_FILENO _fileno(stdin)
#define STDOUT_FILENO _fileno(stdout)
#define isatty _isatty
#endif

}  // namespace shcore

#endif  // MYSQLSHDK_LIBS_UTILS_UTILS_GENERAL_H_
