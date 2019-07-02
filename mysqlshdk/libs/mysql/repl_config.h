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

#ifndef MYSQLSHDK_LIBS_MYSQL_REPL_CONFIG_H_
#define MYSQLSHDK_LIBS_MYSQL_REPL_CONFIG_H_

#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "mysql/instance.h"
#include "mysqlshdk/libs/config/config.h"
#include "mysqlshdk/libs/utils/nullable.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlshdk {
namespace mysql {

static constexpr char k_value_not_set[] = "<not set>";
static constexpr char k_no_value[] = "<no value>";
static constexpr char k_must_be_initialized[] = "<must be initialized>";

enum class Config_type { SERVER, CONFIG, RESTART_ONLY };

typedef mysqlshdk::utils::Enum_set<Config_type, Config_type::RESTART_ONLY>
    Config_types;

struct Invalid_config {
  std::string var_name;
  std::string current_val;
  std::string required_val;
  mysqlshdk::utils::nullable<std::string> persisted_val;
  Config_types types;
  bool restart;
  shcore::Value_type val_type;

  // constructors
  Invalid_config(std::string name, std::string req_val)
      : Invalid_config(std::move(name), k_must_be_initialized,
                       std::move(req_val), Config_types(), false,
                       shcore::Value_type::String) {}
  Invalid_config(std::string name, std::string curr_val, std::string req_val,
                 Config_types types, bool rest, shcore::Value_type val_t)
      : var_name(std::move(name)),
        current_val(std::move(curr_val)),
        required_val(std::move(req_val)),
        types(std::move(types)),
        restart(rest),
        val_type(val_t) {}
  // comparison operator to be used for sorting Invalid_config objects
  bool operator<(const Invalid_config &rhs) const {
    return var_name < rhs.var_name ||
           (var_name == rhs.var_name && current_val < rhs.current_val);
  }
};

/**
 * Checks if several of the required MySQL variables for Replication are valid.
 *
 * @param config Config object to obtain the settings to check. It can hold
 *        more than one configuration handler, for example for the server and
 *        option file (e.g., my.cnf).
 * @param group_replication if true, assumes configuration for GR and
 *        if false, for regular async replication.
 * @param out_invalid_vec pointer to vector that will be filled with the details
 *        of the invalid settings.
 */
void check_server_variables_compatibility(
    const mysqlshdk::config::Config &config, bool group_replication,
    std::vector<Invalid_config> *out_invalid_vec);

/**
 * Checks if the server_id variable is valid for Replication.
 *
 * @param instance Instance object that points to the server to be checked.
 * @param config Config object to obtain the settings to check. It can hold
 *        more than one configuration handler, for example for the server and
 *        option file (e.g., my.cnf).
 * @param out_invalid_vec pointer to vector that will be filled with the details
 *        of the invalid settings.
 */
void check_server_id_compatibility(
    const mysqlshdk::mysql::IInstance &instance,
    const mysqlshdk::config::Config &config,
    std::vector<Invalid_config> *out_invalid_vec);

/**
 * Checks if the the log_bin variable for is valid for Replication.
 *
 * @param instance Instance object that points to the server to be checked.
 * @param config Config object to obtain the settings to check. It can hold
 *        more than one configuration handler, for example for the server and
 *        option file (e.g., my.cnf).
 * @param out_invalid_vec pointer to vector that will be filled with the details
 *        of the invalid settings.
 */
void check_log_bin_compatibility(const mysqlshdk::mysql::IInstance &instance,
                                 const mysqlshdk::config::Config &config,
                                 std::vector<Invalid_config> *out_invalid_vec);

}  // namespace mysql
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_MYSQL_REPL_CONFIG_H_
