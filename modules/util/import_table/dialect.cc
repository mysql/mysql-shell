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

#include <algorithm>
#include <stdexcept>

#include "modules/util/import_table/dialect.h"
#include "modules/util/import_table/helpers.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace import_table {

void Dialect::validate() {
  if (!((lines_terminated_by.size() / 2) < BUFFER_SIZE)) {
    throw std::invalid_argument("Line terminator string is too long.");
  }

  if (fields_escaped_by.size() > 1) {
    throw std::invalid_argument("fieldsEscapedBy must be empty or a char.");
  }

  // If you specify one separator that is the same as or a prefix of another,
  // LOAD DATA INFILE cannot interpret the input properly.
  if (!lines_terminated_by.empty() && !fields_escaped_by.empty()) {
    auto index =
        std::search(lines_terminated_by.begin(), lines_terminated_by.end(),
                    fields_escaped_by.begin(), fields_escaped_by.end());
    if (index == lines_terminated_by.begin()) {
      throw std::invalid_argument(
          "Separators cannot be the same or be a prefix of another.");
    }
  }

  if (fields_enclosed_by.size() > 1) {
    throw std::invalid_argument("fieldsEnclosedBy must be empty or a char.");
  }

  if (fields_optionally_enclosed && fields_enclosed_by.empty()) {
    throw std::invalid_argument(
        "fieldsEnclosedBy must be set if fieldsOptionallyEnclosed is true.");
  }

  // If LINES TERMINATED BY is an empty string and FIELDS TERMINATED BY is
  // nonempty, lines are also terminated with FIELDS TERMINATED BY.
  if (lines_terminated_by.empty() && !fields_terminated_by.empty()) {
    lines_terminated_by = fields_terminated_by;
  }
}

Dialect Dialect::json() {
  Dialect dialect;
  dialect.lines_terminated_by = std::string{"\n"};
  dialect.fields_escaped_by = std::string{};
  dialect.fields_terminated_by = std::string{"\n"};
  dialect.fields_enclosed_by = std::string{};
  dialect.fields_optionally_enclosed = false;
  dialect.lines_starting_by = std::string{};
  return dialect;
}

Dialect Dialect::csv() {
  Dialect dialect;
  dialect.lines_terminated_by = std::string{"\r\n"};
  dialect.fields_escaped_by = std::string{"\\"};
  dialect.fields_terminated_by = std::string{","};
  dialect.fields_enclosed_by = std::string{"\""};
  dialect.fields_optionally_enclosed = true;
  dialect.lines_starting_by = std::string{""};
  return dialect;
}

Dialect Dialect::tsv() {
  Dialect dialect = csv();
  dialect.fields_terminated_by = std::string{"\t"};
  return dialect;
}

Dialect Dialect::csv_unix() {
  Dialect dialect;
  dialect.lines_terminated_by = std::string{"\n"};
  dialect.fields_escaped_by = std::string{"\\"};
  dialect.fields_terminated_by = std::string{","};
  dialect.fields_enclosed_by = std::string{"\""};
  dialect.fields_optionally_enclosed = false;
  dialect.lines_starting_by = std::string{""};
  return dialect;
}

std::string Dialect::build_sql() {
  using sqlstring = shcore::sqlstring;
  std::string sql =
      (sqlstring("FIELDS TERMINATED BY ?", 0) << fields_terminated_by).str();

  if (!fields_enclosed_by.empty()) {
    if (fields_optionally_enclosed) {
      sql += " OPTIONALLY";
    }
    sql += (sqlstring(" ENCLOSED BY ?", 0) << fields_enclosed_by).str();
  }

  sql += (sqlstring(" ESCAPED BY ? LINES STARTING BY ? TERMINATED BY ?", 0)
          << fields_escaped_by << lines_starting_by << lines_terminated_by)
             .str();
  return sql;
}
}  // namespace import_table
}  // namespace mysqlsh
