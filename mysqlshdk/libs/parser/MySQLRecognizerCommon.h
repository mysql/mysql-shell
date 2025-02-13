/*
 * Copyright (c) 2016, 2025, Oracle and/or its affiliates.
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

#pragma once

#include <cstdint>
#include <string>
#include <utility>

#include "mysqlshdk/libs/parser/base/symbol-info.h"
#include "mysqlshdk/libs/parser/parsers-common.h"

namespace antlr4 {
class RuleContext;
class ParserRuleContext;

namespace tree {
class ParseTree;
}

namespace dfa {
class Vocabulary;
}
}  // namespace antlr4

namespace parsers {

class PARSERS_PUBLIC_TYPE MySQLLexer;

/**
 * This class describes functionality found in both, lexer and parser classes.
 */
class PARSERS_PUBLIC_TYPE MySQLRecognizerCommon {
 public:
  /** SQL modes that control parsing behavior. */
  enum SqlMode {
    NoMode = 0,
    AnsiQuotes = 1 << 0,
    HighNotPrecedence = 1 << 1,
    PipesAsConcat = 1 << 2,
    IgnoreSpace = 1 << 3,
    NoBackslashEscapes = 1 << 4
  };

  /** The server version to use for lexing/parsing. */
  uint32_t serverVersion;

  /** SQL modes to use for lexing/parsing. */
  SqlMode sqlMode;

  /** Enable Multi Language Extension support. */
  bool supportMle = true;

  /** @returns true if the given mode (one of the enums above) is set. */
  bool isSqlModeActive(size_t mode) const;

  /**
   * Parses the given mode string and keeps all found SQL mode in a private
   * member. Only used for lexer.
   */
  void sqlModeFromString(std::string modes);

  static std::string dumpTree(antlr4::RuleContext *context,
                              const antlr4::dfa::Vocabulary &vocabulary);
  static std::string sourceTextForContext(antlr4::ParserRuleContext *ctx,
                                          bool keepQuotes = false);
  static std::string sourceTextForRange(antlr4::Token *start,
                                        antlr4::Token *stop,
                                        bool keepQuotes = false);
  static std::string sourceTextForRange(antlr4::tree::ParseTree *start,
                                        antlr4::tree::ParseTree *stop,
                                        bool keepQuotes = false);

  static antlr4::tree::ParseTree *getPrevious(antlr4::tree::ParseTree *tree);
  static antlr4::tree::ParseTree *getNext(antlr4::tree::ParseTree *tree);
  static antlr4::tree::ParseTree *terminalFromPosition(
      antlr4::tree::ParseTree *root, std::pair<size_t, size_t> position);
  static antlr4::tree::ParseTree *contextFromPosition(
      antlr4::tree::ParseTree *root, size_t position);
};

class SymbolTable;

// Returns a symbol table for all predefined system functions in MySQL.
PARSERS_PUBLIC_TYPE SymbolTable *functionSymbolsForVersion(
    base::MySQLVersion version);
}  // namespace parsers
