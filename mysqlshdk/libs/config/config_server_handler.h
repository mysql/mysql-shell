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
                                 mysql::Var_qualifier var_qualifier) noexcept;

  /**
   * Constructor
   *
   * @param instance shared pointer with the target Instance (server) to be
   *        handled.
   * @param var_qualifier type of variable qualifier to be used for the gets and
   * sets
   */
  explicit Config_server_handler(std::shared_ptr<mysql::IInstance> instance,
                                 mysql::Var_qualifier var_qualifier) noexcept;

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
  std::optional<bool> get_bool(const std::string &name) const override;

  /**
   * Get the string value for the specified server configuration (system
   * variable).
   *
   * @param name string with the name of the configuration to get.
   * @return nullable string with the value for the specified configuration.
   * @throw std::out_of_range if the specified configuration does not exist.
   */
  std::optional<std::string> get_string(const std::string &name) const override;

  /**
   * Get the integer value for the specified server configuration (system
   * variable).
   *
   * @param name string with the name of the configuration to get.
   * @return nullable integer with the value for the specified configuration.
   * @throw std::out_of_range if the specified configuration does not exist.
   */
  std::optional<int64_t> get_int(const std::string &name) const override;

  /**
   * Set the given server configuration (system variable) with the specified
   * value.
   *
   * @param name string with the name of the configuration to set.
   * @param value nullable boolean with the value to set.
   * @param context string with the configuration context to include in error
   *                messages if defined.
   * @throw std::invalid_argument if the specified value is null.
   */
  void set(const std::string &name, std::optional<bool> value,
           const std::string &context = "") override;

  /**
   * Set the given server configuration (system variable) with the specified
   * value.
   *
   * @param name string with the name of the configuration to set.
   * @param value nullable integer with the value to set.
   * @param context string with the configuration context to include in error
   *                messages if defined.
   * @throw std::invalid_argument if the specified value is null.
   */
  void set(const std::string &name, std::optional<int64_t> value,
           const std::string &context = "") override;

  /**
   * Set the given server configuration (system variable) with the specified
   * value.
   *
   * @param name string with the name of the configuration to set.
   * @param value nullable string with the value to set.
   * @param context string with the configuration context to include in error
   *                messages if defined.
   * @throw std::invalid_argument if the specified value is null.
   */
  void set(const std::string &name, const std::optional<std::string> &value,
           const std::string &context = "") override;

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
  void apply(bool skip_default_value_check = false) override;

  std::string get_server_uuid() const override;

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
  std::optional<bool> get_bool(const std::string &name,
                               const mysql::Var_qualifier var_qualifier) const;

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
  std::optional<std::string> get_string(
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
  std::optional<int64_t> get_int(
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
  std::optional<bool> get_bool_now(
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
  std::optional<std::string> get_string_now(
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
  std::optional<int64_t> get_int_now(
      const std::string &name, const mysql::Var_qualifier var_qualifier) const;

  /**
   * Set the given server configuration (system variable) with the specified
   * value and specified variable qualifier.
   *
   * @param name string with the name of the configuration to set.
   * @param value nullable string with the value to set.
   * @param var_qualifier Var_qualifier with the qualifier used to set the
   *        variable.
   * @param delay time to wait after actually setting the variable when
   *              executing the apply() function. By default 0, do not wait.
   *              NOTE: This allow to workaround BUG#27629719, ensuring
   *              different timestamps are set for variable that need to be set
   *              in a specific order.
   * @param context string describing the configuration being set to include in
   *                the error message if defined.
   * @throw std::invalid_argument if the specified value is null.
   *
   */
  template <typename T>
  void set(const std::string &name, const std::optional<T> &value,
           const mysql::Var_qualifier var_qualifier,
           std::chrono::milliseconds delay = std::chrono::milliseconds::zero(),
           const std::string &context = "") {
    // The value cannot be null (not supported by set_sysvar())
    if (!value)
      throw std::invalid_argument{"The value parameter cannot be null."};

    // NOTE: PERSIST_ONLY variables are only stored in the internal sequence of
    // changes to apply, but not on the map that hold a cache of the changes.
    shcore::Value val(*value);

    {
      VarData var_data;
      var_data.name = name;
      var_data.value = val;
      var_data.qualifier = var_qualifier;
      var_data.timeout = delay;
      var_data.context = context;
      m_change_sequence.emplace_back(std::move(var_data));
    }

    // Store variable changes in an internal cache (used by get()).
    if (var_qualifier == mysql::Var_qualifier::GLOBAL ||
        var_qualifier == mysql::Var_qualifier::PERSIST) {
      m_global_change_tracker[name] = std::move(val);
    } else if (var_qualifier == mysql::Var_qualifier::SESSION) {
      m_session_change_tracker[name] = std::move(val);
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

  /**
   * Get the persisted value for the specified server configuration (system
   * variable).
   *
   * NOTE: This function gets the value directly from the server (i.e.,
   * Performance Schema 'persisted_variables' table).
   *
   * @param name string with the name of the configuration to get.
   * @return nullable string with the value for the specified configuration,
   *         the value is null if no value was found.
   * @throw std::runtime_error if an error occurs trying to get the persisted
   *        value (e.g., for 5.7 servers that do not support SET PERSIST).
   */
  std::optional<std::string> get_persisted_value(const std::string &name) const;

  /**
   * Restores all variables values.
   *
   * This method only has any effect if called after apply().
   */
  void undo_changes();

  /**
   * Removes all variables from the undo list.
   *
   * This essentially disables the undo method: after calling this method,
   * calling undo_changes() is a no-op.
   */
  void remove_all_from_undo();

  /**
   * Removes a variable from the undo list.
   *
   * @param name name of the variable to remove from the undo list
   */
  void remove_from_undo(const std::string &name);

 private:
  template <typename T>
  void add_undo_change(const std::string &name,
                       const mysql::Var_qualifier var_qualifier) {
    static_assert(std::is_same_v<T, bool> || std::is_same_v<T, int64_t> ||
                  std::is_same_v<T, std::string>);

    if (std::find_if(m_change_undo_sequence.begin(),
                     m_change_undo_sequence.end(),
                     [&name](const auto &var_data) {
                       return (var_data.name == name);
                     }) != m_change_undo_sequence.end())
      return;

    std::optional<T> value;
    if (var_qualifier != mysql::Var_qualifier::PERSIST_ONLY) {
      auto get_qualifier = var_qualifier;
      if (get_qualifier == mysql::Var_qualifier::PERSIST)
        get_qualifier = mysql::Var_qualifier::GLOBAL;

      if constexpr (std::is_same_v<T, bool>) {
        try {
          value = get_bool_now(name, get_qualifier);
        } catch (const std::exception &) {
          return;  // just ignore it
        }

      } else if constexpr (std::is_same_v<T, int64_t>) {
        try {
          value = get_int_now(name, get_qualifier);
        } catch (const std::exception &) {
          return;  // just ignore it
        }
      } else {
        try {
          value = get_string_now(name, get_qualifier);
        } catch (const std::exception &) {
          return;  // just ignore it
        }
      }
    }

    VarData var_data;
    var_data.name = name;
    var_data.value =
        value.has_value() ? shcore::Value{*value} : shcore::Value::Null();
    var_data.qualifier = var_qualifier;
    m_change_undo_sequence.emplace_back(std::move(var_data));
  }

 private:
  mysql::IInstance *m_instance;
  std::shared_ptr<mysql::IInstance> m_instance_shared_ptr;
  mysql::Var_qualifier m_var_qualifier;
  mysql::Var_qualifier m_get_scope;

  struct VarData {
    std::string name;
    shcore::Value value;
    mysql::Var_qualifier qualifier;
    std::chrono::milliseconds timeout;
    std::string context;
  };

  // List of the changes to apply.
  std::vector<VarData> m_change_sequence;

  // List of the changes to undo
  std::vector<VarData> m_change_undo_sequence;
  std::set<std::string> m_change_undo_ignore;

  std::map<std::string, shcore::Value> m_global_change_tracker;
  std::map<std::string, shcore::Value> m_session_change_tracker;
};

}  // namespace config
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_CONFIG_CONFIG_SERVER_HANDLER_H_
