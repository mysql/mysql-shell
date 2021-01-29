/*
 * Copyright (c) 2020, 2021, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_MOD_DBA_CLUSTER_SET_H_
#define MODULES_ADMINAPI_MOD_DBA_CLUSTER_SET_H_

#include <memory>
#include <string>
#include "modules/adminapi/cluster_set/api_options.h"
#include "modules/adminapi/cluster_set/cluster_set_impl.h"
#include "modules/adminapi/common/common.h"
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
 * $(CLUSTERSET_BRIEF)
 */
class ClusterSet : public std::enable_shared_from_this<ClusterSet>,
                   public shcore::Cpp_object_bridge {
 public:
#if DOXYGEN_JS
  String name;  //!< $(CLUSTERSET_GETNAME_BRIEF)
  String getName();
  Undefined disconnect();
  Cluster createReplicaCluster(InstanceDef instance, String clusterName,
                               Dictionary options);
  Undefined removeCluster(String clusterName, Dictionary options);
  Undefined rejoinCluster(String clusterName, Dictionary options);
  Undefined setPrimaryCluster(String clusterName, Dictionary options);
  Undefined forcePrimaryCluster(String clusterName, Dictionary options);
  String status(Dictionary options);
  String describe();
#elif DOXYGEN_PY
  str name;  //!< $(CLUSTERSET_GETNAME_BRIEF)
  str get_name();
  None disconnect();
  Cluster create_replica_cluster(InstanceDef instance, String clusterName,
                                 dict options);
  None remove_cluster(str cluster_name, dict options);
  None rejoin_cluster(str cluster_name, dict options);
  None set_primary_cluster(str cluster_name, dict options);
  None force_primary_cluster(str cluster_name, dict options);
  str status(dict options);
  str describe();
#endif

  explicit ClusterSet(const std::shared_ptr<Cluster_set_impl> &clusterset);

  virtual ~ClusterSet();

  std::string class_name() const override { return "ClusterSet"; }
  std::string &append_descr(std::string &s_out, int indent = -1,
                            int quote_strings = 0) const override;
  void append_json(shcore::JSON_dumper &dumper) const override;
  bool operator==(const Object_bridge &other) const override;

  shcore::Value get_member(const std::string &prop) const override;

 public:
  void disconnect();

  shcore::Value create_replica_cluster(
      const std::string &instance_def, const std::string &cluster_name,
      const shcore::Option_pack_ref<clusterset::Create_replica_cluster_options>
          &options = {});
  void remove_cluster(const std::string &cluster_name,
                      const shcore::Option_pack_ref<
                          clusterset::Remove_cluster_options> &options = {});
  void rejoin_cluster(const std::string & /*cluster_name*/,
                      const shcore::Dictionary_t & /*options*/) {
    throw std::logic_error("Not implemented yet");
  }
  void set_primary_cluster(const std::string & /*cluster_name*/,
                           const shcore::Dictionary_t & /*options*/) {
    throw std::logic_error("Not implemented yet");
  }
  void force_primary_cluster(const std::string & /*cluster_name*/,
                             const shcore::Dictionary_t & /*options*/) {
    throw std::logic_error("Not implemented yet");
  }
  shcore::Value status(const shcore::Dictionary_t & /*options*/) {
    throw std::logic_error("Not implemented yet");
  }
  shcore::Value describe() { throw std::logic_error("Not implemented yet"); }

 protected:
  void init();

 private:
  std::shared_ptr<Cluster_set_impl> m_impl;

  std::shared_ptr<Cluster_set_impl> impl() const { return m_impl; }

  void assert_valid(const std::string &function_name);
};

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_MOD_DBA_CLUSTER_SET_H_
