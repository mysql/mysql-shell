/*
 * Copyright (c) 2014, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <optional>
#include "mysqlshdk/libs/db/file_uri.h"
#include "mysqlshdk/libs/db/generic_uri.h"
#include "mysqlshdk/libs/db/uri_parser.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "unittest/gtest_clean.h"

using mysqlshdk::db::Transport_type;
using mysqlshdk::db::uri::Type;
using mysqlshdk::db::uri::Uri_parser;
namespace testing {
std::optional<const char *> no_string;
std::optional<int> no_int;

#define NO_SCHEME no_string
#define NO_SCHEMA no_string
#define NO_USER no_string
#define NO_PASSWORD no_string
#define NO_HOST no_string
#define NO_PORT no_int
#define NO_SOCK no_string
#define NO_DB no_string
#define HAS_PASSWORD true
#define HAS_NO_PASSWORD false
#define HAS_PORT true
#define HAS_NO_PORT false

namespace proj_parser_tests {
void validate_bad_uri(const std::string &connstring, const std::string &error,
                      Type type = Type::DevApi) {
  SCOPED_TRACE(connstring);

  Uri_parser parser(type);

  try {
    switch (type) {
      case Type::DevApi: {
        mysqlshdk::db::Connection_options options;
        parser.parse(connstring, &options);
        break;
      }
      case Type::Ssh: {
        mysqlshdk::ssh::Ssh_connection_options options;
        parser.parse(connstring, &options);
        break;
      }
      case Type::Generic: {
        mysqlshdk::db::uri::Generic_uri options;
        parser.parse(connstring, &options);
        break;
      }
      case Type::File: {
        mysqlshdk::db::uri::File_uri options;
        parser.parse(connstring, &options);
        break;
      }
    }

    if (!error.empty()) {
      SCOPED_TRACE("MISSING ERROR: " + error);
      FAIL();
    }
  } catch (const std::invalid_argument &err) {
    std::string found_error(err.what());

    if (error.empty()) {
      SCOPED_TRACE("UNEXPECTED ERROR: " + found_error);
      FAIL();
    } else if (found_error.find(error) == std::string::npos) {
      SCOPED_TRACE("ACTUAL: " + found_error);
      SCOPED_TRACE("EXPECTED: " + error);
      FAIL();
    }
  }
}

void validate_uri(
    const std::string &connstring, const std::optional<const char *> &scheme,
    const std::optional<const char *> &user,
    const std::optional<const char *> &password,
    const std::optional<const char *> &host, const std::optional<int> &port,
    const std::optional<const char *> &sock,
    const std::optional<const char *> &db, bool /* has_password */,
    bool /* has_port */, Transport_type target_type, bool reverse_check = true,
    bool generic_check = true,
    const std::map<std::string, std::vector<std::string>> *attributes = 0) {
  SCOPED_TRACE(connstring);

  Uri_parser parser;

  try {
    mysqlshdk::db::Connection_options data = parser.parse(connstring);

    // TEST to ensure the URI is re-generated identical after parsed
    if (reverse_check) {
      SCOPED_TRACE("Testing decode DevAPI URI");
      ASSERT_EQ(connstring,
                data.as_uri(mysqlshdk::db::uri::Tokens_mask::all()));

      // TEST to ensure the URI is re-generated identical after parsed using the
      // Generic parser
      if (generic_check) {
        SCOPED_TRACE("Testing decode Generic URI");
        mysqlshdk::db::uri::Generic_uri uri(connstring);
        ASSERT_EQ(connstring,
                  uri.as_uri(mysqlshdk::db::uri::Tokens_mask::all()));
      }
    }

    if (!scheme.has_value())
      ASSERT_FALSE(data.has_scheme());
    else
      ASSERT_STREQ(*scheme, data.get_scheme().c_str());

    if (!user.has_value())
      ASSERT_FALSE(data.has_user());
    else
      ASSERT_STREQ(*user, data.get_user().c_str());

    if (!password.has_value())
      ASSERT_FALSE(data.has_password());
    else
      ASSERT_STREQ(*password, data.get_password().c_str());

    // TODO(rennox): The callers should be updated to indicate IF
    // transport type should actually be defined for each case
    if (data.has_transport_type()) {
      ASSERT_EQ(target_type, data.get_transport_type());

      switch (data.get_transport_type()) {
        case Transport_type::Tcp:
          if (!host.has_value())
            ASSERT_FALSE(data.has_password());
          else
            ASSERT_STREQ(*host, data.get_host().c_str());

          if (!port.has_value())
            ASSERT_FALSE(data.has_port());
          else
            ASSERT_EQ(*port, data.get_port());
          break;
        case Transport_type::Socket:
          if (!sock.has_value())
            ASSERT_FALSE(data.has_socket());
          else
            ASSERT_STREQ(*sock, data.get_socket().c_str());
          break;
        case Transport_type::Pipe:
          if (!sock.has_value())
            ASSERT_FALSE(data.has_socket());
          else
            ASSERT_STREQ(*sock, data.get_pipe().c_str());
          break;
      }
    }

    if (!db.has_value())
      ASSERT_FALSE(data.has_schema());
    else
      ASSERT_STREQ(*db, data.get_schema().c_str());

    if (attributes) {
      for (auto att : *attributes) {
        SCOPED_TRACE(att.first);

        std::string atts;
        if (att.second.size() == 1)
          atts = att.second[0];
        else
          atts = "[" + shcore::str_join(att.second, ",") + "]";
        ASSERT_STREQ(atts.c_str(), data.get(att.first).c_str());
      }
    }
  } catch (const std::invalid_argument &err) {
    std::string found_error(err.what());

    SCOPED_TRACE("UNEXPECTED ERROR: " + found_error);
    FAIL();
  }
}

void validate_ssh_uri(const std::string &connstring,
                      const std::optional<const char *> &scheme,
                      const std::optional<const char *> &user,
                      const std::optional<const char *> &password,
                      const std::optional<const char *> &host,
                      const std::optional<int> &port,
                      bool reverse_check = true) {
  SCOPED_TRACE(connstring);

  Uri_parser parser(Type::Ssh);

  try {
    mysqlshdk::ssh::Ssh_connection_options data;
    parser.parse(connstring, &data);

    if (reverse_check) {
      ASSERT_EQ(connstring,
                data.as_uri(mysqlshdk::db::uri::Tokens_mask::all()));
    }

    if (!scheme.has_value())
      ASSERT_FALSE(data.has_scheme());
    else
      ASSERT_STREQ(*scheme, data.get_scheme().c_str());

    if (!user.has_value())
      ASSERT_FALSE(data.has_user());
    else
      ASSERT_STREQ(*user, data.get_user().c_str());

    if (!password.has_value())
      ASSERT_FALSE(data.has_password());
    else
      ASSERT_STREQ(*password, data.get_password().c_str());
    if (!host.has_value())
      ASSERT_FALSE(data.has_host());
    else
      ASSERT_STREQ(*host, data.get_host().c_str());

    if (!port.has_value())
      ASSERT_FALSE(data.has_port());
    else
      ASSERT_EQ(*port, data.get_port());
  } catch (const std::invalid_argument &err) {
    std::string found_error(err.what());

    SCOPED_TRACE("UNEXPECTED ERROR: " + found_error);
    FAIL();
  }
}

void validate_file_uri(const std::string &connstring,
                       const std::optional<const char *> &scheme,
                       const std::optional<const char *> &host,
                       const std::optional<const char *> &path,
                       bool reverse_check = true, bool generic_check = true) {
  SCOPED_TRACE(connstring);

  try {
    mysqlshdk::db::uri::File_uri data;
    mysqlshdk::db::uri::Uri_parser parser(Type::File);
    parser.parse(connstring, &data);

    // TEST to ensure the URI is re-generated identical after parsed
    if (reverse_check) {
      SCOPED_TRACE("Testing decode File URI");
      ASSERT_EQ(connstring,
                data.as_uri(mysqlshdk::db::uri::Tokens_mask::all()));

      // TEST to ensure the URI is re-generated identical after parsed using the
      // Generic parser
      if (generic_check) {
        SCOPED_TRACE("Testing decode Generic URI");
        mysqlshdk::db::uri::Generic_uri uri(connstring);
        ASSERT_EQ(connstring,
                  uri.as_uri(mysqlshdk::db::uri::Tokens_mask::all()));
      }
    }

    if (!scheme.has_value())
      ASSERT_FALSE(data.has_value(mysqlshdk::db::kScheme));
    else
      ASSERT_STREQ(*scheme, data.get(mysqlshdk::db::kScheme).c_str());

    if (!host.has_value())
      ASSERT_FALSE(data.has_value(mysqlshdk::db::kHost));
    else
      ASSERT_STREQ(*host, data.get(mysqlshdk::db::kHost).c_str());

    if (!path.has_value())
      ASSERT_FALSE(data.has_value(mysqlshdk::db::kPath));
    else
      ASSERT_STREQ(*path, data.get(mysqlshdk::db::kPath).c_str());

  } catch (const std::invalid_argument &err) {
    std::string found_error(err.what());

    SCOPED_TRACE("UNEXPECTED ERROR: " + found_error);
    FAIL();
  }
}

void validate_ipv6(const std::string &address,
                   const std::string &expected = "") {
  std::string result =
      expected.empty() ? address.substr(1, address.length() - 2) : expected;
  validate_uri("mysqlx://user:sample@" + address + ":2540/testing", "mysqlx",
               "user", "sample", result.c_str(), 2540, NO_SOCK, "testing",
               HAS_PASSWORD, HAS_PORT, Transport_type::Tcp, expected.empty());
}

TEST(Uri_parser, parse_scheme) {
  validate_uri("mysqlx://mysql.com", "mysqlx", NO_USER, NO_PASSWORD,
               "mysql.com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD,
               HAS_NO_PORT, Transport_type::Tcp, true, false);
  validate_uri("mysql://mysql.com", "mysql", NO_USER, NO_PASSWORD, "mysql.com",
               NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT,
               Transport_type::Tcp);
  validate_uri("mysql.com", NO_SCHEMA, NO_USER, NO_PASSWORD, "mysql.com",
               NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT,
               Transport_type::Tcp);

  validate_ssh_uri("ssh://mysql.com", "ssh", NO_USER, NO_PASSWORD, "mysql.com",
                   NO_PORT);

  validate_ssh_uri("mysql.com", NO_SCHEME, NO_USER, NO_PASSWORD, "mysql.com",
                   NO_PORT);

  //                0    0    1    1    2    2    3    3    4    4    5    5
  //                0    5    0    5    0    5    0    5    0    5    0    5
  validate_bad_uri("://mysql.com", "Scheme is missing");
  validate_bad_uri("other://mysql.com",
                   "Invalid scheme [other], supported schemes include: mysql, "
                   "mysqlx");
  validate_bad_uri("mysqlx ://mysql.com", "Illegal space found at position 6");
  validate_bad_uri("mysq=lx://mysql.com",
                   "Illegal character [=] found at position 4");
  validate_bad_uri("mysqlx+ssh+other://mysql.com",
                   "Invalid scheme format [mysqlx+ssh+other], only one "
                   "extension is supported");
  validate_bad_uri("mysqlx+ssh://mysql.com",
                   "Scheme extension [ssh] is not supported");

  validate_bad_uri("://mysql.com", "Scheme is missing", Type::Ssh);
  validate_bad_uri("other://mysql.com",
                   "Invalid scheme [other], supported schemes include: ssh",
                   Type::Ssh);
  validate_bad_uri("ssh ://mysql.com", "Illegal space found at position 3",
                   Type::Ssh);
  validate_bad_uri("ss=h://mysql.com",
                   "Illegal character [=] found at position 2", Type::Ssh);
  validate_bad_uri("mysqlx+ssh+other://mysql.com",
                   "Invalid scheme format [mysqlx+ssh+other], only one "
                   "extension is supported",
                   Type::Ssh);
  validate_bad_uri("mysqlx+ssh://mysql.com",
                   "Scheme extension [ssh] is not supported", Type::Ssh);
}

TEST(Uri_parser, parse_user_info) {
  validate_uri("mysqlx://user@mysql.com", "mysqlx", "user", NO_PASSWORD,
               "mysql.com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD,
               HAS_NO_PORT, Transport_type::Tcp);
  validate_uri("mysqlx://user.name@mysql.com", "mysqlx", "user.name",
               NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB,
               HAS_NO_PASSWORD, HAS_NO_PORT, Transport_type::Tcp);
  validate_uri("mysqlx://user-name@mysql.com", "mysqlx", "user-name",
               NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB,
               HAS_NO_PASSWORD, HAS_NO_PORT, Transport_type::Tcp);
  validate_uri("mysqlx://user~name@mysql.com", "mysqlx", "user~name",
               NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB,
               HAS_NO_PASSWORD, HAS_NO_PORT, Transport_type::Tcp);
  validate_uri("mysqlx://user!name@mysql.com", "mysqlx", "user!name",
               NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB,
               HAS_NO_PASSWORD, HAS_NO_PORT, Transport_type::Tcp);
  validate_uri("mysqlx://user$name@mysql.com", "mysqlx", "user$name",
               NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB,
               HAS_NO_PASSWORD, HAS_NO_PORT, Transport_type::Tcp);
  validate_uri("mysqlx://user&name@mysql.com", "mysqlx", "user&name",
               NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB,
               HAS_NO_PASSWORD, HAS_NO_PORT, Transport_type::Tcp);
  validate_uri("mysqlx://user'name@mysql.com", "mysqlx", "user'name",
               NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB,
               HAS_NO_PASSWORD, HAS_NO_PORT, Transport_type::Tcp);
  validate_uri("mysqlx://user(name)@mysql.com", "mysqlx", "user(name)",
               NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB,
               HAS_NO_PASSWORD, HAS_NO_PORT, Transport_type::Tcp);
  validate_uri("mysqlx://user*name@mysql.com", "mysqlx", "user*name",
               NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB,
               HAS_NO_PASSWORD, HAS_NO_PORT, Transport_type::Tcp);
  validate_uri("mysqlx://user+name@mysql.com", "mysqlx", "user+name",
               NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB,
               HAS_NO_PASSWORD, HAS_NO_PORT, Transport_type::Tcp);
  validate_uri("mysqlx://user,name@mysql.com", "mysqlx", "user,name",
               NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB,
               HAS_NO_PASSWORD, HAS_NO_PORT, Transport_type::Tcp);
  validate_uri("mysqlx://user;name@mysql.com", "mysqlx", "user;name",
               NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB,
               HAS_NO_PASSWORD, HAS_NO_PORT, Transport_type::Tcp);
  validate_uri("mysqlx://user=name@mysql.com", "mysqlx", "user=name",
               NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB,
               HAS_NO_PASSWORD, HAS_NO_PORT, Transport_type::Tcp);
  validate_uri("mysqlx://user%2Aname@mysql.com", "mysqlx", "user*name",
               NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, NO_DB,
               HAS_NO_PASSWORD, HAS_NO_PORT, Transport_type::Tcp, false);
  validate_uri("mysqlx://user%2Aname:password@mysql.com", "mysqlx", "user*name",
               "password", "mysql.com", NO_PORT, NO_SOCK, NO_DB, HAS_PASSWORD,
               HAS_NO_PORT, Transport_type::Tcp, false);

  validate_ssh_uri("ssh://user@mysql.com", "ssh", "user", NO_PASSWORD,
                   "mysql.com", NO_PORT);

  validate_ssh_uri("ssh://user.name@mysql.com", "ssh", "user.name", NO_PASSWORD,
                   "mysql.com", NO_PORT);

  validate_ssh_uri("ssh://user-name@mysql.com", "ssh", "user-name", NO_PASSWORD,
                   "mysql.com", NO_PORT);

  validate_ssh_uri("ssh://user~name@mysql.com", "ssh", "user~name", NO_PASSWORD,
                   "mysql.com", NO_PORT);

  validate_ssh_uri("ssh://user!name@mysql.com", "ssh", "user!name", NO_PASSWORD,
                   "mysql.com", NO_PORT);

  validate_ssh_uri("ssh://user$name@mysql.com", "ssh", "user$name", NO_PASSWORD,
                   "mysql.com", NO_PORT);
  validate_ssh_uri("ssh://user&name@mysql.com", "ssh", "user&name", NO_PASSWORD,
                   "mysql.com", NO_PORT);
  validate_ssh_uri("ssh://user'name@mysql.com", "ssh", "user'name", NO_PASSWORD,
                   "mysql.com", NO_PORT);
  validate_ssh_uri("ssh://user(name)@mysql.com", "ssh", "user(name)",
                   NO_PASSWORD, "mysql.com", NO_PORT);
  validate_ssh_uri("ssh://user*name@mysql.com", "ssh", "user*name", NO_PASSWORD,
                   "mysql.com", NO_PORT);
  validate_ssh_uri("ssh://user+name@mysql.com", "ssh", "user+name", NO_PASSWORD,
                   "mysql.com", NO_PORT);
  validate_ssh_uri("ssh://user,name@mysql.com", "ssh", "user,name", NO_PASSWORD,
                   "mysql.com", NO_PORT);
  validate_ssh_uri("ssh://user;name@mysql.com", "ssh", "user;name", NO_PASSWORD,
                   "mysql.com", NO_PORT);
  validate_ssh_uri("ssh://user=name@mysql.com", "ssh", "user=name", NO_PASSWORD,
                   "mysql.com", NO_PORT);
  validate_ssh_uri("ssh://user%2Aname@mysql.com", "ssh", "user*name",
                   NO_PASSWORD, "mysql.com", NO_PORT, false);
  validate_ssh_uri("ssh://user%2Aname:password@mysql.com", "ssh", "user*name",
                   "password", "mysql.com", NO_PORT, false);

  //                0    0    1    1    2    2    3    3    4    4    5    5
  //                0    5    0    5    0    5    0    5    0    5    0    5
  validate_bad_uri("mysqlx://@mysql.com", "Missing user information");
  validate_bad_uri("mysqlx ://user_name@mysql.com",
                   "Illegal space found at position 6");
  validate_bad_uri("mysqlx://user%name@mysql.com",
                   "Illegal character [%] found at position 13");
  validate_bad_uri("mysqlx://user%1name@mysql.com",
                   "Illegal character [%] found at position 13");
  validate_bad_uri("mysqlx://user%2Aname:p@ssword@mysql.com",
                   "Illegal character [@] found at position 29");
  validate_bad_uri("mysqlx://:password@mysql.com", "Missing user name");

  validate_bad_uri("ssh://@mysql.com", "Missing user information", Type::Ssh);
  validate_bad_uri("ssh ://user_name@mysql.com",
                   "Illegal space found at position 3", Type::Ssh);
  validate_bad_uri("ssh://user%name@mysql.com",
                   "Illegal character [%] found at position 10", Type::Ssh);
  validate_bad_uri("ssh://user%1name@mysql.com",
                   "Illegal character [%] found at position 10", Type::Ssh);
  validate_bad_uri("ssh://user%2Aname:p@ssword@mysql.com",
                   "Illegal character [@] found at position 26", Type::Ssh);
  validate_bad_uri("ssh://:password@mysql.com", "Missing user name", Type::Ssh);

  // BUG#36105235 - an unescaped '@' character in the target path
  validate_uri("mysqlx://mysql.com/@.manifest.json", "mysqlx", NO_USER,
               NO_PASSWORD, "mysql.com", NO_PORT, NO_SOCK, "@.manifest.json",
               HAS_NO_PASSWORD, HAS_NO_PORT, Transport_type::Tcp, false);
}

TEST(Uri_parser, parse_host_name) {
#ifdef WIN32
  validate_uri("mysqlx://user@localhost", "mysqlx", "user", NO_PASSWORD,
               "localhost", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD,
               HAS_NO_PORT, Transport_type::Tcp);
#else
  validate_uri("mysqlx://user@localhost", "mysqlx", "user", NO_PASSWORD,
               "localhost", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD,
               HAS_NO_PORT, Transport_type::Pipe);
#endif
  validate_uri("mysqlx://user@localhost:3306", "mysqlx", "user", NO_PASSWORD,
               "localhost", 3306, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_PORT,
               Transport_type::Tcp);
  validate_uri("mysqlx://user@mysql.com", "mysqlx", "user", NO_PASSWORD,
               "mysql.com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD,
               HAS_NO_PORT, Transport_type::Tcp);
  validate_uri("mysqlx://user@mysql1.com", "mysqlx", "user", NO_PASSWORD,
               "mysql1.com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD,
               HAS_NO_PORT, Transport_type::Tcp);
  validate_uri("mysqlx://user@mysql1-com", "mysqlx", "user", NO_PASSWORD,
               "mysql1-com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD,
               HAS_NO_PORT, Transport_type::Tcp);
  validate_uri("mysqlx://user@mysql1~com", "mysqlx", "user", NO_PASSWORD,
               "mysql1~com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD,
               HAS_NO_PORT, Transport_type::Tcp);
  validate_uri("mysqlx://user@mysql1_com", "mysqlx", "user", NO_PASSWORD,
               "mysql1_com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD,
               HAS_NO_PORT, Transport_type::Tcp);
  validate_uri("mysqlx://user@mysql1!com", "mysqlx", "user", NO_PASSWORD,
               "mysql1!com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD,
               HAS_NO_PORT, Transport_type::Tcp);
  validate_uri("mysqlx://user@mysql1$com", "mysqlx", "user", NO_PASSWORD,
               "mysql1$com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD,
               HAS_NO_PORT, Transport_type::Tcp);
  validate_uri("mysqlx://user@mysql1&com", "mysqlx", "user", NO_PASSWORD,
               "mysql1&com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD,
               HAS_NO_PORT, Transport_type::Tcp);
  validate_uri("mysqlx://user@mysql1'com", "mysqlx", "user", NO_PASSWORD,
               "mysql1'com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD,
               HAS_NO_PORT, Transport_type::Tcp);
  validate_uri("mysqlx://user@mysql1(com)", "mysqlx", "user", NO_PASSWORD,
               "mysql1(com)", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD,
               HAS_NO_PORT, Transport_type::Tcp);
  validate_uri("mysqlx://user@mysql1*com", "mysqlx", "user", NO_PASSWORD,
               "mysql1*com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD,
               HAS_NO_PORT, Transport_type::Tcp);
  validate_uri("mysqlx://user@mysql1+com", "mysqlx", "user", NO_PASSWORD,
               "mysql1+com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD,
               HAS_NO_PORT, Transport_type::Tcp);
  validate_uri("mysqlx://user@mysql1,com", "mysqlx", "user", NO_PASSWORD,
               "mysql1,com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD,
               HAS_NO_PORT, Transport_type::Tcp);
  validate_uri("mysqlx://user@mysql1;com", "mysqlx", "user", NO_PASSWORD,
               "mysql1;com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD,
               HAS_NO_PORT, Transport_type::Tcp);
  validate_uri("mysqlx://user@mysql1=com", "mysqlx", "user", NO_PASSWORD,
               "mysql1=com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD,
               HAS_NO_PORT, Transport_type::Tcp);
  validate_uri("mysqlx://user@mysql1%2ecom", "mysqlx", "user", NO_PASSWORD,
               "mysql1.com", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD,
               HAS_NO_PORT, Transport_type::Tcp, false);
  validate_uri("mysqlx://user@mysql1%2ecom:2845", "mysqlx", "user", NO_PASSWORD,
               "mysql1.com", 2845, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_PORT,
               Transport_type::Tcp, false);

  validate_ssh_uri("ssh://user@localhost:3306", "ssh", "user", NO_PASSWORD,
                   "localhost", 3306);
  validate_ssh_uri("ssh://user@mysql.com", "ssh", "user", NO_PASSWORD,
                   "mysql.com", NO_PORT);
  validate_ssh_uri("ssh://user@mysql1.com", "ssh", "user", NO_PASSWORD,
                   "mysql1.com", NO_PORT);
  validate_ssh_uri("ssh://user@mysql1-com", "ssh", "user", NO_PASSWORD,
                   "mysql1-com", NO_PORT);
  validate_ssh_uri("ssh://user@mysql1~com", "ssh", "user", NO_PASSWORD,
                   "mysql1~com", NO_PORT);
  validate_ssh_uri("ssh://user@mysql1_com", "ssh", "user", NO_PASSWORD,
                   "mysql1_com", NO_PORT);
  validate_ssh_uri("ssh://user@mysql1!com", "ssh", "user", NO_PASSWORD,
                   "mysql1!com", NO_PORT);
  validate_ssh_uri("ssh://user@mysql1$com", "ssh", "user", NO_PASSWORD,
                   "mysql1$com", NO_PORT);
  validate_ssh_uri("ssh://user@mysql1&com", "ssh", "user", NO_PASSWORD,
                   "mysql1&com", NO_PORT);
  validate_ssh_uri("ssh://user@mysql1'com", "ssh", "user", NO_PASSWORD,
                   "mysql1'com", NO_PORT);
  validate_ssh_uri("ssh://user@mysql1(com)", "ssh", "user", NO_PASSWORD,
                   "mysql1(com)", NO_PORT);
  validate_ssh_uri("ssh://user@mysql1*com", "ssh", "user", NO_PASSWORD,
                   "mysql1*com", NO_PORT);
  validate_ssh_uri("ssh://user@mysql1+com", "ssh", "user", NO_PASSWORD,
                   "mysql1+com", NO_PORT);
  validate_ssh_uri("ssh://user@mysql1,com", "ssh", "user", NO_PASSWORD,
                   "mysql1,com", NO_PORT);
  validate_ssh_uri("ssh://user@mysql1;com", "ssh", "user", NO_PASSWORD,
                   "mysql1;com", NO_PORT);
  validate_ssh_uri("ssh://user@mysql1=com", "ssh", "user", NO_PASSWORD,
                   "mysql1=com", NO_PORT);
  validate_ssh_uri("ssh://user@mysql1%2ecom", "ssh", "user", NO_PASSWORD,
                   "mysql1.com", NO_PORT, false);
  validate_ssh_uri("ssh://user@mysql1%2ecom:2845", "ssh", "user", NO_PASSWORD,
                   "mysql1.com", 2845, false);

  //                0    0    1    1    2    2    3    3    4    4    5    5
  //                0    5    0    5    0    5    0    5    0    5    0    5
  validate_bad_uri("mysqlx://user@mysql1%com",
                   "Illegal character [%] found at position 20");
  validate_bad_uri("mysqlx://user@mysql1%cgcom",
                   "Illegal character [%] found at position 20");
  validate_bad_uri("mysqlx://user@mysql1%2ecom:65538",
                   "Port is out of the valid range: 0 - 65535");
  validate_bad_uri("mysqlx://user@mysql1%2ecom:", "Missing port number");
  validate_bad_uri("mysqlx://user@mysql1%2ecom:2845f",
                   "Illegal character [f] found at position 31");
  validate_bad_uri("mysqlx://user@mysql1%2ecom:invalid",
                   "Illegal character [i] found at position 27");

  // ssh part
  validate_bad_uri("ssh://user@mysql1%com",
                   "Illegal character [%] found at position 17", Type::Ssh);
  validate_bad_uri("ssh://user@mysql1%cgcom",
                   "Illegal character [%] found at position 17", Type::Ssh);
  validate_bad_uri("ssh://user@mysql1%2ecom:65538",
                   "Port is out of the valid range: 0 - 65535", Type::Ssh);
  validate_bad_uri("ssh://user@mysql1%2ecom:", "Missing port number",
                   Type::Ssh);
  validate_bad_uri("ssh://user@mysql1%2ecom:2845f",
                   "Illegal character [f] found at position 28", Type::Ssh);
  validate_bad_uri("ssh://user@mysql1%2ecom:invalid",
                   "Illegal character [i] found at position 24", Type::Ssh);
}

TEST(Uri_parser, parse_host_ipv4) {
  validate_uri("mysqlx://user@10.150.123.45", "mysqlx", "user", NO_PASSWORD,
               "10.150.123.45", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD,
               HAS_NO_PORT, Transport_type::Tcp);
  validate_uri("mysqlx://user@10.150.123", "mysqlx", "user", NO_PASSWORD,
               "10.150.123", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD,
               HAS_NO_PORT, Transport_type::Tcp);
  validate_uri("mysqlx://user@10.150.123.whatever", "mysqlx", "user",
               NO_PASSWORD, "10.150.123.whatever", NO_PORT, NO_SOCK, NO_DB,
               HAS_NO_PASSWORD, HAS_NO_PORT, Transport_type::Tcp);
  validate_uri("mysqlx://user@0.0.0.0", "mysqlx", "user", NO_PASSWORD,
               "0.0.0.0", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_NO_PORT,
               Transport_type::Tcp);
  validate_uri("mysqlx://user@255.255.255.255", "mysqlx", "user", NO_PASSWORD,
               "255.255.255.255", NO_PORT, NO_SOCK, NO_DB, HAS_NO_PASSWORD,
               HAS_NO_PORT, Transport_type::Tcp);
  validate_uri("mysqlx://user@10.150.123.45:2845", "mysqlx", "user",
               NO_PASSWORD, "10.150.123.45", 2845, NO_SOCK, NO_DB,
               HAS_NO_PASSWORD, HAS_PORT, Transport_type::Tcp);
  validate_uri("mysqlx://user@10.150.123.45:0", "mysqlx", "user", NO_PASSWORD,
               "10.150.123.45", 0, NO_SOCK, NO_DB, HAS_NO_PASSWORD, HAS_PORT,
               Transport_type::Tcp);

  validate_ssh_uri("ssh://user@10.150.123.45", "ssh", "user", NO_PASSWORD,
                   "10.150.123.45", NO_PORT);
  validate_ssh_uri("ssh://user@10.150.123", "ssh", "user", NO_PASSWORD,
                   "10.150.123", NO_PORT);
  validate_ssh_uri("ssh://user@10.150.123.whatever", "ssh", "user", NO_PASSWORD,
                   "10.150.123.whatever", NO_PORT);
  validate_ssh_uri("ssh://user@0.0.0.0", "ssh", "user", NO_PASSWORD, "0.0.0.0",
                   NO_PORT);
  validate_ssh_uri("ssh://user@255.255.255.255", "ssh", "user", NO_PASSWORD,
                   "255.255.255.255", NO_PORT);
  validate_ssh_uri("ssh://user@10.150.123.45:2845", "ssh", "user", NO_PASSWORD,
                   "10.150.123.45", 2845);
  validate_ssh_uri("ssh://user@10.150.123.45:0", "ssh", "user", NO_PASSWORD,
                   "10.150.123.45", 0);

  //                0    0    1    1    2    2    3    3    4    4    5    5
  //                0    5    0    5    0    5    0    5    0    5    0    5
  validate_bad_uri("mysqlx://user@256.255.255.255",
                   "Octet value out of bounds [256], valid range for IPv4 is "
                   "0 to 255 at position 14");
  validate_bad_uri("mysqlx://user@255.256.255.255",
                   "Octet value out of bounds [256], valid range for IPv4 is "
                   "0 to 255 at position 18");
  validate_bad_uri("mysqlx://user@255.255.256.255",
                   "Octet value out of bounds [256], valid range for IPv4 is "
                   "0 to 255 at position 22");
  validate_bad_uri("mysqlx://user@255.255.255.256",
                   "Octet value out of bounds [256], valid range for IPv4 is "
                   "0 to 255 at position 26");
  validate_bad_uri("mysqlx://user@10.150.123.45:68000",
                   "Port is out of the valid range: 0 - 65535");
  validate_bad_uri("mysqlx://user@10.150.123.45:", "Missing port number");
  validate_bad_uri("mysqlx://user@10.150.123.45:2845f",
                   "Illegal character [f] found at position 32");
  validate_bad_uri("mysqlx://user@10.150.123.45:2147483647",
                   "Port is out of the valid range: 0 - 65535");
  validate_bad_uri("mysqlx://user@10.150.123.45:2147483648",
                   "Port is out of the valid range: 0 - 65535");
  validate_bad_uri("mysqlx://user@10.150.123.45:4294967295",
                   "Port is out of the valid range: 0 - 65535");
  validate_bad_uri("mysqlx://user@10.150.123.45:4294967296",
                   "Port is out of the valid range: 0 - 65535");
  validate_bad_uri("mysqlx://user@10.150.123.45:928482174821947214627846278",
                   "Port is out of the valid range: 0 - 65535");
  validate_bad_uri("mysqlx://user@10.150.123.45:-4294967296",
                   "Illegal character [-] found at position 28");
  validate_bad_uri("mysqlx://user@10.150.123.45:-1",
                   "Illegal character [-] found at position 28");
  validate_bad_uri("mysqlx://user@10.150.123.45:-0",
                   "Illegal character [-] found at position 28");
  validate_bad_uri("mysqlx://user@10.150.123.45:invalid",
                   "Illegal character [i] found at position 28");

  // ssh
  validate_bad_uri("ssh://user@256.255.255.255",
                   "Octet value out of bounds [256], valid range for IPv4 is "
                   "0 to 255 at position 11",
                   Type::Ssh);
  validate_bad_uri("ssh://user@255.256.255.255",
                   "Octet value out of bounds [256], valid range for IPv4 is "
                   "0 to 255 at position 15",
                   Type::Ssh);
  validate_bad_uri("ssh://user@255.255.256.255",
                   "Octet value out of bounds [256], valid range for IPv4 is "
                   "0 to 255 at position 19",
                   Type::Ssh);
  validate_bad_uri("ssh://user@255.255.255.256",
                   "Octet value out of bounds [256], valid range for IPv4 is "
                   "0 to 255 at position 23",
                   Type::Ssh);
  validate_bad_uri("ssh://user@10.150.123.45:68000",
                   "Port is out of the valid range: 0 - 65535", Type::Ssh);
  validate_bad_uri("ssh://user@10.150.123.45:", "Missing port number",
                   Type::Ssh);
  validate_bad_uri("ssh://user@10.150.123.45:2845f",
                   "Illegal character [f] found at position 29", Type::Ssh);
  validate_bad_uri("ssh://user@10.150.123.45:2147483647",
                   "Port is out of the valid range: 0 - 65535", Type::Ssh);
  validate_bad_uri("ssh://user@10.150.123.45:2147483648",
                   "Port is out of the valid range: 0 - 65535", Type::Ssh);
  validate_bad_uri("ssh://user@10.150.123.45:4294967295",
                   "Port is out of the valid range: 0 - 65535", Type::Ssh);
  validate_bad_uri("ssh://user@10.150.123.45:4294967296",
                   "Port is out of the valid range: 0 - 65535", Type::Ssh);
  validate_bad_uri("ssh://user@10.150.123.45:928482174821947214627846278",
                   "Port is out of the valid range: 0 - 65535", Type::Ssh);
  validate_bad_uri("ssh://user@10.150.123.45:-4294967296",
                   "Illegal character [-] found at position 25", Type::Ssh);
  validate_bad_uri("ssh://user@10.150.123.45:-1",
                   "Illegal character [-] found at position 25", Type::Ssh);
  validate_bad_uri("ssh://user@10.150.123.45:-0",
                   "Illegal character [-] found at position 25", Type::Ssh);
  validate_bad_uri("ssh://user@10.150.123.45:invalid",
                   "Illegal character [i] found at position 25", Type::Ssh);
}

TEST(Uri_parser, parse_host_ipv6) {
  // Complete
  validate_ipv6("[1:2:3:4:5:6:7:8]");

  // Hiding 1
  validate_ipv6("[::1:2:3:4:5:6:7]");
  validate_ipv6("[1::2:3:4:5:6:7]");
  validate_ipv6("[1:2::3:4:5:6:7]");
  validate_ipv6("[1:2:3::4:5:6:7]");
  validate_ipv6("[1:2:3:4::5:6:7]");
  validate_ipv6("[1:2:3:4:5::6:7]");
  validate_ipv6("[1:2:3:4:5:6::7]");
  validate_ipv6("[1:2:3:4:5:6:7::]");

  // Hiding 2
  validate_ipv6("[::1:2:3:4:5:6]");
  validate_ipv6("[1::2:3:4:5:6]");
  validate_ipv6("[1:2::3:4:5:6]");
  validate_ipv6("[1:2:3::4:5:6]");
  validate_ipv6("[1:2:3:4::5:6]");
  validate_ipv6("[1:2:3:4:5::6]");
  validate_ipv6("[1:2:3:4:5:6::]");

  // Hiding 3
  validate_ipv6("[::1:2:3:4:5]");
  validate_ipv6("[1::2:3:4:5]");
  validate_ipv6("[1:2::3:4:5]");
  validate_ipv6("[1:2:3::4:5]");
  validate_ipv6("[1:2:3:4::5]");
  validate_ipv6("[1:2:3:4:5::]");

  // Hiding 4
  validate_ipv6("[::1:2:3:4]");
  validate_ipv6("[1::2:3:4]");
  validate_ipv6("[1:2::3:4]");
  validate_ipv6("[1:2:3::4]");
  validate_ipv6("[1:2:3:4::]");

  // Hiding 5
  validate_ipv6("[::1:2:3]");
  validate_ipv6("[1::2:3]");
  validate_ipv6("[1:2::3]");
  validate_ipv6("[1:2:3::]");

  // Hiding 6
  validate_ipv6("[::1:2]");
  validate_ipv6("[1::2]");
  validate_ipv6("[1:2::]");

  // Hiding 7
  validate_ipv6("[::1]");
  validate_ipv6("[1::]");

  // Hiding 8
  validate_ipv6("[::]");

  // IPv4 Embedded
  // Complete
  validate_ipv6("[1:2:3:4:5:6:1.2.3.4]");

  // Hiding 1
  validate_ipv6("[::1:2:3:4:5:1.2.3.4]");
  validate_ipv6("[1::2:3:4:5:1.2.3.4]");
  validate_ipv6("[1:2::3:4:5:1.2.3.4]");
  validate_ipv6("[1:2:3::4:5:1.2.3.4]");
  validate_ipv6("[1:2:3:4::5:1.2.3.4]");
  validate_ipv6("[1:2:3:4:5::1.2.3.4]");

  // Hiding 2
  validate_ipv6("[::1:2:3:4:1.2.3.4]");
  validate_ipv6("[1::2:3:4:1.2.3.4]");
  validate_ipv6("[1:2::3:4:1.2.3.4]");
  validate_ipv6("[1:2:3::4:1.2.3.4]");
  validate_ipv6("[1:2:3:4::1.2.3.4]");

  // Hiding 3
  validate_ipv6("[::1:2:3:1.2.3.4]");
  validate_ipv6("[1::2:3:1.2.3.4]");
  validate_ipv6("[1:2::3:1.2.3.4]");
  validate_ipv6("[1:2:3::1.2.3.4]");

  // Hiding 4
  validate_ipv6("[::1:2:1.2.3.4]");
  validate_ipv6("[1::2:1.2.3.4]");
  validate_ipv6("[1:2::1.2.3.4]");

  // Hiding 5
  validate_ipv6("[::1:1.2.3.4]");
  validate_ipv6("[1::1.2.3.4]");

  // Hiding 6
  validate_ipv6("[::1.2.3.4]");

  // Artistic Cases
  // Hexadecimals are OK
  validate_ipv6("[A:B:C:D:E:F:7:8]");
  //                0    0    1    1    2    2    3    3    4    4    5    5
  //                0    5    0    5    0    5    0    5    0    5    0    5
  validate_bad_uri("mysqlx://user:sample@[A:B:C:D:E:G:7:8]:2540/testing",
                   "Unexpected data [G] found at position 32");
  validate_bad_uri(
      "mysqlx://user:sample@[A:B:C:D:E:A1B23:7:8]:2540/testing",
      "Invalid IPv6 value [A1B23], maximum 4 hexadecimal digits accepted");
  validate_bad_uri(
      "mysqlx://user:sample@[A:B:C:D:E:F:7:8:9]:2540/testing",
      "Invalid IPv6: the number of segments does not match the specification");
  validate_bad_uri(
      "mysqlx://user:sample@[::1:2:3:4:5:6:7:8]:2540/testing",
      "Invalid IPv6: the number of segments does not match the specification");
  validate_bad_uri("mysqlx://user:sample@[1::3:4::6:7:8]:2540/testing",
                   "Unexpected data [:] found at position 29");
  validate_bad_uri("mysqlx://user:sample@[1:2:3:4:5:6:7]:8]:2540/testing",
                   "Invalid IPv6: the number of segments does not match the "
                   "specification");
  validate_bad_uri("mysqlx://user:sample@[[1:2:3:4:5:6:7:8]]:2540/testing",
                   "Unexpected data [[] found at position 22");
  validate_bad_uri("mysqlx://user:sample@[.1:2:3:4:5:6:7:8]:2540/testing",
                   "Unexpected data [.] found at position 22");
  validate_bad_uri("mysqlx://user:sample@[1[:2:3:4:5:6:7:8]:2540/testing",
                   "Expected token type : at position 23 but found type [ ([)");
  validate_bad_uri("mysqlx://user:sample@[1.:2:3:4:5:6:7:8]:2540/testing",
                   "Expected token type : at position 23 but found type . (.)");
  validate_bad_uri("mysqlx://user:sample@[1:[2:3:4:5:6:7:8]:2540/testing",
                   "Unexpected data [[] found at position 24");
  validate_bad_uri("mysqlx://user:sample@[1:.2:3:4:5:6:7:8]:2540/testing",
                   "Unexpected data [.] found at position 24");
  validate_bad_uri("mysqlx://user:sample@[1:2[2:3:4:5:6:7:8]:2540/testing",
                   "Expected token type : at position 25 but found type [ ([)");
  validate_bad_uri("mysqlx://user:sample@[1:2.2:3:4:5:6:7:8]:2540/testing",
                   "Expected token type : at position 25 but found type . (.)");
  validate_bad_uri("mysqlx://user:sample@[1:2[:3:4:5:6:7:8]:2540/testing",
                   "Expected token type : at position 25 but found type [ ([)");
  validate_bad_uri("mysqlx://user:sample@[1:2.:3:4:5:6:7:8]:2540/testing",
                   "Expected token type : at position 25 but found type . (.)");

  // ssh
  validate_bad_uri("ssh://user:sample@[A:B:C:D:E:G:7:8]:2540/testing",
                   "Unexpected data [G] found at position 29", Type::Ssh);
  validate_bad_uri(
      "ssh://user:sample@[A:B:C:D:E:A1B23:7:8]:2540/testing",
      "Invalid IPv6 value [A1B23], maximum 4 hexadecimal digits accepted",
      Type::Ssh);
  validate_bad_uri(
      "ssh://user:sample@[A:B:C:D:E:F:7:8:9]:2540/testing",
      "Invalid IPv6: the number of segments does not match the specification",
      Type::Ssh);
  validate_bad_uri(
      "ssh://user:sample@[::1:2:3:4:5:6:7:8]:2540/testing",
      "Invalid IPv6: the number of segments does not match the specification",
      Type::Ssh);
  validate_bad_uri("ssh://user:sample@[1::3:4::6:7:8]:2540/testing",
                   "Unexpected data [:] found at position 26", Type::Ssh);
  validate_bad_uri("ssh://user:sample@[1:2:3:4:5:6:7]:8]:2540/testing",
                   "Invalid IPv6: the number of segments does not match the "
                   "specification",
                   Type::Ssh);
  validate_bad_uri("ssh://user:sample@[[1:2:3:4:5:6:7:8]]:2540/testing",
                   "Unexpected data [[] found at position 19", Type::Ssh);
  validate_bad_uri("ssh://user:sample@[.1:2:3:4:5:6:7:8]:2540/testing",
                   "Unexpected data [.] found at position 19", Type::Ssh);
  validate_bad_uri("ssh://user:sample@[1[:2:3:4:5:6:7:8]:2540/testing",
                   "Expected token type : at position 20 but found type [ ([)",
                   Type::Ssh);
  validate_bad_uri("ssh://user:sample@[1.:2:3:4:5:6:7:8]:2540/testing",
                   "Expected token type : at position 20 but found type . (.)",
                   Type::Ssh);
  validate_bad_uri("ssh://user:sample@[1:[2:3:4:5:6:7:8]:2540/testing",
                   "Unexpected data [[] found at position 21", Type::Ssh);
  validate_bad_uri("ssh://user:sample@[1:.2:3:4:5:6:7:8]:2540/testing",
                   "Unexpected data [.] found at position 21", Type::Ssh);
  validate_bad_uri("ssh://user:sample@[1:2[2:3:4:5:6:7:8]:2540/testing",
                   "Expected token type : at position 22 but found type [ ([)",
                   Type::Ssh);
  validate_bad_uri("ssh://user:sample@[1:2.2:3:4:5:6:7:8]:2540/testing",
                   "Expected token type : at position 22 but found type . (.)",
                   Type::Ssh);
  validate_bad_uri("ssh://user:sample@[1:2[:3:4:5:6:7:8]:2540/testing",
                   "Expected token type : at position 22 but found type [ ([)",
                   Type::Ssh);
  validate_bad_uri("ssh://user:sample@[1:2.:3:4:5:6:7:8]:2540/testing",
                   "Expected token type : at position 22 but found type . (.)",
                   Type::Ssh);

  // zone ID
  validate_ipv6("[fe80::850a:5a7c:6ab7:aec4%25enp0s3]",
                "fe80::850a:5a7c:6ab7:aec4%enp0s3");
  validate_ipv6("[fe80::850a:5a7c:6ab7:aec4%25%65%6e%70%30%733]",
                "fe80::850a:5a7c:6ab7:aec4%enp0s3");
  validate_ipv6("[fe80::850a:5a7c:6ab7:aec4%25%65%6E%70%30%733]",
                "fe80::850a:5a7c:6ab7:aec4%enp0s3");
  validate_ipv6("[fe80::850a:5a7c:6ab7:aec4%25eth%25]",
                "fe80::850a:5a7c:6ab7:aec4%eth%");
  validate_ipv6("[fe80::850a:5a7c:6ab7:aec4%25eth%3F]",
                "fe80::850a:5a7c:6ab7:aec4%eth?");
  validate_ipv6("[fe80::850a:5a7c:6ab7:aec4%2513]",
                "fe80::850a:5a7c:6ab7:aec4%13");

  validate_bad_uri("mysqlx://user:sample@[1:2:3:4:5:6:7:8%25eth:]:2540/testing",
                   "Unexpected data [:] found at position 43");
  validate_bad_uri("mysqlx://user:sample@[1:2:3:4:5:6:7:8%25eth[]:2540/testing",
                   "Unexpected data [[] found at position 43");
  validate_bad_uri("mysqlx://user:sample@[1:2:3:4:5:6:7:8%25eth]]:2540/testing",
                   "Illegal character []] found at position 44");
  validate_bad_uri("mysqlx://user:sample@[1:2:3:4:5:6:7:8%25eth#]:2540/testing",
                   "Illegal character [#] found at position 43");
  validate_bad_uri("mysqlx://user:sample@[1:2:x:4:5:6:7:8%25eth0]:2540/testing",
                   "Unexpected data [x] found at position 26");
  validate_bad_uri(
      "mysqlx://user:sample@[1:2%3A3:4:5:6:7:8%25eth0]:2540/testing",
      "Unexpected data [%3A] found at position 25");
  validate_bad_uri(
      "mysqlx://user:sample@[1%252:3:4:5:6:7:8%25eth0]:2540/testing",
      "Unexpected data [:] found at position 27");
  validate_bad_uri("mysqlx://user:sample@[1:2:3:4:5:6:7:8%25]:2540/testing",
                   "Zone ID cannot be empty");

  // ssh
  validate_bad_uri("ssh://user:sample@[1:2:3:4:5:6:7:8%25eth:]:2540/testing",
                   "Unexpected data [:] found at position 40", Type::Ssh);
  validate_bad_uri("ssh://user:sample@[1:2:3:4:5:6:7:8%25eth[]:2540/testing",
                   "Unexpected data [[] found at position 40", Type::Ssh);
  validate_bad_uri("ssh://user:sample@[1:2:3:4:5:6:7:8%25eth]]:2540/testing",
                   "Illegal character []] found at position 41", Type::Ssh);
  validate_bad_uri("ssh://user:sample@[1:2:3:4:5:6:7:8%25eth#]:2540/testing",
                   "Illegal character [#] found at position 40", Type::Ssh);
  validate_bad_uri("ssh://user:sample@[1:2:x:4:5:6:7:8%25eth0]:2540/testing",
                   "Unexpected data [x] found at position 23", Type::Ssh);
  validate_bad_uri("ssh://user:sample@[1:2%3A3:4:5:6:7:8%25eth0]:2540/testing",
                   "Unexpected data [%3A] found at position 22", Type::Ssh);
  validate_bad_uri("ssh://user:sample@[1%252:3:4:5:6:7:8%25eth0]:2540/testing",
                   "Unexpected data [:] found at position 24", Type::Ssh);
  validate_bad_uri("ssh://user:sample@[1:2:3:4:5:6:7:8%25]:2540/testing",
                   "Zone ID cannot be empty", Type::Ssh);

  // IPv6 + zone ID
  // Complete
  validate_ipv6("[1:2:3:4:5:6:7:8%25eth0]", "1:2:3:4:5:6:7:8%eth0");

  // Hiding 1
  validate_ipv6("[::1:2:3:4:5:6:7%25eth0]", "::1:2:3:4:5:6:7%eth0");
  validate_ipv6("[1::2:3:4:5:6:7%25eth0]", "1::2:3:4:5:6:7%eth0");
  validate_ipv6("[1:2::3:4:5:6:7%25eth0]", "1:2::3:4:5:6:7%eth0");
  validate_ipv6("[1:2:3::4:5:6:7%25eth0]", "1:2:3::4:5:6:7%eth0");
  validate_ipv6("[1:2:3:4::5:6:7%25eth0]", "1:2:3:4::5:6:7%eth0");
  validate_ipv6("[1:2:3:4:5::6:7%25eth0]", "1:2:3:4:5::6:7%eth0");
  validate_ipv6("[1:2:3:4:5:6::7%25eth0]", "1:2:3:4:5:6::7%eth0");
  validate_ipv6("[1:2:3:4:5:6:7::%25eth0]", "1:2:3:4:5:6:7::%eth0");

  // Hiding 2
  validate_ipv6("[::1:2:3:4:5:6%25eth0]", "::1:2:3:4:5:6%eth0");
  validate_ipv6("[1::2:3:4:5:6%25eth0]", "1::2:3:4:5:6%eth0");
  validate_ipv6("[1:2::3:4:5:6%25eth0]", "1:2::3:4:5:6%eth0");
  validate_ipv6("[1:2:3::4:5:6%25eth0]", "1:2:3::4:5:6%eth0");
  validate_ipv6("[1:2:3:4::5:6%25eth0]", "1:2:3:4::5:6%eth0");
  validate_ipv6("[1:2:3:4:5::6%25eth0]", "1:2:3:4:5::6%eth0");
  validate_ipv6("[1:2:3:4:5:6::%25eth0]", "1:2:3:4:5:6::%eth0");

  // Hiding 3
  validate_ipv6("[::1:2:3:4:5%25eth0]", "::1:2:3:4:5%eth0");
  validate_ipv6("[1::2:3:4:5%25eth0]", "1::2:3:4:5%eth0");
  validate_ipv6("[1:2::3:4:5%25eth0]", "1:2::3:4:5%eth0");
  validate_ipv6("[1:2:3::4:5%25eth0]", "1:2:3::4:5%eth0");
  validate_ipv6("[1:2:3:4::5%25eth0]", "1:2:3:4::5%eth0");
  validate_ipv6("[1:2:3:4:5::%25eth0]", "1:2:3:4:5::%eth0");

  // Hiding 4
  validate_ipv6("[::1:2:3:4%25eth0]", "::1:2:3:4%eth0");
  validate_ipv6("[1::2:3:4%25eth0]", "1::2:3:4%eth0");
  validate_ipv6("[1:2::3:4%25eth0]", "1:2::3:4%eth0");
  validate_ipv6("[1:2:3::4%25eth0]", "1:2:3::4%eth0");
  validate_ipv6("[1:2:3:4::%25eth0]", "1:2:3:4::%eth0");

  // Hiding 5
  validate_ipv6("[::1:2:3%25eth0]", "::1:2:3%eth0");
  validate_ipv6("[1::2:3%25eth0]", "1::2:3%eth0");
  validate_ipv6("[1:2::3%25eth0]", "1:2::3%eth0");
  validate_ipv6("[1:2:3::%25eth0]", "1:2:3::%eth0");

  // Hiding 6
  validate_ipv6("[::1:2%25eth0]", "::1:2%eth0");
  validate_ipv6("[1::2%25eth0]", "1::2%eth0");
  validate_ipv6("[1:2::%25eth0]", "1:2::%eth0");

  // Hiding 7
  validate_ipv6("[::1%25eth0]", "::1%eth0");
  validate_ipv6("[1::%25eth0]", "1::%eth0");

  // Hiding 8
  validate_ipv6("[::%25eth0]", "::%eth0");

  // IPv4 Embedded
  // Complete
  validate_ipv6("[1:2:3:4:5:6:1.2.3.4%25eth0]", "1:2:3:4:5:6:1.2.3.4%eth0");

  // Hiding 1
  validate_ipv6("[::1:2:3:4:5:1.2.3.4%25eth0]", "::1:2:3:4:5:1.2.3.4%eth0");
  validate_ipv6("[1::2:3:4:5:1.2.3.4%25eth0]", "1::2:3:4:5:1.2.3.4%eth0");
  validate_ipv6("[1:2::3:4:5:1.2.3.4%25eth0]", "1:2::3:4:5:1.2.3.4%eth0");
  validate_ipv6("[1:2:3::4:5:1.2.3.4%25eth0]", "1:2:3::4:5:1.2.3.4%eth0");
  validate_ipv6("[1:2:3:4::5:1.2.3.4%25eth0]", "1:2:3:4::5:1.2.3.4%eth0");
  validate_ipv6("[1:2:3:4:5::1.2.3.4%25eth0]", "1:2:3:4:5::1.2.3.4%eth0");

  // Hiding 2
  validate_ipv6("[::1:2:3:4:1.2.3.4%25eth0]", "::1:2:3:4:1.2.3.4%eth0");
  validate_ipv6("[1::2:3:4:1.2.3.4%25eth0]", "1::2:3:4:1.2.3.4%eth0");
  validate_ipv6("[1:2::3:4:1.2.3.4%25eth0]", "1:2::3:4:1.2.3.4%eth0");
  validate_ipv6("[1:2:3::4:1.2.3.4%25eth0]", "1:2:3::4:1.2.3.4%eth0");
  validate_ipv6("[1:2:3:4::1.2.3.4%25eth0]", "1:2:3:4::1.2.3.4%eth0");

  // Hiding 3
  validate_ipv6("[::1:2:3:1.2.3.4%25eth0]", "::1:2:3:1.2.3.4%eth0");
  validate_ipv6("[1::2:3:1.2.3.4%25eth0]", "1::2:3:1.2.3.4%eth0");
  validate_ipv6("[1:2::3:1.2.3.4%25eth0]", "1:2::3:1.2.3.4%eth0");
  validate_ipv6("[1:2:3::1.2.3.4%25eth0]", "1:2:3::1.2.3.4%eth0");

  // Hiding 4
  validate_ipv6("[::1:2:1.2.3.4%25eth0]", "::1:2:1.2.3.4%eth0");
  validate_ipv6("[1::2:1.2.3.4%25eth0]", "1::2:1.2.3.4%eth0");
  validate_ipv6("[1:2::1.2.3.4%25eth0]", "1:2::1.2.3.4%eth0");

  // Hiding 5
  validate_ipv6("[::1:1.2.3.4%25eth0]", "::1:1.2.3.4%eth0");
  validate_ipv6("[1::1.2.3.4%25eth0]", "1::1.2.3.4%eth0");

  // Hiding 6
  validate_ipv6("[::1.2.3.4%25eth0]", "::1.2.3.4%eth0");

  // Hexadecimals are OK
  validate_ipv6("[A:B:C:D:E:F:7:8%25eth0]", "A:B:C:D:E:F:7:8%eth0");
}

TEST(Uri_parser, parse_socket) {
  validate_uri("mysqlx://user:password@/socket", "mysqlx", "user", "password",
               NO_HOST, NO_PORT, "/socket", NO_SCHEMA, HAS_PASSWORD,
               HAS_NO_PORT, Transport_type::Socket);

  validate_uri("mysqlx://user:password@/socket/schema", "mysqlx", "user",
               "password", NO_HOST, NO_PORT, "/socket", "schema", HAS_PASSWORD,
               HAS_NO_PORT, Transport_type::Socket, true, false);

  // TODO(rennox): Should parser/encoder include the trailing / on these cases??
  validate_uri("mysqlx://user:password@/socket/", "mysqlx", "user", "password",
               NO_HOST, NO_PORT, "/socket", NO_SCHEMA, HAS_PASSWORD,
               HAS_NO_PORT, Transport_type::Socket, false);
  validate_uri("mysqlx://user:password@/socket", "mysqlx", "user", "password",
               NO_HOST, NO_PORT, "/socket", NO_SCHEMA, HAS_PASSWORD,
               HAS_NO_PORT, Transport_type::Socket);
  validate_uri("mysqlx://user:password@a/schema", "mysqlx", "user", "password",
               "a", NO_PORT, NO_SOCK, "schema", HAS_PASSWORD, HAS_NO_PORT,
               Transport_type::Tcp);
  validate_uri("mysqlx://user:password@a/", "mysqlx", "user", "password", "a",
               NO_PORT, NO_SOCK, NO_SCHEMA, HAS_PASSWORD, HAS_NO_PORT,
               Transport_type::Tcp, false);

  validate_uri("mysqlx://user:password@/path%2Fto%2Fsocket/schema", "mysqlx",
               "user", "password", NO_HOST, NO_PORT, "/path/to/socket",
               "schema", HAS_PASSWORD, HAS_NO_PORT, Transport_type::Socket,
               true, false);
  validate_uri("mysqlx://user:password@/path%2Fto%2Fsocket/", "mysqlx", "user",
               "password", NO_HOST, NO_PORT, "/path/to/socket", NO_SCHEMA,
               HAS_PASSWORD, HAS_NO_PORT, Transport_type::Socket, false);
  validate_uri("mysqlx://user:password@/path%2Fto%2Fsocket", "mysqlx", "user",
               "password", NO_HOST, NO_PORT, "/path/to/socket", NO_SCHEMA,
               HAS_PASSWORD, HAS_NO_PORT, Transport_type::Socket);

  // TODO(rennox): Should encoder re-create the same path? probably it should,
  // considering the path between parenthesis is to avoid percent encoding by
  // DevAPI standard
  validate_uri("mysqlx://user:password@(/path/to/mysql.sock)/schema", "mysqlx",
               "user", "password", NO_HOST, NO_PORT, "/path/to/mysql.sock",
               "schema", HAS_PASSWORD, HAS_NO_PORT, Transport_type::Socket,
               false);
  validate_uri("mysqlx://user:password@(/path/to/mysql.sock)/", "mysqlx",
               "user", "password", NO_HOST, NO_PORT, "/path/to/mysql.sock",
               NO_SCHEMA, HAS_PASSWORD, HAS_NO_PORT, Transport_type::Socket,
               false);
  validate_uri("mysqlx://user:password@(/path/to/mysql.sock)", "mysqlx", "user",
               "password", NO_HOST, NO_PORT, "/path/to/mysql.sock", NO_SCHEMA,
               HAS_PASSWORD, HAS_NO_PORT, Transport_type::Socket, false);

  validate_uri("mysqlx://user:password@(/path\\to\\mysql.sock)/schema",
               "mysqlx", "user", "password", NO_HOST, NO_PORT,
               "/path\\to\\mysql.sock", "schema", HAS_PASSWORD, HAS_NO_PORT,
               Transport_type::Socket, false);
  validate_uri("mysqlx://user:password@(/path\\to\\mysql.sock)/", "mysqlx",
               "user", "password", NO_HOST, NO_PORT, "/path\\to\\mysql.sock",
               NO_SCHEMA, HAS_PASSWORD, HAS_NO_PORT, Transport_type::Socket,
               false);
  validate_uri("mysqlx://user:password@(/path\\to\\mysql.sock)", "mysqlx",
               "user", "password", NO_HOST, NO_PORT, "/path\\to\\mysql.sock",
               NO_SCHEMA, HAS_PASSWORD, HAS_NO_PORT, Transport_type::Socket,
               false);

  validate_uri("mysqlx://user:password@.%2Fpath%2Fto%2Fsocket/schema", "mysqlx",
               "user", "password", NO_HOST, NO_PORT, "./path/to/socket",
               "schema", HAS_PASSWORD, HAS_NO_PORT, Transport_type::Socket,
               true, false);
  validate_uri("mysqlx://user:password@.%2Fpath%2Fto%2Fsocket/", "mysqlx",
               "user", "password", NO_HOST, NO_PORT, "./path/to/socket",
               NO_SCHEMA, HAS_PASSWORD, HAS_NO_PORT, Transport_type::Socket,
               false);
  validate_uri("mysqlx://user:password@.%2Fpath%2Fto%2Fsocket", "mysqlx",
               "user", "password", NO_HOST, NO_PORT, "./path/to/socket",
               NO_SCHEMA, HAS_PASSWORD, HAS_NO_PORT, Transport_type::Socket,
               true, false);

  validate_uri("mysqlx://user:password@(./path/to/socket)/schema", "mysqlx",
               "user", "password", NO_HOST, NO_PORT, "./path/to/socket",
               "schema", HAS_PASSWORD, HAS_NO_PORT, Transport_type::Socket,
               false);
  validate_uri("mysqlx://user:password@(./path/to/socket)/", "mysqlx", "user",
               "password", NO_HOST, NO_PORT, "./path/to/socket", NO_SCHEMA,
               HAS_PASSWORD, HAS_NO_PORT, Transport_type::Socket, false);
  validate_uri("mysqlx://user:password@(./path/to/socket)", "mysqlx", "user",
               "password", NO_HOST, NO_PORT, "./path/to/socket", NO_SCHEMA,
               HAS_PASSWORD, HAS_NO_PORT, Transport_type::Socket, false);

  validate_uri("mysqlx://user:password@..%2Fpath%2Fto%2Fsocket/schema",
               "mysqlx", "user", "password", NO_HOST, NO_PORT,
               "../path/to/socket", "schema", HAS_PASSWORD, HAS_NO_PORT,
               Transport_type::Socket, false);
  validate_uri("mysqlx://user:password@..%2Fpath%2Fto%2Fsocket/", "mysqlx",
               "user", "password", NO_HOST, NO_PORT, "../path/to/socket",
               NO_SCHEMA, HAS_PASSWORD, HAS_NO_PORT, Transport_type::Socket,
               false);
  validate_uri("mysqlx://user:password@..%2Fpath%2Fto%2Fsocket", "mysqlx",
               "user", "password", NO_HOST, NO_PORT, "../path/to/socket",
               NO_SCHEMA, HAS_PASSWORD, HAS_NO_PORT, Transport_type::Socket,
               false);

  validate_uri("mysqlx://user:password@(../path/to/socket)/schema", "mysqlx",
               "user", "password", NO_HOST, NO_PORT, "../path/to/socket",
               "schema", HAS_PASSWORD, HAS_NO_PORT, Transport_type::Socket,
               false);
  validate_uri("mysqlx://user:password@(../path/to/socket)/", "mysqlx", "user",
               "password", NO_HOST, NO_PORT, "../path/to/socket", NO_SCHEMA,
               HAS_PASSWORD, HAS_NO_PORT, Transport_type::Socket, false);
  validate_uri("mysqlx://user:password@(../path/to/socket)", "mysqlx", "user",
               "password", NO_HOST, NO_PORT, "../path/to/socket", NO_SCHEMA,
               HAS_PASSWORD, HAS_NO_PORT, Transport_type::Socket, false);

  validate_bad_uri("mysqlx://user:password@/path/to/socket/schema",
                   "Illegal character [/] found at position 28");
  validate_bad_uri("mysqlx://user:password@./path/to/socket/schema",
                   "Illegal character [/] found at position 24");
  validate_bad_uri("mysqlx://user:password@../path/to/socket/schema",
                   "Illegal character [/] found at position 25");
}

TEST(Uri_parser, parse_pipe) {
  // TODO(rennox): Another case to be reviewed, should the encoder produce the
  // exact path that was originally parsed??
  validate_uri("mysql://user:password@\\\\.\\d%3A%5Cpath%5Cto%5Csocket/schema",
               "mysql", "user", "password", NO_HOST, NO_PORT,
               "d:\\path\\to\\socket", "schema", HAS_PASSWORD, HAS_NO_PORT,
               Transport_type::Pipe, false);
  validate_uri("mysql://user:password@\\\\.\\d%3A%5Cpath%5Cto%5Csocket/",
               "mysql", "user", "password", NO_HOST, NO_PORT,
               "d:\\path\\to\\socket", NO_SCHEMA, HAS_PASSWORD, HAS_NO_PORT,
               Transport_type::Pipe, false);
  validate_uri("mysql://user:password@\\\\.\\d%3A%5Cpath%5Cto%5Csocket",
               "mysql", "user", "password", NO_HOST, NO_PORT,
               "d:\\path\\to\\socket", NO_SCHEMA, HAS_PASSWORD, HAS_NO_PORT,
               Transport_type::Pipe, false);

  validate_uri("mysql://user:password@(\\\\.\\d:%5Cpath%5Cto%5Csocket)/schema",
               "mysql", "user", "password", NO_HOST, NO_PORT,
               "d:\\path\\to\\socket", "schema", HAS_PASSWORD, HAS_NO_PORT,
               Transport_type::Pipe, false);
  validate_uri("mysql://user:password@(\\\\.\\d:%5Cpath%5Cto%5Csocket)/",
               "mysql", "user", "password", NO_HOST, NO_PORT,
               "d:\\path\\to\\socket", NO_SCHEMA, HAS_PASSWORD, HAS_NO_PORT,
               Transport_type::Pipe, false);
  validate_uri("mysql://user:password@(\\\\.\\d:%5Cpath%5Cto%5Csocket)",
               "mysql", "user", "password", NO_HOST, NO_PORT,
               "d:\\path\\to\\socket", NO_SCHEMA, HAS_PASSWORD, HAS_NO_PORT,
               Transport_type::Pipe, false);

  validate_uri("mysql://user:password@\\\\.\\d%3A%2Fpath%2Fto%2Fsocket/schema",
               "mysql", "user", "password", NO_HOST, NO_PORT,
               "d:/path/to/socket", "schema", HAS_PASSWORD, HAS_NO_PORT,
               Transport_type::Pipe, false);
  validate_uri("mysql://user:password@\\\\.\\d%3A%2Fpath%2Fto%2Fsocket/",
               "mysql", "user", "password", NO_HOST, NO_PORT,
               "d:/path/to/socket", NO_SCHEMA, HAS_PASSWORD, HAS_NO_PORT,
               Transport_type::Pipe, false);
  validate_uri("mysql://user:password@\\\\.\\d%3A%2Fpath%2Fto%2Fsocket",
               "mysql", "user", "password", NO_HOST, NO_PORT,
               "d:/path/to/socket", NO_SCHEMA, HAS_PASSWORD, HAS_NO_PORT,
               Transport_type::Pipe, false);

  validate_uri("mysql://user:password@(\\\\.\\d:/path/to/socket)/schema",
               "mysql", "user", "password", NO_HOST, NO_PORT,
               "d:/path/to/socket", "schema", HAS_PASSWORD, HAS_NO_PORT,
               Transport_type::Pipe, false);
  validate_uri("mysql://user:password@(\\\\.\\d:/path/to/socket)/", "mysql",
               "user", "password", NO_HOST, NO_PORT, "d:/path/to/socket",
               NO_SCHEMA, HAS_PASSWORD, HAS_NO_PORT, Transport_type::Pipe,
               false);
  validate_uri("mysql://user:password@(\\\\.\\d:/path/to/socket)", "mysql",
               "user", "password", NO_HOST, NO_PORT, "d:/path/to/socket",
               NO_SCHEMA, HAS_PASSWORD, HAS_NO_PORT, Transport_type::Pipe,
               false);

  validate_uri("mysql://user:password@(\\\\.\\d:\\path\\to\\socket)/schema",
               "mysql", "user", "password", NO_HOST, NO_PORT,
               "d:\\path\\to\\socket", "schema", HAS_PASSWORD, HAS_NO_PORT,
               Transport_type::Pipe, false);
  validate_uri("mysql://user:password@(\\\\.\\d:\\path\\to\\socket)/", "mysql",
               "user", "password", NO_HOST, NO_PORT, "d:\\path\\to\\socket",
               NO_SCHEMA, HAS_PASSWORD, HAS_NO_PORT, Transport_type::Pipe,
               false);
  validate_uri("mysql://user:password@(\\\\.\\d:\\path\\to\\socket)", "mysql",
               "user", "password", NO_HOST, NO_PORT, "d:\\path\\to\\socket",
               NO_SCHEMA, HAS_PASSWORD, HAS_NO_PORT, Transport_type::Pipe,
               false);

  validate_bad_uri(
      "mysql://user:password@\\\\.\\d:%5Cpath%5Cto%5Csocket/schema",
      "Illegal character [:] found at position 27");
  validate_bad_uri(
      "mysql://user:password@\\\\.\\d%3A\\path%5Cto%5Csocket/schema",
      "Illegal character [\\] found at position 30");

  validate_bad_uri("mysqlx://user:password@(\\\\.\\d:/path/to/socket)",
                   "Pipe can only be used with Classic session");
  validate_bad_uri("mysql://user:password@\\(\\\\.\\d:\\path\\to\\socket)",
                   "Illegal character [\\] found at position 22");

  validate_bad_uri("mysql://user:password@\\\\.\\",
                   "Named pipe cannot be empty.");
  validate_bad_uri("mysql://user:password@(\\\\.\\)",
                   "Named pipe cannot be empty.");
}

TEST(Uri_parser, parse_path) {
  validate_uri("mysqlx://user@10.150.123.45:2845/", "mysqlx", "user",
               NO_PASSWORD, "10.150.123.45", 2845, NO_SOCK, NO_DB,
               HAS_NO_PASSWORD, HAS_PORT, Transport_type::Tcp, false);
  validate_uri("mysqlx://user@10.150.123.45:2845/world", "mysqlx", "user",
               NO_PASSWORD, "10.150.123.45", 2845, NO_SOCK, "world",
               HAS_NO_PASSWORD, HAS_PORT, Transport_type::Tcp);
  validate_uri("mysqlx://user@10.150.123.45:2845/world@x", "mysqlx", "user",
               NO_PASSWORD, "10.150.123.45", 2845, NO_SOCK, "world@x",
               HAS_NO_PASSWORD, HAS_PORT, Transport_type::Tcp, true, false);
  validate_uri("mysqlx://user@10.150.123.45:2845/world:x", "mysqlx", "user",
               NO_PASSWORD, "10.150.123.45", 2845, NO_SOCK, "world:x",
               HAS_NO_PASSWORD, HAS_PORT, Transport_type::Tcp, true, false);
  validate_uri("mysqlx://user@10.150.123.45:2845/world%20x", "mysqlx", "user",
               NO_PASSWORD, "10.150.123.45", 2845, NO_SOCK, "world x",
               HAS_NO_PASSWORD, HAS_PORT, Transport_type::Tcp);

  //                0    0    1    1    2    2    3    3    4    4    5    5
  //                0    5    0    5    0    5    0    5    0    5    0    5
  validate_bad_uri("mysqlx://user@10.150.123.45:2845/world%x",
                   "Illegal character [%] found at position 38");
  validate_bad_uri("mysqlx://user@10.150.123.45:2845/world%2x",
                   "Illegal character [%] found at position 38");
  validate_bad_uri("mysqlx://user@10.150.123.45:2845/world%20x/subpath",
                   "Illegal character [/] found at position 32");
}

// NOTE(rennox): The URI parser is able to parse arrays and null values
// But the produced Connection_options does not allow them since they
// are not required by the current connection specs
TEST(Uri_parser, parse_query) {
  std::map<std::string, std::vector<std::string>> atts;
  atts[mysqlshdk::db::kSslCa] = {"value"};
  atts[mysqlshdk::db::kAuthMethod] = {"my space"};
  // atts[mysqlshdk::db::kSslCert] = {};
  atts[mysqlshdk::db::kSslCert] = {"cert"};
  atts[mysqlshdk::db::kSslCert] = {"cert"};
  // atts[kSslCaPath] = {"[one,two,three]"};
  atts[mysqlshdk::db::kSslCaPath] = {"ca_path"};
  atts[mysqlshdk::db::kSslCipher] = {"/what/e/ver"};
  // validate_uri("mysqlx://user@10.150.123.45:2845/world?ssl-ca=value&auth-method=my%20space&ssl-cert&sslCapath=[one,two,three]&ssl-cipher=(/what/e/ver)",
  // "mysqlx", "user", NO_PASSWORD, "10.150.123.45", 2845, NO_SOCK, "world",
  // HAS_NO_PASSWORD, HAS_PORT, Transport_type::Tcp, &atts);
  validate_uri(
      "mysqlx://user@10.150.123.45:2845/"
      "world?ssl-ca=value&auth-method=my%20space&ssl-cert=cert&ssl-capath=ca_"
      "path&"
      "ssl-cipher=(/what/e/ver)",
      "mysqlx", "user", NO_PASSWORD, "10.150.123.45", 2845, NO_SOCK, "world",
      HAS_NO_PASSWORD, HAS_PORT, Transport_type::Tcp, false, false, &atts);
}

TEST(Uri_parser, parse_file) {
  validate_file_uri("file:/sample/file/path", "file", NO_HOST,
                    "/sample/file/path");
  validate_file_uri("file://user", "file", "user", "");
  validate_file_uri("file:/some%20path", "file", NO_HOST, "/some%20path");
}

TEST(Uri_parser, hide_password_in_uri) {
  auto check = [](mysqlshdk::db::uri::Type /*type*/, const std::string &expect,
                  const std::string &input) {
    EXPECT_EQ(expect, mysqlshdk::db::uri::hide_password_in_uri(input));
    EXPECT_LE(expect.size(), input.size());
  };

  {
    mysqlshdk::db::uri::Type type = mysqlshdk::db::uri::Type::DevApi;
    check(type, "root@abcroot123.com:3306",
          "%72%6f%6f%74:123456@abcroot123.com:3306");
    check(type, "abcroot123.com:3306", "abcroot123.com:3306");
    check(type, "root@abcroot123.com:3306", "%72%6f%6f%74@abcroot123.com:3306");
    check(type, "mysql://ro@(/path/to/whatever/socket.sock)",
          "mysql://ro:::ot:fd%40safdsa@(/path/to/whatever/socket.sock)");
    check(type, "mysql://ro@(/path/to/whatever/socket.sock)",
          "mysql://ro@(/path/to/whatever/socket.sock)");
  }
  {
    mysqlshdk::db::uri::Type type = mysqlshdk::db::uri::Type::Ssh;
    check(type, "ssh://mysql.com", "ssh://mysql.com");
    check(type, "ssh://user.name@mysql.com", "ssh://user.name:pwd@mysql.com");
    check(type, "ssh://user.name@mysql.com", "ssh://user.name@mysql.com");
    check(type, "ssh://user@10.150.123.45:2845",
          "ssh://user:pass@10.150.123.45:2845");
    check(type, "ssh://user@10.150.123.45:2845",
          "ssh://user@10.150.123.45:2845");
  }
}

}  // namespace proj_parser_tests
}  // namespace testing
