/*
 * Copyright (c) 2020, 2021, Oracle and/or its affiliates.
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
#include "mysqlshdk/libs/utils/utils_sqlstring.h"

namespace mysqlsh {
namespace dump {

Export_table_options::Export_table_options() {
  // calling this in the constructor sets the default value
  set_compression(mysqlshdk::storage::Compression::NONE);
}

const shcore::Option_pack_def<Export_table_options>
    &Export_table_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Export_table_options>()
          .include<Dump_options>()
          .include(&Export_table_options::m_dialect_unpacker)
          .include(&Export_table_options::m_oci_option_unpacker)
          .on_done(&Export_table_options::on_unpacked_options)
          .on_log(&Export_table_options::on_log_options);

  return opts;
}

void Export_table_options::on_unpacked_options() {
  set_dialect(m_dialect_unpacker);

  if (import_table::Dialect::json() == dialect()) {
    throw std::invalid_argument("The 'json' dialect is not supported.");
  }

  set_oci_options(m_oci_option_unpacker);
}

void Export_table_options::set_table(const std::string &schema_table) {
  try {
    shcore::split_schema_and_table(schema_table, &m_schema, &m_table);
    set_includes();
  } catch (const std::runtime_error &e) {
    throw std::invalid_argument("Failed to parse table to be exported '" +
                                schema_table + "': " + e.what());
  }
}

void Export_table_options::on_set_session(
    const std::shared_ptr<mysqlshdk::db::ISession> &session) {
  if (m_schema.empty()) {
    const auto result = session->query_log_error("SELECT SCHEMA();");

    if (const auto row = result->fetch_one()) {
      if (!row->is_null(0)) {
        m_schema = row->get_string(0);
      }
    }

    set_includes();
  }
}

void Export_table_options::validate_options() const {
  if (schema().empty()) {
    throw std::invalid_argument(
        "The table was given without a schema and there is no active schema "
        "on the current session, unable to deduce which table to export.");
  }

  if (!exists(schema(), table())) {
    throw std::invalid_argument(
        "The requested table " + shcore::quote_identifier(schema()) + "." +
        shcore::quote_identifier(table()) + " was not found in the database.");
  }
}

void Export_table_options::set_includes() {
  if (!schema().empty()) {
    m_included_schemas.emplace(schema());
    m_included_tables[schema()].emplace(table());
  }
}

}  // namespace dump
}  // namespace mysqlsh
