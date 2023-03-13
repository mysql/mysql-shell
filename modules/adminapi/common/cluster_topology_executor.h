/*
 * Copyright (c) 2022, 2023, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_COMMON_CLUSTER_TOPOLOGY_EXECUTOR_H_
#define MODULES_ADMINAPI_COMMON_CLUSTER_TOPOLOGY_EXECUTOR_H_

#include <utility>

#include "modules/adminapi/common/undo.h"
#include "modules/mod_utils.h"

namespace mysqlsh::dba {

// Policy-based design pattern:
//
//  - Compile-time strategy pattern variant using template metaprogramming
//  - The "host" class (Cluster_topology_executor) takes a type parameter input
//    when instantiated which then implements the desired interface
//  - Defines a common method to execute the command's interface: run()

template <typename TCluster_topology_op>
class Cluster_topology_executor : private TCluster_topology_op {
  using TCluster_topology_op::do_run;
  using TCluster_topology_op::m_undo_list;
  using TCluster_topology_op::m_undo_tracker;

 public:
  template <typename... Args>
  explicit Cluster_topology_executor(Args &&... args)
      : TCluster_topology_op(std::forward<Args>(args)...) {}

  template <typename... TArgs>
  auto run(TArgs &&... args) {
    shcore::Scoped_callback undo([&]() {
      if (!m_undo_list.empty()) {
        m_undo_list.cancel();
      }
    });

    try {
      return do_run(std::forward<TArgs>(args)...);
    } catch (...) {
      if (!m_undo_list.empty() || !m_undo_tracker.empty()) {
        // Handle exceptions
        auto console = mysqlsh::current_console();

        auto exception_str = format_active_exception();

        // Ignore custom exceptions with default c-tor (e.g. cancel_sync)
        if (exception_str != "std::exception") {
          console->print_error(exception_str);
        }

        console->print_info();

        console->print_note("Reverting changes...");

        if (!m_undo_list.empty()) {
          m_undo_list.call();
        }

        if (!m_undo_tracker.empty()) {
          m_undo_tracker.execute();
        }

        console->print_info();
        console->print_info("Changes successfully reverted.");
      }

      throw;
    }
  }
};

}  // namespace mysqlsh::dba

#endif  // MODULES_ADMINAPI_COMMON_CLUSTER_TOPOLOGY_EXECUTOR_H_
