/*
 * Copyright (c) 2015, 2024, Oracle and/or its affiliates.
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

#ifndef _TOKENIZER_H_
#define _TOKENIZER_H_

#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace mysqlx {
class Token {
 public:
  enum class Type {
    NOT = 1,
    AND = 2,
    OR = 3,
    XOR = 4,
    IS = 5,
    LPAREN = 6,
    RPAREN = 7,
    LSQBRACKET = 8,
    RSQBRACKET = 9,
    BETWEEN = 10,
    TRUE_ = 11,
    T_NULL = 12,
    FALSE_ = 13,
    IN_ = 14,
    LIKE = 15,
    INTERVAL = 16,
    REGEXP = 17,
    ESCAPE = 18,
    IDENT = 19,
    LSTRING = 20,
    LNUM = 21,
    DOT = 22,
    // AT = 23,
    COMMA = 24,
    EQ = 25,
    NE = 26,
    GT = 27,
    GE = 28,
    LT = 29,
    LE = 30,
    BITAND = 31,
    BITOR = 32,
    BITXOR = 33,
    LSHIFT = 34,
    RSHIFT = 35,
    PLUS = 36,
    MINUS = 37,
    MUL = 38,
    DIV = 39,
    HEX = 40,
    BIN = 41,
    NEG = 42,
    BANG = 43,
    MICROSECOND = 44,
    SECOND = 45,
    MINUTE = 46,
    HOUR = 47,
    DAY = 48,
    WEEK = 49,
    MONTH = 50,
    QUARTER = 51,
    YEAR = 52,
    PLACEHOLDER = 53,
    DOUBLESTAR = 54,
    MOD = 55,
    AS = 56,
    ASC = 57,
    DESC = 58,
    CAST = 59,
    CHARACTER = 60,
    SET = 61,
    CHARSET = 62,
    ASCII = 63,
    UNICODE = 64,
    BYTE = 65,
    BINARY = 66,
    CHAR = 67,
    NCHAR = 68,
    DATE = 69,
    DATETIME = 70,
    TIME = 71,
    DECIMAL = 72,
    SIGNED = 73,
    UNSIGNED = 74,
    INTEGER = 75,   // 'integer' keyword
    LINTEGER = 76,  // integer number
    DOLLAR = 77,
    JSON = 78,
    COLON = 79,
    LCURLY = 80,
    RCURLY = 81,
    ARROW = 82,  // literal ->
    QUOTE = 83,
    OVERLAPS = 84,
    TWOHEADARROW = 85  // literal ->>
  };

  Token(Token::Type type, const std::string &text, size_t cur_pos);

  const std::string &get_text() const { return m_text; }
  Type get_type() const { return m_type; }
  const std::string &get_type_name() const;
  size_t get_pos() const { return m_pos; }
  size_t get_length() const { return m_text.length(); }

 private:
  Type m_type;
  std::string m_text;
  size_t m_pos;
};

class Tokenizer {
 public:
  explicit Tokenizer(const std::string &input);

  typedef std::vector<Token> tokens_t;

  bool next_char_is(tokens_t::size_type i, int tok);
  void assert_cur_token(Token::Type type);
  bool cur_token_type_is(Token::Type type);
  bool cur_token_type_is_keyword();
  bool next_token_type(Token::Type type);
  bool pos_token_type_is(tokens_t::size_type pos, Token::Type type);
  const std::string &consume_token(Token::Type type);
  const Token &peek_token();
  void unget_token();
  void inc_pos_token();
  int get_token_pos() { return static_cast<int>(_pos); }
  const Token &consume_any_token();
  void assert_tok_position();
  bool tokens_available() const;
  bool is_interval_units_type();
  bool is_type_within_set(const std::set<Token::Type> &types);

  std::vector<Token>::const_iterator begin() const { return _tokens.begin(); }
  std::vector<Token>::const_iterator end() const { return _tokens.end(); }

  void get_tokens();
  const std::string &get_input() { return _input; }

 protected:
  std::vector<Token> _tokens;
  std::string _input;
  tokens_t::size_type _pos;

 public:
  struct Cmp_icase {
    bool operator()(const std::string &lhs, const std::string &rhs) const;
  };

  struct Maps {
    typedef std::map<std::string, Token::Type, Cmp_icase> reserved_words_t;
    reserved_words_t reserved_words;
    std::set<Token::Type> interval_units;
    std::map<std::string, std::string, Cmp_icase> operator_names;
    std::map<std::string, std::string, Cmp_icase> unary_operator_names;

    Maps();
  };

 public:
  static Maps map;
};

class Parser_error : public std::runtime_error {
 public:
  explicit Parser_error(const std::string &msg) : std::runtime_error(msg) {}
  Parser_error(const std::string &msg, const Token &token,
               const std::string &line = "");

 private:
  static std::string format(const Token &token, const std::string &line);
};

const std::string &to_string(const Token::Type &type);

}  // namespace mysqlx

#endif
