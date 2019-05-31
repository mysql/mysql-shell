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

#ifndef MYSQLSHDK_LIBS_MYSQL_PLUGIN_H_
#define MYSQLSHDK_LIBS_MYSQL_PLUGIN_H_

#include <string>
#include "mysql/instance.h"
#include "mysqlshdk/libs/config/config.h"

namespace mysqlshdk {
namespace mysql {

/**
 * Check if the target plugin is installed, and if not try to
 * install it. The option file with the instance configuration is updated
 * accordingly if provided.
 *
 * @param plugin plugin identifier
 * @param instance session object to connect to the target instance.
 * @paran config instance configuration file to be updated
 *
 * @throw std::runtime_error if the target plugin is DISABLED (cannot be
 * installed online) or if an error occurs installing the plugin.
 *
 * @return A boolean value indicating if the target plugin was installed (true)
 * or if it is already installed and ACTIVE (false).
 */
bool install_plugin(const std::string &plugin,
                    const mysqlshdk::mysql::IInstance &instance,
                    mysqlshdk::config::Config *config);

/**
 * Check if the target plugin plugin is installed and uninstall it if
 * needed. The option file with the instance configuration is updated
 * accordingly if provided (plugin disabled).
 *
 * @param plugin plugin identifier
 * @param instance session object to connect to the target instance.
 * @paran config instance configuration file to be updated
 *
 * @throw std::runtime_error if an error occurs uninstalling the target plugin
 *
 * @return A boolean value indicating if the target plugin was uninstalled
 * (true) or if it is already not available (false).
 */
bool uninstall_plugin(const std::string &plugin,
                      const mysqlshdk::mysql::IInstance &instance,
                      mysqlshdk::config::Config *config);

}  // namespace mysql
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_MYSQL_PLUGIN_H_
