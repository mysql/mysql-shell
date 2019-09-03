/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/libs/mysql/repl_config.h"

#include <cassert>
#include <iomanip>
#include <limits>
#include <memory>
#include <set>

#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <mysqld_error.h>

#include "mysqlshdk/libs/config/config.h"
#include "mysqlshdk/libs/config/config_file_handler.h"
#include "mysqlshdk/libs/config/config_server_handler.h"
#include "mysqlshdk/libs/mysql/utils.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/trandom.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/libs/utils/uuid_gen.h"

namespace mysqlshdk {
namespace mysql {

/**
 * Auxiliary function that is used to validate a given invalid config against
 * a given handler and list of valid_values
 * @param values a vector with the values to the variable with
 * @param allowed_values if true, the values list is considered the list of
 *        allowed values, if false the list of forbidden values.
 * @param handler handler that is going to be used for the validation
 * @param change Invalid config struct with the name and expected values already
 *        initialized.
 * @param change_type type of change to be used for the invalid_config
 * @param restart boolean value that is true if the variable requires a restart
 *        to change and false otherwise.
 * @param set_cur_val If true, the current_val field of the invalid config
 *        will be set with the value read from the handler if it isn't
 *        initialized yet.
 *        Note: This is necessary so that the invalid config has a value on the
 *        current value field for logging even if the value is the one expected.
 */
void check_variable_compliance(const std::vector<std::string> &values,
                               bool allowed_values,
                               const config::IConfig_handler &handler,
                               Invalid_config *change, Config_type change_type,
                               bool restart, bool set_cur_val) {
  std::string value;
  // convert value to a string that we can lookup in the valid_values vector
  try {
    auto nullable_value = handler.get_string(change->var_name);
    if (nullable_value.is_null())
      value = k_no_value;
    else
      value = shcore::str_upper(*nullable_value);
  } catch (const std::out_of_range &err) {
    // variable is not defined
    value = k_value_not_set;
  }
  if (set_cur_val && change->current_val == k_must_be_initialized)
    change->current_val = value;
  // If the value is not on the list of allowed values or if it is on the list
  // of forbidden values, then the configuration is not valid.
  auto found_it = std::find(values.begin(), values.end(), value);
  if ((found_it == values.end() && allowed_values) ||
      (found_it != values.end() && !allowed_values)) {
    change->current_val = value;
    change->types.set(change_type);
    change->restart = restart;
  }
}

void check_persisted_value_compliance(
    const std::vector<std::string> &values, bool allowed_values,
    const config::Config_server_handler &srv_handler, Invalid_config *change) {
  mysqlshdk::utils::nullable<std::string> persisted_value =
      srv_handler.get_persisted_value(change->var_name);

  // Check only needed if there is a persisted value.
  if (!persisted_value.is_null()) {
    std::string value = shcore::str_upper(*persisted_value);

    if (change->current_val != value) {
      // When the persisted value is different from the current sysvar value
      // check if it is valid and take the necessary action.

      auto found_it = std::find(values.begin(), values.end(), value);
      if ((found_it == values.end() && allowed_values) ||
          (found_it != values.end() && !allowed_values)) {
        // Persisted value is invalid, thus it must to be changed:
        // if sysvar values is correct then the persisted value must be changed
        // but restart is not required, otherwise maintain the current change.
        if (!change->types.is_set(Config_type::SERVER)) {
          // Sysvar value is correct
          change->types.set(Config_type::SERVER);
          change->restart = false;
        }
      } else {
        // Persisted value is valid, thus only a restart is required.
        change->restart = true;
        change->types.unset(Config_type::SERVER);
        change->types.set(Config_type::RESTART_ONLY);
      }
    }

    // Add persisted value information when available.
    change->persisted_val = value;
  }
}

/**
 * Auxiliary function that does the logging of an invalid config.
 * @param change The Invalid config object
 */
void log_invalid_config(const Invalid_config &change) {
  if (change.types.empty()) {
    log_debug("OK: '%s' value '%s' is compatible.", change.var_name.c_str(),
              change.current_val.c_str());
  } else {
    log_debug(
        "FAIL: '%s' value '%s' is not compatible. "
        "Required value: '%s'.",
        change.var_name.c_str(), change.current_val.c_str(),
        change.required_val.c_str());
  }
}

void check_server_variables_compatibility(
    const mysqlshdk::config::Config &config, bool group_replication,
    std::vector<Invalid_config> *out_invalid_vec) {
  // create a vector for all the variables required values. Each entry is
  // a string with the name of the variable, then a vector of strings with the
  // accepted values and finally a boolean value that says if the variable
  // requires a restart to change or not.
  // NOTE: The order of the variables in the vector is important since it
  // is used by the configure operation to set the correct variable values and
  // as such it serves as a workaround for BUG#27629719, which requires
  // some GR required variables to be set in a certain order, namely
  // enforce_gtid_consistency before gtid_mode.
  std::vector<std::tuple<std::string, std::vector<std::string>, bool>>
      requirements{std::make_tuple("binlog_format",
                                   std::vector<std::string>{"ROW"}, false),
                   std::make_tuple("log_slave_updates",
                                   std::vector<std::string>{"ON", "1"}, true),
                   std::make_tuple("enforce_gtid_consistency",
                                   std::vector<std::string>{"ON", "1"}, true),
                   std::make_tuple("gtid_mode",
                                   std::vector<std::string>{"ON", "1"}, true),
                   std::make_tuple("master_info_repository",
                                   std::vector<std::string>{"TABLE"}, true),
                   std::make_tuple("relay_log_info_repository",
                                   std::vector<std::string>{"TABLE"}, true)};

  // Option checks specific to GR
  if (group_replication) {
    requirements.push_back(std::make_tuple(
        "binlog_checksum", std::vector<std::string>{"NONE"}, false));
    requirements.push_back(std::make_tuple(
        "transaction_write_set_extraction",
        std::vector<std::string>{"XXHASH64", "2", "MURMUR32", "1"}, true));
  }

  if (config.has_handler(mysqlshdk::config::k_dft_cfg_server_handler)) {
    // Add an extra requirement for the report_port
    std::string report_port = *(
        config.get_string("port", mysqlshdk::config::k_dft_cfg_server_handler));
    std::vector<std::string> report_port_vec = {report_port};
    requirements.emplace_back("report_port", report_port_vec, false);

    // Check if MTS is enabled (slave_parallel_workers > 0) and if so, add
    // extra requirements.
    utils::nullable<int64_t> slave_p_workers = config.get_int(
        "slave_parallel_workers", mysqlshdk::config::k_dft_cfg_server_handler);
    if (group_replication && !slave_p_workers.is_null() &&
        *slave_p_workers > 0) {
      std::vector<std::string> slave_parallel_vec = {"LOGICAL_CLOCK"};
      std::vector<std::string> slave_commit_vec = {"ON", "1"};
      requirements.emplace_back("slave_parallel_type", slave_parallel_vec,
                                false);
      requirements.emplace_back("slave_preserve_commit_order", slave_commit_vec,
                                false);
    }
  }

  for (auto &req : requirements) {
    std::string var_name;
    std::vector<std::string> valid_values;
    bool restart;
    std::tie(var_name, valid_values, restart) = req;
    log_debug("Checking if '%s' is compatible with InnoDB Cluster.",
              var_name.c_str());
    // assuming the expected value is the first of the valid values list
    Invalid_config change = Invalid_config(var_name, valid_values.at(0));
    // If config object has has a config handler
    if (config.has_handler(mysqlshdk::config::k_dft_cfg_file_handler)) {
      check_variable_compliance(
          valid_values, true,
          *config.get_handler(mysqlshdk::config::k_dft_cfg_file_handler),
          &change, Config_type::CONFIG, false, true);
    }

    // If config object has a server handler
    if (config.has_handler(mysqlshdk::config::k_dft_cfg_server_handler)) {
      // Get the config server handler.
      auto srv_cfg_handler =
          dynamic_cast<mysqlshdk::config::Config_server_handler *>(
              config.get_handler(mysqlshdk::config::k_dft_cfg_server_handler));

      // Determine if the config server handler supports SET PERSIST.
      bool use_persist = (srv_cfg_handler->get_default_var_qualifier() ==
                          mysqlshdk::mysql::Var_qualifier::PERSIST);

      // Check the variables compliance.
      check_variable_compliance(valid_values, true, *srv_cfg_handler, &change,
                                Config_type::SERVER, restart, true);

      // Check persisted value if supported, because it can be different from
      // the current sysvar value (when PERSIST_ONLY was used).
      if (use_persist) {
        check_persisted_value_compliance(valid_values, true, *srv_cfg_handler,
                                         &change);
      }
    }

    log_invalid_config(change);
    // if there are any changes to be made, add them to the vector of changes
    if (!change.types.empty()) out_invalid_vec->push_back(std::move(change));
  }
}

void check_server_id_compatibility(
    const mysqlshdk::mysql::IInstance &instance,
    const mysqlshdk::config::Config &config,
    std::vector<Invalid_config> *out_invalid_vec) {
  // initialize change object with default values for this specific variable
  Invalid_config change = Invalid_config("server_id", "<unique ID>");

  log_debug("Checking if 'server_id' is compatible.");
  // If config object has has a config handler
  if (config.has_handler(mysqlshdk::config::k_dft_cfg_file_handler)) {
    std::vector<std::string> forbidden_values{"0", k_no_value, k_value_not_set};
    check_variable_compliance(
        forbidden_values, false,
        *config.get_handler(mysqlshdk::config::k_dft_cfg_file_handler), &change,
        Config_type::CONFIG, false, true);
  }

  // The test for the server_id on the server_handler is special since
  // the 1 value can be both allowed or not depending on being the default
  // value or not. As such we will not make use of the check_variable_compliance
  // function.
  if (config.has_handler(mysqlshdk::config::k_dft_cfg_server_handler)) {
    auto server_id = config.get_int(
        "server_id", mysqlshdk::config::k_dft_cfg_server_handler);
    // server id cannot be null on the server, so we can read its value without
    // any problems
    if (*server_id == 0) {
      // if server_id is 0, then it is not valid for gr usage and must be
      // changed
      change.current_val = "0";
      change.types.set(Config_type::SERVER);
      change.restart = true;
      change.val_type = shcore::Value_type::Integer;
    } else if (instance.get_version() >= mysqlshdk::utils::Version(8, 0, 3) &&
               instance.has_variable_compiled_value("server_id")) {
      // Starting from MySQL 8.0.3, server_id = 1 by default (to enable
      // binary logging). For this versions we check if the default value was
      // changed by the user. Otherwise server_id is 0 (not set) by default
      // for previous server versions (it cannot be 0 for any version).
      change.current_val = std::to_string(*server_id);
      change.types.set(Config_type::SERVER);
      change.restart = true;
      change.val_type = shcore::Value_type::Integer;
    } else {
      // If no invalid config was found, store the current variable value to
      // be used for the debug message.
      if (change.types.empty()) change.current_val = std::to_string(*server_id);
    }
  }
  // if there are any changes to be made, add them to the vector of changes
  log_invalid_config(change);
  if (!change.types.empty()) out_invalid_vec->push_back(std::move(change));
}

void check_log_bin_compatibility(const mysqlshdk::mysql::IInstance &instance,
                                 const mysqlshdk::config::Config &config,
                                 std::vector<Invalid_config> *out_invalid_vec) {
  log_debug("Checking if 'log_bin' is compatible.");
  // If config object has has a config handler
  if (config.has_handler(mysqlshdk::config::k_dft_cfg_file_handler)) {
    // On MySQL 8.0.3 the binary log is enabled by default, so there is no need
    // to add the log_bin option to the config file. However on 5.7 there is.
    if (instance.get_version() < mysqlshdk::utils::Version(8, 0, 3)) {
      Invalid_config change = Invalid_config("log_bin", k_no_value);
      std::vector<std::string> forbidden_values{k_value_not_set};
      check_variable_compliance(
          forbidden_values, false,
          *config.get_handler(mysqlshdk::config::k_dft_cfg_file_handler),
          &change, Config_type::CONFIG, false, true);
      // if there are any changes to be made, add them to the vector of changes
      if (!change.types.empty()) {
        log_debug(
            "FAIL: '%s' value '%s' is not compatible. "
            "Required value: '%s'.",
            change.var_name.c_str(), change.current_val.c_str(),
            change.required_val.c_str());
        out_invalid_vec->push_back(std::move(change));
      }
    }
    // We must also check that neither of the skip-log-bin or disable-log-bin
    // options are set.
    Invalid_config change_skip =
        Invalid_config("skip_log_bin", k_value_not_set);
    Invalid_config change_disable =
        Invalid_config("disable_log_bin", k_value_not_set);
    std::vector<std::string> allowed_skip_dis_values{k_value_not_set};
    check_variable_compliance(
        allowed_skip_dis_values, true,
        *config.get_handler(mysqlshdk::config::k_dft_cfg_file_handler),
        &change_skip, Config_type::CONFIG, false, true);
    check_variable_compliance(
        allowed_skip_dis_values, true,
        *config.get_handler(mysqlshdk::config::k_dft_cfg_file_handler),
        &change_disable, Config_type::CONFIG, false, true);

    log_invalid_config(change_disable);
    // if there are any changes to be made, add them to the vector of changes
    if (!change_disable.types.empty())
      out_invalid_vec->push_back(std::move(change_disable));
    log_invalid_config(change_skip);
    if (!change_skip.types.empty())
      out_invalid_vec->push_back(std::move(change_skip));
  }

  if (config.has_handler(mysqlshdk::config::k_dft_cfg_server_handler)) {
    // create invalid_config with default values for server, which are
    // different from the ones for the config file.
    Invalid_config change = Invalid_config("log_bin", "ON");
    std::vector<std::string> valid_values{"1", "ON"};
    check_variable_compliance(
        valid_values, true,
        *config.get_handler(mysqlshdk::config::k_dft_cfg_server_handler),
        &change, Config_type::SERVER, true, true);
    // If the log_bin value is not on the valid values, then the configuration
    // is not valid.
    if (!change.types.empty()) {
      // If the configuration is not valid on the server, and no config file
      // handler was provided, we must add an invalid config to fix the
      // value on the configuration file since the value cannot cannot be
      // persisted. If the config file handler exists, we already checked
      // if an invalid config is required.
      if (!config.has_handler(mysqlshdk::config::k_dft_cfg_file_handler)) {
        out_invalid_vec->emplace_back("log_bin", k_value_not_set, k_no_value,
                                      Config_types(Config_type::CONFIG), false,
                                      shcore::Value_type::String);
      }
    }
    log_invalid_config(change);
    // if there are any changes to be made, add them to the vector of changes
    if (!change.types.empty()) out_invalid_vec->push_back(std::move(change));
  }
}

}  // namespace mysql
}  // namespace mysqlshdk
