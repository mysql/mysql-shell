/*
  Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License, version 2.0,
  as published by the Free Software Foundation.

  This program is also distributed with certain software (including
  but not limited to OpenSSL) that is licensed under separate terms, as
  designated in a particular file or component or in included license
  documentation.  The authors of MySQL hereby grant you an additional
  permission to link the program and your derivative works with the
  separately licensed software that they have included with MySQL.
  This program is distributed in the hope that it will be useful,  but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
  the GNU General Public License, version 2.0, for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#ifndef MYSQLD_MOCK_JSON_STATEMENT_READER_INCLUDED
#define MYSQLD_MOCK_JSON_STATEMENT_READER_INCLUDED

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace server_mock {

/** @class QueriesJsonReader
 *
 * @brief  Responsible for reading the json file with
 *         the expected statements and simplifying the data
 *         structures used by RapidJSON into vectors.
 **/
class QueriesJsonReader {
 public:

  /** @brief Constructor.
   *
   * @param filename Path to the json file with definitins
   *         of the expected SQL statements and responses
   **/
  QueriesJsonReader(const std::string &filename);

  /** @enum statement_result_type
   *
   * Response expected for given SQL statement.
   **/
  enum class statement_result_type {
     STMT_RES_OK, STMT_RES_ERROR, STMT_RES_RESULT
  };

  /** @brief Map for keeping column metadata parameter-values pairs
   *
   **/
  using column_info_type = std::map<std::string, std::string>;

  /** @brief Vector for keeping string representation of the values
   *         of the single row (ordered by column)
   **/
  using row_values_type = std::vector<std::string>;

  /** @brief Keeps result data for single SQL statement that returns
   *         resultset.
   **/
  struct resultset_type {
    std::vector<column_info_type> columns;
    std::vector<row_values_type> rows;
  };

  /** @brief Keeps single SQL statement data.
   **/
  struct statement_info {
    // true if statement is a regex
    bool statement_is_regex{false};
    // SQL statement
    std::string statement;
    // exected response type for the statement
    statement_result_type result_type;
    // result data if the expected response is resultset
    resultset_type resultset;
  };

  /** @brief Returns the data about the next statement from the
   *         json file. If there is no more statements it returns
   *         empty statement.
   **/
  statement_info get_next_statement();

  ~QueriesJsonReader();
private:
  // This is to avoid including RapidJSON headers here, which would cause
  // them included also in other files (they give tons of warnings, which
  // better suppres only for single implementation file).
  struct Pimpl;
  std::unique_ptr<Pimpl> pimpl_;
};


} // namespace

#endif // MYSQLD_MOCK_JSON_STATEMENT_READER_INCLUDED
