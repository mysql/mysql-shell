/*
 * Copyright (c) 2016, 2024, Oracle and/or its affiliates.
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

#include <map>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace antlr4 {
class Parser;

namespace dfa {
class Vocabulary;
}

namespace atn {
class ATN;
}

using TokenList = std::vector<size_t>;
using RuleList = std::vector<size_t>;

// All the candidates which have been found. Tokens and rules are separated
// (both use a numeric value).
struct CandidatesCollection {
  std::map<size_t, TokenList> tokens;
  std::map<size_t, RuleList> rules;
};

// The main class for doing the collection process.
class CodeCompletionCore {
 public:
  // Debugging options. Print human readable ATN state and other info.
  bool showResult = false;       // Not dependent on showDebugOutput. Prints the
                                 // collected rules + tokens to terminal.
  bool showDebugOutput = false;  // Enables printing ATN state info to terminal.
  bool debugOutputWithTransitions =
      false;  // Only relevant when showDebugOutput is true. Enables transition
              // printing for a state.
  bool showRuleStack = false;  // Also depends on showDebugOutput. Enables call
                               // stack printing for each rule recursion.

  // Tailoring of the result.
  std::unordered_set<size_t>
      ignoredTokens;  // Tokens which should not appear in the candidates set.
  std::unordered_set<size_t>
      preferredRules;  // Rules which replace any candidate token they contain.
                       // This allows to return descriptive rules (e.g.
                       // className, instead of ID/identifier).
  explicit CodeCompletionCore(antlr4::Parser *parser);

  CandidatesCollection collectCandidates(size_t caretTokenIndex,
                                         ParserRuleContext *context);

  /**
   * Clears follow sets.
   *
   * If parser is nullptr, all follow sets are cleared.
   *
   * @param parser Parser type whose follow sets should be cleared.
   */
  static void clearFollowSets(antlr4::Parser *parser);

 private:
  // Token stream position info after a rule was processed.
  using RuleEndStatus = std::unordered_set<size_t>;

  antlr4::Parser *_parser;
  antlr4::atn::ATN const &_atn;

  antlr4::dfa::Vocabulary const &_vocabulary;
  std::vector<std::string> const &_ruleNames;

  // Just the token types in input order - and only those from the rule start
  // index to the caret token index.
  std::vector<size_t> _tokens;

  size_t _tokenStartIndex;  // The index of the token which is the start token
                            // in a given parser rule context.

  size_t _statesProcessed;
  std::unordered_map<size_t, std::unordered_map<size_t, RuleEndStatus>>
      _shortcutMap;
  CandidatesCollection
      _candidates;  // The collected candidates (rules and tokens).

  // A record for a follow set along with the path at which this set was found.
  // If there is only a single symbol in the interval set then we also collect
  // and store tokens which follow this symbol directly in its rule (i.e. there
  // is no intermediate rule transition). Only single label transitions are
  // considered. This is useful if you have a chain of tokens which can be
  // suggested as a whole, because there is a fixed sequence in the grammar.
  using FollowSetWithPath = struct {
    antlr4::misc::IntervalSet intervals;
    RuleList path;
    TokenList following;
  };
  using FollowSetsList = std::vector<FollowSetWithPath>;

  // A list of follow sets (for a given state number) + all of them combined for
  // quick hit tests. This data is static in nature (because the used ATN states
  // are part of a static struct: the ATN) as long as preconditions are
  // constant. Hence it can be shared between all C3 instances, however it
  // depends on the actual parser class (type). If preconditions change, call
  // clearFollowSets() to reset the sets.
  using FollowSetsHolder = struct {
    FollowSetsList sets;
    antlr4::misc::IntervalSet combined;
  };

  using FollowSetsPerState = std::unordered_map<size_t, FollowSetsHolder>;
  static std::unordered_map<std::type_index, FollowSetsPerState>
      _followSetsByATN;

  struct PipelineEntry {
    antlr4::atn::ATNState *state;
    size_t tokenIndex;
  };

  bool checkPredicate(const antlr4::atn::PredicateTransition *transition) const;
  bool translateToRuleIndex(std::vector<size_t> const &ruleStack);
  void printRuleState(std::string const &currentIndent,
                      std::vector<size_t> const &stack) const;

  TokenList getFollowingTokens(const antlr4::atn::Transition *transition) const;
  FollowSetsList determineFollowSets(antlr4::atn::ATNState *start,
                                     antlr4::atn::ATNState *stop) const;
  void collectFollowSets(antlr4::atn::ATNState *s,
                         antlr4::atn::ATNState *stopState,
                         FollowSetsList &followSets,
                         std::vector<size_t> &ruleStack) const;

  RuleEndStatus processRule(antlr4::atn::ATNState *startState,
                            size_t tokenIndex, std::vector<size_t> &callStack,
                            std::string indentation);

  std::string generateBaseDescription(antlr4::atn::ATNState *state) const;
  void printDescription(std::string const &currentIndent,
                        antlr4::atn::ATNState *state,
                        std::string const &baseDescription,
                        size_t tokenIndex) const;
};

}  // namespace antlr4
