/*
 * Copyright (c) 2022, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_PARSER_CODE_COMPLETION_MYSQL_CODE_COMPLETION_RESULT_H_
#define MYSQLSHDK_LIBS_PARSER_CODE_COMPLETION_MYSQL_CODE_COMPLETION_RESULT_H_

#include <map>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

namespace parsers {

struct Sql_completion_result {
  using Names = std::set<std::string>;

  struct Table_reference {
    std::string schema;
    std::string table;
    std::string alias;
  };

  struct Prefix {
    std::string full;
    std::wstring full_wide;
    bool quoted = false;
    char quote = 0;
    std::wstring as_identifier;
    bool quoted_as_identifier = false;
    std::wstring as_string;
    bool quoted_as_string = false;
    std::wstring as_string_or_identifier;
  };

  struct Account_part {
    std::string full;
    bool quoted = false;
    char quote = 0;
    std::string unquoted;
  };

  struct Context {
    Prefix prefix;
    struct {
      Account_part user;
      Account_part host;
      bool has_at_sign = false;
    } as_account;
    std::vector<std::string> qualifier;
    std::vector<Table_reference> references;
    std::vector<std::string> labels;
  };

  enum class Candidate {
    SCHEMA,
    TABLE,
    VIEW,
    COLUMN,
    INTERNAL_COLUMN,
    PROCEDURE,
    FUNCTION,
    TRIGGER,
    EVENT,
    ENGINE,
    UDF,
    RUNTIME_FUNCTION,
    LOGFILE_GROUP,
    USER_VAR,
    SYSTEM_VAR,
    TABLESPACE,
    USER,
    CHARSET,
    COLLATION,
    PLUGIN,
    LABEL,
  };

  // schema -> tables/views
  using Columns = std::map<std::string, Names>;

  Context context;
  Names keywords;
  Names system_functions;
  Names tables;
  Names tables_from;
  Names views_from;
  Names functions_from;
  Names procedures_from;
  Names triggers_from;
  Names events_from;
  Columns columns;
  Columns internal_columns;
  std::unordered_set<Candidate> candidates;
};

}  // namespace parsers

#endif  // MYSQLSHDK_LIBS_PARSER_CODE_COMPLETION_MYSQL_CODE_COMPLETION_RESULT_H_
