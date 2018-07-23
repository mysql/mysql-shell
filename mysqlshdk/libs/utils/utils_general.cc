/*
 * Copyright (c) 2014, 2018, Oracle and/or its affiliates. All rights reserved.
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
#include "mysqlshdk/include/mysh_config.h"
#include "mysqlshdk/libs/textui/textui.h"

#ifdef WIN32
#include <Lmcons.h>
#else
#include <unistd.h>
#ifdef HAVE_GETPWUID_R
#include <pwd.h>
#endif
#endif

#include <cctype>
#include <cstdio>
#include <ctime>
#include <locale>

#include "mysh_config.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/db/uri_parser.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "shellcore/utils_help.h"

#include <mysql_version.h>

namespace shcore {
bool is_valid_identifier(const std::string &name) {
  bool ret_val = false;

  if (!name.empty()) {
    std::locale locale;

    ret_val = std::isalpha(name[0], locale) || name[0] == '_';

    size_t index = 1;
    while (ret_val && index < name.size()) {
      ret_val = std::isalnum(name[index], locale) || name[index] == '_';
      index++;
    }
  }

  return ret_val;
}

std::string strip_password(const std::string &connstring) {
  std::string remaining = connstring;
  std::string password;
  std::string scheme;

  std::string::size_type p;
  p = remaining.find("://");
  if (p != std::string::npos) {
    scheme = remaining.substr(0, p + 3);
    remaining = remaining.substr(p + 3);
  }

  std::string s = remaining;
  p = remaining.find('/');
  if (p != std::string::npos) {
    s = remaining.substr(0, p);
  }
  p = s.rfind('@');
  std::string user_part;

  if (p == std::string::npos) {
    // by default, connect using the current OS username
    user_part = get_system_user();
  } else {
    user_part = s.substr(0, p);
  }

  if ((p = user_part.find(':')) != std::string::npos) {
    password = user_part.substr(p + 1);
    std::string uri_stripped = remaining;
    std::string::size_type i = uri_stripped.find(":" + password);
    if (i != std::string::npos) uri_stripped.erase(i, password.length() + 1);

    return scheme + uri_stripped;
  }

  // no password to strip, return original one
  return connstring;
}

/*std::string strip_ssl_args(const std::string &connstring) {
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

  if (set_defaults) connection_options.set_default_connection_data();

  return connection_options;
}

/**
 * Overrides connection data parameters with specific values, also adds
 * parameters with default values if missing
 */
void update_connection_data(
    mysqlshdk::db::Connection_options *connection_options,
    const std::string &user, const char *password, const std::string &host,
    int port, const std::string &sock, const std::string &database,
    const mysqlshdk::db::Ssl_options &ssl_options,
    const std::string &auth_method, bool get_server_public_key,
    const std::string &server_public_key_path,
    const std::string &connect_timeout) {
  if (!user.empty()) {
    connection_options->clear_user();
    connection_options->set_user(user);
  }

  if (!host.empty()) {
    connection_options->clear_host();
    connection_options->set_host(host);
  }

  if (port != 0) {
    connection_options->clear_port();
    connection_options->set_port(port);
  }

  if (password) {
    connection_options->clear_password();
    connection_options->set_password(password);
  }

  if (!database.empty()) {
    connection_options->clear_schema();
    connection_options->set_schema(database);
  }

  if (!sock.empty()) {
    connection_options->clear_socket();
#ifdef _WIN32
    connection_options->set_pipe(sock);
#else   // !_WIN32
    connection_options->set_socket(sock);
#endif  // !_WIN32
  }

  if (ssl_options.has_ca()) {
    connection_options->get_ssl_options().clear_ca();
    connection_options->get_ssl_options().set_ca(ssl_options.get_ca());
  }

  if (ssl_options.has_cert()) {
    connection_options->get_ssl_options().clear_cert();
    connection_options->get_ssl_options().set_cert(ssl_options.get_cert());
  }

  if (ssl_options.has_key()) {
    connection_options->get_ssl_options().clear_key();
    connection_options->get_ssl_options().set_key(ssl_options.get_key());
  }

  if (ssl_options.has_capath()) {
    connection_options->get_ssl_options().clear_capath();
    connection_options->get_ssl_options().set_capath(ssl_options.get_capath());
  }

  if (ssl_options.has_crl()) {
    connection_options->get_ssl_options().clear_crl();
    connection_options->get_ssl_options().set_crl(ssl_options.get_crl());
  }

  if (ssl_options.has_crlpath()) {
    connection_options->get_ssl_options().clear_crlpath();
    connection_options->get_ssl_options().set_crlpath(
        ssl_options.get_crlpath());
  }

  if (ssl_options.has_cipher()) {
    connection_options->get_ssl_options().clear_cipher();
    connection_options->get_ssl_options().set_cipher(ssl_options.get_cipher());
  }

  if (ssl_options.has_tls_version()) {
    connection_options->get_ssl_options().clear_tls_version();
    connection_options->get_ssl_options().set_tls_version(
        ssl_options.get_tls_version());
  }

  if (ssl_options.has_mode()) {
    connection_options->get_ssl_options().clear_mode();
    connection_options->get_ssl_options().set_mode(ssl_options.get_mode());
  }

  if (!auth_method.empty()) {
    if (connection_options->has(mysqlshdk::db::kAuthMethod))
      connection_options->remove(mysqlshdk::db::kAuthMethod);

    connection_options->set(mysqlshdk::db::kAuthMethod, {auth_method});
  }

  if (get_server_public_key) {
    if (connection_options->has(mysqlshdk::db::kGetServerPublicKey)) {
      connection_options->remove(mysqlshdk::db::kGetServerPublicKey);
    }
    connection_options->set(mysqlshdk::db::kGetServerPublicKey, {"true"});
  }

  if (!server_public_key_path.empty()) {
    if (connection_options->has(mysqlshdk::db::kServerPublicKeyPath)) {
      connection_options->remove(mysqlshdk::db::kServerPublicKeyPath);
    }
    connection_options->set(mysqlshdk::db::kServerPublicKeyPath,
                            {server_public_key_path});
  }

  if (!connect_timeout.empty()) {
    if (connection_options->has(mysqlshdk::db::kConnectTimeout)) {
      connection_options->remove(mysqlshdk::db::kConnectTimeout);
    }
    connection_options->set(mysqlshdk::db::kConnectTimeout, {connect_timeout});
  }
}

std::string get_system_user() {
  std::string ret_val;

#ifdef WIN32
  char username[UNLEN + 1];
  DWORD username_len = UNLEN + 1;
  if (GetUserName(username, &username_len)) {
    ret_val.assign(username);
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
#else
    ret_val = "UNKNOWN_USER";
#endif
  }
#endif

  return ret_val;
}

std::string errno_to_string(int err) {
#ifdef _WIN32
#define strerror_r(E, B, S) strerror_s(B, S, E)
#endif
#if defined(_WIN32) || defined(__SunOS)
  char buf[256];
  if (!strerror_r(err, buf, sizeof(buf))) return std::string(buf);
  return "";
#elif __APPLE__ || ((_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && \
                    !_GNU_SOURCE)  // NOLINT
  std::string ret;
  ret.resize(256);
  auto i = strerror_r(err, &ret[0], ret.size());
  assert(i == 0);
  (void)i;
  ret.resize(strlen(&ret[0]));
  return ret;
#else
  char buf[256];
  return strerror_r(err, buf, sizeof(buf));
#endif
}

std::vector<std::string> split_string(const std::string &input,
                                      const std::string &separator,
                                      bool compress) {
  std::vector<std::string> ret_val;

  size_t index = 0, new_find = 0;

  while (new_find != std::string::npos) {
    new_find = input.find(separator, index);

    if (new_find != std::string::npos) {
      // When compress is enabled, consecutive separators
      // do not generate new elements
      if (new_find > index || !compress || new_find == 0)
        ret_val.push_back(input.substr(index, new_find - index));

      index = new_find + separator.length();
    } else {
      ret_val.push_back(input.substr(index));
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
std::vector<std::string> split_string_chars(const std::string &input,
                                            const std::string &separator_chars,
                                            bool compress) {
  std::vector<std::string> ret_val;

  size_t index = 0, new_find = 0;

  while (new_find != std::string::npos) {
    new_find = input.find_first_of(separator_chars, index);

    if (new_find != std::string::npos) {
      // When compress is enabled, consecutive separators
      // do not generate new elements
      if (new_find > index || !compress || new_find == 0)
        ret_val.push_back(input.substr(index, new_find - index));

      index = new_find + 1;
    } else {
      ret_val.push_back(input.substr(index));
    }
  }

  return ret_val;
}

// Retrieves a member name on a specific NamingStyle
// NOTE: Assumption is given that everything is created using a lowerUpperCase
// naming style
//       Which is the default to be used on C++ and JS
std::string get_member_name(const std::string &name,
                            shcore::NamingStyle style) {
  std::string new_name;
  switch (style) {
    // This is the default style, input is returned without modifications
    case shcore::LowerCamelCase:
      return new_name = name;
    case shcore::LowerCaseUnderscores: {
      // Uppercase letters will be converted to underscore+lowercase letter
      // except in two situations:
      // - When it is the first letter
      // - When an underscore is already before the uppercase letter
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
      break;
    }
    case Constants: {
      for (auto character : name) {
        if (character >= 97 && character <= 122)
          new_name.append(1, character - 32);
        else
          new_name.append(1, character);
      }
      break;
    }
  }

  return new_name;
}

/** Convert string from under_score/dash naming convention to camelCase

  As a special case, if string is longer than 2 characters and
  all characters are uppercase, conversion will be skipped.
  */
std::string to_camel_case(const std::string &name) {
  std::string new_name;
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
  if (upper_count == name.length()) return name;
  return new_name;
}

/** Convert string from camcelCase naming convention to under_score

  As a special case, if string is longer than 2 characters and
  all characters are uppercase, conversion will be skipped.
  */
std::string from_camel_case(const std::string &name) {
  std::string new_name;
  size_t upper_count = 0;
  // Uppercase letters will be converted to underscore+lowercase letter
  // except in two situations:
  // - When it is the first letter
  // - When an underscore is already before the uppercase letter
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
      skip_underscore = character == '_';
      if (skip_underscore) upper_count++;
      new_name.append(1, character);
    }
  }
  if (upper_count == name.length()) return name;
  return new_name;
}

static std::size_t span_quotable_identifier(const std::string &s, std::size_t p,
                                            std::string *out_string) {
  bool seen_not_a_digit = false;
  if (s.size() <= p) return p;

  char quote = s[p];
  if (quote !=
      '`') {  // Ignoring \" since ANSI_QUOTES is not meant to be supported
    // check if valid initial char
    if (!std::isalnum(quote) && quote != '_' && quote != '$')
      throw std::runtime_error("Invalid character in identifier");
    if (!std::isdigit(quote)) seen_not_a_digit = true;
    quote = 0;
  } else {
    seen_not_a_digit = true;
    p++;
  }

  if (quote == 0) {
    while (p < s.size()) {
      if (!std::isalnum(s[p]) && s[p] != '_' && s[p] != '$') break;
      if (out_string) out_string->push_back(s[p]);
      if (!seen_not_a_digit && !isdigit(s[p])) seen_not_a_digit = true;
      ++p;
    }
    if (!seen_not_a_digit)
      throw std::runtime_error(
          "Invalid identifier: identifiers may begin with a digit but unless "
          "quoted may not consist solely of digits.");

  } else {
    int esc = 0;
    bool done = false;
    while (p < s.size() && !done) {
      if (esc == quote && s[p] != esc) {
        done = true;
        break;
      }
      switch (s[p]) {
        case '`':
          if (esc == quote) {
            if (out_string) out_string->push_back(s[p]);
            esc = 0;
          } else {
            esc = s[p];
          }
          break;
        default:
          if (esc == quote) {
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
      throw std::runtime_error("Invalid syntax in identifier");
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
    // Do not allow quote characters unless they are surrounded by quotes
    if (s[p] == s[s.size() - 1] && (s[p] == '\'' || s[p] == '"')) {
      // hostname surrounded by quotes.
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
        bool quoted = ((s[p] == '\'' && s.at(s.size() - 1) == '\'') ||
                       (s[p] == '"' && s.at(s.size() - 1) == '"'));

        try_quoting = !quoted;
      }
    } catch (std::runtime_error &e) {
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
                   std::string *out_host, bool auto_quote_hosts) {
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
      if (pos == 0) throw std::runtime_error("Empty user name: " + account);
      if (out_user != nullptr) out_user->assign(account, 0, pos);
    }
  }
  if (std::string::npos != pos && account[pos] == '@' &&
      ++pos < account.length()) {
    if (account.compare(pos, std::string::npos, "skip-grants host") == 0) {
      pos = account.length();
      if (out_host != nullptr) *out_host = "skip-grants host";
    } else {
      pos = span_account_hostname_relaxed(account, pos, out_host,
                                          auto_quote_hosts);
    }
  }
  if (pos < account.size())
    throw std::runtime_error("Invalid syntax in account name '" + account +
                             "'");
}

/** Join MySQL account components into a string suitable for use with GRANT
 *  and similar
 */
std::string make_account(const std::string &user, const std::string &host) {
  return shcore::sqlstring("?@?", 0) << user << host;
}

void sleep_ms(uint32_t ms) {
#ifdef _WIN32
  Sleep(ms);
#else
  usleep(ms * 1000);
#endif
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
#elif __SunOS
  os = OperatingSystem::SOLARIS;
#elif __linux__
  os = OperatingSystem::LINUX;

  // Detect the distribution
  std::string distro_buffer, proc_version = "/proc/version";

  if (shcore::file_exists(proc_version)) {
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

/**
 * https://research.swtch.com/glob
 */
bool _match_glob(const std::string &pat, const std::string &str) {
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
        case '?':
          if (sx < send) {
            ++px;
            ++sx;
            continue;
          }
          break;

        case '*':
          ppx = px;
          psx = sx + 1;
          ++px;
          continue;

        case '\\':
          // if '\' is followed by * or ?, it's an escape sequence, skip '\'
          if ((px + 1) < pend && (pat[px + 1] == '*' || pat[px + 1] == '?')) {
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
bool match_glob(const std::string &pattern, const std::string &s,
                bool case_sensitive) {
  std::string str = case_sensitive ? s : str_lower(s);
  std::string pat = case_sensitive ? pattern : str_lower(pattern);
  return _match_glob(pat, str);
}

std::string fmttime(const char *fmt) {
  time_t t = time(nullptr);
  char buf[64];

#ifdef WIN32
  struct tm lt;
  localtime_s(&lt, &t);
  strftime(buf, sizeof(buf), fmt, &lt);
#else
  struct tm lt;
  localtime_r(&t, &lt);
  strftime(buf, sizeof(buf), fmt, &lt);
#endif

  return buf;
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

}  // namespace shcore
