/*
 * Copyright (c) 2018, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/util/import_table/import_table_options.h"

#include <errno.h>
#include <algorithm>
#include <limits>

#include "modules/mod_utils.h"
#include "modules/util/import_table/helpers.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/include/shellcore/base_session.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_path.h"

namespace mysqlsh {
namespace import_table {

Import_table_options::Import_table_options(const std::string &filename,
                                           const shcore::Dictionary_t &options)
    : m_filename(filename) {
  m_table = std::get<0>(
      shcore::path::split_extension(shcore::path::basename(filename)));
  m_full_path = shcore::path::expand_user(filename);
  unpack(options);
}

void Import_table_options::validate() {
  m_dialect.validate();

  if (!m_base_session || !m_base_session->is_open() ||
      m_base_session->get_node_type().compare("mysql") != 0) {
    throw shcore::Exception::runtime_error(
        "A classic protocol session is required to perform this operation.");
  }

  if (m_schema.empty()) {
    m_schema = m_base_session->get_current_schema();
    if (m_schema.empty()) {
      throw std::runtime_error(
          "There is no active schema on the current session, the target "
          "schema for the import operation must be provided in the options.");
    }
  }

  {
    auto result = m_base_session->get_core_session()->query(
        "SHOW GLOBAL VARIABLES LIKE 'local_infile'");
    auto row = result->fetch_one();
    auto local_infile_value = row->get_string(1);

    if (shcore::str_caseeq(local_infile_value, "off")) {
      mysqlsh::current_console()->print_error(
          "The 'local_infile' global system variable must be set to ON in "
          "the target server, after the server is verified to be trusted.");
      throw shcore::Exception::runtime_error("Invalid preconditions");
    }
  }

  auto fd = detail::file_open(m_full_path);
  if (!(fd >= 0)) {
    throw std::runtime_error("Cannot open file '" + m_full_path + "'");
  } else {
    detail::file_close(fd);
  }

  m_file_size = shcore::file_size(m_full_path);
  m_threads_size = calc_thread_size();
}

size_t Import_table_options::calc_thread_size() {
  // We need at least one thread
  int64_t threads_size = std::max(static_cast<int64_t>(1), m_threads_size);

  // We do not need to spawn more threads than file chunks
  const size_t calculated_threads = (m_file_size / bytes_per_chunk()) + 1;
  if (calculated_threads <
      static_cast<size_t>(std::numeric_limits<int64_t>::max())) {
    threads_size =
        std::min(threads_size, static_cast<int64_t>(calculated_threads));
  }
  return threads_size;
}

size_t Import_table_options::max_rate() const {
  if (!m_max_rate.empty()) {
    return std::max(static_cast<size_t>(0),
                    mysqlshdk::utils::expand_to_bytes(m_max_rate));
  }
  return 0;
}

Connection_options Import_table_options::connection_options() const {
  Connection_options connection_options =
      m_base_session->get_connection_options();

  if (connection_options.has(mysqlshdk::db::kLocalInfile)) {
    connection_options.remove(mysqlshdk::db::kLocalInfile);
  }
  connection_options.set(mysqlshdk::db::kLocalInfile, "true");
  return connection_options;
}

size_t Import_table_options::bytes_per_chunk() const {
  constexpr const size_t min_bytes_per_chunk = 2 * BUFFER_SIZE;
  return std::max(mysqlshdk::utils::expand_to_bytes(m_bytes_per_chunk),
                  min_bytes_per_chunk);
}

std::string Import_table_options::target_import_info() const {
  auto connection_options = m_base_session->get_connection_options();
  std::string info_msg =
      "Importing from file '" + full_path() + "' to table `" + schema() +
      "`.`" + table() + "` in MySQL Server at " +
      connection_options.as_uri(mysqlshdk::db::uri::formats::only_transport()) +
      " using " + std::to_string(threads_size());
  info_msg += threads_size() == 1 ? " thread" : " threads";
  return info_msg;
}

void Import_table_options::unpack(const shcore::Dictionary_t &options) {
  auto unpack_options = Unpack_options(options);
  unpack_options.optional("dialect", &m_base_dialect_name);
  if (m_base_dialect_name.empty()) {
    // nop
  } else if (shcore::str_casecmp(m_base_dialect_name, "default") == 0) {
    // nop
  } else if (shcore::str_casecmp(m_base_dialect_name, "csv") == 0) {
    m_dialect = Dialect::csv();
  } else if (shcore::str_casecmp(m_base_dialect_name, "tsv") == 0) {
    m_dialect = Dialect::tsv();
  } else if (shcore::str_casecmp(m_base_dialect_name, "json") == 0) {
    m_dialect = Dialect::json();
  } else if (shcore::str_casecmp(m_base_dialect_name, "csv-unix") == 0) {
    m_dialect = Dialect::csv_unix();
  } else {
    throw shcore::Exception::argument_error(
        "dialect value must be csv, tsv, json or csv-unix.");
  }

  unpack_options.optional("table", &m_table)
      .optional("schema", &m_schema)
      .optional("threads", &m_threads_size)
      .optional("bytesPerChunk", &m_bytes_per_chunk)
      .optional("columns", &m_columns)
      .optional("fieldsTerminatedBy", &m_dialect.fields_terminated_by)
      .optional("fieldsEnclosedBy", &m_dialect.fields_enclosed_by)
      .optional("fieldsOptionallyEnclosed",
                &m_dialect.fields_optionally_enclosed)
      .optional("fieldsEscapedBy", &m_dialect.fields_escaped_by)
      .optional("linesTerminatedBy", &m_dialect.lines_terminated_by)
      .optional("replaceDuplicates", &m_replace_duplicates)
      .optional("maxRate", &m_max_rate)
      .optional("showProgress", &m_show_progress)
      .optional("skipRows", &m_skip_rows_count)
      .end();
}

}  // namespace import_table
}  // namespace mysqlsh
