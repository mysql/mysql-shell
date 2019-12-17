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

#ifndef MODULES_ADMINAPI_MOD_DBA_REPLICA_SET_H_
#define MODULES_ADMINAPI_MOD_DBA_REPLICA_SET_H_

#include <memory>
#include <string>
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/replica_set/replica_set_impl.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "scripting/types.h"
#include "scripting/types_cpp.h"
#include "shellcore/shell_options.h"

namespace mysqlsh {
namespace dba {
class Dba;
class MetadataStorage;
class Global_topology_manager;

/**
 * \ingroup AdminAPI
 * $(REPLICASET_BRIEF)
 */
class ReplicaSet : public std::enable_shared_from_this<ReplicaSet>,
                   public shcore::Cpp_object_bridge {
 public:
#if DOXYGEN_JS
  String name;  //!< $(ASYNCREPLICASET_GETNAME_BRIEF)
  Undefined addInstance(String instance, Dictionary options);
  String describe();
  Undefined disconnect();
  Undefined dissolve(Dictionary options);
  String getName();
  Dictionary listRouters(Dictionary options);
  Undefined rejoinInstance(String instance, Dictionary options);
  Undefined removeInstance(String instance, Dictionary options);
  String status(Dictionary options);
  Undefined setPrimaryInstance(String instance, Dictionary options);
  Undefined forcePrimaryInstance(String instance, Dictionary options);
  Boolean removeRouterMetadata(RouterDef routerDef);
#elif DOXYGEN_PY
  str name;  //!< $(REPLICASET_GETNAME_BRIEF)
  None add_instance(str instance, dict options);
  str describe();
  None disconnect();
  None dissolve(dict options);
  str get_name();
  dict list_routers(dict options);
  None rejoin_instance(str instance, dict options);
  None remove_instance(str instance, dict options);
  str status(dict options);
  None set_primary_instance(str instance, dict options);
  None force_primary_instance(str instance, dict options);
  bool remove_router_metadata(RouterDef routerDef);
#endif

  explicit ReplicaSet(const std::shared_ptr<Replica_set_impl> &cluster);

  virtual ~ReplicaSet();

  std::string class_name() const override { return "ReplicaSet"; }
  std::string &append_descr(std::string &s_out, int indent = -1,
                            int quote_strings = 0) const override;
  void append_json(shcore::JSON_dumper &dumper) const override;
  bool operator==(const Object_bridge &other) const override;

  shcore::Value get_member(const std::string &prop) const override;

  std::shared_ptr<Replica_set_impl> impl() const { return m_impl; }

  void assert_valid(const std::string &option_name) const;

  /**
   * Mark the cluster as invalid (e.g., dissolved).
   */
  void invalidate() { m_invalidated = true; }

 public:
  void add_instance(const std::string &instance_def,
                    const shcore::Dictionary_t &options);

  void rejoin_instance(const std::string &instance_def,
                       const shcore::Dictionary_t &options);

  void remove_instance(const std::string &instance_def,
                       const shcore::Dictionary_t &options);

  shcore::Value describe(void);
  shcore::Value status(const shcore::Dictionary_t &options);
  void dissolve(const shcore::Dictionary_t &options);

  void disconnect();

  void set_primary_instance(const std::string &instance_def,
                            const shcore::Dictionary_t &options);

  void force_primary_instance(const std::string &instance_def,
                              const shcore::Dictionary_t &options);

  shcore::Dictionary_t list_routers(const shcore::Dictionary_t &options);
  void remove_router_metadata(const std::string &router_def);

  void setup_admin_account(const std::string &name,
                           const shcore::Dictionary_t &options);

  void setup_router_account(const std::string &name,
                            const shcore::Dictionary_t &options);

 protected:
  void init();

 private:
  std::shared_ptr<Replica_set_impl> m_impl;
  bool m_invalidated = false;

  shcore::Value execute_with_pool(const std::function<shcore::Value()> &f,
                                  bool interactive);
};

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_MOD_DBA_REPLICA_SET_H_
