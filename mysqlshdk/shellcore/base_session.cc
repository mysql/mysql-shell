/*
 * Copyright (c) 2015, 2021, Oracle and/or its affiliates.
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

#include "mysqlshdk/include/shellcore/base_session.h"

#include "modules/devapi/mod_mysqlx_session.h"
#include "modules/mod_mysql_session.h"
#include "mysqlshdk/include/scripting/common.h"
#include "mysqlshdk/include/scripting/lang_base.h"
#include "mysqlshdk/include/scripting/object_factory.h"
#include "mysqlshdk/include/scripting/proxy_object.h"
#include "mysqlshdk/include/shellcore/interrupt_handler.h"
#include "mysqlshdk/include/shellcore/shell_core.h"
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "mysqlshdk/libs/ssh/ssh_manager.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_json.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

using namespace mysqlsh;
using namespace shcore;

DEBUG_OBJ_ENABLE(ShellBaseSession);

ShellBaseSession::ShellBaseSession() : _tx_deep(0) {
  DEBUG_OBJ_ALLOC(ShellBaseSession);
}

std::shared_ptr<ShellBaseSession> ShellBaseSession::wrap_session(
    std::shared_ptr<mysqlshdk::db::ISession> session) {
  if (auto classic =
          std::dynamic_pointer_cast<mysqlshdk::db::mysql::Session>(session)) {
    return std::make_shared<mysql::ClassicSession>(classic);
  } else if (auto x = std::dynamic_pointer_cast<mysqlshdk::db::mysqlx::Session>(
                 session)) {
    return std::make_shared<mysqlsh::mysqlx::Session>(x);
  }

  throw shcore::Exception::argument_error(
      "Invalid session type given for shell connection.");
}

ShellBaseSession::ShellBaseSession(const ShellBaseSession &s)
    : Cpp_object_bridge(),
      _connection_options(s._connection_options),
      _tx_deep(s._tx_deep) {
  DEBUG_OBJ_ALLOC(ShellBaseSession);
}

ShellBaseSession::~ShellBaseSession() { DEBUG_OBJ_DEALLOC(ShellBaseSession); }

std::string &ShellBaseSession::append_descr(std::string &s_out, int /*indent*/,
                                            int /*quote_strings*/) const {
  if (!is_open())
    s_out.append("<" + class_name() + ":disconnected>");
  else
    s_out.append("<" + class_name() + ":" +
                 uri(mysqlshdk::db::uri::formats::user_transport()) + ">");
  return s_out;
}

std::string &ShellBaseSession::append_repr(std::string &s_out) const {
  return append_descr(s_out, false);
}

void ShellBaseSession::append_json(shcore::JSON_dumper &dumper) const {
  dumper.start_object();

  dumper.append_string("class", class_name());
  dumper.append_bool("connected", is_open());

  if (is_open())
    dumper.append_string(
        "uri", uri(mysqlshdk::db::uri::formats::scheme_user_transport()));

  dumper.end_object();
}

bool ShellBaseSession::operator==(const Object_bridge &other) const {
  return class_name() == other.class_name() && this == &other;
}

std::string ShellBaseSession::get_quoted_name(const std::string &name) {
  size_t index = 0;
  std::string quoted_name(name);

  while ((index = quoted_name.find("`", index)) != std::string::npos) {
    quoted_name.replace(index, 1, "``");
    index += 2;
  }

  quoted_name = "`" + quoted_name + "`";

  return quoted_name;
}

std::string ShellBaseSession::uri(
    mysqlshdk::db::uri::Tokens_mask format) const {
  return _connection_options.as_uri(format);
}

std::string ShellBaseSession::ssh_uri() const {
  if (_connection_options.get_ssh_options().has_data()) {
    return _connection_options.get_ssh_options().as_uri(
        mysqlshdk::db::uri::formats::no_schema_no_query());
  }
  return "";
}

std::string ShellBaseSession::get_default_schema() {
  return _connection_options.has_schema() ? _connection_options.get_schema()
                                          : "";
}

void ShellBaseSession::reconnect() {
  const auto &ssh_data = _connection_options.get_ssh_options();

  // we need to increment port usage and decrement it later cause during
  // reconnect, the decrement will be called, so tunnel would be lost
  mysqlshdk::ssh::current_ssh_manager()->port_usage_increment(ssh_data);

  shcore::Scoped_callback scoped([ssh_data] {
    mysqlshdk::ssh::current_ssh_manager()->port_usage_decrement(ssh_data);
  });
  connect(_connection_options);
}

std::string ShellBaseSession::sub_query_placeholders(
    const std::string &query, const shcore::Array_t &args) {
  if (args) {
    shcore::sqlstring squery(query.c_str(), 0);
    int i = 0;
    for (const shcore::Value &value : *args) {
      try {
        switch (value.type) {
          case shcore::Integer:
            squery << value.as_int();
            break;
          case shcore::Bool:
            squery << value.as_bool();
            break;
          case shcore::Float:
            squery << value.as_double();
            break;
          case shcore::String:
            squery << value.get_string();
            break;
          case shcore::Null:
            squery << nullptr;
            break;
          default:
            throw Exception::argument_error(shcore::str_format(
                "Invalid type for placeholder value at index #%i", i));
        }
      } catch (Exception &e) {
        throw;
      } catch (const std::exception &e) {
        throw Exception::argument_error(shcore::str_format(
            "%s while substituting placeholder value at index #%i", e.what(),
            i));
      }
      ++i;
    }
    try {
      return squery.str();
    } catch (const std::exception &e) {
      throw Exception::argument_error(
          "Insufficient number of values for placeholders in query");
    }
  }
  return query;
}

void ShellBaseSession::begin_query() {
  if (_guard_active++ == 0) {
    // Install kill query as ^C handler
    current_interrupt()->push_handler([this]() {
      kill_query();
      return true;
    });
  }
}

void ShellBaseSession::end_query() {
  if (--_guard_active == 0) {
    current_interrupt()->pop_handler();
  }
}
