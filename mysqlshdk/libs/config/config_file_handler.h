/*
 * Copyright (c) 2018, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_LIBS_CONFIG_CONFIG_FILE_HANDLER_H_
#define MYSQLSHDK_LIBS_CONFIG_CONFIG_FILE_HANDLER_H_

#include <memory>
#include <string>
#include "mysqlshdk/libs/config/config.h"
#include "mysqlshdk/libs/config/config_file.h"

namespace mysqlshdk {
namespace config {

/**
 * Object to manage (handle) MySQL option file configurations.
 *
 * This object allows to manage the option file of a local MySQL server,
 * implementing the IConfig_handler interface (so
 * that it can be used by the generic mysqlshdk::config::Config object).
 */
class Config_file_handler : public IConfig_handler {
 public:
  /**
   * Constructor
   *
   * @param input_config_path path to the option file to be read.
   * @param output_cnf_path option path to the configuration file where any
   * changes will be saved by the apply method. This path can be the same
   * as the input_config_path.
   */
  Config_file_handler(const std::string &input_config_path,
                      const std::string &output_cnf_path);

  /**
   * Constructor
   *
   * IMPORTANT NOTE: no configurations are read from the specified file even
   * if it exists, therefore any existing configurations will be overwritten,
   * assuming the file is empty.
   *
   * @param output_cnf_path option path to the option file where any
   * changes will be saved by the apply method.
   */
  explicit Config_file_handler(const std::string &output_cnf_path);

  /**
   * Get the boolean value for the specified option in the mysqld group.
   *
   * @param name string with the name of the configuration to get.
   * @return nullable boolean with the value for the specified option.
   * @throw std::out_of_range if the specified option does not exist in the
   * option file in the mysqld group.
   */
  utils::nullable<bool> get_bool(const std::string &name) const override;
  /**
   * Get the string value for the specified option in the mysqld group.
   *
   * @param name string with the name of the configuration to get.
   * @return nullable string with the value for the specified option.
   * @throw std::out_of_range if the specified option does not exist in the
   * option file in the mysqld group.
   */
  utils::nullable<std::string> get_string(
      const std::string &name) const override;
  /**
   * Get the integer value for the specified option in the mysqld group.
   *
   * @param name string with the name of the configuration to get.
   * @return nullable integer with the value for the specified option.
   * @throw std::out_of_range if the specified option does not exist in the
   * option file in the mysqld group.
   */
  utils::nullable<int64_t> get_int(const std::string &name) const override;

  /**
   * Set the specified option with the specified value in the mysqld group.
   *
   * @param name string with the name of the configuration to get.
   * @param value nullable boolean with the value to set.
   * @param context string with the configuration context to include in error
   *                messages if defined.
   */
  void set(const std::string &name, const utils::nullable<bool> &value,
           const std::string &context = "") override;

  /**
   * Set the specified option with the specified value in the mysqld group.
   *
   * @param name string with the name of the configuration to get.
   * @param value nullable integer with the value to set.
   * @param context string with the configuration context to include in error
   *                messages if defined.
   */
  void set(const std::string &name, const utils::nullable<int64_t> &value,
           const std::string &context = "") override;

  /**
   * Set the specified option with the specified value in the mysqld group.
   *
   * @param name string with the name of the configuration to get.
   * @param value nullable string with the value to set.
   * @param context string with the configuration context to include in error
   *                messages if defined.
   */
  void set(const std::string &name, const utils::nullable<std::string> &value,
           const std::string &context = "") override;
  /**
   * Effectively apply all the configuration file changes.
   *
   * All changes performed using the set() function are only registered
   * internally without (immediately) applying them (i.e., not directly changing
   * the option file).
   * This function applies all recorded changes to the option file specified in
   * in the output_cnf_path parameter of the constructor.
   *
   * @throw std::ios::failure if any error occurs regarding access to the
   * permission access to the option file(s).
   *        std::runtime_error if any runtime exception happens while writing
   *        to the option file(s).
   */
  void apply() override;

  /**
   * Remove the specified option from the mysqld group.
   *
   * If the option exists and it is successfully removed true is returned,
   * otherwise false. However, if the specified group does not exists an
   * exception will be raised.
   *
   * @param name string with the name of the option to remove.
   * @return boolean, true if the option exists and is removed, otherwise false.
   * @throw out_of_range exception if the mysqld group does not exist.
   */
  bool remove(const std::string &name);

  /**
   * Set the specified option with the specified value immediately on the
   * option file (applying it) for the mysqld group.
   *
   * @param name string with the name of the configuration to set.
   * @return nullable boolean with the value for the specified option.
   */
  void set_now(const std::string &name, const utils::nullable<bool> &value);

  /**
   * Set the specified option with the specified value immediately on the
   * option file (applying it) for the mysqld group.
   *
   * @param name string with the name of the configuration to set.
   * @return nullable integer with the value for the specified option.
   */
  void set_now(const std::string &name, const utils::nullable<int64_t> &value);

  /**
   * Set the specified option with the specified value immediately on the
   * option file (applying it) for the mysqld group.
   *
   * @param name string with the name of the configuration to set.
   * @return nullable string with the value for the specified option.
   */
  void set_now(const std::string &name,
               const utils::nullable<std::string> &value);

  // setter for the m_output_cnf_path
  void set_output_config_path(const std::string &m_output_config_path);

 private:
  std::string m_input_config_path;
  std::string m_output_config_path;
  Config_file m_config_file;
  std::string m_group{
      "mysqld"};  // the handler will only change settings in the mysqld group
};
}  // namespace config
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_CONFIG_CONFIG_FILE_HANDLER_H_
