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

#include <stdio.h>
#include <cctype>
#include "utils/utils_general.h"
#include "utils/uri_parser.h"
#include "utils/utils_file.h"
#include "utils/utils_sqlstring.h"
#include "utils/utils_string.h"
#include <locale>
#include "include/mysh_config.h"
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
#include "utils/utils_connection.h"

#include "utils_connection.h"
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

std::string build_connection_string(Value::Map_type_ref data, bool with_password) {
  std::string uri;

  // If needed we construct the URi from the individual parameters
  if (data) {
    if (data->has_key(kDbUser))
      uri.append((*data)[kDbUser].as_string());
    else if (data->has_key(kUser))
      uri.append((*data)[kUser].as_string());

    // Appends password definition, either if it is empty or not
    // only if a user was specified
    if (with_password && !uri.empty()) {
      uri.append(":");
      if (data->has_key(kDbPassword))
        uri.append((*data)[kDbPassword].as_string());
      else if (data->has_key(kPassword))
        uri.append((*data)[kPassword].as_string());
    }

    // Appends the user@host separator, if a user has specified
    if (!uri.empty())
      uri.append("@");

    // Sets the socket
    if (data->has_key(kSocket))
      uri.append((*data)[kSocket].as_string());

    else{ // the uri either has a socket, or an hostname and port
      // Sets the host
      if (data->has_key(kHost))
        uri.append((*data)[kHost].as_string());

      // Sets the port
      if (data->has_key(kPort)) {
        uri.append(":");
        uri.append((*data)[kPort].descr(true));
      }
    }

    // Sets the database
    if (data->has_key(kSchema)) {
      uri.append("/");
      uri.append((*data)[kSchema].as_string());
    }

    bool has_ssl = false;
    mysqlshdk::utils::Ssl_info ssl_info;

    if (data->has_key(kSslCa)) {
      ssl_info.ca = (*data)[kSslCa].as_string();
      has_ssl = true;
    }

    if (data->has_key(kSslCert)) {
      ssl_info.cert = (*data)[kSslCert].as_string();
      has_ssl = true;
    }

    if (data->has_key(kSslKey)) {
      ssl_info.key = (*data)[kSslKey].as_string();
      has_ssl = true;
    }

    if (data->has_key(kSslCaPath)) {
      ssl_info.capath = (*data)[kSslCaPath].as_string();
      has_ssl = true;
    }

    if (data->has_key(kSslCrl)) {
      ssl_info.crl = (*data)[kSslCrl].as_string();
      has_ssl = true;
    }

    if (data->has_key(kSslCrlPath)) {
      ssl_info.crlpath = (*data)[kSslCrlPath].as_string();
      has_ssl = true;
    }

    if (data->has_key(kSslCiphers)) {
      ssl_info.ciphers = (*data)[kSslCiphers].as_string();
      has_ssl = true;
    }

    if (data->has_key(kSslTlsVersion)) {
      ssl_info.tls_version = (*data)[kSslTlsVersion].as_string();
      has_ssl = true;
    }

    if (data->has_key(kSslMode)) {
      ssl_info.mode = shcore::MapSslModeNameToValue::get_value((*data)[kSslMode].as_string());
      has_ssl = true;
    }

    if (has_ssl)
    {
      conn_str_cat_ssl_data(uri, ssl_info);
    }
  }

  return uri;
}

void conn_str_cat_ssl_data(std::string& uri, const mysqlshdk::utils::Ssl_info &ssl_info) {

  std::string uri_tmp;
  if (!ssl_info.ca.is_null())
    uri_tmp.append(kSslCa).append("=").append(*ssl_info.ca);

  if (!ssl_info.capath.is_null()) {
    if (!uri_tmp.empty())
      uri_tmp.append("&");
    uri_tmp.append(kSslCaPath).append("=").append(*ssl_info.capath);
  }

  if (!ssl_info.cert.is_null()) {
    if (!uri_tmp.empty())
      uri_tmp.append("&");
    uri_tmp.append(kSslCert).append("=").append(*ssl_info.cert);
  }

  if (!ssl_info.key.is_null()) {
    if (!uri_tmp.empty())
      uri_tmp.append("&");
    uri_tmp.append(kSslKey).append("=").append(*ssl_info.key);
  }

  if (!ssl_info.ciphers.is_null()) {
    if (!uri_tmp.empty())
      uri_tmp.append("&");
    uri_tmp.append(kSslCiphers).append("=").append(*ssl_info.ciphers);
  }

  if (!ssl_info.crl.is_null()) {
    if (!uri_tmp.empty())
      uri_tmp.append("&");
    uri_tmp.append(kSslCrl).append("=").append(*ssl_info.crl);
  }

  if (!ssl_info.crlpath.is_null()) {
    if (!uri_tmp.empty())
      uri_tmp.append("&");
    uri_tmp.append(kSslCrlPath).append("=").append(*ssl_info.crlpath);
  }

  if (ssl_info.mode != 0) {
    if (!uri_tmp.empty())
      uri_tmp.append("&");
    uri_tmp.append(kSslMode).append("=").append(shcore::MapSslModeNameToValue::get_value(ssl_info.mode));
  }

  if (!ssl_info.tls_version.is_null()) {
    if (!uri_tmp.empty())
      uri_tmp.append("&");
    uri_tmp.append(kSslTlsVersion).append("=").append(*ssl_info.tls_version);
  }

  if (!uri_tmp.empty())  {
    uri.append("?");
    uri.append(uri_tmp);
  }
}

void parse_mysql_connstring(const std::string &connstring,
                            std::string &scheme, std::string &user, std::string &password,
                            std::string &host, int &port, std::string &sock,
                            std::string &db, int &pwd_found, struct mysqlshdk::utils::Ssl_info& ssl_info,
                            bool set_defaults) {
  try {
    uri::Uri_parser parser;
    uri::Uri_data data = parser.parse(connstring);

    scheme = data.get_scheme();
    user = data.get_user();
    host = data.get_host();

    switch (data.get_type()) {
      case uri::Tcp:
        if (data.has_port())
          port = data.get_port();
        break;
      case uri::Socket:
        sock = data.get_socket();
        break;
      case uri::Pipe:
        sock = data.get_pipe();
        break;
    }

    db = data.get_db();

    pwd_found = data.has_password();
    if (pwd_found)
      password = data.get_password();

    ssl_info.skip = false;

    if (data.has_attribute(kSslCa))
      ssl_info.ca = data.get_attribute(kSslCa);

    if (data.has_attribute(kSslCert))
      ssl_info.cert = data.get_attribute(kSslCert);

    if (data.has_attribute(kSslKey))
      ssl_info.key = data.get_attribute(kSslKey);

    if (data.has_attribute(kSslCaPath))
      ssl_info.capath = data.get_attribute(kSslCaPath);

    if (data.has_attribute(kSslCrl))
      ssl_info.crl = data.get_attribute(kSslCrl);

    if (data.has_attribute(kSslCrlPath))
      ssl_info.crlpath = data.get_attribute(kSslCrlPath);

    if (data.has_attribute(kSslCiphers))
      ssl_info.ciphers = data.get_attribute(kSslCiphers);

    if (data.has_attribute(kSslTlsVersion))
      ssl_info.tls_version = data.get_attribute(kSslTlsVersion);

    if (data.has_attribute(kSslMode)) {
      int mode = shcore::MapSslModeNameToValue::get_value(data.get_attribute(kSslMode));
      if (mode != 0) {
        ssl_info.mode = mode;
      }
      else {
        throw std::runtime_error(shcore::str_format(
          "Invalid value for '%s' (must be any of [DISABLED, PREFERRED, REQUIRED, VERIFY_CA, VERIFY_IDENTITY] ) ",
          data.get_attribute(kSslMode).c_str()));
      }
    }

    if (set_defaults) {
      if (user.empty())
        user = get_system_user();
    }
  } catch (std::exception &err) {
    throw Exception::argument_error(err.what());
  }
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

bool SHCORE_PUBLIC validate_uri(const std::string &uri) {
  std::string uri_protocol;
  std::string uri_user;
  std::string uri_password;
  std::string uri_host;
  int uri_port = 0;
  std::string uri_sock;
  std::string uri_database;
  struct mysqlshdk::utils::Ssl_info ssl_info;
  int pwd_found = 0;

  bool ret_val = false;

  if (!uri.empty()) {
    try {
      parse_mysql_connstring(uri, uri_protocol, uri_user, uri_password, uri_host, uri_port, uri_sock, uri_database, pwd_found,
                                       ssl_info);

      ret_val = true;
    } catch (std::exception &e) {
      //TODO: Log error
    }
  }

  return ret_val;
}

// Builds a connection data dictionary using the URI
Value::Map_type_ref get_connection_data(const std::string &uri, bool set_defaults) {
  // NOTE: protocol is left in case an URI still uses it, however, it is ignored everywhere
  std::string uri_scheme;
  std::string uri_user;
  std::string uri_password;
  std::string uri_host;
  int uri_port = 0;
  std::string uri_sock;
  std::string uri_database;
  struct mysqlshdk::utils::Ssl_info uri_ssl_info;
  int pwd_found = 0;

  // Creates the connection dictionary
  Value::Map_type_ref ret_val(new shcore::Value::Map_type);

  // Parses the URI if provided
  if (!uri.empty()) {
    parse_mysql_connstring(uri, uri_scheme, uri_user, uri_password, uri_host, uri_port, uri_sock, uri_database, pwd_found,
                           uri_ssl_info, set_defaults);

    if (!uri_scheme.empty())
      (*ret_val)[kScheme] = Value(uri_scheme);

    if (!uri_user.empty())
      (*ret_val)[kDbUser] = Value(uri_user);

    if (!uri_host.empty())
      (*ret_val)[kHost] = Value(uri_host);

    if (uri_port != 0)
      (*ret_val)[kPort] = Value(uri_port);

    if (pwd_found)
      (*ret_val)[kDbPassword] = Value(uri_password);

    if (!uri_database.empty())
      (*ret_val)[kSchema] = Value(uri_database);

    if (!uri_sock.empty())
      (*ret_val)[kSocket] = Value(uri_sock);

    if (!uri_ssl_info.ca.is_null())
      (*ret_val)[kSslCa] = Value(*uri_ssl_info.ca);

    if (!uri_ssl_info.cert.is_null())
      (*ret_val)[kSslCert] = Value(*uri_ssl_info.cert);

    if (!uri_ssl_info.key.is_null())
      (*ret_val)[kSslKey] = Value(*uri_ssl_info.key);

    if (!uri_ssl_info.capath.is_null())
      (*ret_val)[kSslCaPath] = Value(*uri_ssl_info.capath);

    if (!uri_ssl_info.crl.is_null())
      (*ret_val)[kSslCrl] = Value(*uri_ssl_info.crl);

    if (!uri_ssl_info.crlpath.is_null())
      (*ret_val)[kSslCrlPath] = Value(*uri_ssl_info.crlpath);

    if (!uri_ssl_info.ciphers.is_null())
      (*ret_val)[kSslCiphers] = Value(*uri_ssl_info.ciphers);

    if (!uri_ssl_info.tls_version.is_null())
      (*ret_val)[kSslTlsVersion] = Value(*uri_ssl_info.tls_version);

    if (uri_ssl_info.mode != 0)
      (*ret_val)[kSslMode] = Value(shcore::MapSslModeNameToValue::get_value(uri_ssl_info.mode));
  }

  // If needed we construct the URi from the individual parameters
  return ret_val;
}

// Overrides connection data parameters with specific values, also adds parameters with default values if missing
void update_connection_data(Value::Map_type_ref data,
                            const std::string &user, const char *password,
                            const std::string &host, int &port, const std::string& sock,
                            const std::string &database,
                            bool ssl, const struct mysqlshdk::utils::Ssl_info& ssl_info,
                            const std::string &auth_method) {
  if (!user.empty())
    (*data)[kDbUser] = Value(user);

  if (!host.empty())
    (*data)[kHost] = Value(host);

  if (port != 0)
    (*data)[kPort] = Value(port);

  if (password)
    (*data)[kDbPassword] = Value(password);

  if (!database.empty())
    (*data)[kSchema] = Value(database);

  if (!sock.empty())
    (*data)[kSocket] = Value(sock);

  if (ssl) {
    if (!ssl_info.ca.is_null())
      (*data)[kSslCa] = Value(*ssl_info.ca);

    if (!ssl_info.cert.is_null())
      (*data)[kSslCert] = Value(*ssl_info.cert);

    if (!ssl_info.key.is_null())
      (*data)[kSslKey] = Value(*ssl_info.key);

    if (!ssl_info.capath.is_null())
      (*data)[kSslCaPath] = Value(*ssl_info.capath);

    if (!ssl_info.crl.is_null())
      (*data)[kSslCrl] = Value(*ssl_info.crl);

    if (!ssl_info.crlpath.is_null())
      (*data)[kSslCrlPath] = Value(*ssl_info.crlpath);

    if (!ssl_info.ciphers.is_null())
      (*data)[kSslCiphers] = Value(*ssl_info.ciphers);

    if (!ssl_info.tls_version.is_null())
      (*data)[kSslTlsVersion] = Value(*ssl_info.tls_version);

    if (ssl_info.mode != 0)
      (*data)[kSslMode] = Value(shcore::MapSslModeNameToValue::get_value(ssl_info.mode));
  } else {
    data->erase(kSslCa);
    data->erase(kSslCert);
    data->erase(kSslKey);
    data->erase(kSslCaPath);
    data->erase(kSslCrl);
    data->erase(kSslCrlPath);
    data->erase(kSslCiphers);
    data->erase(kSslTlsVersion);
    data->erase(kSslMode);
  }

  if (!auth_method.empty())
    (*data)[kAuthMethod] = Value(auth_method);
}

void set_default_connection_data(Value::Map_type_ref data) {
  // Default values
  if (!data->has_key(kDbUser)) {
    std::string username = get_system_user();

    if (!username.empty())
      (*data)[kDbUser] = Value(username);
  }

  if (!data->has_key(kHost) && !data->has_key(kSocket))
    (*data)[kHost] = Value("localhost");
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




std::vector<std::string> split_string(const std::string& input, const std::string& separator, bool compress) {
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
    } else
      ret_val.push_back(input.substr(index));
  }

  return ret_val;
}

std::vector<std::string> SHCORE_PUBLIC split_string(const std::string& input, std::vector<size_t> max_lengths) {
  std::vector<std::string> chunks;

  size_t index = max_lengths[0];
  size_t last_length = max_lengths[0];
  size_t start = 0;

  // Index will eventually overpass the input size
  // As lines are added
  while (input.size() > index) {
    auto pos = input.rfind(" ", index);

    if (pos != std::string::npos && pos > start) {
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
    } else
      ret_val.push_back(input.substr(index));
  }

  return ret_val;
}

// Retrieves a member name on a specific NamingStyle
// NOTE: Assumption is given that everything is created using a lowerUpperCase naming style
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

  if (s.size() - p <= 0)
    return p;

  char quote = s[p];
  if (quote != '\'' && quote != '"') {
    // check if valid initial char
    if (!std::isalpha(quote) && quote != '_' && quote != '$')
      throw std::runtime_error("Invalid character in identifier");
    quote = 0;
  } else {
    p++;
  }

  if (quote == 0) {
    while (p < s.size()) {
      if (!std::isalnum(s[p]) && s[p] != '_' && s[p] != '$')
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
    if (!done && esc == quote)
      done = true;
    else if (!done) {
      throw std::runtime_error("Invalid syntax in identifier");
    }
  }
  return p;
}

/** Split a MySQL account string (in the form user@host) into its username and
 *  hostname components. The returned strings will be unquoted.
 *
 *  Includes correct handling for quotes (e.g. 'my@user'@'192.168.%')
 */
void split_account(const std::string& account, std::string *out_user, std::string *out_host) {
  std::size_t pos = 0;
  if (out_user) *out_user = "";
  if (out_host) *out_host = "";

  pos = span_quotable_identifier(account, 0, out_user);
  if (account[pos] == '@') {
    pos = span_quotable_identifier(account, pos+1, out_host);
  }
  if (pos < account.size())
    throw std::runtime_error("Invalid syntax in account name '"+account+"'");
}


/** Join MySQL account components into a string suitable for use with GRANT
 *  and similar
 */
std::string make_account(const std::string& user, const std::string &host) {
  return shcore::sqlstring("?@?", 0) << user << host;
}
} // namespace
