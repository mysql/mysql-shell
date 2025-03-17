/*
 * Copyright (c) 2025, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "modules/util/common/dump/vector_store_info.h"

#include <utility>

namespace mysqlsh {
namespace dump {
namespace common {

void serialize(const Vector_store_info &info, shcore::JSON_dumper *dumper) {
  dumper->start_object();

  if (info.relative_path.has_value()) {
    dumper->append("relativeLocation", *info.relative_path);
  }

  if (info.resource_principals.valid()) {
    dumper->append("resourcePrincipals");
    serialize(info.resource_principals, dumper);
  }

  dumper->end_object();
}

Vector_store_info vector_store_info(const shcore::json::Object &info) {
  Vector_store_info result;

  if (auto rl = shcore::json::optional(info, "relativeLocation", true);
      rl.has_value()) {
    result.relative_path = std::move(*rl);
  }

  if (const auto rp = shcore::json::optional_object(info, "resourcePrincipals");
      rp.has_value()) {
    result.resource_principals = resource_principals_info(*rp);
  }

  return result;
}

}  // namespace common
}  // namespace dump
}  // namespace mysqlsh
