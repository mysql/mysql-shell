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

#ifndef MYSQLSHDK_LIBS_CONFIG_CONFIG_H_
#define MYSQLSHDK_LIBS_CONFIG_CONFIG_H_

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "mysqlshdk/libs/utils/nullable.h"

namespace mysqlshdk {
namespace config {

static constexpr char k_dft_cfg_server_handler[] = "config_server";
static constexpr char k_dft_cfg_file_handler[] = "config_file";

/**
 * Interface to be implemented by any configuration handler.
 *
 * In a nutshell, each handler implementing this interface must be able to get
 * and set configurations, tracking changes internally and only applying them
 * to the handled resource (e.g., option file or server) when the apply()
 * function if executed.
 *
 * This design enables changes to be effectively applied only when explicitly
 * requested (for example at the end of an operation), avoiding changing the
 * state of a resource directly along the execution of an operation. This is
 * especially useful to prevent leaving a resource in an inconsistent state
 * in the case of errors, due to unnecessary premature changes to
 * configurations that could have been all done at once at the end of an
 * operation.
 */
class IConfig_handler {
 public:
  virtual ~IConfig_handler() {}

  virtual utils::nullable<bool> get_bool(const std::string &name) const = 0;
  virtual utils::nullable<std::string> get_string(
      const std::string &name) const = 0;
  virtual utils::nullable<int64_t> get_int(const std::string &name) const = 0;
  virtual void set(const std::string &name,
                   const utils::nullable<bool> &value) = 0;
  virtual void set(const std::string &name,
                   const utils::nullable<int64_t> &value) = 0;
  virtual void set(const std::string &name,
                   const utils::nullable<std::string> &value) = 0;
  virtual void apply() = 0;
};

/**
 * Generic configuration object able to manage several config handlers.
 *
 * This object allows to simultaneously manage configurations from different
 * resources (e.g., options in a my.cnf file or system variables of an online
 * server). Each resources being specifically managed by a configuration
 * handler implementing the IConfig_handler interface.
 *
 * It allows configurations with the same name (value type and behaviour) to be
 * changed simply by a single function for all configuration handlers
 * (resources), but it also allows each configuration handler to be managed
 * individually in a custom way. This later feature is required because some
 * MySQL configuration do not behave the same way on different resources, like
 * for example 'log_bin' (which can have a string with a base name value or can
 * be a boolean if it has no value for option files, but it is a read-only
 * server variable that in reality corresponds to two server variables: log_bin
 * and login_base_name).
 */
class Config : public IConfig_handler {
 public:
  /**
   * Constructor
   */
  Config() = default;

  /**
   * Get the boolean value for the specified configuration from the default
   * configuration handler.
   *
   * @param name string with the name of the configuration to get.
   * @return nullable boolean with the value for the specified configuration
   *         (from the default configuration handler).
   */
  utils::nullable<bool> get_bool(const std::string &name) const override;

  /**
   * Get the string value for the specified configuration from the default
   * configuration handler.
   *
   * @param name string with the name of the configuration to get.
   * @return nullable string with the value for the specified configuration
   *         (from the default configuration handler).
   */
  utils::nullable<std::string> get_string(
      const std::string &name) const override;

  /**
   * Get the integer value for the specified configuration from the default
   * configuration handler.
   *
   * @param name string with the name of the configuration to get.
   * @return nullable integer with the value for the specified configuration
   *         (from the default configuration handler).
   */
  utils::nullable<int64_t> get_int(const std::string &name) const override;

  /**
   * Set the given configuration with the specified value (on all registered
   * configuration handlers).
   *
   * @param name string with the name of the configuration to set.
   * @param value nullable boolean with the value to set.
   */
  void set(const std::string &name,
           const utils::nullable<bool> &value) override;

  /**
   * Set the given configuration with the specified value (on all registered
   * configuration handlers).
   *
   * @param name string with the name of the configuration to set.
   * @param value nullable integer with the value to set.
   */
  void set(const std::string &name,
           const utils::nullable<int64_t> &value) override;

  /**
   * Set the given configuration with the specified value (on all registered
   * configuration handlers).
   *
   * @param name string with the name of the configuration to set.
   * @param value nullable string with the value to set.
   */
  void set(const std::string &name,
           const utils::nullable<std::string> &value) override;

  /**
   * Effectively apply the configuration changes (on all registered
   * configuration handlers).
   *
   * All changes performed using the set() function are only registered
   * internally on the configuration handlers but not applied to the target
   * resource assigned to each handler. This function instructs all handlers to
   * effectively apply all recorded changes to their target resources (e.g.,
   * option file or serve instance).
   */
  void apply() override;

  /**
   * Verify if the specified configuration handler exists (is registered).
   *
   * @param handler_name string with the name of the configuration handler to
   *        check.
   * @return boolean, true if the configuration handler exists, otherwise false
   */
  bool has_handler(const std::string &handler_name) const;

  /**
   * Add (register) a new configuration handler.
   *
   * NOTE: The first handler to be added will be assumed to be the default
   *       handler.
   *
   * @param handler_name string with the name of the handler to add.
   * @param handler unique pointer with the handler object to add. The handler
   *                object object must implement the IConfig_handler interface.
   *                Note: the ownership of the given handler object will be
   *                changed to the internal state of the Config object.
   * @return boolean, true if the handler was added (does not exist), false if
   *         it already exists.
   */
  bool add_handler(const std::string &handler_name,
                   std::unique_ptr<IConfig_handler> handler);

  /**
   * Remove (unregister) the specified configuration handler.
   *
   * @param handler_name string with the name of the handler to remove.
   * @throw std::out_of_range if the specified handler does not exist.
   * @throw std::runtime_error if the specified handler is the default one.
   */
  void remove_handler(const std::string &handler_name);

  /**
   * Remove all configuration handlers (including the default one).
   */
  void clear_handlers();

  /**
   * Set (change) the default configuration handler.
   *
   * @param handler_name string with the name of the handler to set as default.
   * @throw std::out_of_range if the specified handler does not exist.
   */
  void set_default_handler(const std::string &handler_name);

  /**
   * Get the boolean value for the specified configuration from the given
   * configuration handler.
   *
   * @param name string with the name of the configuration to get.
   * @param handler_name string with the name of the target handler.
   * @return nullable boolean with the value for the specified configuration
   *         (from the specified configuration handler).
   * @throw std::out_of_range if the specified handler does not exist.
   */
  utils::nullable<bool> get_bool(const std::string &name,
                                 const std::string &handler_name) const;

  /**
   * Get the string value for the specified configuration from the given
   * configuration handler.
   *
   * @param name string with the name of the configuration to get.
   * @param handler_name string with the name of the target handler.
   * @return nullable string with the value for the specified configuration
   *         (from the specified configuration handler).
   * @throw std::out_of_range if the specified handler does not exist.
   */
  utils::nullable<std::string> get_string(
      const std::string &name, const std::string &handler_name) const;

  /**
   * Get the integer value for the specified configuration from the given
   * configuration handler.
   *
   * @param name string with the name of the configuration to get.
   * @param handler_name string with the name of the target handler.
   * @return nullable integer with the value for the specified configuration
   *         (from the specified configuration handler).
   * @throw std::out_of_range if the specified handler does not exist.
   */
  utils::nullable<int64_t> get_int(const std::string &name,
                                   const std::string &handler_name) const;

  /**
   * Set the given configuration with the specified value on a specific
   * configuration handlers.
   *
   * @param name string with the name of the configuration to set.
   * @param value nullable boolean with the value to set.
   * @param handler_name string with the name of the target handler.
   * @throw std::out_of_range if the specified handler does not exist.
   */
  void set(const std::string &name, const utils::nullable<bool> &value,
           const std::string &handler_name);

  /**
   * Set the given configuration with the specified value on a specific
   * configuration handlers.
   *
   * @param name string with the name of the configuration to set.
   * @param value nullable integer with the value to set.
   * @param handler_name string with the name of the target handler.
   * @throw std::out_of_range if the specified handler does not exist.
   */
  void set(const std::string &name, const utils::nullable<int64_t> &value,
           const std::string &handler_name);

  /**
   * Set the given configuration with the specified value on a specific
   * configuration handlers.
   *
   * @param name string with the name of the configuration to set.
   * @param value nullable string with the value to set.
   * @param handler_name string with the name of the target handler.
   * @throw std::out_of_range if the specified handler does not exist.
   */
  void set(const std::string &name, const utils::nullable<std::string> &value,
           const std::string &handler_name);

  /**
   * Get a pointer to a specified configuration handler.
   *
   * This can be useful in specific situations (for variables with some
   * peculiarities, e.g. log_bin), allowing to execute functions specific to
   * an handler from the returned pointer. This function is expected to be used
   * only on those exceptional situations.
   *
   * NOTE: This function keeps the ownership of each configuration handler
   *       on the Config object, only "borrowing" the pointer for the execution
   *       of some function specific to the returned handler (not covered by the
   *       Config interface).
   *
   * @param handler_name string with the name of the target handler.
   * @return a pointer to the configuration handler hold by the Config object
   *         (without any ownership transfer).
   * @throw std::out_of_range if the specified handler does not exist.
   */
  IConfig_handler *get_handler(const std::string &handler_name) const;

  /**
   * Get the list of names of all configuration handlers.
   *
   * @return vector with the names (string) of all configuration handlers.
   */
  std::vector<std::string> list_handler_names() const;

 private:
  // Map holding all configuration handlers.
  std::map<std::string, std::unique_ptr<IConfig_handler>> m_config_handlers;

  // String with the name of the default configuration handler.
  std::string m_default_handler;
};

}  // namespace config
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_CONFIG_CONFIG_H_
