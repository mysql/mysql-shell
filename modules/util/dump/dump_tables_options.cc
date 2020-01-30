/*
 * Copyright (c) 2020, Oracle and/or its affiliates.
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

#include "modules/util/dump/dump_tables_options.h"

namespace mysqlsh {
namespace dump {

Dump_tables_options::Dump_tables_options(const std::string &schema,
                                         const std::vector<std::string> &tables,
                                         const std::string &output_url)
    : Ddl_dumper_options(output_url),
      m_schema(schema),
      m_tables(tables.begin(), tables.end()) {}

void Dump_tables_options::unpack_options(shcore::Option_unpacker *unpacker) {
  Ddl_dumper_options::unpack_options(unpacker);

  unpacker->optional("all", &m_dump_all);
}

void Dump_tables_options::validate_options() const {
  Ddl_dumper_options::validate_options();

  if (schema().empty()) {
    throw std::invalid_argument(
        "The 'schema' parameter cannot be an empty string.");
  }

  if (!dump_all() && tables().empty()) {
    throw std::invalid_argument(
        "The 'tables' parameter cannot be an empty list.");
  }

  if (dump_all() && !tables().empty()) {
    throw std::invalid_argument(
        "When the 'all' parameter is set to true, the 'tables' parameter must "
        "be an empty list.");
  }
}

}  // namespace dump
}  // namespace mysqlsh
