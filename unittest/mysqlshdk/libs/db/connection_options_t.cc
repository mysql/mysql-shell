/* Copyright (c) 2017, 2024, Oracle and/or its affiliates.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License, version 2.0,
 as published by the Free Software Foundation.

 This program is designed to work with certain software (including
 but not limited to OpenSSL) that is licensed under separate terms,
 as designated in a particular file or component or in included license
 documentation.  The authors of MySQL hereby grant you an additional
 permission to link the program and your derivative works with the
 separately licensed software that they have either included with
 the program or referenced in the documentation.

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
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "unittest/gtest_clean.h"

#define MY_EXPECT_THROW(e, m, c)         \
  EXPECT_THROW(                          \
      {                                  \
        try {                            \
          c;                             \
        } catch (const e &error) {       \
          EXPECT_STREQ(m, error.what()); \
          throw;                         \
        }                                \
      },                                 \
      e)

using mysqlshdk::db::Connection_options;
using mysqlshdk::db::Transport_type;
using mysqlshdk::db::uri_connection_attributes;
using mysqlshdk::utils::nullable_options::Comparison_mode;
namespace testing {

namespace {

void EXPECT_TRANSPORT_TYPE(const mysqlshdk::db::Connection_options &d) {
  SCOPED_TRACE(d.as_uri(mysqlshdk::db::uri::formats::full()));

  ASSERT_TRUE(d.has_transport_type());

#ifdef _WIN32
  // on Windows, pipe is only used when host is not set, or is set to '.'
  EXPECT_EQ(Transport_type::Tcp, d.get_transport_type());
#else
  EXPECT_EQ(mysqlshdk::db::k_default_local_transport_type,
            d.get_transport_type());
#endif
}

}  // namespace

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
  ASSERT_FALSE(options.has_compression());
  ASSERT_FALSE(options.has_compression_algorithms());
  ASSERT_FALSE(options.has_compression_level());

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
  options.set_scheme("mysqlx");
  EXPECT_TRUE(options.has_scheme());
  EXPECT_STREQ("mysqlx", options.get_scheme().c_str());
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
  options.set_user("value1");
  EXPECT_TRUE(options.has_user());
  EXPECT_STREQ("value1", options.get_user().c_str());
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
  options.set_password("value1");
  EXPECT_TRUE(options.has_password());
  EXPECT_STREQ("value1", options.get_password().c_str());
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
  options.set_host("value1");
  EXPECT_TRUE(options.has_host());
  EXPECT_STREQ("value1", options.get_host().c_str());
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
  options.set_port(3307);
  EXPECT_TRUE(options.has_port());
  EXPECT_EQ(3307, options.get_port());
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
  options.set_schema("value1");
  EXPECT_TRUE(options.has_schema());
  EXPECT_STREQ("value1", options.get_schema().c_str());
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
  options.set_socket("value1");
  EXPECT_TRUE(options.has_socket());
  EXPECT_STREQ("value1", options.get_socket().c_str());
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
  options.set_pipe("value1");
  EXPECT_TRUE(options.has_pipe());
  EXPECT_STREQ("value1", options.get_pipe().c_str());
  EXPECT_NO_THROW(options.clear_pipe());
  EXPECT_FALSE(options.has_pipe());
  msg = "The connection option '";
  msg.append(mysqlshdk::db::kPipe).append("' has no value.");
  MY_EXPECT_THROW(std::invalid_argument, msg.c_str(), options.get_pipe());
}

TEST(Connection_options, uri_constructor) {
  const std::string hosts[][2] = {{"localhost", "localhost"},
                                  {"[::1]", "::1"},
                                  {"[fe80::850a:5a7c:6ab7:aec4%25enp0s3]",
                                   "fe80::850a:5a7c:6ab7:aec4%enp0s3"}};

  for (const auto &test : hosts) {
    std::string uri = "mysqlx://user:password@" + test[0] +
                      ":3300/"
                      "myschema?ssl-mode=REQUIRED";

    SCOPED_TRACE(uri);

    mysqlshdk::db::Connection_options data(uri);

    ASSERT_TRUE(data.has_scheme());
    ASSERT_STREQ("mysqlx", data.get_scheme().c_str());

    ASSERT_TRUE(data.has_user());
    ASSERT_STREQ("user", data.get_user().c_str());

    ASSERT_TRUE(data.has_password());
    ASSERT_STREQ("password", data.get_password().c_str());

    ASSERT_TRUE(data.has_host());
    ASSERT_EQ(test[1], data.get_host());

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

    ASSERT_EQ(uri, data.as_uri(mysqlshdk::db::uri::formats::full()));
  }
}

void combine(
    const std::string &input,
    std::function<void(const std::string &, const std::string)> callback) {
  std::string my_string(input);

  callback(input, my_string);

  // Turn first character of each word to uppercase
  bool convert = true;
  for (size_t index = 0; index < my_string.size(); index++) {
    if (convert) {
      my_string[index] = std::toupper(my_string[index]);
      convert = false;
      callback(input, my_string);
    } else {
      convert = my_string[index] == '-';
    }
  }

  // Now try with all characters uppercase
  my_string = shcore::str_upper(my_string);
  callback(input, my_string);
}

namespace case_insensitive {
auto callback = [](const std::string &property, const std::string &twisted,
                   const std::string &arg_value) {
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

using get_prop_t =
    std::function<int(const mysqlshdk::db::Connection_options &data)>;

auto callback_int = [](const std::string &property, const std::string &twisted,
                       const std::string &arg_value, get_prop_t get_property) {
  std::string uri = "mysql://root@host?" + twisted + "=" + arg_value;
  mysqlshdk::db::Connection_options data(uri);
  if (!data.has(property)) {
    std::string failed = "Failed Property: " + twisted;
    SCOPED_TRACE(failed);
    ADD_FAILURE();
  } else {
    std::string value = std::to_string(get_property(data));
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

  combine(mysqlshdk::db::kSslMode, callback);
}

TEST(Connection_options, case_insensitive_get_server_public_key) {
  // This callback will get called with every combination of
  // uppercase/lowercase letters on get-server-public-key
  auto callback = std::bind(case_insensitive::callback, std::placeholders::_1,
                            std::placeholders::_2, "true");

  combine(mysqlshdk::db::kGetServerPublicKey, callback);
}

TEST(Connection_options, case_insensitive_server_public_key_path) {
  // This callback will get called with every combination of
  // uppercase/lowercase letters on server-public-key-path
  auto callback = std::bind(case_insensitive::callback, std::placeholders::_1,
                            std::placeholders::_2, "pubkey.pem");

  combine(mysqlshdk::db::kServerPublicKeyPath, callback);
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
  attributes.erase(mysqlshdk::db::kConnectTimeout);
  attributes.erase(mysqlshdk::db::kNetReadTimeout);
  attributes.erase(mysqlshdk::db::kNetWriteTimeout);
  attributes.erase(mysqlshdk::db::kCompression);
  attributes.erase(mysqlshdk::db::kCompressionLevel);
  attributes.erase(mysqlshdk::db::kConnectionAttributes);
#ifdef _WIN32
  attributes.erase(mysqlshdk::db::kKerberosClientAuthMode);
#endif
  attributes.erase(mysqlshdk::db::kOpenIdConnectAuthenticationClientTokenFile);

  for (auto property : attributes) {
    combine(property, callback);
  }
}

void test_int_timeout(const char *timeout_param,
                      case_insensitive::get_prop_t get_property) {
  auto callback_int =
      std::bind(case_insensitive::callback_int, std::placeholders::_1,
                std::placeholders::_2, "1000", get_property);

  combine(timeout_param, callback_int);

  // Test rejection of invalid values

  auto invalid_values = {"-1", "-100", "10.0", "whatever"};

  for (const auto &value : invalid_values) {
    std::string uri("root@host?");
    uri.append(timeout_param);
    uri.append("=");
    uri.append(value);

    std::string msg("Invalid value '");
    msg.append(value);
    msg.append("' for '");
    msg.append(timeout_param);
    msg.append(
        "'. The timeout value must be a positive integer (including "
        "0).");

    std::string uri_msg("Invalid URI: ");
    uri_msg.append(msg);
    MY_EXPECT_THROW(std::invalid_argument, uri_msg.c_str(),
                    { mysqlshdk::db::Connection_options data(uri); });

    mysqlshdk::db::Connection_options sample;
    MY_EXPECT_THROW(std::invalid_argument, msg.c_str(),
                    sample.set(timeout_param, value));
  }

  // Tests acceptance of valid values
  auto valid_values = {"0", "1000", "10000"};

  for (const auto &value : valid_values) {
    std::string uri("root@host?");
    uri.append(timeout_param);
    uri.append("=");
    uri.append(value);

    EXPECT_NO_THROW({ mysqlshdk::db::Connection_options data(uri); });

    mysqlshdk::db::Connection_options sample;
    EXPECT_NO_THROW(sample.set(timeout_param, value));
  }
}

TEST(Connection_options, connect_timeout) {
  // Tests case insensitiveness of the connect-timeout option
  // This callback will get called with every combination of
  // uppercase/lowercase letters for connect-timeout

  test_int_timeout(mysqlshdk::db::kConnectTimeout,
                   &mysqlshdk::db::Connection_options::get_connect_timeout);
}

TEST(Connection_options, net_read_timeout) {
  // Tests case insensitiveness of the net-read-timeout option
  // This callback will get called with every combination of
  // uppercase/lowercase letters for net-read-timeout

  test_int_timeout(mysqlshdk::db::kNetReadTimeout,
                   &mysqlshdk::db::Connection_options::get_net_read_timeout);
}

TEST(Connection_options, net_write_timeout) {
  // Tests case insensitiveness of the net-read-timeout option
  // This callback will get called with every combination of
  // uppercase/lowercase letters for net-read-timeout

  test_int_timeout(mysqlshdk::db::kNetWriteTimeout,
                   &mysqlshdk::db::Connection_options::get_net_write_timeout);
}

TEST(Connection_options, compression) {
  // Tests case insensitiveness of the compression option
  // This callback will get called with every combination of
  // uppercase/lowercase letters for connect-timeout
  auto callback = std::bind(case_insensitive::callback, std::placeholders::_1,
                            std::placeholders::_2, "REQUIRED");

  combine(mysqlshdk::db::kCompression, callback);

  // Test rejection of invalid values
  auto invalid_values = {"-1", "-100", "10.0", "whatever"};

  for (const auto &value : invalid_values) {
    std::string uri("root@host?compression=");
    uri.append(value);

    std::string msg("Invalid value '");
    msg.append(value);
    msg.append(
        "' for 'compression'. Allowed values: 'REQUIRED', 'PREFERRED', "
        "'DISABLED', 'True', 'False', '1', and '0'.");

    std::string uri_msg("Invalid URI: ");
    uri_msg.append(msg);
    MY_EXPECT_THROW(std::invalid_argument, uri_msg.c_str(),
                    { mysqlshdk::db::Connection_options data(uri); });

    mysqlshdk::db::Connection_options sample;
    MY_EXPECT_THROW(std::invalid_argument, msg.c_str(),
                    sample.set(mysqlshdk::db::kCompression, value));
  }

  // Tests acceptance of valid values
  std::vector<std::string> valid_values = {
      "1", "true", "0", "false", "required", "Disabled", "PREFERRED"};

  for (const auto &value : valid_values) {
    std::string uri("root@host?compression=");
    uri.append(value);

    EXPECT_NO_THROW({
      mysqlshdk::db::Connection_options data(uri);
      try {
        if (shcore::lexical_cast<bool>(value))
          EXPECT_EQ("REQUIRED", data.get_compression());
        else
          EXPECT_EQ("DISABLED", data.get_compression());
      } catch (...) {
        EXPECT_EQ(shcore::str_upper(value), data.get_compression());
      }
    });

    mysqlshdk::db::Connection_options sample;
    EXPECT_NO_THROW(sample.set(mysqlshdk::db::kCompression, {value}));
  }
}

TEST(Connection_options, compression_level) {
  auto invalid_values = {"-1e", "-0.5", "10.0", "whatever"};

  for (const auto &value : invalid_values) {
    std::string uri("root@host?compression-level=");
    uri.append(value);

    std::string msg = "The value of 'compression-level' must be an integer.";
    std::string uri_msg("Invalid URI: ");
    uri_msg.append(msg);
    MY_EXPECT_THROW(std::invalid_argument, uri_msg.c_str(),
                    { mysqlshdk::db::Connection_options data(uri); });

    mysqlshdk::db::Connection_options sample;
    MY_EXPECT_THROW(std::invalid_argument, msg.c_str(),
                    sample.set(mysqlshdk::db::kCompressionLevel, value));
  }

  // Tests acceptance of valid values
  std::vector<std::string> valid_values = {"-10", "1", "7", "22", "200"};

  for (const auto &value : valid_values) {
    std::string uri("root@host?compression-level=");
    uri.append(value);

    EXPECT_NO_THROW({
      mysqlshdk::db::Connection_options data(uri);
      EXPECT_EQ(shcore::lexical_cast<int>(value), data.get_compression_level());
    });

    mysqlshdk::db::Connection_options sample;
    EXPECT_NO_THROW(sample.set(mysqlshdk::db::kCompressionLevel, {value}));
  }
}

TEST(Connection_options, set_host) {
  // localhost defaults to socket for mysql, tcp for mysqlx
  // NOTE: mysqlx is default now, but it should be changed to mysql
  {
    mysqlshdk::db::Connection_options data;
    data.set_host("localhost");

    EXPECT_TRUE(data.has_host());
    EXPECT_STREQ("localhost", data.get_host().c_str());
    data.set_default_data();
    EXPECT_TRUE(data.has_transport_type());
    EXPECT_EQ(Transport_type::Tcp, data.get_transport_type());
  }

  {
    mysqlshdk::db::Connection_options data;
    data.set_host("localhost");
    data.set_scheme("mysql");
    data.set_default_data();

    EXPECT_TRANSPORT_TYPE(data);
  }

  // Any other host would set the connection type to tcp
  {
    mysqlshdk::db::Connection_options data;
    data.set_host("127.0.0.1");
    data.set_default_data();
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
    data.set_default_data();
    EXPECT_TRUE(data.has_host());
    EXPECT_STREQ("localhost", data.get_host().c_str());
    EXPECT_EQ(Transport_type::Tcp, data.get_transport_type());
  }

  // Setting host causes no error if a pipe connection is set
  {
    mysqlshdk::db::Connection_options data;
    data.set_pipe("PipeName");

    data.set_host("localhost");
  }
}

TEST(Connection_options, set_socket) {
  {
    mysqlshdk::db::Connection_options data;
#ifdef _WIN32
    data.set_pipe("/Path/To/Socket");
#else
    data.set_socket("/Path/To/Socket");
#endif
    data.set_default_data();
    EXPECT_TRUE(data.has_socket());
    EXPECT_STREQ("/Path/To/Socket", data.get_socket().c_str());
    EXPECT_TRUE(data.has_transport_type());
    EXPECT_EQ(mysqlshdk::db::k_default_local_transport_type,
              data.get_transport_type());
  }

  // Using socket is allowed for localhost
  {
    mysqlshdk::db::Connection_options data;
    data.set_host("localhost");
    EXPECT_NO_THROW(data.set_socket("/Path/To/Socket"));
    data.set_default_data();
    EXPECT_TRUE(data.has_host());
    EXPECT_TRUE(data.has_socket());

    EXPECT_STREQ("localhost", data.get_host().c_str());
    EXPECT_STREQ("/Path/To/Socket", data.get_socket().c_str());
    EXPECT_TRANSPORT_TYPE(data);
  }

  // Using socket is now allowed for localhost if port was defined
  {
    mysqlshdk::db::Connection_options data;
    data.set_host("localhost");
    data.set_port(3307);
    data.set_default_data();
    EXPECT_EQ(Transport_type::Tcp, data.get_transport_type());
  }

  {
    mysqlshdk::db::Connection_options data;
    data.set_host("localhost");
    data.set_port(3307);
    EXPECT_NO_THROW(data.set_socket("/Path/To/Socket"));
    data.set_default_data();
    EXPECT_TRANSPORT_TYPE(data);
  }

  // Setting socket is allowed if host is 127.0.0.1, but socket is only used if
  // host is localhost
  {
    mysqlshdk::db::Connection_options data;
    data.set_host("127.0.0.1");
    EXPECT_NO_THROW(data.set_socket("/Path/To/Socket"));
    data.set_default_data();

    EXPECT_TRUE(data.has_host());
    EXPECT_TRUE(data.has_socket());
    EXPECT_TRUE(data.has_transport_type());
    EXPECT_STREQ("127.0.0.1", data.get_host().c_str());
    EXPECT_EQ(Transport_type::Tcp, data.get_transport_type());
  }
  {
    mysqlshdk::db::Connection_options data;
    data.set_host("127.0.0.1");
    EXPECT_NO_THROW(data.set_socket("/Path/To/Socket"));
    data.set_host("localhost");
    data.set_default_data();

    EXPECT_TRANSPORT_TYPE(data);
  }

  // Using socket is now allowed for 127.0.0.1 if port was defined
  {
    mysqlshdk::db::Connection_options data;
    data.set_host("127.0.0.1");
    data.set_port(3307);

    EXPECT_NO_THROW(data.set_socket("/Path/To/Socket"));
  }

  // NO Error trying to set a socket when a host was previously set
  {
    mysqlshdk::db::Connection_options data;
    data.set_host("192.168.1.110");

    EXPECT_NO_THROW(data.set_socket("/Path/To/Socket"));
  }

  // NO Error trying to set socket when a port was previously set
  {
    mysqlshdk::db::Connection_options data;
    data.set_port(3307);

    EXPECT_NO_THROW(data.set_socket("/Path/To/Socket"));
  }

  // set a socket when a pipe was previously set = override
  {
    mysqlshdk::db::Connection_options data;
    data.set_pipe("PipeName");

    EXPECT_NO_THROW(data.set_socket("/Path/To/Socket"));
  }
}

TEST(Connection_options, case_insensitive_duplicated_attribute) {
  // This callback will get called with every combination of
  // uppercase/lowercase letters on ssl-mode
  auto callback = [](const std::string & /* property */,
                     const std::string &twisted) {
    mysqlshdk::db::Connection_options data;
    data.set(mysqlshdk::db::kAuthMethod, "Value");

    std::string message = "The option '" + twisted +
                          "' is already defined as "
                          "'Value'.";

    MY_EXPECT_THROW(std::invalid_argument, message.c_str(),
                    data.set(twisted, "Whatever"));
  };

  combine(mysqlshdk::db::kAuthMethod, callback);
}

TEST(Connection_options, invalid_options_after_WL10912) {
  // This callback will get called with every combination of
  // uppercase/lowercase letters for each property
  auto callback = [](const std::string & /* property */,
                     const std::string &twisted) {
    std::string uri = "mysql://root@host?" + twisted + "=whatever";
    std::string message =
        "Invalid URI: Invalid connection option '" + twisted + "'.";

    MY_EXPECT_THROW(std::invalid_argument, message.c_str(),
                    mysqlshdk::db::Connection_options data(uri));
  };

  std::set<std::string> invalid_options = {
      "sslMode",   "sslCa",   "sslCaPath", "sslCrl",     "sslCrlPath",
      "sslCipher", "sslCert", "sslKey",    "authMethod", "sslTlsVersion"};

  for (auto property : invalid_options) combine(property, callback);
}

}  // namespace testing
