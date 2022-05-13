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

#include "mysqlshdk/libs/parser/mysql/MySQLLexer.h"
#include "mysqlshdk/libs/parser/mysql/MySQLParser.h"

namespace mysqlshdk {
namespace parser {

struct AST_node {
  bool is_terminal;
  std::string name;
  std::string text;
};

namespace internal {
template <typename T>
void do_traverse_statement_ast(
    const std::vector<std::string> &rule_names,
    const antlr4::dfa::Vocabulary &vocabulary, antlr4::tree::ParseTree *tree,
    T *parent_data, const std::function<T *(const AST_node &, T *)> &fn) {
  switch (tree->getTreeType()) {
    case antlr4::tree::ParseTreeType::TERMINAL: {
      auto term = dynamic_cast<antlr4::tree::TerminalNode *>(tree);

      AST_node node;

      node.is_terminal = true;
      node.name = vocabulary.getSymbolicName(term->getSymbol()->getType());
      node.text = term->getText();

      fn(node, parent_data);
      break;
    }
    case antlr4::tree::ParseTreeType::RULE: {
      auto rule = dynamic_cast<antlr4::RuleContext *>(tree);

      AST_node node;
      node.is_terminal = false;
      node.name = rule_names[rule->getRuleIndex()];

      parent_data = fn(node, parent_data);

      for (const auto child : tree->children) {
        do_traverse_statement_ast(rule_names, vocabulary, child, parent_data,
                                  fn);
      }
      break;
    }
    case antlr4::tree::ParseTreeType::ERROR:
      break;
  }
}
}  // namespace internal

template <typename T>
void traverse_statement_ast(
    const std::string &stmt, T *root_data,
    const std::function<T *(const AST_node &, T *)> &fn) {
  antlr4::ANTLRInputStream input(stmt);
  parsers::MySQLLexer lexer(&input);
  antlr4::CommonTokenStream tokens(&lexer);
  parsers::MySQLParser parser(&tokens);

  parser.setBuildParseTree(true);

  auto &rule_names = parser.getRuleNames();
  auto &vocabulary = parser.getVocabulary();

  internal::do_traverse_statement_ast(rule_names, vocabulary, parser.query(),
                                      root_data, fn);
}

}  // namespace parser
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_PARSER_MYSQL_PARSER_UTILS_H_
