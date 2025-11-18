/*
 * Copyright (c) 2022, 2025, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_PARSER_MYSQL_PARSER_UTILS_H_
#define MYSQLSHDK_LIBS_PARSER_MYSQL_PARSER_UTILS_H_

#include <cassert>
#include <functional>
#include <memory>
#include <ostream>
#include <stack>
#include <string>
#include <string_view>
#include <vector>

#include "mysqlshdk/libs/utils/version.h"

namespace mysqlshdk {
namespace parser {

struct AST_rule_node {
  std::string name;
  int rule_index;
};

struct AST_terminal_node {
  std::string name;
  std::string text;
  int terminal_symbol;
  int offset;
};

struct AST_error_node {
  std::string name;
  std::string text;
  int terminal_symbol;
  int line;
  int offset;
};

struct IParser_listener {
  virtual ~IParser_listener() = default;

  virtual void on_rule(const AST_rule_node &node, bool enter) = 0;

  virtual void on_terminal(const AST_terminal_node &node) = 0;

  virtual void on_error(const AST_error_node &node) = 0;
};

template <typename T>
class Parser_listener : public IParser_listener {
 public:
  explicit Parser_listener(T *root) { m_contexts.emplace(root); }

  ~Parser_listener() override = default;

  virtual T *on_rule_enter(const AST_rule_node &node, T *) = 0;

  virtual void on_rule_exit(const AST_rule_node &node, T *) = 0;

  virtual void on_terminal(const AST_terminal_node &node, T *) = 0;

  virtual void on_error(const AST_error_node &node, T *) = 0;

 private:
  void on_rule(const AST_rule_node &node, bool enter) override {
    if (enter) {
      m_contexts.push(on_rule_enter(node, m_contexts.top()));
    } else {
      m_contexts.pop();
      assert(!m_contexts.empty());
      on_rule_exit(node, m_contexts.top());
    }
  }

  void on_terminal(const AST_terminal_node &node) override {
    on_terminal(node, m_contexts.top());
  }

  void on_error(const AST_error_node &node) override {
    on_error(node, m_contexts.top());
  }

  std::stack<T *> m_contexts;
};

struct Parser_config {
  mysqlshdk::utils::Version mysql_version;
  bool ansi_quotes = false;
  bool no_backslash_escapes = false;
};

class Sql_syntax_error : public std::runtime_error {
 public:
  Sql_syntax_error(const std::string &msg,
                   const std::string &offending_token_text, size_t line,
                   size_t offset)
      : std::runtime_error(msg),
        m_token_text(offending_token_text),
        m_line(line),
        m_offset(offset) {}

  const std::string &token_text() const { return m_token_text; }
  size_t line() const { return m_line; }
  size_t offset() const { return m_offset; }

 private:
  std::string m_token_text;
  size_t m_line;
  size_t m_offset;
};

void traverse_statement_ast(std::string_view stmt, const Parser_config &config,
                            IParser_listener *listener);

void traverse_script_ast(
    const std::string &script, const Parser_config &config,
    const std::function<void(std::string_view, size_t, bool enter)> &on_stmt,
    IParser_listener *listener);

void check_sql_syntax(const std::string &script,
                      const Parser_config &config = {});

bool tokenize_statement(
    std::string_view stmt, const Parser_config &config,
    const std::function<bool(std::string_view, std::string_view)> &visitor);

struct Table_reference {
  std::string schema;
  std::string table;

  bool operator==(const Table_reference &other) const;

  bool operator<(const Table_reference &other) const;

  // helper used by gtest
  friend std::ostream &operator<<(std::ostream &os, const Table_reference &r);
};

/**
 * Extracts all table references from the given statement. If reference is not
 * fully qualified, schema name is going to be empty.
 *
 * NOTE: Statements needs to be in canonical form.
 *
 * @param stmt Statement to be analyzed.
 * @param version Server version where this statement is valid.
 *
 * @returns Extracted table references.
 */
std::vector<Table_reference> extract_table_references(
    std::string_view stmt, const mysqlshdk::utils::Version &version);

/**
 * Same as above, but caches lexer/parser, improving execution times of
 * subsequent invocations.
 */
class Extract_table_references final {
 public:
  explicit Extract_table_references(const mysqlshdk::utils::Version &version);

  ~Extract_table_references();

  std::vector<Table_reference> run(std::string_view stmt);

 private:
  class Impl;
  std::unique_ptr<Impl> m_impl;
};

}  // namespace parser
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_PARSER_MYSQL_PARSER_UTILS_H_
