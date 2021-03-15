/*
 * Copyright (c) 2019, 2021, Oracle and/or its affiliates.
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

#include "modules/adminapi/common/cluster_types.h"

#include <stdexcept>
#include <string>

namespace mysqlsh {
namespace dba {

std::string to_string(Cluster_set_global_status state) {
  switch (state) {
    case Cluster_set_global_status::HEALTHY:
      return "HEALTHY";

    case Cluster_set_global_status::AVAILABLE:
      return "AVAILABLE";

    case Cluster_set_global_status::UNAVAILABLE:
      return "UNAVAILABLE";

    case Cluster_set_global_status::UNKNOWN:
      return "UNKNOWN";
  }
  throw std::logic_error("internal error");
}

std::string to_string(Cluster_type type) {
  switch (type) {
    case Cluster_type::NONE:
      return "NONE";

    case Cluster_type::GROUP_REPLICATION:
      return "GROUP-REPLICATION";

    case Cluster_type::ASYNC_REPLICATION:
      return "ASYNC-REPLICATION";

    case Cluster_type::REPLICATED_CLUSTER:
      return "REPLICATED-CLUSTER";
  }
  throw std::logic_error("internal error");
}

std::string to_string(Cluster_status state) {
  std::string ret_val;

  switch (state) {
    case Cluster_status::OK:
      ret_val = "OK";
      break;
    case Cluster_status::OK_PARTIAL:
      ret_val = "OK_PARTIAL";
      break;
    case Cluster_status::OK_NO_TOLERANCE:
      ret_val = "OK_NO_TOLERANCE";
      break;
    case Cluster_status::OK_NO_TOLERANCE_PARTIAL:
      ret_val = "OK_NO_TOLERANCE_PARTIAL";
      break;
    case Cluster_status::NO_QUORUM:
      ret_val = "NO_QUORUM";
      break;
    case Cluster_status::OFFLINE:
      ret_val = "OFFLINE";
      break;
    case Cluster_status::INVALIDATED:
      ret_val = "INVALIDATED";
      break;
    case Cluster_status::ERROR:
      ret_val = "ERROR";
      break;
    case Cluster_status::UNKNOWN:
      ret_val = "UNKNOWN";
      break;
  }
  return ret_val;
}

Cluster_status to_cluster_status(const std::string &s) {
  if (s == "OK")
    return Cluster_status::OK;
  else if (s == "OK_PARTIAL")
    return Cluster_status::OK_PARTIAL;
  else if (s == "OK_NO_TOLERANCE")
    return Cluster_status::OK_NO_TOLERANCE;
  else if (s == "OK_NO_TOLERANCE_PARTIAL")
    return Cluster_status::OK_NO_TOLERANCE_PARTIAL;
  else if (s == "NO_QUORUM")
    return Cluster_status::NO_QUORUM;
  else if (s == "UNKNOWN")
    return Cluster_status::UNKNOWN;
  else
    throw std::invalid_argument("Invalid value " + s);
}

std::string to_string(Cluster_global_status status) {
  switch (status) {
    case Cluster_global_status::OK:
      return "OK";
    case Cluster_global_status::OK_NOT_REPLICATING:
      return "OK_NOT_REPLICATING";
    case Cluster_global_status::OK_NOT_CONSISTENT:
      return "OK_NOT_CONSISTENT";
    case Cluster_global_status::OK_MISCONFIGURED:
      return "OK_MISCONFIGURED";
    case Cluster_global_status::NOT_OK:
      return "NOT_OK";
    case Cluster_global_status::INVALIDATED:
      return "INVALIDATED";
    case Cluster_global_status::UNKNOWN:
      return "UNKNOWN";
  }
  throw std::logic_error("internal error");
}

std::string to_string(Cluster_channel_status status) {
  switch (status) {
    case Cluster_channel_status::OK:
      return "OK";
    case Cluster_channel_status::STOPPED:
      return "STOPPED";
    case Cluster_channel_status::ERROR:
      return "ERROR";
    case Cluster_channel_status::MISCONFIGURED:
      return "MISCONFIGURED";
    case Cluster_channel_status::MISSING:
      return "MISSING";
    case Cluster_channel_status::UNKNOWN:
      return "UNKNOWN";
  }
  throw std::logic_error("internal error");
}

std::string to_display_string(Cluster_type type, Display_form form) {
  switch (type) {
    case Cluster_type::NONE:
      return "NONE";

    case Cluster_type::GROUP_REPLICATION:
      switch (form) {
        case Display_form::THING_FULL:
          return "InnoDB cluster";

        case Display_form::A_THING_FULL:
          return "an InnoDB cluster";

        case Display_form::THINGS_FULL:
          return "InnoDB clusters";

        case Display_form::THING:
          return "cluster";

        case Display_form::A_THING:
          return "a cluster";

        case Display_form::THINGS:
          return "clusters";

        case Display_form::API_CLASS:
          return "Cluster";
      }
      break;

    case Cluster_type::ASYNC_REPLICATION:
      switch (form) {
        case Display_form::THING_FULL:
          return "InnoDB ReplicaSet";

        case Display_form::A_THING_FULL:
          return "an InnoDB ReplicaSet";

        case Display_form::THINGS_FULL:
          return "InnoDB ReplicaSets";

        case Display_form::THING:
          return "replicaset";

        case Display_form::A_THING:
          return "a replicaset";

        case Display_form::THINGS:
          return "replicasets";

        case Display_form::API_CLASS:
          return "ReplicaSet";
      }
      break;

    case Cluster_type::REPLICATED_CLUSTER:
      switch (form) {
        case Display_form::THING_FULL:
          return "InnoDB ClusterSet";

        case Display_form::A_THING_FULL:
          return "an InnoDB ClusterSet";

        case Display_form::THINGS_FULL:
          return "InnoDB ClusterSets";

        case Display_form::THING:
          return "clusterset";

        case Display_form::A_THING:
          return "a clusterset";

        case Display_form::THINGS:
          return "clustersets";

        case Display_form::API_CLASS:
          return "ClusterSet";
      }
      break;
  }
  throw std::logic_error("internal error");
}

}  // namespace dba
}  // namespace mysqlsh
