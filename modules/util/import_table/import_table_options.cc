/*
 * Copyright (c) 2018, 2022, Oracle and/or its affiliates.
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
#include <stack>
#include <utility>

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

namespace {
template <typename FwdIter>
FwdIter consume_string(FwdIter first, FwdIter last, const char qchar,
                       const char escchar) {
  if (first == last) return last;
  if (*first != qchar) return last;
  ++first;
  while (first != last) {
    if (*first == qchar) {
      return first;
    } else if (*first == escchar) {
      ++first;
      if (first == last) {
        return last;
      }
    }
    ++first;
  }
  return last;
}

bool transformation_validation(const std::string &expr) {
  std::stack<char> s;
  auto first = expr.begin();
  auto last = expr.end();
  while (first != last) {
    switch (*first) {
      case '[':
      case '(':
      case '{':
        s.push(*first);
        break;
      case ']':
        if (s.empty() || s.top() != '[') {
          return false;
        }
        s.pop();
        break;
      case ')':
        if (s.empty() || s.top() != '(') {
          return false;
        }
        s.pop();
        break;
      case '}':
        if (s.empty() || s.top() != '{') {
          return false;
        }
        s.pop();
        break;
      case '\'':
      case '"':
      case '`':
        first = consume_string(first, last, *first, '\\');
        if (first == last) return false;
        break;
    }
    ++first;
  }
  return s.empty();
}

bool has_wildcard(const std::string &s) {
  return s.find('*') != std::string::npos || s.find('?') != std::string::npos;
}
}  // namespace

namespace mysqlsh {
namespace import_table {

constexpr auto k_default_chunk_size = "50M";

const shcore::Option_pack_def<Import_table_option_pack>
    &Import_table_option_pack::options() {
  static const auto opts =
      shcore::Option_pack_def<Import_table_option_pack>()
          .optional("table", &Import_table_option_pack::m_table)
          .optional("schema", &Import_table_option_pack::m_schema)
          .optional("threads", &Import_table_option_pack::m_threads_size)
          .optional("bytesPerChunk",
                    &Import_table_option_pack::set_bytes_per_chunk)
          .optional("maxBytesPerTransaction",
                    &Import_table_option_pack::set_max_transaction_size)
          .optional("columns", &Import_table_option_pack::m_columns)
          .optional("replaceDuplicates",
                    &Import_table_option_pack::m_replace_duplicates)
          .optional("maxRate", &Import_table_option_pack::set_max_rate)
          .optional("showProgress", &Import_table_option_pack::m_show_progress)
          .optional("skipRows", &Import_table_option_pack::m_skip_rows_count)
          .optional("decodeColumns",
                    &Import_table_option_pack::set_decode_columns)
          .optional("characterSet", &Import_table_option_pack::m_character_set)
          .optional("sessionInitSql",
                    &Import_table_option_pack::m_session_init_sql)
          .include(&Import_table_option_pack::m_dialect)
          .include(&Import_table_option_pack::m_oci_options);

  return opts;
}

void Import_table_option_pack::set_filenames(
    const std::vector<std::string> &filenames) {
  m_filelist_from_user = filenames;

  if (!is_multifile()) {
    // If loading single file, chunk size defaults to 50M if not specified by
    // the user.
    if (m_bytes_per_chunk.is_null()) {
      m_bytes_per_chunk =
          mysqlshdk::utils::expand_to_bytes(k_default_chunk_size);
    }

    // If loading single file, table name is taken from the file if not
    // specified by the user.
    if (m_table.empty()) {
      m_table = std::get<0>(shcore::path::split_extension(
          shcore::path::basename(m_filelist_from_user[0])));
    }
  }
}

void Import_table_option_pack::set_decode_columns(
    const shcore::Dictionary_t &decode_columns) {
  if (!decode_columns) {
    return;
  }

  for (const auto &it : *decode_columns) {
    if (it.second.type != shcore::Null) {
      auto transformation = it.second.descr();
      if (shcore::str_caseeq(transformation, std::string{"UNHEX"}) ||
          shcore::str_caseeq(transformation, std::string{"FROM_BASE64"})) {
        m_decode_columns[it.first] = shcore::str_upper(transformation);
      } else {
        // Try to initially validate user input, i.e. check if
        // brackets are balanced in provided user input.
        if (!transformation_validation(transformation)) {
          throw std::runtime_error(
              "Invalid SQL expression in decodeColumns option "
              "for column '" +
              it.first + "'");
        }
        m_decode_columns[it.first] = std::move(transformation);
      }
    }
  }

  if (!m_decode_columns.empty()) {
    if (!m_columns) {
      throw std::runtime_error(
          "The 'columns' option is required when 'decodeColumns' is set.");
    }
    if (m_columns->empty()) {
      throw std::runtime_error(
          "The 'columns' option must be a non-empty list.");
    }
  }
}

bool Import_table_option_pack::is_multifile() const {
  if (m_filelist_from_user.size() == 1) {
    if (has_wildcard(m_filelist_from_user[0])) {
      return true;
    }
    // const auto &extension = std::get<1>(shcore::path::split_extension(path));
    // if (extension == ".gz" || extension == ".zst") {
    //   return true;
    // }
    return false;
  }
  return true;
}

Import_table_options::Import_table_options(
    const Import_table_option_pack &pack) {
  // Copies the options defined in pack into this object
  Import_table_option_pack &this_pack(*this);
  this_pack = pack;
}

void Import_table_options::validate() {
  if (m_table.empty()) {
    throw std::runtime_error(
        "Target table is not set. The target table for the import operation "
        "must be provided in the options.");
  }

  // remove empty paths provided by user
  m_filelist_from_user.erase(
      std::remove_if(m_filelist_from_user.begin(), m_filelist_from_user.end(),
                     [](const auto &s) { return s.empty(); }),
      m_filelist_from_user.end());

  if (m_filelist_from_user.empty()) {
    throw std::runtime_error("File list cannot be empty.");
  }

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
      throw std::runtime_error("Invalid preconditions");
    }
  }

  if (m_oci_options) {
    m_oci_options.check_option_values();
  }

  if (is_multifile()) {
    if (!m_bytes_per_chunk.is_null()) {
      throw std::runtime_error(
          "The 'bytesPerChunk' option cannot be used when loading from "
          "multiple files.");
    }
  } else {
    if (!m_filelist_from_user[0].empty()) {
      if (m_oci_options) {
        // this call is here to verify if filename does not have a scheme
        mysqlshdk::oci::parse_oci_options(m_filelist_from_user[0], {},
                                          &m_oci_options);
      }

      m_file_handle = create_file_handle(m_filelist_from_user[0]);
      m_file_handle->open(mysqlshdk::storage::Mode::READ);
      m_file_size = m_file_handle->file_size();
      m_file_handle->close();
    }
  }
  m_threads_size = calc_thread_size();
}

std::unique_ptr<mysqlshdk::storage::IFile>
Import_table_option_pack::create_file_handle(
    const std::string &filepath) const {
  std::unique_ptr<mysqlshdk::storage::IFile> file;

  if (!m_oci_options.os_bucket_name.is_null()) {
    file = mysqlshdk::storage::make_file(filepath, m_oci_options);
  } else {
    file = mysqlshdk::storage::make_file(filepath);
  }
  return create_file_handle(std::move(file));
}

std::unique_ptr<mysqlshdk::storage::IFile>
Import_table_option_pack::create_file_handle(
    std::unique_ptr<mysqlshdk::storage::IFile> file_handler) const {
  mysqlshdk::storage::Compression compr;
  try {
    compr = mysqlshdk::storage::from_extension(
        std::get<1>(shcore::path::split_extension(file_handler->filename())));
  } catch (...) {
    compr = mysqlshdk::storage::Compression::NONE;
  }
  return mysqlshdk::storage::make_file(std::move(file_handler), compr);
}

size_t Import_table_option_pack::calc_thread_size() {
  // We need at least one thread
  int64_t threads_size = std::max(static_cast<int64_t>(1), m_threads_size);

  if (!is_multifile()) {
    // We do not need to spawn more threads than file chunks
    const size_t calculated_threads = (m_file_size / bytes_per_chunk()) + 1;
    if (calculated_threads <
        static_cast<size_t>(std::numeric_limits<int64_t>::max())) {
      threads_size =
          std::min(threads_size, static_cast<int64_t>(calculated_threads));
    }
  }
  return threads_size;
}

uint64_t Import_table_option_pack::max_rate() const { return m_max_rate; }

void Import_table_option_pack::set_max_rate(const std::string &value) {
  if (!value.empty()) {
    m_max_rate = std::max<size_t>(0, mysqlshdk::utils::expand_to_bytes(value));
  }
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

uint64_t Import_table_option_pack::bytes_per_chunk() const {
  return *m_bytes_per_chunk;
}

void Import_table_option_pack::set_bytes_per_chunk(const std::string &value) {
  if (value.empty()) {
    return;
  }

  constexpr const size_t min_bytes_per_chunk = 2 * BUFFER_SIZE;
  m_bytes_per_chunk =
      std::max(mysqlshdk::utils::expand_to_bytes(value), min_bytes_per_chunk);
}

void Import_table_option_pack::set_max_transaction_size(
    const std::string &value) {
  if (value.empty()) {
    throw std::invalid_argument(
        "The option 'maxBytesPerTransaction' cannot be set to an empty "
        "string.");
  }

  m_max_bytes_per_transaction = mysqlshdk::utils::expand_to_bytes(value);

  // Minimal value of max_binlog_cache_size value is 4096.
  constexpr const uint64_t minimum_max_bytes_per_transaction = 4096;
  if (m_max_bytes_per_transaction < minimum_max_bytes_per_transaction) {
    throw std::invalid_argument(
        "The value of 'maxBytesPerTransaction' option must be greater than or "
        "equal to " +
        std::to_string(minimum_max_bytes_per_transaction) + " bytes.");
  }
}

size_t Import_table_option_pack::max_transaction_size() const {
  return m_max_bytes_per_transaction;
}

std::string Import_table_options::target_import_info() const {
  auto connection_options = m_base_session->get_connection_options();

  if (!is_multifile()) {
    std::string info_msg = "Importing from file '" + full_path().masked() +
                           "' to table `" + schema() + "`.`" + table() +
                           "` in MySQL Server at " +
                           connection_options.as_uri(
                               mysqlshdk::db::uri::formats::only_transport()) +
                           " using " + std::to_string(threads_size());
    info_msg += threads_size() == 1 ? " thread" : " threads";
    return info_msg;
  }
  std::string info_msg =
      "Importing from multiple files to table `" + schema() + "`.`" + table() +
      "` in MySQL Server at " +
      connection_options.as_uri(mysqlshdk::db::uri::formats::only_transport()) +
      " using " + std::to_string(threads_size());
  info_msg += threads_size() == 1 ? " thread" : " threads";
  return info_msg;
}

}  // namespace import_table
}  // namespace mysqlsh
