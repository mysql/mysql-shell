/*
 * Copyright (c) 2021, 2022, Oracle and/or its affiliates.
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
#include "modules/adminapi/cluster/api_options.h"

#include <cinttypes>
#include <utility>
#include <vector>

#include "modules/adminapi/common/common.h"
#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/db/utils_connection.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "shellcore/shell_options.h"

namespace mysqlsh {
namespace dba {
namespace cluster {

const shcore::Option_pack_def<Add_instance_options>
    &Add_instance_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Add_instance_options>()
          .include(&Add_instance_options::gr_options)
          .include(&Add_instance_options::clone_options)
          .optional(kLabel, &Add_instance_options::label)
          .optional(kWaitRecovery, &Add_instance_options::set_wait_recovery)
          .include<Password_interactive_options>();
  return opts;
}

void Add_instance_options::set_wait_recovery(int value) {
  // Validate waitRecovery option UInteger [0, 3]
  if (value < 0 || value > 3) {
    throw shcore::Exception::argument_error(
        shcore::str_format("Invalid value '%d' for option '%s'. It must be an "
                           "integer in the range [0, 3].",
                           value, kWaitRecovery));
  }

  wait_recovery = value;
}

const shcore::Option_pack_def<Rejoin_instance_options>
    &Rejoin_instance_options::options() {
  static const auto opts = shcore::Option_pack_def<Rejoin_instance_options>()
                               .include(&Rejoin_instance_options::gr_options)
                               .include<Password_interactive_options>();

  return opts;
}

const shcore::Option_pack_def<Remove_instance_options>
    &Remove_instance_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Remove_instance_options>()
          .optional(kForce, &Remove_instance_options::force)
          .include<Password_interactive_options>();

  return opts;
}

const shcore::Option_pack_def<Status_options> &Status_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Status_options>()
          .optional(kExtended, &Status_options::set_extended)
          .optional(kQueryMembers, &Status_options::set_query_members);

  return opts;
}

void Status_options::set_extended(uint64_t value) {
  // Validate extended option UInteger [0, 3] or Boolean.
  if (value > 3) {
    throw shcore::Exception::argument_error(
        shcore::str_format("Invalid value '%" PRIu64
                           "' for option '%s'. It must be an integer in the "
                           "range [0, 3].",
                           value, kExtended));
  }

  extended = value;
}

void Status_options::set_query_members(bool value) {
  auto console = mysqlsh::current_console();

  std::string specific_value = value ? " with value 3" : "";
  console->print_warning(
      shcore::str_format("The '%s' option is deprecated. "
                         "Please use the 'extended' option%s instead.",
                         kQueryMembers, specific_value.c_str()));

  console->print_info();

  if (value) {
    extended = 3;
  }
}

const shcore::Option_pack_def<Options_options> &Options_options::options() {
  static const auto opts = shcore::Option_pack_def<Options_options>().optional(
      kAll, &Options_options::all);

  return opts;
}

const shcore::Option_pack_def<Rescan_options> &Rescan_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Rescan_options>()
          .include<Interactive_option>()
          .optional(kUpdateTopologyMode,
                    &Rescan_options::set_update_topology_mode)
          .optional(kAddInstances, &Rescan_options::set_list_option)
          .optional(kRemoveInstances, &Rescan_options::set_list_option)
          .optional(kUpgradeCommProtocol,
                    &Rescan_options::upgrade_comm_protocol)
          .optional(kUpdateViewChangeUuid,
                    &Rescan_options::update_view_change_uuid);

  return opts;
}

void Rescan_options::set_update_topology_mode(bool /*value*/) {
  auto console = mysqlsh::current_console();
  console->print_info(shcore::str_format(
      "The %s option is deprecated. The topology-mode is now automatically "
      "updated.",
      kUpdateTopologyMode));
  console->print_info();
}

void Rescan_options::set_list_option(const std::string &option,
                                     const shcore::Value &value) {
  std::vector<mysqlshdk::db::Connection_options> *instances_list;

  // Selects the target list
  instances_list =
      option == kRemoveInstances ? &remove_instances_list : &add_instances_list;

  bool *auto_option = option == kRemoveInstances ? &auto_remove : &auto_add;

  if (value.type == shcore::Value_type::String) {
    auto str_val = value.as_string();
    if (shcore::str_caseeq(str_val, "auto")) {
      *auto_option = true;
    } else {
      throw shcore::Exception::argument_error(shcore::str_format(
          "Option '%s' only accepts 'auto' as a valid string "
          "value, otherwise a list of instances is expected.",
          option.c_str()));
    }
  } else if (value.type == shcore::Value_type::Array) {
    auto instances_array = value.as_array();
    if (instances_array->empty()) {
      throw shcore::Exception::argument_error(shcore::str_format(
          "The list for '%s' option cannot be empty.", option.c_str()));
    }

    // Process values from addInstances list (must be valid connection data).
    for (const shcore::Value &instance : *instances_array.get()) {
      try {
        auto cnx_opt = mysqlsh::get_connection_options(instance);

        if (cnx_opt.get_host().empty()) {
          throw shcore::Exception::argument_error("host cannot be empty.");
        }

        if (!cnx_opt.has_port()) {
          throw shcore::Exception::argument_error("port is missing.");
        }

        instances_list->emplace_back(std::move(cnx_opt));
      } catch (const std::exception &err) {
        std::string error(err.what());
        throw shcore::Exception::argument_error(shcore::str_format(
            "Invalid value '%s' for '%s' option: %s", instance.descr().c_str(),
            option.c_str(), error.c_str()));
      }
    }
  } else {
    throw shcore::Exception::argument_error(shcore::str_format(
        "The '%s' option must be a string or a list of strings.",
        option.c_str()));
  }
}

const shcore::Option_pack_def<Set_primary_instance_options>
    &Set_primary_instance_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Set_primary_instance_options>().optional(
          "runningTransactionsTimeout",
          &Set_primary_instance_options::running_transactions_timeout);

  return opts;
}

}  // namespace cluster
}  // namespace dba
}  // namespace mysqlsh
