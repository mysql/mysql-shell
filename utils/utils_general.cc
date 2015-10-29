/*
* Copyright (c) 2014, 2015, Oracle and/or its affiliates. All rights reserved.
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
#include "utils_file.h"
#include "shellcore/server_registry.h"
#include "shellcore/types.h"
#include <locale>

#include <boost/format.hpp>

namespace shcore
{
  bool is_valid_identifier(const std::string& name)
  {
    bool ret_val = false;

    if (!name.empty())
    {
      std::locale locale;

      ret_val = std::isalpha(name[0], locale);

      size_t index = 1;
      while (ret_val && index < name.size())
      {
        ret_val = std::isalnum(name[index], locale) || name[index] == '_';
        index++;
      }
    }

    return ret_val;
  }

  void build_connection_string(std::string &uri,
    const std::string &uri_protocol, const std::string &uri_user, const std::string &uri_password,
    const std::string &uri_host, int &port,
    const std::string &uri_database, bool prompt_pwd, const std::string &uri_ssl_ca,
    const std::string &uri_ssl_cert, const std::string &uri_ssl_key)
  {
    // If needed we construct the URi from the individual parameters
    {
      // Configures the URI string
      if (!uri_protocol.empty())
      {
        uri.append(uri_protocol);
        uri.append("://");
      }

      // Sets the user and password
      if (!uri_user.empty())
      {
        uri.append(uri_user);

        // If the password will not be prompted and is defined appends both :password
        if (!prompt_pwd)
          uri.append(":").append(uri_password);

        uri.append("@");
      }

      // Sets the host
      if (!uri_host.empty())
        uri.append(uri_host);

      // Sets the port
      if (!uri_host.empty() && port > 0)
        uri.append((boost::format(":%i") % port).str());

      // Sets the database
      if (!uri_database.empty())
      {
        uri.append("/");
        uri.append(uri_database);
      }

      conn_str_cat_ssl_data(uri, uri_ssl_ca, uri_ssl_cert, uri_ssl_key);
    }
  }

  void conn_str_cat_ssl_data(std::string& uri, const std::string& ssl_ca, const std::string& ssl_cert, const std::string& ssl_key)
  {
    if (!uri.empty() && (!ssl_ca.empty() || !ssl_cert.empty() || !ssl_key.empty()))
    {
      bool first = false;
      int cnt = 0;
      if (!ssl_ca.empty()) ++cnt;
      if (!ssl_cert.empty()) ++cnt;
      if (!ssl_key.empty()) ++cnt;

      if (!ssl_ca.empty() && uri.rfind("ssl_ca=") == std::string::npos)
      {
        if (!first)
        {
          first = true;
          uri.append("?");
        }
        uri.append("ssl_ca=").append(ssl_ca);
        if (--cnt) uri.append("&");
      }
      if (!ssl_cert.empty() && uri.rfind("ssl_cert=") == std::string::npos)
      {
        if (!first)
        {
          first = true;
          uri.append("?");
        }
        uri.append("ssl_cert=").append(ssl_cert);
        if (--cnt) uri.append("&");
      }
      if (!ssl_key.empty() && uri.rfind("ssl_key=") == std::string::npos)
      {
        if (!first)
        {
          first = true;
          uri.append("?");
        }
        uri.append("ssl_key=").append(ssl_key);
      }
    }
  }

  bool parse_mysql_connstring(const std::string &connstring,
                              std::string &protocol, std::string &user, std::string &password,
                              std::string &host, int &port, std::string &sock,
                              std::string &db, int &pwd_found, std::string& ssl_ca, std::string& ssl_cert, std::string& ssl_key)
  {
    // format is [user[:pass]]@host[:port][/db] or user[:pass]@::socket[/db], like what cmdline utilities use
    // with SSL [user[:pass]]@host[:port][/db]?ssl_ca=path_to_ca&ssl_cert=path_to_cert&ssl_key=path_to_key
    // with app name format is $app_name
    pwd_found = 0;
    std::string remaining = connstring;
    std::string::size_type p;
    std::string::size_type p_query;
    p = remaining.find("://");
    if (p != std::string::npos)
    {
      protocol = connstring.substr(0, p);
      remaining = remaining.substr(p + 3);
    }

    std::string s = remaining;
    p = remaining.find('/');
    p_query = remaining.find('?');
    if (p != std::string::npos)
    {
      if (p_query == std::string::npos)
        db = remaining.substr(p + 1);
      else
        db = remaining.substr(p + 1, p_query);
      s = remaining.substr(0, p);
    }

    p = s.rfind('@');
    std::string user_part;
    std::string server_part = (p == std::string::npos) ? s : s.substr(p + 1);

    if (p == std::string::npos)
    {
      // by default, connect using the current OS username
#ifdef _WIN32
      //XXX find out current username here
#else
      const char *tmp = getenv("USER");
      user_part = tmp ? tmp : "";
#endif
    }
    else
      user_part = s.substr(0, p);

    if ((p = user_part.find(':')) != std::string::npos)
    {
      user = user_part.substr(0, p);
      password = user_part.substr(p + 1);
      pwd_found = 1;
    }
    else
      user = user_part;

    p = server_part.find(':');
    if (p != std::string::npos)
    {
      host = server_part.substr(0, p);
      server_part = server_part.substr(p + 1);
      p = server_part.find(':');
      if (p != std::string::npos)
        sock = server_part.substr(p + 1);
      else
        if (!sscanf(server_part.substr(0, p).c_str(), "%i", &port))
          return false;
    }
    else
      host = server_part;

    std::map<std::string, std::string> ssl_data;
    ssl_data["ssl_ca"] = "";
    ssl_data["ssl_key"] = "";
    ssl_data["ssl_cert"] = "";
    if (p_query != std::string::npos)
    {
      // Parsing SSL data
      std::string::size_type p_next = p_query;
      do
      {
        ++p_next;
        std::string::size_type p_eq = remaining.find('=', p_next);
        if (p_eq == std::string::npos)
          throw Exception::argument_error((boost::format("Expected '=' in connection string uri starting at pos %d.") % p_next).str());
        const std::string name = remaining.substr(p_next, p_eq - p_next);
        p_next = remaining.find('&', p_next + 1);
        const std::string value = remaining.substr(p_eq + 1, p_next - p_eq - 1);

        if (ssl_data.find(name) == ssl_data.end())
          throw Exception::argument_error((boost::format("Unknown key provided %s in connection string uri (expected any of ssl_ca, ssl_cert, ssl_key)") % name).str());

        ssl_data[name] = value;
      } while (p_next != std::string::npos);
    }
    if (!ssl_data["ssl_ca"].empty())
      ssl_ca = ssl_data["ssl_ca"];
    if (!ssl_data["ssl_cert"].empty())
      ssl_cert = ssl_data["ssl_cert"];
    if (!ssl_data["ssl_key"].empty())
      ssl_key = ssl_data["ssl_key"];

    return true;
  }

  std::string strip_password(const std::string &connstring)
  {
    std::string remaining = connstring;
    std::string password;

    std::string::size_type p;
    p = remaining.find("://");
    if (p != std::string::npos)
    {
      remaining = remaining.substr(p + 3);
    }

    std::string s = remaining;
    p = remaining.find('/');
    if (p != std::string::npos)
    {
      s = remaining.substr(0, p);
    }
    p = s.rfind('@');
    std::string user_part;

    if (p == std::string::npos)
    {
      // by default, connect using the current OS username
#ifdef _WIN32
      //XXX find out current username here
#else
      const char *tmp = getenv("USER");
      user_part = tmp ? tmp : "";
#endif
    }
    else
      user_part = s.substr(0, p);

    if ((p = user_part.find(':')) != std::string::npos)
    {
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

  std::string strip_ssl_args(const std::string &connstring)
  {
    std::string result = connstring;
    std::string::size_type pos;
    if ((pos = result.find("ssl_ca=")) != std::string::npos)
    {
      std::string::size_type pos2 = result.find("&");
      result = result.replace(pos, (pos2 == std::string::npos) ? std::string::npos : pos2 - pos + 1, "");
    }
    if ((pos = result.find("ssl_cert=")) != std::string::npos)
    {
      std::string::size_type pos2 = result.find("&");
      result = result.replace(pos, (pos2 == std::string::npos) ? std::string::npos : pos2 - pos + 1, "");
    }
    if ((pos = result.find("ssl_key=")) != std::string::npos)
    {
      std::string::size_type pos2 = result.find("&");
      result = result.replace(pos, (pos2 == std::string::npos) ? std::string::npos : pos2 - pos + 1, "");
    }
    if (result.at(result.size() - 1) == '?')
    {
      result.resize(result.size() - 1);
    }

    return result;
  }

  char *mysh_get_stdin_password(const char *prompt)
  {
    if (prompt)
    {
      fputs(prompt, stdout);
      fflush(stdout);
    }
    char buffer[128];
    if (fgets(buffer, sizeof(buffer), stdin))
    {
      char *p = strchr(buffer, '\r');
      if (p) *p = 0;
      p = strchr(buffer, '\n');
      if (p) *p = 0;
      return strdup(buffer);
    }
    return NULL;
  }
}