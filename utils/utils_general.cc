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
#include "utils_file.h"
#include "shellcore/server_registry.h"
#include <locale>
#ifdef WIN32
#include <windows.h>
#include <Lmcons.h>
#else
#include <unistd.h>
#endif

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

  std::string build_connection_string(Value::Map_type_ref data, bool with_password)
  {
    std::string uri;

    // If needed we construct the URi from the individual parameters
    {
      if (data->has_key("dbUser"))
      uri.append((*data)["dbUser"].as_string());

      // Appends password definition, either if it is empty or not
      if (with_password)
      {
        uri.append(":");
        if (data->has_key("dbPassword"))
          uri.append((*data)["dbPassword"].as_string());
      }

      uri.append("@");

      // Sets the host
      if (data->has_key("host"))
        uri.append((*data)["host"].as_string());

      // Sets the port
      if (data->has_key("port"))
      {
        uri.append(":");
        uri.append((*data)["port"].descr(true));
      }

      // Sets the database
      if (data->has_key("schema"))
      {
        uri.append("/");
        uri.append((*data)["schema"].as_string());
      }

      if (data->has_key("ssl_ca") && data->has_key("ssl_cert") && data->has_key("ssl_key"))
        conn_str_cat_ssl_data(uri, (*data)["ssl_ca"].as_string(), (*data)["ssl_cert"].as_string(), (*data)["ssl_key"].as_string());
    }

    return uri;
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

  void parse_mysql_connstring(const std::string &connstring,
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
      {
        std::string str_port = server_part.substr(0, p);
        if (!sscanf(str_port.c_str(), "%i", &port))
          throw Exception::argument_error((boost::format("Invalid value found for port component: %1%") % str_port).str());
      }
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

  bool SHCORE_PUBLIC validate_uri(const std::string &uri)
  {
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

    if (!uri.empty())
    {
      try
      {
        parse_mysql_connstring(uri, uri_protocol, uri_user, uri_password, uri_host, uri_port, uri_sock, uri_database, pwd_found,
                                         uri_ssl_ca, uri_ssl_cert, uri_ssl_key);

        ret_val = true;
      }
      catch (std::exception &e)
      {
        //TODO: Log error
      }
    }

    return ret_val;
  }

  // Builds a connection data dictionary using the URI
  Value::Map_type_ref get_connection_data(const std::string &uri)
  {
    // NOTE: protocol is left in case an URI still uses it, however, it is ignored everywhere
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

    // Creates the connection dictionary
    Value::Map_type_ref ret_val(new shcore::Value::Map_type);

    // Parses the URI if provided
    if (!uri.empty())
    {
      try
      {
        parse_mysql_connstring(uri, uri_protocol, uri_user, uri_password, uri_host, uri_port, uri_sock, uri_database, pwd_found,
                               uri_ssl_ca, uri_ssl_cert, uri_ssl_key);

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
          (*ret_val)["ssl_ca"] = Value(uri_ssl_ca);

        if (!uri_ssl_cert.empty())
          (*ret_val)["ssl_cert"] = Value(uri_ssl_cert);

        if (!uri_ssl_key.empty())
          (*ret_val)["ssl_key"] = Value(uri_ssl_key);
      }
      catch (...)
      {
        // If there's an error simply ignores the parsing
      }
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
                              const std::string &auth_method)
  {
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

    if (ssl)
    {
      if (!ssl_ca.empty())
        (*data)["ssl_ca"] = Value(ssl_ca);

      if (!ssl_cert.empty())
        (*data)["ssl_cert"] = Value(ssl_cert);

      if (!ssl_key.empty())
        (*data)["ssl_key"] = Value(ssl_key);
    }
    else
    {
      data->erase("ssl_ca");
      data->erase("ssl_cert");
      data->erase("ssl_key");
    }

    if (!auth_method.empty())
      (*data)["authMethod"] = Value(auth_method);
  }

  void set_default_connection_data(Value::Map_type_ref data, int defaultPort)
  {
    // Default values
    if (!data->has_key("dbUser"))
    {
      std::string username = get_system_user();

      if (!username.empty())
        (*data)["dbUser"] = Value(get_system_user());
    }

    if (!data->has_key("host"))
      (*data)["host"] = Value("localhost");

    if (!data->has_key("port"))
      (*data)["port"] = Value(defaultPort);
  }

  std::string get_system_user()
  {
    std::string ret_val;

#ifdef WIN32
    char username[UNLEN + 1];
    DWORD username_len = UNLEN + 1;
    if (GetUserName(username, &username_len))
    {
      ret_val.assign(username);
    }
#else
    char username[30];
    if (!getlogin_r(username, sizeof(username)))
      ret_val.assign(username);
#endif

    return ret_val;
  }
}