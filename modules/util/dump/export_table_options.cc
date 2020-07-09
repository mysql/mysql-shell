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

#include "modules/util/dump/export_table_options.h"

#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlsh {
namespace dump {

Export_table_options::Export_table_options(const std::string &schema_table,
                                           const std::string &output_url)
    : Dump_options(output_url) {
  try {
    shcore::split_schema_and_table(schema_table, &m_schema, &m_table);
  } catch (const std::runtime_error &e) {
    throw std::invalid_argument("Failed to parse table to be exported '" +
                                schema_table + "': " + e.what());
  }

  // calling this in the constructor sets the default value
  set_compression(mysqlshdk::storage::Compression::NONE);
}

void Export_table_options::unpack_options(shcore::Option_unpacker *unpacker) {
  set_dialect(import_table::Dialect::unpack(unpacker));
}

void Export_table_options::on_set_session(
    const std::shared_ptr<mysqlshdk::db::ISession> &session) {
  if (m_schema.empty()) {
    const auto result = session->query("SELECT SCHEMA();");

    if (const auto row = result->fetch_one()) {
      if (!row->is_null(0)) {
        m_schema = row->get_string(0);
      }
    }
  }
}

void Export_table_options::validate_options() const {
  if (schema().empty()) {
    throw std::invalid_argument(
        "The table was given without a schema and there is no active schema on "
        "the current session, unable to deduce which table to export.");
  }

  if (import_table::Dialect::json() == dialect()) {
    throw std::invalid_argument("The 'json' dialect is not supported.");
  }
}

}  // namespace dump
}  // namespace mysqlsh
