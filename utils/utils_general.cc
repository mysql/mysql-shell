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
}