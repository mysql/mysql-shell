/*
 * Copyright (c) 2018, 2024, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/config/config_file_handler.h"

#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace config {

namespace {
bool config_var_to_bool(const std::string &name, std::string_view str_value) {
  if (shcore::str_caseeq(str_value, "YES") ||
      shcore::str_caseeq(str_value, "TRUE") ||
      shcore::str_caseeq(str_value, "1") || shcore::str_caseeq(str_value, "ON"))
    return true;
  if (shcore::str_caseeq(str_value, "NO") ||
      shcore::str_caseeq(str_value, "FALSE") ||
      shcore::str_caseeq(str_value, "0") ||
      shcore::str_caseeq(str_value, "OFF"))
    return false;

  throw std::runtime_error("The value of option '" + name +
                           "' cannot be converted to a boolean.");
}
}  // namespace

Config_file_handler::Config_file_handler(std::string server_uuid,
                                         std::string input_config_path,
                                         std::string output_cnf_path)
    : m_input_config_path(std::move(input_config_path)),
      m_output_config_path(std::move(output_cnf_path)),
      m_server_uuid(std::move(server_uuid)) {
  // read the config file
  m_config_file.read(m_input_config_path);
}

Config_file_handler::Config_file_handler(std::string server_uuid,
                                         std::string output_cnf_path)
    : m_output_config_path(std::move(output_cnf_path)),
      m_server_uuid(std::move(server_uuid)) {}

std::optional<bool> Config_file_handler::get_bool(
    const std::string &name) const {
  auto config_value = m_config_file.get(m_group, name);
  if (!config_value) return {};
  return config_var_to_bool(name, *config_value);
}

std::optional<std::string> Config_file_handler::get_string(
    const std::string &name) const {
  return m_config_file.get(m_group, name);
}

std::optional<int64_t> Config_file_handler::get_int(
    const std::string &name) const {
  auto config_value = m_config_file.get(m_group, name);
  if (!config_value) return {};

  try {
    return shcore::lexical_cast<int64_t>(*config_value);
  } catch (const std::invalid_argument &) {  // conversion failed.
    throw std::runtime_error("The value '" + *config_value + "' for option '" +
                             name + "' cannot be converted to an integer.");
  }
}

void Config_file_handler::set(const std::string &name,
                              std::optional<bool> value,
                              const std::string &context) {
  std::optional<std::string> value_to_set;
  if (value.has_value()) value_to_set = *value ? "ON" : "OFF";

  // create mysqld section if it doesn't exist
  if (!m_config_file.has_group(m_group)) m_config_file.add_group(m_group);
  try {
    m_config_file.set(m_group, name, value_to_set);
  } catch (const std::exception &err) {
    if (context.empty()) throw;

    std::string str_value =
        !value.has_value() ? "NULL" : (*value ? "true" : "false");

    throw std::runtime_error(
        shcore::str_format("Unable to set value %s for '%s': %s",
                           str_value.c_str(), context.c_str(), err.what()));
  }
}

void Config_file_handler::set(const std::string &name,
                              std::optional<int64_t> value,
                              const std::string &context) {
  std::optional<std::string> value_to_set;
  if (value.has_value())
    value_to_set = shcore::lexical_cast<std::string>(*value);

  // create mysqld section if it doesn't exist
  if (!m_config_file.has_group(m_group)) m_config_file.add_group(m_group);
  try {
    m_config_file.set(m_group, name, value_to_set);
  } catch (const std::exception &err) {
    if (context.empty()) throw;

    std::string str_value =
        !value.has_value() ? "NULL" : shcore::lexical_cast<std::string>(*value);

    throw std::runtime_error(
        shcore::str_format("Unable to set value %s for '%s': %s",
                           str_value.c_str(), context.c_str(), err.what()));
  }
}

void Config_file_handler::set(const std::string &name,
                              const std::optional<std::string> &value,
                              const std::string &context) {
  // create mysqld section if it doesn't exist
  if (!m_config_file.has_group(m_group)) m_config_file.add_group(m_group);
  try {
    m_config_file.set(m_group, name, value);
  } catch (const std::exception &err) {
    if (context.empty()) throw;

    std::string str_value =
        !value ? "NULL" : shcore::str_format("'%s'", value->c_str());

    throw std::runtime_error(
        shcore::str_format("Unable to set value %s for '%s': %s",
                           str_value.c_str(), context.c_str(), err.what()));
  }
}

void Config_file_handler::apply(bool) {
  // write to the output_config_path
  m_config_file.write(m_output_config_path);
}

bool Config_file_handler::remove(const std::string &name) {
  return m_config_file.remove_option(m_group, name);
}

void Config_file_handler::set_now(const std::string &name,
                                  std::optional<bool> value) {
  // Read existing configuration on the option file.
  Config_file config_file;
  if (!m_input_config_path.empty()) {
    config_file.read(m_input_config_path);
  } else {
    // Only read the contents of the output file if it exists.
    if (shcore::is_file(m_output_config_path)) {
      config_file.read(m_output_config_path);
    }
  }

  // Create de default group (mysqld) if it does not exist.
  if (!config_file.has_group(m_group)) {
    config_file.add_group(m_group);
  }

  // Set the specified option.
  std::optional<std::string> value_to_set;
  if (value) value_to_set = *value ? "ON" : "OFF";

  config_file.set(m_group, name, value_to_set);

  // Apply changes to the output option file.
  config_file.write(m_output_config_path);
}

void Config_file_handler::set_now(const std::string &name,
                                  std::optional<int64_t> value) {
  // Read existing configuration on the option file.
  Config_file config_file;
  if (!m_input_config_path.empty()) {
    config_file.read(m_input_config_path);
  } else {
    // Only read the contents of the output file if it exists.
    if (shcore::is_file(m_output_config_path)) {
      config_file.read(m_output_config_path);
    }
  }

  // Create the default group (mysqld) if it does not exist.
  if (!config_file.has_group(m_group)) {
    config_file.add_group(m_group);
  }

  // Set the specified option.
  std::optional<std::string> value_to_set;
  if (value) value_to_set = shcore::lexical_cast<std::string>(*value);

  config_file.set(m_group, name, value_to_set);

  // Apply changes to the output option file.
  config_file.write(m_output_config_path);
}

void Config_file_handler::set_now(const std::string &name,
                                  const std::optional<std::string> &value) {
  // Read existing configuration on the option file.
  Config_file config_file;
  if (!m_input_config_path.empty()) {
    config_file.read(m_input_config_path);
  } else {
    // Only read the contents of the output file if it exists.
    if (shcore::is_file(m_output_config_path)) {
      config_file.read(m_output_config_path);
    }
  }

  // Create de default group (mysqld) if it does not exist.
  if (!config_file.has_group(m_group)) {
    config_file.add_group(m_group);
  }

  // Set the specified option.
  config_file.set(m_group, name, value);

  // Apply changes to the output option file.
  config_file.write(m_output_config_path);
}

void Config_file_handler::set_output_config_path(
    std::string output_config_path) {
  m_output_config_path = std::move(output_config_path);
}

}  // namespace config
}  // namespace mysqlshdk
