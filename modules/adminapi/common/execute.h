/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_COMMON_EXECUTE_H_
#define MODULES_ADMINAPI_COMMON_EXECUTE_H_

#include <chrono>
#include <string>
#include <variant>
#include <vector>

#include "modules/adminapi/common/instance_pool.h"
#include "mysqlshdk/include/scripting/types.h"

namespace mysqlsh::dba {

class Cluster_impl;
class Cluster_set_impl;
class Replica_set_impl;

class Execute {
 public:
  using Instances_def =
      std::variant<std::monostate, std::string, std::vector<std::string>>;

  static Instances_def convert_to_instances_def(const shcore::Value &value,
                                                bool to_exclude);

 private:
  struct InstanceData {
    Scoped_instance instance;
    std::string address;
    std::string label;

    InstanceData(std::shared_ptr<Instance> instance_,
                 std::string label_) noexcept
        : instance{std::move(instance_)}, label{std::move(label_)} {}
    InstanceData(Scoped_instance instance_, std::string label_) noexcept
        : instance{std::move(instance_)}, label{std::move(label_)} {}
    InstanceData(std::string address_, std::string label_) noexcept
        : address{std::move(address_)}, label{std::move(label_)} {}
  };

  static std::vector<InstanceData> gather_instances(
      const Cluster_impl &cluster, const Instances_def &instances_add);
  static void exclude_instances(const Cluster_impl &cluster,
                                std::vector<InstanceData> &instances,
                                const Instances_def &instances_exclude);

  static std::vector<InstanceData> gather_instances(
      Cluster_set_impl &cset, const Instances_def &instances_add);
  static void exclude_instances(Cluster_set_impl &cset,
                                std::vector<InstanceData> &instances,
                                const Instances_def &instances_exclude);

  static std::vector<InstanceData> gather_instances(
      const Replica_set_impl &rset, const Instances_def &instances_add);
  static void exclude_instances(const Replica_set_impl &rset,
                                std::vector<InstanceData> &instances,
                                const Instances_def &instances_exclude);

 protected:
  explicit Execute(const Cluster_impl &cluster, bool dry_run = false) noexcept
      : m_topo{cluster}, m_dry_run{dry_run} {}
  explicit Execute(Cluster_set_impl &cluster_set, bool dry_run = false) noexcept
      : m_topo{cluster_set}, m_dry_run{dry_run} {}
  explicit Execute(const Replica_set_impl &replica_set,
                   bool dry_run = false) noexcept
      : m_topo{replica_set}, m_dry_run{dry_run} {}

  shcore::Value do_run(const std::string &cmd,
                       const Instances_def &instances_add,
                       const Instances_def &instances_exclude,
                       std::chrono::seconds timeout);
  static constexpr bool supports_undo() noexcept { return false; }

 private:
  std::vector<shcore::Value> execute_parallel(
      const std::string &cmd, const std::vector<InstanceData> &instances,
      std::chrono::seconds timeout) const;

 private:
  std::variant<std::reference_wrapper<const Cluster_impl>,
               std::reference_wrapper<Cluster_set_impl>,
               std::reference_wrapper<const Replica_set_impl>>
      m_topo;
  bool m_dry_run{false};
};

}  // namespace mysqlsh::dba

#endif  // MODULES_ADMINAPI_COMMON_EXECUTE_H_
