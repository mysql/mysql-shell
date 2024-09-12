/*
 * Copyright (c) 2017, 2024, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/db/connection_options.h"

#include <mysql.h>

#include <map>

#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/libs/db/uri_encoder.h"
#include "mysqlshdk/libs/db/uri_parser.h"
#include "mysqlshdk/libs/db/utils_connection.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

using mysqlshdk::db::uri::Uri_encoder;
using mysqlshdk::utils::nullable_options::Comparison_mode;
using mysqlshdk::utils::nullable_options::Set_mode;

namespace mysqlshdk {
namespace db {

namespace {
const std::set<std::string> uri_extra_options = {
    kAuthMethod,
    kGetServerPublicKey,
    kServerPublicKeyPath,
    kConnectTimeout,
    kNetReadTimeout,
    kNetWriteTimeout,
    kCompression,
    kCompressionAlgorithms,
    kLocalInfile,
    kNetBufferLength,
    kMaxAllowedPacket,
    kMysqlPluginDir,
#ifdef _WIN32
    kKerberosClientAuthMode,
#endif
    kOciConfigFile,
    kOciAuthenticationClientConfigProfile,
    kClientInteractive,
};

int lexical_cast_timeout(const std::string &name, const std::string &value) {
  try {
    auto result = shcore::lexical_cast<int>(value);
    if (result < 0) throw std::exception();
    return result;
  } catch (...) {
    Connection_options::throw_invalid_timeout(name.c_str(), value);
  }
  throw std::runtime_error("lexical_cast_timeout invalid operation.");
}

}  // namespace

std::string to_string(Transport_type type) {
  switch (type) {
    case Transport_type::Tcp:
      return "TCP/IP";
    case Transport_type::Socket:
      return "Unix socket";
    case Transport_type::Pipe:
      return "Pipe";
  }
  return "Unknown";
}

Connection_options::Connection_options(Comparison_mode mode)
    : IConnection("connection", mode),
      m_ssl_options(m_mode),
      m_ssh_options(m_mode),
      m_extra_options(m_mode),
      m_enable_connection_attributes(true),
      m_connection_attributes(m_mode) {
  for (auto o : {kSocket, kPipe})
    m_options.set(o, nullptr, Set_mode::CREATE_AND_UPDATE);
}

Connection_options::Connection_options(const std::string &uri,
                                       Comparison_mode mode)
    : Connection_options(mode) {
  try {
    uri::Uri_parser parser(mysqlshdk::db::uri::Type::DevApi);
    *this = parser.parse(uri, m_options.get_mode());
  } catch (const std::invalid_argument &error) {
    std::string msg = "Invalid URI: ";
    msg.append(error.what());
    throw std::invalid_argument(msg);
  }
}

void Connection_options::override_with(const Connection_options &options) {
  m_options.override_from(options.m_options, true);
  m_extra_options.override_from(options.m_extra_options, true);
  m_ssl_options.override_from(options.m_ssl_options, true);
  m_connection_attributes.override_from(options.m_connection_attributes, true);
  m_enable_connection_attributes = options.m_enable_connection_attributes;

  if (options.has_compression_level()) {
    clear_compression_level();
    set_compression_level(options.get_compression_level());
  }

  if (options.has_port()) {
    set_port(options.get_port());
  } else {
    // "default" port should also override old value
    clear_port();
  }

  m_default_transport_type = options.m_default_transport_type;
  m_transport_type = options.m_transport_type;

  for (int f = 0; f < k_max_auth_factors; f++) {
    if (options.has_needs_password(f))
      set_needs_password(f, options.needs_password(f));
    if (options.has_mfa_password(f))
      set_mfa_password(f, options.get_mfa_password(f));
  }
}

void Connection_options::set_login_options_from(
    const Connection_options &options) {
  clear_user();
  if (options.has_user()) {
    set_user(options.get_user());
  }
  for (int f = 0; f < k_max_auth_factors; f++) {
    clear_mfa_password(f);
    if (options.has_mfa_password(f))
      set_mfa_password(f, options.get_mfa_password(f));
  }

  m_ssl_options.clear_ca();
  m_ssl_options.clear_capath();
  m_ssl_options.clear_cert();
  m_ssl_options.clear_cipher();
  m_ssl_options.clear_key();
  m_ssl_options.clear_crl();
  m_ssl_options.clear_crlpath();
  m_ssl_options.clear_tls_version();
  m_ssl_options.clear_tls_ciphersuites();
  const Ssl_options &ssl = options.get_ssl_options();
  if (ssl.has_ca()) m_ssl_options.set_ca(ssl.get_ca());
  if (ssl.has_capath()) m_ssl_options.set_capath(ssl.get_capath());
  if (ssl.has_cert()) m_ssl_options.set_cert(ssl.get_cert());
  if (ssl.has_cipher()) m_ssl_options.set_cipher(ssl.get_cipher());
  if (ssl.has_key()) m_ssl_options.set_key(ssl.get_key());
  if (ssl.has_crl()) m_ssl_options.set_crl(ssl.get_crl());
  if (ssl.has_crlpath()) m_ssl_options.set_crlpath(ssl.get_crlpath());
  if (ssl.has_tls_version())
    m_ssl_options.set_tls_version(ssl.get_tls_version());
  if (ssl.has_tls_ciphersuites())
    m_ssl_options.set_tls_ciphersuites(ssl.get_tls_ciphersuites());
}

void Connection_options::set_ssl_options(const Ssl_options &options) {
  m_ssl_options = options;
}

void Connection_options::set_password(const std::string &password) {
  IConnection::set_password(password);
}

void Connection_options::set_mfa_password(int factor,
                                          const std::string &password) {
  switch (factor) {
    case 0:
      set_password(password);
      break;
    case 1:
      m_mfa_password_2 = password;
      break;
    case 2:
      m_mfa_password_3 = password;
      break;
    default:
      assert(!"invalid factor #");
      break;
  }
}

void Connection_options::set_needs_password(int factor, bool flag) {
  assert(factor >= 0 && factor < k_max_auth_factors);

  m_needs_password[factor] = flag;
}

const std::string &Connection_options::get_password() const {
  return get_value(kPassword);
}

const std::string &Connection_options::get_mfa_password(int factor) const {
  assert(has_mfa_password(factor));

  switch (factor) {
    case 0:
      return get_password();
    case 1:
      return *m_mfa_password_2;
    case 2:
      return *m_mfa_password_3;
    default:
      assert(0);
      throw std::logic_error("invalid factor #");
  }
}

bool Connection_options::has_mfa_password(int factor) const {
  switch (factor) {
    case 0:
      return has_password();
    case 1:
      return m_mfa_password_2.has_value();
    case 2:
      return m_mfa_password_3.has_value();
    default:
      assert(0);
      throw std::logic_error("invalid factor #");
  }
}

bool Connection_options::has_needs_password(int factor) const {
  assert(factor >= 0 && factor < k_max_auth_factors);

  return m_needs_password[factor].has_value();
}

bool Connection_options::needs_password(int factor) const {
  assert(has_needs_password(factor));

  return *m_needs_password[factor];
}

bool Connection_options::should_prompt_password(int factor) const {
  return (!has_mfa_password(factor) && has_needs_password(factor) &&
          needs_password(factor)) ||
         // prompt for password1 by default
         (factor == 0 && !has_needs_password(0) && !has_password());
}

void Connection_options::clear_password() { IConnection::clear_password(); }

void Connection_options::clear_mfa_password(int factor) {
  if (!has_mfa_password(factor)) return;

  switch (factor) {
    case 0:
      clear_password();
      break;
    case 1:
      shcore::clear_buffer(*m_mfa_password_2);
      m_mfa_password_2.reset();
      break;
    case 2:
      shcore::clear_buffer(*m_mfa_password_3);
      m_mfa_password_3.reset();
      break;
    default:
      assert(!"invalid factor #");
      break;
  }
}

void Connection_options::clear_needs_password(int factor) {
  assert(factor >= 0 && factor < k_max_auth_factors);

  m_needs_password[factor].reset();
}

void Connection_options::set_ssh_options(
    const mysqlshdk::ssh::Ssh_connection_options &options) {
  m_ssh_options = options;
}

ssh::Ssh_connection_options &Connection_options::get_ssh_options_handle(
    int fallback_port) {
  if (m_ssh_options.has_data()) {
    int mysql_port = fallback_port;
    if (m_port.has_value()) {
      mysql_port = *m_port;
    } else if (has_scheme()) {
      if (get_scheme() == "mysql") {
        mysql_port = k_default_mysql_port;
      } else if (get_scheme() == "mysqlx") {
        mysql_port = k_default_mysql_x_port;
      }
    }

    if (has_host() && mysql_port != 0) {
      m_ssh_options.set_remote_host(get_host());

      m_ssh_options.clear_remote_port();
      m_ssh_options.set_remote_port(mysql_port);
    } else {
      throw std::invalid_argument(
          "Host and port for database are required in advance when using "
          "SSH tunneling functionality");
    }
  }

  return m_ssh_options;
}

bool Connection_options::has_data() const {
  return IConnection::has_data() || has_mfa_password(1) ||
         has_mfa_password(2) || has_port() || has_value(kSocket) ||
         m_ssh_options.has_data() || m_ssl_options.has_data();
}

void Connection_options::_set_fixed(const std::string &key,
                                    const std::string &val) {
  m_options.set(key, val, Set_mode::CREATE_AND_UPDATE);
}

void Connection_options::set_pipe(const std::string &pipe) {
  if (m_options.has_value(kScheme) && get_value(kScheme) != "mysql") {
    throw std::invalid_argument{"Pipe can only be used with Classic session"};
  }

  m_options.set(kSocket, pipe, Set_mode::CREATE_AND_UPDATE);
  m_options.set(kPipe, pipe, Set_mode::CREATE_AND_UPDATE);
  m_default_transport_type = Transport_type::Pipe;
}

void Connection_options::check_compression_conflicts() {
  if (has_compression_algorithms() &&
      get_compression_algorithms() == "uncompressed" && has_compression() &&
      get_compression() == kCompressionRequired)
    throw std::invalid_argument(
        "Conflicting connection options: compression=REQUIRED, "
        "compression-algorithms=uncompressed.");
}

void Connection_options::set_compression(const std::string &compression) {
  if (has_compression())
    throw std::invalid_argument(std::string("Redefinition of '") +
                                kCompression + "' option");

  auto c = shcore::str_upper(shcore::str_strip(compression));
  if (c == "1" || c == "TRUE" || c == kCompressionRequired)
    m_extra_options.set(mysqlshdk::db::kCompression, kCompressionRequired);
  else if (c == "0" || c == "FALSE" || c == kCompressionDisabled)
    m_extra_options.set(mysqlshdk::db::kCompression, kCompressionDisabled);
  else if (c == kCompressionPreferred)
    m_extra_options.set(mysqlshdk::db::kCompression, kCompressionPreferred);
  else
    throw std::invalid_argument(
        "Invalid value '" + compression + "' for '" + kCompression +
        "'. Allowed values: '" + kCompressionRequired + "', '" +
        kCompressionPreferred + "', '" + kCompressionDisabled +
        "', 'True', 'False', '1', and '0'.");

  check_compression_conflicts();
}

void Connection_options::set_compression_algorithms(
    const std::string &compression_algorithms) {
  auto ca = shcore::str_lower(compression_algorithms);
  m_extra_options.set(kCompressionAlgorithms, ca);
  check_compression_conflicts();
}

void Connection_options::set_socket(const std::string &socket) {
  m_options.set(kSocket, socket, Set_mode::CREATE_AND_UPDATE);
  m_default_transport_type = Transport_type::Socket;
}

void Connection_options::set_host(const std::string &host) {
  m_options.set(kHost, host, Set_mode::CREATE_AND_UPDATE);

#ifdef _WIN32
  if (host == ".") {
    // host is set to ".", libmysql will force a pipe connection, we do the same
    m_transport_type = Transport_type::Pipe;
  } else {
    m_transport_type = Transport_type::Tcp;
  }
#else
  if (host == "localhost") {
    // transport type depends on other options
    m_transport_type.reset();
  } else {
    // non-localhost forces TCP
    m_transport_type = Transport_type::Tcp;
  }
#endif  // _WIN32
}

void Connection_options::set_port(int port) {
  m_port = port;
  m_default_transport_type = Transport_type::Tcp;
}

bool Connection_options::is_extra_option(const std::string &option) {
  return std::find_if(uri_extra_options.begin(), uri_extra_options.end(),
                      [this, &option](
                          const decltype(uri_extra_options)::value_type &item) {
                        return m_options.compare(option, item) == 0;
                      }) != uri_extra_options.end();
}

bool Connection_options::is_bool_value(const std::string &value) {
  auto lower_case_value = shcore::str_lower(value);
  return lower_case_value == "true" || lower_case_value == "false" ||
         lower_case_value == "1" || lower_case_value == "0";
}

void Connection_options::set(const std::string &name,
                             const std::string &value) {
  using namespace std::string_literals;
  if (compare(name, db::kScheme) == 0) {
    set_scheme(value);
  } else if (compare(name, db::kUser) == 0) {
    set_user(value);
  } else if (compare(name, db::kPassword) == 0) {
    set_password(value);
  } else if (compare(name, db::kHost) == 0) {
    set_host(value);
  } else if (compare(name, db::kPath) == 0) {
    set_path(value);
  } else if (compare(name, db::kSocket) == 0) {
    set_socket(value);
  } else if (compare(name, db::kPipe) == 0) {
    set_pipe(value);
  } else if (compare(name, db::kSchema) == 0) {
    set_schema(value);
  } else if (m_ssl_options.has(name)) {
    m_ssl_options.set(name, value);
  } else if (name == kCompression) {
    set_compression(value);
  } else if (name == kCompressionAlgorithms) {
    set_compression_algorithms(value);
  } else if (compare(name, db::kConnectTimeout) == 0) {
    set_connect_timeout(lexical_cast_timeout(db::kConnectTimeout, value));
  } else if (compare(name, db::kNetReadTimeout) == 0) {
    set_net_read_timeout(lexical_cast_timeout(db::kNetReadTimeout, value));
  } else if (compare(name, db::kNetWriteTimeout) == 0) {
    set_net_write_timeout(lexical_cast_timeout(db::kNetWriteTimeout, value));
  }
#ifdef _WIN32
  else if (name == kKerberosClientAuthMode) {
    set_kerberos_auth_mode(value);
  }
#endif
  else if (is_extra_option(name)) {
    if (name == kGetServerPublicKey || name == kClientInteractive) {
      if (!is_bool_value(value)) {
        throw std::invalid_argument(
            shcore::str_format("Invalid value '%s' for '%s'. Allowed "
                               "values: true, false, 1, 0.",
                               value.c_str(), name.c_str()));
      }
    }

    m_extra_options.set(name, value, Set_mode::CREATE);
  } else if (name == kConnectionAttributes) {
    // When connection-attributes has assigned a single value, it must
    // be a boolean. We do NOT do anything with it: in DevAPI standard
    // it should avoid ANY connection attribute to be sent, however
    // on the shell this behavior is not present, see WL12446 NOTES on FR10
    // for more details
    if (!value.empty() && !is_bool_value(value)) {
      throw std::invalid_argument(
          "The value of 'connection-attributes' must be either a boolean or a "
          "list of key-value pairs.");
    } else {
      auto lower_case_value = shcore::str_lower(value);
      if (lower_case_value == "0" || lower_case_value == "false") {
        m_enable_connection_attributes = false;
      }
    }
  } else if (name == kCompressionLevel) {
    try {
      set_compression_level(shcore::lexical_cast<int>(value));
    } catch (...) {
      throw std::invalid_argument("The value of '"s + kCompressionLevel +
                                  "' must be an integer."s);
    }
  } else {
    throw std::invalid_argument("Invalid connection option '" + name + "'.");
  }
}

void Connection_options::set(const std::string &name, int value) {
  if (name == db::kPort) {
    set_port(value);
  } else if (name == db::kCompressionLevel) {
    set_compression_level(value);
  } else if (name == kConnectTimeout) {
    if (value < 0)
      throw_invalid_timeout(kConnectTimeout, std::to_string(value));
    set_connect_timeout(value);
  } else if (name == kNetReadTimeout) {
    if (value < 0)
      throw_invalid_timeout(kNetReadTimeout, std::to_string(value));
    set_net_read_timeout(value);
  } else if (name == kNetWriteTimeout) {
    if (value < 0)
      throw_invalid_timeout(kNetWriteTimeout, std::to_string(value));
    set_net_write_timeout(value);
  } else {
    throw std::invalid_argument("Invalid connection option '" + name + "'.");
  }
}

void Connection_options::set_unchecked(const std::string &name,
                                       const char *value) {
  if (name == kConnectTimeout) {
    set_connect_timeout(shcore::lexical_cast<int>(value));
  } else if (name == kNetReadTimeout) {
    set_net_read_timeout(shcore::lexical_cast<int>(value));
  } else if (name == kNetWriteTimeout) {
    set_net_write_timeout(shcore::lexical_cast<int>(value));
  } else {
    m_extra_options.set(name, value);
  }
}

void Connection_options::set(const std::string &name,
                             const std::vector<std::string> &values) {
  if (name == kConnectionAttributes) {
    set_connection_attributes(values);
  } else if (shcore::str_caseeq(name, kSslTlsVersions)) {
    m_ssl_options.set_tls_version(
        shcore::str_join(values.begin(), values.end(), ","));
  } else if (shcore::str_caseeq(name, kSslTlsCiphersuites)) {
    m_ssl_options.set_tls_ciphersuites(
        shcore::str_join(values.begin(), values.end(), ":"));
  } else if (m_options.has(name) || m_ssl_options.has(name) ||
             is_extra_option(name)) {
    // All the connection parameters accept only 1 value
    throw std::invalid_argument("The connection option '" + name +
                                "' requires exactly one value.");
  } else {
    throw std::invalid_argument("Invalid connection option '" + name + "'.");
  }
}

bool Connection_options::has(const std::string &name) const {
  if (name == db::kCompressionLevel) return has_compression_level();
  if (name == db::kConnectTimeout) return has_connect_timeout();
  if (name == db::kNetReadTimeout) return has_net_read_timeout();
  if (name == db::kNetWriteTimeout) return has_net_write_timeout();

  return m_options.has(name) || m_ssl_options.has(name) ||
         m_extra_options.has(name) || m_options.compare(name, kPort) == 0;
}

bool Connection_options::has_value(const std::string &name) const {
  if (m_options.compare(name, kSocket) == 0)
    return uses_local_transport() && has_socket();
  else if (m_options.compare(name, kPipe) == 0)
    return uses_local_transport() && has_pipe();
  else if (m_options.compare(name, kConnectTimeout) == 0)
    return has_connect_timeout();
  else if (m_options.compare(name, kNetReadTimeout) == 0)
    return has_net_read_timeout();
  else if (m_options.compare(name, kNetWriteTimeout) == 0)
    return has_net_write_timeout();
  else if (m_options.has(name))
    return m_options.has_value(name);
  else if (m_ssl_options.has(name))
    return m_ssl_options.has_value(name);
  else if (m_options.compare(name, kPort) == 0)
    return m_port.has_value();
  else if (m_extra_options.has(name))
    return m_extra_options.has_value(name);

  return false;
}

std::vector<std::pair<std::string, std::optional<std::string>>>
Connection_options::query_attributes() const {
  std::vector<std::pair<std::string, std::optional<std::string>>> attributes;

  // From SSL options, only the options with value are considered
  for (const auto &ssl_option : m_ssl_options) {
    if (ssl_option.second.has_value()) {
      attributes.push_back(ssl_option);
    }
  }

  // From the extra options, only the predefined list of attributes is
  // considered, with or without value
  for (const auto &option : m_extra_options) {
    if (uri_connection_attributes.find(option.first) !=
            uri_connection_attributes.end() ||
        uri_extra_options.find(option.first) != uri_extra_options.end()) {
      attributes.push_back(option);
    }
  }

  if (has_connect_timeout()) {
    attributes.push_back(
        {kConnectTimeout, std::to_string(get_connect_timeout())});
  }
  if (has_net_read_timeout()) {
    attributes.push_back(
        {kNetReadTimeout, std::to_string(get_net_read_timeout())});
  }
  if (has_net_write_timeout()) {
    attributes.push_back(
        {kNetWriteTimeout, std::to_string(get_net_write_timeout())});
  }
  if (has_compression_level()) {
    attributes.push_back(
        {kCompressionLevel, std::to_string(get_compression_level())});
  }

  return attributes;
}

bool Connection_options::has_ssh_options() const {
  return m_ssh_options.has_data();
}

int Connection_options::get_port() const {
  if (!m_port.has_value()) {
    throw std::invalid_argument(
        shcore::str_format("The connection option '%s' has no value.", kPort));
  }

  return *m_port;
}

/**
 * This function is used to determine the port to be used to create the MySQL
 * connection, i.e if through SSH tunnels returns the tunnel local port,
 * otherwise the configured port in the connection data.
 */
int Connection_options::get_target_port() const {
  int port = 0;
  if (m_ssh_options.has_data()) {
    port = m_ssh_options.get_local_port();
  } else if (has_port()) {
    port = get_port();
  }
  return port;
}

void Connection_options::set_transport_type(Transport_type type) {
  m_transport_type = type;
}

Transport_type Connection_options::get_transport_type() const {
  if (!m_transport_type.has_value())
    throw std::invalid_argument("Transport Type is undefined.");

  return *m_transport_type;
}

int64_t Connection_options::get_compression_level() const {
  if (!m_compress_level.has_value())
    throw std::invalid_argument("Compression level is undefined.");
  return *m_compress_level;
}

const std::string &Connection_options::get(const std::string &name) const {
  if (m_ssl_options.has(name)) return m_ssl_options.get_value(name);
  if (m_extra_options.has(name)) return m_extra_options.get_value(name);
  if (m_options.compare(name, kSocket) == 0) return get_socket();
  if (m_options.compare(name, kPipe) == 0) return get_pipe();

  return get_value(name);
}

int Connection_options::get_numeric(const std::string &name) const {
  if (name == db::kPort) return get_port();
  if (name == db::kCompressionLevel) return get_compression_level();
  if (name == db::kConnectTimeout) return get_connect_timeout();
  if (name == db::kNetReadTimeout) return get_net_read_timeout();
  if (name == db::kNetWriteTimeout) return get_net_write_timeout();

  throw std::invalid_argument(
      shcore::str_format("Invalid URI property: %s", name.c_str()));
}

void Connection_options::clear_transport_type() { m_transport_type.reset(); }

void Connection_options::clear_host() { clear_value(kHost); }

void Connection_options::clear_port() { m_port.reset(); }

void Connection_options::clear_socket() { clear_value(kSocket); }

void Connection_options::clear_pipe() {
  clear_value(kSocket);
  clear_value(kPipe);
}

void Connection_options::remove(const std::string &name) {
  // Can't delete fixed values
  if (m_options.has(name))
    throw std::invalid_argument(
        "Unable to delete Connection option, use "
        "clear instead!!");

  if (m_ssl_options.has(name))
    throw std::invalid_argument(
        "Unable to delete SSL Connection option, use "
        "clear instead!!");

  m_extra_options.remove(name);
}

bool Connection_options::operator==(const Connection_options &other) const {
  return m_options == other.m_options && m_ssl_options == other.m_ssl_options &&
         m_extra_options == other.m_extra_options &&
         m_enable_connection_attributes ==
             other.m_enable_connection_attributes &&
         m_connection_attributes == other.m_connection_attributes &&
         m_port == other.m_port && m_transport_type == other.m_transport_type;
}

bool Connection_options::operator!=(const Connection_options &other) const {
  return !(*this == other);
}

mysqlsh::SessionType Connection_options::get_session_type() const {
  if (!has_scheme()) return mysqlsh::SessionType::Auto;

  const auto &scheme = get_scheme();
  if (scheme == "mysqlx") return mysqlsh::SessionType::X;
  if (scheme == "mysql") return mysqlsh::SessionType::Classic;

  throw std::invalid_argument("Unknown MySQL URI type " + scheme);
}

void Connection_options::set_default_data() {
  // Default values
  if (!has_user() && !is_auth_method(mysqlshdk::db::kAuthMethodKerberos)) {
    // The system user is not used with kerberos to trigger the client lib
    // functionality of using cashed TGT
    set_user(shcore::get_system_user());
  }

  const auto local_transport = uses_local_transport();

  if (!has_host() && !local_transport) {
    set_host("localhost");
  }

  if (!has_transport_type()) {
    m_transport_type = local_transport ? k_default_local_transport_type
                                       : mysqlshdk::db::Transport_type::Tcp;
  }

#ifdef _WIN32
  if (mysqlshdk::db::Transport_type::Pipe == m_transport_type) {
    if (!has_pipe()) {
      set_pipe(mysql_unix_port);
    }

    if (!has_scheme()) {
      set_scheme("mysql");
    }
  }
#endif  // _WIN32

  if (m_ssh_options.has_data()) {
    m_ssh_options.set_default_data();
  }
}

void Connection_options::throw_invalid_timeout(const char *name,
                                               const std::string &value) {
  throw std::invalid_argument(
      shcore::str_format("Invalid value '%s' for '%s'. The "
                         "timeout value must be a positive integer "
                         "(including 0).",
                         value.c_str(), name));
}

bool Connection_options::is_auth_method(const std::string &method_id) const {
  bool ret_val = false;

  if (has_value(mysqlshdk::db::kAuthMethod)) {
    auto auth_method = get(mysqlshdk::db::kAuthMethod);
    if (method_id == mysqlshdk::db::kAuthMethodKerberos) {
      ret_val = auth_method == mysqlshdk::db::kAuthMethodKerberos ||
                auth_method == mysqlshdk::db::kAuthMethodLdapSasl;
    } else {
      ret_val = auth_method == method_id;
    }
  }

  return ret_val;
}

bool Connection_options::has_oci_config_file() const {
  return has_value(mysqlshdk::db::kOciConfigFile);
}

void Connection_options::set_oci_config_file(const std::string &path) {
  set(mysqlshdk::db::kOciConfigFile, path);
}

std::string Connection_options::get_oci_config_file() const {
  return m_extra_options.get_value(mysqlshdk::db::kOciConfigFile);
}

bool Connection_options::has_oci_client_config_profile() const {
  return has_value(mysqlshdk::db::kOciAuthenticationClientConfigProfile);
}

void Connection_options::set_oci_client_config_profile(
    const std::string &path) {
  set(mysqlshdk::db::kOciAuthenticationClientConfigProfile, path);
}

std::string Connection_options::get_oci_client_config_profile() const {
  return m_extra_options.get_value(
      mysqlshdk::db::kOciAuthenticationClientConfigProfile);
}

#ifdef _WIN32
bool Connection_options::has_kerberos_auth_mode() const {
  return has_value(mysqlshdk::db::kKerberosClientAuthMode);
}

void Connection_options::set_kerberos_auth_mode(const std::string &mode) {
  std::string uppermode = shcore::str_upper(mode);
  if (uppermode != mysqlshdk::db::kKerberosAuthModeSSPI &&
      uppermode != mysqlshdk::db::kKerberosAuthModeGSSAPI) {
    throw std::invalid_argument(shcore::str_format(
        "Invalid value: %s. Allowed values: SSPI, GSSAPI.", mode.c_str()));
  }

  m_extra_options.set(mysqlshdk::db::kKerberosClientAuthMode, uppermode,
                      Set_mode::CREATE_AND_UPDATE);
}

std::string Connection_options::get_kerberos_auth_mode() const {
  if (has_kerberos_auth_mode()) {
    auto result =
        m_extra_options.get_value(mysqlshdk::db::kKerberosClientAuthMode);
    if (result == mysqlshdk::db::kKerberosAuthModeSSPI ||
        result == mysqlshdk::db::kKerberosAuthModeGSSAPI)
      return result;
  }
  return mysqlshdk::db::kKerberosAuthModeSSPI;
}
#endif

bool Connection_options::has_connect_timeout() const {
  return m_connect_timeout.has_value();
}

void Connection_options::set_connect_timeout(int value) {
  m_connect_timeout = value;
}

int Connection_options::get_connect_timeout() const {
  return m_connect_timeout.value();
}

void Connection_options::clear_connect_timeout() { m_connect_timeout.reset(); }

bool Connection_options::has_net_read_timeout() const {
  return m_net_read_timeout.has_value();
}

void Connection_options::set_net_read_timeout(int value) {
  m_net_read_timeout = value;
}

int Connection_options::get_net_read_timeout() const {
  return m_net_read_timeout.value();
}

void Connection_options::clear_net_read_timeout() {
  m_net_read_timeout.reset();
}

bool Connection_options::has_net_write_timeout() const {
  return m_net_write_timeout.has_value();
}

void Connection_options::set_net_write_timeout(int value) {
  m_net_write_timeout = value;
}

int Connection_options::get_net_write_timeout() const {
  return m_net_write_timeout.value();
}

void Connection_options::clear_net_write_timeout() {
  m_net_write_timeout.reset();
}

bool Connection_options::is_enabled(const std::string &option) const {
  if (has(option)) {
    auto value = shcore::str_upper(m_extra_options.get_value(option));
    return value == "1" || value == "TRUE";
  }

  return false;
}

void Connection_options::set_interactive(bool value) {
  m_interactive = value;
  if (!m_interactive && has(kClientInteractive)) {
    remove(kClientInteractive);
  }
}

bool Connection_options::is_interactive() const {
  return m_interactive || is_enabled(kClientInteractive);
}

void Connection_options::set_connection_attribute(const std::string &attribute,
                                                  const std::string &value) {
  std::string error;

  if (attribute.empty()) {
    error =
        "Invalid attribute on 'connection-attributes', empty name is not "
        "allowed.";
  } else if (attribute[0] == '_') {
    error = "Key names in 'connection-attributes' cannot start with '_'";
  } else if (m_connection_attributes.has(attribute)) {
    error =
        "Duplicate key '" + attribute + "' used in 'connection-attributes'.";
  }

  if (error.empty())
    m_connection_attributes.set(attribute, value);
  else
    throw std::invalid_argument(error);
}

void Connection_options::set_connection_attributes(
    const std::vector<std::string> &attributes) {
  // Enables the connection attributes
  m_enable_connection_attributes = true;

  for (const auto &pair : attributes) {
    if (!pair.empty()) {
      auto tokens = shcore::split_string(pair, "=");
      size_t len = tokens.size();

      std::string attribute, value;
      switch (len) {
        case 2:
          attribute = tokens[0];
          value = tokens[1];
          break;
        case 1:
          attribute = tokens[0];
          break;
        default:
          throw std::invalid_argument(
              "The value of 'connection-attributes' must be "
              "either a boolean or a list of key=value pairs.");
      }
      set_connection_attribute(attribute, value);
    }
  }
}

bool Connection_options::uses_local_transport() const {
  if (m_transport_type.has_value())
    return *m_transport_type != Transport_type::Tcp;

  if (m_default_transport_type.has_value()) {
    return *m_default_transport_type != Transport_type::Tcp;
  }

#ifdef _WIN32
  // Windows always uses TCP by default
  return false;
#else
  if (!has_scheme() || get_scheme() == "mysqlx") {
    // xproto connections connect via TCP by default
    return false;
  } else {
    // classic connections connect via socket by default
    return true;
  }
#endif
}

int64_t default_connect_timeout() {
  // some tests execute SQL before options are available, in that case use the
  // default value
  const auto options = mysqlsh::current_shell_options(true);
  const auto &storage =
      options ? options->get() : mysqlsh::Shell_options::Storage();
  return storage.connect_timeout * 1000;
}

std::string default_mysql_plugins_dir() {
  // some tests execute SQL before options are available, in that case use the
  // default value

  if (const auto options = mysqlsh::current_shell_options(true)) {
    return options->get().mysql_plugin_dir;
  }

  return "";
}

}  // namespace db
}  // namespace mysqlshdk
