/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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
#include <cassert>
#include <algorithm>
#include "mysqlshdk/libs/db/uri_encoder.h"
#include "mysqlshdk/libs/db/uri_parser.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

using mysqlshdk::utils::nullable;
using mysqlshdk::utils::nullable_options::Set_mode;
using mysqlshdk::utils::nullable_options::Comparison_mode;
using mysqlshdk::utils::Nullable_options;
using mysqlshdk::db::uri::Uri_encoder;

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
static constexpr const char *fixed_str_list[] = {
    kHost, kSocket, kScheme, kSchema, kUser, kPassword};
}

Connection_options::Connection_options(Comparison_mode mode)
    : Nullable_options(mode, "connection"),
      _ssl_options(_mode),
      _extra_options(_mode) {
  for (auto o : fixed_str_list)
    Nullable_options::set(o, nullptr, Set_mode::CREATE);
}

Connection_options::Connection_options(const std::string& uri,
                                       Comparison_mode mode)
    : Nullable_options(mode, "connection"),
      _ssl_options(_mode),
      _extra_options(_mode) {
  for (auto o : fixed_str_list)
    Nullable_options::set(o, nullptr, Set_mode::CREATE);

  try {
    uri::Uri_parser parser;
    *this = parser.parse(uri, _mode);
  } catch (const std::invalid_argument& error) {
    std::string msg = "Invalid URI: ";
    msg.append(error.what());
    throw std::invalid_argument(msg);
  }
}

void Connection_options::set_login_options_from(
    const Connection_options& options) {
  clear_user();
  if (options.has_user()) {
    set_user(options.get_user());
  }
  clear_password();
  if (options.has_password()) {
    set_password(options.get_password());
  }

  _ssl_options.clear_cert();
  _ssl_options.clear_key();
  // SSL client certificate options are login options
  const Ssl_options &ssl = options.get_ssl_options();
  if (ssl.has_cert())
    _ssl_options.set_cert(ssl.get_cert());
  if (ssl.has_key())
    _ssl_options.set_key(ssl.get_key());
}

void Connection_options::set_ssl_connection_options_from(
    const Ssl_options& options) {
  Ssl_options orig(_ssl_options);
  // Copy all SSL options
  _ssl_options = options;
  // Restore the client certificate options
  _ssl_options.clear_cert();
  _ssl_options.clear_key();
  // SSL client certificate options are login options
  if (orig.has_cert())
    _ssl_options.set_cert(orig.get_cert());
  if (orig.has_key())
    _ssl_options.set_key(orig.get_key());
}

void Connection_options::set_ssl_options(const Ssl_options &options) {
  _ssl_options = options;
}

bool Connection_options::has_data() const {
  for (auto o : fixed_str_list) {
    if (has_value(o)) return true;
  }
  return false;
}

void Connection_options::_set_fixed(const std::string& key,
                                    const std::string& val) {
  Nullable_options::set(key, val, Set_mode::UPDATE_NULL);
}

void Connection_options::set_pipe(const std::string& pipe) {
#ifdef _WIN32
  const bool win32 = true;
#else
  const bool win32 = false;
#endif
  if ((!_transport_type.is_null() && *_transport_type == Socket) ||
      !_port.is_null() ||
      (Nullable_options::has_value(kHost) &&
       (!shcore::is_local_host(get_value(kHost), false) &&
        !(get_value(kHost) == "." && win32))))
    raise_connection_type_error("pipe connection to '" + pipe + "'");

  Nullable_options::set(kSocket, pipe, Set_mode::UPDATE_NULL);
  _transport_type = Pipe;
}

void Connection_options::set_socket(const std::string& socket) {
  if ((!_transport_type.is_null() && *_transport_type == Pipe) ||
      !_port.is_null() ||
      (Nullable_options::has_value(kHost) &&
       !shcore::is_local_host(get_value(kHost), false)))
    raise_connection_type_error("socket connection to '" + socket + "'");

  Nullable_options::set(kSocket, socket, Set_mode::UPDATE_NULL);
  _transport_type = Socket;
}

void Connection_options::raise_connection_type_error(
    const std::string& source) {
  std::string type;
  std::string target;

  if (Nullable_options::has_value(kHost) || !_port.is_null()) {
    if (Nullable_options::has_value(kHost)) {
      if (get_value(kHost) != "localhost")
        type = "tcp ";

      target = "to '" + get_value(kHost);
    }

    if (!_port.is_null()) {
      if (target.empty())
        target = "to port '";
      else
        target.append(":");

      target.append(std::to_string(*_port));
      type = "tcp ";
    }

    target.append("'");
  } else if (*_transport_type == Socket) {
    type = "socket ";
    target = "to '" + get_value(kSocket) + "'";
  } else if (*_transport_type == Pipe) {
    type = "pipe ";
    target = "to '" + get_value(kSocket) + "'";
  }

  throw std::invalid_argument(
      shcore::str_format("Unable to set a %s, a %s"
                         "connection %s is already defined.",
                         source.c_str(), type.c_str(), target.c_str()));
}

void Connection_options::set_host(const std::string& host) {
  if (!_transport_type.is_null() && *_transport_type != Tcp)
    raise_connection_type_error("connection to '" + host + "'");

  if (host != "localhost")
    _transport_type = Tcp;
  else if (_port.is_null())
    _transport_type.reset();

  Nullable_options::set(kHost, host, Set_mode::UPDATE_NULL);
}

void Connection_options::set_port(int port) {
  if (_transport_type.is_null() || _port.is_null()) {
    _port = port;
    _transport_type = Tcp;
  } else {
    raise_connection_type_error(
        "tcp connection to "
        "port '" +
        std::to_string(port) + "'");
  }
}

std::string Connection_options::get_iname(const std::string& name) const {
  // DbUser and DbPassword are only supported as input
  // Internally, they are handled as User and Password
  std::string iname(name);
  if (compare(name, kDbUser) == 0)
    return kUser;
  else if (compare(name, kDbPassword) == 0)
    return kPassword;

  return name;
}

void Connection_options::set(const std::string& name,
                             const std::vector<std::string>& values) {
  std::string iname = get_iname(name);

  auto is_extra_option = [this, &name]() -> bool {
    return std::find_if(uri_extra_options.begin(), uri_extra_options.end(),
                        [this, &name](const decltype(
                            uri_extra_options)::value_type& item) {
                          return compare(name, item) == 0;
                        }) != uri_extra_options.end();
  };

  if (Nullable_options::has(iname) || _ssl_options.has(iname) ||
      is_extra_option()) {
    // All the connection parameters accept only 1 value
    if (values.size() != 1) {
      throw std::invalid_argument("The connection option '" + name +
                                  "'"
                                  " requires exactly one value.");
    }

    if (Nullable_options::has(iname)) {
      Nullable_options::set(iname, values[0], Set_mode::UPDATE_NULL);
    } else if (_ssl_options.has(iname)) {
      _ssl_options.set(iname, values[0]);
    } else {
      // TODO(rennox) at the moment, the only extra option supported are those
      // listed in uri_extra_options set, in one hand we have issues claiming
      // errors should be raised if not valid options are given, and on the
      // other side the URI specification does not explicitly forbid other
      // values. This conflict needs to be resolved at the DevAPI Court
      if (name == kGetServerPublicKey) {
        auto lower_case_value = shcore::str_lower(values[0]);
        if (!(lower_case_value == "true" || lower_case_value == "false" ||
              lower_case_value == "1" || lower_case_value == "0")) {
          throw std::invalid_argument(
              shcore::str_format("Invalid value '%s' for '%s'. Allowed "
                                 "values: true, false, 1, 0.",
                                 values[0].c_str(), name.c_str()));
        }
      }
      _extra_options.set(iname, values[0], Set_mode::CREATE);
    }

  } else {
    throw_invalid_option(name);
  }
}

bool Connection_options::has(const std::string& name) const {
  std::string iname = get_iname(name);

  return Nullable_options::has(iname) || _ssl_options.has(iname) ||
         _extra_options.has(iname) || compare(iname, kPort) == 0;
}

bool Connection_options::has_value(const std::string& name) const {
  std::string iname = get_iname(name);

  if (Nullable_options::has(iname))
    return Nullable_options::has_value(iname);
  else if (_ssl_options.has(iname))
    return _ssl_options.has_value(iname);
  else if (compare(iname, kPort) == 0)
    return !_port.is_null();
  else
    return _extra_options.has_value(iname);
}

int Connection_options::get_port() const {
  if (_port.is_null())
    throw_no_value(kPort);

  return *_port;
}

Transport_type Connection_options::get_transport_type() const {
  if (_transport_type.is_null())
    throw std::invalid_argument("Transport Type is undefined.");

  return *_transport_type;
}

const std::string& Connection_options::get(const std::string& name) const {
  if (Nullable_options::has(name))
    return get_value(name);
  else if (_ssl_options.has(name))
    return _ssl_options.get_value(name);
  else if (_extra_options.has(name))
    return _extra_options.get_value(name);
  else
    return get_value(name);  // <-- This will throw the standard message
}

void Connection_options::clear_host() {
  if (!_transport_type.is_null() && *_transport_type == Transport_type::Tcp &&
      _port.is_null())
    _transport_type.reset();

  clear_value(kHost);
}

void Connection_options::clear_port() {
  if (Nullable_options::has_value(kHost) && get_value(kHost) == "localhost")
    _transport_type.reset();

  _port.reset();
}

void Connection_options::clear_socket() {
  _transport_type.reset();
  clear_value(kSocket);
}

void Connection_options::clear_pipe() {
  _transport_type.reset();
  clear_value(kSocket);
}

void Connection_options::remove(const std::string& name) {
  // Can't delete fixed values
  if (Nullable_options::has(name))
    throw std::invalid_argument(
        "Unable to delete Connection option, use "
        "clear instead!!");

  if (_ssl_options.has(name))
    throw std::invalid_argument(
        "Unable to delete SSL Connection option, use "
        "clear instead!!");

  _extra_options.remove(name);
}

std::string Connection_options::as_uri(
    mysqlshdk::db::uri::Tokens_mask format) const {
  Uri_encoder encoder;
  return encoder.encode_uri(*this, format);
}

bool Connection_options::operator==(const Connection_options& other) const {
  return Nullable_options::operator==(other) &&
         _ssl_options == other._ssl_options &&
         _extra_options == other._extra_options;
}

bool Connection_options::operator!=(const Connection_options& other) const {
  return !(*this == other);
}

}  // namespace db
}  // namespace mysqlshdk
