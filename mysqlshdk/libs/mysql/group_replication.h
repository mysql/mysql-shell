/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_LIBS_MYSQL_GROUP_REPLICATION_H_
#define MYSQLSHDK_LIBS_MYSQL_GROUP_REPLICATION_H_

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

#ifdef _WIN32
#undef ERROR
#endif

namespace mysqlshdk {
// TODO(.) namespace gr should probably be renamed to mysql
namespace gr {

static constexpr char kPluginName[] = "group_replication";
static constexpr char kPluginActive[] = "ACTIVE";
static constexpr char kPluginDisabled[] = "DISABLED";
static constexpr char k_value_not_set[] = "<not set>";
static constexpr char k_no_value[] = "<no value>";
static constexpr char k_must_be_initialized[] = "<must be initialized>";

/**
 * Enumeration of the supported states for Group Replication members.
 */
enum class Member_state {
  ONLINE,
  RECOVERING,
  OFFLINE,
  ERROR,
  UNREACHABLE,
  MISSING
};

enum class Member_role { PRIMARY, SECONDARY };

std::string to_string(const Member_state state);
Member_state to_member_state(const std::string &state);

std::string to_string(const Member_role role);
Member_role to_member_role(const std::string &role);

/**
 * Data structure representing a Group Replication member.
 */
struct Member {
  // Address of the member
  std::string host;
  // GR port of the member
  int gr_port;
  // member_id aka server_uuid of the member
  std::string uuid;
  // State of the member
  Member_state state = Member_state::MISSING;
  // Role of the member (primary vs secondary)
  Member_role role;
};

enum class Config_type {
  SERVER,
  CONFIG,
};

typedef mysqlshdk::utils::Enum_set<Config_type, Config_type::CONFIG>
    Config_types;

struct Invalid_config {
  std::string var_name;
  std::string current_val;
  std::string required_val;
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
    return var_name < rhs.var_name;
  }
};

// Function to check membership and state.
bool is_member(const mysqlshdk::mysql::IInstance &instance);
bool is_member(const mysqlshdk::mysql::IInstance &instance,
               const std::string &group_name);
Member_state get_member_state(const mysqlshdk::mysql::IInstance &instance);
std::vector<Member> get_members(const mysqlshdk::mysql::IInstance &instance,
                                bool *out_single_primary_mode = nullptr);

bool is_primary(const mysqlshdk::mysql::IInstance &instance);

bool has_quorum(const mysqlshdk::mysql::IInstance &instance,
                int *out_unreachable, int *out_total);

// Fetch various basic info bits from the group the given instance is member of
bool get_group_information(const mysqlshdk::mysql::IInstance &instance,
                           Member_state *out_member_state,
                           std::string *out_member_id,
                           std::string *out_group_name,
                           bool *out_single_primary_mode);

std::string get_group_primary_uuid(const std::shared_ptr<db::ISession> &session,
                                   bool *out_single_primary_mode);

/**
 *
 * @param instance
 * @return map with all the group_replication variables and respective values
 * found on the provided instance.
 */
std::map<std::string, utils::nullable<std::string>> get_all_configurations(
    const mysqlshdk::mysql::IInstance &instance);

// Function to do a change master (set the GR recovery user)
void do_change_master(const mysqlshdk::mysql::IInstance &instance,
                      const std::string &rpl_user, const std::string &rpl_pwd);

// Functions to manage the GR plugin
bool install_plugin(const mysqlshdk::mysql::IInstance &instance,
                    mysqlshdk::config::Config *config);
bool uninstall_plugin(const mysqlshdk::mysql::IInstance &instance,
                      mysqlshdk::config::Config *config);
void start_group_replication(const mysqlshdk::mysql::IInstance &instance,
                             const bool bootstrap,
                             const uint16_t read_only_timeout = 900);
void stop_group_replication(const mysqlshdk::mysql::IInstance &instance);

std::string generate_group_name();

// Function to manage the replication (recovery) user for GR.
mysql::User_privileges_result check_replication_user(
    const mysqlshdk::mysql::IInstance &instance, const std::string &user,
    const std::string &host);

void create_replication_random_user_pass(
    const mysqlshdk::mysql::IInstance &instance, std::string *out_user,
    const std::vector<std::string> &hosts, std::string *out_pwd);

void create_replication_user_random_pass(
    const mysqlshdk::mysql::IInstance &instance, const std::string &user,
    const std::vector<std::string> &hosts, std::string *out_pwd);

std::string get_recovery_user(const mysqlshdk::mysql::IInstance &instance);

// Function to check compliance to use GR.
std::map<std::string, std::string> check_data_compliance(
    const mysqlshdk::mysql::IInstance &instance, const uint16_t max_errors = 0);

/**
 * Checks if several of the required MySQL variables for GR are valid.
 *
 * @param config Config object to obtain the settings to check. It can hold
 *        more than one configuration handler, for example for the server and
 *        option file (e.g., my.cnf).
 * @param out_invalid_vec pointer to vector that will be filled with the details
 *        of the invalid settings.
 */
void check_server_variables_compatibility(
    const mysqlshdk::config::Config &config,
    std::vector<Invalid_config> *out_invalid_vec);

/**
 * Checks if the server_id variable is valid for Group Replication.
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
 * Checks if the the log_bin variable for is valid for Group Replication.
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

/**
 * Checks if the thread for a delayed initialization of the group replication is
 * currently running on the given instance.
 *
 * @param instance Instance to be checked.
 *
 * @return True if group replication is currently being initialized.
 */
bool is_group_replication_delayed_starting(
    const mysqlshdk::mysql::IInstance &instance);

}  // namespace gr
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_MYSQL_GROUP_REPLICATION_H_
