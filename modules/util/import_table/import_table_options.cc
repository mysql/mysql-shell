/*
 * Copyright (c) 2018, 2024, Oracle and/or its affiliates.
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

#include "modules/util/import_table/import_table_options.h"

#include <errno.h>

#include <algorithm>
#include <array>
#include <limits>
#include <stack>
#include <stdexcept>
#include <utility>

#include "modules/mod_utils.h"
#include "modules/util/import_table/helpers.h"
#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/libs/storage/compressed_file.h"
#include "mysqlshdk/libs/storage/utils.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"

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
          .include<common::Common_options>()
          .optional("table", &Import_table_option_pack::m_table)
          .optional("schema", &Import_table_option_pack::m_schema)
          .optional("threads", &Import_table_option_pack::m_threads_size)
          .optional("bytesPerChunk",
                    &Import_table_option_pack::set_bytes_per_chunk)
          .optional("maxBytesPerTransaction",
                    &Import_table_option_pack::set_max_transaction_size)
          .optional("columns", &Import_table_option_pack::set_columns)
          .optional("replaceDuplicates",
                    &Import_table_option_pack::set_replace_duplicates)
          .optional("maxRate", &Import_table_option_pack::set_max_rate)
          .optional("skipRows", &Import_table_option_pack::m_skip_rows_count)
          .optional("decodeColumns",
                    &Import_table_option_pack::set_decode_columns)
          .optional("characterSet", &Import_table_option_pack::m_character_set)
          .optional("sessionInitSql",
                    &Import_table_option_pack::m_session_init_sql)
          .include(&Import_table_option_pack::m_dialect);

  return opts;
}

Import_table_option_pack::Import_table_option_pack()
    : Common_options(Common_options::Config{"util.importTable", true, true}) {}

void Import_table_option_pack::set_filenames(
    const std::vector<std::string> &filenames) {
  m_filelist_from_user = filenames;

  // remove empty paths provided by user
  m_filelist_from_user.erase(
      std::remove_if(m_filelist_from_user.begin(), m_filelist_from_user.end(),
                     [](const auto &s) { return s.empty(); }),
      m_filelist_from_user.end());

  if (m_filelist_from_user.empty()) {
    throw std::runtime_error("File list cannot be empty.");
  }

  for (const auto &file : m_filelist_from_user) {
    throw_on_url_and_storage_conflict(file, "importing from a URL");
  }

  m_is_multifile = check_if_multifile();

  if (!is_multifile()) {
    // If loading single file, chunk size defaults to 50M if not specified by
    // the user.
    if (!m_bytes_per_chunk.has_value()) {
      m_bytes_per_chunk =
          mysqlshdk::utils::expand_to_bytes(k_default_chunk_size);
    }

    // If loading single file, table name is taken from the file if not
    // specified by the user.
    if (m_table.empty()) {
      m_table = std::get<0>(
          shcore::path::split_extension(shcore::path::basename(single_file())));
    }
  } else {
    if (m_bytes_per_chunk.has_value()) {
      throw std::runtime_error(
          "The 'bytesPerChunk' option cannot be used when loading from "
          "multiple files.");
    }
  }
}

void Import_table_option_pack::set_decode_columns(
    const shcore::Dictionary_t &decode_columns) {
  if (!decode_columns) {
    return;
  }

  for (const auto &it : *decode_columns) {
    if (it.second.get_type() != shcore::Null) {
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

  if (!m_decode_columns.empty() && m_columns.empty()) {
    throw std::runtime_error(
        "The 'columns' option is required when 'decodeColumns' is set.");
  }
}

void Import_table_option_pack::set_columns(const shcore::Array_t &columns) {
  if (!columns || columns->empty()) {
    throw std::runtime_error("The 'columns' option must be a non-empty list.");
  }

  for (const auto &c : *columns) {
    if (c.get_type() == shcore::Value_type::UInteger) {
      m_columns.emplace_back(c.as_uint());
    } else if (c.get_type() == shcore::Value_type::Integer) {
      const auto v = c.as_int();

      // We do not want user variable to be negative: `@-1`
      if (v < 0) {
        throw shcore::Exception::value_error(
            "User variable binding in 'columns' option must be non-negative "
            "integer value");
      }

      m_columns.emplace_back(static_cast<uint64_t>(v));
    } else if (c.get_type() == shcore::Value_type::String) {
      m_columns.emplace_back(c.as_string());
    } else {
      throw shcore::Exception::type_error(
          "Option 'columns' " + type_name(shcore::Value_type::String) +
          " (column name) or non-negative " +
          type_name(shcore::Value_type::Integer) +
          " (user variable binding) expected, but value is " +
          type_name(c.get_type()));
    }
  }
}

bool Import_table_option_pack::check_if_multifile() {
  if (m_filelist_from_user.size() == 1) {
    if (const auto storage = storage_config(m_filelist_from_user[0]);
        (storage && storage->valid()) ||
        shcore::OperatingSystem::WINDOWS_OS != shcore::get_os_type()) {
      // local non-Windows and remote filesystems allow for * and ? characters
      // in file names
      // BUG#35895247: if all wildcard characters are escaped, load the file in
      // chunks
      if (auto path = shcore::unescape_glob(m_filelist_from_user[0]);
          path.has_value()) {
        m_filelist_from_user[0] = std::move(*path);
        return false;
      } else {
        return true;
      }
    } else {
      return has_wildcard(m_filelist_from_user[0]);
    }
  }

  return true;
}

void Import_table_option_pack::set_replace_duplicates(bool flag) {
  if (flag) {
    set_duplicate_handling(Duplicate_handling::Replace);
  } else {
    set_duplicate_handling(Duplicate_handling::Ignore);
  }
}

Import_table_options::Import_table_options(
    const Import_table_option_pack &pack) {
  // Copies the options defined in pack into this object
  Import_table_option_pack &this_pack(*this);
  this_pack = pack;
}

Import_table_options Import_table_options::unpack(
    const shcore::Dictionary_t &options) {
  Import_table_options ret;
  Import_table_options::options().unpack(options, &ret);
  return ret;
}

void Import_table_options::on_validate() const {
  Common_options::on_validate();

  if (m_table.empty()) {
    throw std::runtime_error(
        "Target table is not set. The target table for the import operation "
        "must be provided in the options.");
  }

  if (m_schema.empty()) {
    const auto res = session()->query("SELECT schema()");
    const auto row = res->fetch_one_or_throw();
    m_schema = row->get_string(0, {});

    if (m_schema.empty()) {
      throw std::runtime_error(
          "There is no active schema on the current session, the target "
          "schema for the import operation must be provided in the options.");
    }
  }
}

void Import_table_options::on_configure() {
  Common_options::on_configure();

  if (!is_multifile()) {
    const auto handle = make_file(single_file());

    // open the file to check if it's readable
    handle->open(mysqlshdk::storage::Mode::READ);
    handle->close();

    // cache some values
    m_file_size = handle->file_size();
    m_full_path = handle->full_path().masked();
  }

  m_threads_size = calc_thread_size();
}

size_t Import_table_options::calc_thread_size() {
  // We need at least one thread
  int64_t threads_size = std::max(static_cast<int64_t>(1), m_threads_size);

  if (!is_multifile()) {
    if (dialect_supports_chunking()) {
      if (!mysqlshdk::storage::is_compressed(single_file())) {
        // We do not need to spawn more threads than file chunks
        const size_t calculated_threads = (m_file_size / bytes_per_chunk()) + 1;
        if (calculated_threads <
            static_cast<size_t>(std::numeric_limits<int64_t>::max())) {
          threads_size =
              std::min(threads_size, static_cast<int64_t>(calculated_threads));
        }
      }
    } else {
      threads_size = 1;
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

uint64_t Import_table_option_pack::bytes_per_chunk() const {
  if (m_bytes_per_chunk.has_value()) {
    return *m_bytes_per_chunk;
  } else {
    // at this point m_bytes_per_chunk should have a value
    assert(false);
    return mysqlshdk::utils::expand_to_bytes(k_default_chunk_size);
  }
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
  const auto uri = session()->get_connection_options().as_uri(
      mysqlshdk::db::uri::formats::only_transport());
  std::string info_msg = "Importing from ";

  if (is_multifile()) {
    info_msg += "multiple files";
  } else {
    info_msg += "file '";
    info_msg += masked_full_path();
    info_msg += '\'';
  }

  info_msg += " to table `" + schema() + "`.`" + table() +
              "` in MySQL Server at " + uri + " using " +
              std::to_string(threads_size()) + " thread";

  if (1 != threads_size()) {
    info_msg += 's';
  }

  return info_msg;
}

bool Import_table_options::dialect_supports_chunking() const {
  return !m_dialect.lines_terminated_by.empty() &&
         m_dialect.lines_terminated_by != m_dialect.fields_terminated_by;
}

}  // namespace import_table
}  // namespace mysqlsh
