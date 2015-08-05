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

// Avoid warnings from includes of other project and protobuf
#if defined __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#elif defined _MSC_VER
#pragma warning (push)
#pragma warning (disable : 4018 4996)
#endif

#include "mysqlx_datatypes.pb.h"
#include "mysqlx_expr.pb.h"
#include "mysqlx_crud.pb.h"

#ifdef __GNUC__
#pragma GCC diagnostic pop
#elif defined _MSC_VER
#pragma warning (pop)
#endif

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
      AS = 56,
      ASC = 57,
      DESC = 58
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
    std::string get_input() { return _input; }

  protected:
    std::vector<Token> _tokens;
    std::string _input;
    tokens_t::size_type _pos;

  public:

    struct Cmp_icase
    {
      bool operator()(const std::string& lhs, const std::string& rhs) const;
    };

    struct Maps
    {
    public:
      typedef std::map<std::string, Token::TokenType, Cmp_icase> reserved_words_t;
      reserved_words_t reserved_words;
      std::set<Token::TokenType> interval_units;
      std::map<std::string, std::string, Cmp_icase> operator_names;
      std::map<std::string, std::string, Cmp_icase> unary_operator_names;

      Maps();
    };

  public:
    static Maps map;
  };

  class Expr_parser
  {
  public:
    Expr_parser(const std::string& expr_str, bool document_mode = false);

    typedef boost::function<Mysqlx::Expr::Expr*(Expr_parser*)> inner_parser_t;

    void paren_expr_list(::google::protobuf::RepeatedPtrField< ::Mysqlx::Expr::Expr >* expr_list);
    Mysqlx::Expr::Identifier* identifier();
    Mysqlx::Expr::Expr* function_call();
    void docpath_member(Mysqlx::Expr::DocumentPathItem& item);
    void docpath_array_loc(Mysqlx::Expr::DocumentPathItem& item);
    void document_path(Mysqlx::Expr::ColumnIdentifier& colid);
    const std::string& id();
    Mysqlx::Expr::Expr* column_identifier();
    Mysqlx::Expr::Expr* atomic_expr();
    Mysqlx::Expr::Expr* parse_left_assoc_binary_op_expr(std::set<Token::TokenType>& types, inner_parser_t inner_parser);
    Mysqlx::Expr::Expr* mul_div_expr();
    Mysqlx::Expr::Expr* add_sub_expr();
    Mysqlx::Expr::Expr* shift_expr();
    Mysqlx::Expr::Expr* bit_expr();
    Mysqlx::Expr::Expr* comp_expr();
    Mysqlx::Expr::Expr* ilri_expr();
    Mysqlx::Expr::Expr* and_expr();
    Mysqlx::Expr::Expr* or_expr();
    Mysqlx::Expr::Expr* expr();

    std::vector<Token>::const_iterator begin() const { return _tokenizer.begin(); }
    std::vector<Token>::const_iterator end() const { return _tokenizer.end(); }

  protected:
    Mysqlx::Expr::Expr* my_expr();
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
    static std::string column_to_string(const Mysqlx::Crud::Projection& c);
    static std::string order_to_string(const Mysqlx::Crud::Order& c);
    static std::string column_list_to_string(google::protobuf::RepeatedPtrField< ::Mysqlx::Crud::Projection > columns);    
    static std::string order_list_to_string(google::protobuf::RepeatedPtrField< ::Mysqlx::Crud::Order> columns);
    

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
