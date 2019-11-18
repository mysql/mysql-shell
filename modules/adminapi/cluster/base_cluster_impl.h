/*
 * Copyright (c) 2016, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_ADMINAPI_CLUSTER_BASE_CLUSTER_IMPL_H_
#define MODULES_ADMINAPI_CLUSTER_BASE_CLUSTER_IMPL_H_

#include <list>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "scripting/types.h"

#include "modules/adminapi/common/cluster_types.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/group_replication_options.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "mysqlshdk/libs/db/connection_options.h"

namespace mysqlsh {
namespace dba {

// User provided option for telling us to assume that the cluster was created
// with a server where the full update history is reflected in its GTID set
constexpr const char k_cluster_attribute_assume_gtid_set_complete[] =
    "opt_gtidSetIsComplete";

class Base_cluster_impl {
 public:
  Base_cluster_impl(const std::string &cluster_name,
                    std::shared_ptr<Instance> group_server,
                    std::shared_ptr<MetadataStorage> metadata_storage);
  virtual ~Base_cluster_impl();

  virtual Cluster_type get_type() const = 0;

  virtual std::string get_topology_type() const = 0;

  bool get_gtid_set_is_complete() const;

  Cluster_id get_id() const { return m_id; }
  void set_id(Cluster_id id) { m_id = id; }

  std::string get_description() const { return m_description; }

  void set_description(const std::string &description) {
    m_description = description;
  }

  std::string get_name() const { return m_cluster_name; }

  void set_cluster_name(const std::string &name) { m_cluster_name = name; }
  const std::string &cluster_name() const { return m_cluster_name; }

  void set_target_server(const std::shared_ptr<Instance> &instance);

  std::shared_ptr<Instance> get_target_server() const {
    return m_target_server;
  }

  std::shared_ptr<Instance> get_primary_master() const {
    return m_primary_master;
  }

  virtual Cluster_check_info check_preconditions(
      const std::string &function_name) const = 0;

  std::shared_ptr<MetadataStorage> get_metadata_storage() const {
    return m_metadata_storage;
  }

  const mysqlshdk::mysql::Auth_options &default_admin_credentials() const {
    return m_admin_credentials;
  }

  virtual mysqlshdk::mysql::IInstance *acquire_primary() = 0;

  virtual void release_primary() = 0;

  mysqlshdk::mysql::Auth_options create_replication_user(
      mysqlshdk::mysql::IInstance *slave, const std::string &user_prefix,
      bool dry_run);

  mysqlshdk::mysql::Auth_options refresh_replication_user(
      mysqlshdk::mysql::IInstance *slave, const std::string &user_prefix,
      bool dry_run);

  void drop_replication_user(mysqlshdk::mysql::IInstance *slave,
                             const std::string &user_prefix);

  std::string get_replication_user(mysqlshdk::mysql::IInstance *target_instance,
                                   const std::string &user_prefix) const;

  virtual void disconnect();

  virtual shcore::Value list_routers(bool only_upgrade_required);

  virtual std::list<Scoped_instance> connect_all_members(
      uint32_t read_timeout, bool skip_primary,
      std::list<Instance_metadata> *out_unreachable) = 0;

 public:
  /*
   * Synchronize transactions on target instance.
   *
   * Wait for all current cluster transactions to be applied on the specified
   * target instance. Function will monitor for replication errors on the named
   * channel and throw an exception if an error is detected.
   *
   * @param target_instance instance to wait for transaction to be applied.
   * @param channel_name the name of the channel to monitor
   * @param timeout number of seconds to wait
   *
   * @throw RuntimeError if the timeout is reached when waiting for
   * transactions to be applied or replication errors are detected.
   */
  void sync_transactions(const mysqlshdk::mysql::IInstance &target_instance,
                         const std::string &channel_name, int timeout) const;

 protected:
  Cluster_id m_id;
  std::string m_cluster_name;
  std::string m_description;
  // Session to a member of the group so we can query its status and other
  // stuff from pfs
  std::shared_ptr<Instance> m_target_server;
  std::shared_ptr<Instance> m_primary_master;

  std::shared_ptr<MetadataStorage> m_metadata_storage;

  mysqlshdk::mysql::Auth_options m_admin_credentials;

  void target_server_invalidated();

  /**
   * Connect to the given instance specification given, while validating its
   * syntax.
   *
   * @param instance_def the instance to connect to, as a host:port string.
   * A URL is allowed, if it matches that of m_target_server.
   * @return instance object owned by ipool
   */
  std::shared_ptr<Instance> connect_target_instance(
      const std::string &instance_def);
};

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_CLUSTER_BASE_CLUSTER_IMPL_H_
