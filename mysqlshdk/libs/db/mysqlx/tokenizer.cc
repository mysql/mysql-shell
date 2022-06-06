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

#include "mysqlshdk/libs/db/mysqlx/tokenizer.h"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlx {

namespace {

const std::map<Token::Type, std::string> k_token_name = {
    {Token::Type::NOT, "NOT"},
    {Token::Type::AND, "AND"},
    {Token::Type::OR, "OR"},
    {Token::Type::XOR, "XOR"},
    {Token::Type::IS, "IS"},
    {Token::Type::LPAREN, "("},
    {Token::Type::RPAREN, ")"},
    {Token::Type::LSQBRACKET, "["},
    {Token::Type::RSQBRACKET, "]"},
    {Token::Type::BETWEEN, "BETWEEN"},
    {Token::Type::OVERLAPS, "OVERLAPS"},
    {Token::Type::TRUE_, "TRUE"},
    {Token::Type::T_NULL, "NULL"},
    {Token::Type::FALSE_, "FALSE"},
    {Token::Type::IN_, "IN"},
    {Token::Type::LIKE, "LIKE"},
    {Token::Type::INTERVAL, "INTERVAL"},
    {Token::Type::REGEXP, "REGEXP"},
    {Token::Type::ESCAPE, "ESCAPE"},
    {Token::Type::IDENT, "identifier"},
    {Token::Type::LSTRING, "string"},
    {Token::Type::LNUM, "number"},
    {Token::Type::DOT, "."},
    //{Token::Type::AT, "AT"},
    {Token::Type::COMMA, ","},
    {Token::Type::EQ, "="},
    {Token::Type::NE, "!="},
    {Token::Type::GT, ">"},
    {Token::Type::GE, ">="},
    {Token::Type::LT, "<"},
    {Token::Type::LE, "<="},
    {Token::Type::BITAND, "&"},
    {Token::Type::BITOR, "|"},
    {Token::Type::BITXOR, "^"},
    {Token::Type::LSHIFT, "<<"},
    {Token::Type::RSHIFT, ">>"},
    {Token::Type::PLUS, "+"},
    {Token::Type::MINUS, "-"},
    {Token::Type::MUL, "*"},
    {Token::Type::DIV, "/"},
    {Token::Type::HEX, "HEX"},
    {Token::Type::BIN, "BIN"},
    {Token::Type::NEG, "~"},
    {Token::Type::BANG, "!"},
    {Token::Type::MICROSECOND, "MICROSECOND"},
    {Token::Type::SECOND, "SECOND"},
    {Token::Type::MINUTE, "MINUTE"},
    {Token::Type::HOUR, "HOUR"},
    {Token::Type::DAY, "DAY"},
    {Token::Type::WEEK, "WEEK"},
    {Token::Type::MONTH, "MONTH"},
    {Token::Type::QUARTER, "QUARTER"},
    {Token::Type::YEAR, "YEAR"},
    {Token::Type::PLACEHOLDER, "?"},
    {Token::Type::DOUBLESTAR, "**"},
    {Token::Type::MOD, "%"},
    {Token::Type::AS, "AS"},
    {Token::Type::ASC, "ASC"},
    {Token::Type::DESC, "DESC"},
    {Token::Type::CAST, "CAST"},
    {Token::Type::CHARACTER, "CHARACTER"},
    {Token::Type::SET, "SET"},
    {Token::Type::CHARSET, "CHARSET"},
    {Token::Type::ASCII, "ASCII"},
    {Token::Type::UNICODE, "UNICODE"},
    {Token::Type::BYTE, "BYTE"},
    {Token::Type::BINARY, "BINARY"},
    {Token::Type::CHAR, "CHAR"},
    {Token::Type::NCHAR, "NCHAR"},
    {Token::Type::DATE, "DATE"},
    {Token::Type::DATETIME, "DATETIME"},
    {Token::Type::TIME, "TIME"},
    {Token::Type::DECIMAL, "DECIMAL"},
    {Token::Type::SIGNED, "SIGNED"},
    {Token::Type::UNSIGNED, "UNSIGNED"},
    {Token::Type::INTEGER, "INTEGER"},
    {Token::Type::LINTEGER, "integer"},
    {Token::Type::DOLLAR, "$"},
    {Token::Type::JSON, "JSON"},
    {Token::Type::COLON, ":"},
    {Token::Type::LCURLY, "{"},
    {Token::Type::RCURLY, "}"},
    {Token::Type::ARROW, "->"},
    {Token::Type::QUOTE, "'"},
    {Token::Type::TWOHEADARROW, "->>"}};
}  // namespace

struct Tokenizer::Maps Tokenizer::map;

Tokenizer::Maps::Maps() {
  reserved_words["and"] = Token::Type::AND;
  reserved_words["or"] = Token::Type::OR;
  reserved_words["xor"] = Token::Type::XOR;
  reserved_words["is"] = Token::Type::IS;
  reserved_words["not"] = Token::Type::NOT;
  reserved_words["like"] = Token::Type::LIKE;
  reserved_words["in"] = Token::Type::IN_;
  reserved_words["regexp"] = Token::Type::REGEXP;
  reserved_words["between"] = Token::Type::BETWEEN;
  reserved_words["overlaps"] = Token::Type::OVERLAPS;
  reserved_words["interval"] = Token::Type::INTERVAL;
  reserved_words["escape"] = Token::Type::ESCAPE;
  reserved_words["div"] = Token::Type::DIV;
  reserved_words["hex"] = Token::Type::HEX;
  reserved_words["bin"] = Token::Type::BIN;
  reserved_words["true"] = Token::Type::TRUE_;
  reserved_words["false"] = Token::Type::FALSE_;
  reserved_words["null"] = Token::Type::T_NULL;
  reserved_words["second"] = Token::Type::SECOND;
  reserved_words["minute"] = Token::Type::MINUTE;
  reserved_words["hour"] = Token::Type::HOUR;
  reserved_words["day"] = Token::Type::DAY;
  reserved_words["week"] = Token::Type::WEEK;
  reserved_words["month"] = Token::Type::MONTH;
  reserved_words["quarter"] = Token::Type::QUARTER;
  reserved_words["year"] = Token::Type::YEAR;
  reserved_words["microsecond"] = Token::Type::MICROSECOND;
  reserved_words["as"] = Token::Type::AS;
  reserved_words["asc"] = Token::Type::ASC;
  reserved_words["desc"] = Token::Type::DESC;
  reserved_words["cast"] = Token::Type::CAST;
  reserved_words["character"] = Token::Type::CHARACTER;
  reserved_words["set"] = Token::Type::SET;
  reserved_words["charset"] = Token::Type::CHARSET;
  reserved_words["ascii"] = Token::Type::ASCII;
  reserved_words["unicode"] = Token::Type::UNICODE;
  reserved_words["byte"] = Token::Type::BYTE;
  reserved_words["binary"] = Token::Type::BINARY;
  reserved_words["char"] = Token::Type::CHAR;
  reserved_words["nchar"] = Token::Type::NCHAR;
  reserved_words["date"] = Token::Type::DATE;
  reserved_words["datetime"] = Token::Type::DATETIME;
  reserved_words["time"] = Token::Type::TIME;
  reserved_words["decimal"] = Token::Type::DECIMAL;
  reserved_words["signed"] = Token::Type::SIGNED;
  reserved_words["unsigned"] = Token::Type::UNSIGNED;
  reserved_words["integer"] = Token::Type::INTEGER;
  reserved_words["int"] = Token::Type::INTEGER;
  reserved_words["json"] = Token::Type::JSON;

  interval_units.insert(Token::Type::MICROSECOND);
  interval_units.insert(Token::Type::SECOND);
  interval_units.insert(Token::Type::MINUTE);
  interval_units.insert(Token::Type::HOUR);
  interval_units.insert(Token::Type::DAY);
  interval_units.insert(Token::Type::WEEK);
  interval_units.insert(Token::Type::MONTH);
  interval_units.insert(Token::Type::QUARTER);
  interval_units.insert(Token::Type::YEAR);

  operator_names["="] = "==";
  operator_names["and"] = "&&";
  operator_names["or"] = "||";
  operator_names["not"] = "not";
  operator_names["xor"] = "xor";
  operator_names["is"] = "is";
  operator_names["between"] = "between";
  operator_names["overlaps"] = "overlaps";
  operator_names["in"] = "in";
  operator_names["like"] = "like";
  operator_names["!="] = "!=";
  operator_names["<>"] = "!=";
  operator_names[">"] = ">";
  operator_names[">="] = ">=";
  operator_names["<"] = "<";
  operator_names["<="] = "<=";
  operator_names["&"] = "&";
  operator_names["|"] = "|";
  operator_names["<<"] = "<<";
  operator_names[">>"] = ">>";
  operator_names["+"] = "+";
  operator_names["-"] = "-";
  operator_names["*"] = "*";
  operator_names["/"] = "/";
  operator_names["~"] = "~";
  operator_names["%"] = "%";

  unary_operator_names["+"] = "sign_plus";
  unary_operator_names["-"] = "sign_minus";
  unary_operator_names["~"] = "~";
  unary_operator_names["not"] = "not";
}

Token::Token(Token::Type type, const std::string &text, size_t cur_pos)
    : m_type(type), m_text(text), m_pos(cur_pos) {}

const std::string &Token::get_type_name() const { return to_string(m_type); }

struct Tokenizer::Maps map;

Tokenizer::Tokenizer(const std::string &input) : _input(input) { _pos = 0; }

bool Tokenizer::next_char_is(tokens_t::size_type i, int tok) {
  return (i + 1) < _input.size() && _input[i + 1] == tok;
}

void Tokenizer::assert_cur_token(Token::Type type) {
  assert_tok_position();
  const Token &tok = _tokens.at(_pos);
  Token::Type tok_type = tok.get_type();
  if (tok_type != type) {
    std::stringstream s;

    s << "Expected " << to_string(type) << " but found ";

    if (shcore::str_caseeq(tok.get_text(), tok.get_type_name())) {
      s << tok.get_text();
    } else {
      s << tok.get_type_name() << " (" << tok.get_text() << ")";
    }

    throw Parser_error(s.str(), tok, get_input());
  }
}

bool Tokenizer::cur_token_type_is(Token::Type type) {
  return pos_token_type_is(_pos, type);
}

bool Tokenizer::cur_token_type_is_keyword() {
  if (_pos < _tokens.size()) {
    return map.reserved_words.find(_tokens[_pos].get_text()) !=
           map.reserved_words.end();
  } else {
    return false;
  }
}

bool Tokenizer::next_token_type(Token::Type type) {
  return pos_token_type_is(_pos + 1, type);
}

bool Tokenizer::pos_token_type_is(tokens_t::size_type pos, Token::Type type) {
  return (pos < _tokens.size()) && (_tokens[pos].get_type() == type);
}

const std::string &Tokenizer::consume_token(Token::Type type) {
  assert_cur_token(type);
  const std::string &v = _tokens[_pos++].get_text();
  return v;
}

const Token &Tokenizer::peek_token() {
  assert_tok_position();
  Token &t = _tokens[_pos];
  return t;
}

void Tokenizer::unget_token() {
  if (_pos == 0)
    throw Parser_error(
        "Attempt to get back a token when already at first token (position "
        "0).");
  --_pos;
}

void Tokenizer::get_tokens() {
  bool arrow_last = false;
  bool inside_arrow = false;
  for (size_t i = 0; i < _input.size(); ++i) {
    char c = _input[i];
    if (std::isspace(c)) {
      // do nothing
      continue;
    } else if (std::isdigit(c)) {
      // numerical literal
      const int start = i;
      // floating grammar is
      // float -> int '.' (int | (int expo[sign] int))
      // int -> digit +
      // expo -> 'E' | 'e'
      // sign -> '-' | '+'
      while (i < _input.size() && std::isdigit(c = _input[i])) ++i;
      if (i < _input.size() && _input[i] == '.') {
        ++i;
        while (i < _input.size() && std::isdigit(_input[i])) ++i;
        if (i < _input.size() && std::toupper(_input[i]) == 'E') {
          ++i;
          if (i < _input.size() && (((c = _input[i]) == '-') || (c == '+')))
            ++i;
          size_t j = i;
          while (i < _input.size() && std::isdigit(_input[i])) i++;
          if (i == j)
            throw Parser_error(
                "Missing exponential value for floating point at position " +
                std::to_string(start));
        }
        _tokens.push_back(Token(Token::Type::LNUM,
                                std::string(_input, start, i - start), start));
      } else {
        _tokens.push_back(Token(Token::Type::LINTEGER,
                                std::string(_input, start, i - start), start));
      }
      if (i < _input.size()) --i;
    } else if (!std::isalpha(c) && c != '_') {
      // # non-identifier, e.g. operator or quoted literal
      if (c == '?') {
        _tokens.push_back(
            Token(Token::Type::PLACEHOLDER, std::string(1, c), i));
      } else if (c == '+') {
        _tokens.push_back(Token(Token::Type::PLUS, std::string(1, c), i));
      } else if (c == '-') {
        if (!arrow_last && next_char_is(i, '>')) {
          if (next_char_is(i + 1, '>')) {
            _tokens.push_back(Token(Token::Type::TWOHEADARROW, "->>", i));
            i += 2;
          } else {
            _tokens.push_back(Token(Token::Type::ARROW, "->", i++));
          }
          arrow_last = true;
          continue;
        } else {
          _tokens.push_back(Token(Token::Type::MINUS, std::string(1, c), i));
        }
      } else if (c == '*') {
        if (next_char_is(i, '*')) {
          _tokens.push_back(
              Token(Token::Type::DOUBLESTAR, std::string("**"), i++));
        } else {
          _tokens.push_back(Token(Token::Type::MUL, std::string(1, c), i));
        }
      } else if (c == '/') {
        _tokens.push_back(Token(Token::Type::DIV, std::string(1, c), i));
      } else if (c == '$') {
        _tokens.push_back(Token(Token::Type::DOLLAR, std::string(1, c), i));
      } else if (c == '%') {
        _tokens.push_back(Token(Token::Type::MOD, std::string(1, c), i));
      } else if (c == '=') {
        _tokens.push_back(Token(Token::Type::EQ, std::string(1, c), i));
      } else if (c == '&') {
        _tokens.push_back(Token(Token::Type::BITAND, std::string(1, c), i));
      } else if (c == '|') {
        _tokens.push_back(Token(Token::Type::BITOR, std::string(1, c), i));
      } else if (c == '(') {
        _tokens.push_back(Token(Token::Type::LPAREN, std::string(1, c), i));
      } else if (c == ')') {
        _tokens.push_back(Token(Token::Type::RPAREN, std::string(1, c), i));
      } else if (c == '[') {
        _tokens.push_back(Token(Token::Type::LSQBRACKET, std::string(1, c), i));
      } else if (c == ']') {
        _tokens.push_back(Token(Token::Type::RSQBRACKET, std::string(1, c), i));
      } else if (c == '{') {
        _tokens.push_back(Token(Token::Type::LCURLY, std::string(1, c), i));
      } else if (c == '}') {
        _tokens.push_back(Token(Token::Type::RCURLY, std::string(1, c), i));
      } else if (c == '~') {
        _tokens.push_back(Token(Token::Type::NEG, std::string(1, c), i));
      } else if (c == ',') {
        _tokens.push_back(Token(Token::Type::COMMA, std::string(1, c), i));
      } else if (c == ':') {
        _tokens.push_back(Token(Token::Type::COLON, std::string(1, c), i));
      } else if (c == '!') {
        if (next_char_is(i, '=')) {
          _tokens.push_back(Token(Token::Type::NE, std::string("!="), i++));
        } else {
          _tokens.push_back(Token(Token::Type::BANG, std::string(1, c), i));
        }
      } else if (c == '<') {
        if (next_char_is(i, '<')) {
          _tokens.push_back(Token(Token::Type::LSHIFT, std::string("<<"), i++));
        } else if (next_char_is(i, '=')) {
          _tokens.push_back(Token(Token::Type::LE, std::string("<="), i++));
        } else if (next_char_is(i, '>')) {
          _tokens.push_back(Token(Token::Type::NE, std::string("!="), i++));
        } else {
          _tokens.push_back(Token(Token::Type::LT, std::string("<"), i));
        }
      } else if (c == '>') {
        if (next_char_is(i, '>')) {
          _tokens.push_back(Token(Token::Type::RSHIFT, std::string(">>"), i++));
        } else if (next_char_is(i, '=')) {
          _tokens.push_back(Token(Token::Type::GE, std::string(">="), i++));
        } else {
          _tokens.push_back(Token(Token::Type::GT, std::string(1, c), i));
        }
      } else if (c == '.') {
        if ((i + 1) < _input.size() && std::isdigit(_input[i + 1])) {
          const size_t start = i;
          ++i;
          // floating grammar is
          // float -> '.' (int | (int expo[sign] int))
          // nint->digit +
          // expo -> 'E' | 'e'
          // sign -> '-' | '+'
          while (i < _input.size() && std::isdigit(_input[i])) ++i;
          if (i < _input.size() && std::toupper(_input[i]) == 'E') {
            ++i;
            if (i < _input.size() && (((c = _input[i]) == '+') || (c == '-')))
              ++i;
            size_t j = i;
            while (i < _input.size() && std::isdigit(_input[i])) ++i;
            if (i == j)
              throw Parser_error(
                  "Missing exponential value for floating point at position " +
                  std::to_string(start));
          }
          _tokens.push_back(Token(
              Token::Type::LNUM, std::string(_input, start, i - start), start));
          if (i < _input.size()) --i;
        } else {
          _tokens.push_back(Token(Token::Type::DOT, std::string(1, c), i));
        }
      } else if (c == '\'' && arrow_last) {
        _tokens.push_back(Token(Token::Type::QUOTE, "'", i));
        if (!inside_arrow)
          inside_arrow = true;
        else {
          arrow_last = false;
          inside_arrow = false;
        }
      } else if (c == '"' || c == '\'' || c == '`') {
        char quote_char = c;
        std::string val;
        size_t start = ++i;

        while (i < _input.size()) {
          c = _input[i];
          if ((c == quote_char) && ((i + 1) < _input.size()) &&
              (_input[i + 1] != quote_char)) {
            // break if we have a quote char that's not double
            break;
          } else if ((c == quote_char) || (c == '\\' && quote_char != '`')) {
            // && quote_char != '`'
            // this quote char has to be doubled
            if ((i + 1) >= _input.size()) break;
            val.append(1, _input[++i]);
          } else {
            val.append(1, c);
          }
          ++i;
        }
        if ((i >= _input.size()) && (_input[i] != quote_char)) {
          throw Parser_error(
              "Unterminated quoted string starting at position " +
              std::to_string(start));
        }
        if (quote_char == '`') {
          _tokens.push_back(Token(Token::Type::IDENT, val, start));
        } else {
          _tokens.push_back(Token(Token::Type::LSTRING, val, start));
        }
      } else {
        throw Parser_error("Unknown character at position " +
                           std::to_string(i));
      }
    } else {
      size_t start = i;
      while (i < _input.size() && (std::isalnum(_input[i]) || _input[i] == '_'))
        ++i;
      std::string val(_input, start, i - start);
      Maps::reserved_words_t::const_iterator it = map.reserved_words.find(val);
      if (it != map.reserved_words.end()) {
        _tokens.push_back(Token(it->second, val, start));
      } else {
        _tokens.push_back(Token(Token::Type::IDENT, val, start));
      }
      --i;
    }
  }
}

void Tokenizer::inc_pos_token() { ++_pos; }

const Token &Tokenizer::consume_any_token() {
  assert_tok_position();
  Token &tok = _tokens[_pos];
  ++_pos;
  return tok;
}

void Tokenizer::assert_tok_position() {
  if (_pos >= _tokens.size())
    throw Parser_error("Unexpected end of expression.");
}

bool Tokenizer::tokens_available() const { return _pos < _tokens.size(); }

bool Tokenizer::is_interval_units_type() {
  assert_tok_position();
  Token::Type type = _tokens[_pos].get_type();
  return map.interval_units.find(type) != map.interval_units.end();
}

bool Tokenizer::is_type_within_set(const std::set<Token::Type> &types) {
  assert_tok_position();
  Token::Type type = _tokens[_pos].get_type();
  return types.find(type) != types.end();
}

Parser_error::Parser_error(const std::string &msg, const Token &token,
                           const std::string &line)
    : std::runtime_error(msg + format(token, line)) {}

std::string Parser_error::format(const Token &token, const std::string &line) {
  std::stringstream s;

  s << ", at position " << token.get_pos();

  if (!line.empty()) {
    s << ",\nin: " << line << "\n"
      << std::string(4 + token.get_pos(), ' ')
      << std::string(token.get_length(), '^')
      << std::string(line.length() - token.get_pos() - token.get_length(), ' ');
  }

  return s.str();
}

bool Tokenizer::Cmp_icase::operator()(const std::string &lhs,
                                      const std::string &rhs) const {
  const char *c_lhs = lhs.c_str();
  const char *c_rhs = rhs.c_str();

  return shcore::str_casecmp(c_lhs, c_rhs) < 0;
}

const std::string &to_string(const Token::Type &type) {
  const auto name = k_token_name.find(type);
  if (name == k_token_name.end()) {
    throw std::runtime_error("Could not find token type: " +
                             std::to_string(static_cast<int>(type)));
  } else {
    return name->second;
  }
}

}  // namespace mysqlx
