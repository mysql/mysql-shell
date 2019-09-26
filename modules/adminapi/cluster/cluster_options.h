/*
 * Copyright (c) 2018, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_ADMINAPI_CLUSTER_CLUSTER_OPTIONS_H_
#define MODULES_ADMINAPI_CLUSTER_CLUSTER_OPTIONS_H_

#include <map>
#include <string>
#include <vector>

#include "modules/adminapi/cluster/cluster_impl.h"
#include "modules/command_interface.h"

namespace mysqlsh {
namespace dba {

class Cluster_options : public Command_interface {
 public:
  Cluster_options(const Cluster_impl &cluster, bool all);

  ~Cluster_options() override;

  /**
   * Prepare the options command for execution.
   * NOTE: Not currently used (does nothing).
   */
  void prepare() override;

  /**
   * Execute the cluster options command.
   * More specifically:
   * - Iterate through all ReplicaSets of the Cluster in order to obtain the
   * configuration options of each one
   *
   * @return an shcore::Value containing a dictionary object with the command
   * output
   */
  shcore::Value execute() override;

  /**
   * Rollback the command.
   *
   * NOTE: Not currently used (does nothing).
   */
  void rollback() override;

  /**
   * Finalize the command execution.
   *
   * NOTE: Not currently used (does nothing).
   */
  void finish() override;

 private:
  const Cluster_impl &m_cluster;
  bool m_all = false;

  shcore::Value get_cluster_options();
};

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_CLUSTER_CLUSTER_OPTIONS_H_
