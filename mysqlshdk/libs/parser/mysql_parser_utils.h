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

#ifndef MYSQLSHDK_LIBS_PARSER_MYSQL_PARSER_UTILS_H_
#define MYSQLSHDK_LIBS_PARSER_MYSQL_PARSER_UTILS_H_

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "mysqlshdk/libs/utils/utils_mysql_parsing.h"
#include "mysqlshdk/libs/utils/utils_string.h"

// hack to workaround antlr4 trying to include Token.h but getting token.h from
// Python in macos
#define Py_LIMITED_API

#include "mysqlshdk/libs/parser/mysql/MySQLLexer.h"
#include "mysqlshdk/libs/parser/mysql/MySQLParser.h"
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

namespace internal {
template <typename T>
void do_traverse_statement_ast(
    const std::vector<std::string> &rule_names,
    const antlr4::dfa::Vocabulary &vocabulary, antlr4::tree::ParseTree *tree,
    T *parent_data,
    const std::function<T *(const AST_rule_node &, bool, T *)> &on_rule,
    const std::function<T *(const AST_terminal_node &, T *)> &on_term,
    const std::function<T *(const AST_error_node &, T *)> &on_error) {
  switch (tree->getTreeType()) {
    case antlr4::tree::ParseTreeType::TERMINAL: {
      auto term = dynamic_cast<antlr4::tree::TerminalNode *>(tree);

      antlr4::Token *token = term->getSymbol();

      AST_terminal_node node;

      node.offset = token->getStartIndex();

      node.terminal_symbol = token->getType();
      node.name = vocabulary.getSymbolicName(node.terminal_symbol);
      node.text = term->getText();

      on_term(node, parent_data);
      break;
    }
    case antlr4::tree::ParseTreeType::RULE: {
      auto rule = dynamic_cast<antlr4::RuleContext *>(tree);

      AST_rule_node node;

      node.rule_index = rule->getRuleIndex();
      node.name = rule_names[rule->getRuleIndex()];

      parent_data = on_rule(node, true, parent_data);

      for (const auto child : tree->children) {
        do_traverse_statement_ast(rule_names, vocabulary, child, parent_data,
                                  on_rule, on_term, on_error);
      }

      on_rule(node, false, parent_data);
      break;
    }
    case antlr4::tree::ParseTreeType::ERROR: {
      auto error = dynamic_cast<antlr4::tree::ErrorNode *>(tree);

      antlr4::Token *token = error->getSymbol();

      AST_error_node node;

      node.terminal_symbol = token->getType();
      node.name = vocabulary.getSymbolicName(token->getType());
      node.text = error->getText();
      node.line = token->getLine();
      node.offset = token->getCharPositionInLine();

      on_error(node, parent_data);
      break;
    }
  }
}

class ParserErrorListener : public antlr4::BaseErrorListener {
 public:
  void syntaxError(antlr4::Recognizer * /* recognizer */,
                   antlr4::Token *offendingSymbol, size_t line,
                   size_t charPositionInLine, const std::string &msg,
                   std::exception_ptr /* e */) override {
    throw Sql_syntax_error(msg, std::string(offendingSymbol->getText()), line,
                           charPositionInLine);
  }
};

}  // namespace internal

void prepare_lexer_parser(parsers::MySQLLexer *lexer,
                          parsers::MySQLParser *parser,
                          const mysqlshdk::utils::Version &mysql_version,
                          bool ansi_quotes, bool no_backslash_escapes);

template <typename T>
void traverse_script_ast(
    const std::string &script, const mysqlshdk::utils::Version &mysql_version,
    bool ansi_quotes, bool no_backslash_escapes, T *root_data,
    const std::function<void(const std::string &, size_t, bool enter)> &on_stmt,
    const std::function<T *(const AST_rule_node &, bool enter, T *)> &on_rule,
    const std::function<T *(const AST_terminal_node &, T *)> &on_term,
    const std::function<T *(const AST_error_node &, T *)> &on_error) {
  antlr4::ANTLRInputStream input;
  parsers::MySQLLexer lexer(&input);
  antlr4::CommonTokenStream tokens(&lexer);
  parsers::MySQLParser parser(&tokens);

  internal::ParserErrorListener error_listener;
  lexer.addErrorListener(&error_listener);
  parser.addErrorListener(&error_listener);

  prepare_lexer_parser(&lexer, &parser, mysql_version, ansi_quotes,
                       no_backslash_escapes);

  parser.setBuildParseTree(true);

  auto &rule_names = parser.getRuleNames();
  auto &vocabulary = parser.getVocabulary();

  std::stringstream stream(script);
  mysqlshdk::utils::iterate_sql_stream(
      &stream, 4098,
      [&](const char *stmt, size_t stmt_len, const std::string & /*delim*/,
          size_t /*lnum*/, size_t offs) {
        std::string sql(stmt, stmt_len);

        if (shcore::str_ibeginswith(sql, "delimiter")) return true;

        parser.reset();
        lexer.reset();
        input.load(sql);
        lexer.setInputStream(&input);
        tokens.setTokenSource(&lexer);
        parser.setTokenStream(&tokens);

        on_stmt(sql, offs, true);
        internal::do_traverse_statement_ast(rule_names, vocabulary,
                                            parser.query(), root_data, on_rule,
                                            on_term, on_error);
        on_stmt(sql, offs, false);
        return true;
      },
      [](const std::string &msg) {
        throw std::runtime_error("Error splitting SQL: " + msg);
      },
      ansi_quotes);
}

template <typename T>
void traverse_statement_ast(
    const std::string &stmt, const mysqlshdk::utils::Version &mysql_version,
    bool ansi_quotes, bool no_backslash_escapes, T *root_data,
    const std::function<T *(const AST_rule_node &, bool enter, T *)> &on_rule,
    const std::function<T *(const AST_terminal_node &, T *)> &on_term,
    const std::function<T *(const AST_error_node &, T *)> &on_error) {
  antlr4::ANTLRInputStream input(stmt);
  parsers::MySQLLexer lexer(&input);
  antlr4::CommonTokenStream tokens(&lexer);
  parsers::MySQLParser parser(&tokens);

  internal::ParserErrorListener error_listener;
  lexer.addErrorListener(&error_listener);
  parser.addErrorListener(&error_listener);

  prepare_lexer_parser(&lexer, &parser, mysql_version, ansi_quotes,
                       no_backslash_escapes);

  parser.setBuildParseTree(true);

  auto &rule_names = parser.getRuleNames();
  auto &vocabulary = parser.getVocabulary();

  internal::do_traverse_statement_ast(rule_names, vocabulary, parser.query(),
                                      root_data, on_rule, on_term, on_error);
}

void check_sql_syntax(const std::string &script,
                      const mysqlshdk::utils::Version &mysql_version = {},
                      bool ansi_quotes = false,
                      bool no_backslash_escapes = false);

}  // namespace parser
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_PARSER_MYSQL_PARSER_UTILS_H_
