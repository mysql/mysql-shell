/*
 * Copyright (c) 2018, 2020, Oracle and/or its affiliates. All rights reserved.
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
#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/oci/oci_options.h"
#include "mysqlshdk/libs/storage/backend/oci_object_storage.h"
#include "mysqlshdk/libs/storage/compressed_file.h"
#include "mysqlshdk/libs/storage/ifile.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_path.h"

namespace mysqlsh {
namespace import_table {

Import_table_options::Import_table_options(const std::string &filename,
                                           const shcore::Dictionary_t &options)
    : m_filename(filename),
      m_oci_options(
          mysqlshdk::oci::Oci_options::Unpack_target::OBJECT_STORAGE) {
  m_table = std::get<0>(
      shcore::path::split_extension(shcore::path::basename(filename)));
  unpack(options);
}

Import_table_options::Import_table_options(const shcore::Dictionary_t &options)
    : m_oci_options(
          mysqlshdk::oci::Oci_options::Unpack_target::OBJECT_STORAGE) {
  unpack(options);
}

void Import_table_options::validate() {
  m_dialect.validate();

  if (m_schema.empty()) {
    auto res = m_base_session->query("SELECT schema()");
    auto row = res->fetch_one_or_throw();
    m_schema = row->is_null(0) ? "" : row->get_string(0);
    if (m_schema.empty()) {
      throw std::runtime_error(
          "There is no active schema on the current session, the target "
          "schema for the import operation must be provided in the options.");
    }
  }

  {
    auto result =
        m_base_session->query("SHOW GLOBAL VARIABLES LIKE 'local_infile'");
    auto row = result->fetch_one();
    auto local_infile_value = row->get_string(1);

    if (shcore::str_caseeq(local_infile_value, "off")) {
      mysqlsh::current_console()->print_error(
          "The 'local_infile' global system variable must be set to ON in "
          "the target server, after the server is verified to be trusted.");
      throw shcore::Exception::runtime_error("Invalid preconditions");
    }
  }

  if (!m_filename.empty() || m_oci_options) {
    mysqlshdk::oci::parse_oci_options(mysqlshdk::oci::Oci_uri_type::FILE,
                                      m_filename, {}, &m_oci_options,
                                      &m_filename);

    m_file_handle = create_file_handle();
    m_file_handle->open(mysqlshdk::storage::Mode::READ);
    m_full_path = m_file_handle->full_path();
    m_file_size = m_file_handle->file_size();
    m_file_handle->close();
    m_threads_size = calc_thread_size();
  }
}

std::unique_ptr<mysqlshdk::storage::IFile>
Import_table_options::create_file_handle() const {
  std::unique_ptr<mysqlshdk::storage::IFile> file;

  if (!m_oci_options.os_bucket_name.is_null()) {
    file = mysqlshdk::storage::make_file(m_filename, m_oci_options);
  } else {
    file = mysqlshdk::storage::make_file(m_filename);
  }

  mysqlshdk::storage::Compression compr;
  try {
    compr = mysqlshdk::storage::from_extension(
        std::get<1>(shcore::path::split_extension(m_filename)));
  } catch (...) {
    compr = mysqlshdk::storage::Compression::NONE;
  }

  return mysqlshdk::storage::make_file(std::move(file), compr);
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

  // Set long timeouts by default
  const std::string timeout =
      std::to_string(24 * 3600 * 1000);  // 1 day in milliseconds
  if (!connection_options.has(mysqlshdk::db::kNetReadTimeout)) {
    connection_options.set(mysqlshdk::db::kNetReadTimeout, timeout);
  }
  if (!connection_options.has(mysqlshdk::db::kNetWriteTimeout)) {
    connection_options.set(mysqlshdk::db::kNetWriteTimeout, timeout);
  }

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

  unpack_options.unpack(&m_oci_options);

  m_dialect = Dialect::unpack(&unpack_options);
  shcore::Dictionary_t decode_columns = nullptr;

  unpack_options.optional("table", &m_table)
      .optional("schema", &m_schema)
      .optional("threads", &m_threads_size)
      .optional("bytesPerChunk", &m_bytes_per_chunk)
      .optional("columns", &m_columns)
      .optional("replaceDuplicates", &m_replace_duplicates)
      .optional("maxRate", &m_max_rate)
      .optional("showProgress", &m_show_progress)
      .optional("skipRows", &m_skip_rows_count)
      .optional("decodeColumns", &decode_columns)
      .optional("defaultCharacterSet", &m_character_set);

  unpack_options.end();

  if (decode_columns) {
    for (const auto &it : *decode_columns) {
      if (it.second.type != shcore::Null) {
        std::string method = shcore::str_upper(it.second.descr());
        if (method != "UNHEX" && method != "FROM_BASE64" && !method.empty())
          throw std::runtime_error("Invalid value for decodeColumns: " +
                                   method);
        if (!method.empty()) m_decode_columns[it.first] = method;
      }
    }
  }
}

}  // namespace import_table
}  // namespace mysqlsh
