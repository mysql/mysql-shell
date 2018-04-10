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

#ifndef MYSQLSHDK_LIBS_INNODBCLUSTER_CLUSTER_H_
#define MYSQLSHDK_LIBS_INNODBCLUSTER_CLUSTER_H_

#include <memory>
#include <string>
#include <vector>

#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/innodbcluster/cluster_metadata.h"
#include "mysqlshdk/libs/mysql/instance.h"

namespace mysqlshdk {
namespace innodbcluster {

// Error codes for InnoDB cluster
enum class Error {
  Metadata_missing = 10001,
  Metadata_version_too_old = 10002,
  Metadata_version_too_new = 10003,
  Metadata_out_of_sync = 10004,
  Metadata_inconsistent = 10005,
  Group_replication_not_active = 10010,
  Group_has_no_quorum = 10011
};

std::string to_string(Error code);

class cluster_error : public std::runtime_error {
 public:
  explicit cluster_error(Error code, const std::string &s)
      : std::runtime_error(s), _code(code) {}

  Error code() const { return _code; }

  std::string format() const {
    return "InnoDB cluster error " + std::to_string(static_cast<int>(_code)) +
           ": " + to_string(_code) + ": " + what();
  }

 private:
  Error _code;
};

class Replicaset;
class Group;

enum class Protocol_type { X, Classic };

/*
 * General purpose API for clients of the InnoDB cluster.
 * Provides functionality for determining status, getting information about
 * online primaries etc.
 *
 * This class represents one specific group in the cluster, along with
 * a session to its metadata server and a session to an arbitrary member of the
 * group.
 *
 * Cluster management routines (changing the cluster) are kept separate.
 */
class Cluster_group_client {
 public:
  Cluster_group_client(std::shared_ptr<Metadata> md,
                       std::shared_ptr<db::ISession> session,
                       bool recovery_mode = false);

  bool single_primary() const { return _single_primary_mode; }

  std::vector<Instance_info> get_online_primaries();
  std::vector<Instance_info> get_online_secondaries();

  std::string find_uri_to_any_primary(Protocol_type type);
  std::string find_uri_to_any_secondary(Protocol_type type);

 private:
  std::shared_ptr<Metadata> _metadata;
  std::unique_ptr<mysqlshdk::mysql::Instance> _instance;
  std::string _group_name;
  bool _single_primary_mode;
};

void check_quorum(mysqlshdk::mysql::Instance *instance);

}  // namespace innodbcluster
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_INNODBCLUSTER_CLUSTER_H_
