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

#ifndef MYSQLSHDK_LIBS_CONFIG_CONFIG_SERVER_HANDLER_H_
#define MYSQLSHDK_LIBS_CONFIG_CONFIG_SERVER_HANDLER_H_

#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/libs/config/config.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlshdk {
namespace config {

/**
 * Object to manage (handle) server configurations.
 *
 * This object allows to manage the configurations, i.e., system variables, of
 * an online (available) server, implementing the IConfig_handler interface (so
 * that it can be used by the generic mysqlshdk::config::Config object).
 */
class Config_server_handler : public IConfig_handler {
 public:
  /**
   * Constructor
   *
   * NOTE: This constructor receives a raw pointer, thus the ownership of the
   * pointer is not taken by Config_server_handler
   *
   * @param instance raw pointer with the target Instance (server) to be
   *        handled.
   * @param var_qualifier type of variable qualifier to be used for the gets and
   * sets
   */
  explicit Config_server_handler(mysql::IInstance *instance,
                                 const mysql::Var_qualifier &var_qualifier);

  /**
   * Constructor
   *
   * NOTE: This constructor receives a unique pointer, thus the ownership of the
   * pointer is taken by Config_server_handler. It will be released and its
   * internal session closed as soon as the object goes out of scope
   *
   * @param instance unique pointer with the target Instance (server) to be
   *        handled.
   * @param var_qualifier type of variable qualifier to be used for the gets and
   * sets
   */
  explicit Config_server_handler(std::unique_ptr<mysql::IInstance> instance,
                                 const mysql::Var_qualifier &var_qualifier);

  /**
   * Destructor
   */
  ~Config_server_handler();

  /**
   * Get the boolean value for the specified server configuration (system
   * variable).
   *
   * @param name string with the name of the configuration to get.
   * @return nullable boolean with the value for the specified configuration.
   * @throw std::out_of_range if the specified configuration does not exist.
   */
  utils::nullable<bool> get_bool(const std::string &name) const override;

  /**
   * Get the string value for the specified server configuration (system
   * variable).
   *
   * @param name string with the name of the configuration to get.
   * @return nullable string with the value for the specified configuration.
   * @throw std::out_of_range if the specified configuration does not exist.
   */
  utils::nullable<std::string> get_string(
      const std::string &name) const override;

  /**
   * Get the integer value for the specified server configuration (system
   * variable).
   *
   * @param name string with the name of the configuration to get.
   * @return nullable integer with the value for the specified configuration.
   * @throw std::out_of_range if the specified configuration does not exist.
   */
  utils::nullable<int64_t> get_int(const std::string &name) const override;

  /**
   * Set the given server configuration (system variable) with the specified
   * value.
   *
   * @param name string with the name of the configuration to set.
   * @param value nullable boolean with the value to set.
   * @throw std::invalid_argument if the specified value is null.
   */
  void set(const std::string &name,
           const utils::nullable<bool> &value) override;

  /**
   * Set the given server configuration (system variable) with the specified
   * value.
   *
   * @param name string with the name of the configuration to set.
   * @param value nullable integer with the value to set.
   * @throw std::invalid_argument if the specified value is null.
   */
  void set(const std::string &name,
           const utils::nullable<int64_t> &value) override;

  /**
   * Set the given server configuration (system variable) with the specified
   * value.
   *
   * @param name string with the name of the configuration to set.
   * @param value nullable string with the value to set.
   * @throw std::invalid_argument if the specified value is null.
   */
  void set(const std::string &name,
           const utils::nullable<std::string> &value) override;

  /**
   * Effectively apply all the server configuration (system variable) changes.
   *
   * All changes performed using the set() function are only registered
   * internally without (immediately) applying them (i.e., not directly changing
   * the corresponding server system variables).
   * This function applies all recorded changes to the server system variables.
   *
   * @throw mysqlshdk::db::Error if any error occurs trying to set (apply) the
   *        configurations on the server.
   */
  void apply() override;

  /**
   * Get the boolean value for the specified server configuration (system
   * variable) and specified variable qualifier.
   *
   * @param name string with the name of the configuration to get.
   * @param var_qualifier Var_qualifier with the qualifier used to get the
   *        variable.
   * @return nullable boolean with the value for the specified configuration.
   * @throw std::out_of_range if the specified configuration does not exist.
   */
  utils::nullable<bool> get_bool(
      const std::string &name, const mysql::Var_qualifier var_qualifier) const;

  /**
   * Get the string value for the specified server configuration (system
   * variable) and specified variable qualifier.
   *
   * @param name string with the name of the configuration to get.
   * @param var_qualifier Var_qualifier with the qualifier used to get the
   *        variable.
   * @return nullable string with the value for the specified configuration.
   * @throw std::out_of_range if the specified configuration does not exist.
   */
  utils::nullable<std::string> get_string(
      const std::string &name, const mysql::Var_qualifier var_qualifier) const;

  /**
   * Get the integer value for the specified server configuration (system
   * variable) and specified variable qualifier.
   *
   * @param name string with the name of the configuration to get.
   * @param var_qualifier Var_qualifier with the qualifier used to get the
   *        variable.
   * @return nullable integer with the value for the specified configuration.
   * @throw std::out_of_range if the specified configuration does not exist.
   */
  utils::nullable<int64_t> get_int(
      const std::string &name, const mysql::Var_qualifier var_qualifier) const;

  /**
   * Get the boolean value for the specified server configuration (system
   * variable) and specified variable qualifier immediately on the server.
   *
   * NOTE: This function gets the value directly from the server (ignoring
   *       the value hold by the handler if it was already read).
   *
   * @param name string with the name of the configuration to get.
   * @param var_qualifier Var_qualifier with the qualifier used to get the
   *        variable.
   * @return nullable boolean with the value for the specified configuration.
   * @throw std::out_of_range if the specified configuration does not exist.
   */
  utils::nullable<bool> get_bool_now(
      const std::string &name, const mysql::Var_qualifier var_qualifier) const;

  /**
   * Get the string value for the specified server configuration (system
   * variable) and specified variable qualifier immediately on the server.
   *
   * NOTE: This function gets the value directly from the server (ignoring
   *       the value hold by the handler if it was already read).
   *
   * @param name string with the name of the configuration to get.
   * @param var_qualifier Var_qualifier with the qualifier used to get the
   *        variable.
   * @return nullable string with the value for the specified configuration.
   * @throw std::out_of_range if the specified configuration does not exist.
   */
  utils::nullable<std::string> get_string_now(
      const std::string &name, const mysql::Var_qualifier var_qualifier) const;

  /**
   * Get the integer value for the specified server configuration (system
   * variable) and specified variable qualifier immediately on the server.
   *
   * NOTE: This function gets the value directly from the server (ignoring
   *       the value hold by the handler if it was already read).
   *
   * @param name string with the name of the configuration to get.
   * @param var_qualifier Var_qualifier with the qualifier used to get the
   *        variable.
   * @return nullable integer with the value for the specified configuration.
   * @throw std::out_of_range if the specified configuration does not exist.
   */
  utils::nullable<int64_t> get_int_now(
      const std::string &name, const mysql::Var_qualifier var_qualifier) const;

  /**
   * Set the given server configuration (system variable) with the specified
   * value and specified variable qualifier.
   *
   * @param name string with the name of the configuration to set.
   * @param value nullable string with the value to set.
   * @param var_qualifier Var_qualifier with the qualifier used to set the
   *        variable.
   * @param delay time in ms to wait after actually setting the variable when
   *              executing the apply() function. By default 0, do not wait.
   *              NOTE: This allow to workaround BUG#27629719, ensuring
   *              different timestamps are set for variable that need to be set
   *              in a specific order.
   * @throw std::invalid_argument if the specified value is null.
   *
   */
  template <typename T>
  void set(const std::string &name, const utils::nullable<T> &value,
           const mysql::Var_qualifier var_qualifier, uint32_t delay = 0) {
    // The value cannot be null (not supported by set_sysvar())
    if (value.is_null()) {
      throw std::invalid_argument{"The value parameter cannot be null."};
    }

    // NOTE: PERSIST_ONLY variables are only stored in the internal sequence of
    // changes to apply, but not on the map that hold a cache of the changes.
    shcore::Value val = nullable_type_to_value(value);
    m_change_sequence.emplace_back(name, val, var_qualifier, delay);

    // Store variable changes in an internal cache (used by get()).
    if (var_qualifier == mysql::Var_qualifier::GLOBAL ||
        var_qualifier == mysql::Var_qualifier::PERSIST) {
      m_global_change_tracker[name] = val;
    } else if (var_qualifier == mysql::Var_qualifier::SESSION) {
      m_session_change_tracker[name] = val;
    }
  }

  /**
   * Set the given server configuration (system variable) with the specified
   * value immediately on the server (applying it).
   *
   * @param name string with the name of the configuration to set.
   * @param value nullable string with the value to set.
   * @param var_qualifier Optional Var_qualifier with the qualifier used to set
   *        the variable. If not specified, the default qualifier set when
   *        creating the handler is used.
   * @param delay time in ms to wait after setting the variable. By default 0,
   *              do not wait.
   *              NOTE: This allow to workaround BUG#27629719, ensuring
   *              different timestamps are set for variable that need to be set
   *              in a specific order.
   * @throw std::invalid_argument if the specified value is null.
   * @throw mysqlshdk::db::Error if any error occurs trying to set specified
   *        configuration on the server.
   */
  template <typename T>
  void set_now(const std::string &name, const utils::nullable<T> &value,
               const mysql::Var_qualifier var_qualifier, uint32_t delay = 0) {
    // The value cannot be null (not supported by set_sysvar()).
    if (value.is_null()) {
      throw std::invalid_argument{"The value parameter cannot be null."};
    }

    m_instance->set_sysvar(name, *value, var_qualifier);

    // Sleep after setting the variable if delay is defined (> 0).
    if (delay > 0) {
      shcore::sleep_ms(delay);
    }
  }

  /**
   * Get the default variable qualifier being used.
   *
   * @return mysqlshdk::mysql::Var_qualifier enum class with the variable
   *         qualifier being used by default.
   */
  mysql::Var_qualifier get_default_var_qualifier() const {
    return m_var_qualifier;
  }

 private:
  /**
   * Auxiliary function to convert a shcore::Value (holding a bool) to a
   * mysqlshdk::utils::nullable<bool>.
   *
   * @param value Value (with the bool) to convert
   * @return utils::nullable<bool> object corresponding to the specified Value
   *         object (with a bool).
   */
  static utils::nullable<bool> value_to_nullable_bool(
      const shcore::Value &value);

  /**
   * Auxiliary function to convert a shcore::Value (holding a int64_t) to a
   * mysqlshdk::utils::nullable<int64_t>.
   *
   * @param value Value (with the int64_t) to convert
   * @return utils::nullable<int64_t> object corresponding to the specified
   *         Value object (with a int64_t).
   */
  static utils::nullable<int64_t> value_to_nullable_int(
      const shcore::Value &value);

  /**
   * Auxiliary function to convert a shcore::Value (holding a std::string) to a
   * mysqlshdk::utils::nullable<std::string>.
   *
   * @param value Value (with the std::string) to convert
   * @return utils::nullable<std::string> object corresponding to the specified
   *         Value object (with a std::string).
   */
  static utils::nullable<std::string> value_to_nullable_string(
      const shcore::Value &value);

  /**
   * Auxiliary function to convert a mysqlshdk::utils::nullable<T> object to
   * corresponding shcore::Value object holding the value of type T.
   *
   * @tparam T type of the base value: bool, int64_t, or std::string.
   * @param value utils::nullable<T> object to convert.
   * @return shcore::Value object corresponding to the specified
   *         utils::nullable<T> object (with a T value).
   */
  template <typename T>
  shcore::Value nullable_type_to_value(const utils::nullable<T> &value) {
    if (value.is_null()) {
      return shcore::Value::Null();
    } else {
      return shcore::Value(*value);
    }
  }

  mysql::IInstance *m_instance;
  std::unique_ptr<mysql::IInstance> m_instance_unique_ptr;
  mysql::Var_qualifier m_var_qualifier;
  mysql::Var_qualifier m_get_scope;

  // List of tuples with the change to apply. Each tuple contains the name of
  // the variable, the value to set, the variable qualifier to use, and the
  // time in ms to wait (if any, by default 0).
  std::vector<
      std::tuple<std::string, shcore::Value, mysql::Var_qualifier, uint32_t>>
      m_change_sequence;
  std::map<std::string, shcore::Value> m_global_change_tracker;
  std::map<std::string, shcore::Value> m_session_change_tracker;
};

}  // namespace config
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_CONFIG_CONFIG_SERVER_HANDLER_H_
