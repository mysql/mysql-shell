/*
 * Copyright (c) 2018, 2025, Oracle and/or its affiliates.
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

#include "modules/util/import_table/dialect.h"

#include <algorithm>
#include <stdexcept>
#include <string_view>

#include "modules/util/import_table/helpers.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace import_table {

namespace {

// characters which are escaped
constexpr std::string_view k_escaped_chars = "ntrb0ZN";
// List of all possible characters of a numeric value text representation.
constexpr std::string_view k_numeric_chars = ".0123456789e+-";

}  // namespace

const shcore::Option_pack_def<Dialect> &Dialect::options() {
  static const auto opts =
      shcore::Option_pack_def<Dialect>()
          .optional("dialect", &Dialect::set_dialect)
          .optional("fieldsTerminatedBy", &Dialect::fields_terminated_by)
          .optional("fieldsEnclosedBy", &Dialect::fields_enclosed_by)
          .optional("fieldsOptionallyEnclosed",
                    &Dialect::fields_optionally_enclosed)
          .optional("fieldsEscapedBy", &Dialect::fields_escaped_by)
          .optional("linesTerminatedBy", &Dialect::lines_terminated_by)
          .on_done(&Dialect::on_unpacked_options);

  return opts;
}

void Dialect::set_dialect(const std::string & /*option*/,
                          const std::string &name) {
  if (!name.empty()) {
    if (shcore::str_caseeq(name, "default")) {
      // nop
    } else if (shcore::str_caseeq(name, "csv")) {
      *this = csv();
    } else if (shcore::str_caseeq(name, "tsv")) {
      *this = tsv();
    } else if (shcore::str_caseeq(name, "json")) {
      *this = json();
    } else if (shcore::str_caseeq(name, "csv-unix")) {
      *this = csv_unix();
    } else {
      throw shcore::Exception::argument_error(
          "dialect value must be default, csv, tsv, json or csv-unix.");
    }
  }
}

bool Dialect::operator==(const Dialect &d) const {
  return lines_terminated_by == d.lines_terminated_by &&
         fields_escaped_by == d.fields_escaped_by &&
         fields_terminated_by == d.fields_terminated_by &&
         fields_enclosed_by == d.fields_enclosed_by &&
         fields_optionally_enclosed == d.fields_optionally_enclosed &&
         lines_starting_by == d.lines_starting_by;
}

void Dialect::validate() const {
  // TODO(pawel): remove this when chunk_file.h is no longer used
  if (!((lines_terminated_by.size() / 2) < BUFFER_SIZE)) {
    throw std::invalid_argument("Line terminator string is too long.");
  }

  if (fields_escaped_by.size() > 1) {
    throw std::invalid_argument("fieldsEscapedBy must be empty or a char.");
  }

  // TODO(pawel): this seems wrong or incomplete, docs have the same comment and
  // an example which uses FIELDS TERMINATED BY and FIELDS ENCLOSED BY; instead
  // data should be handled by the server and if it's not possible to parse it,
  // an error should be reported

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

  if (fields_terminated_by.empty() && fields_enclosed_by.empty()) {
    throw std::invalid_argument(
        "The fieldsTerminatedBy and fieldsEnclosedBy are both empty, resulting "
        "in a fixed-row format. This is currently not supported.");
  }

  // based on Query_result_export::prepare(), server issues a warning in this
  // case
  if (!fields_terminated_by.empty() &&
      ((fields_enclosed_by.empty() &&
        std::string_view::npos !=
            k_escaped_chars.find(fields_terminated_by[0])) ||
       (fields_optionally_enclosed &&
        std::string_view::npos !=
            k_numeric_chars.find(fields_terminated_by[0])))) {
    throw std::invalid_argument(shcore::str_format(
        "First character of the fieldsTerminatedBy string is ambiguous: %c, "
        "please use non-empty fieldsEnclosedBy and set "
        "fieldsOptionallyEnclosed to false.",
        fields_terminated_by[0]));
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

std::string Dialect::build_sql() const {
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

void Dialect::on_unpacked_options() {
  validate();

  // TODO(pawel): values provided by the user should not be altered, instead
  // code which uses these two sequences should handle this case properly

  // If LINES TERMINATED BY is an empty string and FIELDS TERMINATED BY is
  // nonempty, lines are also terminated with FIELDS TERMINATED BY.
  if (lines_terminated_by.empty() && !fields_terminated_by.empty()) {
    lines_terminated_by = fields_terminated_by;
  }
}

}  // namespace import_table
}  // namespace mysqlsh
