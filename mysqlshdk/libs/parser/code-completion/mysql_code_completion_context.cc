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

#include "mysqlshdk/libs/parser/code-completion/mysql_code_completion_context.h"

#include <algorithm>
#include <set>

#include "mysqlshdk/libs/db/charset.h"
#include "mysqlshdk/libs/parser/code-completion/mysql-code-completion.h"
#include "mysqlshdk/libs/parser/mysql/MySQLLexer.h"
#include "mysqlshdk/libs/parser/mysql/MySQLParser.h"

namespace mysqlshdk {

using utils::Version;

namespace {

std::set<std::string> initialize_charsets() {
  std::set<std::string> result;

  for (const auto &charset : db::charset::charset_names()) {
    // character set literals need to begin with an underscore
    result.emplace("_" + std::string{charset});
  }

  return result;
}

}  // namespace

class Sql_completion_context::Impl {
 public:
  explicit Impl(const Version &version)
      : m_lexer(&m_input), m_tokens(&m_lexer), m_parser(&m_tokens) {
    set_server_version(version);

    static const auto k_charsets = initialize_charsets();
    m_lexer.charsets = k_charsets;

    m_lexer.removeErrorListeners();
    m_parser.removeParseListeners();
    m_parser.removeErrorListeners();
  }

  Sql_completion_result complete(const std::string &sql, std::size_t offset) {
    prepare(sql);
    return getCodeCompletion(offset, m_active_schema, m_uppercase_keywords,
                             m_filtered, &m_parser);
  }

  void set_server_version(const Version &version) {
    m_parser.serverVersion = version.numeric();

    if (m_parser.serverVersion != m_lexer.serverVersion) {
      resetPreconditions(&m_parser);
    }

    m_lexer.serverVersion = m_parser.serverVersion;
  }

  void set_sql_mode(const std::string &sql_mode) {
    m_parser.sqlModeFromString(sql_mode);

    if (m_parser.sqlMode != m_lexer.sqlMode) {
      resetPreconditions(&m_parser);
    }

    m_lexer.sqlMode = m_parser.sqlMode;
  }

  void set_uppercase_keywords(bool uppercase) {
    m_uppercase_keywords = uppercase;
  }

  void set_filtered(bool filtered) { m_filtered = filtered; }

  void set_active_schema(const std::string &active_schema) {
    m_active_schema = active_schema;
  }

 private:
  void prepare(const std::string &sql) {
    m_parser.reset();
    m_lexer.reset();
    m_input.load(sql);
    m_lexer.setInputStream(&m_input);
    m_tokens.setTokenSource(&m_lexer);
    m_parser.setTokenStream(&m_tokens);
  }

  antlr4::ANTLRInputStream m_input;
  parsers::MySQLLexer m_lexer;
  antlr4::CommonTokenStream m_tokens;
  parsers::MySQLParser m_parser;

  bool m_uppercase_keywords = true;
  bool m_filtered = true;
  std::string m_active_schema;
};

Sql_completion_context::Sql_completion_context(const Version &version)
    : m_impl(std::make_unique<Impl>(version)) {}

Sql_completion_context::~Sql_completion_context() = default;

Sql_completion_result Sql_completion_context::complete(const std::string &sql,
                                                       std::size_t offset) {
  return m_impl->complete(sql, offset);
}

void Sql_completion_context::set_server_version(const utils::Version &version) {
  m_impl->set_server_version(version);
}

void Sql_completion_context::set_sql_mode(const std::string &sql_mode) {
  m_impl->set_sql_mode(sql_mode);
}

void Sql_completion_context::set_uppercase_keywords(bool uppercase) {
  m_impl->set_uppercase_keywords(uppercase);
}

void Sql_completion_context::set_filtered(bool filtered) {
  m_impl->set_filtered(filtered);
}

void Sql_completion_context::set_active_schema(
    const std::string &active_schema) {
  m_impl->set_active_schema(active_schema);
}

}  // namespace mysqlshdk
