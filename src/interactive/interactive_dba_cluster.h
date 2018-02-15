/*
 * Copyright (c) 2016, 2018, Oracle and/or its affiliates. All rights reserved.
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

// Interactive DB access module
// (the one exposed as the db variable in the shell)

#ifndef SRC_INTERACTIVE_INTERACTIVE_DBA_CLUSTER_H_
#define SRC_INTERACTIVE_INTERACTIVE_DBA_CLUSTER_H_

#include <string>
#include <memory>

#include "modules/interactive_object_wrapper.h"
#include "modules/adminapi/mod_dba_common.h"

namespace shcore {
class Interactive_dba_cluster : public Interactive_object_wrapper {
 public:
  explicit Interactive_dba_cluster(
      Shell_core &shell_core,
      std::shared_ptr<mysqlsh::IConsole> console_handler) :
    Interactive_object_wrapper("dba", shell_core, console_handler) { init(); }

  void init();

  shcore::Value add_seed_instance(const shcore::Argument_list &args);
  shcore::Value add_instance(const shcore::Argument_list &args);
  shcore::Value rejoin_instance(const shcore::Argument_list &args);
  shcore::Value remove_instance(const shcore::Argument_list &args);
  shcore::Value dissolve(const shcore::Argument_list &args);
  shcore::Value check_instance_state(const shcore::Argument_list &args);
  shcore::Value rescan(const shcore::Argument_list &args);
  shcore::Value force_quorum_using_partition_of(
        const shcore::Argument_list &args);

 private:
  bool resolve_instance_options(const std::string &function,
                                const shcore::Argument_list &args,
                                shcore::Value::Map_type_ref &options) const;
  mysqlsh::dba::ReplicationGroupState check_preconditions(
      const std::string &function_name) const;
  void assert_valid(const std::string& function_name) const;
};
}  // namespace shcore

#endif  // SRC_INTERACTIVE_INTERACTIVE_DBA_CLUSTER_H_
