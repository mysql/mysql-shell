/*
 * Copyright (c) 2016, 2020, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_ADMINAPI_MOD_DBA_CLUSTER_H_
#define MODULES_ADMINAPI_MOD_DBA_CLUSTER_H_

#include <memory>
#include <string>

#include "scripting/types.h"
#include "scripting/types_cpp.h"
#include "shellcore/shell_options.h"

#include "modules/adminapi/cluster/cluster_impl.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/group_replication_options.h"
#include "mysqlshdk/libs/db/connection_options.h"

namespace mysqlsh {
namespace dba {

/**
 * \ingroup AdminAPI
 * $(CLUSTER_BRIEF)
 */
class Cluster : public std::enable_shared_from_this<Cluster>,
                public shcore::Cpp_object_bridge {
 public:
#if DOXYGEN_JS
  String name;  //!< $(CLUSTER_GETNAME_BRIEF)
  Undefined addInstance(InstanceDef instance, Dictionary options);
  Dictionary checkInstanceState(InstanceDef instance);
  String describe();
  Undefined disconnect();
  Undefined dissolve(Dictionary options);
  Undefined forceQuorumUsingPartitionOf(InstanceDef instance, String password);
  String getName();
  Dictionary listRouters(Dictionary options);
  Undefined rejoinInstance(InstanceDef instance, Dictionary options);
  Undefined removeInstance(InstanceDef instance, Dictionary options);
  Undefined rescan(Dictionary options);
  Undefined resetRecoveryAccountsPassword(Dictionary options);
  String status(Dictionary options);
  Undefined switchToSinglePrimaryMode(InstanceDef instance);
  Undefined switchToMultiPrimaryMode();
  Undefined setPrimaryInstance(InstanceDef instance);
  String options(Dictionary options);
  Undefined setOption(String option, String value);
  Undefined setInstanceOption(InstanceDef instance, String option,
                              String value);
  Boolean removeRouterMetadata(RouterDef routerDef);
  Undefined setupAdminAccount(String user, Dictionary options);
  Undefined setupRouterAccount(String user, Dictionary options);
#elif DOXYGEN_PY
  str name;  //!< $(CLUSTER_GETNAME_BRIEF)
  None add_instance(InstanceDef instance, dict options);
  dict check_instance_state(InstanceDef instance);
  str describe();
  None disconnect();
  None dissolve(dict options);
  None force_quorum_using_partition_of(InstanceDef instance, str password);
  str get_name();
  dict list_routers(dict options);
  None rejoin_instance(InstanceDef instance, dict options);
  None remove_instance(InstanceDef instance, dict options);
  None rescan(dict options);
  None reset_recovery_accounts_password(dict options);
  str status(dict options);
  None switch_to_single_primary_mode(InstanceDef instance);
  None switch_to_multi_primary_mode();
  None set_primary_instance(InstanceDef instance);
  str options(dict options);
  None set_option(str option, str value);
  None set_instance_option(InstanceDef instance, str option, str value);
  bool remove_router_metadata(RouterDef routerDef);
  None setup_admin_account(str user, dict options);
  None setup_router_account(str user, dict options);
#endif

  explicit Cluster(const std::shared_ptr<Cluster_impl> &impl);

  virtual ~Cluster();

  std::string class_name() const override { return "Cluster"; }
  std::string &append_descr(std::string &s_out, int indent = -1,
                            int quote_strings = 0) const override;
  void append_json(shcore::JSON_dumper &dumper) const override;
  bool operator==(const Object_bridge &other) const override;

  shcore::Value call(const std::string &name,
                     const shcore::Argument_list &args) override;
  shcore::Value get_member(const std::string &prop) const override;

  std::shared_ptr<Cluster_impl> impl() const { return m_impl; }

  void assert_valid(const std::string &option_name) const;

  /**
   * Mark the cluster as invalid (e.g., dissolved).
   */
  void invalidate() { m_invalidated = true; }

  void add_instance(const Connection_options &instance_def,
                    const shcore::Dictionary_t &options = {});
  void rejoin_instance(const Connection_options &instance_def,
                       const shcore::Dictionary_t &options = {});
  void remove_instance(const Connection_options &instance_def,
                       const shcore::Dictionary_t &options = {});
  shcore::Value get_replicaset(const shcore::Argument_list &args);
  shcore::Value describe(void);
  shcore::Value status(const shcore::Dictionary_t &options);
  shcore::Dictionary_t list_routers(const shcore::Dictionary_t &options);
  void dissolve(const shcore::Dictionary_t &options);
  shcore::Value check_instance_state(const Connection_options &instance_def);
  void rescan(const shcore::Dictionary_t &options);
  void reset_recovery_accounts_password(const shcore::Dictionary_t &options);
  void force_quorum_using_partition_of(const Connection_options &instance_def,
                                       const char *password = nullptr);
  void disconnect();

  void remove_router_metadata(const std::string &router_def);

  void setup_admin_account(const std::string &user,
                           const shcore::Dictionary_t &options);

  void setup_router_account(const std::string &user,
                            const shcore::Dictionary_t &options);

  void switch_to_single_primary_mode(
      const Connection_options &instance_def = Connection_options());

  void switch_to_multi_primary_mode(void);
  void set_primary_instance(const Connection_options &instance_def);

  shcore::Value options(const shcore::Dictionary_t &options);

  void set_option(const std::string &option, const shcore::Value &value);

  void set_instance_option(const Connection_options &instance_def,
                           const std::string &option,
                           const shcore::Value &value);

 protected:
  // Used shell options
  void init();

 private:
  std::shared_ptr<Cluster_impl> m_impl;
  bool m_invalidated = false;

  void verify_add_rejoin_deprecations(const shcore::Dictionary_t &options);
};

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_MOD_DBA_CLUSTER_H_
