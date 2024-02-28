/*
 * Copyright (c) 2020, 2024, Oracle and/or its affiliates.
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

#include "modules/util/dump/dump_options.h"

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include <mutex>

#include <algorithm>
#include <iterator>

#include "modules/util/common/dump/utils.h"
#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/db/mysql/result.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/libs/utils/utils_lexing.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace dump {

Dump_options::Dump_options()
    : m_show_progress(isatty(fileno(stdout)) ? true : false) {}

const shcore::Option_pack_def<Dump_options> &Dump_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Dump_options>()
          .on_start(&Dump_options::on_start_unpack)
          .optional("maxRate", &Dump_options::set_string_option)
          .optional("showProgress", &Dump_options::m_show_progress)
          .optional("compression", &Dump_options::set_string_option)
          .optional("defaultCharacterSet", &Dump_options::m_character_set)
          .include(&Dump_options::m_dialect_unpacker)
          .on_done(&Dump_options::on_unpacked_options)
          .on_log(&Dump_options::on_log_options);

  return opts;
}

const mysqlshdk::utils::Version &Dump_options::current_version() {
  static const auto k_current_version = mysqlshdk::utils::Version(MYSH_VERSION);
  return k_current_version;
}

void Dump_options::on_start_unpack(const shcore::Dictionary_t &options) {
  m_options = options;
}

void Dump_options::set_string_option(const std::string &option,
                                     const std::string &value) {
  if (option == "maxRate") {
    if (!value.empty()) {
      m_max_rate = mysqlshdk::utils::expand_to_bytes(value);
    }
  } else if (option == "compression") {
    if (value.empty()) {
      throw std::invalid_argument(
          "The option 'compression' cannot be set to an empty string.");
    }

    m_compression =
        mysqlshdk::storage::to_compression(value, &m_compression_options);
  } else {
    // This function should only be called with the options above.
    assert(false);
  }
}

void Dump_options::set_storage_config(
    std::shared_ptr<mysqlshdk::storage::Config> storage_config) {
  m_storage_config = std::move(storage_config);
}

void Dump_options::on_log_options(const char *msg) const {
  log_info("Dump options: %s", msg);
}

void Dump_options::set_session(
    const std::shared_ptr<mysqlshdk::db::ISession> &session) {
  m_session = session;

  on_set_session(session);
}

void Dump_options::on_unpacked_options() {
  m_dialect = m_dialect_unpacker;

  if (import_table::Dialect::json() == dialect()) {
    throw std::invalid_argument("The 'json' dialect is not supported.");
  }
}

void Dump_options::validate() const {
  // cross-filter conflicts cannot be checked earlier, as filters are not
  // initialized at the same time
  m_filter_conflicts |= filters().tables().error_on_cross_filters_conflicts();
  m_filter_conflicts |= filters().events().error_on_cross_filters_conflicts();
  m_filter_conflicts |= filters().routines().error_on_cross_filters_conflicts();
  m_filter_conflicts |= filters().triggers().error_on_cross_filters_conflicts();

  if (m_filter_conflicts) {
    throw std::invalid_argument("Conflicting filtering options");
  }

  validate_partitions();

  validate_options();
}

bool Dump_options::exists(const std::string &schema) const {
  return find_missing({schema}).empty();
}

bool Dump_options::exists(const std::string &schema,
                          const std::string &table) const {
  return find_missing(schema, {table}).empty();
}

std::set<std::string> Dump_options::find_missing(
    const std::unordered_set<std::string> &schemas) const {
  return find_missing_impl(
      "SELECT SCHEMA_NAME AS name FROM information_schema.schemata", schemas);
}

std::set<std::string> Dump_options::find_missing(
    const std::string &schema,
    const std::unordered_set<std::string> &tables) const {
  return find_missing_impl(
      shcore::sqlstring("SELECT TABLE_NAME AS name "
                        "FROM information_schema.tables WHERE TABLE_SCHEMA = ?",
                        0)
          << schema,
      tables);
}

std::set<std::string> Dump_options::find_missing_impl(
    const std::string &subquery,
    const std::unordered_set<std::string> &objects) const {
  std::string query =
      "SELECT n.name FROM (SELECT " +
      shcore::str_join(objects, " UNION SELECT ",
                       [&objects](const std::string &v) {
                         return (shcore::sqlstring("?", 0) << v).str() +
                                (v == *objects.begin() ? " AS name" : "");
                       });

  query += ") AS n LEFT JOIN (" + subquery +
           ") AS o ON STRCMP(o.name COLLATE utf8_bin, n.name)=0 "
           "WHERE o.name IS NULL";

  const auto result = session()->query(query);
  std::set<std::string> missing;

  while (const auto row = result->fetch_one()) {
    missing.emplace(row->get_string(0));
  }

  return missing;
}

void Dump_options::set_where_clause(
    const std::map<std::string, std::string> &where) {
  std::string schema;
  std::string table;

  for (const auto &w : where) {
    schema.clear();
    table.clear();

    common::parse_schema_and_object(w.first,
                                    "table name key of the 'where' option",
                                    "table", &schema, &table);

    set_where_clause(schema, table, w.second);
  }
}

void Dump_options::set_where_clause(const std::string &schema,
                                    const std::string &table,
                                    const std::string &where) {
  if (where.empty()) {
    return;
  }

  // prevent SQL injection
  mysqlshdk::utils::SQL_iterator it{where};
  int parentheses = 0;

  const auto throw_error = [&schema, &table, &where]() {
    throw std::invalid_argument("Malformed condition used for table '" +
                                schema + "'.'" + table + "': " + where);
  };

  while (it.valid()) {
    const auto token = it.next_token();

    if (shcore::str_caseeq("(", token)) {
      ++parentheses;
    } else if (shcore::str_caseeq(")", token)) {
      --parentheses;

      if (parentheses < 0) {
        throw_error();
      }
    }
  }

  if (parentheses != 0) {
    throw_error();
  }

  m_where[schema][table] = '(' + where + ')';
}

void Dump_options::set_partitions(
    const std::map<std::string, std::unordered_set<std::string>> &partitions) {
  std::string schema;
  std::string table;

  for (const auto &p : partitions) {
    schema.clear();
    table.clear();

    common::parse_schema_and_object(p.first,
                                    "table name key of the 'partitions' option",
                                    "table", &schema, &table);

    set_partitions(schema, table, p.second);
  }
}

void Dump_options::set_partitions(
    const std::string &schema, const std::string &table,
    const std::unordered_set<std::string> &partitions) {
  if (partitions.empty()) {
    return;
  }

  m_partitions[schema][table] = partitions;
}

const std::string &Dump_options::where(const std::string &schema,
                                       const std::string &table) const {
  static std::string def;

  const auto s = m_where.find(schema);

  if (m_where.end() == s) {
    return def;
  }

  const auto t = s->second.find(table);

  if (s->second.end() == t) {
    return def;
  }

  return t->second;
}

void Dump_options::validate_partitions() const {
  bool valid = true;
  const auto console = current_console();

  for (const auto &schema : m_partitions) {
    for (const auto &table : schema.second) {
      if (!exists(schema.first, table.first)) {
        log_warning(
            "Table '%s'.'%s' does not exist, 'partition' option was ignored "
            "for this table",
            schema.first.c_str(), table.first.c_str());
        continue;
      }

      const auto condition = shcore::sqlformat(
          "PARTITION_NAME IS NOT NULL AND TABLE_SCHEMA=? AND TABLE_NAME=?",
          schema.first, table.first);
      const auto missing = find_missing_impl(
          "SELECT PARTITION_NAME AS name FROM information_schema.partitions "
          "WHERE " +
              condition +
              " UNION SELECT SUBPARTITION_NAME FROM "
              "information_schema.partitions WHERE SUB" +
              condition,
          table.second);

      if (!missing.empty()) {
        console->print_error(
            "Following partitions were not found in table '" + schema.first +
            "'.'" + table.first +
            "': " + shcore::str_join(missing, ", ", [](const std::string &p) {
              return "'" + p + "'";
            }));
        valid = false;
      }
    }
  }

  if (!valid) {
    throw std::invalid_argument("Invalid partitions");
  }
}

void Dump_options::set_target_version(const mysqlshdk::utils::Version &version,
                                      bool validate) {
  if (validate) {
    const auto k_minimum_version = mysqlshdk::utils::Version(8, 0, 25);

    if (version > current_version()) {
      throw std::invalid_argument("Requested MySQL version '" +
                                  version.get_base() +
                                  "' is newer than the maximum version '" +
                                  current_version().get_base() +
                                  "' supported by this version of MySQL Shell");
    } else if (version < k_minimum_version) {
      // 8.0.25 is the minimum MDS version we support
      throw std::invalid_argument("Requested MySQL version '" +
                                  version.get_base() +
                                  "' is older than the minimum version '" +
                                  k_minimum_version.get_base() +
                                  "' supported by this version of MySQL Shell");
    }
  }

  m_target_version = version;
}

}  // namespace dump
}  // namespace mysqlsh
