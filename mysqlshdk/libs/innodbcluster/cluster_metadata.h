/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifndef MYSQLSHDK_LIBS_INNODBCLUSTER_CLUSTER_METADATA_H_
#define MYSQLSHDK_LIBS_INNODBCLUSTER_CLUSTER_METADATA_H_

#include <memory>
#include <string>
#include <vector>

#include "mysqlshdk/libs/db/session.h"

namespace mysqlshdk {
namespace innodbcluster {

using Cluster_id = uint64_t;
using Replicaset_id = uint64_t;
using Group_id = std::string;

struct Cluster_info {
  Cluster_id id;
  std::string name;
  std::string description;
  std::string attributes;
};

struct Instance_info {
  std::string uuid;
  std::string name;
  std::string classic_endpoint;
  std::string x_endpoint;
};

// Metadata query methods go here
class Metadata : public std::enable_shared_from_this<Metadata> {
 public:
  virtual bool exists() = 0;
 public:
  virtual bool get_cluster_named(const std::string &name,
                                 Cluster_info *out_info) = 0;
  //
  // virtual bool get_cluster_for_group_name(const std::string &group_name,
  //                                         Cluster_info *out_info) = 0;

  virtual Replicaset_id get_default_replicaset(
      Cluster_id cluster_id, std::vector<Group_id> *out_group_ids) = 0;

  virtual std::vector<Instance_info> get_group_instances(
      const Group_id &rsid) = 0;
};

// Metadata update methods go here
class Metadata_mutable : public Metadata {
 public:
};

class Metadata_mysql : public Metadata_mutable {
 public:
  static std::shared_ptr<Metadata_mysql> create(
      std::shared_ptr<db::ISession> session);

  bool exists() override;

 public:
  bool get_cluster_named(const std::string &name,
                         Cluster_info *out_info) override;

  // bool get_cluster_for_group_name(const std::string &group_name,
  //                                 Cluster_info *out_info) override;

  Replicaset_id get_default_replicaset(
      Cluster_id cluster_id, std::vector<Group_id> *out_group_ids) override;

  std::vector<Instance_info> get_group_instances(const Group_id &rsid) override;

  std::shared_ptr<db::ISession> get_session() const { return _session; }

 private:
  explicit Metadata_mysql(std::shared_ptr<db::ISession> session);

  std::shared_ptr<db::ISession> _session;

  std::shared_ptr<db::IResult> query(const std::string &sql);
};

}  // namespace innodbcluster
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_INNODBCLUSTER_CLUSTER_METADATA_H_
