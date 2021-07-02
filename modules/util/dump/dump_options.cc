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

#include "modules/util/dump/dump_options.h"

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include <mutex>

#include <algorithm>
#include <iterator>

#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/libs/utils/nullable.h"
#include "mysqlshdk/libs/utils/strformat.h"
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
          .on_log(&Dump_options::on_log_options);

  return opts;
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

    m_compression = mysqlshdk::storage::to_compression(value);
  } else {
    // This function should only be called with the options above.
    assert(false);
  }
}

void Dump_options::set_oci_options(
    const mysqlshdk::oci::Oci_options &oci_options) {
  m_oci_options = oci_options;

  m_oci_options.check_option_values();
}

void Dump_options::on_log_options(const char *msg) const {
  log_info("Dump options: %s", msg);
}

void Dump_options::set_session(
    const std::shared_ptr<mysqlshdk::db::ISession> &session) {
  m_session = session;

  on_set_session(session);
}

void Dump_options::validate() const { validate_options(); }

bool Dump_options::exists(const std::string &schema) const {
  return find_missing({schema}).empty();
}

bool Dump_options::exists(const std::string &schema,
                          const std::string &table) const {
  return find_missing(schema, {table}).empty();
}

std::set<std::string> Dump_options::find_missing(
    const std::set<std::string> &schemas) const {
  return find_missing_impl(
      "SELECT SCHEMA_NAME AS name FROM information_schema.schemata", schemas);
}

std::set<std::string> Dump_options::find_missing(
    const std::string &schema, const std::set<std::string> &tables) const {
  return find_missing_impl(
      shcore::sqlstring("SELECT TABLE_NAME AS name "
                        "FROM information_schema.tables WHERE TABLE_SCHEMA = ?",
                        0)
          << schema,
      tables);
}

std::set<std::string> Dump_options::find_missing_impl(
    const std::string &subquery, const std::set<std::string> &objects) const {
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

  const auto result = m_session->query(query);
  std::set<std::string> missing;

  while (const auto row = result->fetch_one()) {
    missing.emplace(row->get_string(0));
  }

  return missing;
}

}  // namespace dump
}  // namespace mysqlsh
