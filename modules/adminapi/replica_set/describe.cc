/*
 * Copyright (c) 2023, Oracle and/or its affiliates.
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

#include "modules/adminapi/replica_set/describe.h"

#include "modules/adminapi/replica_set/replica_set_impl.h"

namespace mysqlsh::dba::replicaset {

shcore::Value Describe::do_run() {
  auto dict = shcore::make_dict();

  // name
  (*dict)["name"] = shcore::Value(m_rset.get_name());

  // topology
  {
    shcore::Array_t list = shcore::make_array();

    for (const auto &i_def : m_rset.get_instances_from_metadata()) {
      auto member = shcore::make_dict();

      (*member)["address"] = shcore::Value(i_def.endpoint);
      (*member)["label"] = shcore::Value(i_def.label);
      if (i_def.invalidated)
        (*member)["instanceRole"] = shcore::Value::Null();
      else
        (*member)["instanceRole"] =
            shcore::Value(i_def.primary_master ? "PRIMARY" : "REPLICA");

      list->push_back(shcore::Value(std::move(member)));
    }

    (*dict)["topology"] = shcore::Value(std::move(list));
  }

  return shcore::Value(std::move(dict));
}

}  // namespace mysqlsh::dba::replicaset
