/*
* Copyright (c) 2014, 2016, Oracle and/or its affiliates. All rights reserved.
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

#include "utils_general.h"
#include "uri_parser.h"
#include "utils_file.h"
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
#endif

#include <boost/format.hpp>

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
    if (data->has_key("dbUser"))
      uri.append((*data)["dbUser"].as_string());
    else if (data->has_key("user"))
      uri.append((*data)["user"].as_string());

    // Appends password definition, either if it is empty or not
    // only if a user was specified
    if (with_password && !uri.empty()) {
      uri.append(":");
      if (data->has_key("dbPassword"))
        uri.append((*data)["dbPassword"].as_string());
      else if (data->has_key("password"))
        uri.append((*data)["password"].as_string());
    }

    // Appends the user@host separator, if a user has specified
    if (!uri.empty())
      uri.append("@");

    // Sets the socket
    if (data->has_key("sock"))
      uri.append((*data)["sock"].as_string());

    else{ // the uri either has a socket, or an hostname and port
      // Sets the host
      if (data->has_key("host"))
        uri.append((*data)["host"].as_string());

      // Sets the port
      if (data->has_key("port")) {
        uri.append(":");
        uri.append((*data)["port"].descr(true));
      }
    }

    // Sets the database
    if (data->has_key("schema")) {
      uri.append("/");
      uri.append((*data)["schema"].as_string());
    }

    if (data->has_key("sslCa") && data->has_key("sslCert") && data->has_key("sslKey"))
      conn_str_cat_ssl_data(uri, (*data)["sslCa"].as_string(), (*data)["sslCert"].as_string(), (*data)["sslKey"].as_string());
  }

  return uri;
}

void conn_str_cat_ssl_data(std::string& uri, const std::string& ssl_ca, const std::string& ssl_cert, const std::string& ssl_key) {
  if (!uri.empty() && (!ssl_ca.empty() || !ssl_cert.empty() || !ssl_key.empty())) {
    bool first = false;
    int cnt = 0;
    if (!ssl_ca.empty()) ++cnt;
    if (!ssl_cert.empty()) ++cnt;
    if (!ssl_key.empty()) ++cnt;

    if (!ssl_ca.empty() && uri.rfind("ssl_ca=") == std::string::npos) {
      if (!first) {
        first = true;
        uri.append("?");
      }
      uri.append("ssl_ca=").append(ssl_ca);
      if (--cnt) uri.append("&");
    }
    if (!ssl_cert.empty() && uri.rfind("ssl_cert=") == std::string::npos) {
      if (!first) {
        first = true;
        uri.append("?");
      }
      uri.append("ssl_cert=").append(ssl_cert);
      if (--cnt) uri.append("&");
    }
    if (!ssl_key.empty() && uri.rfind("ssl_key=") == std::string::npos) {
      if (!first) {
        first = true;
        uri.append("?");
      }
      uri.append("ssl_key=").append(ssl_key);
    }
  }
}

void parse_mysql_connstring(const std::string &connstring,
                            std::string &scheme, std::string &user, std::string &password,
                            std::string &host, int &port, std::string &sock,
                            std::string &db, int &pwd_found, std::string& ssl_ca, std::string& ssl_cert, std::string& ssl_key,
                            bool set_defaults) {
  try {
    uri::Uri_parser parser;
    uri::Uri_data data = parser.parse(connstring);

    scheme = data.get_scheme();
    user = data.get_user();

    switch (data.get_type()) {
      case uri::Tcp:
        host = data.get_host();
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

    if (data.has_attribute("sslCa"))
      ssl_ca = data.get_attribute("sslCa");

    if (data.has_attribute("sslCert"))
      ssl_cert = data.get_attribute("sslCert");

    if (data.has_attribute("sslKey"))
      ssl_key = data.get_attribute("sslKey");

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
    //XXX find out current username here
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

std::string strip_ssl_args(const std::string &connstring) {
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
}

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
  std::string uri_ssl_ca;
  std::string uri_ssl_cert;
  std::string uri_ssl_key;
  int pwd_found = 0;

  bool ret_val = false;

  if (!uri.empty()) {
    try {
      parse_mysql_connstring(uri, uri_protocol, uri_user, uri_password, uri_host, uri_port, uri_sock, uri_database, pwd_found,
                                       uri_ssl_ca, uri_ssl_cert, uri_ssl_key);

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
  std::string uri_ssl_ca;
  std::string uri_ssl_cert;
  std::string uri_ssl_key;
  int pwd_found = 0;

  // Creates the connection dictionary
  Value::Map_type_ref ret_val(new shcore::Value::Map_type);

  // Parses the URI if provided
  if (!uri.empty()) {
    parse_mysql_connstring(uri, uri_scheme, uri_user, uri_password, uri_host, uri_port, uri_sock, uri_database, pwd_found,
                           uri_ssl_ca, uri_ssl_cert, uri_ssl_key, set_defaults);

    if (!uri_scheme.empty())
      (*ret_val)["scheme"] = Value(uri_scheme);

    if (!uri_user.empty())
      (*ret_val)["dbUser"] = Value(uri_user);

    if (!uri_host.empty())
      (*ret_val)["host"] = Value(uri_host);

    if (uri_port != 0)
      (*ret_val)["port"] = Value(uri_port);

    if (pwd_found)
      (*ret_val)["dbPassword"] = Value(uri_password);

    if (!uri_database.empty())
      (*ret_val)["schema"] = Value(uri_database);

    if (!uri_sock.empty())
      (*ret_val)["sock"] = Value(uri_sock);

    if (!uri_ssl_ca.empty())
      (*ret_val)["sslCa"] = Value(uri_ssl_ca);

    if (!uri_ssl_cert.empty())
      (*ret_val)["sslCert"] = Value(uri_ssl_cert);

    if (!uri_ssl_key.empty())
      (*ret_val)["sslKey"] = Value(uri_ssl_key);
  }

  // If needed we construct the URi from the individual parameters
  return ret_val;
}

// Overrides connection data parameters with specific values, also adds parameters with default values if missing
void update_connection_data(Value::Map_type_ref data,
                            const std::string &user, const char *password,
                            const std::string &host, int &port, const std::string& sock,
                            const std::string &database,
                            bool ssl, const std::string &ssl_ca,
                            const std::string &ssl_cert, const std::string &ssl_key,
                            const std::string &auth_method) {
  if (!user.empty())
    (*data)["dbUser"] = Value(user);

  if (!host.empty())
    (*data)["host"] = Value(host);

  if (port != 0)
    (*data)["port"] = Value(port);

  if (password)
    (*data)["dbPassword"] = Value(password);

  if (!database.empty())
    (*data)["schema"] = Value(database);

  if (!sock.empty())
    (*data)["sock"] = Value(sock);

  if (ssl) {
    if (!ssl_ca.empty())
      (*data)["sslCa"] = Value(ssl_ca);

    if (!ssl_cert.empty())
      (*data)["sslCert"] = Value(ssl_cert);

    if (!ssl_key.empty())
      (*data)["sslKey"] = Value(ssl_key);
  } else {
    data->erase("sslCa");
    data->erase("sslCert");
    data->erase("sslKey");
  }

  if (!auth_method.empty())
    (*data)["authMethod"] = Value(auth_method);
}

void set_default_connection_data(Value::Map_type_ref data) {
  // Default values
  if (!data->has_key("dbUser")) {
    std::string username = get_system_user();

    if (!username.empty())
      (*data)["dbUser"] = Value(username);
  }

  if (!data->has_key("host") && !data->has_key("sock"))
    (*data)["host"] = Value("localhost");
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
  char username[30];
  if (!getlogin_r(username, sizeof(username)))
    ret_val.assign(username);
#endif

  return ret_val;
}

/* MySql SSL APIs require to have the ca & ca path in separete args, this function takes care of that normalization */
void normalize_sslca_args(std::string &ssl_ca, std::string &ssl_ca_path) {
  if (!ssl_ca_path.empty()) return;
  std::string ca_path(ssl_ca);
  std::string::size_type p;
#ifdef WIN32
  p = ca_path.rfind("\\");
#else
  p = ca_path.rfind("/");
#endif
  if (p != std::string::npos) {
    // The function int SSL_CTX_load_verify_locations(SSL_CTX* ctx, const char* file, const char* path)
    // requires the ssl_ca to also keep the path
    ssl_ca_path = ca_path.substr(0, p);
  } else {
    ssl_ca_path = "";
    ssl_ca = ca_path.c_str();
  }
}

std::vector<std::string> split_string(const std::string& input, const std::string& separator, bool compress) {
  std::vector<std::string> ret_val;

  size_t index = 0, new_find = 0;

  while (new_find != std::string::npos) {
    new_find = input.find(separator, index);

    if (new_find != std::string::npos) {
      // When compress is enabled, consecutive separators
      // do not generate new elements
      if (new_find > index || !compress)
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

std::string join_strings(const std::set<std::string>& strings, const std::string& separator) {
  std::set<std::string> input(strings);
  std::string ret_val;

  ret_val += *input.begin();

  input.erase(input.begin());

  for (auto item : input)
    ret_val += separator + item;

  return ret_val;
}

std::string join_strings(const std::vector<std::string>& strings, const std::string& separator) {
  std::vector<std::string> input(strings);
  std::string ret_val;

  if (!input.empty()) {
    ret_val += *input.begin();

    input.erase(input.begin());

    for (auto item : input)
      ret_val += separator + item;
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
  int ret, family, addrlen;

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
} // namespace
