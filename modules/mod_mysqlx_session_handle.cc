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

#include "mod_mysqlx_session_handle.h"
#include "mysqlxtest_utils.h"
#include "mysqlx_connection.h"

using namespace mysh;
using namespace shcore;
using namespace mysh::mysqlx;

void SessionHandle::open(const std::string &host, int port, const std::string &schema,
                          const std::string &user, const std::string &pass,
                          const std::string &ssl_ca, const std::string &ssl_cert,
                          const std::string &ssl_key, const std::size_t timeout,
                          const std::string &auth_method, const bool get_caps)
{
  ::mysqlx::Ssl_config ssl;
  memset(&ssl, 0, sizeof(ssl));

  ssl.ca = ssl_ca.c_str();
  ssl.cert = ssl_cert.c_str();
  ssl.key = ssl_key.c_str();

  // TODO: Define a proper timeout for the session creation
  _session = ::mysqlx::openSession(host, port, schema, user, pass, ssl, 10000, auth_method, true);
}

boost::shared_ptr< ::mysqlx::Result> SessionHandle::execute_sql(const std::string &sql) const
{
  return _session->executeSql(sql);
}

void SessionHandle::enable_protocol_trace(bool value)
{
  _session->connection()->set_trace_protocol(value);
}

void SessionHandle::reset()
{
  if (_session)
  {
    _session->close();

    _session.reset();
  }
}

boost::shared_ptr< ::mysqlx::Result> SessionHandle::execute_statement(const std::string &domain, const std::string& command, const Argument_list &args) const
{
  // Will return the result of the SQL execution
  // In case of error will be Undefined
  boost::shared_ptr< ::mysqlx::Result> ret_val;

  if (!_session)
    throw Exception::logic_error("Not connected.");
  else
  {
    // Converts the arguments from shcore to mysqlxtest format
    std::vector< ::mysqlx::ArgumentValue> arguments;
    for (size_t index = 0; index < args.size(); index++)
      arguments.push_back(get_argument_value(args[index]));

    try
    {
      _last_result = _session->executeStmt(domain, command, arguments);

      // Calls wait so any error is properly triggered at execution time
      _last_result->wait();

      ret_val = _last_result;
    }
    CATCH_AND_TRANSLATE();
  }

  return ret_val;
}

::mysqlx::ArgumentValue SessionHandle::get_argument_value(shcore::Value source) const
{
  ::mysqlx::ArgumentValue ret_val;
  switch (source.type)
  {
    case shcore::Bool:
      ret_val = ::mysqlx::ArgumentValue(source.as_bool());
      break;
    case shcore::UInteger:
      ret_val = ::mysqlx::ArgumentValue(source.as_uint());
      break;
    case shcore::Integer:
      ret_val = ::mysqlx::ArgumentValue(source.as_int());
      break;
    case shcore::String:
      ret_val = ::mysqlx::ArgumentValue(source.as_string());
      break;
    case shcore::Float:
      ret_val = ::mysqlx::ArgumentValue(source.as_double());
      break;
    case shcore::Object:
    case shcore::Null:
    case shcore::Array:
    case shcore::Map:
    case shcore::MapRef:
    case shcore::Function:
    case shcore::Undefined:
      std::stringstream str;
      str << "Unsupported value received: " << source.descr();
      throw shcore::Exception::argument_error(str.str());
      break;
  }

  return ret_val;
}

shcore::Value SessionHandle::get_capability(const std::string& name)
{
  shcore::Value ret_val;

  if (_session)
  {
    const Mysqlx::Connection::Capabilities &caps(_session->connection()->capabilities());
    for (int c = caps.capabilities_size(), i = 0; i < c; i++)
    {
      if (caps.capabilities(i).name() == name)
      {
        const Mysqlx::Connection::Capability &cap(caps.capabilities(i));
        if (cap.value().type() == Mysqlx::Datatypes::Any::SCALAR &&
            cap.value().scalar().type() == Mysqlx::Datatypes::Scalar::V_STRING)
            ret_val = shcore::Value(cap.value().scalar().v_string().value());
        else if (cap.value().type() == Mysqlx::Datatypes::Any::SCALAR &&
                 cap.value().scalar().type() == Mysqlx::Datatypes::Scalar::V_OCTETS)
                 ret_val = shcore::Value(cap.value().scalar().v_octets().value());
      }
    }
  }

  return ret_val;
}

uint64_t SessionHandle::get_client_id()
{
  if (_session)
  {
    return _session->connection()->client_id();
  }
  else
    return 0;
}