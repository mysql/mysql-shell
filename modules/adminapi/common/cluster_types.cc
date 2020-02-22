/*
 * Copyright (c) 2019, 2020, Oracle and/or its affiliates. All rights reserved.
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

std::string to_string(Cluster_type type) {
  switch (type) {
    case Cluster_type::NONE:
      return "NONE";

    case Cluster_type::GROUP_REPLICATION:
      return "GROUP-REPLICATION";

    case Cluster_type::ASYNC_REPLICATION:
      return "ASYNC-REPLICATION";
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
  }
  throw std::logic_error("internal error");
}

}  // namespace dba
}  // namespace mysqlsh
