/*
 * Copyright (c) 2018, 2021, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/config/config_file_handler.h"

#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace config {

Config_file_handler::Config_file_handler(const std::string &server_uuid,
                                         const std::string &input_config_path,
                                         const std::string &output_cnf_path)
    : m_input_config_path(input_config_path),
      m_output_config_path(output_cnf_path),
      m_server_uuid(server_uuid) {
  // read the config file
  m_config_file.read(m_input_config_path);
}

Config_file_handler::Config_file_handler(const std::string &server_uuid,
                                         const std::string &output_cnf_path)
    : m_output_config_path(output_cnf_path), m_server_uuid(server_uuid) {}

namespace {
bool config_var_to_bool(const std::string &name, const std::string &str_value) {
  const char *value = str_value.c_str();
  bool ret_val;
  if (shcore::str_caseeq(value, "YES") || shcore::str_caseeq(value, "TRUE") ||
      shcore::str_caseeq(value, "1") || shcore::str_caseeq(value, "ON"))
    ret_val = true;
  else if (shcore::str_caseeq(value, "NO") ||
           shcore::str_caseeq(value, "FALSE") ||
           shcore::str_caseeq(value, "0") || shcore::str_caseeq(value, "OFF"))
    ret_val = false;
  else
    throw std::runtime_error("The value of option '" + name +
                             "' cannot be converted to a boolean.");
  return ret_val;
}
}  // namespace

utils::nullable<bool> Config_file_handler::get_bool(
    const std::string &name) const {
  utils::nullable<bool> ret_val;
  auto config_value = m_config_file.get(m_group, name);
  if (!config_value.is_null()) {
    ret_val = config_var_to_bool(name, *config_value);
  }
  return ret_val;
}

utils::nullable<std::string> Config_file_handler::get_string(
    const std::string &name) const {
  return m_config_file.get(m_group, name);
}

utils::nullable<int64_t> Config_file_handler::get_int(
    const std::string &name) const {
  auto config_value = m_config_file.get(m_group, name);
  if (!config_value.is_null()) {
    try {
      return utils::nullable<int64_t>(
          shcore::lexical_cast<int64_t>(*config_value));
    } catch (const std::invalid_argument &) {  // conversion failed.
      throw std::runtime_error("The value '" + *config_value +
                               "' for option '" + name +
                               "' cannot be converted to an integer.");
    }
  } else {
    return utils::nullable<int64_t>();
  }
}

void Config_file_handler::set(const std::string &name,
                              const utils::nullable<bool> &value,
                              const std::string &context) {
  utils::nullable<std::string> value_to_set;
  if (!value.is_null()) {
    *value ? value_to_set = "ON" : value_to_set = "OFF";
  }
  // create mysqld section if it doesn't exist
  if (!m_config_file.has_group(m_group)) m_config_file.add_group(m_group);
  try {
    m_config_file.set(m_group, name, value_to_set);
  } catch (const std::exception &err) {
    if (!context.empty()) {
      std::string str_value =
          (value.is_null()) ? "NULL" : (*value) ? "true" : "false";
      throw std::runtime_error("Unable to set value " + str_value + " for '" +
                               context + "': " + err.what());
    } else {
      throw;
    }
  }
}

void Config_file_handler::set(const std::string &name,
                              const utils::nullable<int64_t> &value,
                              const std::string &context) {
  utils::nullable<std::string> value_to_set;
  if (!value.is_null()) {
    value_to_set = std::to_string(*value);
  }
  // create mysqld section if it doesn't exist
  if (!m_config_file.has_group(m_group)) m_config_file.add_group(m_group);
  try {
    m_config_file.set(m_group, name, value_to_set);
  } catch (const std::exception &err) {
    if (!context.empty()) {
      std::string str_value =
          (value.is_null()) ? "NULL" : std::to_string(*value);
      throw std::runtime_error("Unable to set value " + str_value + " for '" +
                               context + "': " + err.what());
    } else {
      throw;
    }
  }
}

void Config_file_handler::set(const std::string &name,
                              const utils::nullable<std::string> &value,
                              const std::string &context) {
  // create mysqld section if it doesn't exist
  if (!m_config_file.has_group(m_group)) m_config_file.add_group(m_group);
  try {
    m_config_file.set(m_group, name, value);
  } catch (const std::exception &err) {
    if (!context.empty()) {
      std::string str_value = (value.is_null()) ? "NULL" : "'" + *value + "'";
      throw std::runtime_error("Unable to set value " + str_value + " for '" +
                               context + "': " + err.what());
    } else {
      throw;
    }
  }
}

void Config_file_handler::apply() {
  // write to the output_config_path
  m_config_file.write(m_output_config_path);
}

bool Config_file_handler::remove(const std::string &name) {
  return m_config_file.remove_option(m_group, name);
}

void Config_file_handler::set_now(const std::string &name,
                                  const utils::nullable<bool> &value) {
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
  utils::nullable<std::string> value_to_set;
  if (!value.is_null()) {
    *value ? value_to_set = "ON" : value_to_set = "OFF";
  }
  config_file.set(m_group, name, value_to_set);

  // Apply changes to the output option file.
  config_file.write(m_output_config_path);
}

void Config_file_handler::set_now(const std::string &name,
                                  const utils::nullable<int64_t> &value) {
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
  utils::nullable<std::string> value_to_set;
  if (!value.is_null()) {
    value_to_set = std::to_string(*value);
  }
  config_file.set(m_group, name, value_to_set);

  // Apply changes to the output option file.
  config_file.write(m_output_config_path);
}

void Config_file_handler::set_now(const std::string &name,
                                  const utils::nullable<std::string> &value) {
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
    const std::string &output_config_path) {
  m_output_config_path = output_config_path;
}

}  // namespace config
}  // namespace mysqlshdk
