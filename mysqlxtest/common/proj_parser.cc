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

#include "proj_parser.h"
#include "mysqlx_crud.pb.h"

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

using namespace mysqlx;

Proj_parser::Proj_parser(const std::string& expr_str, bool document_mode, bool allow_alias)
: _tokenizer(expr_str), _document_mode(document_mode), _allow_alias(allow_alias)
{
  _tokenizer.get_tokens();
}

/*
 * document_mode = false:
 *   column_identifier ::= ( IDENT [ DOT IDENT [ DOT IDENT ] ] [ '@' document_path ] ) [as_rule]
 * document_mode = true:
 *   column_identifier ::=  ( [ IDENT ] document_path ) [as_rule]
 *
 * NOTE: 'as_rule' only applies if allow_alias is true (see Proj_parser ctor)
 * as_rule ::= AS IDENT
 */
void Proj_parser::column_identifier(Mysqlx::Crud::Column &col)
{
  if (!_document_mode)
  {
    std::vector<std::string> parts;
    const std::string& part = _tokenizer.consume_token(Token::IDENT);
    parts.push_back(part);
    while (_tokenizer.cur_token_type_is(Token::DOT))
    {
      _tokenizer.consume_token(Token::DOT);
      parts.push_back(_tokenizer.consume_token(Token::IDENT));
    }
    if (parts.size() > 3)
      throw Parser_error((boost::format("Too many parts to identifier at %d") % _tokenizer.get_token_pos()).str());
    std::string fullname;
    std::vector<std::string>::const_iterator it, myend = parts.end();
    size_t i = 0;
    for (it = parts.begin(); it != myend; ++it, ++i)
    {
      fullname += *it;
      if (i + 1 < parts.size())
        fullname += ".";
    }
    col.set_name(fullname.c_str());

    if (_tokenizer.cur_token_type_is(Token::AT))
    {
      _tokenizer.consume_token(Token::AT);
      document_path(col);
    }
  }
  else
  {
    if (_tokenizer.cur_token_type_is(Token::IDENT))
    {
      Mysqlx::Expr::DocumentPathItem& item = *col.mutable_document_path()->Add();
      item.set_type(Mysqlx::Expr::DocumentPathItem::MEMBER);
      const std::string& value = _tokenizer.consume_token(Token::IDENT);
      item.set_value(value.c_str(), value.size());
    }
    document_path(col);
  }
  if (_tokenizer.cur_token_type_is(Token::AS))
  {
    if (_allow_alias)
    {
      _tokenizer.consume_token(Token::AS);
      const std::string& alias = _tokenizer.consume_token(Token::IDENT);
      col.set_alias(alias.c_str());
    }
    else
    {
      throw Parser_error((boost::format("Unexpected token 'AS' at pos %d") % _tokenizer.get_token_pos()).str());
    }
  }
}

/*
 * docpath_member ::= DOT ( IDENT | LSTRING | MUL )
 */
void Proj_parser::docpath_member(Mysqlx::Expr::DocumentPathItem& item)
{
  _tokenizer.consume_token(Token::DOT);
  item.set_type(Mysqlx::Expr::DocumentPathItem::MEMBER);
  if (_tokenizer.cur_token_type_is(Token::IDENT))
  {
    const std::string& ident = _tokenizer.consume_token(Token::IDENT);
    item.set_value(ident.c_str(), ident.size());
  }
  else if (_tokenizer.cur_token_type_is(Token::LSTRING))
  {
    const std::string& lstring = _tokenizer.consume_token(Token::LSTRING);
    item.set_value(lstring.c_str(), lstring.size());
  }
  else if (_tokenizer.cur_token_type_is(Token::MUL))
  {
    const std::string& mul = _tokenizer.consume_token(Token::MUL);
    item.set_value(mul.c_str(), mul.size());
  }
  else
  {
    throw Parser_error((boost::format("Expected token type IDENT or LSTRING in JSON path at token pos %d") % _tokenizer.get_token_pos()).str());
  }
}

/*
 * docpath_array_lock ::= LSQBRACKET ( MUL | LNUM ) RSQBRACKET
 */
void Proj_parser::docpath_array_loc(Mysqlx::Expr::DocumentPathItem& item)
{
  _tokenizer.consume_token(Token::LSQBRACKET);
  if (_tokenizer.cur_token_type_is(Token::MUL))
  {
    _tokenizer.consume_token(Token::RSQBRACKET);
    item.set_type(Mysqlx::Expr::DocumentPathItem::ARRAY_INDEX_ASTERISK);
  }
  else if (_tokenizer.cur_token_type_is(Token::LNUM))
  {
    const std::string& value = _tokenizer.consume_token(Token::LNUM);
    int v = boost::lexical_cast<int>(value.c_str(), value.size());
    if (v < 0)
      throw Parser_error((boost::format("Array index cannot be negative at %d") % _tokenizer.get_token_pos()).str());
    _tokenizer.consume_token(Token::RSQBRACKET);
    item.set_type(Mysqlx::Expr::DocumentPathItem::ARRAY_INDEX);
    item.set_index(v);
  }
  else
  {
    throw Parser_error((boost::format("Exception token type MUL or LNUM in JSON path array index at token pos %d") % _tokenizer.get_token_pos()).str());
  }
}

/*
 * document_path ::= ( docpath_member | docpath_array_loc | '**' )+
 */
void Proj_parser::document_path(Mysqlx::Crud::Column& col)
{
  // Parse a JSON-style document path, like WL#7909, but prefix by @. instead of $.
  while (true)
  {
    if (_tokenizer.cur_token_type_is(Token::DOT))
    {
      docpath_member(*col.mutable_document_path()->Add());
    }
    else if (_tokenizer.cur_token_type_is(Token::LSQBRACKET))
    {
      docpath_array_loc(*col.mutable_document_path()->Add());
    }
    else if (_tokenizer.cur_token_type_is(Token::DOUBLESTAR))
    {
      _tokenizer.consume_token(Token::DOUBLESTAR);
      Mysqlx::Expr::DocumentPathItem& item = *col.mutable_document_path()->Add();
      item.set_type(Mysqlx::Expr::DocumentPathItem::DOUBLE_ASTERISK);
    }
    else
    {
      break;
    }
  }
  size_t size = col.document_path_size();
  if (size > 0 && (col.document_path(size - 1).type() == Mysqlx::Expr::DocumentPathItem::DOUBLE_ASTERISK))
  {
    throw Parser_error((boost::format("JSON path may not end in '**' at %d") % _tokenizer.get_token_pos()).str());
  }
}