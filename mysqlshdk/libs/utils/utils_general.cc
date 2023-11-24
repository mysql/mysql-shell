/*
 * Copyright (c) 2014, 2023, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/utils/utils_general.h"

#include <string_view>
#include <utility>

#include "mysqlshdk/libs/textui/textui.h"

#include "include/mysh_config.h"
#include "my_config.h"

#ifdef WIN32
#include <Lmcons.h>
#else
#include <unistd.h>
#ifdef HAVE_GETPWUID_R
#include <pwd.h>
#endif

#if defined HAVE_EXPLICIT_BZERO
#if defined HAVE_BSD_STRING_H
#include <bsd/string.h>
#else
#include <strings.h>
#endif
#elif defined HAVE_MEMSET_S
#include <string.h>
extern "C" {
errno_t memset_s(void *__s, rsize_t __smax, int __c, rsize_t __n);
}
#endif
#endif

#include <cctype>
#include <cstdio>
#include <ctime>
#include <locale>

#include "mysqlshdk/include/shellcore/interrupt_handler.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/db/uri_parser.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_lexing.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "shellcore/utils_help.h"

#include <mysql_version.h>

namespace shcore {

bool compare_floating_point(double valueA, double valueB,
                            double precision) noexcept {
  return (std::abs(valueA - valueB) < precision);
}

bool is_valid_identifier(std::string_view name) {
  if (name.empty()) return false;

  std::locale locale;

  if (!std::isalpha(name.front(), locale) && (name.front() != '_'))
    return false;

  return std::all_of(
      std::next(name.begin()), name.end(), [&locale](auto cur_char) {
        return std::isalnum(cur_char, locale) || (cur_char == '_');
      });
}

std::string strip_password(std::string_view connstring) {
  auto remaining = connstring;
  std::string_view scheme;

  auto p = remaining.find("://");
  if (p != std::string_view::npos) {
    scheme = remaining.substr(0, p + 3);
    remaining = remaining.substr(p + 3);
  }

  auto s = remaining;
  p = remaining.find('/');
  if (p != std::string::npos) {
    s = remaining.substr(0, p);
  }
  p = s.rfind('@');

  std::string_view user_part;
  if (p != std::string_view::npos) {
    user_part = s.substr(0, p);
  }

  if ((p = user_part.find(':')) != std::string_view::npos) {
    auto password = std::string{user_part.substr(p)};

    auto uri_stripped = std::string{remaining};
    auto i = uri_stripped.find(password);
    if (i != std::string::npos) uri_stripped.erase(i, password.length());

    return std::string{scheme} + uri_stripped;
  }

  // no password to strip, return original one
  return std::string{connstring};
}

/*std::string strip_ssl_args(std::string_view connstring) {
  std::string result = connstring;
  std::string::size_type pos;
  if ((pos = result.find("ssl_ca=")) != std::string::npos) {
    std::string::size_type pos2 = result.find("&");
    result = result.replace(pos, (pos2 == std::string::npos) ? std::string::npos
: pos2 - pos + 1, "");
  }
  if ((pos = result.find("ssl_cert=")) != std::string::npos) {
    std::string::size_type pos2 = result.find("&");
    result = result.replace(pos, (pos2 == std::string::npos) ? std::string::npos
: pos2 - pos + 1, "");
  }
  if ((pos = result.find("ssl_key=")) != std::string::npos) {
    std::string::size_type pos2 = result.find("&");
    result = result.replace(pos, (pos2 == std::string::npos) ? std::string::npos
: pos2 - pos + 1, "");
  }
  if (result.at(result.size() - 1) == '?') {
    result.resize(result.size() - 1);
  }

  return result;
}*/

char *mysh_get_stdin_password(const char *prompt) {
  if (prompt) {
    fputs(prompt, stdout);
    fflush(stdout);
  }
  char buffer[128];
  if (fgets(buffer, sizeof(buffer), stdin)) {
    char *p = strchr(buffer, '\r');
    if (p) *p = 0;
    p = strchr(buffer, '\n');
    if (p) *p = 0;
    return strdup(buffer);
  }
  return NULL;
}

// Builds a connection data dictionary using the URI
mysqlshdk::db::Connection_options get_connection_options(const std::string &uri,
                                                         bool set_defaults) {
  mysqlshdk::db::Connection_options connection_options(uri);

  if (set_defaults) connection_options.set_default_data();

  return connection_options;
}

mysqlshdk::ssh::Ssh_connection_options get_ssh_connection_options(
    const std::string &uri, bool set_defaults, const std::string &config_path) {
  mysqlshdk::ssh::Ssh_connection_options config(uri);
  if (!config_path.empty()) {
    config.set_config_file(config_path);
  }

  if (set_defaults) config.set_default_data();

  return config;
}

std::string get_system_user() {
  std::string ret_val{"UNKNOWN_USER"};

#ifdef _WIN32
  wchar_t username[UNLEN + 1] = {};
  DWORD username_len = UNLEN + 1;
  if (GetUserNameW(username, &username_len) != 0) {
    ret_val = shcore::wide_to_utf8(username, username_len - 1);
  }
#else
  if (geteuid() == 0) {
    ret_val = "root"; /* allow use of surun */
  } else {
#if defined(HAVE_GETPWUID_R) && defined(HAVE_GETLOGIN_R)
    auto name_size = sysconf(_SC_LOGIN_NAME_MAX);
    char *name = reinterpret_cast<char *>(malloc(name_size));
    if (!getlogin_r(name, name_size)) {
      ret_val.assign(name);
    } else {
      auto buffer_size = sysconf(_SC_GETPW_R_SIZE_MAX);
      char *buffer = reinterpret_cast<char *>(malloc(buffer_size));
      struct passwd pwd;
      struct passwd *res;

      if (!getpwuid_r(geteuid(), &pwd, buffer, buffer_size, &res) && res) {
        ret_val.assign(pwd.pw_name);
      } else {
        char *str;
        if ((str = getenv("USER")) || (str = getenv("LOGNAME")) ||
            (str = getenv("LOGIN"))) {
          ret_val.assign(str);
        } else {
          ret_val = "UNKNOWN_USER";
        }
      }
      free(buffer);
    }

    free(name);
#elif HAVE_CUSERID
    char username[L_cuserid];
    if (cuserid(username)) ret_val.assign(username);
#endif
  }
#endif

  return ret_val;
}

std::string errno_to_string(int err) {
  if (!err) return std::string();

#ifdef _WIN32
#define strerror_r(E, B, S) strerror_s(B, S, E)
#endif

#if defined(_WIN32) || defined(__sun) || defined(__APPLE__) || \
    ((_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) &&   \
     !_GNU_SOURCE)  // NOLINT
  char buf[256];
  if (!strerror_r(err, buf, sizeof(buf))) return std::string(buf);
  return std::string();
#else
  char buf[256];
  return strerror_r(err, buf, sizeof(buf));
#endif
}

std::vector<std::string> split_string(std::string_view input,
                                      std::string_view separator,
                                      bool compress) {
  std::vector<std::string> ret_val;

  size_t index = 0, new_find = 0;

  while (new_find != std::string_view::npos) {
    new_find = input.find(separator, index);

    if (new_find != std::string_view::npos) {
      // When compress is enabled, consecutive separators
      // do not generate new elements
      if (new_find > index || !compress || new_find == 0)
        ret_val.emplace_back(input.substr(index, new_find - index));

      index = new_find + separator.length();
    } else {
      ret_val.emplace_back(input.substr(index));
    }
  }

  return ret_val;
}

/**
 * Splits string based on each of the individual characters of the separator
 * string
 *
 * @param input The string to be split
 * @param separator_chars String containing characters wherein the input string
 * is split on any of the characters
 * @param compress Boolean value which when true ensures consecutive separators
 * do not generate new elements in the split
 *
 * @returns vector of splitted strings
 */
std::vector<std::string> split_string_chars(std::string_view input,
                                            std::string_view separator_chars,
                                            bool compress) {
  std::vector<std::string> ret_val;

  size_t index = 0, new_find = 0;

  while (new_find != std::string_view::npos) {
    new_find = input.find_first_of(separator_chars, index);

    if (new_find != std::string_view::npos) {
      // When compress is enabled, consecutive separators
      // do not generate new elements
      if (new_find > index || !compress || new_find == 0)
        ret_val.emplace_back(input.substr(index, new_find - index));

      index = new_find + 1;
    } else {
      ret_val.emplace_back(input.substr(index));
    }
  }

  return ret_val;
}

// Retrieves a member name on a specific NamingStyle
// NOTE: Assumption is given that everything is created using a lowerUpperCase
// naming style
//       Which is the default to be used on C++ and JS
std::string get_member_name(std::string_view name, shcore::NamingStyle style) {
  switch (style) {
    // This is the default style, input is returned without modifications
    case shcore::LowerCamelCase:
      return std::string{name};
    case shcore::LowerCaseUnderscores: {
      // Uppercase letters will be converted to underscore+lowercase letter
      // except in two situations:
      // - When it is the first letter
      // - When an underscore is already before the uppercase letter
      std::string new_name;
      new_name.reserve(name.size());

      bool skip_underscore = true;
      for (auto character : name) {
        if (character >= 65 && character <= 90) {
          if (!skip_underscore)
            new_name.append(1, '_');
          else
            skip_underscore = false;

          new_name.append(1, character + 32);
        } else {
          // if character is '_'
          skip_underscore = character == 95;

          new_name.append(1, character);
        }
      }
      return new_name;
    }
    case Constants: {
      std::string new_name{name};

      for (auto &character : new_name) {
        if (character >= 97 && character <= 122) character -= 32;
      }

      return new_name;
    }
  }

  return {};
}

/** Convert string from under_score/dash naming convention to camelCase

  As a special case, if string is longer than 2 characters and
  all characters are uppercase, conversion will be skipped.
  */
std::string to_camel_case(std::string_view name) {
  if (name.empty()) return {};

  std::string new_name;
  new_name.reserve(name.size());

  bool upper_next = false;
  size_t upper_count = 0;
  for (auto ch : name) {
    if (isupper(ch)) upper_count++;
    if (ch == '_' || ch == '-') {
      upper_count++;
      upper_next = true;
    } else if (upper_next) {
      upper_next = false;
      new_name.push_back(toupper(ch));
    } else {
      new_name.push_back(ch);
    }
  }

  if (upper_count == name.length()) return std::string{name};
  return new_name;
}

/** Convert string from camcelCase naming convention to under_score

  As a special case, if string is longer than 2 characters and
  all characters are uppercase, conversion will be skipped.
  */
std::string from_camel_case(std::string_view name) {
  if (name.empty()) return {};

  std::string new_name;
  new_name.reserve(name.size());

  size_t upper_count = 0;
  // Uppercase letters will be converted to underscore+lowercase letter
  // except in two situations:
  // - When it is the first letter
  // - When an underscore or a dash is already before the uppercase letter
  bool skip_underscore = true;
  for (auto character : name) {
    if (isupper(character)) {
      upper_count++;
      if (!skip_underscore)
        new_name.append(1, '_');
      else
        skip_underscore = false;
      new_name.append(1, tolower(character));
    } else {
      // if character is '_'
      skip_underscore = character == '_' || character == '-';
      if (skip_underscore) upper_count++;
      new_name.append(1, character);
    }
  }

  if (upper_count == name.length()) return std::string{name};
  return new_name;
}

std::string from_camel_case_to_dashes(std::string_view name) {
  return str_replace(from_camel_case(name), "_", "-");
}

static std::size_t span_quotable_identifier(const std::string &s, std::size_t p,
                                            std::string *out_string,
                                            bool allow_ansi_quotes = false) {
  if (p >= s.length()) return p;

  const auto copy_current_character = [&s, &p, out_string]() {
    if (out_string) out_string->push_back(s[p]);
  };

  if ('`' == s[p] || (allow_ansi_quotes && '"' == s[p])) {
    const char quote = s[p++];
    bool esc = false;
    bool done = false;

    while (!done && p < s.size()) {
      if (quote == s[p]) {
        if (esc) {
          copy_current_character();
          esc = false;
        } else {
          esc = true;
        }
      } else {
        if (esc) {
          done = true;
          break;
        } else {
          copy_current_character();
        }
      }

      ++p;
    }

    // was the last character a quote?
    if (!done && esc) {
      done = true;
    }

    if (!done) {
      throw std::runtime_error("Invalid syntax in identifier");
    }
  } else {
    const auto first = p;
    bool seen_not_a_digit = false;

    while (p < s.size()) {
      if (!std::isalnum(s[p]) && s[p] != '_' && s[p] != '$') {
        if (first == p) {
          throw std::runtime_error("Invalid character in identifier");
        } else {
          break;
        }
      }

      copy_current_character();

      if (!seen_not_a_digit && !isdigit(s[p])) seen_not_a_digit = true;

      ++p;
    }

    if (!seen_not_a_digit) {
      throw std::runtime_error(
          "Invalid identifier: identifiers may begin with a digit but unless "
          "quoted may not consist solely of digits.");
    }
  }

  return p;
}

static std::size_t span_quotable_string_literal(
    const std::string &s, std::size_t p, std::string *out_string,
    bool allow_number_at_beginning = false) {
  if (s.size() <= p) return p;

  char quote = s[p];
  if (quote != '\'' && quote != '"') {
    // check if valid initial char
    if (!std::isalpha(quote) &&
        !(allow_number_at_beginning && std::isdigit(quote)) && quote != '_' &&
        quote != '$' && quote != '%')
      throw std::runtime_error("Invalid character in string literal");
    quote = 0;
  } else {
    p++;
  }

  if (quote == 0) {
    while (p < s.size()) {
      if (!std::isalnum(s[p]) && s[p] != '_' && s[p] != '$' && s[p] != '.' &&
          s[p] != '%')
        break;
      if (out_string) out_string->push_back(s[p]);
      ++p;
    }
  } else {
    int esc = 0;
    bool done = false;
    while (p < s.size() && !done) {
      if (esc == quote && s[p] != esc) {
        done = true;
        break;
      }
      switch (s[p]) {
        case '"':
        case '\'':
          if (quote == s[p]) {
            if (esc == quote || esc == '\\') {
              if (out_string) out_string->push_back(s[p]);
              esc = 0;
            } else {
              esc = s[p];
            }
          } else {
            if (out_string) out_string->push_back(s[p]);
            esc = 0;
          }
          break;
        case '\\':
          if (esc == '\\') {
            if (out_string) out_string->push_back(s[p]);
            esc = 0;
          } else if (esc == 0) {
            esc = '\\';
          } else {
            done = true;
          }
          break;
        case 'n':
          if (esc == '\\') {
            if (out_string) out_string->push_back('\n');
            esc = 0;
          } else if (esc == 0) {
            if (out_string) out_string->push_back(s[p]);
          } else {
            done = true;
          }
          break;
        case 't':
          if (esc == '\\') {
            if (out_string) out_string->push_back('\t');
            esc = 0;
          } else if (esc == 0) {
            if (out_string) out_string->push_back(s[p]);
          } else {
            done = true;
          }
          break;
        case 'b':
          if (esc == '\\') {
            if (out_string) out_string->push_back('\b');
            esc = 0;
          } else if (esc == 0) {
            if (out_string) out_string->push_back(s[p]);
          } else {
            done = true;
          }
          break;
        case 'r':
          if (esc == '\\') {
            if (out_string) out_string->push_back('\r');
            esc = 0;
          } else if (esc == 0) {
            if (out_string) out_string->push_back(s[p]);
          } else {
            done = true;
          }
          break;
        case '0':
          if (esc == '\\') {
            if (out_string) out_string->push_back('\0');
            esc = 0;
          } else if (esc == 0) {
            if (out_string) out_string->push_back(s[p]);
          } else {
            done = true;
          }
          break;
        case 'Z':
          if (esc == '\\') {
            if (out_string) {
              out_string->push_back(26);
            }
            esc = 0;
          } else if (esc == 0) {
            if (out_string) out_string->push_back(s[p]);
          } else {
            done = true;
          }
          break;
        default:
          if (esc == '\\') {
            if (out_string) out_string->push_back(s[p]);
            esc = 0;
          } else if (esc == 0) {
            if (out_string) out_string->push_back(s[p]);
          } else {
            done = true;
          }
          break;
      }
      ++p;
    }
    if (!done && esc == quote) {
      done = true;
    } else if (!done) {
      throw std::runtime_error("Invalid syntax in string literal");
    }
  }
  return p;
}

static std::size_t span_account_hostname_relaxed(const std::string &s,
                                                 std::size_t p,
                                                 std::string *out_string,
                                                 bool auto_quote_hosts) {
  if (s.size() <= p) return p;

  // Use the span_quotable_identifier, if an error occurs, try to see if quotes
  // would fix it, however first check for the existence of the '@' character
  // which is not allowed in hostnames
  std::size_t old_p = p, res = 0;

  if (s.find('@', p) != std::string::npos) {
    std::string err_msg = "Malformed hostname (illegal symbol: '@')";
    throw std::runtime_error(err_msg);
  }
  // Check if hostname starts with string literal or identifier depending on the
  // first character being a backtick or not.
  if (s[p] == '`') {
    res = span_quotable_identifier(s, p, out_string);
  } else {
    bool quoted = false;
    // Do not allow quote characters unless they are surrounded by quotes
    if (s[p] == s[s.size() - 1] && (s[p] == '\'' || s[p] == '"')) {
      // hostname surrounded by quotes.
      quoted = true;
    } else {
      if ((s.find('\'', p) != std::string::npos) ||
          (s.find('"', p) != std::string::npos)) {
        throw std::runtime_error(
            "Malformed hostname. Cannot use \"'\" or '\"' "
            "characters on the hostname without quotes");
      }
    }
    bool try_quoting = false;
    try {
      res = span_quotable_string_literal(s, p, out_string, true);

      // If the complete string was not consumed could be a hostname that
      // requires quotes, they should be enabled only if not quoted already
      if (res < s.size() && auto_quote_hosts) {
        try_quoting = !quoted;
      }
    } catch (const std::runtime_error &) {
      // In case of error parsing, tries quoting
      try_quoting = auto_quote_hosts;
    }

    if (try_quoting) {
      std::string quoted_s =
          s.substr(0, old_p) + "'" + escape_backticks(s.substr(old_p)) + "'";
      // reset out_string
      if (out_string) *out_string = "";
      res = span_quotable_string_literal(quoted_s, old_p, out_string, true);
    }
  }
  return res;
}

/** Split a MySQL account string (in the form user@host) into its username and
 *  hostname components. The returned strings will be unquoted.
 *  The supported format is the <a
 * href="https://dev.mysql.com/doc/refman/en/account-names.html">standard MySQL
 * account name format</a>. This means it supports both identifiers and string
 * literals for username and hostname.
 */
void split_account(const std::string &account, std::string *out_user,
                   std::string *out_host, Account::Auto_quote auto_quote) {
  std::size_t pos = 0;
  if (out_user) *out_user = "";
  if (out_host) *out_host = "";

  // Check if account starts with string literal or identifier depending on the
  // first character being a backtick or not.
  if (!account.empty()) {
    if (account[0] == '`') {
      pos = span_quotable_identifier(account, 0, out_user);
    } else if (account[0] == '\'' || account[0] == '"') {
      pos = span_quotable_string_literal(account, 0, out_user);
    } else {
      pos = account.rfind('@');
      if (pos == 0) throw std::runtime_error("User name must not be empty.");

      if (Account::Auto_quote::USER_AND_HOST != auto_quote) {
        // don't allow @ on the username unless it is quoted
        if (account.rfind('@', pos - 1) != std::string::npos) {
          throw std::runtime_error("Invalid user name: " +
                                   account.substr(0, pos));
        }
      }

      if (out_user != nullptr) out_user->assign(account, 0, pos);
    }
  } else {
    throw std::runtime_error("User name must not be empty.");
  }

  if (std::string::npos != pos && account[pos] == '@' &&
      ++pos < account.length()) {
    if (account.compare(pos, std::string::npos, "skip-grants host") == 0) {
      pos = account.length();
      if (out_host != nullptr) *out_host = "skip-grants host";
    } else {
      pos = span_account_hostname_relaxed(
          account, pos, out_host, Account::Auto_quote::NO != auto_quote);
    }
  }
  if (pos < account.size())
    throw std::runtime_error("Invalid syntax in account name '" + account +
                             "'");
}

Account split_account(const std::string &account,
                      Account::Auto_quote auto_quote) {
  Account result;
  split_account(account, &result.user, &result.host, auto_quote);
  return result;
}

/** Join MySQL account components into a string suitable for use with GRANT
 *  and similar
 */
std::string make_account(const std::string &user, const std::string &host) {
  return shcore::sqlstring("?@?", 0) << user << host;
}

std::string make_account(const Account &account) {
  return make_account(account.user, account.host);
}

namespace {

void ensure_dot(const std::string &str, std::size_t pos) {
  if (str[pos] != '.') {
    throw std::runtime_error(
        std::string("Invalid object name, expected '.', but got: '") +
        str[pos] + "'.");
  }
}

void ensure_eos(const std::string &str, std::size_t pos) {
  if (pos < str.length()) {
    throw std::runtime_error(
        std::string("Invalid object name, expected end of name, but got: '") +
        str[pos] + "'.");
  }
}

}  // namespace

void split_schema_and_table(const std::string &str, std::string *out_schema,
                            std::string *out_table, bool allow_ansi_quotes) {
  std::string schema;
  std::string table;

  auto pos = span_quotable_identifier(str, 0, &schema, allow_ansi_quotes);

  if (pos < str.length()) {
    ensure_dot(str, pos);

    pos = span_quotable_identifier(str, ++pos, &table, allow_ansi_quotes);

    ensure_eos(str, pos);
  } else {
    // there's only table name
    std::swap(schema, table);
  }

  if (table.empty()) {
    throw std::runtime_error(
        "Invalid object name, table name cannot be empty.");
  }

  if (out_schema) {
    *out_schema = std::move(schema);
  }

  if (out_table) {
    *out_table = std::move(table);
  }
}

void split_schema_table_and_object(const std::string &str,
                                   std::string *out_schema,
                                   std::string *out_table,
                                   std::string *out_object,
                                   bool allow_ansi_quotes) {
  std::string schema;
  std::string table;
  std::string object;

  auto pos = span_quotable_identifier(str, 0, &schema, allow_ansi_quotes);

  if (pos < str.length()) {
    ensure_dot(str, pos);

    pos = span_quotable_identifier(str, ++pos, &table, allow_ansi_quotes);

    if (pos < str.length()) {
      ensure_dot(str, pos);

      pos = span_quotable_identifier(str, ++pos, &object, allow_ansi_quotes);

      ensure_eos(str, pos);
    } else {
      // there are table and object names
      std::swap(table, object);
      std::swap(schema, table);
    }
  } else {
    // there's only object name
    std::swap(schema, object);
  }

  if (object.empty()) {
    throw std::runtime_error(
        "Invalid identifier, object name cannot be empty.");
  }

  if (out_schema) {
    *out_schema = std::move(schema);
  }

  if (out_table) {
    *out_table = std::move(table);
  }

  if (out_object) {
    *out_object = std::move(object);
  }
}

void split_priv_level(const std::string &str, std::string *out_schema,
                      std::string *out_object, size_t *out_leftover) {
  assert(out_schema && out_object);
  // *
  // *.*
  // schema.*
  // *.object
  // schema.object
  std::string schema;
  std::string object;

  *out_schema = "";
  *out_object = "";

  if (str.empty()) return;

  std::string::size_type pos;

  if (str.front() == '*') {
    *out_schema = "*";
    pos = 1;
  } else {
    pos = span_quotable_identifier(str, 0, out_schema);
  }

  if (pos < str.length()) {
    if (str[pos] != '.') {
      throw std::runtime_error(std::string("Invalid object name '" + str +
                                           "', expected '.', but got: '") +
                               str[pos] + "'.");
    }
    ++pos;

    if (pos < str.length()) {
      if (str[pos] == '*') {
        *out_object = "*";
        pos++;
      } else {
        pos = span_quotable_identifier(str, pos, out_object);
      }
    } else {
      throw std::runtime_error("Invalid object name '" + str +
                               "', expected object name after '.'");
    }
    if (pos < str.length()) {
      if (out_leftover) {
        *out_leftover = pos;
      } else {
        throw std::runtime_error(
            std::string("Invalid object name '" + str +
                        "', expected end of name, but got: '") +
            str[pos] + "'.");
      }
    }
  }
}

std::string SHCORE_PUBLIC unquote_identifier(const std::string &str,
                                             bool allow_ansi_quotes) {
  std::string object;
  const auto pos = span_quotable_identifier(str, 0, &object, allow_ansi_quotes);

  if (pos < str.length()) {
    throw std::runtime_error(
        std::string("Invalid object name, expected end of name, but got: '") +
        str[pos] + "'.");
  }

  if (object.empty()) {
    throw std::runtime_error("Object name cannot be empty.");
  }

  return object;
}

std::string SHCORE_PUBLIC unquote_sql_string(const std::string &s) {
  if (s.length() < 2 || s[0] != '\'' || s[0] != s.back())
    throw std::invalid_argument("string is not properly quoted");

  std::string result;
  size_t offs;
  size_t end = s.length();

  result.reserve(end);

  offs = 1;  // skip opening quote

  for (;;) {
    auto p = s.find_first_of("'\\", offs);
    if (p == std::string::npos)
      throw std::invalid_argument("string is not properly quoted");

    if (p > offs) {
      result.append(s.c_str() + offs, p - offs);
    }

    if (s[p] == '\\') {
      ++p;
      switch (s[p]) {
        case 0:
          // string too short?
          throw std::invalid_argument("string is not properly quoted");
        case '0':
          result.push_back(0);
          break;
        case 'n':
          result.push_back('\n');
          break;
        case 'r':
          result.push_back('\r');
          break;
        case 'Z': /* This gives problems on Win32 */
          result.push_back('\032');
          break;
        case '\\':
        case '\'':
        case '"':
        default:
          result.push_back(s[p]);
          break;
      }
    } else {
      ++p;
      if (s[p] == '\'')
        result.push_back('\'');
      else if (p == end)
        break;
      else
        throw std::invalid_argument("string is not properly quoted");
    }
    offs = p + 1;
  }

  return result;
}

void sleep_ms(uint32_t ms) { shcore::current_interrupt()->wait(ms); }
void sleep(std::chrono::milliseconds duration) {
  shcore::current_interrupt()->wait(duration.count());
}

/*
 * Determines the current Operating System
 *
 * @return an enum representing the current operating system
 * (shcore::OperatingSystem)
 */
OperatingSystem get_os_type() {
  OperatingSystem os;

#ifdef WIN32
  os = OperatingSystem::WINDOWS;
#elif __APPLE__
  os = OperatingSystem::MACOS;
#elif __sun
  os = OperatingSystem::SOLARIS;
#elif __linux__
  os = OperatingSystem::LINUX;

  // Detect the distribution
  std::string distro_buffer, proc_version = "/proc/version";

  if (is_file(proc_version)) {
    // Read the proc_version file
    std::ifstream s(proc_version.c_str());

    if (!s.fail())
      std::getline(s, distro_buffer);
    else
      log_warning("Failed to read file: %s", proc_version.c_str());

    // Transform all to lowercase
    std::transform(distro_buffer.begin(), distro_buffer.end(),
                   distro_buffer.begin(), ::tolower);

    const std::vector<std::string> distros = {"ubuntu", "debian", "red hat"};

    for (const auto &value : distros) {
      if (distro_buffer.find(value) != std::string::npos) {
        if (value == "ubuntu" || value == "debian") {
          os = OperatingSystem::DEBIAN;
          break;
        } else if (value == "red hat") {
          os = OperatingSystem::REDHAT;
          break;
        } else {
          continue;
        }
      }
    }
  } else {
    log_warning(
        "Failed to detect the Linux distribution. '%s' "
        "does not exist.",
        proc_version.c_str());
  }
#else
#error Unsupported platform
  os = OperatingSystem::UNKNOWN;
#endif

  return os;
}

std::string get_machine_type() {
  static_assert(std::char_traits<char>::length(MACHINE_TYPE) > 0);
  return MACHINE_TYPE;
}

std::string to_string(OperatingSystem os_type) {
  switch (os_type) {
    case OperatingSystem::UNKNOWN:
      return "unknown";
    case OperatingSystem::DEBIAN:
      return "debian";
    case OperatingSystem::REDHAT:
      return "redhat";
    case OperatingSystem::LINUX:
      return "linux";
    case OperatingSystem::WINDOWS:
      return "windows";
    case OperatingSystem::MACOS:
      return "macos";
    case OperatingSystem::SOLARIS:
      return "solaris";
    default:
      assert(0);
      return "unknown";
  }
}

namespace {

constexpr char k_glob_escape = '\\';
constexpr char k_glob_match_single = '?';
constexpr char k_glob_match_many = '*';

/**
 * https://research.swtch.com/glob
 */
bool _match_glob(const std::string_view pat, const std::string_view str) {
  size_t pend = pat.length();
  size_t send = str.length();
  size_t px = 0;
  size_t sx = 0;
  size_t ppx = 0;
  size_t psx = 0;

  while (px < pend || sx < send) {
    if (px < pend) {
      char c = pat[px];

      switch (c) {
        case k_glob_match_single:
          if (sx < send) {
            ++px;
            ++sx;
            continue;
          }
          break;

        case k_glob_match_many:
          ppx = px;
          psx = sx + 1;
          ++px;
          continue;

        case k_glob_escape:
          // if '\' is followed by \, * or ?, it's an escape sequence, skip '\'
          if ((px + 1) < pend && (pat[px + 1] == k_glob_escape ||
                                  pat[px + 1] == k_glob_match_many ||
                                  pat[px + 1] == k_glob_match_single)) {
            ++px;
            c = pat[px];
          }

          // fall through

        default:
          if (sx < send && str[sx] == c) {
            ++px;
            ++sx;
            continue;
          }
          break;
      }
    }

    if (0 < psx && psx <= send) {
      // original code has here:
      //  px = ppx;
      //  sx = psx;
      // which jumps back to the '*' case, we skip than one iteration by setting
      // the correct values right away
      px = ppx + 1;
      sx = psx;
      ++psx;
      continue;
    }

    return false;
  }

  return true;
}

}  // namespace

/**
 * Match a string against a glob-like pattern.
 *
 * Allowed wildcard characters: '*', '?'.
 * Supports escaping wildcards via '\\' character.
 *
 * Note: works with ASCII only, no UTF8 support
 */
bool match_glob(const std::string_view pattern, const std::string_view s,
                bool case_sensitive) {
  if (!case_sensitive) {
    const std::string &str = str_lower(s);
    const std::string &pat = str_lower(pattern);
    return _match_glob(pat, str);
  }
  return _match_glob(pattern, s);
}

std::optional<std::string> unescape_glob(const std::string_view pattern) {
  const auto length = pattern.length();
  std::string result;
  result.reserve(length);

  for (auto i = decltype(length){0}; i < length; ++i) {
    auto c = pattern[i];

    switch (c) {
      case k_glob_match_single:
      case k_glob_match_many:
        // unescaped wildcard
        return std::nullopt;

      case k_glob_escape:
        if (i + 1 < length) {
          if (const auto cc = pattern[i + 1]; k_glob_escape == cc ||
                                              k_glob_match_single == cc ||
                                              k_glob_match_many == cc) {
            // escaped wildcard, skip escape character and copy wildcard
            // verbatim
            ++i;
            c = cc;
          }
        }
        [[fallthrough]];

      default:
        result.push_back(c);
        break;
    }
  }

  return result;
}

const char *get_long_version() {
  return "Ver " MYSH_FULL_VERSION " for " SYSTEM_TYPE " on " MACHINE_TYPE
         " - for MySQL " LIBMYSQL_VERSION " (" MYSQL_COMPILATION_COMMENT ")";
}

#ifdef _WIN32

std::string SHCORE_PUBLIC last_error_to_string(DWORD code) {
  LPTSTR lpMsgBuf = nullptr;
  std::string ret;

  if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_IGNORE_INSERTS,
                    nullptr, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (LPTSTR)&lpMsgBuf, 0, nullptr) > 0) {
    ret = lpMsgBuf;
    LocalFree(lpMsgBuf);
  } else {
    ret = str_format("Unknown error code: %lu", code);
  }

  return ret;
}

#endif  // _WIN32

std::istream &getline(std::istream &in, std::string &out) {
  if (std::getline(in, out)) {
    if (!out.empty() && out.back() == '\r') {
      out.pop_back();
    }
  }

  return in;
}

bool verify_status_code(int status, std::string *error) {
  assert(error);

  if (status < 0) {
    const auto code = errno;
    *error = "failed to execute: " + errno_to_string(code);
  } else {
#ifdef _WIN32
    const bool exited = true;
    const auto exit_code = status;
#else   // !_WIN32
    const bool exited = WIFEXITED(status);
    const auto exit_code = WEXITSTATUS(status);
#endif  // !_WIN32

    if (exited) {
      if (0 != exit_code) {
        *error = "returned exit code: " + std::to_string(exit_code);
      } else {
        return true;
      }
    }
#ifndef _WIN32
    else if (WIFSIGNALED(status)) {
      *error = "was terminated by signal: " + std::to_string(WTERMSIG(status));
    } else if (WIFSTOPPED(status)) {
      *error = "was stopped by signal: " + std::to_string(WSTOPSIG(status));
    } else {
      *error = "failed with unknown code: " + std::to_string(status);
    }
#endif  // !_WIN32
  }

  return false;
}

bool setenv(const char *name, const char *value) {
  if (nullptr == value || '\0' == *value) {
    return shcore::unsetenv(name);
  }

  const auto ret =
#ifdef _WIN32
      _putenv_s(name, value);
#else   // !_WIN32
      ::setenv(name, value, 1 /* overwrite */);
#endif  // !_WIN32
  return ret == 0;
}

bool setenv(const char *name, const std::string &value) {
  return shcore::setenv(name, value.c_str());
}

bool setenv(const std::string &name, const std::string &value) {
  return shcore::setenv(name.c_str(), value.c_str());
}

bool setenv(const std::string &name_value) {
  const auto pos = name_value.find('=');

  if (std::string::npos != pos) {
    return shcore::setenv(name_value.substr(0, pos),
                          name_value.substr(pos + 1));
  }

  return false;
}

bool unsetenv(const char *name) {
  const auto ret =
#ifdef _WIN32
      _putenv_s(name, "");
#else   // !_WIN32
      ::unsetenv(name);
#endif  // !_WIN32
  return ret == 0;
}

bool unsetenv(const std::string &name) {
  return shcore::unsetenv(name.c_str());
}

}  // namespace shcore
