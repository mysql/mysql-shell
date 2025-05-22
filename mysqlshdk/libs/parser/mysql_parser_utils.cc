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

#include "mysqlshdk/libs/parser/mysql_parser_utils.h"

#include <antlr4-runtime.h>

#include <stdexcept>
#include <tuple>
#include <unordered_set>
#include <utility>

#include "mysqlshdk/libs/parser/mysql/MySQLLexer.h"
#include "mysqlshdk/libs/parser/mysql/MySQLParser.h"
#include "mysqlshdk/libs/parser/mysql/MySQLParserBaseListener.h"

#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_mysql_parsing.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace parser {

namespace {

using antlr4::ANTLRInputStream;
using antlr4::CommonTokenStream;

using parsers::MySQLLexer;
using parsers::MySQLParser;
using parsers::MySQLRecognizerCommon;

class Parser_error_listener : public antlr4::BaseErrorListener {
 public:
  void syntaxError(antlr4::Recognizer * /* recognizer */,
                   antlr4::Token *offendingSymbol, size_t line,
                   size_t charPositionInLine, const std::string &msg,
                   std::exception_ptr /* e */) override {
    throw Sql_syntax_error(msg,
                           offendingSymbol
                               ? std::string(offendingSymbol->getText())
                               : std::string{},
                           line, charPositionInLine);
  }
};

class Parser_context {
 public:
  Parser_context(std::string_view stmt, const Parser_config &config)
      : m_input(stmt),
        m_lexer(&m_input),
        m_tokens(&m_lexer),
        m_parser(&m_tokens),
        m_rule_names(m_parser.getRuleNames()),
        m_vocabulary(m_parser.getVocabulary()) {
    configure(config);

    m_parser.setBuildParseTree(true);

    m_lexer.removeErrorListeners();
    m_lexer.addErrorListener(&m_error_listener);

    m_parser.removeErrorListeners();
    m_parser.addErrorListener(&m_error_listener);
  }

  inline MySQLParser::QueryContext *query() { return m_parser.query(); }

  void traverse_ast(IParser_listener *listener) {
    const auto q = query();

    if (listener) traverse_ast(listener, q);
  }

 private:
  void configure(const Parser_config &config) {
    using SqlMode = parsers::MySQLRecognizerCommon::SqlMode;

    SqlMode mode = SqlMode::NoMode;

    // TODO(alfredo) stop forcing ansi_quotes when BUG#37018247 is fixed
    if (config.ansi_quotes || true) {
      mode = SqlMode(mode | SqlMode::AnsiQuotes);
    }

    if (config.no_backslash_escapes) {
      mode = SqlMode(mode | SqlMode::NoBackslashEscapes);
    }

    m_parser.sqlMode = m_lexer.sqlMode = mode;
    m_parser.serverVersion = m_lexer.serverVersion =
        (config.mysql_version ? config.mysql_version : utils::k_shell_version)
            .numeric();
  }

  void traverse_ast(IParser_listener *listener, antlr4::tree::ParseTree *tree) {
    switch (tree->getTreeType()) {
      case antlr4::tree::ParseTreeType::TERMINAL: {
        const auto term = dynamic_cast<antlr4::tree::TerminalNode *>(tree);
        const auto token = term->getSymbol();

        AST_terminal_node node;

        node.offset = token->getStartIndex();

        node.terminal_symbol = token->getType();
        node.name = m_vocabulary.getSymbolicName(node.terminal_symbol);
        node.text = term->getText();

        listener->on_terminal(node);
        break;
      }

      case antlr4::tree::ParseTreeType::RULE: {
        auto rule = dynamic_cast<antlr4::RuleContext *>(tree);

        AST_rule_node node;

        node.rule_index = rule->getRuleIndex();
        node.name = m_rule_names[rule->getRuleIndex()];

        listener->on_rule(node, true);

        for (const auto child : tree->children) {
          traverse_ast(listener, child);
        }

        listener->on_rule(node, false);
        break;
      }

      case antlr4::tree::ParseTreeType::ERROR: {
        const auto error = dynamic_cast<antlr4::tree::ErrorNode *>(tree);
        const auto *token = error->getSymbol();

        AST_error_node node;

        node.terminal_symbol = token->getType();
        node.name = m_vocabulary.getSymbolicName(token->getType());
        node.text = error->getText();
        node.line = token->getLine();
        node.offset = token->getCharPositionInLine();

        listener->on_error(node);
        break;
      }
    }
  }

  ANTLRInputStream m_input;
  MySQLLexer m_lexer;
  CommonTokenStream m_tokens;
  MySQLParser m_parser;
  Parser_error_listener m_error_listener;

  const std::vector<std::string> &m_rule_names;
  const antlr4::dfa::Vocabulary &m_vocabulary;
};

class Tokenizer_context {
 public:
  Tokenizer_context(std::string_view stmt, const Parser_config &config)
      : m_input(stmt),
        m_lexer(&m_input),
        m_rule_names(m_lexer.getRuleNames()),
        m_vocabulary(m_lexer.getVocabulary()) {
    configure(config);

    m_lexer.removeErrorListeners();
    m_lexer.addErrorListener(&m_error_listener);
  }

  bool traverse(
      const std::function<bool(std::string_view, std::string_view)> &visitor) {
    for (;;) {
      auto token = m_lexer.nextToken();
      if (token->getType() == antlr4::Token::EOF) break;

      if (!visitor(m_vocabulary.getSymbolicName(token->getType()),
                   token->getText()))
        return false;
    }
    return true;
  }

 private:
  void configure(const Parser_config &config) {
    using SqlMode = parsers::MySQLRecognizerCommon::SqlMode;

    SqlMode mode = SqlMode::NoMode;

    // TODO(alfredo) stop forcing ansi_quotes when BUG#37018247 is fixed
    if (config.ansi_quotes || true) {
      mode = SqlMode(mode | SqlMode::AnsiQuotes);
    }

    if (config.no_backslash_escapes) {
      mode = SqlMode(mode | SqlMode::NoBackslashEscapes);
    }

    m_lexer.sqlMode = mode;
    m_lexer.serverVersion =
        (config.mysql_version ? config.mysql_version : utils::k_shell_version)
            .numeric();
  }

  ANTLRInputStream m_input;
  MySQLLexer m_lexer;
  const std::vector<std::string> &m_rule_names;
  const antlr4::dfa::Vocabulary &m_vocabulary;
  Parser_error_listener m_error_listener;
};
}  // namespace

void traverse_statement_ast(std::string_view stmt, const Parser_config &config,
                            IParser_listener *listener) {
  Parser_context context{stmt, config};
  context.traverse_ast(listener);
}

void traverse_script_ast(
    const std::string &script, const Parser_config &config,
    const std::function<void(std::string_view, size_t, bool enter)> &on_stmt,
    IParser_listener *listener) {
  std::stringstream stream{script};

  mysqlshdk::utils::iterate_sql_stream(
      &stream, 4098,
      [&](std::string_view stmt, std::string_view /*delim*/, size_t /*lnum*/,
          size_t offs) {
        if (shcore::str_ibeginswith(stmt, "delimiter")) return true;

        if (on_stmt) {
          on_stmt(stmt, offs, true);
        }

        traverse_statement_ast(stmt, config, listener);

        if (on_stmt) {
          on_stmt(stmt, offs, false);
        }

        return true;
      },
      [](std::string_view msg) {
        throw std::runtime_error(
            shcore::str_format("Error splitting SQL: %.*s",
                               static_cast<int>(msg.size()), msg.data()));
      },
      config.ansi_quotes, config.no_backslash_escapes);
}

void check_sql_syntax(const std::string &script, const Parser_config &config) {
  traverse_script_ast(script, config, {}, nullptr);
}

bool tokenize_statement(
    std::string_view stmt, const Parser_config &config,
    const std::function<bool(std::string_view, std::string_view)> &visitor) {
  Tokenizer_context context{stmt, config};

  return context.traverse(visitor);
}

bool Table_reference::operator==(const Table_reference &other) const {
  return schema == other.schema && table == other.table;
}

bool Table_reference::operator<(const Table_reference &other) const {
  return std::tie(schema, table) < std::tie(other.schema, other.table);
}

std::ostream &operator<<(std::ostream &os, const Table_reference &r) {
  os << '`';

  if (!r.schema.empty()) {
    os << r.schema << "`.`";
  }

  return os << r.table << '`';
}

namespace {

class Table_ref_listener : public parsers::MySQLParserBaseListener {
 public:
  explicit Table_ref_listener(std::vector<Table_reference> *references)
      : m_references(references) {}

  void exitTableRef(MySQLParser::TableRefContext *ctx) override {
    Table_reference reference;

    if (const auto qualified_identifier = ctx->qualifiedIdentifier()) {
      reference.table = qualified_identifier->identifier()->getText();

      if (const auto dot_identifier = qualified_identifier->dotIdentifier()) {
        // Full schema.table reference.
        reference.schema = std::move(reference.table);
        reference.table = dot_identifier->identifier()->getText();
      }
    } else {
      // No schema reference.
      reference.table = ctx->dotIdentifier()->identifier()->getText();
    }

    if (is_quoted(reference.schema)) {
      reference.schema = shcore::unquote_identifier(reference.schema);
    }

    if (is_quoted(reference.table)) {
      reference.table = shcore::unquote_identifier(reference.table);
    }

    m_references->emplace_back(std::move(reference));
  }

  void exitCommonTableExpression(
      MySQLParser::CommonTableExpressionContext *ctx) override {
    // capture names of Common Table Expressions (CTEs)
    auto id = ctx->identifier()->getText();

    if (is_quoted(id)) {
      id = shcore::unquote_identifier(id);
    }

    m_cte_identifiers.emplace(std::move(id));
  }

  void exitQuery(MySQLParser::QueryContext *) override {
    // remove CTEs from list of references
    m_references->erase(
        std::remove_if(m_references->begin(), m_references->end(),
                       [this](const auto &ref) {
                         return ref.schema.empty() &&
                                m_cte_identifiers.contains(ref.table);
                       }),
        m_references->end());
  }

 private:
  inline bool is_quoted(const std::string &s) {
    return s.length() >= 2 && '`' == s.front() && s.front() == s.back();
  }

  std::vector<Table_reference> *m_references;
  std::unordered_set<std::string> m_cte_identifiers;
};

}  // namespace

std::vector<Table_reference> extract_table_references(
    std::string_view stmt, const mysqlshdk::utils::Version &version) {
  Parser_context context{stmt, Parser_config{version}};

  const auto tree = context.query();
  std::vector<Table_reference> result;

  Table_ref_listener listener{&result};
  antlr4::tree::ParseTreeWalker::DEFAULT.walk(&listener, tree);

  return result;
}

}  // namespace parser
}  // namespace mysqlshdk
