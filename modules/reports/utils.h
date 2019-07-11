/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_REPORTS_UTILS_H_
#define MODULES_REPORTS_UTILS_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/libs/db/session.h"

namespace Mysqlx {
namespace Expr {
class Expr;
}  // namespace Expr
}  // namespace Mysqlx

namespace mysqlsh {
namespace reports {

/**
 * Creates a text report in a vertical format, similar to the results of a MySQL
 * query terminated with \G.
 *
 * Report is expected to be an array of arrays, with the first row holding
 * the headers, while the remaining ones hold the data.
 *
 * Report must contain at least the row with headers. If there are no data rows,
 * output is empty.
 *
 * @param report - report data
 *
 * @returns formatted report
 */
std::string vertical_formatter(const shcore::Array_t &report);

/**
 * Creates a text report in a table format, similar to the results of a MySQL
 * query.
 *
 * Report is expected to be an array of arrays, with the first row holding
 * the headers, while the remaining ones hold the data.
 *
 * Report must contain at least the row with headers. If there are no data rows,
 * output is empty.
 *
 * @param report - report data
 *
 * @returns formatted report
 */
std::string table_formatter(const shcore::Array_t &report);

/**
 * Creates a brief text report. Each line will contain a single data row with
 * headers and values.
 *
 * Report is expected to be an array of arrays, with the first row holding
 * the headers, while the remaining ones hold the data.
 *
 * Report must contain at least the row with headers. If there are no data rows,
 * output is empty.
 *
 * @param report - report data
 *
 * @returns formatted report
 */
std::string brief_formatter(const shcore::Array_t &report);

/**
 * Creates a status-like text report. Output of this formatter is similar to the
 * \status command.
 *
 * Report is expected to be an array of arrays, with the first row holding
 * the headers, while the remaining ones hold the data.
 *
 * Report must contain at least the row with headers. If there are no data rows,
 * output is empty.
 *
 * @param report - report data
 *
 * @returns formatted report
 */
std::string status_formatter(const shcore::Array_t &report);

/**
 * Specifies a mapping of a column ID to name presented to the user.
 */
struct Column_mapping {
  Column_mapping(const char *i = nullptr, const char *n = nullptr)
      : id(i), name(n) {}

  const char *id = nullptr;    //!< ID of a column
  const char *name = nullptr;  //!< name presented to the user
};

/**
 * Defines a column in a report based on a JSON object.
 */
struct Column_definition : public Column_mapping {
  /**
   * Allows to use the aggregate initialization.
   */
  Column_definition(const char *i = nullptr, const char *n = nullptr,
                    const char *q = nullptr,
                    const std::vector<Column_mapping> &m = {})
      : Column_mapping(i, n), query(q), mapping(m) {}

  const char *query =
      nullptr;  //!< column name or a query which is used to obtain the data
  std::vector<Column_mapping> mapping;  //!< used to rename subcolumns, if query
                                        //!< returns a complex JSON type
};

/**
 * Creates a report from the specified query. Each row returned by the query
 * should contain exactly one JSON object.
 *
 * If the 'columns' variable is empty, query to be executed will not be
 * modified.
 *
 * If the 'columns' variable is not empty, it is going to be used to construct
 * the contents of the resultant JSON object. It is also going to be used to
 * specify the column headers. Query to be executed will be appended to: 'SELECT
 * json_object(<columns>) '.
 *
 * If the 'return_columns' variable is empty, the contents of 'columns' variable
 * is going to be used instead.
 *
 * If both 'return_columns' and 'columns' variables are empty, the report will
 * contain JSON object keys as column headers.
 *
 * If the 'return_columns' variable is not empty, it is going to be used to
 * specify the column headers of the report.
 *
 * If a column holds a JSON object, it's contents is going to be merged with the
 * report, using the key names of the JSON sub-object as column headers. If
 * Column_definition::mapping of such column is not empty, it's going to be used
 * so specify the extra column headers instead.
 *
 * @param session - session used to execute the query
 * @param query - query to be executed
 * @param columns - definition of columns in the report
 * @param return_columns - columns to be returned in the report
 *
 * @returns created report
 */
shcore::Array_t create_report_from_json_object(
    const std::shared_ptr<mysqlshdk::db::ISession> &session,
    const std::string &query,
    const std::vector<Column_definition> &columns = {},
    const std::vector<Column_definition> &return_columns = {});

/**
 * Parses an SQL condition (i.e. WHERE, HAVING)
 */
class Sql_condition {
 public:
  /**
   * Validation callback.
   *
   * @param in - input string
   *
   * @returns input string after processing
   */
  using Validator = std::function<std::string(const std::string &in)>;

  /**
   * Creates a parser for SQL condition.
   *
   * @param ident - validator for an SQL identifier
   * @param op - validator for an SQL operator
   */
  explicit Sql_condition(const Validator &ident = identifier_validator,
                         const Validator &op = operator_validator);

  /**
   * Default validator of SQL identifiers.
   *
   * @param ident - identifier
   *
   * @returns input string
   */
  static std::string identifier_validator(const std::string &ident);

  /**
   * Default validator of SQL operators.
   *
   * Allows for the following operators: "==", "!=", ">", ">=", "<", "<=",
   * "like", "&&", "||", "not".
   *
   * @param op - operator
   *
   * @returns input string, or '=' in case of '=='
   */
  static std::string operator_validator(const std::string &op);

  /**
   * Parses and validates the given condition.
   *
   * Uses the validator callbacks.
   *
   * @param condition - condition to be parsed
   *
   * @returns preprocessed condition
   */
  std::string parse(const std::string &condition) const;

 private:
  std::string validate_expression(const Mysqlx::Expr::Expr &expr) const;

  Validator m_identifier;
  Validator m_operator;
};

}  // namespace reports
}  // namespace mysqlsh

#endif  // MODULES_REPORTS_UTILS_H_
