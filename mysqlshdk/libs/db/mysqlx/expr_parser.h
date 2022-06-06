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

#ifndef _EXPR_PARSER_H_
#define _EXPR_PARSER_H_

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "db/mysqlx/mysqlxclient_clean.h"
#include "tokenizer.h"

namespace mysqlx {
class Expr_parser {
 public:
  Expr_parser(const std::string &expr_str, bool document_mode = false,
              bool allow_alias = false,
              std::vector<std::string> *place_holders = NULL);

  typedef std::function<std::unique_ptr<Mysqlx::Expr::Expr>(void)>
      inner_parser_t;

  void paren_expr_list(
      ::google::protobuf::RepeatedPtrField<::Mysqlx::Expr::Expr> *expr_list);
  std::unique_ptr<Mysqlx::Expr::Identifier> identifier();
  std::unique_ptr<Mysqlx::Expr::Expr> function_call();
  void docpath_member(Mysqlx::Expr::DocumentPathItem &item);
  void docpath_array_loc(Mysqlx::Expr::DocumentPathItem &item);
  void document_path(Mysqlx::Expr::ColumnIdentifier &colid);
  const std::string &id();
  std::unique_ptr<Mysqlx::Expr::Expr> column_field();
  std::unique_ptr<Mysqlx::Expr::Expr> document_field();
  std::unique_ptr<Mysqlx::Expr::Expr> atomic_expr();
  std::unique_ptr<Mysqlx::Expr::Expr> array_();
  std::unique_ptr<Mysqlx::Expr::Expr> parse_left_assoc_binary_op_expr(
      std::set<Token::Type> &types, inner_parser_t inner_parser);
  std::unique_ptr<Mysqlx::Expr::Expr> mul_div_expr();
  std::unique_ptr<Mysqlx::Expr::Expr> add_sub_expr();
  std::unique_ptr<Mysqlx::Expr::Expr> shift_expr();
  std::unique_ptr<Mysqlx::Expr::Expr> bit_expr();
  std::unique_ptr<Mysqlx::Expr::Expr> comp_expr();
  std::unique_ptr<Mysqlx::Expr::Expr> ilri_expr();
  std::unique_ptr<Mysqlx::Expr::Expr> and_expr();
  std::unique_ptr<Mysqlx::Expr::Expr> or_expr();
  std::unique_ptr<Mysqlx::Expr::Expr> expr();

  std::vector<Token>::const_iterator begin() const {
    return _tokenizer.begin();
  }
  std::vector<Token>::const_iterator end() const { return _tokenizer.end(); }

  bool tokens_available() const { return _tokenizer.tokens_available(); }

 protected:
  struct operator_list {
    std::set<Token::Type> mul_div_expr_types;
    std::set<Token::Type> add_sub_expr_types;
    std::set<Token::Type> shift_expr_types;
    std::set<Token::Type> bit_expr_types;
    std::set<Token::Type> comp_expr_types;
    std::set<Token::Type> and_expr_types;
    std::set<Token::Type> or_expr_types;

    operator_list() {
      mul_div_expr_types.insert(Token::Type::MUL);
      mul_div_expr_types.insert(Token::Type::DIV);
      mul_div_expr_types.insert(Token::Type::MOD);

      add_sub_expr_types.insert(Token::Type::PLUS);
      add_sub_expr_types.insert(Token::Type::MINUS);

      shift_expr_types.insert(Token::Type::LSHIFT);
      shift_expr_types.insert(Token::Type::RSHIFT);

      bit_expr_types.insert(Token::Type::BITAND);
      bit_expr_types.insert(Token::Type::BITOR);
      bit_expr_types.insert(Token::Type::BITXOR);

      comp_expr_types.insert(Token::Type::GE);
      comp_expr_types.insert(Token::Type::GT);
      comp_expr_types.insert(Token::Type::LE);
      comp_expr_types.insert(Token::Type::LT);
      comp_expr_types.insert(Token::Type::EQ);
      comp_expr_types.insert(Token::Type::NE);

      and_expr_types.insert(Token::Type::AND);

      or_expr_types.insert(Token::Type::OR);
    }
  };

  static operator_list _ops;

  // json
  void json_key_value(Mysqlx::Expr::Object *obj);
  std::unique_ptr<Mysqlx::Expr::Expr> json_doc();
  // placeholder
  std::vector<std::string> _place_holders;
  std::vector<std::string> *_place_holder_ref;
  std::unique_ptr<Mysqlx::Expr::Expr> placeholder();
  // cast
  std::unique_ptr<Mysqlx::Expr::Expr> my_expr();
  std::unique_ptr<Mysqlx::Expr::Expr> cast();
  std::unique_ptr<Mysqlx::Expr::Expr> binary();
  std::string cast_data_type();
  std::string cast_data_type_dimension(bool double_dimension = false);
  std::string opt_binary();
  std::string charset_def();
  Tokenizer _tokenizer;
  bool _document_mode;
  bool _allow_alias;
};

class Expr_unparser {
 public:
  static std::string any_to_string(const Mysqlx::Datatypes::Any &a);
  static std::string escape_literal(const std::string &s);
  static std::string scalar_to_string(const Mysqlx::Datatypes::Scalar &s);
  static std::string data_type_to_string(const Mysqlx::Datatypes::Scalar &s);
  static std::string document_path_to_string(
      const ::google::protobuf::RepeatedPtrField<
          ::Mysqlx::Expr::DocumentPathItem> &dp);
  static std::string column_identifier_to_string(
      const Mysqlx::Expr::ColumnIdentifier &colid);
  static std::string function_call_to_string(
      const Mysqlx::Expr::FunctionCall &fc);
  static std::string operator_to_string(const Mysqlx::Expr::Operator &op);
  static std::string quote_identifier(const std::string &id);
  static std::string expr_to_string(const Mysqlx::Expr::Expr &e);
  static std::string object_to_string(const Mysqlx::Expr::Object &o);
  static std::string placeholder_to_string(const Mysqlx::Expr::Expr &e);
  static std::string column_to_string(const Mysqlx::Crud::Projection &c);
  static std::string order_to_string(const Mysqlx::Crud::Order &c);
  static std::string array_to_string(const Mysqlx::Expr::Expr &e);
  static std::string column_list_to_string(
      google::protobuf::RepeatedPtrField<::Mysqlx::Crud::Projection> columns);
  static std::string order_list_to_string(
      google::protobuf::RepeatedPtrField<::Mysqlx::Crud::Order> columns);

  static void replace(std::string &target, const std::string &old_val,
                      const std::string &new_val);
};
}  // namespace mysqlx

#endif
