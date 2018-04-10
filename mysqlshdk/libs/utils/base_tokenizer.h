/*
 * Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _BASE_TOKENIZER_H_
#define _BASE_TOKENIZER_H_

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

// Avoid warnings from includes of other project and protobuf
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#elif defined _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4018 4996)
#endif

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#elif defined _MSC_VER
#pragma warning(pop)
#endif

namespace shcore {
class BaseToken {
 public:
  BaseToken(const std::string &type, const std::string &text, int cur_pos);

  const std::string &get_text() const { return _text; }
  const std::string &get_type() const { return _type; }
  int get_pos() const { return _pos; }

 private:
  std::string _type;
  std::string _text;
  int _pos;
};

/**
 * This will enable working with different methods to tokenize strings.
 *
 * It was introduced for URI parsing where a fixed Token set can'e be used for
 * the parsing of the different components on a URI.
 */
class BaseTokenizer {
 public:
  BaseTokenizer();
  void set_input(const std::string &input) { _input = input; }
  void process(const std::pair<size_t, size_t> range);
  void reset();

  typedef std::vector<BaseToken> tokens_t;

  bool next_char_is(tokens_t::size_type i, int tok);
  void assert_cur_token(const std::string &type);
  bool cur_token_type_is(const std::string &type);
  bool next_token_type(const std::string &type, size_t pos = 1);
  bool pos_token_type_is(tokens_t::size_type pos, const std::string &type);
  const std::string &consume_token(const std::string &type);
  const BaseToken &peek_token();
  const BaseToken *peek_last_token();
  void unget_token();
  void inc_pos_token();
  int get_token_pos() { return _pos; }
  const BaseToken &consume_any_token();
  void assert_tok_position();
  bool tokens_available();

  std::vector<BaseToken>::const_iterator begin() const {
    return _tokens.begin();
  }
  std::vector<BaseToken>::const_iterator end() const { return _tokens.end(); }

  // int get_quoted_token(char quote, size_t &index);
  void get_tokens(size_t start, size_t end);
  const std::string &get_input() { return _input; }
  void add_token(const BaseToken &token);

 protected:
  std::vector<BaseToken> _tokens;
  std::string _input;
  tokens_t::size_type _pos;
  size_t _parent_offset;

 public:
  // These functions set the rulse to be followed when tokenizing a string.
  // Sets a rule for a token that would generate a fixed string
  void set_custom_token(const std::string &type, const std::string &text) {
    _base_tokens[type] = text;
  }

  // Sets rule for single char tokens returning the token itself
  // Every received char creates a separate token
  void set_simple_tokens(const std::string &tokens) {
    for (auto token : tokens) {
      _base_tokens[std::string(&token, 1)] = token;
    }
  };

  // To remove simple tokens
  void remove_simple_tokens(const std::string &tokens) {
    for (auto token : tokens) {
      _base_tokens.erase(std::string(&token, 1));
    }
  };

  // Sets rule for tokens formed based on several single tokens, each token must
  // fall into a specified set Example if the next rule is defined: "PCTE",
  // ["%", "01234567890ABCDEF", "01234567890ABCDEF"] A token of type "PCTE will
  // be created whenever a sequence like %XX appears on the string, X is "
  void set_complex_token(const std::string &type,
                         const std::vector<std::string> &groups);

  // Sets rule for tokens created based on a custom function
  // this is meant o be used with labndas defning a token based on whatever
  // logic, so for complicated cases
  void set_complex_token_callback(
      const std::string &type,
      std::function<bool(const std::string &input, size_t &, std::string &)>
          func);

  // Sets rule for tokens created with sequences of chars
  // A token of the given type would be created with all the consecutive
  // characters that contained in the defined group IE: "DIGITS", "01234567890"
  void set_complex_token(const std::string &type, const std::string &group);

  void remove_complex_token(const std::string &name);

  // Enable or disable spaces, i.e. if disabled and found, an error will be
  // raised
  void set_allow_spaces(bool allow = true) { _allow_spaces = allow; }

  // Enable or disable spaces, i.e. if disabled and found, an error will be
  // raised
  void set_allow_unknown_tokens(bool allow = true) {
    _allow_unknown_tokens = allow;
  }

  // Sets the tokens that mark the end of the parsing, remaining input is
  // returned as a final type token
  void set_final_token_group(const std::string &type,
                             const std::string &group) {
    _final_type = type;
    _final_group = group;
  };

 private:
  bool _allow_spaces;
  bool _allow_unknown_tokens;
  std::string _unknown_token;
  std::string _final_type;
  std::string _final_group;

  std::map<std::string, std::string> _base_tokens;
  std::vector<std::string> _custom_tokens;
  std::map<std::string, std::string> _token_sequences;
  std::map<std::string, std::vector<std::string>> _token_vectors;
  std::map<std::string, std::function<bool(const std::string &input, size_t &,
                                           std::string &)>>
      _token_functions;
};
}  // namespace shcore

#endif
