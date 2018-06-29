/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/libs/config/config_server_handler.h"

namespace mysqlshdk {
namespace config {

Config_server_handler::Config_server_handler(
    mysql::IInstance *instance, const mysql::Var_qualifier &var_qualifier)
    : m_instance(instance), m_var_qualifier(var_qualifier) {
  assert(instance);
  if (var_qualifier == mysql::Var_qualifier::SESSION) {
    m_get_scope = mysql::Var_qualifier::SESSION;
  } else {
    m_get_scope = mysql::Var_qualifier::GLOBAL;
  }
}

utils::nullable<bool> Config_server_handler::value_to_nullable_bool(
    const shcore::Value &value) {
  if (value.type == shcore::Value_type::Null) {
    return utils::nullable<bool>();
  } else {
    return utils::nullable<bool>(value.as_bool());
  }
}

utils::nullable<int64_t> Config_server_handler::value_to_nullable_int(
    const shcore::Value &value) {
  if (value.type == shcore::Value_type::Null) {
    return utils::nullable<int64_t>();
  } else {
    return utils::nullable<int64_t>(value.as_int());
  }
}

utils::nullable<std::string> Config_server_handler::value_to_nullable_string(
    const shcore::Value &value) {
  if (value.type == shcore::Value_type::Null) {
    return utils::nullable<std::string>();
  } else {
    return utils::nullable<std::string>(value.as_string());
  }
}

utils::nullable<bool> Config_server_handler::get_bool(
    const std::string &name) const {
  return get_bool(name, m_var_qualifier);
}

utils::nullable<std::string> Config_server_handler::get_string(
    const std::string &name) const {
  return get_string(name, m_var_qualifier);
}

utils::nullable<int64_t> Config_server_handler::get_int(
    const std::string &name) const {
  return get_int(name, m_var_qualifier);
}

void Config_server_handler::set(const std::string &name,
                                const utils::nullable<bool> &value) {
  set(name, value, m_var_qualifier);
}

void Config_server_handler::set(const std::string &name,
                                const utils::nullable<int64_t> &value) {
  set(name, value, m_var_qualifier);
}

void Config_server_handler::set(const std::string &name,
                                const utils::nullable<std::string> &value) {
  set(name, value, m_var_qualifier);
}

void Config_server_handler::apply() {
  for (const auto &var_tuple : m_change_sequence) {
    if (std::get<1>(var_tuple).type == shcore::Value_type::Bool) {
      m_instance->set_sysvar(std::get<0>(var_tuple),
                             *value_to_nullable_bool(std::get<1>(var_tuple)),
                             std::get<2>(var_tuple));
    } else if (std::get<1>(var_tuple).type == shcore::Value_type::Integer) {
      m_instance->set_sysvar(std::get<0>(var_tuple),
                             *value_to_nullable_int(std::get<1>(var_tuple)),
                             std::get<2>(var_tuple));
    } else {
      m_instance->set_sysvar(std::get<0>(var_tuple),
                             *value_to_nullable_string(std::get<1>(var_tuple)),
                             std::get<2>(var_tuple));
    }

    // Sleep after setting the variable if delay is defined (> 0).
    if (std::get<3>(var_tuple) > 0) {
      shcore::sleep_ms(std::get<3>(var_tuple));
    }
  }

  m_change_sequence.clear();
  m_global_change_tracker.clear();
  m_session_change_tracker.clear();
}

utils::nullable<bool> Config_server_handler::get_bool(
    const std::string &name, const mysql::Var_qualifier var_qualifier) const {
  // NOTE: PERSIST_ONLY variables are not stored in the internal map with the
  // cache of changes, always read directly from the server.
  if (var_qualifier == mysql::Var_qualifier::GLOBAL ||
      var_qualifier == mysql::Var_qualifier::PERSIST) {
    if (m_global_change_tracker.find(name) != m_global_change_tracker.end()) {
      return value_to_nullable_bool(m_global_change_tracker.at(name));
    }
  } else if (var_qualifier == mysql::Var_qualifier::SESSION) {
    if (m_session_change_tracker.find(name) != m_session_change_tracker.end()) {
      return value_to_nullable_bool(m_session_change_tracker.at(name));
    }
  }

  // if variable not on the cache
  return get_bool_now(name, m_get_scope);
}

utils::nullable<std::string> Config_server_handler::get_string(
    const std::string &name, const mysql::Var_qualifier var_qualifier) const {
  // NOTE: PERSIST_ONLY variables are not stored in the internal map with the
  // cache of changes, always read directly from the server.
  if (var_qualifier == mysql::Var_qualifier::GLOBAL ||
      var_qualifier == mysql::Var_qualifier::PERSIST) {
    if (m_global_change_tracker.find(name) != m_global_change_tracker.end()) {
      return value_to_nullable_string(m_global_change_tracker.at(name));
    }
  } else if (var_qualifier == mysql::Var_qualifier::SESSION) {
    if (m_session_change_tracker.find(name) != m_session_change_tracker.end()) {
      return value_to_nullable_string(m_session_change_tracker.at(name));
    }
  }

  // if variable not on the cache
  return get_string_now(name, m_get_scope);
}

utils::nullable<int64_t> Config_server_handler::get_int(
    const std::string &name, const mysql::Var_qualifier var_qualifier) const {
  // NOTE: PERSIST_ONLY variables are not stored in the internal map with the
  // cache of changes, always read directly from the server.
  if (var_qualifier == mysql::Var_qualifier::GLOBAL ||
      var_qualifier == mysql::Var_qualifier::PERSIST) {
    if (m_global_change_tracker.find(name) != m_global_change_tracker.end()) {
      return value_to_nullable_int(m_global_change_tracker.at(name));
    }
  } else if (var_qualifier == mysql::Var_qualifier::SESSION) {
    if (m_session_change_tracker.find(name) != m_session_change_tracker.end()) {
      return value_to_nullable_int(m_session_change_tracker.at(name));
    }
  }
  // if variable not on the cache
  return get_int_now(name, m_get_scope);
}

utils::nullable<bool> Config_server_handler::get_bool_now(
    const std::string &name, const mysql::Var_qualifier var_qualifier) const {
  // Throw an error if the variable does not exist instead of returning null,
  // to be consistent with other config handlers.
  utils::nullable<bool> res = m_instance->get_sysvar_bool(name, var_qualifier);
  if (res.is_null()) {
    throw std::out_of_range{"Variable '" + name + "' does not exist."};
  }
  return res;
}

utils::nullable<int64_t> Config_server_handler::get_int_now(
    const std::string &name, const mysql::Var_qualifier var_qualifier) const {
  // Throw an error if the variable does not exist instead of returning null,
  // to be consistent with other config handlers.
  utils::nullable<int64_t> res =
      m_instance->get_sysvar_int(name, var_qualifier);
  if (res.is_null()) {
    throw std::out_of_range{"Variable '" + name + "' does not exist."};
  }
  return res;
}

utils::nullable<std::string> Config_server_handler::get_string_now(
    const std::string &name, const mysql::Var_qualifier var_qualifier) const {
  // Throw an error if the variable does not exist instead of returning null,
  // to be consistent with other config handlers.
  utils::nullable<std::string> res =
      m_instance->get_sysvar_string(name, var_qualifier);
  if (res.is_null()) {
    throw std::out_of_range{"Variable '" + name + "' does not exist."};
  }
  return res;
}

}  // namespace config
}  // namespace mysqlshdk
