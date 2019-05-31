/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include <string>

#include "mysqlshdk/libs/config/config.h"
#include "mysqlshdk/libs/config/config_file_handler.h"
#include "mysqlshdk/libs/mysql/plugin.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace mysql {

bool install_plugin(const std::string &plugin,
                    const mysqlshdk::mysql::IInstance &instance,
                    mysqlshdk::config::Config *config) {
  // Get plugin state.
  utils::nullable<std::string> plugin_state =
      instance.get_plugin_status(plugin);

  // Get the current value of super_read_only
  // TODO(.) Move out the super_read_only handling to the caller
  utils::nullable<bool> current_super_read_only = instance.get_sysvar_bool(
      "super_read_only", mysqlshdk::mysql::Var_qualifier::GLOBAL);

  // Install the plugin if no state info is available (not installed).
  bool res = false;
  if (plugin_state.is_null()) {
    // Disable super_read_only if needed
    if (!current_super_read_only.is_null() && *current_super_read_only) {
      instance.set_sysvar("super_read_only", false,
                          mysqlshdk::mysql::Var_qualifier::GLOBAL);
    }

    // Install the plugin.
    instance.install_plugin(plugin);
    res = true;

    // Re-enable super_read_only if needed.
    if (!current_super_read_only.is_null() && *current_super_read_only) {
      instance.set_sysvar("super_read_only", *current_super_read_only,
                          mysqlshdk::mysql::Var_qualifier::GLOBAL);
    }

    // Check the plugin state after installation;
    plugin_state = instance.get_plugin_status(plugin);
  } else if ((*plugin_state).compare("ACTIVE") == 0) {
    // plugin is already active.
    return false;
  }

  if (!plugin_state.is_null() && (*plugin_state).compare("DISABLED") == 0) {
    // If the plugin is disabled then try to activate and install it, but it
    // can only be done if the option file is available.
    if (config &&
        config->has_handler(mysqlshdk::config::k_dft_cfg_file_handler)) {
      // Enable the plugin on the configuration file.
      auto cfg_file_handler =
          dynamic_cast<mysqlshdk::config::Config_file_handler *>(
              config->get_handler(mysqlshdk::config::k_dft_cfg_file_handler));
      auto clone_plugin_status = cfg_file_handler->get_string(plugin);
      cfg_file_handler->set_now(plugin, utils::nullable<std::string>("ON"));

      try {
        // Disable super_read_only if needed
        utils::nullable<bool> read_only;
        if (!read_only.is_null() && *read_only) {
          instance.set_sysvar("super_read_only", false,
                              mysqlshdk::mysql::Var_qualifier::GLOBAL);
        }

        // Uninstall the plugin so it can be installed again after being
        // enabled on the option file.
        instance.uninstall_plugin(plugin);

        // Reinstall the plugin.
        instance.install_plugin(plugin);

        // Re-enable super_read_only if needed.
        if (!read_only.is_null() && *read_only) {
          instance.set_sysvar("super_read_only", *read_only,
                              mysqlshdk::mysql::Var_qualifier::GLOBAL);
        }

        // Check the plugin state after the attempting to activate it;
        plugin_state = instance.get_plugin_status(plugin);
      } catch (const std::exception &err) {
        // restore previous value on configuration file
        cfg_file_handler->set_now(plugin, clone_plugin_status);
        throw;
      }
    }
  }

  if (!plugin_state.is_null()) {
    // Raise an exception if the plugin is not active (disabled, deleted or
    // inactive), cannot be enabled online.
    if ((*plugin_state).compare("ACTIVE") != 0) {
      std::string enable_error =
          plugin +
          " plugin is %s and cannot be enabled on runtime. Please enable "
          "the plugin and restart the server.";
      throw std::runtime_error(
          shcore::str_format(enable_error.c_str(), plugin_state->c_str()));
    }
  }

  // Plugin installed and not disabled (active).
  return res;
}

bool uninstall_plugin(const std::string &plugin,
                      const mysqlshdk::mysql::IInstance &instance,
                      mysqlshdk::config::Config *config) {
  // Get plugin state.
  utils::nullable<std::string> plugin_state =
      instance.get_plugin_status(plugin);

  // Get the current value of super_read_only
  utils::nullable<bool> current_super_read_only = instance.get_sysvar_bool(
      "super_read_only", mysqlshdk::mysql::Var_qualifier::GLOBAL);

  if (!plugin_state.is_null()) {
    if (!current_super_read_only.is_null() && *current_super_read_only) {
      instance.set_sysvar("super_read_only", false,
                          mysqlshdk::mysql::Var_qualifier::GLOBAL);
    }

    // Uninstall the plugin if state info is available (installed).
    instance.uninstall_plugin(plugin);

    // Re-enable super_read_only if needed.
    if (!current_super_read_only.is_null() && *current_super_read_only) {
      instance.set_sysvar("super_read_only", *current_super_read_only,
                          mysqlshdk::mysql::Var_qualifier::GLOBAL);
    }

    // If the config file handler is available disable the plugin.
    if (config &&
        config->has_handler(mysqlshdk::config::k_dft_cfg_file_handler)) {
      mysqlshdk::config::Config_file_handler *cfg_file_handler =
          static_cast<mysqlshdk::config::Config_file_handler *>(
              config->get_handler(mysqlshdk::config::k_dft_cfg_file_handler));
      cfg_file_handler->set_now(plugin, utils::nullable<std::string>("OFF"));
    }
    return true;
  } else {
    // plugin is not installed.
    return false;
  }
}

}  // namespace mysql
}  // namespace mysqlshdk
