/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
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
#include "mysqlshdk/libs/utils/nullable.h"

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
  std::string role;
};

class Metadata : public std::enable_shared_from_this<Metadata> {
 public:
  Metadata() = default;
  Metadata(const Metadata &other) = default;
  Metadata(Metadata &&other) = default;
  Metadata &operator=(const Metadata &other) = default;
  Metadata &operator=(Metadata &&other) = default;
  virtual ~Metadata() = default;

  virtual bool exists() = 0;

 public:
  virtual bool get_cluster_named(const std::string &name,
                                 Cluster_info *out_info) = 0;

  virtual utils::nullable<Instance_info> get_instance_info_by_uuid(
      const std::string &uuid) const = 0;

  virtual std::string get_instance_uuid_by_address(
      const std::string &address) const = 0;

  // virtual bool get_cluster_for_group_name(const std::string &group_name,
  //                                         Cluster_info *out_info) = 0;

  virtual Replicaset_id get_default_replicaset(
      Cluster_id cluster_id, std::vector<Group_id> *out_group_ids) = 0;

  virtual std::vector<Instance_info> get_group_instances(
      const Group_id &rsid) = 0;

  virtual std::vector<Instance_info> get_replicaset_instances(
      const Replicaset_id &rsid) = 0;
};

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

  utils::nullable<Instance_info> get_instance_info_by_uuid(
      const std::string &uuid) const override;

  std::string get_instance_uuid_by_address(
      const std::string &address) const override;

  // bool get_cluster_for_group_name(const std::string &group_name,
  //                                 Cluster_info *out_info) override;

  Replicaset_id get_default_replicaset(
      Cluster_id cluster_id, std::vector<Group_id> *out_group_ids) override;

  std::vector<Instance_info> get_group_instances(const Group_id &rsid) override;

  std::vector<Instance_info> get_replicaset_instances(
      const Replicaset_id &rsid) override;

  std::shared_ptr<db::ISession> get_session() const { return _session; }

 private:
  explicit Metadata_mysql(std::shared_ptr<db::ISession> session);

  std::shared_ptr<db::ISession> _session;

  std::shared_ptr<db::IResult> query(const std::string &sql);
};

}  // namespace innodbcluster
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_INNODBCLUSTER_CLUSTER_METADATA_H_
