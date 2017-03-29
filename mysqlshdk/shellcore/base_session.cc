/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "shellcore/base_session.h"

#include "scripting/object_factory.h"
#include "shellcore/shell_core.h"
#include "scripting/lang_base.h"
#include "scripting/common.h"

#include "scripting/proxy_object.h"

#include "utils/utils_general.h"
#include "utils/utils_file.h"

#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

#define MAX_COLUMN_LENGTH 1024
#define MIN_COLUMN_LENGTH 4

using namespace mysqlsh;
using namespace shcore;




ShellBaseSession::ShellBaseSession() :
_port(0), _tx_deep(0) {
}

ShellBaseSession::ShellBaseSession(const ShellBaseSession& s) :
_user(s._user), _password(s._password), _host(s._host), _port(s._port), _sock(s._sock), _schema(s._schema),
_ssl_info(s._ssl_info), _tx_deep(s._tx_deep) {
}

std::string &ShellBaseSession::append_descr(std::string &s_out, int UNUSED(indent), int UNUSED(quote_strings)) const {
  if (!is_open())
    s_out.append("<" + class_name() + ":disconnected>");
  else
    s_out.append("<" + class_name() + ":" + _uri + ">");
  return s_out;
}

std::string &ShellBaseSession::append_repr(std::string &s_out) const {
  return append_descr(s_out, false);
}

void ShellBaseSession::append_json(shcore::JSON_dumper& dumper) const {
  dumper.start_object();

  dumper.append_string("class", class_name());
  dumper.append_bool("connected", is_open());

  if (is_open())
    dumper.append_string("uri", _uri);

  dumper.end_object();
}

void ShellBaseSession::load_connection_data(const shcore::Argument_list &args) {
  // The connection data can come from different sources
  std::string uri;
  std::string auth_method;
  std::string connections_file; // The default connection file or the indicated on the map as dataSourceFile
  shcore::Value::Map_type_ref options; // Map with the connection data

  //-----------------------------------------------------
  // STEP 1: Identifies the source of the connection data
  //-----------------------------------------------------
  if (args[0].type == String) {
    std::string temp = args.string_at(0);
    uri = temp;
  }

  // Connection data comes in a dictionary
  else if (args[0].type == Map) {
    options = args.map_at(0);

    // Use a custom stored sessions file, rather than the default one
    if (options->has_key("dataSourceFile"))
      connections_file = (*options)["dataSourceFile"].as_string();
  } else
    throw shcore::Exception::argument_error("Unexpected argument on connection data.");

  //-------------------------------------------------------------------------
  // STEP 2: Gets the individual connection parameters whatever the source is
  //-------------------------------------------------------------------------
  // Handles the case where an URI was received
  //struct shcore::SslInfo ssl_info;
  if (!uri.empty()) {
    std::string protocol;
    int pwd_found;
    parse_mysql_connstring(uri, protocol, _user, _password, _host, _port, _sock, _schema, pwd_found, _ssl_info);
  }

  // If the connection data came in a dictionary, the values in the dictionary override whatever
  // is already loaded: i.e. if the dictionary indicated a stored session, that info is already
  // loaded but will be overriden with whatever extra values exist on the dictionary
  if (options) {
    if (options->has_key(kHost))
      _host = (*options)[kHost].as_string();

    if (options->has_key(kPort))
      _port = (*options)[kPort].as_int();

    if (options->has_key(kSocket))
      _sock = (*options)[kSocket].as_string();

    if (options->has_key(kSchema))
      _schema = (*options)[kSchema].as_string();

    if (options->has_key(kDbUser))
      _user = (*options)[kDbUser].as_string();
    else if (options->has_key(kUser))
      _user = (*options)[kUser].as_string();

    if (options->has_key(kDbPassword))
      _password = (*options)[kDbPassword].as_string();
    else if (options->has_key(kPassword))
      _password = (*options)[kPassword].as_string();


    _ssl_info.skip = true;

    if (options->has_key(kSslCa)) {
      _ssl_info.ca = (*options)[kSslCa].as_string();
      _ssl_info.skip = false;
    } else {
      _ssl_info.ca = "";
    }

    if (options->has_key(kSslCert)) {
      _ssl_info.cert = (*options)[kSslCert].as_string();
      _ssl_info.skip = false;
    } else {
      _ssl_info.cert = "";
    }

    if (options->has_key(kSslKey)) {
      _ssl_info.key = (*options)[kSslKey].as_string();
      _ssl_info.skip = false;
    } else {
      _ssl_info.key = "";
    }

    if (options->has_key(kSslCaPath)) {
      _ssl_info.capath = (*options)[kSslCaPath].as_string();
      _ssl_info.skip = false;
    } else {
      _ssl_info.capath = "";
    }

    if (options->has_key(kSslCrl)) {
      _ssl_info.crl = (*options)[kSslCrl].as_string();
      _ssl_info.skip = false;
    } else {
      _ssl_info.crl = "";
    }

    if (options->has_key(kSslCrlPath)) {
      _ssl_info.crlpath = (*options)[kSslCrlPath].as_string();
      _ssl_info.skip = false;
    } else {
      _ssl_info.crlpath = "";
    }

    if (options->has_key(kSslCiphers)) {
      _ssl_info.ciphers = (*options)[kSslCiphers].as_string();
      _ssl_info.skip = false;
    } else {
      _ssl_info.ciphers = "";
    }

    if (options->has_key(kSslTlsVersion)) {
      _ssl_info.tls_version = (*options)[kSslTlsVersion].as_string();
      _ssl_info.skip = false;
    } else {
      _ssl_info.tls_version = "";
    }

    if (options->has_key(kSslMode)) {
      if ((*options)[kSslMode].type == String) {
        const std::string& s = (*options)[kSslMode].as_string();
        int ssl_mode = MapSslModeNameToValue::get_value(s);
        if (ssl_mode == 0)
          throw std::runtime_error(
          "Invalid value for mode (must be any of [DISABLED, PREFERRED, REQUIRED, VERIFY_CA, VERIFY_IDENTITY] )");
        _ssl_info.mode = ssl_mode;
        _ssl_info.skip = false;
      } else {
        throw std::runtime_error(
          "Invalid value for mode (must be any of [DISABLED, PREFERRED, REQUIRED, VERIFY_CA, VERIFY_IDENTITY] )");
      }
    }

    if (options->has_key(kAuthMethod))
      _auth_method = (*options)[kAuthMethod].as_string();
  }

  // If password is received as parameter, then it overwrites
  // Anything found on any of the indicated sources: URI, options map and stored session
  if (2 == args.size())
    _password = args.string_at(1).c_str();

  if (_port == 0 && _sock.empty())
    _port = get_default_port();

  std::string sock_port = (_port == 0) ? _sock : boost::lexical_cast<std::string>(_port);

  if (_schema.empty())
    _uri = (boost::format("%1%@%2%:%3%") % _user % _host % sock_port).str();
  else
    _uri = (boost::format("%1%@%2%:%3%/%4%") % _user % _host % sock_port % _schema).str();
}

bool ShellBaseSession::operator == (const Object_bridge &other) const {
  return class_name() == other.class_name() && this == &other;
}

std::string ShellBaseSession::get_quoted_name(const std::string& name) {
  size_t index = 0;
  std::string quoted_name(name);

  while ((index = quoted_name.find("`", index)) != std::string::npos) {
    quoted_name.replace(index, 1, "``");
    index += 2;
  }

  quoted_name = "`" + quoted_name + "`";

  return quoted_name;
}

std::string ShellBaseSession::address() {
  std::string res;
  if (!_sock.empty())
    // If using a socket, then the host is localhost
    res = "localhost:" + _sock;
  else if (_port != 0)
    // if using a port, then the host is what was provided
    res = _host + ":" + std::to_string(_port);
  return res;
}

void ShellBaseSession::reconnect() {
  shcore::Argument_list args;
  args.push_back(shcore::Value(_uri));
  args.push_back(shcore::Value(_password));

  connect(args);
}

shcore::Object_bridge_ref ShellBaseSession::get_schema(const std::string &name) const {
  shcore::Object_bridge_ref ret_val;
  std::string type = "Schema";

  if (name.empty())
    throw Exception::runtime_error("Schema name must be specified");

  std::string found_name = db_object_exists(type, name, "");

  if (!found_name.empty()) {
    update_schema_cache(found_name, true);

    ret_val = (*_schemas)[found_name].as_object();
  } else {
    update_schema_cache(name, false);

    throw Exception::runtime_error("Unknown database '" + name + "'");
  }

  return ret_val;
}
