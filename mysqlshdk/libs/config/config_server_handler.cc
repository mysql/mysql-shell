/*
 * Copyright (c) 2018, 2023, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/utils/logger.h"

namespace mysqlshdk {
namespace config {

Config_server_handler::Config_server_handler(mysql::IInstance *instance,
                                             mysql::Var_qualifier var_qualifier)
    : m_instance(instance), m_var_qualifier(var_qualifier) {
  assert(instance);
  m_get_scope = (var_qualifier == mysql::Var_qualifier::SESSION)
                    ? mysql::Var_qualifier::SESSION
                    : mysql::Var_qualifier::GLOBAL;
}

Config_server_handler::Config_server_handler(
    std::shared_ptr<mysql::IInstance> instance,
    mysql::Var_qualifier var_qualifier)
    : m_instance(instance.get()),
      m_instance_shared_ptr(std::move(instance)),
      m_var_qualifier(var_qualifier) {
  assert(m_instance);
  m_get_scope = (var_qualifier == mysql::Var_qualifier::SESSION)
                    ? mysql::Var_qualifier::SESSION
                    : mysql::Var_qualifier::GLOBAL;
}

Config_server_handler::~Config_server_handler() = default;

std::optional<bool> Config_server_handler::value_to_nullable_bool(
    const shcore::Value &value) {
  if (value.get_type() == shcore::Value_type::Null) return std::nullopt;
  return value.as_bool();
}

std::optional<int64_t> Config_server_handler::value_to_nullable_int(
    const shcore::Value &value) {
  if (value.get_type() == shcore::Value_type::Null) return std::nullopt;
  return value.as_int();
}

std::optional<std::string> Config_server_handler::value_to_nullable_string(
    const shcore::Value &value) {
  if (value.get_type() == shcore::Value_type::Null) return std::nullopt;
  return value.as_string();
}

std::optional<bool> Config_server_handler::get_bool(
    const std::string &name) const {
  return get_bool(name, m_var_qualifier);
}

std::optional<std::string> Config_server_handler::get_string(
    const std::string &name) const {
  return get_string(name, m_var_qualifier);
}

std::optional<int64_t> Config_server_handler::get_int(
    const std::string &name) const {
  return get_int(name, m_var_qualifier);
}

void Config_server_handler::set(const std::string &name,
                                std::optional<bool> value,
                                const std::string &context) {
  set(name, value, m_var_qualifier, std::chrono::milliseconds::zero(), context);
}

void Config_server_handler::set(const std::string &name,
                                std::optional<int64_t> value,
                                const std::string &context) {
  set(name, value, m_var_qualifier, std::chrono::milliseconds::zero(), context);
}

void Config_server_handler::set(const std::string &name,
                                const std::optional<std::string> &value,
                                const std::string &context) {
  set(name, value, m_var_qualifier, std::chrono::milliseconds::zero(), context);
}

void Config_server_handler::apply() {
  for (const auto &var : m_change_sequence) {
    auto var_type = var.value.get_type();
    try {
      if (var_type == shcore::Value_type::Bool) {
        bool value = *value_to_nullable_bool(var.value);
        log_debug("Set '%s'=%s", var.name.c_str(), value ? "true" : "false");
        m_instance->set_sysvar(var.name, value, var.qualifier);
      } else if (var_type == shcore::Value_type::Integer) {
        int64_t value = *value_to_nullable_int(var.value);
        log_debug("Set '%s'=%" PRId64, var.name.c_str(), value);
        m_instance->set_sysvar(var.name, value, var.qualifier);
      } else {
        std::string value = *value_to_nullable_string(var.value);
        log_debug("Set '%s'='%s'", var.name.c_str(), value.c_str());
        m_instance->set_sysvar(var.name, value, var.qualifier);
      }
    } catch (const std::exception &err) {
      if (var.context.empty()) throw;

      // Send error with context information (more user friendly).
      std::string value =
          (var_type == shcore::Value_type::Null)
              ? "NULL"
              : shcore::str_format("'%s'", var.value.as_string().c_str());
      throw std::runtime_error(
          shcore::str_format("Unable to set value %s for '%s': %s",
                             value.c_str(), var.context.c_str(), err.what()));
    }

    // Sleep after setting the variable if delay is defined (> 0).
    if (var.timeout.count() > 0) shcore::sleep(var.timeout);
  }

  m_change_sequence.clear();
  m_global_change_tracker.clear();
  m_session_change_tracker.clear();
}

std::string Config_server_handler::get_server_uuid() const {
  return *get_string("server_uuid");
}

std::optional<bool> Config_server_handler::get_bool(
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

std::optional<std::string> Config_server_handler::get_string(
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

std::optional<int64_t> Config_server_handler::get_int(
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

std::optional<bool> Config_server_handler::get_bool_now(
    const std::string &name, const mysql::Var_qualifier var_qualifier) const {
  // Throw an error if the variable does not exist instead of returning null,
  // to be consistent with other config handlers.
  std::optional<bool> res = m_instance->get_sysvar_bool(name, var_qualifier);
  if (!res) throw std::out_of_range{"Variable '" + name + "' does not exist."};

  return res;
}

std::optional<int64_t> Config_server_handler::get_int_now(
    const std::string &name, const mysql::Var_qualifier var_qualifier) const {
  // Throw an error if the variable does not exist instead of returning null,
  // to be consistent with other config handlers.
  std::optional<int64_t> res = m_instance->get_sysvar_int(name, var_qualifier);
  if (!res) throw std::out_of_range{"Variable '" + name + "' does not exist."};

  return res;
}

std::optional<std::string> Config_server_handler::get_string_now(
    const std::string &name, const mysql::Var_qualifier var_qualifier) const {
  // Throw an error if the variable does not exist instead of returning null,
  // to be consistent with other config handlers.
  std::optional<std::string> res =
      m_instance->get_sysvar_string(name, var_qualifier);
  if (!res) throw std::out_of_range{"Variable '" + name + "' does not exist."};

  return res;
}

std::optional<std::string> Config_server_handler::get_persisted_value(
    const std::string &name) const {
  try {
    return m_instance->get_persisted_value(name);
  } catch (const mysqlshdk::db::Error &err) {
    throw std::runtime_error{"Unable to get persisted value for '" + name +
                             "': " + err.what()};
  }
}

}  // namespace config
}  // namespace mysqlshdk
