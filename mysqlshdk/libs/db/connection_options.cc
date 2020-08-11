/*
 * Copyright (c) 2017, 2020, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
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
#include <algorithm>
#include <cassert>
#include "mysqlshdk/libs/db/uri_encoder.h"
#include "mysqlshdk/libs/db/uri_parser.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_string.h"

using mysqlshdk::db::uri::Uri_encoder;
using mysqlshdk::utils::nullable;
using mysqlshdk::utils::nullable_options::Comparison_mode;
using mysqlshdk::utils::nullable_options::Set_mode;

namespace mysqlshdk {
namespace db {

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

namespace {
static constexpr const char *fixed_str_list[] = {kHost,   kSocket, kScheme,
                                                 kSchema, kUser,   kPassword};
}

Connection_options::Connection_options(Comparison_mode mode)
    : m_mode(mode),
      m_options(m_mode, "connection"),
      m_ssl_options(m_mode),
      m_extra_options(m_mode),
      m_enable_connection_attributes(true),
      m_connection_attributes(m_mode) {
  for (auto o : fixed_str_list) m_options.set(o, nullptr, Set_mode::CREATE);
}

Connection_options::Connection_options(const std::string &uri,
                                       Comparison_mode mode)
    : Connection_options(mode) {
  try {
    uri::Uri_parser parser;
    *this = parser.parse(uri, m_options.get_mode());
  } catch (const std::invalid_argument &error) {
    std::string msg = "Invalid URI: ";
    msg.append(error.what());
    throw std::invalid_argument(msg);
  }
}

void Connection_options::set_login_options_from(
    const Connection_options &options) {
  clear_user();
  if (options.has_user()) {
    set_user(options.get_user());
  }
  clear_password();
  if (options.has_password()) {
    set_password(options.get_password());
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

bool Connection_options::has_data() const {
  for (auto o : fixed_str_list) {
    if (has_value(o)) return true;
  }
  return false;
}

void Connection_options::_set_fixed(const std::string &key,
                                    const std::string &val) {
  m_options.set(key, val, Set_mode::UPDATE_NULL);
}

void Connection_options::set_pipe(const std::string &pipe) {
#ifdef _WIN32
  const bool win32 = true;
#else
  const bool win32 = false;
#endif
  if ((!m_transport_type.is_null() && *m_transport_type == Socket) ||
      !m_port.is_null() ||
      (m_options.has_value(kHost) &&
       (get_value(kHost) != "localhost" &&  // only localhost means "use socket"
        !(get_value(kHost) == "." && win32))))
    raise_connection_type_error("pipe connection to '" + pipe + "'");

  if (m_options.has_value(kScheme) && get_value(kScheme) != "mysql") {
    throw std::invalid_argument{"Pipe can only be used with Classic session"};
  }

  m_options.set(kSocket, pipe, Set_mode::UPDATE_NULL);
  m_transport_type = Pipe;
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
  if ((!m_transport_type.is_null() && *m_transport_type == Pipe) ||
      !m_port.is_null() ||
      (has_value(kHost) &&
       get_value(kHost) != "localhost"))  // only localhost means "use socket"
    raise_connection_type_error("socket connection to '" + socket + "'");

  m_options.set(kSocket, socket, Set_mode::UPDATE_NULL);
  m_transport_type = Socket;
}

void Connection_options::raise_connection_type_error(
    const std::string &source) {
  std::string type;
  std::string target;

  if (has_value(kHost) || !m_port.is_null()) {
    if (has_value(kHost)) {
      if (get_value(kHost) != "localhost") type = "tcp ";

      target = "to '" + get_value(kHost);
    }

    if (!m_port.is_null()) {
      if (target.empty())
        target = "to port '";
      else
        target.append(":");

      target.append(std::to_string(*m_port));
      type = "tcp ";
    }

    target.append("'");
  } else if (*m_transport_type == Socket) {
    type = "socket ";
    target = "to '" + get_value(kSocket) + "'";
  } else if (*m_transport_type == Pipe) {
    type = "pipe ";
    target = "to '" + get_value(kSocket) + "'";
  }

  throw std::invalid_argument(
      shcore::str_format("Unable to set a %s, a %s"
                         "connection %s is already defined.",
                         source.c_str(), type.c_str(), target.c_str()));
}

void Connection_options::set_host(const std::string &host) {
  if (!m_transport_type.is_null() && *m_transport_type != Tcp)
    raise_connection_type_error("connection to '" + host + "'");

  if (host != "localhost"
#ifdef _WIN32
      && host != "."
#endif  // _WIN32
  )
    m_transport_type = Tcp;
  else if (m_port.is_null())
    m_transport_type.reset();

  m_options.set(kHost, host, Set_mode::UPDATE_NULL);
}

void Connection_options::set_port(int port) {
  if (m_transport_type.is_null() || m_port.is_null()) {
    m_port = port;
    m_transport_type = Tcp;
  } else {
    raise_connection_type_error(
        "tcp connection to "
        "port '" +
        std::to_string(port) + "'");
  }
}

std::string Connection_options::get_iname(const std::string &name) const {
  // Deprecated options are only supported as input
  // Internally, they are handled with replacements
  const auto it = deprecated_connection_attributes.find(name);
  if (it != deprecated_connection_attributes.end() && !it->second.empty())
    return it->second;
  return name;
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
  const std::string iname = get_iname(name);

  if (name != iname)
    m_warnings.emplace_back("'" + name +
                            "' connection option is deprecated, use '" + iname +
                            "' option instead.");

  if (m_options.has(iname)) {
    m_options.set(iname, value, Set_mode::UPDATE_NULL);
  } else if (m_ssl_options.has(iname)) {
    m_ssl_options.set(iname, value);
  } else if (iname == kCompression) {
    set_compression(value);
  } else if (iname == kCompressionAlgorithms) {
    set_compression_algorithms(value);
  } else if (is_extra_option(iname)) {
    if (iname == kGetServerPublicKey) {
      if (!is_bool_value(value)) {
        throw std::invalid_argument(
            shcore::str_format("Invalid value '%s' for '%s'. Allowed "
                               "values: true, false, 1, 0.",
                               value.c_str(), iname.c_str()));
      }
    } else if (iname == kConnectTimeout) {
      for (auto digit : value) {
        if (!std::isdigit(digit)) {
          throw_invalid_connect_timeout(value);
        }
      }
    }

    m_extra_options.set(iname, value, Set_mode::CREATE);
  } else if (iname == kConnectionAttributes) {
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
  } else if (iname == kCompressionLevel) {
    try {
      set_compression_level(shcore::lexical_cast<int>(value));
    } catch (...) {
      throw std::invalid_argument(std::string("The value of '") +
                                  kCompressionLevel + "' must be an integer.");
    }
  } else {
    throw std::invalid_argument("Invalid connection option '" + name + "'.");
  }
}

void Connection_options::set_unchecked(const std::string &name,
                                       const char *value) {
  m_extra_options.set(name, value);
}

void Connection_options::set(const std::string &name,
                             const std::vector<std::string> &values) {
  std::string iname = get_iname(name);

  if (name == kConnectionAttributes) {
    set_connection_attributes(values);
  } else if (shcore::str_caseeq(name, kSslTlsVersions)) {
    m_ssl_options.set_tls_version(
        shcore::str_join(values.begin(), values.end(), ","));
  } else if (shcore::str_caseeq(name, kSslTlsCiphersuites)) {
    m_ssl_options.set_tls_ciphersuites(
        shcore::str_join(values.begin(), values.end(), ":"));
  } else if (m_options.has(iname) || m_ssl_options.has(iname) ||
             is_extra_option(iname)) {
    // All the connection parameters accept only 1 value
    throw std::invalid_argument("The connection option '" + name +
                                "' requires exactly one value.");
  } else {
    throw std::invalid_argument("Invalid connection option '" + name + "'.");
  }
}

bool Connection_options::has(const std::string &name) const {
  std::string iname = get_iname(name);

  return m_options.has(iname) || m_ssl_options.has(iname) ||
         m_extra_options.has(iname) || m_options.compare(iname, kPort) == 0;
}

bool Connection_options::has_value(const std::string &name) const {
  std::string iname = get_iname(name);

  if (m_options.has(iname))
    return m_options.has_value(iname);
  else if (m_ssl_options.has(iname))
    return m_ssl_options.has_value(iname);
  else if (m_options.compare(iname, kPort) == 0)
    return !m_port.is_null();
  else if (m_extra_options.has(iname))
    return m_extra_options.has_value(iname);

  return false;
}

int Connection_options::get_port() const {
  if (m_port.is_null()) {
    throw std::invalid_argument(
        shcore::str_format("The connection option '%s' has no value.", kPort));
  }

  return *m_port;
}

Transport_type Connection_options::get_transport_type() const {
  if (m_transport_type.is_null())
    throw std::invalid_argument("Transport Type is undefined.");

  return *m_transport_type;
}

int64_t Connection_options::get_compression_level() const {
  if (m_compress_level.is_null())
    throw std::invalid_argument("Compression level is undefined.");
  return *m_compress_level;
}

const std::string &Connection_options::get(const std::string &name) const {
  if (m_options.has(name))
    return get_value(name);
  else if (m_ssl_options.has(name))
    return m_ssl_options.get_value(name);
  else if (m_extra_options.has(name))
    return m_extra_options.get_value(name);
  else
    return get_value(name);  // <-- This will throw the standard message
}

void Connection_options::clear_host() {
  if (!m_transport_type.is_null() && *m_transport_type == Transport_type::Tcp &&
      m_port.is_null())
    m_transport_type.reset();

  clear_value(kHost);
}

void Connection_options::clear_port() {
  if (has_value(kHost) && get_value(kHost) == "localhost")
    m_transport_type.reset();

  m_port.reset();
}

void Connection_options::clear_socket() {
  m_transport_type.reset();
  clear_value(kSocket);
}

void Connection_options::clear_pipe() {
  m_transport_type.reset();
  clear_value(kSocket);
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

std::string Connection_options::as_uri(
    mysqlshdk::db::uri::Tokens_mask format) const {
  Uri_encoder encoder;
  return encoder.encode_uri(*this, format);
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
  if (!has_scheme()) {
    return mysqlsh::SessionType::Auto;
  } else {
    std::string scheme = get_scheme();
    if (scheme == "mysqlx")
      return mysqlsh::SessionType::X;
    else if (scheme == "mysql")
      return mysqlsh::SessionType::Classic;
    else
      throw std::invalid_argument("Unknown MySQL URI type " + scheme);
  }
}

void Connection_options::set_default_connection_data() {
  // Default values
  if (!has_user()) set_user(shcore::get_system_user());

  if (!has_host() &&
      (!has_transport_type() || get_transport_type() == mysqlshdk::db::Tcp))
    set_host("localhost");

#ifdef _WIN32
  if (!has_pipe() && has_host() && get_host() == ".") {
    set_pipe("MySQL");
  }

  if (has_pipe() && !has_scheme()) {
    set_scheme("mysql");
  }
#endif  // _WIN32
}

void Connection_options::throw_invalid_connect_timeout(
    const std::string &value) {
  throw std::invalid_argument(
      shcore::str_format("Invalid value '%s' for '%s'. The "
                         "connection timeout value must be a positive integer "
                         "(including 0).",
                         value.c_str(), kConnectTimeout));
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

}  // namespace db
}  // namespace mysqlshdk
