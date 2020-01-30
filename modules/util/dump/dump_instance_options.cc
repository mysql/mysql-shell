/*
 * Copyright (c) 2020, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/util/dump/dump_instance_options.h"

#include <utility>
#include <vector>

#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlsh {
namespace dump {

Dump_instance_options::Dump_instance_options(const std::string &output_dir)
    : Dump_schemas_options(output_dir, true) {}

void Dump_instance_options::unpack_options(shcore::Option_unpacker *unpacker) {
  Dump_schemas_options::unpack_options(unpacker);

  std::vector<std::string> schemas;
  unpacker->optional("excludeSchemas", &schemas);

  for (auto &schema : schemas) {
    m_excluded_schemas.emplace(std::move(schema));
  }
}

void Dump_instance_options::validate_options() const {
  // call method up the chain, Dump_schemas_options has an empty list of schemas
  // and would throw
  Ddl_dumper_options::validate_options();
}

}  // namespace dump
}  // namespace mysqlsh
