/* Copyright (c) 2014, 2015, Oracle and/or its affiliates. All rights reserved.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; version 2 of the License.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */

#include "utils/uri_parser.h"
#include <gtest/gtest.h>

namespace shcore
{
#define NO_SCHEMA ""
#define NO_USER ""
#define NO_PASSWORD ""
#define NO_HOST ""
#define NO_PORT 0
#define NO_SOCK ""
#define NO_DB ""
#define HAS_PASSWORD true
#define HAS_NO_PASSWORD false
#define HAS_PORT true
#define HAS_NO_PORT false
#define NO_ERROR ""

  namespace proj_parser_tests
  {
    void validate_uri(const std::string &connstring,
                      const std::string &schema, const std::string &user, const std::string &password,
                      const std::string &host, const int &port, const std::string &sock,
                      const std::string &db, bool has_password, bool has_port, const std::string &error = "", const std::map<std::string, std::string> *attributes = 0)
    {
      SCOPED_TRACE(connstring);

      Uri_parser parser;

      try
      {
        parser.parse(connstring);

        ASSERT_STREQ(schema.c_str(), parser.get_scheme().c_str());
        ASSERT_STREQ(user.c_str(), parser.get_user().c_str());
        ASSERT_EQ(has_password, parser.has_password());

        if (has_password)
          ASSERT_STREQ(password.c_str(), parser.get_password().c_str());

        ASSERT_STREQ(host.c_str(), parser.get_host().c_str());

        ASSERT_EQ(has_port, parser.has_port());

        if (has_port)
          ASSERT_EQ(port, parser.get_port());

        ASSERT_STREQ(sock.c_str(), parser.get_socket().c_str());
        ASSERT_STREQ(db.c_str(), parser.get_db().c_str());

        if (attributes)
        {
          for (auto att : *attributes)
          {
            SCOPED_TRACE(att.first);
            ASSERT_STREQ(att.second.c_str(), parser.get_attribute(att.first).c_str());
          }
        }
      }
      catch (std::runtime_error& err)
      {
        std::string found_error(err.what());
        if (found_error.find(error) == std::string::npos)
        {
          SCOPED_TRACE("ACTUAL: " + found_error);
          SCOPED_TRACE("EXPECTED: " + error);
          FAIL();
        }
      }
    }

    TEST(Uri_parser, parse_scheme)
    {
      validate_uri("mysqlx://mysql.com", "mysqlx", NO_USER, NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("mysql://mysql.com", "mysql", NO_USER, NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("mysql.com", NO_SCHEMA, NO_USER, NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("://mysql.com", NO_SCHEMA, NO_USER, NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, "Scheme is missing");
      validate_uri("other://mysql.com", NO_SCHEMA, NO_USER, NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, "Invalid scheme [other], supported schemes include: mysql, mysqlx");
      validate_uri("mysqlx ://mysql.com", NO_SCHEMA, NO_USER, NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, "Illegal space found at position 6");
      validate_uri("mysq=lx://mysql.com", NO_SCHEMA, NO_USER, NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, "Illegal character [=] found at position 4");
      validate_uri("mysqlx+ssh+other://mysql.com", NO_SCHEMA, NO_USER, NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, "Invalid scheme format [mysqlx+ssh+other], only one extension is supported");
      validate_uri("mysqlx+ssh://mysql.com", NO_SCHEMA, NO_USER, NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, "Scheme extension [ssh] is not supported");
    }

    TEST(Uri_parser, parse_user_info)
    {
      validate_uri("mysqlx://user@mysql.com", "mysqlx", "user", NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("mysqlx://@mysql.com", "mysqlx", "user", NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, "Missing user information");
      validate_uri("mysqlx://user.name@mysql.com", "mysqlx", "user.name", NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("mysqlx://user-name@mysql.com", "mysqlx", "user-name", NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("mysqlx ://user_name@mysql.com", "mysqlx", "user_name", NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("mysqlx://user~name@mysql.com", "mysqlx", "user~name", NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("mysqlx://user!name@mysql.com", "mysqlx", "user!name", NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("mysqlx://user$name@mysql.com", "mysqlx", "user$name", NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("mysqlx://user&name@mysql.com", "mysqlx", "user&name", NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("mysqlx://user'name@mysql.com", "mysqlx", "user'name", NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("mysqlx://user(name)@mysql.com", "mysqlx", "user(name)", NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("mysqlx://user*name@mysql.com", "mysqlx", "user*name", NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("mysqlx://user+name@mysql.com", "mysqlx", "user+name", NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("mysqlx://user,name@mysql.com", "mysqlx", "user,name", NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("mysqlx://user;name@mysql.com", "mysqlx", "user;name", NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("mysqlx://user=name@mysql.com", "mysqlx", "user=name", NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("mysqlx://user%name@mysql.com", "mysqlx", "user%name", NO_PASSWORD, "does not matter", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, "Illegal character [%] found at position 13");
      validate_uri("mysqlx://user%1name@mysql.com", "mysqlx", "user%1name", NO_PASSWORD, "does not matter", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, "Illegal character [%] found at position 13");
      validate_uri("mysqlx://user%2Aname@mysql.com", "mysqlx", "user%2Aname", NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("mysqlx://user%2Aname:password@mysql.com", "mysqlx", "user%2Aname", "password", "mysql.com", NO_PORT, NO_SOCK, NO_DB, HAS_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("mysqlx://user%2Aname:!@#$%^&*@mysql.com", "mysqlx", "user%2Aname", "!@#$%^&*", "mysql.com", NO_PORT, NO_SOCK, NO_DB, HAS_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("mysqlx://:!@#$%^&*@mysql.com", "mysqlx", "user%2Aname", "!@#$%^&*", "mysql.com", NO_PORT, NO_SOCK, NO_DB, HAS_PASSWORD, HAS_NO_PORT, "Missing user information");
    }

    TEST(Uri_parser, parse_host_info)
    {
      validate_uri("mysqlx://user@mysql.com", "mysqlx", "user", NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("mysqlx://user@mysql1.com", "mysqlx", "user", NO_PASSWORD, "mysql1.com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("mysqlx://user@mysql1-com", "mysqlx", "user", NO_PASSWORD, "mysql1-com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("mysqlx://user@mysql1~com", "mysqlx", "user", NO_PASSWORD, "mysql1~com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("mysqlx://user@mysql1_com", "mysqlx", "user", NO_PASSWORD, "mysql1_com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("mysqlx://user@mysql1!com", "mysqlx", "user", NO_PASSWORD, "mysql1!com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("mysqlx://user@mysql1$com", "mysqlx", "user", NO_PASSWORD, "mysql1$com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("mysqlx://user@mysql1&com", "mysqlx", "user", NO_PASSWORD, "mysql1&com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("mysqlx://user@mysql1'com", "mysqlx", "user", NO_PASSWORD, "mysql1'com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("mysqlx://user@mysql1(com)", "mysqlx", "user", NO_PASSWORD, "mysql1(com)", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("mysqlx://user@mysql1*com", "mysqlx", "user", NO_PASSWORD, "mysql1*com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("mysqlx://user@mysql1+com", "mysqlx", "user", NO_PASSWORD, "mysql1+com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("mysqlx://user@mysql1,com", "mysqlx", "user", NO_PASSWORD, "mysql1,com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("mysqlx://user@mysql1;com", "mysqlx", "user", NO_PASSWORD, "mysql1;com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("mysqlx://user@mysql1=com", "mysqlx", "user", NO_PASSWORD, "mysql1=com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("mysqlx://user@mysql1%com", "mysqlx", "user", NO_PASSWORD, "does not matter", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, "Illegal character [%] found at position 20");
      validate_uri("mysqlx://user@mysql1%cgcom", "mysqlx", "user", NO_PASSWORD, "does not matter", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, "Illegal character [%] found at position 20");
      validate_uri("mysqlx://user@mysql1%cdcom", "mysqlx", "user", NO_PASSWORD, "mysql1%cdcom", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, NO_ERROR);

      validate_uri("mysqlx://user@mysql1%cdcom:2845", "mysqlx", "user", NO_PASSWORD, "mysql1%cdcom", 2845, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_PORT, NO_ERROR);
      validate_uri("mysqlx://user@mysql1%cdcom:65538", "mysqlx", "user", NO_PASSWORD, "mysql1%cdcom", 2845, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_PORT, "Port is out of the valid range: 0 - 65536");
      validate_uri("mysqlx://user@mysql1%cdcom:", "mysqlx", "user", NO_PASSWORD, "mysql1%cdcom", 2845, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_PORT, "Missing port number");
      validate_uri("mysqlx://user@mysql1%cdcom:2845f", "mysqlx", "user", NO_PASSWORD, "mysql1%cdcom", 2845, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, "Illegal character after port definition at position 31");
      validate_uri("mysqlx://user@mysql1%cdcom:invalid", "mysqlx", "user", NO_PASSWORD, "mysql1%cdcom", 2845, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, "Invalid port definition");

      validate_uri("mysqlx://user@10.150.123.45", "mysqlx", "user", NO_PASSWORD, "10.150.123.45", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("mysqlx://user@10.150.123", "mysqlx", "user", NO_PASSWORD, "10.150.123", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("mysqlx://user@10.150.123.whatever", "mysqlx", "user", NO_PASSWORD, "10.150.123.whatever", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("mysqlx://user@0.0.0.0", "mysqlx", "user", NO_PASSWORD, "0.0.0.0", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("mysqlx://user@255.255.255.255", "mysqlx", "user", NO_PASSWORD, "255.255.255.255", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, NO_ERROR);
      validate_uri("mysqlx://user@256.255.255.255", "mysqlx", "user", NO_PASSWORD, "255.255.255.255", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, "Octect value out of bounds, valid range for IPv4 is 0 to 255");
      validate_uri("mysqlx://user@255.256.255.255", "mysqlx", "user", NO_PASSWORD, "255.255.255.255", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, "Octect value out of bounds, valid range for IPv4 is 0 to 255");
      validate_uri("mysqlx://user@255.255.256.255", "mysqlx", "user", NO_PASSWORD, "255.255.255.255", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, "Octect value out of bounds, valid range for IPv4 is 0 to 255");
      validate_uri("mysqlx://user@255.255.255.256", "mysqlx", "user", NO_PASSWORD, "255.255.255.255", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, "Octect value out of bounds, valid range for IPv4 is 0 to 255");

      validate_uri("mysqlx://user@10.150.123.45:2845", "mysqlx", "user", NO_PASSWORD, "10.150.123.45", 2845, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_PORT, NO_ERROR);
      validate_uri("mysqlx://user@10.150.123.45:68000", "mysqlx", "user", NO_PASSWORD, "10.150.123.45", 2845, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_PORT, "Port is out of the valid range: 0 - 65536");
      validate_uri("mysqlx://user@10.150.123.45:", "mysqlx", "user", NO_PASSWORD, "10.150.123.45", 2845, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_PORT, "Missing port number");
      validate_uri("mysqlx://user@10.150.123.45:2845f", "mysqlx", "user", NO_PASSWORD, "10.150.123.45", 2845, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, "Illegal character after port definition at position 32");
      validate_uri("mysqlx://user@10.150.123.45:invalid", "mysqlx", "user", NO_PASSWORD, "10.150.123.45", 2845, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT, "Invalid port definition");
    }

    TEST(Uri_parser, parse_path)
    {
      validate_uri("mysqlx://user@10.150.123.45:2845/", "mysqlx", "user", NO_PASSWORD, "10.150.123.45", 2845, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_PORT, NO_ERROR);
      validate_uri("mysqlx://user@10.150.123.45:2845/world", "mysqlx", "user", NO_PASSWORD, "10.150.123.45", 2845, NO_SOCK, "world", HAS_NO_PASSWORD, HAS_PORT, NO_ERROR);
      validate_uri("mysqlx://user@10.150.123.45:2845/world@x", "mysqlx", "user", NO_PASSWORD, "10.150.123.45", 2845, NO_SOCK, "world@x", HAS_NO_PASSWORD, HAS_PORT, NO_ERROR);
      validate_uri("mysqlx://user@10.150.123.45:2845/world:x", "mysqlx", "user", NO_PASSWORD, "10.150.123.45", 2845, NO_SOCK, "world:x", HAS_NO_PASSWORD, HAS_PORT, NO_ERROR);
      validate_uri("mysqlx://user@10.150.123.45:2845/world%x", "mysqlx", "user", NO_PASSWORD, "10.150.123.45", 2845, NO_SOCK, "world%x", HAS_NO_PASSWORD, HAS_PORT, "Illegal character [%] found at position 38");
      validate_uri("mysqlx://user@10.150.123.45:2845/world%2x", "mysqlx", "user", NO_PASSWORD, "10.150.123.45", 2845, NO_SOCK, "world%2x", HAS_NO_PASSWORD, HAS_PORT, "Illegal character [%] found at position 38");
      validate_uri("mysqlx://user@10.150.123.45:2845/world%20x", "mysqlx", "user", NO_PASSWORD, "10.150.123.45", 2845, NO_SOCK, "world%20x", HAS_NO_PASSWORD, HAS_PORT, NO_ERROR);
      validate_uri("mysqlx://user@10.150.123.45:2845/world%20x/subpath", "mysqlx", "user", NO_PASSWORD, "10.150.123.45", 2845, NO_SOCK, "subpath", HAS_NO_PASSWORD, HAS_PORT, "Multiple levels defined on the path component");
    }

    TEST(Uri_parser, parse_query)
    {
      std::map<std::string, std::string> atts;
      atts["sample"] = "value";
      atts["sample2"] = "whatever";
      atts["sample3"] = "";
      atts["sample4"] = "[one,two,three]";
      atts["sample5"] = "(/what/e/ver)";
      validate_uri("mysqlx://user@10.150.123.45:2845/world?sample=value&sample2=whatever&sample3&sample4=[one,two,three], sample5=(/what/e/ver)", "mysqlx", "user", NO_PASSWORD, "10.150.123.45", 2845, NO_SOCK, "world", HAS_NO_PASSWORD, HAS_PORT, NO_ERROR, &atts);
    }
  };
};