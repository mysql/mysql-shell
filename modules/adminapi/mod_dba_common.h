/*
 * Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _MODULES_ADMINAPI_MOD_DBA_COMMON_
#define _MODULES_ADMINAPI_MOD_DBA_COMMON_

#include "shellcore/types.h"
#include "shellcore/lang_base.h"
#include "modules/adminapi/mod_dba_provisioning_interface.h"

namespace mysqlsh {
namespace dba {
class MetadataStorage;

// Note that this structure may be initialized using initializer
// lists, so the order of hte fields is very important
struct FunctionAvailability {
  int instance_config_state;
  int cluster_status;
  int instance_status;
};

enum GRInstanceType {
  Standalone = 1 << 0,
  GroupReplication = 1 << 1,
  InnoDBCluster = 1 << 2,
  Any = Standalone | GroupReplication | InnoDBCluster
};

enum class SlaveReplicationState {
  New,
  Recoverable,
  Diverged,
  Irrecoverable
};

namespace ManagedInstance {
enum State {
  OnlineRW = 1 << 0,
  OnlineRO = 1 << 1,
  Recovering = 1 << 2,
  Unreachable = 1 << 3,
  Offline = 1 << 4,
  Error = 1 << 5,
  Any = OnlineRO | OnlineRW | Recovering | Unreachable | Offline | Error
};

std::string describe(State state);
};

namespace ReplicationQuorum {
enum State {
  Normal = 1 << 0,
  Quorumless = 1 << 1,
  Dead = 1 << 2,
  Any = Normal | Quorumless | Dead
};
}

struct ReplicationGroupState {
  ReplicationQuorum::State quorum;    // The state of the cluster from the quorm point of view
  std::string master;                 // The UIUD of the master instance
  std::string source;                 // The UUID of the instance from which the data was consulted
  GRInstanceType source_type;         // The configuration type of the instance from which the data was consulted
  ManagedInstance::State source_state;// The state of the instance from which the data was consulted
};

namespace ReplicaSetStatus {
enum Status {
  OK,
  OK_PARTIAL,
  OK_NOTOLERANCE,
  NOQUORUM,
  UNKNOWN
};

std::string describe(Status state);
};

shcore::Value::Map_type_ref get_instance_options_map(const shcore::Argument_list &args, bool get_password_from_options = false);
void resolve_instance_credentials(const shcore::Value::Map_type_ref& options, shcore::Interpreter_delegate* delegate = nullptr);
std::string get_mysqlprovision_error_string(const shcore::Value::Array_type_ref& errors);
ReplicationGroupState check_function_preconditions(const std::string& class_name, const std::string& base_function_name, const std::string &function_name, const std::shared_ptr<MetadataStorage>& metadata);

void validate_ssl_instance_options(shcore::Value::Map_type_ref &options);
}
}
#endif
