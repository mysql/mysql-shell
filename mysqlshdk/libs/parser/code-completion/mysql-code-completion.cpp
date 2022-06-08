/*
 * Copyright (c) 2015, 2022, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/parser/code-completion/mysql-code-completion.h"

#include <algorithm>
#include <deque>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include <antlr4-runtime.h>

#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "mysqlshdk/libs/parser/SymbolTable.h"
#include "mysqlshdk/libs/parser/mysql/MySQLLexer.h"
#include "mysqlshdk/libs/parser/mysql/MySQLParser.h"
#include "mysqlshdk/libs/parser/mysql/MySQLParserBaseListener.h"
#include "mysqlshdk/libs/parser/parsers-common.h"

#include "mysqlshdk/libs/parser/code-completion/CodeCompletionCore.h"

using antlr4::ANTLRInputStream;
using antlr4::BufferedTokenStream;
using antlr4::CandidatesCollection;
using antlr4::CodeCompletionCore;
using antlr4::CommonTokenStream;
using antlr4::ParserRuleContext;

using parsers::MySQLLexer;
using parsers::MySQLParser;
using parsers::Scanner;

namespace {

inline bool is_quote_for_identifier(char c, bool ansi_quotes) {
  return '`' == c || ('"' == c && ansi_quotes);
}

inline bool is_quote_for_string(char c) { return '\'' == c || '"' == c; }

// If we know that a token is an identifier, we can safely set ansi_quotes to
// true. If ANSI_QUOTES is not set, then a double quoted token will not be
// recognized as an identifier, and it will not reach this function. If
// ANSI_QUOTES is set, then a double quoted token can be an identifier and we'll
// safely unquote it
std::string unquote_identifier(const std::string &text,
                               bool ansi_quotes = true) {
  if (text.size() < 2 || !is_quote_for_identifier(text[0], ansi_quotes) ||
      text[0] != text.back()) {
    return text;
  }

  try {
    return shcore::unquote_identifier(text, ansi_quotes);
  } catch (const std::runtime_error &) {
    return text;
  }
}

bool is_ansi_quotes_active(const parsers::MySQLRecognizerCommon &recognizer) {
  return recognizer.isSqlModeActive(
      parsers::MySQLRecognizerCommon::SqlMode::AnsiQuotes);
}

std::string unquote_string(const std::string &text) {
  if (text.size() < 2 || !is_quote_for_string(text[0]) ||
      text[0] != text.back()) {
    return text;
  }

  try {
    return shcore::unquote_string(text, text[0]);
  } catch (const std::runtime_error &) {
    return text;
  }
}

}  // namespace

namespace base {

/**
 * Convenience function to determine if 2 strings are the same. This works also
 * for culturally equal letters (e.g. german ß and ss) and any normalization
 * form.
 */
bool same_string(std::string_view first, std::string_view second,
                 bool case_sensitive = true) {
  // TODO(pawel): this should support UTF-8
  if (case_sensitive) {
    return first == second;
  } else {
    return shcore::str_caseeq(first, second);
  }
}

}  // namespace base

//----------------------------------------------------------------------------------------------------------------------

using Table_reference = parsers::Sql_completion_result::Table_reference;

// Context structure for code completion results and token info.
struct AutoCompletionContext {
  CandidatesCollection completionCandidates;

  // A hierarchical view of all table references in the code, updated by
  // visiting all relevant FROM clauses after the candidate collection.
  // Organized as stack to be able to easily remove sets of references when
  // changing nesting level. Implemented as deque however, to allow iterating
  // it.
  std::deque<std::vector<Table_reference>> referencesStack;

  // A flat list of possible references for easier lookup.
  std::vector<Table_reference> references;

  std::vector<std::string> labels;

  //--------------------------------------------------------------------------------------------------------------------

  void collectCandidates(MySQLParser *parser, Scanner &scanner) {
    CodeCompletionCore c3(parser);

    c3.ignoredTokens = {
        MySQLLexer::EOF,
        MySQLLexer::EQUAL_OPERATOR,
        MySQLLexer::ASSIGN_OPERATOR,
        MySQLLexer::NULL_SAFE_EQUAL_OPERATOR,
        MySQLLexer::GREATER_OR_EQUAL_OPERATOR,
        MySQLLexer::GREATER_THAN_OPERATOR,
        MySQLLexer::LESS_OR_EQUAL_OPERATOR,
        MySQLLexer::LESS_THAN_OPERATOR,
        MySQLLexer::NOT_EQUAL_OPERATOR,
        MySQLLexer::NOT_EQUAL2_OPERATOR,
        MySQLLexer::PLUS_OPERATOR,
        MySQLLexer::MINUS_OPERATOR,
        MySQLLexer::MULT_OPERATOR,
        MySQLLexer::DIV_OPERATOR,
        MySQLLexer::MOD_OPERATOR,
        MySQLLexer::LOGICAL_NOT_OPERATOR,
        MySQLLexer::BITWISE_NOT_OPERATOR,
        MySQLLexer::SHIFT_LEFT_OPERATOR,
        MySQLLexer::SHIFT_RIGHT_OPERATOR,
        MySQLLexer::LOGICAL_AND_OPERATOR,
        MySQLLexer::BITWISE_AND_OPERATOR,
        MySQLLexer::BITWISE_XOR_OPERATOR,
        MySQLLexer::LOGICAL_OR_OPERATOR,
        MySQLLexer::BITWISE_OR_OPERATOR,
        MySQLLexer::DOT_SYMBOL,
        MySQLLexer::COMMA_SYMBOL,
        MySQLLexer::SEMICOLON_SYMBOL,
        MySQLLexer::COLON_SYMBOL,
        MySQLLexer::OPEN_PAR_SYMBOL,
        MySQLLexer::CLOSE_PAR_SYMBOL,
        MySQLLexer::OPEN_CURLY_SYMBOL,
        MySQLLexer::CLOSE_CURLY_SYMBOL,
        MySQLLexer::UNDERLINE_SYMBOL,
        MySQLLexer::NULL2_SYMBOL,
        MySQLLexer::PARAM_MARKER,
        MySQLLexer::CONCAT_PIPES_SYMBOL,
        MySQLLexer::BACK_TICK_QUOTED_ID,
        MySQLLexer::SINGLE_QUOTED_TEXT,
        MySQLLexer::DOUBLE_QUOTED_TEXT,
        MySQLLexer::NCHAR_TEXT,
        MySQLLexer::UNDERSCORE_CHARSET,
        MySQLLexer::IDENTIFIER,
        MySQLLexer::INT_NUMBER,
        MySQLLexer::LONG_NUMBER,
        MySQLLexer::ULONGLONG_NUMBER,
        MySQLLexer::DECIMAL_NUMBER,
        MySQLLexer::BIN_NUMBER,
        MySQLLexer::HEX_NUMBER,
    };

    c3.preferredRules = {
        MySQLParser::RuleSchemaRef,

        MySQLParser::RuleTableRef,
        MySQLParser::RuleTableRefWithWildcard,
        MySQLParser::RuleFilterTableRef,

        MySQLParser::RuleColumnRef,
        MySQLParser::RuleColumnInternalRef,
        MySQLParser::RuleTableWild,

        MySQLParser::RuleFunctionRef,
        MySQLParser::RuleFunctionCall,
        MySQLParser::RuleRuntimeFunctionCall,
        MySQLParser::RuleTriggerRef,
        MySQLParser::RuleViewRef,
        MySQLParser::RuleProcedureRef,
        MySQLParser::RuleLogfileGroupRef,
        MySQLParser::RuleTablespaceRef,
        MySQLParser::RuleEngineRef,
        MySQLParser::RuleCollationName,
        MySQLParser::RuleCharsetName,
        MySQLParser::RuleEventRef,
        MySQLParser::RuleServerRef,
        MySQLParser::RuleUser,
        MySQLParser::RulePluginRef,
        MySQLParser::RuleComponentRef,

        MySQLParser::RuleUserVariableIdentifier,
        MySQLParser::RuleLabelRef,
        MySQLParser::RuleLvalueVariable,
        MySQLParser::RuleRvalueSystemVariable,

        MySQLParser::RuleProcedureName,
        MySQLParser::RuleFunctionName,
        MySQLParser::RuleTriggerName,
        MySQLParser::RuleViewName,
        MySQLParser::RuleEventName,
        MySQLParser::RuleTableName,

        // For better handling, but will be ignored.
        MySQLParser::RuleParameterName,
        MySQLParser::RuleProcedureName,
        MySQLParser::RuleIdentifier,
        MySQLParser::RuleLabelIdentifier,
    };

    static std::set<size_t> noSeparatorRequiredFor = {
        MySQLLexer::EQUAL_OPERATOR,
        MySQLLexer::ASSIGN_OPERATOR,
        MySQLLexer::NULL_SAFE_EQUAL_OPERATOR,
        MySQLLexer::GREATER_OR_EQUAL_OPERATOR,
        MySQLLexer::GREATER_THAN_OPERATOR,
        MySQLLexer::LESS_OR_EQUAL_OPERATOR,
        MySQLLexer::LESS_THAN_OPERATOR,
        MySQLLexer::NOT_EQUAL_OPERATOR,
        MySQLLexer::NOT_EQUAL2_OPERATOR,
        MySQLLexer::PLUS_OPERATOR,
        MySQLLexer::MINUS_OPERATOR,
        MySQLLexer::MULT_OPERATOR,
        MySQLLexer::DIV_OPERATOR,
        MySQLLexer::MOD_OPERATOR,
        MySQLLexer::LOGICAL_NOT_OPERATOR,
        MySQLLexer::BITWISE_NOT_OPERATOR,
        MySQLLexer::SHIFT_LEFT_OPERATOR,
        MySQLLexer::SHIFT_RIGHT_OPERATOR,
        MySQLLexer::LOGICAL_AND_OPERATOR,
        MySQLLexer::BITWISE_AND_OPERATOR,
        MySQLLexer::BITWISE_XOR_OPERATOR,
        MySQLLexer::LOGICAL_OR_OPERATOR,
        MySQLLexer::BITWISE_OR_OPERATOR,
        MySQLLexer::DOT_SYMBOL,
        MySQLLexer::COMMA_SYMBOL,
        MySQLLexer::SEMICOLON_SYMBOL,
        MySQLLexer::COLON_SYMBOL,
        MySQLLexer::OPEN_PAR_SYMBOL,
        MySQLLexer::CLOSE_PAR_SYMBOL,
        MySQLLexer::OPEN_CURLY_SYMBOL,
        MySQLLexer::CLOSE_CURLY_SYMBOL,
        MySQLLexer::PARAM_MARKER,
        MySQLLexer::AT_SIGN_SYMBOL,
    };

    // Certain tokens (like identifiers) must be treated as if the char directly
    // following them still belongs to that token (e.g. a whitespace after a
    // name), because visually the caret is placed between that token and the
    // whitespace creating the impression we are still at the identifier (and we
    // should show candidates for this identifier position). Other tokens (like
    // operators) don't need a separator and hence we can take the caret index
    // as is for them.
    // We don't want to move back if we're pointing to a whitespace, because
    // this would mean we'll be completing the previous token.
    size_t caretIndex = scanner.tokenIndex();
    if (MySQLLexer::DEFAULT_TOKEN_CHANNEL == scanner.tokenChannel() &&
        caretIndex > 0 &&
        noSeparatorRequiredFor.count(scanner.lookBack()) == 0) {
      --caretIndex;
    }

    c3.showResult = false;
    c3.showDebugOutput = false;
    referencesStack.emplace_front();  // For the root level of table references.

    parser->reset();
    ParserRuleContext *context = parser->query();

    completionCandidates = c3.collectCandidates(caretIndex, context);

    // Post processing some entries.
    if (completionCandidates.tokens.count(MySQLLexer::NOT2_SYMBOL) > 0) {
      // NOT2 is a NOT with special meaning in the operator precedence chain.
      // For code completion it's the same as NOT.
      completionCandidates.tokens[MySQLLexer::NOT_SYMBOL] =
          completionCandidates.tokens[MySQLLexer::NOT2_SYMBOL];
      completionCandidates.tokens.erase(MySQLLexer::NOT2_SYMBOL);
    }

    // If a column reference is required then we have to continue scanning the
    // query for table references.
    for (auto ruleEntry : completionCandidates.rules) {
      if (ruleEntry.first == MySQLParser::RuleColumnRef ||
          ruleEntry.first == MySQLParser::RuleTableWild) {
        collectLeadingTableReferences(parser, scanner, caretIndex, false);
        takeReferencesSnapshot();
        collectRemainingTableReferences(parser, scanner);
        takeReferencesSnapshot();
        break;
      } else if (ruleEntry.first == MySQLParser::RuleColumnInternalRef) {
        // Note:: rule columnInternalRef is not only used for ALTER TABLE, but
        // atm. we only support that here.
        collectLeadingTableReferences(parser, scanner, caretIndex, true);
        takeReferencesSnapshot();
        break;
      } else if (ruleEntry.first == MySQLParser::RuleLabelRef) {
        collect_leading_labels(&scanner, caretIndex);
        break;
      }
    }

    return;
  }

  //--------------------------------------------------------------------------------------------------------------------

 private:
  // A listener to handle references as we traverse a parse tree.
  // We have two modes here:
  //   fromClauseMode = true: we are not interested in subqueries and don't need
  //   to stop at the caret. otherwise: go down all subqueries and stop when the
  //   caret position is reached.
  class TableRefListener : public parsers::MySQLParserBaseListener {
   public:
    TableRefListener(AutoCompletionContext &context, bool fromClauseMode)
        : _context(context), _fromClauseMode(fromClauseMode) {}

    void exitTableRef(MySQLParser::TableRefContext *ctx) override {
      if (_done) return;

      if (!_fromClauseMode || _level == 0) {
        Table_reference reference;
        if (ctx->qualifiedIdentifier() != nullptr) {
          reference.table = unquote_identifier(
              ctx->qualifiedIdentifier()->identifier()->getText());
          if (ctx->qualifiedIdentifier()->dotIdentifier() != nullptr) {
            // Full schema.table reference.
            reference.schema = reference.table;
            reference.table = unquote_identifier(ctx->qualifiedIdentifier()
                                                     ->dotIdentifier()
                                                     ->identifier()
                                                     ->getText());
          }
        } else {
          // No schema reference.
          reference.table =
              unquote_identifier(ctx->dotIdentifier()->identifier()->getText());
        }
        _context.referencesStack.front().emplace_back(std::move(reference));
      }
    }

    void exitTableAlias(MySQLParser::TableAliasContext *ctx) override {
      if (_done) return;

      if (_level == 0 && !_context.referencesStack.empty() &&
          !_context.referencesStack.front().empty()) {
        // Appears after a single or derived table.
        // Since derived tables can be very complex it is not possible here to
        // determine possible columns for completion, hence we just walk over
        // them and thus have no field where to store the found alias.
        _context.referencesStack.front().back().alias =
            unquote_identifier(ctx->identifier()->getText());
      }
    }

    void enterSubquery(MySQLParser::SubqueryContext *) override {
      if (_done) return;

      if (_fromClauseMode)
        ++_level;
      else
        _context.referencesStack.emplace_front();
    }

    void exitSubquery(MySQLParser::SubqueryContext *) override {
      if (_done) return;

      if (_fromClauseMode)
        --_level;
      else
        _context.referencesStack.pop_front();
    }

   private:
    bool _done = false;  // Only used in full mode.
    size_t _level = 0;   // Only used in FROM clause traversal.
    AutoCompletionContext &_context;
    bool _fromClauseMode;
  };

  //--------------------------------------------------------------------------------------------------------------------

  /**
   * Called if one of the candidates is a column reference, for table references
   * *before* the caret. SQL code must be valid up to the caret, so we can check
   * nesting strictly.
   */
  void collectLeadingTableReferences(MySQLParser *parser, Scanner &scanner,
                                     size_t caretIndex, bool forTableAlter) {
    scanner.push();

    if (forTableAlter) {
      // For ALTER TABLE commands we do a simple backscan (no nesting is
      // allowed) until we find ALTER TABLE.
      while (scanner.previous() &&
             (scanner.tokenType() != MySQLLexer::ALTER_SYMBOL ||
              scanner.lookAhead() != MySQLLexer::TABLE_SYMBOL)) {
      }

      if (scanner.skipTokenSequence(
              {MySQLLexer::ALTER_SYMBOL, MySQLLexer::TABLE_SYMBOL})) {
        Table_reference reference;
        reference.table = unquote_identifier(scanner.tokenText());
        if (scanner.next() && scanner.is(MySQLLexer::DOT_SYMBOL)) {
          reference.schema = std::move(reference.table);
          scanner.next();
          reference.table = unquote_identifier(scanner.tokenText());
        }
        referencesStack.front().emplace_back(std::move(reference));
      }
    } else {
      scanner.seek(0);

      size_t level = 0;
      while (true) {
        bool found = scanner.tokenType() == MySQLLexer::FROM_SYMBOL;
        while (!found) {
          if (!scanner.next() || scanner.tokenIndex() >= caretIndex) break;

          switch (scanner.tokenType()) {
            case MySQLLexer::OPEN_PAR_SYMBOL:
              ++level;
              referencesStack.emplace_front();

              break;

            case MySQLLexer::CLOSE_PAR_SYMBOL:
              if (level == 0) {
                scanner.pop();
                return;  // We cannot go above the initial nesting level.
              }

              --level;
              referencesStack.pop_front();

              break;

            case MySQLLexer::FROM_SYMBOL:
              found = true;
              break;

            default:
              break;
          }
        }

        if (!found) {
          scanner.pop();
          return;  // No more FROM clause found.
        }

        parseTableReferences(scanner.tokenSubText(), parser);
        if (scanner.tokenType() == MySQLLexer::FROM_SYMBOL) scanner.next();
      }
    }

    scanner.pop();
  }

  //--------------------------------------------------------------------------------------------------------------------

  /**
   * Called if one of the candidates is a column reference, for table references
   * *after* the caret. The function attempts to get table references together
   * with aliases where possible. This is the only place where we actually look
   * beyond the caret and hence different rules apply: the query doesn't need to
   * be valid beyond that point. We simply scan forward until we find a FROM
   * keyword and work from there. This makes it much easier to work on
   * incomplete queries, which nonetheless need e.g. columns from table
   * references. Because inner queries can use table references from outer
   * queries we can simply scan for all outer FROM clauses (skip over
   * subqueries).
   */
  void collectRemainingTableReferences(MySQLParser *parser, Scanner &scanner) {
    scanner.push();

    // Continously scan forward to all FROM clauses on the current or any higher
    // nesting level. With certain syntax errors this can lead to a wrong FROM
    // clause (e.g. if parentheses don't match). But that is acceptable.
    size_t level = 0;
    while (true) {
      bool found = scanner.tokenType() == MySQLLexer::FROM_SYMBOL;
      while (!found) {
        if (!scanner.next()) break;

        switch (scanner.tokenType()) {
          case MySQLLexer::OPEN_PAR_SYMBOL:
            ++level;
            break;

          case MySQLLexer::CLOSE_PAR_SYMBOL:
            if (level > 0) --level;
            break;

          case MySQLLexer::FROM_SYMBOL:
            // Open and close parentheses don't need to match, if we come from
            // within a subquery.
            if (level == 0) found = true;
            break;

          default:
            break;
        }
      }

      if (!found) {
        scanner.pop();
        return;  // No more FROM clause found.
      }

      parseTableReferences(scanner.tokenSubText(), parser);
      if (scanner.tokenType() == MySQLLexer::FROM_SYMBOL) scanner.next();
    }
  }

  //--------------------------------------------------------------------------------------------------------------------

  /**
   * Parses the given FROM clause text using a local parser and collects all
   * found table references.
   */
  void parseTableReferences(std::string const &fromClause,
                            MySQLParser *parserTemplate) {
    // We use a local parser just for the FROM clause to avoid messing up tokens
    // on the autocompletion parser (which would affect the processing of the
    // found candidates).
    ANTLRInputStream input(fromClause);
    MySQLLexer lexer(&input);
    CommonTokenStream tokens(&lexer);
    MySQLParser fromParser(&tokens);

    lexer.serverVersion = parserTemplate->serverVersion;
    lexer.sqlMode = parserTemplate->sqlMode;
    fromParser.serverVersion = parserTemplate->serverVersion;
    fromParser.sqlMode = parserTemplate->sqlMode;
    fromParser.setBuildParseTree(true);

    lexer.removeErrorListeners();
    fromParser.removeErrorListeners();
    antlr4::tree::ParseTree *tree = fromParser.fromClause();

    TableRefListener listener(*this, true);
    antlr4::tree::ParseTreeWalker::DEFAULT.walk(&listener, tree);
  }

  //--------------------------------------------------------------------------------------------------------------------

  /**
   * Copies the current references stack into the references map.
   */
  void takeReferencesSnapshot() {
    // Don't clear the references map here. Can happen we have to take multiple
    // snapshots. We automatically remove duplicates by using a map.
    for (auto &entry : referencesStack) {
      for (auto &reference : entry) references.push_back(reference);
    }
  }

  //--------------------------------------------------------------------------------------------------------------------

  void collect_leading_labels(Scanner *scanner, size_t caret_index) {
    scanner->push();
    scanner->seek(0);

    // we're using scanner to find labels, as using listener+parser is not going
    // to be effective: at enterLabel() the label is not know, exitLabel() will
    // not be called if we're in the middle of labelled statement

    // labelled block:
    //   [begin_label:] BEGIN .. END [end_label]
    //   [begin_label:] LOOP .. END LOOP [end_label]
    //   [begin_label:] REPEAT .. END REPEAT [end_label]
    //   [begin_label:] WHILE .. END WHILE [end_label]

    // statements and blocks with END symbol:
    //   XA END ..
    //   CASE .. END
    //   CASE .. END CASE
    //   IF .. END IF
    //   all labelled block

    // Label is only valid within the block, so we push it when we enter a block
    // and pop it once we leave a block. Since END which is not followed by a
    // symbol can be a part of BEGIN .. END block, but it also can be a part of
    // CASE .. END block, we handle all blocks which can contain an END symbol.
    // In these cases, the pushed label is empty, all empty labels are removed
    // afterwards. We ignore XA BEGIN and XA END, which do not appear in the
    // same statement.

    while (scanner->tokenIndex() < caret_index) {
      const auto current = scanner->tokenType();
      const auto previous = scanner->lookBack();

      if ((MySQLLexer::BEGIN_SYMBOL == current &&
           MySQLLexer::XA_SYMBOL != previous) ||
          MySQLLexer::LOOP_SYMBOL == current ||
          MySQLLexer::REPEAT_SYMBOL == current ||
          MySQLLexer::WHILE_SYMBOL == current) {
        std::string label;

        // fetch label if this is a labelled block and has a begin_label
        if (MySQLLexer::COLON_SYMBOL == previous) {
          scanner->push();

          // move to colon
          scanner->previous();
          // move to label identifier
          scanner->previous();
          // store label
          label = unquote_identifier(scanner->tokenText());

          scanner->pop();
        }

        labels.emplace_back(std::move(label));
      } else if (MySQLLexer::CASE_SYMBOL == current ||
                 MySQLLexer::IF_SYMBOL == current) {
        // empty label
        labels.emplace_back();
      } else if (MySQLLexer::END_SYMBOL == current &&
                 MySQLLexer::XA_SYMBOL != previous) {
        const auto next = scanner->lookAhead();

        // if END is followed by any of the symbols we care about, it needs to
        // be consumed
        if (MySQLLexer::LOOP_SYMBOL == next ||
            MySQLLexer::REPEAT_SYMBOL == next ||
            MySQLLexer::WHILE_SYMBOL == next ||
            MySQLLexer::CASE_SYMBOL == next || MySQLLexer::IF_SYMBOL == next) {
          scanner->next();
        }

        // if the END token is just before the caret, we don't want to remove
        // the label, as it is a candidate for the end_label
        if (scanner->tokenIndex() + 1 != caret_index) {
          labels.pop_back();
        }
      }

      if (!scanner->next()) break;
    }

    scanner->pop();

    // remove empty labels
    labels.erase(std::remove(labels.begin(), labels.end(), std::string{}),
                 labels.end());
  }

  //--------------------------------------------------------------------------------------------------------------------
};

//----------------------------------------------------------------------------------------------------------------------

enum ObjectFlags {
  // For 3 part identifiers.
  ShowSchemas = 1 << 0,
  ShowTables = 1 << 1,
  ShowColumns = 1 << 2,

  // For 2 part identifiers.
  ShowFirst = 1 << 3,
  ShowSecond = 1 << 4,
};

//----------------------------------------------------------------------------------------------------------------------

/**
 * Determines the qualifier used for a qualified identifier with up to 2 parts
 * (id or id.id). Returns the found qualifier (if any) and a flag indicating
 * what should be shown.
 *
 * Note: it is essential to understand that we do the determination only up to
 * the caret (or the token following it, solely for getting a terminator). Since
 * we cannot know the user's intention, we never look forward.
 */
static ObjectFlags determineQualifier(Scanner &scanner, MySQLLexer *lexer,
                                      parsers::Sql_completion_result *r,
                                      std::string_view *qualifier) {
  // Five possible positions here:
  //   - In the first id (including the position directly after the last char).
  //   - In the space between first id and a dot.
  //   - On a dot (visually directly before the dot).
  //   - In space after the dot, that includes the position directly after the
  //   dot.
  //   - In the second id.
  // All parts are optional (though not at the same time). The on-dot position
  // is considered the same as in first id as it visually belongs to the first
  // id.

  size_t position = scanner.tokenIndex();

  if (scanner.tokenChannel() != 0)
    scanner.next(true);  // First skip to the next non-hidden token.

  if (!scanner.is(MySQLLexer::DOT_SYMBOL) &&
      !lexer->isIdentifier(scanner.tokenType())) {
    // We are at the end of an incomplete identifier spec. Jump back, so that
    // the other tests succeed.
    scanner.previous(true);
  }

  // Go left until we find something not related to an id or find at most 1 dot.
  if (position > 0) {
    if (lexer->isIdentifier(scanner.tokenType()) &&
        scanner.lookBack() == MySQLLexer::DOT_SYMBOL)
      scanner.previous(true);
    if (scanner.is(MySQLLexer::DOT_SYMBOL) &&
        lexer->isIdentifier(scanner.lookBack()))
      scanner.previous(true);
  }

  // The scanner is now on the leading identifier or dot (if there's no leading
  // id).
  *qualifier = "";
  std::string temp;
  if (lexer->isIdentifier(scanner.tokenType())) {
    temp = unquote_identifier(scanner.tokenText());
    scanner.next(true);
  }

  // Bail out if there is no more id parts or we are already behind the caret
  // position.
  if (!scanner.is(MySQLLexer::DOT_SYMBOL) || position <= scanner.tokenIndex())
    return ObjectFlags(ShowFirst | ShowSecond);

  // this function can be called multiple times for different rules, but result
  // is always the same, no need to repeatedly set the qualifier
  if (r->context.qualifier.empty()) {
    log_debug3("Qualifier is: '%.*s'", static_cast<int>(temp.length()),
               temp.data());
    r->context.qualifier.emplace_back(std::move(temp));
  }

  *qualifier = r->context.qualifier.front();

  return ShowSecond;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Enhanced variant of the previous function that determines schema and table
 * qualifiers for column references (and table_wild in multi table delete, for
 * that matter). Returns a set of flags that indicate what to show for that
 * identifier, as well as schema and table if given. The returned schema can be
 * either for a schema.table situation (which requires to show tables) or a
 * schema.table.column situation. Which one is determined by whether showing
 * columns alone or not.
 */
static ObjectFlags determineSchemaTableQualifier(
    Scanner &scanner, MySQLLexer *lexer, parsers::Sql_completion_result *r,
    std::string_view *schema, std::string_view *table) {
  size_t position = scanner.tokenIndex();
  if (scanner.tokenChannel() != 0) scanner.next(true);

  size_t tokenType = scanner.tokenType();
  if (tokenType != MySQLLexer::DOT_SYMBOL &&
      !lexer->isIdentifier(scanner.tokenType())) {
    // Just like in the simpler function. If we have found no identifier or dot
    // then we are at the end of an incomplete definition. Simply seek back to
    // the previous non-hidden token.
    scanner.previous(true);
  }

  // Go left until we find something not related to an id or at most 2 dots.
  if (position > 0) {
    if (lexer->isIdentifier(scanner.tokenType()) &&
        (scanner.lookBack() == MySQLLexer::DOT_SYMBOL))
      scanner.previous(true);
    if (scanner.is(MySQLLexer::DOT_SYMBOL) &&
        lexer->isIdentifier(scanner.lookBack())) {
      scanner.previous(true);

      // And once more.
      if (scanner.lookBack() == MySQLLexer::DOT_SYMBOL) {
        scanner.previous(true);
        if (lexer->isIdentifier(scanner.lookBack())) scanner.previous(true);
      }
    }
  }

  // The scanner is now on the leading identifier or dot (if there's no leading
  // id).
  *schema = "";
  *table = "";

  std::string temp;
  if (lexer->isIdentifier(scanner.tokenType())) {
    temp = unquote_identifier(scanner.tokenText());
    scanner.next(true);
  }

  // Bail out if there is no more id parts or we are already behind the caret
  // position.
  if (!scanner.is(MySQLLexer::DOT_SYMBOL) || position <= scanner.tokenIndex())
    return ObjectFlags(ShowSchemas | ShowTables | ShowColumns);

  scanner.next(true);  // Skip dot.

  // this function can be called multiple times for different rules, but result
  // is always the same, no need to repeatedly set the qualifier
  const auto add_qualifier = r->context.qualifier.empty();

  if (add_qualifier) {
    log_debug3("First qualifier is: '%.*s'", static_cast<int>(temp.length()),
               temp.data());
    r->context.qualifier.emplace_back(std::move(temp));
  }

  *schema = r->context.qualifier.front();
  *table = *schema;

  if (lexer->isIdentifier(scanner.tokenType())) {
    temp = unquote_identifier(scanner.tokenText());
    scanner.next(true);

    if (!scanner.is(MySQLLexer::DOT_SYMBOL) || position <= scanner.tokenIndex())
      return ObjectFlags(ShowTables |
                         ShowColumns);  // Schema only valid for tables. Columns
                                        // must use default schema.

    if (add_qualifier) {
      log_debug3("Second qualifier is: '%.*s'", static_cast<int>(temp.length()),
                 temp.data());
      r->context.qualifier.emplace_back(std::move(temp));
    }

    *schema = r->context.qualifier.front();
    *table = r->context.qualifier.back();

    return ShowColumns;
  }

  return ObjectFlags(ShowTables |
                     ShowColumns);  // Schema only valid for tables. Columns
                                    // must use default schema.
}

//----------------------------------------------------------------------------------------------------------------------

namespace {

using Columns = parsers::Sql_completion_result::Columns;
using Names = parsers::Sql_completion_result::Names;
using Candidate = parsers::Sql_completion_result::Candidate;
using Object_names = std::unordered_set<std::string_view>;

void insert(Names *target, std::string_view value, const char *context) {
  if (!value.empty()) {
    log_debug3("Adding %s '%.*s'", context, static_cast<int>(value.length()),
               value.data());
    target->emplace(value);
  }
}

void insert(Names *target, std::string value, const char *context) {
  if (!value.empty()) {
    log_debug3("Adding %s '%.*s'", context, static_cast<int>(value.length()),
               value.data());
    target->emplace(std::move(value));
  }
}

void insert(Names *target, const Object_names &values, const char *context) {
  for (const auto &value : values) {
    insert(target, value, context);
  }
}

void insert_system_function(parsers::Sql_completion_result *r,
                            std::string_view function) {
  insert(&r->system_functions, std::string{function} + "()",
         "a system function");
}

void insert_schemas(parsers::Sql_completion_result *r) {
  log_debug3("Adding schema names");
  r->candidates.emplace(Candidate::SCHEMA);
}

void insert_functions(parsers::Sql_completion_result *r,
                      std::string_view schema) {
  r->candidates.emplace(Candidate::FUNCTION);
  insert(&r->functions_from, schema, "functions from");
}

void insert_procedures(parsers::Sql_completion_result *r,
                       std::string_view schema) {
  r->candidates.emplace(Candidate::PROCEDURE);
  insert(&r->procedures_from, schema, "procedures from");
}

void insert_table(parsers::Sql_completion_result *r, std::string_view table) {
  r->candidates.emplace(Candidate::TABLE);
  insert(&r->tables, table, "a table");
}

void insert_tables(parsers::Sql_completion_result *r, std::string_view schema) {
  r->candidates.emplace(Candidate::TABLE);
  insert(&r->tables_from, schema, "tables from");
}

void insert_tables(parsers::Sql_completion_result *r,
                   const Object_names &schemas) {
  r->candidates.emplace(Candidate::TABLE);
  insert(&r->tables_from, schemas, "tables from");
}

void insert_views(parsers::Sql_completion_result *r, std::string_view schema) {
  r->candidates.emplace(Candidate::VIEW);
  insert(&r->views_from, schema, "views from");
}

void insert_views(parsers::Sql_completion_result *r,
                  const Object_names &schemas) {
  r->candidates.emplace(Candidate::VIEW);
  insert(&r->views_from, schemas, "views from");
}

void insert_columns(Columns *target, std::string_view schema,
                    std::string_view table, bool internal) {
  std::string s{schema};
  auto it = target->find(s);

  if (target->end() == it) {
    it = target->emplace(std::move(s), Names{}).first;
  }

  std::string context;

  if (internal) {
    context += "internal ";
  }

  context += "columns from '" + std::string{schema} + "'.";

  insert(&it->second, table, context.c_str());
}

void insert_columns(parsers::Sql_completion_result *r, std::string_view schema,
                    std::string_view table) {
  r->candidates.emplace(Candidate::COLUMN);
  insert_columns(&r->columns, schema, table, false);
}

void insert_internal_columns(parsers::Sql_completion_result *r,
                             std::string_view schema, std::string_view table) {
  r->candidates.emplace(Candidate::INTERNAL_COLUMN);
  insert_columns(&r->internal_columns, schema, table, true);
}

void insert_triggers(parsers::Sql_completion_result *r,
                     std::string_view schema) {
  r->candidates.emplace(Candidate::TRIGGER);
  insert(&r->triggers_from, schema, "triggers from");
}

void insert_events(parsers::Sql_completion_result *r, std::string_view schema) {
  r->candidates.emplace(Candidate::EVENT);
  insert(&r->events_from, schema, "events from");
}

void insert_system_variables(parsers::Sql_completion_result *r) {
  log_debug3("Adding system variables");
  r->candidates.emplace(Candidate::SYSTEM_VAR);
}

std::string get_prefix(MySQLParser *parser, Scanner *scanner, size_t offset) {
  if (0 == offset) {
    // there's no prefix if we're at the beginning of a statement
    return {};
  }

  // find prefix we're completing
  const auto current_token = scanner->tokenType();
  const auto current_token_channel = scanner->tokenChannel();
  const auto current_token_start = scanner->tokenOffset();

  const auto substr = [parser](std::size_t begin, std::size_t end) {
    return parser->getTokenStream()
        ->getTokenSource()
        ->getInputStream()
        ->getText(antlr4::misc::Interval(begin, end));
  };

  // X for the parser means get char at X offset, we want to get char up to X
  // offset, hence we subtract one
  --offset;

  if (!scanner->previous(false)) {
    // if we cannot move back, then it's an empty string or we're at the first
    // token
    return substr(0, offset);
  }

  const auto past_previous_token_end = scanner->tokenEndOffset() + 1;
  std::string result;

  const auto should_use_token = [](size_t channel, size_t type) {
    // we're using a token unless it's a hidden token or an operator
    return channel == MySQLLexer::DEFAULT_TOKEN_CHANNEL &&
           !MySQLLexer::isOperator(type);
  };

  if (MySQLLexer::EOF == current_token) {
    // if we're at the end, then the previous token is being completed
    if (past_previous_token_end == current_token_start) {
      // if the previous token is positioned directly next to the end, we're
      // completing it
      if (should_use_token(scanner->tokenChannel(), scanner->tokenType())) {
        result = scanner->tokenText();
      }
    } else {
      // if the previous token is not next to the end, then we're completing
      // text which is between these two tokens
      result = substr(past_previous_token_end, offset);
    }
  } else {
    if (current_token_start == offset + 1) {
      // if the previous token is positioned directly next to the offset, we're
      // completing it
      if (should_use_token(scanner->tokenChannel(), scanner->tokenType())) {
        result = scanner->tokenText();
      }
    } else if (current_token_start <= offset) {
      // we're at the token that's being completed
      if (should_use_token(current_token_channel, current_token)) {
        result = substr(current_token_start, offset);
      }
    } else {
      // we've moved past the text that's being completed, we're completing text
      // which is between previous and current tokens
      result = substr(past_previous_token_end, offset);
    }
  }

  // restore scanner position
  scanner->pop();
  scanner->push();

  return result;
}

std::string unquote_prefix_as_identifier(const std::string &prefix,
                                         bool ansi_quotes) {
  if (prefix.empty() || !is_quote_for_identifier(prefix[0], ansi_quotes)) {
    return prefix;
  }

  if (1 == prefix.length()) {
    // just a quote, return an empty string
    return {};
  }

  if (prefix[0] == prefix.back()) {
    // if quote is closed, try to unquote the identifier
    try {
      return shcore::unquote_identifier(prefix, ansi_quotes);
    } catch (const std::runtime_error &) {
      // we can fail if the last character was a quote, but it was escaped,
      // append the quote and try again
    }
  }

  try {
    // if quote is not closed, close it, then unquote the identifier
    return shcore::unquote_identifier(prefix + prefix[0], ansi_quotes);
  } catch (const std::runtime_error &) {
    return prefix;
  }
}

std::string unquote_prefix_as_string(const std::string &prefix) {
  if (prefix.empty() || !is_quote_for_string(prefix[0])) {
    return prefix;
  }

  if (1 == prefix.length()) {
    // just a quote, return an empty string
    return {};
  }

  if (prefix[0] == prefix.back()) {
    // if quote is closed, try to unquote the string
    try {
      return shcore::unquote_string(prefix, prefix[0]);
    } catch (const std::runtime_error &) {
      // we can fail if the last character was a quote, but it was escaped,
      // append the quote and try again
    }
  }

  try {
    // if quote is not closed, close it, then unquote the string
    return shcore::unquote_string(prefix + prefix[0], prefix[0]);
  } catch (const std::runtime_error &) {
    return prefix;
  }
}

void init_prefix_info(parsers::Sql_completion_result::Prefix *prefix,
                      bool ansi_quotes) {
  prefix->as_identifier =
      unquote_prefix_as_identifier(prefix->full, ansi_quotes);
  prefix->quoted_as_identifier = prefix->full != prefix->as_identifier;

  prefix->as_string = unquote_prefix_as_string(prefix->full);
  prefix->quoted_as_string = prefix->full != prefix->as_string;

  prefix->quoted = prefix->quoted_as_identifier || prefix->quoted_as_string;

  if (prefix->quoted) {
    prefix->quote = prefix->full[0];
    prefix->as_string_or_identifier = prefix->quoted_as_identifier
                                          ? prefix->as_identifier
                                          : prefix->as_string;
  } else {
    prefix->as_string_or_identifier = prefix->full;
  }

  log_debug3("Prefix: |%s| - |%s| - |%s|", prefix->full.c_str(),
             prefix->as_identifier.c_str(), prefix->as_string.c_str());
}

void init_prefix_as_user(Scanner *scanner,
                         parsers::Sql_completion_result::Context *context) {
  using Prefix = parsers::Sql_completion_result::Prefix;

  const auto copy = [](const Prefix &prefix,
                       parsers::Sql_completion_result::Account_part *part) {
    part->full = prefix.full;
    part->quoted = prefix.quoted;
    part->quote = prefix.quote;
    part->unquoted = prefix.as_string_or_identifier;
  };

  // we're positioned either past or at the token which is being completed, move
  // back one token
  if (!scanner->previous()) {
    return;
  }

  // if we're now at the prefix, we were positioned past the token which is
  // being completed, move back one more time
  if (context->prefix.full == scanner->tokenText()) {
    if (!scanner->previous()) {
      return;
    }
  }

  // if we're now at '@' then context.prefix holds the host, otherwise it's a
  // user name
  if (MySQLLexer::AT_SIGN_SYMBOL == scanner->tokenType()) {
    if (!scanner->previous()) {
      return;
    }

    Prefix user;
    user.full = scanner->tokenText();
    init_prefix_info(&user, false);

    copy(user, &context->as_account.user);
    copy(context->prefix, &context->as_account.host);
    context->as_account.has_at_sign = true;

    // adjust the prefix
    context->prefix.full = user.full + "@" + context->prefix.full;
  } else {
    copy(context->prefix, &context->as_account.user);
  }

  // reset prefix information, so it unlikely that it's going to match non-user
  // candidates
  context->prefix.quoted = false;
  context->prefix.quote = 0;
  context->prefix.as_identifier = context->prefix.full;
  context->prefix.quoted_as_identifier = false;
  context->prefix.as_string = context->prefix.full;
  context->prefix.quoted_as_string = false;
  context->prefix.as_string_or_identifier = context->prefix.full;

  // restore scanner position
  scanner->pop();
  scanner->push();
}

}  // namespace

//----------------------------------------------------------------------------------------------------------------------

parsers::Sql_completion_result getCodeCompletion(size_t offset,
                                                 std::string_view active_schema,
                                                 bool uppercaseKeywords,
                                                 bool filtered,
                                                 MySQLParser *parser) {
  log_debug("Invoking code completion");

  static const std::map<size_t, std::vector<std::string>> synonyms = {
      {MySQLLexer::CHAR_SYMBOL, {"CHARACTER"}},
      {MySQLLexer::NOW_SYMBOL,
       {"CURRENT_TIMESTAMP", "LOCALTIME", "LOCALTIMESTAMP"}},
      {MySQLLexer::DAY_SYMBOL, {"DAYOFMONTH"}},
      {MySQLLexer::DECIMAL_SYMBOL, {"DEC"}},
      {MySQLLexer::DISTINCT_SYMBOL, {"DISTINCTROW"}},
      {MySQLLexer::CHAR_SYMBOL, {"CHARACTER"}},
      {MySQLLexer::COLUMNS_SYMBOL, {"FIELDS"}},
      {MySQLLexer::FLOAT_SYMBOL, {"FLOAT4"}},
      {MySQLLexer::DOUBLE_SYMBOL, {"FLOAT8"}},
      {MySQLLexer::INT_SYMBOL, {"INTEGER", "INT4"}},
      {MySQLLexer::RELAY_THREAD_SYMBOL, {"IO_THREAD"}},
      {MySQLLexer::SUBSTRING_SYMBOL, {"MID"}},
      {MySQLLexer::MID_SYMBOL, {"MEDIUMINT"}},
      {MySQLLexer::MEDIUMINT_SYMBOL, {"MIDDLEINT"}},
      {MySQLLexer::NDBCLUSTER_SYMBOL, {"NDB"}},
      {MySQLLexer::REGEXP_SYMBOL, {"RLIKE"}},
      {MySQLLexer::DATABASE_SYMBOL, {"SCHEMA"}},
      {MySQLLexer::DATABASES_SYMBOL, {"SCHEMAS"}},
      {MySQLLexer::USER_SYMBOL, {"SESSION_USER"}},
      {MySQLLexer::STD_SYMBOL, {"STDDEV", "STDDEV_POP"}},
      {MySQLLexer::SUBSTRING_SYMBOL, {"SUBSTR"}},
      {MySQLLexer::VARCHAR_SYMBOL, {"VARCHARACTER"}},
      {MySQLLexer::VARIANCE_SYMBOL, {"VAR_POP"}},
      {MySQLLexer::TINYINT_SYMBOL, {"INT1"}},
      {MySQLLexer::SMALLINT_SYMBOL, {"INT2"}},
      {MySQLLexer::MEDIUMINT_SYMBOL, {"INT3"}},
      {MySQLLexer::BIGINT_SYMBOL, {"INT8"}},
      {MySQLLexer::SECOND_SYMBOL, {"SQL_TSI_SECOND"}},
      {MySQLLexer::MINUTE_SYMBOL, {"SQL_TSI_MINUTE"}},
      {MySQLLexer::HOUR_SYMBOL, {"SQL_TSI_HOUR"}},
      {MySQLLexer::DAY_SYMBOL, {"SQL_TSI_DAY"}},
      {MySQLLexer::WEEK_SYMBOL, {"SQL_TSI_WEEK"}},
      {MySQLLexer::MONTH_SYMBOL, {"SQL_TSI_MONTH"}},
      {MySQLLexer::QUARTER_SYMBOL, {"SQL_TSI_QUARTER"}},
      {MySQLLexer::YEAR_SYMBOL, {"SQL_TSI_YEAR"}},
  };

  // Also create a separate scanner which allows us to easily navigate the
  // tokens without affecting the token stream used by the parser.
  Scanner scanner(
      dynamic_cast<BufferedTokenStream *>(parser->getTokenStream()));

  // Move to the token that ends past the given offset. Note that we don't check
  // start of the token here, because if a word which does not form a token yet
  // (i.e. an identifier which starts with a back tick) is being completed, it's
  // not going to be on the tokens list and we want to move past it.
  do {
    log_debug3(
        "|%s| - %s (%zu), %zu-%zu", scanner.tokenText().c_str(),
        parser->getVocabulary().getDisplayName(scanner.tokenType()).c_str(),
        scanner.tokenType(), scanner.tokenOffset(), scanner.tokenEndOffset());

    if (offset <= scanner.tokenEndOffset()) {
      break;
    }
  } while (scanner.next(false));

  // Store position on the scanner stack.
  scanner.push();

  AutoCompletionContext context;
  context.collectCandidates(parser, scanner);

  MySQLQueryType queryType = QtUnknown;
  MySQLLexer *lexer =
      dynamic_cast<MySQLLexer *>(parser->getTokenStream()->getTokenSource());
  if (lexer != nullptr) {
    lexer->reset();  // Set back the input position to the beginning for query
                     // type determination.
    queryType = lexer->determineQueryType();
  }

  parsers::Sql_completion_result r;

  r.context.prefix.full = get_prefix(parser, &scanner, offset);
  init_prefix_info(&r.context.prefix, is_ansi_quotes_active(*parser));

  {
    const auto &vocabulary = parser->getVocabulary();
    const auto process_entry = [](std::string *e) {
      if (e->rfind("_SYMBOL") != std::string::npos) {
        e->resize(e->size() - 7);
      } else {
        *e = unquote_string(*e);
      }
    };
    // use the full prefix here, if it begins with a quote we don't want to
    // to match any keywords
    const auto should_insert_entry =
        [filtered, &prefix = std::as_const(r.context.prefix.full)](
            const std::string &entry) {
          // entry is a keyword or a function, ASCII-based check is sufficient
          return !filtered || shcore::str_ibeginswith(entry, prefix);
        };

    bool function_call = false;

    const auto add_entry = [&should_insert_entry, &function_call, &r,
                            &uppercaseKeywords](std::string e,
                                                const char *msg) {
      if (should_insert_entry(e)) {
        if (!uppercaseKeywords) e = shcore::str_lower(e);

        if (function_call) {
          insert_system_function(&r, e);
        } else {
          insert(&r.keywords, std::move(e), msg);
        }
      }
    };

    // if we're at the beginning of a statement add the DELIMITER command
    if (scanner.tokenIndex() == 0 ||
        (scanner.tokenIndex() == 1 && scanner.tokenType() == MySQLLexer::EOF)) {
      add_entry("DELIMITER", "a command");
    }

    for (const auto &candidate : context.completionCandidates.tokens) {
      if (antlr4::Token::EOF == candidate.first ||
          antlr4::Token::EPSILON == candidate.first) {
        continue;
      }

      auto entry = vocabulary.getDisplayName(candidate.first);
      function_call = false;

      process_entry(&entry);

      if (!candidate.second.empty()) {
        // A function call?
        if (candidate.second[0] == MySQLLexer::OPEN_PAR_SYMBOL) {
          function_call = true;
        } else {
          for (auto token : candidate.second) {
            auto subentry = vocabulary.getDisplayName(token);

            process_entry(&subentry);

            if (!MySQLLexer::isOperator(token)) {
              entry += " ";
            }

            entry += subentry;
          }
        }
      }

      add_entry(std::move(entry), "a keyword");

      // Add also synonyms, if there are any.
      if (const auto s = synonyms.find(candidate.first); s != synonyms.end()) {
        for (const auto &synonym : s->second) {
          add_entry(synonym, "a synonym");
        }
      }
    }
  }

  for (const auto &candidate : context.completionCandidates.rules) {
    // Restore the scanner position to the caret position and store that value
    // again for the next round.
    scanner.pop();
    scanner.push();

    if (shcore::Logger::LOG_LEVEL::LOG_DEBUG3 ==
        shcore::current_logger()->get_log_level()) {
      const auto &rules = parser->getRuleNames();
      log_debug3("Rule: '%s' (%zu)", rules[candidate.second.front()].c_str(),
                 candidate.second.front());

      auto it = candidate.second.begin();
      while (++it != candidate.second.end()) {
        log_debug3("      '%s' (%zu)", rules[*it].c_str(), *it);
      }

      log_debug3("      '%s' (%zu)", rules[candidate.first].c_str(),
                 candidate.first);
    }

    switch (candidate.first) {
      case MySQLParser::RuleRuntimeFunctionCall: {
        log_debug3("Adding runtime function names");
        r.candidates.emplace(Candidate::RUNTIME_FUNCTION);
        break;
      }

      case MySQLParser::RuleFunctionRef:
      case MySQLParser::RuleFunctionCall: {
        std::string_view qualifier;
        ObjectFlags flags = determineQualifier(scanner, lexer, &r, &qualifier);

        if (qualifier.empty()) {
          log_debug3("Adding user defined function names");
          r.candidates.emplace(Candidate::UDF);
        }

        if ((flags & ShowFirst) != 0) insert_schemas(&r);

        if ((flags & ShowSecond) != 0) {
          if (qualifier.empty()) qualifier = active_schema;

          insert_functions(&r, qualifier);
        }

        break;
      }

      case MySQLParser::RuleEngineRef: {
        log_debug3("Adding engine names");
        r.candidates.emplace(Candidate::ENGINE);
        break;
      }

      case MySQLParser::RuleSchemaRef: {
        insert_schemas(&r);
        break;
      }

      case MySQLParser::RuleProcedureRef: {
        std::string_view qualifier;
        ObjectFlags flags = determineQualifier(scanner, lexer, &r, &qualifier);

        if ((flags & ShowFirst) != 0) insert_schemas(&r);

        if ((flags & ShowSecond) != 0) {
          if (qualifier.empty()) qualifier = active_schema;

          insert_procedures(&r, qualifier);
        }
        break;
      }

      case MySQLParser::RuleTableRefWithWildcard: {
        // A special form of table references (id.id.*) used only in multi-table
        // delete. Handling is similar as for column references (just that we
        // have table/view objects instead of column refs).
        std::string_view schema, table;
        ObjectFlags flags =
            determineSchemaTableQualifier(scanner, lexer, &r, &schema, &table);

        if ((flags & ShowSchemas) != 0) insert_schemas(&r);

        if ((flags & ShowTables) != 0) {
          if (schema.empty()) {
            schema = active_schema;
          }

          insert_tables(&r, schema);
          insert_views(&r, schema);
        }
        break;
      }

      case MySQLParser::RuleTableRef:
      case MySQLParser::RuleFilterTableRef: {
        // Tables refs - also allow view refs (conditionally).
        std::string_view qualifier;
        ObjectFlags flags = determineQualifier(scanner, lexer, &r, &qualifier);

        if ((flags & ShowFirst) != 0) insert_schemas(&r);

        if ((flags & ShowSecond) != 0) {
          if (qualifier.empty()) {
            qualifier = active_schema;
          }

          insert_tables(&r, qualifier);

          if (QtSelect == queryType) {
            insert_views(&r, qualifier);
          }
        }
        break;
      }

      case MySQLParser::RuleTableWild:
      case MySQLParser::RuleColumnRef: {
        // Try limiting what to show to the smallest set possible.
        // If we have table references show columns only from them.
        // Show columns from the default schema only if there are no _
        std::string_view schema, table;
        ObjectFlags flags =
            determineSchemaTableQualifier(scanner, lexer, &r, &schema, &table);

        if ((flags & ShowSchemas) != 0) insert_schemas(&r);

        // If a schema is given then list only tables + columns from that
        // schema. If no schema is given but we have table references use the
        // schemas from them. Otherwise use the default schema.
        // TODO: case sensitivity.
        Object_names schemas;

        if (!schema.empty()) {
          schemas.insert(schema);
        } else if (!context.references.empty()) {
          for (const auto &reference : context.references) {
            if (!reference.schema.empty()) schemas.insert(reference.schema);
          }
        }

        if (schemas.empty()) schemas.insert(active_schema);

        if ((flags & ShowTables) != 0) {
          insert_tables(&r, schemas);

          if (candidate.first == MySQLParser::RuleColumnRef) {
            // Insert also views.
            insert_views(&r, schemas);

            // Insert also tables from our references list.
            for (const auto &reference : context.references) {
              // If no schema was specified then allow also tables without a
              // given schema. Otherwise the reference's schema must match any
              // of the specified schemas (which include those from the ref
              // list).
              if ((schema.empty() && reference.schema.empty()) ||
                  (schemas.count(reference.schema) > 0)) {
                auto t =
                    reference.alias.empty() ? reference.table : reference.alias;

                // TODO(pawel): support UTF-8
                if (!filtered || shcore::str_ibeginswith(
                                     t, r.context.prefix.as_identifier)) {
                  insert_table(&r, t);
                }
              }
            }
          }
        }

        if ((flags & ShowColumns) != 0) {
          const auto add_referenced_table =
              [&active_schema, &r](const Table_reference &reference) {
                const auto s =
                    reference.schema.empty() ? active_schema : reference.schema;

                if (!s.empty()) {
                  insert_columns(&r, s, reference.table);
                }
              };

          switch (r.context.qualifier.size()) {
            case 0:
              // no qualifiers, get columns from referenced tables
              for (const auto &reference : context.references) {
                add_referenced_table(reference);
              }
              break;

            case 1:
              // if there's a default schema, get columns from the given table
              // in that schema
              if (!active_schema.empty()) {
                insert_columns(&r, active_schema, table);
              }

              // this could be an alias
              for (const auto &reference : context.references) {
                if (base::same_string(table, reference.alias)) {
                  add_referenced_table(reference);
                  break;
                }
              }

              // Special deal here: triggers. Show columns for the "new" and
              // "old" qualifiers too. Use the first reference in the list,
              // which is the table to which this trigger belongs (there can be
              // more if the trigger body references other tables).
              if (queryType == QtCreateTrigger && !context.references.empty() &&
                  (shcore::str_caseeq(table, "old") ||
                   shcore::str_caseeq(table, "new"))) {
                add_referenced_table(context.references[0]);
              }
              break;

            case 2:
              // columns from schema.table
              insert_columns(&r, schema, table);
              break;
          }
        }

        break;
      }

      case MySQLParser::RuleColumnInternalRef: {
        if (!context.references.empty()) {
          const auto &reference = context.references[0];
          const auto s =
              reference.schema.empty() ? active_schema : reference.schema;

          if (!s.empty()) {
            insert_internal_columns(&r, s, reference.table);
          }
        }
        break;
      }

      case MySQLParser::RuleTriggerRef: {
        // While triggers are bound to a table they are schema objects and are
        // referenced as "[schema.]trigger" e.g. in DROP TRIGGER.
        std::string_view qualifier;
        ObjectFlags flags = determineQualifier(scanner, lexer, &r, &qualifier);

        if ((flags & ShowFirst) != 0) insert_schemas(&r);

        if ((flags & ShowSecond) != 0) {
          if (qualifier.empty()) {
            qualifier = active_schema;
          }

          insert_triggers(&r, qualifier);
        }
        break;
      }

      case MySQLParser::RuleViewRef: {
        // View refs only (no table references), e.g. like in DROP VIEW ...
        std::string_view qualifier;
        ObjectFlags flags = determineQualifier(scanner, lexer, &r, &qualifier);

        if ((flags & ShowFirst) != 0) insert_schemas(&r);

        if ((flags & ShowSecond) != 0) {
          if (qualifier.empty()) {
            qualifier = active_schema;
          }

          insert_views(&r, qualifier);
        }
        break;
      }

      case MySQLParser::RuleLogfileGroupRef: {
        log_debug3("Adding logfile group names");
        r.candidates.emplace(Candidate::LOGFILE_GROUP);
        break;
      }

      case MySQLParser::RuleTablespaceRef: {
        log_debug3("Adding tablespace names");
        r.candidates.emplace(Candidate::TABLESPACE);
        break;
      }

      case MySQLParser::RuleUserVariableIdentifier: {
        log_debug3("Adding user variables");
        r.candidates.emplace(Candidate::USER_VAR);
        break;
      }

      case MySQLParser::RuleLabelRef: {
        log_debug3("Adding label references");
        r.candidates.emplace(Candidate::LABEL);
        break;
      }

      case MySQLParser::RuleLvalueVariable:
      case MySQLParser::RuleRvalueSystemVariable: {
        insert_system_variables(&r);
        break;
      }

      case MySQLParser::RuleCharsetName: {
        log_debug3("Adding charsets");
        r.candidates.emplace(Candidate::CHARSET);
        break;
      }

      case MySQLParser::RuleCollationName: {
        log_debug3("Adding collations");
        r.candidates.emplace(Candidate::COLLATION);
        break;
      }

      case MySQLParser::RuleEventRef: {
        std::string_view qualifier;
        ObjectFlags flags = determineQualifier(scanner, lexer, &r, &qualifier);

        if ((flags & ShowFirst) != 0) insert_schemas(&r);

        if ((flags & ShowSecond) != 0) {
          if (qualifier.empty()) {
            qualifier = active_schema;
          }

          insert_events(&r, qualifier);
        }
        break;
      }

      case MySQLParser::RuleUser: {
        log_debug3("Adding users");
        r.candidates.emplace(Candidate::USER);
        init_prefix_as_user(&scanner, &r.context);
        break;
      }

      case MySQLParser::RulePluginRef: {
        log_debug3("Adding plugins");
        r.candidates.emplace(Candidate::PLUGIN);
        break;
      }

      case MySQLParser::RuleIdentifier: {
        switch (queryType) {
          case QtShowColumns:
          case QtShowEvents:
          case QtShowIndexes:
          case QtShowOpenTables:
          case QtShowTableStatus:
          case QtShowTables:
          case QtShowTriggers:
          case QtUse: {
            std::string_view qualifier;
            const auto flags =
                determineQualifier(scanner, lexer, &r, &qualifier);

            // identifier in these queries can only by a schema, if there's
            // something like schema.prefix, it's an error
            if ((flags & ShowFirst) != 0) insert_schemas(&r);

            break;
          }

          case QtResetPersist:
            insert_system_variables(&r);
            break;

          default:
            break;
        }
        break;
      }

      case MySQLParser::RuleProcedureName:
      case MySQLParser::RuleFunctionName:
      case MySQLParser::RuleTriggerName:
      case MySQLParser::RuleViewName:
      case MySQLParser::RuleEventName:
      case MySQLParser::RuleTableName: {
        // these are used in: CREATE object object_name, object_name can be
        // qualified with schema, we can only suggest candidates if there's no
        // DOT
        std::string_view qualifier;
        const auto flags = determineQualifier(scanner, lexer, &r, &qualifier);
        if ((flags & ShowFirst) != 0) insert_schemas(&r);
        break;
      }
    }
  }

  scanner.pop();  // Clear the scanner stack.

  r.context.references = std::move(context.references);

  if (filtered) {
    r.context.labels.reserve(context.labels.size());

    for (auto &label : context.labels) {
      if (shcore::str_beginswith(label, r.context.prefix.as_identifier)) {
        r.context.labels.emplace_back(std::move(label));
      }
    }
  } else {
    r.context.labels = std::move(context.labels);
  }

  return r;
}

//----------------------------------------------------------------------------------------------------------------------

void resetPreconditions(parsers::MySQLParser *parser) {
  CodeCompletionCore::clearFollowSets(parser);
}

//----------------------------------------------------------------------------------------------------------------------
