/*
 * Copyright (c) 2015, 2025, Oracle and/or its affiliates.
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

#include "mysqlshdk/include/shellcore/base_session.h"

#include "modules/devapi/mod_mysqlx_session.h"
#include "modules/mod_mysql_session.h"
#include "mysqlshdk/include/scripting/obj_date.h"
#include "mysqlshdk/include/shellcore/scoped_contexts.h"
#include "mysqlshdk/include/shellcore/sql_handler.h"
#include "mysqlshdk/libs/ssh/ssh_manager.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_json.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

using namespace mysqlsh;
using namespace shcore;

DEBUG_OBJ_ENABLE(ShellBaseSession);

bool Query_attribute_store::set(const std::string &name,
                                const shcore::Value &value) {
  constexpr size_t MAX_QUERY_ATTRIBUTES = 32;
  constexpr size_t MAX_QUERY_ATTRIBUTE_LENGTH = 1024;

  // Searches for the element first
  const auto it = m_store.find(name);

  // Validates name for any new attribute
  if (it == m_store.end()) {
    if (m_order.size() >= MAX_QUERY_ATTRIBUTES) {
      m_exceeded.push_back(name);
      return false;
    }
    if (name.size() > MAX_QUERY_ATTRIBUTE_LENGTH) {
      m_invalid_names.push_back(name);
      return false;
    };
  }

  // Validates the value type
  auto type = value.get_type();
  if (type == shcore::Value_type::Undefined ||
      type == shcore::Value_type::Array || type == shcore::Value_type::Map ||
      type == shcore::Value_type::Function ||
      type == shcore::Value_type::Binary ||
      (type == shcore::Value_type::Object && !value.as_object<Date>())) {
    m_unsupported_type.push_back(name);
    return false;
  }

  // Validates the string value lengths
  if (type == shcore::Value_type::String &&
      value.get_string().size() > MAX_QUERY_ATTRIBUTE_LENGTH) {
    m_invalid_value_length.push_back(name);
    return false;
  }

  // Inserts or updates the value
  m_store[name] = value;

  // Adds the new value to the order
  if (it == m_store.end()) {
    m_order.emplace_back(m_store.find(name)->first);
  }

  return true;
}

bool Query_attribute_store::set(const shcore::Dictionary_t &attributes) {
  bool ret_val = true;

  clear();

  for (auto it = attributes->begin(); it != attributes->end(); it++) {
    if (!set(it->first, it->second)) {
      ret_val = false;
    }
  }

  return ret_val;
}

void Query_attribute_store::handle_errors(bool raise_error) {
  std::vector<std::string> issues;
  auto validate = [&issues](const std::vector<std::string> &data,
                            const std::string &message) -> void {
    if (!data.empty()) {
      bool singular = data.size() == 1;
      issues.push_back(
          shcore::str_format(message.data(), (singular ? "" : "s"),
                             shcore::str_join(data, ", ").c_str()));
    }
  };

  validate(m_invalid_names,
           "The following query attribute%s exceed the maximum name length "
           "(1024): %s");

  validate(m_invalid_value_length,
           "The following query attribute%s exceed the maximum value length "
           "(1024): %s");

  validate(m_unsupported_type,
           "The following query attribute%s have an unsupported data type: %s");

  validate(m_exceeded,
           "The following query attribute%s exceed the maximum limit (32): %s");

  if (!issues.empty()) {
    std::string error = "Invalid query attributes found";

    if (!raise_error) error.append(", they will be ignored");

    const auto message = shcore::str_format(
        "%s: %s", error.c_str(), shcore::str_join(issues, "\n\t").c_str());

    if (raise_error) {
      clear();
      throw std::invalid_argument(message);
    } else {
      mysqlsh::current_console()->print_warning(message);
    }
  }
}

void Query_attribute_store::clear() {
  m_order.clear();
  m_store.clear();
  m_invalid_names.clear();
  m_invalid_value_length.clear();
  m_unsupported_type.clear();
  m_exceeded.clear();
}

std::vector<mysqlshdk::db::Query_attribute>
Query_attribute_store::get_query_attributes(
    const std::function<std::unique_ptr<mysqlshdk::db::IQuery_attribute_value>(
        const shcore::Value &)> &translator_cb) const {
  assert(translator_cb);

  std::vector<mysqlshdk::db::Query_attribute> attributes;
  attributes.reserve(m_order.size());

  for (const auto &name : m_order) {
    attributes.emplace_back(name, translator_cb(m_store.at(std::string{name})));
  }

  return attributes;
}

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
    s_out.append("<").append(class_name()).append(":disconnected>");
  else
    s_out.append("<")
        .append(class_name())
        .append(":")
        .append(uri(mysqlshdk::db::uri::formats::user_transport()))
        .append(">");
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

/**
 * Handles the execution of SQL statements considering any registered SQL
 * execution handlers.
 *
 * - If many SQL handlers are registered, they will be executed in the order of
 *   registration.
 * - The execution will be interrupted as soon as an SQL handler returns a
 *   Result object.
 */
std::shared_ptr<mysqlshdk::db::IResult> ShellBaseSession::execute_sql(
    std::string_view query, const shcore::Array_t &args,
    const std::vector<mysqlshdk::db::Query_attribute> &query_attributes,
    bool use_sql_handlers) {
  std::shared_ptr<mysqlshdk::db::IResult> ret_val;

  const auto full_query = sub_query_placeholders(query, args);
  query = full_query;

  // Gets the SQL handlers to be executed for the given query
  shcore::Sql_handler_ptr sql_handler = nullptr;

  // Only use SQL handlers for SQL coming from the user, determined by
  // enable_sql_handlers
  if (use_sql_handlers) {
    sql_handler =
        shcore::current_sql_handler_registry()->get_handler_for_sql(query);
  }

  if (sql_handler) {
    if (m_active_custom_sql.empty()) {
      m_active_custom_sql = query;
    } else {
      throw shcore::Exception::logic_error(
          shcore::str_format("Unable to execute a sql handler while another "
                             "is being executed. "
                             "Executing SQL: %s. Unable to execute: %.*s",
                             m_active_custom_sql.c_str(),
                             static_cast<int>(query.length()), query.data()));
    }

    shcore::Scoped_callback enable_sql_handlers(
        [this]() { m_active_custom_sql.clear(); });

    shcore::Argument_list sql_handler_args;
    sql_handler_args.emplace_back(get_shared_this());
    sql_handler_args.emplace_back(query);

    const auto value = sql_handler->callback()->invoke(sql_handler_args);

    // An SQL handler may return either nothing or a Result object
    if (value && !value.is_null()) {
      bool invalid_return_type = false;
      try {
        auto result = value.as_object<mysqlsh::ShellBaseResult>();
        if (result) {
          ret_val = result->get_result();
        } else {
          // Could be any other object
          invalid_return_type = true;
        }
      } catch (...) {
        // This happens when returned data is not an object
        invalid_return_type = true;
      }
      if (invalid_return_type) {
        throw shcore::Exception::runtime_error(shcore::str_format(
            "Invalid data returned by the SQL handler, if "
            "any, the returned data should be a result object."));
      }
    }
  }

  if (!ret_val) {
    // The statement will be executed on this session ONLY if it was not already
    // executed by a handler
    ret_val = do_execute_sql(query, {}, query_attributes);
  }

  return ret_val;
}

std::string ShellBaseSession::get_quoted_name(std::string_view name) {
  std::string quoted_name;
  quoted_name.reserve(name.size() + 2);

  size_t index = 1;  // ignore the first '`'
  quoted_name.append("`");
  quoted_name.append(name);

  while ((index = quoted_name.find('`', index)) != std::string::npos) {
    quoted_name.replace(index, 1, "``");
    index += 2;
  }

  quoted_name.append("`");

  return quoted_name;
}

std::string ShellBaseSession::uri(
    mysqlshdk::db::uri::Tokens_mask format) const {
  return _connection_options.as_uri(format);
}

std::string ShellBaseSession::ssh_uri() const {
  if (_connection_options.get_ssh_options().has_data()) {
    return _connection_options.get_ssh_options().as_uri(
        mysqlshdk::db::uri::formats::full_no_password());
  }
  return "";
}

std::string ShellBaseSession::get_default_schema() {
  return _connection_options.has_schema() ? _connection_options.get_schema()
                                          : "";
}

void ShellBaseSession::reconnect() {
  // we need to hold the reference to the SSH tunnel while we reconnect, to
  // prevent it from being closed when the session closes
  const auto ssh = mysqlshdk::ssh::current_ssh_manager()->get_tunnel(
      _connection_options.get_ssh_options());

  connect(_connection_options);
}

std::string ShellBaseSession::sub_query_placeholders(
    std::string_view query, const shcore::Array_t &args) {
  if (!args || args->empty()) return std::string{query};

  shcore::sqlstring squery(std::string{query}, 0);
  int i = 0;
  for (const shcore::Value &value : *args) {
    try {
      switch (value.get_type()) {
        case shcore::Integer:
          squery << value.as_int();
          break;
        case shcore::Bool:
          squery << value.as_bool();
          break;
        case shcore::Float:
          squery << value.as_double();
          break;
        case shcore::Binary:
          squery << value.get_string();
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
    } catch (const Exception &) {
      throw;
    } catch (const std::exception &e) {
      throw Exception::argument_error(shcore::str_format(
          "%s while substituting placeholder value at index #%i", e.what(), i));
    }
    ++i;
  }
  try {
    return squery.str();
  } catch (const std::exception &) {
    throw Exception::argument_error(
        "Insufficient number of values for placeholders in query");
  }

  throw std::logic_error{"ShellBaseSession::sub_query_placeholders()"};
}

void ShellBaseSession::set_query_attributes(const shcore::Dictionary_t &args) {
  if (!m_query_attributes.set(args)) {
    m_query_attributes.handle_errors(true);
  }
}

void ShellBaseSession::set_client_data(const std::string &key,
                                       const shcore::Value &value) {
  if (!m_client_data) m_client_data = shcore::make_dict();

  (*m_client_data)[key] = value;
}

shcore::Value ShellBaseSession::get_client_data(const std::string &key) const {
  if (m_client_data) {
    if (auto it = m_client_data->find(key); it != m_client_data->end()) {
      return it->second;
    }
  }
  return {};
}

std::string ShellBaseSession::get_sql_mode() {
  if (!m_tracking_sql_mode) get_core_session()->refresh_sql_mode();
  return get_core_session()->get_sql_mode();
}

std::string ShellBaseSession::track_system_variable(
    const std::string &variable) {
  auto new_value = get_core_session()->track_system_variable(variable);
  m_tracking_sql_mode =
      shcore::has_session_track_system_variable(new_value, "sql_mode");
  return new_value;
}
