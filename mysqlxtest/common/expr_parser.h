/*
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifndef _EXPR_PARSER_H_
#define _EXPR_PARSER_H_

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <stdexcept>

#include <boost/function.hpp>

#include "mysqlx_datatypes.pb.h"
#include "mysqlx_expr.pb.h"
#include "mysqlx_crud.pb.h"

namespace mysqlx
{
  class Token
  {
  public:
    enum TokenType
    {
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
      AT = 23,
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
      AS = 56
    };

    Token(TokenType type, const std::string& text);
    // TODO: it is better if this one returns a pointer (std::string*)
    const std::string& get_text() const;
    TokenType get_type() const;

  private:
    TokenType _type;
    std::string _text;
  };

  class Expr_builder
  {
  public:
    static Mysqlx::Datatypes::Any* build_null_any();
    static Mysqlx::Datatypes::Any* build_double_any(double d);
    static Mysqlx::Datatypes::Any* build_int_any(google::protobuf::int64 i);
    static Mysqlx::Datatypes::Any* build_string_any(const std::string& s);
    static Mysqlx::Datatypes::Any* build_bool_any(bool b);
    static Mysqlx::Expr::Expr* build_literal_expr(Mysqlx::Datatypes::Any* a);
    static Mysqlx::Expr::Expr* build_unary_op(const std::string& name, Mysqlx::Expr::Expr* param);
  };

  class Tokenizer
  {
  public:
    Tokenizer(const std::string& input);

    typedef std::vector<Token> tokens_t;

    bool next_char_is(tokens_t::size_type i, int tok);
    void assert_cur_token(Token::TokenType type);
    bool cur_token_type_is(Token::TokenType type);
    bool next_token_type(Token::TokenType type);
    bool pos_token_type_is(tokens_t::size_type pos, Token::TokenType type);
    const std::string& consume_token(Token::TokenType type);
    const Token& peek_token();
    void unget_token();
    void inc_pos_token();
    int get_token_pos();
    const Token& consume_any_token();
    void assert_tok_position();
    bool tokens_available();
    bool is_interval_units_type();
    bool is_type_within_set(const std::set<Token::TokenType>& types);

    std::vector<Token>::const_iterator begin() const { return _tokens.begin(); }
    std::vector<Token>::const_iterator end() const { return _tokens.end(); }

    void get_tokens();

  protected:
    std::vector<Token> _tokens;
    std::string _input;
    tokens_t::size_type _pos;

  public:

    struct cmp_icase
    {
      bool operator()(const std::string& lhs, const std::string& rhs) const;
    };

    struct maps
    {
    public:
      typedef std::map<std::string, Token::TokenType, cmp_icase> reserved_words_t;
      reserved_words_t reserved_words;
      std::set<Token::TokenType> interval_units;
      std::map<std::string, std::string, cmp_icase> operator_names;
      std::map<std::string, std::string, cmp_icase> unary_operator_names;

      maps()
      {
        reserved_words["and"] = Token::AND;
        reserved_words["or"] = Token::OR;
        reserved_words["xor"] = Token::XOR;
        reserved_words["is"] = Token::IS;
        reserved_words["not"] = Token::NOT;
        reserved_words["like"] = Token::LIKE;
        reserved_words["in"] = Token::IN_;
        reserved_words["regexp"] = Token::REGEXP;
        reserved_words["between"] = Token::BETWEEN;
        reserved_words["interval"] = Token::INTERVAL;
        reserved_words["escape"] = Token::ESCAPE;
        reserved_words["div"] = Token::DIV;
        reserved_words["hex"] = Token::HEX;
        reserved_words["bin"] = Token::BIN;
        reserved_words["true"] = Token::TRUE_;
        reserved_words["false"] = Token::FALSE_;
        reserved_words["null"] = Token::T_NULL;
        reserved_words["second"] = Token::SECOND;
        reserved_words["minute"] = Token::MINUTE;
        reserved_words["hour"] = Token::HOUR;
        reserved_words["day"] = Token::DAY;
        reserved_words["week"] = Token::WEEK;
        reserved_words["month"] = Token::MONTH;
        reserved_words["quarter"] = Token::QUARTER;
        reserved_words["year"] = Token::YEAR;
        reserved_words["microsecond"] = Token::MICROSECOND;

        interval_units.insert(Token::MICROSECOND);
        interval_units.insert(Token::SECOND);
        interval_units.insert(Token::MINUTE);
        interval_units.insert(Token::HOUR);
        interval_units.insert(Token::DAY);
        interval_units.insert(Token::WEEK);
        interval_units.insert(Token::MONTH);
        interval_units.insert(Token::QUARTER);
        interval_units.insert(Token::YEAR);

        operator_names["="] = "==";
        operator_names["and"] = "&&";
        operator_names["or"] = "||";
        operator_names["not"] = "not";
        operator_names["xor"] = "xor";
        operator_names["is"] = "is";
        operator_names["between"] = "between";
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
    };

  public:
    struct maps map;
  };

  class Expr_parser
  {
  public:
    Expr_parser(const std::string& expr_str, bool document_mode = false);

    typedef boost::function<std::auto_ptr<Mysqlx::Expr::Expr>(Expr_parser*)> inner_parser_t;

    void paren_expr_list(::google::protobuf::RepeatedPtrField< ::Mysqlx::Expr::Expr >* expr_list);
    std::auto_ptr<Mysqlx::Expr::Identifier> identifier();
    std::auto_ptr<Mysqlx::Expr::Expr> function_call();
    void docpath_member(Mysqlx::Expr::DocumentPathItem& item);
    void docpath_array_loc(Mysqlx::Expr::DocumentPathItem& item);
    void document_path(Mysqlx::Expr::ColumnIdentifier& colid);
    std::auto_ptr<Mysqlx::Expr::Expr> column_identifier();
    std::auto_ptr<Mysqlx::Expr::Expr> atomic_expr();
    std::auto_ptr<Mysqlx::Expr::Expr> parse_left_assoc_binary_op_expr(std::set<Token::TokenType>& types, inner_parser_t inner_parser);
    std::auto_ptr<Mysqlx::Expr::Expr> mul_div_expr();
    std::auto_ptr<Mysqlx::Expr::Expr> add_sub_expr();
    std::auto_ptr<Mysqlx::Expr::Expr> shift_expr();
    std::auto_ptr<Mysqlx::Expr::Expr> bit_expr();
    std::auto_ptr<Mysqlx::Expr::Expr> comp_expr();
    std::auto_ptr<Mysqlx::Expr::Expr> ilri_expr();
    std::auto_ptr<Mysqlx::Expr::Expr> and_expr();
    std::auto_ptr<Mysqlx::Expr::Expr> or_expr();
    std::auto_ptr<Mysqlx::Expr::Expr> expr();

    std::vector<Token>::const_iterator begin() const { return _tokenizer.begin(); }
    std::vector<Token>::const_iterator end() const { return _tokenizer.end(); }

  private:
    Tokenizer _tokenizer;
    bool _document_mode;
  };

  class Expr_unparser
  {
  public:
    static std::string any_to_string(const Mysqlx::Datatypes::Any& a);
    static std::string escape_literal(const std::string& s);
    static std::string scalar_to_string(const Mysqlx::Datatypes::Scalar& s);
    static std::string document_path_to_string(const ::google::protobuf::RepeatedPtrField< ::Mysqlx::Expr::DocumentPathItem >& dp);
    static std::string column_identifier_to_string(const Mysqlx::Expr::ColumnIdentifier& colid);
    static std::string function_call_to_string(const Mysqlx::Expr::FunctionCall& fc);
    static std::string operator_to_string(const Mysqlx::Expr::Operator& op);
    static std::string quote_identifier(const std::string& id);
    static std::string expr_to_string(const Mysqlx::Expr::Expr& e);
    static std::string column_to_string(const Mysqlx::Crud::Column& c);
    static std::string column_list_to_string(std::vector<Mysqlx::Crud::Column*>& columns);

    static void replace(std::string& target, const std::string& old_val, const std::string& new_val);
  };

  class Parser_error : public std::runtime_error
  {
  public:
    Parser_error(const std::string& msg) : std::runtime_error(msg)
    {
    }
  };
};

#endif
