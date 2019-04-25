/*
 * Copyright (c) 2014, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_LIBS_UTILS_UTILS_GENERAL_H_
#define MYSQLSHDK_LIBS_UTILS_UTILS_GENERAL_H_

#include <functional>
#include <set>
#include <sstream>
#include <string>
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

#include "mysqlshdk/include/shellcore/ishell_core.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "scripting/types.h"
#include "scripting/types_cpp.h"

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
  explicit Scoped_callback(std::function<void()> c) : callback(c) {}

  ~Scoped_callback() { callback(); }

 private:
  std::function<void()> callback;
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
void SHCORE_PUBLIC update_connection_data(
    mysqlshdk::db::Connection_options *connection_options,
    const std::string &user, const char *password, const std::string &host,
    int port, const mysqlshdk::utils::nullable<std::string> &sock,
    const std::string &database, const mysqlshdk::db::Ssl_options &ssl_options,
    const std::string &auth_method, bool get_server_public_key,
    const std::string &server_public_key_path,
    const std::string &connect_timeout, bool compression);

std::string SHCORE_PUBLIC get_system_user();

std::string SHCORE_PUBLIC strip_password(const std::string &connstring);

std::string SHCORE_PUBLIC strip_ssl_args(const std::string &connstring);

char SHCORE_PUBLIC *mysh_get_stdin_password(const char *prompt);

std::vector<std::string> SHCORE_PUBLIC
split_string(const std::string &input, const std::string &separator,
             bool compress = false);
std::vector<std::string> SHCORE_PUBLIC
split_string_chars(const std::string &input, const std::string &separator_chars,
                   bool compress = false);

bool SHCORE_PUBLIC match_glob(const std::string &pattern, const std::string &s,
                              bool case_sensitive = false);

std::string SHCORE_PUBLIC to_camel_case(const std::string &name);
std::string SHCORE_PUBLIC from_camel_case(const std::string &name);

std::string SHCORE_PUBLIC errno_to_string(int err);

void SHCORE_PUBLIC split_account(const std::string &account,
                                 std::string *out_user, std::string *out_host,
                                 bool auto_quote_hosts = false);
std::string SHCORE_PUBLIC make_account(const std::string &user,
                                       const std::string &host);

std::string SHCORE_PUBLIC get_member_name(const std::string &name,
                                          shcore::NamingStyle style);

void clear_buffer(char *buffer, size_t size);

void SHCORE_PUBLIC sleep_ms(uint32_t ms);

OperatingSystem SHCORE_PUBLIC get_os_type();

/**
 * Provides long version of mysqlsh, including version number, OS type, MySQL
 * version number and build type.
 *
 * @return version string
 */
const char *SHCORE_PUBLIC get_long_version();

#ifdef _WIN32

std::string SHCORE_PUBLIC last_error_to_string(DWORD code);

// We inline these functions to avoid trouble with memory and DLL boundaries
inline std::string win_w_to_a_string(const std::wstring &wstr, int wstrl) {
  std::string str;

  // No ceonversion needed at all
  if (!wstr.empty()) {
    // Calculates the required output buffer size
    // Since -1 is used as input length, the null termination character will be
    // included on the length calculation
    int r = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, &wstr[0], -1,
                                nullptr, 0, nullptr, nullptr);

    if (r > 0) {
      str = std::string(r, 0);
      // Copies the buffer, since -1 is passed as input buffer length this
      // includes the null termination character
      r = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, &wstr[0], -1,
                              &str[0], r, nullptr, nullptr);
    }
  }

  return str;
}

inline std::wstring win_a_to_w_string(const std::string &str) {
  std::wstring wstr;

  // No conversion needed
  if (!str.empty()) {
    // Calculates the required output buffer size
    // Since -1 is used as input length, the null termination character will be
    // included on the length calculation
    int r = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, &str[0], -1,
                                nullptr, 0);

    if (r > 0) {
      wstr = std::wstring(r, 0);
      r = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, &str[0], -1,
                              &wstr[0], r);
    }
  }
  return wstr;
}

class Sigint_event final {
 public:
  ~Sigint_event();

  Sigint_event(const Sigint_event &) = delete;
  Sigint_event(Sigint_event &&) = delete;
  Sigint_event &operator=(const Sigint_event &) = delete;
  Sigint_event &operator=(Sigint_event &&) = delete;

  static Sigint_event &get();

  void notify();

  void wait(uint32_t ms);

 private:
  Sigint_event();

  HANDLE m_event;
};

#endif  // _WIN32

template <class T>
T lexical_cast(const T &data) {
  return data;
}

template <class T, class S>
typename std::enable_if<std::is_same<T, std::string>::value, T>::type
lexical_cast(const S &data) {
  std::stringstream ss;
  if (std::is_same<S, bool>::value) ss << std::boolalpha;
  ss << data;
  return ss.str();
}

template <class T, class S>
typename std::enable_if<!std::is_same<T, std::string>::value, T>::type
lexical_cast(const S &data) {
  std::stringstream ss;
  ss << data;
  if (std::is_unsigned<T>::value && ss.peek() == '-')
    throw std::invalid_argument("Unable to perform conversion.");
  T t;
  ss >> t;
  if (ss.fail()) {
    if (std::is_same<T, bool>::value) {
      ss.clear();
      ss >> std::boolalpha >> t;
    }
    if (ss.fail()) throw std::invalid_argument("Unable to perform conversion.");
  } else if (!ss.eof()) {
    throw std::invalid_argument("Conversion did not consume whole input.");
  }
  return t;
}

/**
 * Wrapper for the std::getline() function which removes the last character
 * if it's carriage return.
 */
std::istream &getline(std::istream &in, std::string &out);

/**
 * Temporary implementation for make_unique
 * Should be deleted when we migrate to C++14
 */
template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args &&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

#ifdef _WIN32
#define STDIN_FILENO _fileno(stdin)
#define STDOUT_FILENO _fileno(stdout)
#define isatty _isatty
#endif

}  // namespace shcore

#endif  // MYSQLSHDK_LIBS_UTILS_UTILS_GENERAL_H_
