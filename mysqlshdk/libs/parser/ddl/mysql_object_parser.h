/*
 * Copyright (c) 2013, 2026, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_PARSER_DDL_MYSQL_OBJECT_PARSER_H_
#define MYSQLSHDK_LIBS_PARSER_DDL_MYSQL_OBJECT_PARSER_H_

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/libs/utils/version.h"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wextra-semi"
#endif

#include "mysqlshdk/libs/parser/mysql/MySQLLexer.h"
#include "mysqlshdk/libs/parser/mysql/MySQLParser.h"
#include "mysqlshdk/libs/parser/mysql/MySQLParserBaseListener.h"

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

namespace mysqlshdk {
namespace parser {

class Mysql_parser_context {
 public:
  virtual ~Mysql_parser_context() {}

  virtual void update_server_version(
      const mysqlshdk::utils::Version &newVersion) = 0;

  virtual void update_sql_mode(const std::string &mode) = 0;

  virtual std::vector<parsers::ParserErrorInfo> errors_with_offset(
      size_t offset) const = 0;

  virtual bool is_case_sensitive() const = 0;

  virtual const mysqlshdk::utils::Version &server_version() const = 0;

  // Identifier determination depends on e.g the sql mode, hence we need extra
  // handling.
  virtual bool is_identifier(size_t type) const = 0;

  virtual shcore::Dictionary_t parse_statement(const std::string &sql) = 0;

  virtual shcore::Dictionary_t parse_ddl_script(const std::string &sql,
                                                const std::string &schema,
                                                bool ignore_errors) = 0;
};

struct Parser_context_config {
  mysqlshdk::utils::Version version;
  bool ansi_quotes = false;
  bool no_backslash_escapes = false;
  bool case_sensitive = false;
};

std::shared_ptr<Mysql_parser_context> create_parser_context(
    const Parser_context_config &config);

shcore::Dictionary_t parse_statement(
    const std::shared_ptr<Mysql_parser_context> &context,
    const std::string &sql);

shcore::Dictionary_t parse_ddl_script(
    const std::shared_ptr<Mysql_parser_context> &context,
    const std::string &sql, const std::string &schema, bool ignore_errors);

struct Data_type_definition {
  std::string type;
  std::optional<int> precision;
  std::optional<int> scale;
  std::optional<int> length;
  std::string explicit_params;
  std::vector<std::string> flags;
};

bool parse_type_definition(const std::string &type_definition,
                           Data_type_definition *out_definition);

}  // namespace parser
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_PARSER_DDL_MYSQL_OBJECT_PARSER_H_
