/* Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License, version 2.0,
 as published by the Free Software Foundation.

 This program is also distributed with certain software (including
 but not limited to OpenSSL) that is licensed under separate terms, as
 designated in a particular file or component or in included license
 documentation.  The authors of MySQL hereby grant you an additional
 permission to link the program and your derivative works with the
 separately licensed software that they have included with MySQL.
 This program is distributed in the hope that it will be useful,  but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 the GNU General Public License, version 2.0, for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA */

#include <algorithm>
#include <functional>
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "unittest/gtest_clean.h"

#define MY_EXPECT_THROW(e, m, c)         \
  EXPECT_THROW(                          \
      {                                  \
        try {                            \
          c;                             \
        } catch (const e& error) {       \
          EXPECT_STREQ(m, error.what()); \
          throw;                         \
        }                                \
      },                                 \
      e)

using mysqlshdk::db::Connection_options;
using mysqlshdk::utils::nullable_options::Comparison_mode;
using mysqlshdk::db::Transport_type;
using mysqlshdk::db::uri_connection_attributes;
namespace testing {

TEST(Connection_options, default_initialization) {
  Connection_options options;

  ASSERT_FALSE(options.has_scheme());
  ASSERT_FALSE(options.has_user());
  ASSERT_FALSE(options.has_password());
  ASSERT_FALSE(options.has_host());
  ASSERT_FALSE(options.has_port());
  ASSERT_FALSE(options.has_schema());
  ASSERT_FALSE(options.has_socket());
  ASSERT_FALSE(options.has_pipe());
  ASSERT_FALSE(options.has_transport_type());
  ASSERT_FALSE(options.get_ssl_options().has_data());

  // has verifies the existence of the option
  EXPECT_TRUE(options.has(mysqlshdk::db::kScheme));
  EXPECT_TRUE(options.has(mysqlshdk::db::kUser));
  EXPECT_TRUE(options.has(mysqlshdk::db::kPassword));
  EXPECT_TRUE(options.has(mysqlshdk::db::kHost));
  EXPECT_TRUE(options.has(mysqlshdk::db::kPort));
  EXPECT_TRUE(options.has(mysqlshdk::db::kSchema));
  EXPECT_TRUE(options.has(mysqlshdk::db::kSocket));
  EXPECT_TRUE(options.has(mysqlshdk::db::kSocket));

  // has_value verifies the existence of a value for the option
  EXPECT_FALSE(options.has_value(mysqlshdk::db::kSchema));
  EXPECT_FALSE(options.has_value(mysqlshdk::db::kUser));
  EXPECT_FALSE(options.has_value(mysqlshdk::db::kPassword));
  EXPECT_FALSE(options.has_value(mysqlshdk::db::kHost));
  EXPECT_FALSE(options.has_value(mysqlshdk::db::kPort));
  EXPECT_FALSE(options.has_value(mysqlshdk::db::kSchema));
  EXPECT_FALSE(options.has_value(mysqlshdk::db::kSocket));
}

TEST(Connection_options, scheme_functions) {
  Connection_options options;
  std::string msg;

  EXPECT_NO_THROW(options.set_scheme("mysql"));
  msg = "The connection option '";
  msg.append(mysqlshdk::db::kScheme);
  msg.append("' is already defined as 'mysql'.");
  MY_EXPECT_THROW(std::invalid_argument, msg.c_str(),
                  options.set_scheme("mysqlx"));
  EXPECT_TRUE(options.has_scheme());
  EXPECT_STREQ("mysql", options.get_scheme().c_str());
  EXPECT_NO_THROW(options.clear_scheme());
  EXPECT_FALSE(options.has_scheme());
  msg = "The connection option '";
  msg.append(mysqlshdk::db::kScheme).append("' has no value.");
  MY_EXPECT_THROW(std::invalid_argument, msg.c_str(), options.get_scheme());
}

TEST(Connection_options, user_functions) {
  Connection_options options;
  std::string msg;

  EXPECT_NO_THROW(options.set_user("value"));
  msg = "The connection option '";
  msg.append(mysqlshdk::db::kUser);
  msg.append("' is already defined as 'value'.");
  MY_EXPECT_THROW(std::invalid_argument, msg.c_str(),
                  options.set_user("value1"));
  EXPECT_TRUE(options.has_user());
  EXPECT_STREQ("value", options.get_user().c_str());
  EXPECT_NO_THROW(options.clear_user());
  EXPECT_FALSE(options.has_user());
  msg = "The connection option '";
  msg.append(mysqlshdk::db::kUser).append("' has no value.");
  MY_EXPECT_THROW(std::invalid_argument, msg.c_str(), options.get_user());
}

TEST(Connection_options, password_functions) {
  Connection_options options;
  std::string msg;

  EXPECT_NO_THROW(options.set_password("value"));
  msg = "The connection option '";
  msg.append(mysqlshdk::db::kPassword);
  msg.append("' is already defined as 'value'.");
  MY_EXPECT_THROW(std::invalid_argument, msg.c_str(),
                  options.set_password("value1"));
  EXPECT_TRUE(options.has_password());
  EXPECT_STREQ("value", options.get_password().c_str());
  EXPECT_NO_THROW(options.clear_password());
  EXPECT_FALSE(options.has_password());
  msg = "The connection option '";
  msg.append(mysqlshdk::db::kPassword).append("' has no value.");
  MY_EXPECT_THROW(std::invalid_argument, msg.c_str(), options.get_password());
}

TEST(Connection_options, host_functions) {
  Connection_options options;
  std::string msg;

  EXPECT_NO_THROW(options.set_host("value"));
  msg = "The connection option '";
  msg.append(mysqlshdk::db::kHost);
  msg.append("' is already defined as 'value'.");
  MY_EXPECT_THROW(std::invalid_argument, msg.c_str(),
                  options.set_host("value1"));
  EXPECT_TRUE(options.has_host());
  EXPECT_STREQ("value", options.get_host().c_str());
  EXPECT_NO_THROW(options.clear_host());
  EXPECT_FALSE(options.has_host());
  msg = "The connection option '";
  msg.append(mysqlshdk::db::kHost).append("' has no value.");
  MY_EXPECT_THROW(std::invalid_argument, msg.c_str(), options.get_host());
}

TEST(Connection_options, port_functions) {
  Connection_options options;
  std::string msg;

  EXPECT_NO_THROW(options.set_port(3306));

  msg =
      "Unable to set a tcp connection to port '3307', a tcp connection to "
      "port '3306' is already defined.";
  MY_EXPECT_THROW(std::invalid_argument, msg.c_str(), options.set_port(3307));
  EXPECT_TRUE(options.has_port());
  EXPECT_EQ(3306, options.get_port());
  EXPECT_NO_THROW(options.clear_port());
  EXPECT_FALSE(options.has_port());
  msg = "The connection option '";
  msg.append(mysqlshdk::db::kPort).append("' has no value.");
  MY_EXPECT_THROW(std::invalid_argument, msg.c_str(), options.get_port());
}

TEST(Connection_options, schema_functions) {
  Connection_options options;
  std::string msg;

  EXPECT_NO_THROW(options.set_schema("value"));
  msg = "The connection option '";
  msg.append(mysqlshdk::db::kSchema);
  msg.append("' is already defined as 'value'.");
  MY_EXPECT_THROW(std::invalid_argument, msg.c_str(),
                  options.set_schema("value1"));
  EXPECT_TRUE(options.has_schema());
  EXPECT_STREQ("value", options.get_schema().c_str());
  EXPECT_NO_THROW(options.clear_schema());
  EXPECT_FALSE(options.has_schema());
  msg = "The connection option '";
  msg.append(mysqlshdk::db::kSchema).append("' has no value.");
  MY_EXPECT_THROW(std::invalid_argument, msg.c_str(), options.get_schema());
}

TEST(Connection_options, socket_functions) {
  Connection_options options;
  std::string msg;

  EXPECT_NO_THROW(options.set_socket("value"));
  msg = "The connection option '";
  msg.append(mysqlshdk::db::kSocket);
  msg.append("' is already defined as 'value'.");
  MY_EXPECT_THROW(std::invalid_argument, msg.c_str(),
                  options.set_socket("value1"));
  EXPECT_TRUE(options.has_socket());
  EXPECT_STREQ("value", options.get_socket().c_str());
  EXPECT_NO_THROW(options.clear_socket());
  EXPECT_FALSE(options.has_socket());
  msg = "The connection option '";
  msg.append(mysqlshdk::db::kSocket).append("' has no value.");
  MY_EXPECT_THROW(std::invalid_argument, msg.c_str(), options.get_socket());
}

TEST(Connection_options, pipe_functions) {
  Connection_options options;
  std::string msg;

  EXPECT_NO_THROW(options.set_pipe("value"));
  msg = "The connection option '";
  msg.append(mysqlshdk::db::kSocket);
  msg.append("' is already defined as 'value'.");
  MY_EXPECT_THROW(std::invalid_argument, msg.c_str(),
                  options.set_pipe("value1"));
  EXPECT_TRUE(options.has_pipe());
  EXPECT_STREQ("value", options.get_pipe().c_str());
  EXPECT_NO_THROW(options.clear_pipe());
  EXPECT_FALSE(options.has_pipe());
  msg = "The connection option '";
  msg.append(mysqlshdk::db::kSocket).append("' has no value.");
  MY_EXPECT_THROW(std::invalid_argument, msg.c_str(), options.get_pipe());
}

TEST(Connection_options, uri_constructor) {
  mysqlshdk::db::Connection_options data(
      "mysqlx://user:password@localhost:3300/"
      "myschema?ssl-mode=REQUIRED");

  ASSERT_TRUE(data.has_scheme());
  ASSERT_STREQ("mysqlx", data.get_scheme().c_str());

  ASSERT_TRUE(data.has_user());
  ASSERT_STREQ("user", data.get_user().c_str());

  ASSERT_TRUE(data.has_password());
  ASSERT_STREQ("password", data.get_password().c_str());

  ASSERT_TRUE(data.has_host());
  ASSERT_STREQ("localhost", data.get_host().c_str());

  ASSERT_TRUE(data.has_port());
  ASSERT_EQ(3300, data.get_port());

  ASSERT_TRUE(data.has_schema());
  ASSERT_STREQ("myschema", data.get_schema().c_str());

  ASSERT_FALSE(data.has_socket());
  ASSERT_FALSE(data.has_pipe());
  ASSERT_TRUE(data.get_ssl_options().has_data());

  auto ssl_options = data.get_ssl_options();
  ASSERT_TRUE(ssl_options.has_mode());
  ASSERT_EQ(mysqlshdk::db::Ssl_mode::Required, ssl_options.get_mode());
  ASSERT_FALSE(ssl_options.has_ca());
  ASSERT_FALSE(ssl_options.has_capath());
  ASSERT_FALSE(ssl_options.has_cert());
  ASSERT_FALSE(ssl_options.has_cipher());
  ASSERT_FALSE(ssl_options.has_crl());
  ASSERT_FALSE(ssl_options.has_crlpath());
  ASSERT_FALSE(ssl_options.has_key());
  ASSERT_FALSE(ssl_options.has_tls_version());
}

void combine(
    const std::string& input, const std::string& twisted, size_t index,
    std::function<void(const std::string&, const std::string)> callback) {
  std::string my_string(twisted);
  if (my_string.empty())
    my_string = shcore::str_lower(input);

  if (index >= my_string.size()) {
    callback(input, my_string);
    return;
  }

  my_string[index] = std::tolower(my_string[index]);
  combine(input, twisted, index + 1, callback);

  my_string[index] = std::toupper(my_string[index]);
  combine(input, my_string, index + 1, callback);
}

namespace case_insensitive {
auto callback = [](const std::string& property, const std::string& twisted,
                   const std::string& arg_value) {
  std::string uri = "mysql://root@host?" + twisted + "=" + arg_value;
  mysqlshdk::db::Connection_options data(uri);
  if (!data.has(property)) {
    std::string failed = "Failed Property: " + twisted;
    SCOPED_TRACE(failed);
    ADD_FAILURE();
  } else {
    std::string value = data.get(property);
    std::string failed = "Unexpected Value: " + value;
    SCOPED_TRACE(failed);
    EXPECT_STREQ(arg_value.c_str(), value.c_str());
  }
};
}  // namespace case_insensitive

TEST(Connection_options, case_insensitive_ssl_mode) {
  // This callback will get called with every combination of
  // uppercase/lowercase letters on ssl-mode
  auto callback = std::bind(case_insensitive::callback, std::placeholders::_1,
                            std::placeholders::_2, "REQUIRED");

  combine(mysqlshdk::db::kSslMode, "", 0, callback);
}

TEST(Connection_options, case_insensitive_get_server_public_key) {
  // This callback will get called with every combination of
  // uppercase/lowercase letters on get-server-public-key
  auto callback = std::bind(case_insensitive::callback, std::placeholders::_1,
                            std::placeholders::_2, "true");

  // We start from 10th index because this function has O(2^n) complexity
  // and tested string length is 21 ("get-server-public-key").
  combine(mysqlshdk::db::kGetServerPublicKey, "", 10, callback);
}

TEST(Connection_options, case_insensitive_server_public_key_path) {
  // This callback will get called with every combination of
  // uppercase/lowercase letters on server-public-key-path
  auto callback = std::bind(case_insensitive::callback, std::placeholders::_1,
                            std::placeholders::_2, "pubkey.pem");

  // We start from 11th index because this function has O(2^n) complexity
  // and tested string length is 22 ("server-public-key-path").
  combine(mysqlshdk::db::kServerPublicKeyPath, "", 11, callback);
}

TEST(Connection_options, case_insensitive_options) {
  // This callback will get called with every combination of
  // uppercase/lowercase letters for each property
  auto callback = std::bind(case_insensitive::callback, std::placeholders::_1,
                            std::placeholders::_2, "whatever");

  std::set<std::string> attributes(uri_connection_attributes.begin(),
                                   uri_connection_attributes.end());

  // handled by other, more specific tests
  attributes.erase(mysqlshdk::db::kSslMode);
  attributes.erase(mysqlshdk::db::kGetServerPublicKey);
  attributes.erase(mysqlshdk::db::kServerPublicKeyPath);

  for (auto property : attributes) {
    combine(property, "", 0, callback);
  }
}

TEST(Connection_options, set_host) {
  // localhost does not determine the session type
  {
    mysqlshdk::db::Connection_options data;
    data.set_host("localhost");

    EXPECT_TRUE(data.has_host());
    EXPECT_STREQ("localhost", data.get_host().c_str());
    EXPECT_FALSE(data.has_transport_type());
  }

  // Any other host would set the connection type to tcp
  {
    mysqlshdk::db::Connection_options data;
    data.set_host("127.0.0.1");

    EXPECT_TRUE(data.has_host());
    EXPECT_TRUE(data.has_transport_type());
    EXPECT_STREQ("127.0.0.1", data.get_host().c_str());
    EXPECT_EQ(Transport_type::Tcp, data.get_transport_type());
  }

  // Setting host causes no error if i.e. port was set before
  {
    mysqlshdk::db::Connection_options data;
    data.set_port(3306);
    data.set_host("localhost");

    EXPECT_TRUE(data.has_host());
    EXPECT_STREQ("localhost", data.get_host().c_str());
    EXPECT_EQ(Transport_type::Tcp, data.get_transport_type());
  }

  // Setting host causes error if a socket connection is set
  {
    mysqlshdk::db::Connection_options data;
    data.set_socket("Some/Socket/Path");

    MY_EXPECT_THROW(std::invalid_argument,
                    "Unable to set a connection to 'localhost', a socket "
                    "connection to 'Some/Socket/Path' is already defined.",
                    data.set_host("localhost"));
  }

  // Setting host causes error if a pipe connection is set
  {
    mysqlshdk::db::Connection_options data;
    data.set_pipe("PipeName");

    MY_EXPECT_THROW(std::invalid_argument,
                    "Unable to set a connection to 'localhost', a pipe "
                    "connection to 'PipeName' is already defined.",
                    data.set_host("localhost"));
  }
}

TEST(Connection_options, set_socket) {
  {
    mysqlshdk::db::Connection_options data;
    data.set_socket("/Path/To/Socket");

    EXPECT_TRUE(data.has_socket());
    EXPECT_STREQ("/Path/To/Socket", data.get_socket().c_str());
    EXPECT_TRUE(data.has_transport_type());
    EXPECT_EQ(Transport_type::Socket, data.get_transport_type());
  }

  // Using socket is allowed for localhost
  {
    mysqlshdk::db::Connection_options data;
    data.set_host("localhost");
    EXPECT_NO_THROW(data.set_socket("/Path/To/Socket"));

    EXPECT_TRUE(data.has_host());
    EXPECT_TRUE(data.has_socket());
    EXPECT_TRUE(data.has_transport_type());

    EXPECT_STREQ("localhost", data.get_host().c_str());
    EXPECT_STREQ("/Path/To/Socket", data.get_socket().c_str());
    EXPECT_EQ(Transport_type::Socket, data.get_transport_type());
  }

  // Using socket is not allowed for localhost is port was defined
  {
    mysqlshdk::db::Connection_options data;
    data.set_host("localhost");
    data.set_port(3307);

    MY_EXPECT_THROW(std::invalid_argument,
                    "Unable to set a socket connection to '/Path/To/Socket', "
                    "a tcp connection to 'localhost:3307' is already defined.",
                    data.set_socket("/Path/To/Socket"));
  }

  // Using socket is allowed for 127.0.0.1
  {
    mysqlshdk::db::Connection_options data;
    data.set_host("127.0.0.1");
    EXPECT_NO_THROW(data.set_socket("/Path/To/Socket"));

    EXPECT_TRUE(data.has_host());
    EXPECT_TRUE(data.has_socket());
    EXPECT_TRUE(data.has_transport_type());

    EXPECT_STREQ("127.0.0.1", data.get_host().c_str());
    EXPECT_STREQ("/Path/To/Socket", data.get_socket().c_str());
    EXPECT_EQ(Transport_type::Socket, data.get_transport_type());
  }

  // Using socket is not allowed for 127.0.0.1 is port was defined
  {
    mysqlshdk::db::Connection_options data;
    data.set_host("127.0.0.1");
    data.set_port(3307);

    MY_EXPECT_THROW(std::invalid_argument,
                    "Unable to set a socket connection to '/Path/To/Socket', "
                    "a tcp connection to '127.0.0.1:3307' is already defined.",
                    data.set_socket("/Path/To/Socket"));
  }

  // Error trying to set a socket when a host was previously set
  {
    mysqlshdk::db::Connection_options data;
    data.set_host("192.168.1.110");

    MY_EXPECT_THROW(std::invalid_argument,
                    "Unable to set a socket connection to '/Path/To/Socket', "
                    "a tcp connection to '192.168.1.110' is already defined.",
                    data.set_socket("/Path/To/Socket"));
  }

  // Error trying to set socket when a port was previously set
  {
    mysqlshdk::db::Connection_options data;
    data.set_port(3307);

    MY_EXPECT_THROW(std::invalid_argument,
                    "Unable to set a socket connection to '/Path/To/Socket', "
                    "a tcp connection to port '3307' is already defined.",
                    data.set_socket("/Path/To/Socket"));
  }

  // Error trying to set a socket when a pipe was previously set
  {
    mysqlshdk::db::Connection_options data;
    data.set_pipe("PipeName");

    MY_EXPECT_THROW(std::invalid_argument,
                    "Unable to set a socket connection to '/Path/To/Socket', "
                    "a pipe connection to 'PipeName' is already defined.",
                    data.set_socket("/Path/To/Socket"));
  }
}

TEST(Connection_options, case_insensitive_duplicated_attribute) {
  // This callback will get called with every combination of
  // uppercase/lowercase letters on ssl-mode
  auto callback = [](const std::string& property, const std::string& twisted) {
    mysqlshdk::db::Connection_options data;
    data.set(mysqlshdk::db::kAuthMethod, {"Value"});

    std::string message = "The option '" + twisted +
                          "' is already defined as "
                          "'Value'.";

    MY_EXPECT_THROW(std::invalid_argument, message.c_str(),
                    data.set(twisted, {"Whatever"}));
  };

  combine(mysqlshdk::db::kAuthMethod, "", 0, callback);
}

TEST(Connection_options, invalid_options_after_WL10912) {
  // This callback will get called with every combination of
  // uppercase/lowercase letters for each property
  auto callback = [](const std::string& property, const std::string& twisted) {
    std::string uri = "mysql://root@host?" + twisted + "=whatever";
    std::string message =
        "Invalid URI: Invalid connection option '" + twisted + "'.";

    MY_EXPECT_THROW(std::invalid_argument, message.c_str(),
                    mysqlshdk::db::Connection_options data(uri));
  };

  std::set<std::string> invalid_options = {
      "sslMode",    "sslCa",   "sslCaPath", "sslCrl",     "sslCrlPath",
      "sslCipher", "sslCert", "sslKey",    "authMethod", "sslTlsVersion"};

  for (auto property : invalid_options)
    combine(property, "", 0, callback);
}

}  // namespace testing
