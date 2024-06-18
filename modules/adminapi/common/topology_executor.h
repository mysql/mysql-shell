/*
 * Copyright (c) 2022, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef MODULES_ADMINAPI_COMMON_TOPOLOGY_EXECUTOR_H_
#define MODULES_ADMINAPI_COMMON_TOPOLOGY_EXECUTOR_H_

#include <type_traits>
#include <utility>

#include "modules/adminapi/common/undo.h"
#include "modules/mod_utils.h"

namespace mysqlsh::dba {

// Policy-based design pattern:
//
//  - Compile-time strategy pattern variant using template metaprogramming
//  - The "host" class (Topology_executor) takes a type parameter input
//    when instantiated which then implements the desired interface
//  - Defines a common method to execute the command's interface: run()

template <typename TTopology_op>
class Topology_executor final : private TTopology_op {
  // Helper class to check if type has a static 'bool supports_undo()' method
  template <class T, class = void>
  struct has_supports_undo : std::false_type {};
  template <class T>
  struct has_supports_undo<T, std::enable_if_t<std::is_invocable_r<
                                  bool, decltype(T::supports_undo)>::value>>
      : std::true_type {};

  static_assert(has_supports_undo<TTopology_op>::value,
                "Type to use with the executor must have a static 'bool "
                "supports_undo()' method.");

 public:
  template <typename... Args>
  explicit Topology_executor(Args &&...args)
      : TTopology_op(std::forward<Args>(args)...) {}

  template <typename... TArgs>
  auto run(TArgs &&...args) {
    try {
      return TTopology_op::do_run(std::forward<TArgs>(args)...);
    } catch (...) {
      if constexpr (!TTopology_op::supports_undo()) {
        throw;
      } else {
        auto console = mysqlsh::current_console();

        // Ignore custom exceptions with default c-tor (e.g. cancel_sync)
        if (auto exception_str = format_active_exception();
            exception_str != "std::exception") {
          console->print_error(exception_str);
        }

        console->print_info();
        console->print_note("Reverting changes...");

        TTopology_op::do_undo();

        console->print_info();
        console->print_info("Changes successfully reverted.");

        throw;
      }
    }
  }
};

}  // namespace mysqlsh::dba

#endif  // MODULES_ADMINAPI_COMMON_TOPOLOGY_EXECUTOR_H_
