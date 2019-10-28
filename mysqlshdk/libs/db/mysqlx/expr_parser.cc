/*
 * Copyright (c) 2015, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/libs/db/mysqlx/expr_parser.h"

#ifndef WIN32
#include <strings.h>
#define _stricmp strcasecmp
#endif

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <utility>

#include "mysqlshdk/libs/db/mysqlx/tokenizer.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlx {

struct Expr_parser::operator_list Expr_parser::_ops;

Mysqlx::Datatypes::Scalar *build_null_scalar() {
  Mysqlx::Datatypes::Scalar *sc = new Mysqlx::Datatypes::Scalar;
  sc->set_type(Mysqlx::Datatypes::Scalar::V_NULL);
  return sc;
}

Mysqlx::Datatypes::Scalar *build_double_scalar(double d) {
  Mysqlx::Datatypes::Scalar *sc = new Mysqlx::Datatypes::Scalar;
  sc->set_type(Mysqlx::Datatypes::Scalar::V_DOUBLE);
  sc->set_v_double(d);
  return sc;
}

Mysqlx::Datatypes::Scalar *build_int_scalar(google::protobuf::int64 i) {
  Mysqlx::Datatypes::Scalar *sc = new Mysqlx::Datatypes::Scalar;
  if (i < 0) {
    sc->set_type(Mysqlx::Datatypes::Scalar::V_SINT);
    sc->set_v_signed_int(i);
  } else {
    sc->set_type(Mysqlx::Datatypes::Scalar::V_UINT);
    sc->set_v_unsigned_int((google::protobuf::uint64)i);
  }
  return sc;
}

Mysqlx::Datatypes::Scalar *build_string_scalar(const std::string &s) {
  Mysqlx::Datatypes::Scalar *sc = new Mysqlx::Datatypes::Scalar;
  sc->set_type(Mysqlx::Datatypes::Scalar::V_OCTETS);
  sc->mutable_v_octets()->set_value(s);
  return sc;
}

Mysqlx::Datatypes::Scalar *build_bool_scalar(bool b) {
  Mysqlx::Datatypes::Scalar *sc = new Mysqlx::Datatypes::Scalar;
  sc->set_type(Mysqlx::Datatypes::Scalar::V_BOOL);
  sc->set_v_bool(b);
  return sc;
}

Mysqlx::Expr::Expr *build_literal_expr(Mysqlx::Datatypes::Scalar *sc) {
  Mysqlx::Expr::Expr *e = new Mysqlx::Expr::Expr();
  e->set_type(Mysqlx::Expr::Expr::LITERAL);
  e->set_allocated_literal(sc);
  return e;
}

Mysqlx::Expr::Expr *build_unary_op(const std::string &name,
                                   std::unique_ptr<Mysqlx::Expr::Expr> param) {
  Mysqlx::Expr::Expr *e(new Mysqlx::Expr::Expr());
  e->set_type(Mysqlx::Expr::Expr::OPERATOR);
  Mysqlx::Expr::Operator *op = e->mutable_operator_();
  op->mutable_param()->AddAllocated(param.release());
  std::string name_normalized;
  name_normalized.assign(name);
  std::transform(name_normalized.begin(), name_normalized.end(),
                 name_normalized.begin(), ::tolower);
  op->set_name(name_normalized.c_str(), name_normalized.size());
  return e;
}

Expr_parser::Expr_parser(const std::string &expr_str, bool document_mode,
                         bool allow_alias,
                         std::vector<std::string> *place_holders)
    : _tokenizer(expr_str),
      _document_mode(document_mode),
      _allow_alias(allow_alias) {
  // If provided uses external placeholder information, if not uses the internal
  if (place_holders)
    _place_holder_ref = place_holders;
  else
    _place_holder_ref = &_place_holders;

  _tokenizer.get_tokens();
}

/*
 * paren_expr_list ::= LPAREN expr ( COMMA expr )* RPAREN
 */
void Expr_parser::paren_expr_list(
    ::google::protobuf::RepeatedPtrField<::Mysqlx::Expr::Expr> *expr_list) {
  // Parse a paren-bounded expression list for function arguments or IN list and
  // return a list of Expr objects
  _tokenizer.consume_token(Token::Type::LPAREN);
  if (!_tokenizer.cur_token_type_is(Token::Type::RPAREN)) {
    expr_list->AddAllocated(my_expr().release());
    while (_tokenizer.cur_token_type_is(Token::Type::COMMA)) {
      _tokenizer.inc_pos_token();
      expr_list->AddAllocated(my_expr().release());
    }
  }
  _tokenizer.consume_token(Token::Type::RPAREN);
}

/*
 * identifier ::= IDENT [ DOT IDENT ]
 */
std::unique_ptr<Mysqlx::Expr::Identifier> Expr_parser::identifier() {
  bool is_keyword = _tokenizer.cur_token_type_is_keyword();

  if (!is_keyword) _tokenizer.assert_cur_token(Token::Type::IDENT);

  auto id = std::make_unique<Mysqlx::Expr::Identifier>();
  if (_tokenizer.next_token_type(Token::Type::DOT)) {
    const std::string &schema_name =
        _tokenizer.consume_token(Token::Type::IDENT);
    id->set_schema_name(schema_name.c_str(), schema_name.size());
    _tokenizer.consume_token(Token::Type::DOT);
  }
  std::string name;
  if (is_keyword) {
    const auto &token = _tokenizer.consume_any_token();
    name = token.get_text();
    id->set_name(name.c_str(), name.size());
  } else {
    name = _tokenizer.consume_token(Token::Type::IDENT);
  }
  id->set_name(name.c_str(), name.size());
  return id;
}

/*
 * function_call ::= IDENT paren_expr_list
 */
std::unique_ptr<Mysqlx::Expr::Expr> Expr_parser::function_call() {
  auto e = std::make_unique<Mysqlx::Expr::Expr>();
  e->set_type(Mysqlx::Expr::Expr::FUNC_CALL);
  Mysqlx::Expr::FunctionCall *func = e->mutable_function_call();
  func->set_allocated_name(identifier().release());

  paren_expr_list(func->mutable_param());
  return e;
}

/*
 * docpath_member ::= DOT ( IDENT | LSTRING | MUL )
 */
void Expr_parser::docpath_member(Mysqlx::Expr::DocumentPathItem &item) {
  _tokenizer.consume_token(Token::Type::DOT);
  item.set_type(Mysqlx::Expr::DocumentPathItem::MEMBER);
  if (_tokenizer.cur_token_type_is(Token::Type::IDENT)) {
    const std::string &ident = _tokenizer.consume_token(Token::Type::IDENT);
    item.set_value(ident.c_str(), ident.size());
  } else if (_tokenizer.cur_token_type_is_keyword()) {
    const auto &token = _tokenizer.consume_any_token();
    auto ident = token.get_text();
    item.set_value(ident.c_str(), ident.size());
  } else if (_tokenizer.cur_token_type_is(Token::Type::LSTRING)) {
    const std::string &lstring = _tokenizer.consume_token(Token::Type::LSTRING);
    item.set_value(lstring.c_str(), lstring.size());
  } else if (_tokenizer.cur_token_type_is(Token::Type::MUL)) {
    const std::string &mul = _tokenizer.consume_token(Token::Type::MUL);
    item.set_value(mul.c_str(), mul.size());
    item.set_type(Mysqlx::Expr::DocumentPathItem::MEMBER_ASTERISK);
  } else {
    const Token &tok = _tokenizer.peek_token();
    throw Parser_error("Expected " + to_string(Token::Type::IDENT) + " or " +
                           to_string(Token::Type::LSTRING) + " in JSON path",
                       tok, _tokenizer.get_input());
  }
}

/*
 * docpath_array_loc ::= LSQBRACKET ( MUL | LINTEGER ) RSQBRACKET
 */
void Expr_parser::docpath_array_loc(Mysqlx::Expr::DocumentPathItem &item) {
  _tokenizer.consume_token(Token::Type::LSQBRACKET);
  const Token &tok = _tokenizer.peek_token();
  if (_tokenizer.cur_token_type_is(Token::Type::MUL)) {
    _tokenizer.consume_token(Token::Type::MUL);
    _tokenizer.consume_token(Token::Type::RSQBRACKET);
    item.set_type(Mysqlx::Expr::DocumentPathItem::ARRAY_INDEX_ASTERISK);
  } else if (_tokenizer.cur_token_type_is(Token::Type::LINTEGER)) {
    const std::string &value = _tokenizer.consume_token(Token::Type::LINTEGER);
    int v = std::stoi(value);
    if (v < 0) {
      throw Parser_error("Array index cannot be negative", tok,
                         _tokenizer.get_input());
    }
    _tokenizer.consume_token(Token::Type::RSQBRACKET);
    item.set_type(Mysqlx::Expr::DocumentPathItem::ARRAY_INDEX);
    item.set_index(v);
  } else {
    throw Parser_error("Expected " + to_string(Token::Type::MUL) + " or " +
                           to_string(Token::Type::LINTEGER) +
                           " in JSON path array index",
                       tok, _tokenizer.get_input());
  }
}

/*
 * document_path ::= ( docpath_member | docpath_array_loc | ( DOUBLESTAR ))+
 */
void Expr_parser::document_path(Mysqlx::Expr::ColumnIdentifier &colid) {
  // Parse a JSON-style document path, like WL#7909, prefixing with $
  while (true) {
    if (_tokenizer.cur_token_type_is(Token::Type::DOT)) {
      docpath_member(*colid.mutable_document_path()->Add());
    } else if (_tokenizer.cur_token_type_is(Token::Type::LSQBRACKET)) {
      docpath_array_loc(*colid.mutable_document_path()->Add());
    } else if (_tokenizer.cur_token_type_is(Token::Type::DOUBLESTAR)) {
      _tokenizer.consume_token(Token::Type::DOUBLESTAR);
      Mysqlx::Expr::DocumentPathItem *item =
          colid.mutable_document_path()->Add();
      item->set_type(Mysqlx::Expr::DocumentPathItem::DOUBLE_ASTERISK);
    } else {
      break;
    }
  }
  int size = colid.document_path_size();
  if (size > 0 && (colid.document_path(size - 1).type() ==
                   Mysqlx::Expr::DocumentPathItem::DOUBLE_ASTERISK)) {
    const Token &tok = _tokenizer.peek_token();
    throw Parser_error("JSON path may not end in '**'", tok,
                       _tokenizer.get_input());
  }
}

/*
 * id ::= IDENT | MUL
 */
const std::string &Expr_parser::id() {
  if (_tokenizer.cur_token_type_is(Token::Type::IDENT)) {
    return _tokenizer.consume_token(Token::Type::IDENT);
  } else if (_tokenizer.cur_token_type_is_keyword()) {
    const auto &token = _tokenizer.consume_any_token();
    return token.get_text();
  } else {
    return _tokenizer.consume_token(Token::Type::MUL);
  }
}

/*
 * column_field ::= [ id DOT ][ id DOT ] id [ (ARROW | TWOHEADARROW) QUOTE
 * DOLLAR docpath QUOTE ]
 */
std::unique_ptr<Mysqlx::Expr::Expr> Expr_parser::column_field() {
  auto e = std::make_unique<Mysqlx::Expr::Expr>();
  std::vector<std::string> parts;
  const std::string &part = id();

  if (part == "*") {
    e->set_type(Mysqlx::Expr::Expr::OPERATOR);
    e->mutable_operator_()->set_name("*");
    return e;
  }

  parts.push_back(part);

  while (_tokenizer.cur_token_type_is(Token::Type::DOT)) {
    _tokenizer.consume_token(Token::Type::DOT);
    parts.push_back(id());
  }
  if (parts.size() > 3) {
    // reposition back to the first extra dot
    for (size_t cnt = 0, end = parts.size() - 3; cnt < end; ++cnt) {
      _tokenizer.unget_token();  // id
      _tokenizer.unget_token();  // .
    }
    const Token &tok = _tokenizer.peek_token();
    throw Parser_error("Too many parts to identifier", tok,
                       _tokenizer.get_input());
  }
  Mysqlx::Expr::ColumnIdentifier *colid = e->mutable_identifier();
  std::vector<std::string>::reverse_iterator myend = parts.rend();
  int i = 0;
  for (std::vector<std::string>::reverse_iterator it = parts.rbegin();
       it != myend; ++it, ++i) {
    std::string &s = *it;
    if (i == 0)
      colid->set_name(s.c_str(), s.size());
    else if (i == 1)
      colid->set_table_name(s.c_str(), s.size());
    else if (i == 2)
      colid->set_schema_name(s.c_str(), s.size());
  }

  // (Arrow | TwoHeadArrow) & docpath
  const bool is_twoheadarrow_token =
      _tokenizer.cur_token_type_is(Token::Type::TWOHEADARROW);
  if (is_twoheadarrow_token ||
      _tokenizer.cur_token_type_is(Token::Type::ARROW)) {
    _tokenizer.consume_any_token();
    _tokenizer.consume_token(Token::Type::QUOTE);
    _tokenizer.consume_token(Token::Type::DOLLAR);
    document_path(*colid);
    _tokenizer.consume_token(Token::Type::QUOTE);
  }

  e->set_type(Mysqlx::Expr::Expr::IDENT);

  if (is_twoheadarrow_token) {
    // wrap with json_unquote function_call
    auto func_unquote = std::make_unique<Mysqlx::Expr::Expr>();
    func_unquote->set_type(Mysqlx::Expr::Expr::FUNC_CALL);
    Mysqlx::Expr::FunctionCall *func = func_unquote->mutable_function_call();
    auto id = std::make_unique<Mysqlx::Expr::Identifier>();
    id->set_name(std::string("JSON_UNQUOTE"));
    func->set_allocated_name(id.release());
    ::google::protobuf::RepeatedPtrField<::Mysqlx::Expr::Expr> *params =
        func->mutable_param();
    params->AddAllocated(e.release());

    return func_unquote;
  }

  return e;
}

/*
 * document_field ::= [ DOLLAR ] IDENT document_path
 */
std::unique_ptr<Mysqlx::Expr::Expr> Expr_parser::document_field() {
  auto e = std::make_unique<Mysqlx::Expr::Expr>();

  if (_tokenizer.cur_token_type_is(Token::Type::DOLLAR))
    _tokenizer.consume_token(Token::Type::DOLLAR);
  Mysqlx::Expr::ColumnIdentifier *colid = e->mutable_identifier();
  if (_tokenizer.cur_token_type_is(Token::Type::IDENT) ||
      _tokenizer.cur_token_type_is_keyword()) {
    Mysqlx::Expr::DocumentPathItem *item =
        colid->mutable_document_path()->Add();
    item->set_type(Mysqlx::Expr::DocumentPathItem::MEMBER);
    std::string value;
    if (_tokenizer.cur_token_type_is(Token::Type::IDENT)) {
      value = _tokenizer.consume_token(Token::Type::IDENT);
    } else {
      const auto &token = _tokenizer.consume_any_token();
      value = token.get_text();
    }
    item->set_value(value.c_str(), value.size());
  }
  document_path(*colid);

  e->set_type(Mysqlx::Expr::Expr::IDENT);
  return e;
}

/*
 * atomic_expr ::=
 *   PLACEHOLDER | ( AT IDENT ) | ( LPAREN expr RPAREN ) | ( [ PLUS | MINUS ]
 * LNUM ) |
 *   (( PLUS | MINUS | NOT | NEG ) atomic_expr ) | LSTRING | NULL | LNUM |
 * LINTEGER | TRUE | FALSE | ( INTERVAL expr ( MICROSECOND | SECOND | MINUTE |
 * HOUR | DAY | WEEK | MONTH | QUARTER | YEAR )) | function_call |
 * column_identifier | cast | binary | placeholder | json_doc | MUL | array
 */
std::unique_ptr<Mysqlx::Expr::Expr> Expr_parser::atomic_expr() {
  // Parse an atomic expression and return a protobuf Expr object
  const Token &t = _tokenizer.consume_any_token();
  const auto type = t.get_type();
  switch (t.get_type()) {
    case Token::Type::PLACEHOLDER:
      return std::unique_ptr<Mysqlx::Expr::Expr>(
          build_literal_expr(build_string_scalar("?")));
    case Token::Type::COLON:
      _tokenizer.unget_token();
      return placeholder();
    case Token::Type::LPAREN: {
      std::unique_ptr<Mysqlx::Expr::Expr> e(my_expr());
      _tokenizer.consume_token(Token::Type::RPAREN);
      return e;
    }
    case Token::Type::PLUS:
    case Token::Type::MINUS:
      if (_tokenizer.cur_token_type_is(Token::Type::LNUM) ||
          _tokenizer.cur_token_type_is(Token::Type::LINTEGER)) {
        const Token &token = _tokenizer.consume_any_token();
        const std::string &val = token.get_text();
        int sign = (type == Token::Type::PLUS) ? 1 : -1;
        if (token.get_type() == Token::Type::LNUM) {
          return std::unique_ptr<Mysqlx::Expr::Expr>(build_literal_expr(
              build_double_scalar(std::stod(val.c_str()) * sign)));
        } else {  // Token::LINTEGER
          return std::unique_ptr<Mysqlx::Expr::Expr>(build_literal_expr(
              build_int_scalar(std::stoi(val.c_str()) * sign)));
        }
      }
    // fallthrough
    case Token::Type::NOT:
    case Token::Type::NEG:
      return std::unique_ptr<Mysqlx::Expr::Expr>(
          build_unary_op(t.get_text(), atomic_expr()));
    case Token::Type::LSTRING:
      return std::unique_ptr<Mysqlx::Expr::Expr>(
          build_literal_expr(build_string_scalar(t.get_text())));
    case Token::Type::T_NULL:
      return std::unique_ptr<Mysqlx::Expr::Expr>(
          build_literal_expr(build_null_scalar()));
    case Token::Type::LNUM:
    case Token::Type::LINTEGER: {
      const std::string &val = t.get_text();
      if (t.get_type() == Token::Type::LNUM) {
        return std::unique_ptr<Mysqlx::Expr::Expr>(
            build_literal_expr(build_double_scalar(std::stod(val.c_str()))));
      } else {  // Token::LINTEGER
        return std::unique_ptr<Mysqlx::Expr::Expr>(
            build_literal_expr(build_int_scalar(std::stoi(val.c_str()))));
      }
    }
    case Token::Type::TRUE_:
    case Token::Type::FALSE_:
      return std::unique_ptr<Mysqlx::Expr::Expr>(
          build_literal_expr(build_bool_scalar(type == Token::Type::TRUE_)));
    case Token::Type::INTERVAL: {
      auto e = std::make_unique<Mysqlx::Expr::Expr>();
      e->set_type(Mysqlx::Expr::Expr::OPERATOR);

      Mysqlx::Expr::Operator *op = e->mutable_operator_();
      op->set_name("interval");
      op->mutable_param()->AddAllocated(my_expr().release());
      // validate the interval units
      if (!_tokenizer.tokens_available() ||
          !_tokenizer.is_interval_units_type()) {
        const Token &tok = _tokenizer.peek_token();
        throw Parser_error("Expected interval units", tok,
                           _tokenizer.get_input());
      }
      const Token &val = _tokenizer.consume_any_token();
      e->mutable_operator_()->mutable_param()->AddAllocated(
          build_literal_expr(build_string_scalar(val.get_text())));
      return e;
    }
    case Token::Type::MUL:
      _tokenizer.unget_token();
      if (!_document_mode)
        return column_field();
      else
        return document_field();
    case Token::Type::CAST:
      _tokenizer.unget_token();
      return cast();
    case Token::Type::LCURLY:
      _tokenizer.unget_token();
      return json_doc();
    case Token::Type::BINARY:
      _tokenizer.unget_token();
      return binary();
    case Token::Type::LSQBRACKET:
      _tokenizer.unget_token();
      return array_();
    case Token::Type::IDENT:
    case Token::Type::DOT:
      _tokenizer.unget_token();
      if (type == Token::Type::IDENT &&
          (_tokenizer.next_token_type(Token::Type::LPAREN) ||
           (_tokenizer.next_token_type(Token::Type::DOT) &&
            _tokenizer.pos_token_type_is(_tokenizer.get_token_pos() + 2,
                                         Token::Type::IDENT) &&
            _tokenizer.pos_token_type_is(_tokenizer.get_token_pos() + 3,
                                         Token::Type::LPAREN)))) {
        return function_call();
      } else {
        if (!_document_mode)
          return column_field();
        else
          return document_field();
      }
    case Token::Type::DOLLAR:
      if (_document_mode) {
        _tokenizer.unget_token();
        return document_field();
      }
      break;
    default:
      // A reserved word should be also treated as identifier
      _tokenizer.unget_token();
      if (_tokenizer.cur_token_type_is_keyword()) {
        if (_tokenizer.next_token_type(Token::Type::LPAREN)) {
          return function_call();
        } else {
          if (!_document_mode)
            return column_field();
          else
            return document_field();
        }
      }
      break;
  }

  throw Parser_error(shcore::str_format("Unexpected %s in expression",
                                        t.get_type_name().c_str()),
                     t, _tokenizer.get_input());
}

/**
 * array ::= LSQBRACKET [ expr (COMMA expr)* ] RQKBRACKET
 */
std::unique_ptr<Mysqlx::Expr::Expr> Expr_parser::array_() {
  auto result = std::make_unique<Mysqlx::Expr::Expr>();

  result->set_type(Mysqlx::Expr::Expr_Type_ARRAY);
  Mysqlx::Expr::Array *a = result->mutable_array();

  _tokenizer.consume_token(Token::Type::LSQBRACKET);

  if (!_tokenizer.cur_token_type_is(Token::Type::RSQBRACKET)) {
    std::unique_ptr<Mysqlx::Expr::Expr> e(my_expr());
    Mysqlx::Expr::Expr *item = a->add_value();
    item->CopyFrom(*e);

    while (_tokenizer.cur_token_type_is(Token::Type::COMMA)) {
      _tokenizer.consume_token(Token::Type::COMMA);
      e = my_expr();
      item = a->add_value();
      item->CopyFrom(*e);
    }
  }

  _tokenizer.consume_token(Token::Type::RSQBRACKET);

  return result;
}

/**
 * json_key_value ::= LSTRING COLON expr
 */
void Expr_parser::json_key_value(Mysqlx::Expr::Object *obj) {
  Mysqlx::Expr::Object_ObjectField *fld = obj->add_fld();
  const std::string &key = _tokenizer.consume_token(Token::Type::LSTRING);
  _tokenizer.consume_token(Token::Type::COLON);
  fld->set_key(key.c_str());
  fld->set_allocated_value(my_expr().release());
}

/**
 * json_doc ::= LCURLY ( json_key_value ( COMMA json_key_value )* )? RCURLY
 */
std::unique_ptr<Mysqlx::Expr::Expr> Expr_parser::json_doc() {
  auto result = std::make_unique<Mysqlx::Expr::Expr>();
  Mysqlx::Expr::Object *obj = result->mutable_object();
  result->set_type(Mysqlx::Expr::Expr_Type_OBJECT);
  _tokenizer.consume_token(Token::Type::LCURLY);
  if (_tokenizer.cur_token_type_is(Token::Type::LSTRING)) {
    json_key_value(obj);
    while (_tokenizer.cur_token_type_is(Token::Type::COMMA)) {
      _tokenizer.consume_any_token();
      json_key_value(obj);
    }
  }
  _tokenizer.consume_token(Token::Type::RCURLY);
  return result;
}

/**
 * placeholder ::= ( COLON INT ) | ( COLON IDENT ) | PLACECHOLDER
 */
std::unique_ptr<Mysqlx::Expr::Expr> Expr_parser::placeholder() {
  auto result = std::make_unique<Mysqlx::Expr::Expr>();
  result->set_type(Mysqlx::Expr::Expr_Type_PLACEHOLDER);

  std::string placeholder_name;
  if (_tokenizer.cur_token_type_is(Token::Type::COLON)) {
    _tokenizer.consume_token(Token::Type::COLON);

    if (_tokenizer.cur_token_type_is(Token::Type::LINTEGER)) {
      placeholder_name = _tokenizer.consume_token(Token::Type::LINTEGER);
    } else if (_tokenizer.cur_token_type_is(Token::Type::IDENT)) {
      placeholder_name = _tokenizer.consume_token(Token::Type::IDENT);
    } else if (_tokenizer.cur_token_type_is_keyword()) {
      const auto &token = _tokenizer.consume_any_token();
      placeholder_name = token.get_text();
    } else {
      placeholder_name = std::to_string(_place_holder_ref->size());
    }
  } else if (_tokenizer.cur_token_type_is(Token::Type::PLACEHOLDER)) {
    _tokenizer.consume_token(Token::Type::PLACEHOLDER);
    placeholder_name = std::to_string(_place_holder_ref->size());
  }

  // Adds a new placeholder if needed
  int position = static_cast<int>(_place_holder_ref->size());
  std::vector<std::string>::iterator index = std::find(
      _place_holder_ref->begin(), _place_holder_ref->end(), placeholder_name);
  if (index == _place_holder_ref->end())
    _place_holder_ref->push_back(placeholder_name);
  else
    position = static_cast<int>(index - _place_holder_ref->begin());

  result->set_position(position);

  return result;
}

/**
 * cast ::= CAST LPAREN expr AS cast_data_type RPAREN
 */
std::unique_ptr<Mysqlx::Expr::Expr> Expr_parser::cast() {
  _tokenizer.consume_token(Token::Type::CAST);
  _tokenizer.consume_token(Token::Type::LPAREN);
  std::unique_ptr<Mysqlx::Expr::Expr> e(my_expr());
  auto result = std::make_unique<Mysqlx::Expr::Expr>();
  // operator
  result->set_type(Mysqlx::Expr::Expr::OPERATOR);
  Mysqlx::Expr::Operator *op = result->mutable_operator_();
  auto id = std::make_unique<Mysqlx::Expr::Identifier>();
  op->set_name(std::string("cast"));
  ::google::protobuf::RepeatedPtrField<::Mysqlx::Expr::Expr> *params =
      op->mutable_param();

  // params
  // 1st arg, expr
  _tokenizer.consume_token(Token::Type::AS);
  params->AddAllocated(e.release());
  // 2nd arg, cast_data_type
  const std::string &type_to_cast = cast_data_type();
  auto type_expr = std::make_unique<Mysqlx::Expr::Expr>();
  type_expr->set_type(Mysqlx::Expr::Expr::LITERAL);
  Mysqlx::Datatypes::Scalar *sc(type_expr->mutable_literal());
  sc->set_type(Mysqlx::Datatypes::Scalar_Type_V_OCTETS);
  sc->mutable_v_octets()->set_value(type_to_cast);
  params->AddAllocated(type_expr.release());
  _tokenizer.consume_token(Token::Type::RPAREN);

  return result;
}

/**
 * cast_data_type ::= ( BINARY dimension? ) | ( CHAR dimension? opt_binary ) | (
 * NCHAR dimension? ) | ( DATE ) | ( DATETIME dimension? ) | ( TIME dimension? )
 *   | ( DECIMAL dimension? ) | ( SIGNED INTEGER? ) | ( UNSIGNED INTEGER? ) |
 * INTEGER | JSON
 */
std::string Expr_parser::cast_data_type() {
  std::string result;
  const Token &token = _tokenizer.peek_token();
  const auto type = token.get_type();

  if ((type == Token::Type::BINARY) || (type == Token::Type::NCHAR) ||
      (type == Token::Type::DATETIME) || (type == Token::Type::TIME)) {
    result += token.get_text();
    _tokenizer.consume_any_token();
    std::string dimension = cast_data_type_dimension();
    if (!dimension.empty()) result += dimension;
  } else if (type == Token::Type::DECIMAL) {
    result += token.get_text();
    _tokenizer.consume_any_token();
    std::string dimension = cast_data_type_dimension(true);
    if (!dimension.empty()) result += dimension;
  } else if (type == Token::Type::DATE) {
    _tokenizer.consume_any_token();
    result += token.get_text();
  } else if (type == Token::Type::CHAR) {
    result += token.get_text();
    _tokenizer.consume_any_token();
    if (_tokenizer.cur_token_type_is(Token::Type::LPAREN))
      result += cast_data_type_dimension();
    const std::string &opt_binary_result = opt_binary();
    if (!opt_binary_result.empty()) result += " " + opt_binary_result;
  } else if (type == Token::Type::SIGNED) {
    result += token.get_text();
    _tokenizer.consume_any_token();
    if (_tokenizer.cur_token_type_is(Token::Type::INTEGER))
      result += " " + _tokenizer.consume_token(Token::Type::INTEGER);
  } else if (type == Token::Type::UNSIGNED) {
    result += token.get_text();
    _tokenizer.consume_any_token();
    if (_tokenizer.cur_token_type_is(Token::Type::INTEGER))
      result += " " + _tokenizer.consume_token(Token::Type::INTEGER);
  } else if (type == Token::Type::INTEGER) {
    result += token.get_text();
    _tokenizer.consume_any_token();
  } else if (type == Token::Type::JSON) {
    result += token.get_text();
    _tokenizer.consume_any_token();
  } else {
    throw Parser_error(shcore::str_format("Unexpected %s in expression",
                                          token.get_type_name().c_str()),
                       token, _tokenizer.get_input());
  }
  return result;
}

/**
 * dimension ::= LPAREN LINTEGER RPAREN
 */
std::string Expr_parser::cast_data_type_dimension(bool double_dimension) {
  if (!_tokenizer.cur_token_type_is(Token::Type::LPAREN)) return "";
  _tokenizer.consume_token(Token::Type::LPAREN);
  std::string result = "(" + _tokenizer.consume_token(Token::Type::LINTEGER);
  if (double_dimension) {
    _tokenizer.consume_token(Token::Type::COMMA);
    result += ", " + _tokenizer.consume_token(Token::Type::LINTEGER);
  }
  result += ")";
  _tokenizer.consume_token(Token::Type::RPAREN);
  return result;
}

/**
 * opt_binary ::= ( ASCII BINARY? ) | ( UNICODE BINARY? ) | ( BINARY ( ASCII |
 * UNICODE | charset_def )? ) | BYTE | < nothing >
 */
std::string Expr_parser::opt_binary() {
  std::string result;
  const Token &token = _tokenizer.peek_token();
  if (token.get_type() == Token::Type::ASCII) {
    result += token.get_text();
    _tokenizer.consume_any_token();
    if (_tokenizer.cur_token_type_is(Token::Type::BINARY))
      result += " " + _tokenizer.consume_any_token().get_text();
    return result;
  } else if (token.get_type() == Token::Type::UNICODE) {
    result += token.get_text();
    _tokenizer.consume_any_token();
    if (_tokenizer.cur_token_type_is(Token::Type::BINARY))
      result += " " + _tokenizer.consume_any_token().get_text();
    return result;
  } else if (token.get_type() == Token::Type::BINARY) {
    result += token.get_text();
    _tokenizer.consume_any_token();
    const Token &token2 = _tokenizer.peek_token();
    if ((token2.get_type() == Token::Type::ASCII) ||
        (token2.get_type() == Token::Type::UNICODE))
      result += " " + token2.get_text();
    else if ((token2.get_type() == Token::Type::CHARACTER) ||
             (token2.get_type() == Token::Type::CHARSET))
      result += " " + charset_def();
    return result;
  } else if (token.get_type() == Token::Type::BYTE) {
    return token.get_text();
  } else {
    return "";
  }
}

/**
 * charset_def ::= (( CHARACTER SET ) | CHARSET ) ( IDENT | STRING | BINARY )
 */
std::string Expr_parser::charset_def() {
  std::string result;
  const Token &token = _tokenizer.consume_any_token();
  if (token.get_type() == Token::Type::CHARACTER) {
    _tokenizer.consume_token(Token::Type::SET);
  } else if (token.get_type() == Token::Type::CHARSET) {
    /* nothing */
  } else {
    throw Parser_error(
        shcore::str_format("Expected %s or %s, but got unexpected %s",
                           to_string(Token::Type::CHARACTER).c_str(),
                           to_string(Token::Type::CHARSET).c_str(),
                           token.get_type_name().c_str()),
        token, _tokenizer.get_input());
  }

  const Token &token2 = _tokenizer.peek_token();
  if ((token2.get_type() == Token::Type::IDENT) ||
      (token2.get_type() == Token::Type::LSTRING) ||
      (token2.get_type() == Token::Type::BINARY)) {
    _tokenizer.consume_any_token();
    result = "charset " + token2.get_text();
  } else {
    throw Parser_error(
        shcore::str_format(
            "Expected either %s, %s or %s, but got unexpected %s",
            to_string(Token::Type::IDENT).c_str(),
            to_string(Token::Type::LSTRING).c_str(),
            to_string(Token::Type::BINARY).c_str(),
            token2.get_type_name().c_str()),
        token2, _tokenizer.get_input());
  }
  return result;
}

/**
 * binary ::= BINARY expr
 */
std::unique_ptr<Mysqlx::Expr::Expr> Expr_parser::binary() {
  // binary
  _tokenizer.consume_token(Token::Type::BINARY);

  auto e = std::make_unique<Mysqlx::Expr::Expr>();
  e->set_type(Mysqlx::Expr::Expr::FUNC_CALL);
  Mysqlx::Expr::FunctionCall *func = e->mutable_function_call();
  auto id = std::make_unique<Mysqlx::Expr::Identifier>();
  id->set_name(std::string("binary"));
  func->set_allocated_name(id.release());
  ::google::protobuf::RepeatedPtrField<::Mysqlx::Expr::Expr> *params =
      func->mutable_param();
  // expr
  params->AddAllocated(my_expr().release());
  return e;
}

std::unique_ptr<Mysqlx::Expr::Expr>
Expr_parser::parse_left_assoc_binary_op_expr(std::set<Token::Type> &types,
                                             inner_parser_t inner_parser) {
  // Given a `set' of types and an Expr-returning inner parser function, parse a
  // left associate binary operator expression
  std::unique_ptr<Mysqlx::Expr::Expr> lhs(inner_parser());
  while (_tokenizer.tokens_available() &&
         _tokenizer.is_type_within_set(types)) {
    auto e = std::make_unique<Mysqlx::Expr::Expr>();
    e->set_type(Mysqlx::Expr::Expr::OPERATOR);
    const Token &t = _tokenizer.consume_any_token();
    const std::string &op_val = t.get_text();
    Mysqlx::Expr::Operator *op = e->mutable_operator_();
    std::string &op_normalized = _tokenizer.map.operator_names.at(op_val);
    op->set_name(op_normalized.c_str(), op_normalized.size());
    op->mutable_param()->AddAllocated(lhs.release());

    op->mutable_param()->AddAllocated(inner_parser().release());
    lhs = std::move(e);
  }
  return lhs;
}

/*
 * mul_div_expr ::= atomic_expr (( MUL | DIV | MOD ) atomic_expr )*
 */
std::unique_ptr<Mysqlx::Expr::Expr> Expr_parser::mul_div_expr() {
  return parse_left_assoc_binary_op_expr(
      _ops.mul_div_expr_types, std::bind(&Expr_parser::atomic_expr, this));
}

/*
 * add_sub_expr ::= mul_div_expr (( PLUS | MINUS ) mul_div_expr )*
 */
std::unique_ptr<Mysqlx::Expr::Expr> Expr_parser::add_sub_expr() {
  return parse_left_assoc_binary_op_expr(
      _ops.add_sub_expr_types, std::bind(&Expr_parser::mul_div_expr, this));
}

/*
 * shift_expr ::= add_sub_expr (( LSHIFT | RSHIFT ) add_sub_expr )*
 */
std::unique_ptr<Mysqlx::Expr::Expr> Expr_parser::shift_expr() {
  return parse_left_assoc_binary_op_expr(
      _ops.shift_expr_types, std::bind(&Expr_parser::add_sub_expr, this));
}

/*
 * bit_expr ::= shift_expr (( BITAND | BITOR | BITXOR ) shift_expr )*
 */
std::unique_ptr<Mysqlx::Expr::Expr> Expr_parser::bit_expr() {
  return parse_left_assoc_binary_op_expr(
      _ops.bit_expr_types, std::bind(&Expr_parser::shift_expr, this));
}

/*
 * comp_expr ::= bit_expr (( GE | GT | LE | LT | QE | NE ) bit_expr )*
 */
std::unique_ptr<Mysqlx::Expr::Expr> Expr_parser::comp_expr() {
  return parse_left_assoc_binary_op_expr(
      _ops.comp_expr_types, std::bind(&Expr_parser::bit_expr, this));
}

/*
 * ilri_expr ::= comp_expr [ NOT ] (( IS [ NOT ] comp_expr ) | ( IN
 * paren_expr_list ) | ( LIKE comp_expr [ ESCAPE comp_expr ] ) | ( BETWEEN
 * comp_expr AND comp_expr ) | ( REGEXP comp_expr ) | (OVERLAPS comp_expr)
 */
std::unique_ptr<Mysqlx::Expr::Expr> Expr_parser::ilri_expr() {
  auto e = std::make_unique<Mysqlx::Expr::Expr>();
  std::unique_ptr<Mysqlx::Expr::Expr> lhs(comp_expr());
  bool is_not = false;
  if (_tokenizer.cur_token_type_is(Token::Type::NOT)) {
    is_not = true;
    _tokenizer.consume_token(Token::Type::NOT);
  }
  if (_tokenizer.tokens_available()) {
    ::google::protobuf::RepeatedPtrField<::Mysqlx::Expr::Expr> *params =
        e->mutable_operator_()->mutable_param();
    const Token &op_name_tok = _tokenizer.peek_token();
    std::string op_name(op_name_tok.get_text());
    bool has_op_name = true;
    std::transform(op_name.begin(), op_name.end(), op_name.begin(), ::tolower);
    if (_tokenizer.cur_token_type_is(Token::Type::IS)) {
      _tokenizer.consume_token(Token::Type::IS);
      // for IS, NOT comes AFTER
      if (_tokenizer.cur_token_type_is(Token::Type::NOT)) {
        is_not = true;
        _tokenizer.consume_token(Token::Type::NOT);
      }
      params->AddAllocated(lhs.release());
      params->AddAllocated(comp_expr().release());
    } else if (_tokenizer.cur_token_type_is(Token::Type::IN_)) {
      _tokenizer.consume_token(Token::Type::IN_);
      params->AddAllocated(lhs.release());
      if (!_tokenizer.cur_token_type_is(Token::Type::LPAREN)) {
        if (is_not)
          op_name = "not_cont_in";
        else
          op_name = "cont_in";
        is_not = false;
        params->AddAllocated(comp_expr().release());
      } else {
        paren_expr_list(params);
      }
    } else if (_tokenizer.cur_token_type_is(Token::Type::LIKE)) {
      _tokenizer.consume_token(Token::Type::LIKE);
      params->AddAllocated(lhs.release());
      params->AddAllocated(comp_expr().release());
      if (_tokenizer.cur_token_type_is(Token::Type::ESCAPE)) {
        _tokenizer.consume_token(Token::Type::ESCAPE);
        params->AddAllocated(comp_expr().release());
      }
    } else if (_tokenizer.cur_token_type_is(Token::Type::BETWEEN)) {
      _tokenizer.consume_token(Token::Type::BETWEEN);
      params->AddAllocated(lhs.release());
      params->AddAllocated(comp_expr().release());
      _tokenizer.consume_token(Token::Type::AND);
      params->AddAllocated(comp_expr().release());
    } else if (_tokenizer.cur_token_type_is(Token::Type::REGEXP)) {
      _tokenizer.consume_token(Token::Type::REGEXP);
      params->AddAllocated(lhs.release());
      params->AddAllocated(comp_expr().release());
    } else if (_tokenizer.cur_token_type_is(Token::Type::OVERLAPS)) {
      _tokenizer.consume_token(Token::Type::OVERLAPS);
      params->AddAllocated(lhs.release());
      params->AddAllocated(comp_expr().release());
    } else {
      if (is_not) {
        throw Parser_error("Unknown token after NOT", op_name_tok,
                           _tokenizer.get_input());
      }
      has_op_name = false;
    }
    if (has_op_name) {
      e->set_type(Mysqlx::Expr::Expr::OPERATOR);
      Mysqlx::Expr::Operator *op = e->mutable_operator_();
      op->set_name(op_name.c_str(), op_name.size());
      if (is_not) {
        // wrap if `NOT'-prefixed
        lhs.reset(build_unary_op("not", std::move(e)));
      } else {
        lhs = std::move(e);
      }
    }
  }

  return lhs;
}

/*
 * and_expr ::= ilri_expr ( AND ilri_expr )*
 */
std::unique_ptr<Mysqlx::Expr::Expr> Expr_parser::and_expr() {
  return parse_left_assoc_binary_op_expr(
      _ops.and_expr_types, std::bind(&Expr_parser::ilri_expr, this));
}

/*
 * or_expr ::= and_expr ( OR and_expr )*
 */
std::unique_ptr<Mysqlx::Expr::Expr> Expr_parser::or_expr() {
  return parse_left_assoc_binary_op_expr(
      _ops.or_expr_types, std::bind(&Expr_parser::and_expr, this));
}

/*
 * my_expr ::= or_expr
 */
std::unique_ptr<Mysqlx::Expr::Expr> Expr_parser::my_expr() { return or_expr(); }

/*
 * expr ::= or_expr
 */
std::unique_ptr<Mysqlx::Expr::Expr> Expr_parser::expr() {
  std::unique_ptr<Mysqlx::Expr::Expr> result(or_expr());
  if (_tokenizer.tokens_available()) {
    const Token &tok = _tokenizer.peek_token();
    throw Parser_error("Expected end of expression", tok,
                       _tokenizer.get_input());
  }
  return result;
}

std::string Expr_unparser::any_to_string(const Mysqlx::Datatypes::Any &a) {
  if (a.type() == Mysqlx::Datatypes::Any::SCALAR) {
    return Expr_unparser::scalar_to_string(a.scalar());
  } else {
    // TODO(?): handle objects & array here
    throw Parser_error("Unknown type tag at Any" + a.DebugString());
  }
  return "";
}

std::string Expr_unparser::escape_literal(const std::string &s) {
  std::string result = s;
  Expr_unparser::replace(result, "\"", "\"\"");
  return result;
}

std::string Expr_unparser::scalar_to_string(
    const Mysqlx::Datatypes::Scalar &s) {
  switch (s.type()) {
    case Mysqlx::Datatypes::Scalar::V_SINT:
      return std::to_string(s.v_signed_int());
    case Mysqlx::Datatypes::Scalar::V_UINT:
      return std::to_string(s.v_unsigned_int());
    case Mysqlx::Datatypes::Scalar::V_DOUBLE:
      return std::to_string(s.v_double());
    case Mysqlx::Datatypes::Scalar::V_BOOL: {
      if (s.v_bool())
        return "TRUE";
      else
        return "FALSE";
    }
    case Mysqlx::Datatypes::Scalar::V_OCTETS: {
      const char *value = s.v_octets().value().c_str();
      return "\"" + Expr_unparser::escape_literal(value) + "\"";
    }
    case Mysqlx::Datatypes::Scalar::V_NULL:
      return "NULL";
    default:
      throw Parser_error("Unknown type tag at Scalar: " + s.DebugString());
  }
}

std::string Expr_unparser::data_type_to_string(
    const Mysqlx::Datatypes::Scalar &s) {
  if (s.type() == Mysqlx::Datatypes::Scalar::V_OCTETS) {
    return s.v_octets().value();
  } else {
    throw Parser_error("Unknown data type at Scalar: " + s.DebugString());
  }
}

std::string Expr_unparser::document_path_to_string(
    const ::google::protobuf::RepeatedPtrField<::Mysqlx::Expr::DocumentPathItem>
        &dp) {
  std::string s;
  std::vector<std::string> parts;
  for (int i = 0; i < dp.size(); ++i) {
    const Mysqlx::Expr::DocumentPathItem &dpi = dp.Get(i);
    switch (dpi.type()) {
      case Mysqlx::Expr::DocumentPathItem::MEMBER:
        parts.push_back("." + dpi.value());
        break;
      case Mysqlx::Expr::DocumentPathItem::MEMBER_ASTERISK:
        parts.push_back("." + dpi.value());
        break;
      case Mysqlx::Expr::DocumentPathItem::ARRAY_INDEX:
        parts.push_back(shcore::str_format("[%d]", dpi.index()));
        break;
      case Mysqlx::Expr::DocumentPathItem::ARRAY_INDEX_ASTERISK:
        parts.push_back("[*]");
        break;
      case Mysqlx::Expr::DocumentPathItem::DOUBLE_ASTERISK:
        parts.push_back("**");
        break;
    }
  }

  std::vector<std::string>::const_iterator myend = parts.end();
  for (std::vector<std::string>::const_iterator it = parts.begin(); it != myend;
       ++it) {
    s = s + *it;
  }

  return s;
}

std::string Expr_unparser::column_identifier_to_string(
    const Mysqlx::Expr::ColumnIdentifier &colid) {
  std::string s = Expr_unparser::quote_identifier(colid.name());
  if (colid.has_table_name()) {
    s = Expr_unparser::quote_identifier(colid.table_name()) + "." + s;
  }
  if (colid.has_schema_name()) {
    s = Expr_unparser::quote_identifier(colid.schema_name()) + "." + s;
  }
  std::string dp =
      Expr_unparser::document_path_to_string(colid.document_path());
  if (!dp.empty()) s = s + "$" + dp;
  return s;
}

std::string Expr_unparser::function_call_to_string(
    const Mysqlx::Expr::FunctionCall &fc) {
  std::string s = Expr_unparser::quote_identifier(fc.name().name()) + "(";
  if (fc.name().has_schema_name()) {
    s = Expr_unparser::quote_identifier(fc.name().schema_name()) + "." + s;
  }
  for (int i = 0; i < fc.param().size(); ++i) {
    s = s + Expr_unparser::expr_to_string(fc.param().Get(i));
    if (i + 1 < fc.param().size()) {
      s = s + ", ";
    }
  }
  return s + ")";
}

std::string Expr_unparser::operator_to_string(
    const Mysqlx::Expr::Operator &op) {
  const ::google::protobuf::RepeatedPtrField<::Mysqlx::Expr::Expr> &ps =
      op.param();
  std::string name = shcore::str_upper(op.name());
  if (name == "IN") {
    std::string s = Expr_unparser::expr_to_string(ps.Get(0)) + " IN (";
    for (int i = 1; i < ps.size(); ++i) {
      s = s + Expr_unparser::expr_to_string(ps.Get(i));
      if (i + 1 < ps.size()) s = s + ", ";
    }
    return s + ")";
  } else if (name == "INTERVAL") {
    std::string result =
        "INTERVAL " + Expr_unparser::expr_to_string(ps.Get(0)) + " ";
    std::string data = Expr_unparser::expr_to_string(ps.Get(1));
    Expr_unparser::replace(data, "\"", "");
    return result + data;
  } else if (name == "BETWEEN") {
    std::string result = Expr_unparser::expr_to_string(ps.Get(0));
    result += " BETWEEN ";
    result += Expr_unparser::expr_to_string(ps.Get(1));
    result += " AND ";
    result += Expr_unparser::expr_to_string(ps.Get(2));
    return result;
  } else if (name == "LIKE" && ps.size() == 3) {
    return Expr_unparser::expr_to_string(ps.Get(0)) + " LIKE " +
           Expr_unparser::expr_to_string(ps.Get(1)) + " ESCAPE " +
           Expr_unparser::expr_to_string(ps.Get(2));
  } else if (name == "CAST" && ps.size() == 2) {
    return "CAST(" + Expr_unparser::expr_to_string(ps.Get(0)) + " AS " +
           Expr_unparser::data_type_to_string(ps.Get(1).literal()) + ")";
  } else if (ps.size() == 2) {
    std::string op_name(op.name());
    std::transform(op_name.begin(), op_name.end(), op_name.begin(), ::toupper);
    std::string result = "(" + Expr_unparser::expr_to_string(ps.Get(0)) + " " +
                         op_name + " " +
                         Expr_unparser::expr_to_string(ps.Get(1)) + ")";
    return result;
  } else if (ps.size() == 1) {
    std::string op_name(op.name());
    std::transform(op_name.begin(), op_name.end(), op_name.begin(), ::toupper);
    if (op_name.size() == 1) {
      return op_name + Expr_unparser::expr_to_string(ps.Get(0));
    } else {
      // something like NOT
      return op_name + " ( " + Expr_unparser::expr_to_string(ps.Get(0)) + ")";
    }
  } else if (name == "*" && ps.size() == 0) {
    return "*";
  } else {
    throw Parser_error(
        shcore::str_format("Unknown operator structure %s", op.name().c_str()));
  }

  return "";
}

void Expr_unparser::replace(std::string &target, const std::string &old_val,
                            const std::string &new_val) {
  size_t len_skip =
      std::abs(static_cast<int>(old_val.size() - new_val.size()) - 1);
  std::string result = target;
  std::string::size_type pos = 0;
  while ((pos = result.find(old_val, pos + len_skip)) != std::string::npos) {
    result = result.replace(pos, old_val.size(), new_val);
  }
  target = result;
}

std::string Expr_unparser::quote_identifier(const std::string &id) {
  if (id.find("`") != std::string::npos || id.find("\"") != std::string::npos ||
      id.find("'") != std::string::npos || id.find("$") != std::string::npos ||
      id.find(".") != std::string::npos) {
    std::string result = id;
    Expr_unparser::replace(result, "`", "``");
    return "`" + result + "`";
  } else {
    return id;
  }
}

std::string Expr_unparser::expr_to_string(const Mysqlx::Expr::Expr &e) {
  if (e.type() == Mysqlx::Expr::Expr::LITERAL) {
    return Expr_unparser::scalar_to_string(e.literal());
  } else if (e.type() == Mysqlx::Expr::Expr::IDENT) {
    return Expr_unparser::column_identifier_to_string(e.identifier());
  } else if (e.type() == Mysqlx::Expr::Expr::FUNC_CALL) {
    return Expr_unparser::function_call_to_string(e.function_call());
  } else if (e.type() == Mysqlx::Expr::Expr::OPERATOR) {
    return Expr_unparser::operator_to_string(e.operator_());
  } else if (e.type() == Mysqlx::Expr::Expr::VARIABLE) {
    return std::string("$") + Expr_unparser::quote_identifier(e.variable());
  } else if (e.type() == Mysqlx::Expr::Expr::OBJECT) {
    return Expr_unparser::object_to_string(e.object());
  } else if (e.type() == Mysqlx::Expr::Expr::PLACEHOLDER) {
    return Expr_unparser::placeholder_to_string(e);
  } else if (e.type() == Mysqlx::Expr::Expr::ARRAY) {
    return Expr_unparser::array_to_string(e);
  } else {
    throw Parser_error(
        shcore::str_format("Unknown expression type: %d", e.type()));
  }

  return "";
}

std::string Expr_unparser::placeholder_to_string(const Mysqlx::Expr::Expr &e) {
  std::string result = ":" + std::to_string(e.position());
  return result;
}

std::string Expr_unparser::array_to_string(const Mysqlx::Expr::Expr &e) {
  std::string result = "[ ";

  const Mysqlx::Expr::Array &a = e.array();
  bool first = true;
  for (int i = 0; i < a.value_size(); i++) {
    if (first)
      first = false;
    else
      result += ", ";
    result += Expr_unparser::expr_to_string(a.value(i));
  }

  result += " ]";

  return result;
}

std::string Expr_unparser::object_to_string(const Mysqlx::Expr::Object &o) {
  bool first = true;
  std::string result = "{ ";
  for (int i = 0; i < o.fld_size(); ++i) {
    if (first)
      first = false;
    else
      result += ", ";
    const Mysqlx::Expr::Object_ObjectField &fld = o.fld(i);
    result += "'" + fld.key() + "' : ";
    result += Expr_unparser::expr_to_string(fld.value());
  }
  result += " }";
  return result;
}

std::string Expr_unparser::column_to_string(const Mysqlx::Crud::Projection &c) {
  std::string result = Expr_unparser::expr_to_string(c.source());

  if (c.has_alias()) result += " as " + c.alias();
  return result;
}

std::string Expr_unparser::order_to_string(const Mysqlx::Crud::Order &c) {
  std::string result = Expr_unparser::expr_to_string(c.expr());
  if ((!c.has_direction()) ||
      (c.direction() == Mysqlx::Crud::Order_Direction_ASC))
    result += " asc";
  else
    result += " desc";
  return result;
}

std::string Expr_unparser::column_list_to_string(
    google::protobuf::RepeatedPtrField<::Mysqlx::Crud::Projection> columns) {
  std::string result("projection (");
  for (int i = 0; i < columns.size(); i++) {
    std::string strcol = Expr_unparser::column_to_string(columns.Get(i));
    result += strcol;
    if (i + 1 < columns.size()) result += ", ";
  }
  result += ")";
  return result;
}

std::string Expr_unparser::order_list_to_string(
    google::protobuf::RepeatedPtrField<::Mysqlx::Crud::Order> columns) {
  std::string result("orderby (");
  for (int i = 0; i < columns.size(); i++) {
    std::string strcol = Expr_unparser::order_to_string(columns.Get(i));
    result += strcol;
    if (i + 1 < columns.size()) result += ", ";
  }
  result += ")";
  return result;
}

}  // namespace mysqlx
