/*
* Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; version 2 of the
* License.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
* 02110-1301  USA
*/

#include "mysh_config.h"

#include <stdio.h>
#include <time.h>
#include <cctype>
#include <locale>
#ifdef WIN32
#include <WinSock2.h>
#include <windows.h>
#include <Lmcons.h>
#define strerror_r(errno,buf,len) strerror_s(buf,len,errno)
#else
#include <unistd.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#ifdef HAVE_GETPWUID_R
#include <pwd.h>
#endif
#endif
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/db/uri_parser.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "shellcore/utils_help.h"

namespace shcore {
bool is_valid_identifier(const std::string& name) {
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

  std::string::size_type p;
  p = remaining.find("://");
  if (p != std::string::npos) {
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
#ifdef _WIN32
    // TODO(anyone) find out current username here
#else
    const char *tmp = getenv("USER");
    user_part = tmp ? tmp : "";
#endif
  } else
  user_part = s.substr(0, p);

  if ((p = user_part.find(':')) != std::string::npos) {
    password = user_part.substr(p + 1);
    std::string uri_stripped = connstring;
    std::string::size_type i = uri_stripped.find(":" + password);
    if (i != std::string::npos)
      uri_stripped.erase(i, password.length() + 1);

    return uri_stripped;
  }

  // no password to strip, return original one
  return connstring;
}

/*std::string strip_ssl_args(const std::string &connstring) {
  std::string result = connstring;
  std::string::size_type pos;
  if ((pos = result.find("ssl_ca=")) != std::string::npos) {
    std::string::size_type pos2 = result.find("&");
    result = result.replace(pos, (pos2 == std::string::npos) ? std::string::npos : pos2 - pos + 1, "");
  }
  if ((pos = result.find("ssl_cert=")) != std::string::npos) {
    std::string::size_type pos2 = result.find("&");
    result = result.replace(pos, (pos2 == std::string::npos) ? std::string::npos : pos2 - pos + 1, "");
  }
  if ((pos = result.find("ssl_key=")) != std::string::npos) {
    std::string::size_type pos2 = result.find("&");
    result = result.replace(pos, (pos2 == std::string::npos) ? std::string::npos : pos2 - pos + 1, "");
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

  if (!connection_options.has_user() && set_defaults)
    connection_options.set_user(get_system_user());

  return connection_options;
}

// Overrides connection data parameters with specific values, also adds parameters with default values if missing
void update_connection_data
  (mysqlshdk::db::Connection_options *connection_options,
   const std::string &user, const char *password,
   const std::string &host, int port,
   const std::string& sock,
   const std::string &database,
   const mysqlshdk::db::Ssl_options& ssl_options,
   const std::string &auth_method) {
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
    connection_options->set_socket(sock);
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

  if (ssl_options.has_ciphers()) {
    connection_options->get_ssl_options().clear_ciphers();
    connection_options->get_ssl_options().set_ciphers(
        ssl_options.get_ciphers());
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
}

void set_default_connection_data
  (mysqlshdk::db::Connection_options *connection_options) {
  // Default values
  if (!connection_options->has_user())
    connection_options->set_user(get_system_user());

  if (!connection_options->has_host() &&
     (!connection_options->has_transport_type() ||
      connection_options->get_transport_type() == mysqlshdk::db::Tcp))
    connection_options->set_host("localhost");
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
    ret_val = "root";    /* allow use of surun */
  } else {
# if defined(HAVE_GETPWUID_R) and defined(HAVE_GETLOGIN_R)
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
        if ((str = getenv("USER")) || (str=getenv("LOGNAME")) ||
        (str = getenv("LOGIN"))) {
          ret_val.assign(str);
        } else {
          ret_val = "UNKNOWN_USER";
        }
      }
      free(buffer);
    }

    free(name);
# elif HAVE_CUSERID
    char username[L_cuserid];
    if (cuserid(username))
      ret_val.assign(username);
# else
    ret_val = "UNKNOWN_USER";
# endif
  }
#endif

  return ret_val;
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

std::vector<std::string> SHCORE_PUBLIC
split_string(const std::string &input, std::vector<size_t> max_lengths) {
  std::vector<std::string> chunks;

  size_t index = max_lengths[0];
  size_t last_length = max_lengths[0];
  size_t start = 0;

  // Index will eventually overpass the input size
  // As lines are added
  while (input.size() > index) {
    auto pos = input.rfind(" ", index);

    if (pos == std::string::npos) {
      break;
    } else if (pos > start) {
      chunks.push_back(input.substr(start, pos - start));
      start = pos + 1;

      if (max_lengths.size() > chunks.size())
        last_length = max_lengths[chunks.size()];

      index = start + last_length;
    }
  }

  // Adds the remainder of the input
  if (start < input.size())
    chunks.push_back(input.substr(start));

  return chunks;
}

/**
* Splits string based on each of the individual characters of the separator string
*
* @param input The string to be split
* @param separator_chars String containing characters wherein the input string is split on any of the characters
* @param compress Boolean value which when true ensures consecutive separators do not generate new elements in the split
*
* @returns vector of splitted strings
*/
std::vector<std::string> split_string_chars(const std::string& input, const std::string& separator_chars, bool compress) {
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
std::string get_member_name(const std::string& name, shcore::NamingStyle style) {
  std::string new_name;
  switch (style) {
    // This is the default style, input is returned without modifications
    case shcore::LowerCamelCase:
      return new_name = name;
    case shcore::LowerCaseUnderscores:
    {
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
    case Constants:
    {
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

std::string format_text(const std::vector<std::string>& lines, size_t width, size_t left_padding, bool paragraph_per_line) {
  std::string ret_val;

  // Considers the new line character being added
  std::vector<size_t> lengths = {width - (left_padding + 1)};
  std::vector<std::string> sublines;
  std::string space(left_padding, ' ');

  if (!lines.empty()) {
    ret_val.reserve(lines.size() * width);

    sublines = split_string(lines[0], lengths);

    // Processes the first line
    // The first subline is meant to be returned without any space prefix
    // Since this will be appended to an existing line
    ret_val = sublines[0];
    sublines.erase(sublines.begin());

    // The remaining lines are just appended with the space prefix
    for (auto subline : sublines) {
      if (' ' == subline[0])
        ret_val += "\n" + space + subline.substr(1);
      else
        ret_val += "\n" + space + subline;
    }
  }

  if (lines.size() > 1) {
    for (size_t index = 1; index < lines.size(); index++) {
      if (paragraph_per_line)
        ret_val += "\n";

      sublines = split_string(lines[index], lengths);
      for (auto subline : sublines) {
        if (' ' == subline[0])
          ret_val += "\n" + space + subline.substr(1);
        else
          ret_val += "\n" + space + subline;
      }
    }
  }

  return ret_val;
};

std::string format_markup_text(const std::vector<std::string>& lines, size_t width, size_t left_padding) {
  std::string ret_val;

  ret_val.reserve(lines.size() * width);
  std::string strpadding(left_padding, ' ');

  bool previous_was_item = false;
  bool current_is_item = false;

  std::string new_line;
  for (auto line : lines) {
    ret_val.append(new_line);

    std::string space;
    std::vector<size_t> lengths;
    // handles list items
    auto pos = line.find("@li");
    if (0 == pos) {
      current_is_item = true;

      // Adds an extra new line to separate the first item from the previous paragraph
      if (previous_was_item != current_is_item)
        ret_val += new_line;

      ret_val += strpadding + " - ";
      ret_val += shcore::format_text({line.substr(4)}, width, left_padding + 3, false);
    } else {
      current_is_item = false;

      // May be a header
      size_t  hstart = line.find("<b>");
      size_t  hend = line.find("</b>");
      if (hstart != std::string::npos && hend != std::string::npos) {
        ret_val += new_line;
        line = "** " + line.substr(hstart + 3, hend - hstart - 3) + " **";
      }

      ret_val += new_line + strpadding + shcore::format_text({line}, width, left_padding, false);
    }

    previous_was_item = current_is_item;

    // The new line only makes sense after the first line
    new_line = "\n";
  }

  return ret_val;
};

std::string replace_text(const std::string& source, const std::string& from, const std::string& to) {
  std::string ret_val;
  size_t start = 0, index = 0;

  index = source.find(from, start);
  while (index != std::string::npos) {
    ret_val += source.substr(start, index);
    ret_val += to;
    start = index + from.length();
    index = source.find(from, start);
  }

  // Appends the remaining text
  ret_val += source.substr(start);

  return ret_val;
}

std::string get_my_hostname() {
  char hostname[1024]  {'\0'};

#if defined(_WIN32) || defined(__APPLE__)
  if (gethostname(hostname, sizeof(hostname)) < 0) {
    char msg[1024];
    (void)strerror_r(errno, msg, sizeof(msg));
    log_error("Could not get hostname: %s", msg);
    throw std::runtime_error("Could not get local hostname");
  }
#else
  struct ifaddrs *ifa, *ifap;
  int ret = EAI_NONAME, family, addrlen;

  if (getifaddrs(&ifa) != 0)
    throw std::runtime_error("Could not get local host address: " + std::string(strerror(errno)));
  for (ifap = ifa; ifap != NULL; ifap = ifap->ifa_next) {
    /* Skip interfaces that are not UP, do not have configured addresses, and loopback interface */
    if ((ifap->ifa_addr == NULL) || (ifap->ifa_flags & IFF_LOOPBACK) || (!(ifap->ifa_flags & IFF_UP)))
      continue;

    /* Only handle IPv4 and IPv6 addresses */
    family = ifap->ifa_addr->sa_family;
    if (family != AF_INET && family != AF_INET6)
      continue;

    addrlen = (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);

    /* Skip IPv6 link-local addresses */
    if (family == AF_INET6) {
      struct sockaddr_in6 *sin6;

      sin6 = (struct sockaddr_in6 *)ifap->ifa_addr;
      if (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr) || IN6_IS_ADDR_MC_LINKLOCAL(&sin6->sin6_addr))
        continue;
    }

    ret = getnameinfo(ifap->ifa_addr, addrlen, hostname, sizeof(hostname), NULL, 0, NI_NAMEREQD);

    if (ret == 0)
      break;

#ifdef __linux__
    if (ret == EAI_NONAME) {
      if ((ret = gethostname(hostname, sizeof(hostname))) < 0)
        continue;
      else
        break;
#endif
    }
  }

  if (ret != 0) {
    if (ret != EAI_NONAME)
      throw std::runtime_error("Could not get local host address: " + std::string(gai_strerror(ret)));
  }
#endif

  return hostname;
}

bool is_local_host(const std::string &host, bool check_hostname) {
  // TODO: Simple implementation for now, we may improve this to analyze
  // a given IP address or hostname against the local interfaces
  return (host == "127.0.0.1" ||
          host == "localhost" ||
          (host == get_my_hostname() && check_hostname));
}

static std::size_t span_quotable_identifier(const std::string &s, std::size_t p,
                                            std::string *out_string) {
  bool seen_not_a_digit = false;
  if (s.size() <= p)
    return p;

  char quote = s[p];
  if (quote !=
      '`') {  // Ignoring \" since ANSI_QUOTES is not meant to be supported
    // check if valid initial char
    if (!std::isalnum(quote) && quote != '_' && quote != '$')
      throw std::runtime_error("Invalid character in identifier");
    if (!std::isdigit(quote))
      seen_not_a_digit = true;
    quote = 0;
  } else {
    seen_not_a_digit = true;
    p++;
  }

  if (quote == 0) {
    while (p < s.size()) {
      if (!std::isalnum(s[p]) && s[p] != '_' && s[p] != '$')
        break;
      if (out_string)
        out_string->push_back(s[p]);
      if (!seen_not_a_digit && !isdigit(s[p]))
        seen_not_a_digit = true;
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
            if (out_string)
              out_string->push_back(s[p]);
            esc = 0;
          } else {
            esc = s[p];
          }
          break;
        default:
          if (esc == quote) {
            if (out_string)
              out_string->push_back(s[p]);
            esc = 0;
          } else if (esc == 0) {
            if (out_string)
              out_string->push_back(s[p]);
          } else {
            done = true;
          }
          break;
      }
      ++p;
    }
    if (!done && esc == quote)
      done = true;
    else if (!done) {
      throw std::runtime_error("Invalid syntax in identifier");
    }
  }
  return p;
}

static std::size_t span_quotable_string_literal(const std::string &s,
                                                std::size_t p,
                                                std::string *out_string) {
  if (s.size() <= p)
    return p;

  char quote = s[p];
  if (quote != '\'' && quote != '"') {
    // check if valid initial char
    if (!std::isalpha(quote) && quote != '_' && quote != '$')
      throw std::runtime_error("Invalid character in string literal");
    quote = 0;
  } else {
    p++;
  }

  if (quote == 0) {
    while (p < s.size()) {
      if (!std::isalnum(s[p]) && s[p] != '_' && s[p] != '$')
        break;
      if (out_string)
        out_string->push_back(s[p]);
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
              if (out_string)
                out_string->push_back(s[p]);
              esc = 0;
            } else {
              esc = s[p];
            }
          } else {
            if (out_string)
              out_string->push_back(s[p]);
            esc = 0;
          }
          break;
        case '\\':
          if (esc == '\\') {
            if (out_string)
              out_string->push_back(s[p]);
            esc = 0;
          } else if (esc == 0) {
            esc = '\\';
          } else {
            done = true;
          }
          break;
        case 'n':
          if (esc == '\\') {
            if (out_string)
              out_string->push_back('\n');
            esc = 0;
          } else if (esc == 0) {
            if (out_string)
              out_string->push_back(s[p]);
          } else {
            done = true;
          }
          break;
        case 't':
          if (esc == '\\') {
            if (out_string)
              out_string->push_back('\t');
            esc = 0;
          } else if (esc == 0) {
            if (out_string)
              out_string->push_back(s[p]);
          } else {
            done = true;
          }
          break;
        case 'b':
          if (esc == '\\') {
            if (out_string)
              out_string->push_back('\b');
            esc = 0;
          } else if (esc == 0) {
            if (out_string)
              out_string->push_back(s[p]);
          } else {
            done = true;
          }
          break;
        case 'r':
          if (esc == '\\') {
            if (out_string)
              out_string->push_back('\r');
            esc = 0;
          } else if (esc == 0) {
            if (out_string)
              out_string->push_back(s[p]);
          } else {
            done = true;
          }
          break;
        case '0':
          if (esc == '\\') {
            if (out_string)
              out_string->push_back('\0');
            esc = 0;
          } else if (esc == 0) {
            if (out_string)
              out_string->push_back(s[p]);
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
            if (out_string)
              out_string->push_back(s[p]);
          } else {
            done = true;
          }
          break;
        default:
          if (esc == '\\') {
            if (out_string)
              out_string->push_back(s[p]);
            esc = 0;
          } else if (esc == 0) {
            if (out_string)
              out_string->push_back(s[p]);
          } else {
            done = true;
          }
          break;
      }
      ++p;
    }
    if (!done && esc == quote)
      done = true;
    else if (!done) {
      throw std::runtime_error("Invalid syntax in string literal");
    }
  }
  return p;
}

static std::size_t span_account_hostname_relaxed(const std::string &s,
                                                 std::size_t p,
                                                 std::string *out_string) {
  if (s.size() <= p)
    return p;

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
  if (s[p] == '`')
    res = span_quotable_identifier(s, p, out_string);
  else {
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
    try {
      res = span_quotable_string_literal(s, p, out_string);
    } catch (std::runtime_error e) {
      std::string quoted_s =
          s.substr(0, old_p) + quote_identifier(s.substr(old_p), '\'');
      // reset out_string
      if (out_string)
        *out_string = "";
      res = span_quotable_string_literal(quoted_s, old_p, out_string);
    }
  }
  return res;
}

/** Split a MySQL account string (in the form user@host) into its username and
 *  hostname components. The returned strings will be unquoted.
 *  The supported format is the <a href="https://dev.mysql.com/doc/refman/en/account-names.html">standard MySQL account name format</a>.
 *  This means it supports both identifiers and string literals for username and hostname.
 */
void split_account(const std::string &account, std::string *out_user,
                   std::string *out_host) {
  std::size_t pos = 0;
  if (out_user)
    *out_user = "";
  if (out_host)
    *out_host = "";

  // Check if account starts with string literal or identifier depending on the
  // first character being a backtick or not.
  if (account.size() > 0 && account[0] == '`')
    pos = span_quotable_identifier(account, 0, out_user);
  else
    pos = span_quotable_string_literal(account, 0, out_user);
  if (account[pos] == '@') {
    pos = span_account_hostname_relaxed(account, pos + 1, out_host);
  }
  if (pos < account.size())
    throw std::runtime_error("Invalid syntax in account name '" + account +
                             "'");
}

/** Join MySQL account components into a string suitable for use with GRANT
 *  and similar
 */
std::string make_account(const std::string& user, const std::string &host) {
  return shcore::sqlstring("?@?", 0) << user << host;
}

void sleep_ms(uint32_t ms) {
#ifdef _WIN32
  Sleep(ms);
#else
  usleep(ms * 1000);
#endif
}


static bool _match_glob(const std::string &pat, size_t ppos,
                        const std::string &str, size_t spos) {
  size_t pend = pat.length();
  size_t send = str.length();
  // we allow the string to be matched up to the \0
  while (ppos < pend && spos <= send) {
    int sc = str[spos];
    int pc = pat[ppos];
    switch (pc) {
      case '*':
        // skip multiple consecutive *
        while (ppos < pend && pat[ppos + 1] == '*')
          ++ppos;

        // match * by trying every substring of str with the rest of the pattern
        for (size_t sp = spos; sp <= send; ++sp) {
          // if something matched, we're fine
          if (_match_glob(pat, ppos + 1, str, sp))
            return true;
        }
        // if there were no matches, then give up
        return false;
      case '\\':
        ++ppos;
        if (ppos >= pend)  // can't have an escape at the end of the pattern
          throw std::logic_error("Invalid pattern "+pat);
        pc = pat[ppos];
        if (sc != pc)
          return false;
        ++ppos;
        ++spos;
        break;
      case '?':
        ++ppos;
        ++spos;
        break;
      default:
        if (sc != pc)
          return false;
        ++ppos;
        ++spos;
        break;
    }
  }
  return ppos == pend && spos == send;
}

/** Match a string against a glob-like pattern using backtracking.
    Not very efficient, but easier than implementing a regex-like matcher
    and good enough for our use cases atm.

    Also <regex> doesn't work in gcc 4.8

    Note: works with ASCII only, no utf8 support
  */
bool match_glob(const std::string &pattern, const std::string &s,
                bool case_sensitive) {
  std::string str = case_sensitive ? s : str_lower(s);
  std::string pat = case_sensitive ? pattern : str_lower(pattern);
  return _match_glob(pat, 0, str, 0);
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
} // namespace
