/*
 * Copyright (c) 2016, 2022, Oracle and/or its affiliates.
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

// has to be first
#include <antlr4-runtime.h>

#include "mysqlshdk/libs/parser/code-completion/CodeCompletionCore.h"

#include <set>
#include <unordered_map>

#include "mysqlshdk/libs/utils/logger.h"

namespace antlr4 {

using atn::ATNState;
using atn::ATNStateType;
using atn::PredicateTransition;
using atn::RuleTransition;
using atn::Transition;
using atn::TransitionType;

using misc::IntervalSet;

//----------------------------------------------------------------------------------------------------------------------

std::unordered_map<std::type_index, CodeCompletionCore::FollowSetsPerState>
    CodeCompletionCore::_followSetsByATN;

//----------------------------------------------------------------------------------------------------------------------

CodeCompletionCore::CodeCompletionCore(Parser *parser)
    : _parser(parser),
      _atn(parser->getATN()),
      _vocabulary(parser->getVocabulary()),
      _ruleNames(parser->getRuleNames()) {}

//----------------------------------------------------------------------------------------------------------------------

CandidatesCollection CodeCompletionCore::collectCandidates(
    size_t caretTokenIndex, ParserRuleContext *context) {
  _shortcutMap.clear();
  _candidates.rules.clear();
  _candidates.tokens.clear();
  _statesProcessed = 0;

  _tokenStartIndex = context != nullptr ? context->start->getTokenIndex() : 0;

  // Get all token types, starting with the rule start token, up to the caret
  // token. These should all have been read already and hence do not place any
  // time penalty on us here.
  _tokens.clear();
  TokenStream *tokenStream = _parser->getTokenStream();
  size_t currentOffset = tokenStream->index();
  tokenStream->seek(_tokenStartIndex);
  size_t offset = 1;
  while (true) {
    Token *token = tokenStream->LT(offset++);
    _tokens.push_back(token->getType());

    if (token->getTokenIndex() >= caretTokenIndex ||
        _tokens.back() == Token::EOF)
      break;
  }
  tokenStream->seek(currentOffset);

  std::vector<size_t> callStack;
  size_t startRule = context != nullptr ? context->getRuleIndex() : 0;
  processRule(_atn.ruleToStartState[startRule], 0, callStack, "");

  if (showResult) {
    log_debug3("Collected rules:");
    for (const auto &rule : _candidates.rules) {
      std::string path;
      for (const auto token : rule.second) {
        path += _ruleNames[token] + " ";
      }
      log_debug3(" - %s, path: %s", _ruleNames[rule.first].c_str(),
                 path.c_str());
    }

    std::set<std::string> sortedTokens;
    for (const auto &token : _candidates.tokens) {
      std::string value = _vocabulary.getDisplayName(token.first);
      for (const auto following : token.second)
        value += " " + _vocabulary.getDisplayName(following);
      sortedTokens.emplace(std::move(value));
    }
    log_debug3("Collected tokens:");
    for (const auto &symbol : sortedTokens) {
      log_debug3(" - %s", symbol.c_str());
    }
  }

  return _candidates;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Checks if the predicate associated with the given transition evaluates to
 * true.
 */
bool CodeCompletionCore::checkPredicate(
    const PredicateTransition *transition) const {
  return transition->getPredicate()->eval(_parser, &ParserRuleContext::EMPTY);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Walks the rule chain upwards to see if that matches any of the preferred
 * rules. If found, that rule is added to the collection candidates and true is
 * returned.
 */
bool CodeCompletionCore::translateToRuleIndex(
    std::vector<size_t> const &ruleStack) {
  if (preferredRules.empty()) return false;

  // Loop over the rule stack from highest to lowest rule level. This way we
  // properly handle the higher rule if it contains a lower one that is also a
  // preferred rule.
  for (size_t i = 0, s = ruleStack.size(); i < s; ++i) {
    const auto entry = ruleStack[i];

    if (preferredRules.count(entry)) {
      // Add the rule to our candidates list along with the current rule path,
      // but only if there isn't already an entry like that.
      auto &rule = _candidates.rules[entry];

      if (rule.empty()) {
        rule = RuleList(ruleStack.begin(), ruleStack.begin() + i);
        if (showDebugOutput) {
          log_debug3("=====> collected: %s", _ruleNames[entry].c_str());
        }
      }

      return true;
    }
  }

  return false;
}

//----------------------------------------------------------------------------------------------------------------------

void CodeCompletionCore::printRuleState(
    std::string const &currentIndent, std::vector<size_t> const &stack) const {
  log_debug3("%s  Rule stack:", currentIndent.c_str());

  if (stack.empty()) {
    log_debug3("%s   <empty stack>", currentIndent.c_str());
    return;
  }

  for (size_t rule : stack) {
    log_debug3("%s   - %s", currentIndent.c_str(), _ruleNames[rule].c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * This method follows the given transition and collects all symbols within the
 * same rule that directly follow it without intermediate transitions to other
 * rules and only if there is a single symbol for a transition.
 */
std::vector<size_t> CodeCompletionCore::getFollowingTokens(
    const Transition *t) const {
  std::vector<size_t> result;

  std::vector<ATNState *> pipeline{t->target};

  while (!pipeline.empty()) {
    ATNState *state = pipeline.back();
    pipeline.pop_back();

    for (const auto &transition : state->transitions) {
      if (transition->getTransitionType() == TransitionType::ATOM) {
        if (!transition->isEpsilon()) {
          std::vector<ssize_t> list = transition->label().toList();
          if (list.size() == 1) {
            result.push_back(list[0]);
            pipeline.push_back(transition->target);
          }
        } else {
          pipeline.push_back(transition->target);
        }
      }
    }
  }

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Entry point for the recursive follow set collection function.
 */
CodeCompletionCore::FollowSetsList CodeCompletionCore::determineFollowSets(
    ATNState *start, ATNState *stop) const {
  FollowSetsList result;

  std::vector<size_t> ruleStack;
  collectFollowSets(start, stop, result, ruleStack);

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Collects possible tokens which could be matched following the given ATN
 * state. This is essentially the same algorithm as used in the LL1Analyzer
 * class, but here we consider predicates also and use no parser rule context.
 *
 * We don't keep track of states we've seen, because we may hit the same state
 * along a different path.
 */
void CodeCompletionCore::collectFollowSets(
    ATNState *s, ATNState *stopState, FollowSetsList &followSets,
    std::vector<size_t> &ruleStack) const {
  if (s == stopState || s->getStateType() == ATNStateType::RULE_STOP) {
    followSets.push_back({IntervalSet::of(Token::EPSILON), ruleStack, {}});
    return;
  }

  for (const auto &transition : s->transitions) {
    if (transition->getTransitionType() == TransitionType::RULE) {
      const auto ruleTransition =
          static_cast<const RuleTransition *>(transition.get());
      if (std::find(ruleStack.rbegin(), ruleStack.rend(),
                    ruleTransition->target->ruleIndex) != ruleStack.rend()) {
        continue;
      }

      ruleStack.push_back(ruleTransition->target->ruleIndex);
      collectFollowSets(transition->target, stopState, followSets, ruleStack);
      ruleStack.pop_back();

    } else if (transition->getTransitionType() == TransitionType::PREDICATE) {
      if (checkPredicate(
              static_cast<const PredicateTransition *>(transition.get())))
        collectFollowSets(transition->target, stopState, followSets, ruleStack);
    } else if (transition->isEpsilon()) {
      collectFollowSets(transition->target, stopState, followSets, ruleStack);
    } else if (transition->getTransitionType() == TransitionType::WILDCARD) {
      followSets.push_back({IntervalSet::of(Token::MIN_USER_TOKEN_TYPE,
                                            (ssize_t)_atn.maxTokenType),
                            ruleStack,
                            {}});
    } else {
      misc::IntervalSet set = transition->label();
      if (!set.isEmpty()) {
        if (transition->getTransitionType() == TransitionType::NOT_SET) {
          set = set.complement(misc::IntervalSet::of(
              Token::MIN_USER_TOKEN_TYPE, (ssize_t)_atn.maxTokenType));
        }
        followSets.push_back(
            {set, ruleStack, getFollowingTokens(transition.get())});
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Walks the ATN for a single rule only. It returns the token stream position
 * for each path that could be matched in this rule. The result can be empty in
 * case we hit only non-epsilon transitions that didn't match the current input
 * or if we hit the caret position.
 */
CodeCompletionCore::RuleEndStatus CodeCompletionCore::processRule(
    ATNState *startState, size_t tokenIndex, std::vector<size_t> &callStack,
    std::string indentation) {
  // Start with rule specific handling before going into the ATN walk.

  // Check first if we've taken this path with the same input before.
  auto &positionMap = _shortcutMap[startState->ruleIndex];
  if (positionMap.find(tokenIndex) != positionMap.end()) {
    if (showDebugOutput) {
      log_debug3("=====> shortcut");
    }
    return positionMap[tokenIndex];
  }

  RuleEndStatus result;

  // For rule start states we determine and cache the follow set, which gives us
  // 3 advantages: 1) We can quickly check if a symbol would be matched when we
  // follow that rule. We can so check in advance
  //    and can save us all the intermediate steps if there is no match.
  // 2) We'll have all symbols that are collectable already together when we are
  // at the caret when entering a rule. 3) We get this lookup for free with any
  // 2nd or further visit of the same rule, which often happens
  //    in non trivial grammars, especially with (recursive) expressions and of
  //    course when invoking code completion multiple times.
  FollowSetsPerState &setsPerState = _followSetsByATN[typeid(_parser)];
  if (setsPerState.count(startState->stateNumber) == 0) {
    ATNState *stop = _atn.ruleToStopState[startState->ruleIndex];
    setsPerState[startState->stateNumber].sets =
        determineFollowSets(startState, stop);

    // Sets are split by path to allow translating them to preferred rules. But
    // for quick hit tests it is also useful to have a set with all symbols
    // combined.
    IntervalSet combined;
    for (auto &set : setsPerState[startState->stateNumber].sets)
      combined.addAll(set.intervals);
    setsPerState[startState->stateNumber].combined = combined;
  }

  const auto &followSets = setsPerState[startState->stateNumber];
  callStack.push_back(startState->ruleIndex);

  if (tokenIndex >= _tokens.size() - 1) {  // At caret?
    if (preferredRules.count(startState->ruleIndex) > 0) {
      // No need to go deeper when collecting entries and we reach a rule that
      // we want to collect anyway.
      translateToRuleIndex(callStack);
    } else {
      // Convert all follow sets to either single symbols or their associated
      // preferred rule and add the result to our candidates list.
      auto fullPath = callStack;
      const auto size = callStack.size();

      for (const auto &set : followSets.sets) {
        fullPath.resize(size);
        fullPath.insert(fullPath.end(), set.path.begin(), set.path.end());

        if (!translateToRuleIndex(fullPath)) {
          for (ssize_t symbol : set.intervals.toList()) {
            if (ignoredTokens.count(symbol) == 0) {
              if (showDebugOutput) {
                std::string debug = _vocabulary.getDisplayName(symbol);

                if (!set.following.empty()) {
                  debug += " ->";

                  for (const auto follows : set.following) {
                    debug += " " + _vocabulary.getDisplayName(follows);
                  }
                }

                log_debug3("=====> collected: %s", debug.c_str());
              }

              // Following is empty if there is more than one entry in the set.
              auto token =
                  _candidates.tokens.try_emplace(symbol, set.following);

              if (!token.second) {
                // Insertion didn't happen, more than one following list for the
                // same symbol.
                if (token.first->second != set.following) {
                  token.first->second.clear();
                }
              }
            }
          }
        }
      }
    }

    callStack.pop_back();
    return {};

  } else {
    // Process the rule if we either could pass it without consuming anything
    // (epsilon transition) or if the current input symbol will be matched
    // somewhere after this entry point.
    size_t currentSymbol = _tokens[tokenIndex];
    if (!followSets.combined.contains(Token::EPSILON) &&
        !followSets.combined.contains(currentSymbol)) {
      callStack.pop_back();
      return {};
    }
  }

  // The current state execution pipeline contains all yet-to-be-processed ATN
  // states in this rule. For each such state we store the token index + a list
  // of rules that lead to it.
  std::vector<PipelineEntry> statePipeline;
  PipelineEntry currentEntry;

  // Bootstrap the pipeline.
  statePipeline.push_back({startState, tokenIndex});

  while (!statePipeline.empty()) {
    currentEntry = statePipeline.back();
    statePipeline.pop_back();
    ++_statesProcessed;

    bool atCaret = currentEntry.tokenIndex >= _tokens.size() - 1;
    if (showDebugOutput) {
      printDescription(indentation, currentEntry.state,
                       generateBaseDescription(currentEntry.state),
                       currentEntry.tokenIndex);
      if (showRuleStack) printRuleState(indentation, callStack);
    }

    switch (currentEntry.state->getStateType()) {
      case ATNStateType::RULE_START:  // Happens only for the first state in
                                      // this rule, not subrules.
        indentation += "  ";
        break;

      // Found the end of this rule. Determine the following states and return
      // to the caller.
      case ATNStateType::RULE_STOP: {
        // Record the token index we are at, to report it to the caller.
        result.insert(currentEntry.tokenIndex);
        continue;
      }

      default:
        break;
    }

    for (const auto &transition : currentEntry.state->transitions) {
      switch (transition->getTransitionType()) {
        case TransitionType::RULE: {
          auto endStatus =
              processRule(transition->target, currentEntry.tokenIndex,
                          callStack, indentation);
          for (auto &status : endStatus)
            statePipeline.push_back(
                {static_cast<const RuleTransition *>(transition.get())
                     ->followState,
                 status});
          break;
        }

        case TransitionType::PREDICATE: {
          if (checkPredicate(
                  static_cast<const PredicateTransition *>(transition.get())))
            statePipeline.push_back(
                {transition->target, currentEntry.tokenIndex});
          break;
        }

        case TransitionType::WILDCARD: {
          if (atCaret) {
            if (!translateToRuleIndex(callStack)) {
              for (ssize_t token :
                   misc::IntervalSet::of(Token::MIN_USER_TOKEN_TYPE,
                                         (ssize_t)_atn.maxTokenType)
                       .toList())
                if (ignoredTokens.count(token) == 0)
                  _candidates.tokens[token].clear();
            }
          } else {
            statePipeline.push_back(
                {transition->target, currentEntry.tokenIndex + 1});
          }
          break;
        }

        default: {
          if (transition->isEpsilon()) {
            if (atCaret) {
              translateToRuleIndex(callStack);
            }

            // Jump over simple states with a single outgoing epsilon
            // transition.
            statePipeline.push_back(
                {transition->target, currentEntry.tokenIndex});
            continue;
          }

          misc::IntervalSet set = transition->label();
          if (!set.isEmpty()) {
            if (transition->getTransitionType() == TransitionType::NOT_SET) {
              set = set.complement(misc::IntervalSet::of(
                  Token::MIN_USER_TOKEN_TYPE, (ssize_t)_atn.maxTokenType));
            }
            if (atCaret) {
              if (!translateToRuleIndex(callStack)) {
                std::vector<ssize_t> list = set.toList();
                bool addFollowing = list.size() == 1;
                for (ssize_t symbol : list)
                  if (ignoredTokens.count(symbol) == 0) {
                    TokenList following;

                    if (addFollowing) {
                      following = getFollowingTokens(transition.get());
                    }

                    if (showDebugOutput) {
                      std::string debug = _vocabulary.getDisplayName(symbol);

                      if (!following.empty()) {
                        debug += " ->";

                        for (const auto follows : following) {
                          debug += " " + _vocabulary.getDisplayName(follows);
                        }
                      }

                      log_debug3("=====> collected: %s", debug.c_str());
                    }

                    auto token =
                        _candidates.tokens.try_emplace(symbol, following);

                    if (!token.second) {
                      // Insertion didn't happen, more than one following list
                      // for the same symbol.
                      if (token.first->second != following) {
                        token.first->second.clear();
                      }
                    }
                  }
              }
            } else {
              size_t currentSymbol = _tokens[currentEntry.tokenIndex];
              if (set.contains(currentSymbol)) {
                if (showDebugOutput) {
                  log_debug3("=====> consumed: %s",
                             _vocabulary.getDisplayName(currentSymbol).c_str());
                }

                statePipeline.push_back(
                    {transition->target, currentEntry.tokenIndex + 1});
              }
            }
          }
          break;
        }
      }
    }
  }

  callStack.pop_back();
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

std::string CodeCompletionCore::generateBaseDescription(ATNState *state) const {
  std::string stateValue = state->stateNumber == ATNState::INVALID_STATE_NUMBER
                               ? "Invalid"
                               : std::to_string(state->stateNumber);
  return "[" + stateValue + " " + atnStateTypeName(state->getStateType()) +
         "] in " + _ruleNames[state->ruleIndex];
}

//----------------------------------------------------------------------------------------------------------------------

void CodeCompletionCore::printDescription(std::string const &currentIndent,
                                          ATNState *state,
                                          std::string const &baseDescription,
                                          size_t tokenIndex) const {
  {
    std::string description;

    if (tokenIndex >= _tokens.size() - 1) {
      description += "<<" + std::to_string(_tokenStartIndex + tokenIndex) +
                     ">> ";  // Show the absolute token index.
    } else {
      description += "<" + std::to_string(_tokenStartIndex + tokenIndex) + "> ";
    }

    log_debug3("%s%sCurrent state: %s", currentIndent.c_str(),
               description.c_str(), baseDescription.c_str());
  }

  if (debugOutputWithTransitions) {
    for (const auto &transition : state->transitions) {
      std::string labels;
      auto symbols = transition->label().toList();
      if (symbols.size() > 2) {
        // Only print start and end symbols to avoid large lists in debug
        // output.
        labels = _vocabulary.getDisplayName(symbols.front()) + " .. " +
                 _vocabulary.getDisplayName(symbols.back());
      } else {
        for (auto symbol : symbols) {
          if (!labels.empty()) labels += ", ";
          labels += _vocabulary.getDisplayName(symbol);
        }
      }
      if (labels.empty()) labels = "ε";

      log_debug3("%s  (%s) -> [%zu %s] in %s", currentIndent.c_str(),
                 labels.c_str(), transition->target->stateNumber,
                 atnStateTypeName(transition->target->getStateType()).c_str(),
                 _ruleNames[transition->target->ruleIndex].c_str());
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

void CodeCompletionCore::clearFollowSets(antlr4::Parser *parser) {
  if (parser) {
    _followSetsByATN.erase(typeid(parser));
  } else {
    _followSetsByATN.clear();
  }
}

//----------------------------------------------------------------------------------------------------------------------

}  // namespace antlr4
