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

#include "expr_parser.h"

#include <stdexcept>
#include <memory>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include <cstdlib>

#ifndef WIN32
#include <strings.h>
#  define _stricmp strcasecmp
#endif

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/case_conv.hpp>

using namespace mysqlx;

Mysqlx::Datatypes::Any* Expr_builder::build_null_any()
{
  Mysqlx::Datatypes::Any* a = new Mysqlx::Datatypes::Any();
  a->set_type(Mysqlx::Datatypes::Any::SCALAR);
  Mysqlx::Datatypes::Scalar *sc = a->mutable_scalar();
  sc->set_type(Mysqlx::Datatypes::Scalar::V_NULL);
  return a;
}

Mysqlx::Datatypes::Any* Expr_builder::build_double_any(double d)
{
  Mysqlx::Datatypes::Any* a = new Mysqlx::Datatypes::Any();
  a->set_type(Mysqlx::Datatypes::Any::SCALAR);
  Mysqlx::Datatypes::Scalar *sc = a->mutable_scalar();
  sc->set_type(Mysqlx::Datatypes::Scalar::V_DOUBLE);
  sc->set_v_double(d);
  return a;
}

Mysqlx::Datatypes::Any* Expr_builder::build_int_any(google::protobuf::int64 i)
{
  Mysqlx::Datatypes::Any* a = new Mysqlx::Datatypes::Any();
  a->set_type(Mysqlx::Datatypes::Any::SCALAR);
  Mysqlx::Datatypes::Scalar *sc = a->mutable_scalar();
  sc->set_type(Mysqlx::Datatypes::Scalar::V_SINT);
  sc->set_v_signed_int(i);
  return a;
}

Mysqlx::Datatypes::Any* Expr_builder::build_string_any(const std::string& s)
{
  Mysqlx::Datatypes::Any* a = new Mysqlx::Datatypes::Any();
  a->set_type(Mysqlx::Datatypes::Any::SCALAR);
  Mysqlx::Datatypes::Scalar *sc = a->mutable_scalar();
  sc->set_type(Mysqlx::Datatypes::Scalar::V_OCTETS);
  sc->set_v_opaque(s.c_str(), s.size());
  return a;
}

Mysqlx::Datatypes::Any* Expr_builder::build_bool_any(bool b)
{
  Mysqlx::Datatypes::Any* a = new Mysqlx::Datatypes::Any();
  a->set_type(Mysqlx::Datatypes::Any::SCALAR);
  Mysqlx::Datatypes::Scalar *sc = a->mutable_scalar();
  sc->set_type(Mysqlx::Datatypes::Scalar::V_BOOL);
  sc->set_v_bool(b);
  return a;
}

Mysqlx::Expr::Expr* Expr_builder::build_literal_expr(Mysqlx::Datatypes::Any* a)
{
  Mysqlx::Expr::Expr *e = new Mysqlx::Expr::Expr();
  e->set_type(Mysqlx::Expr::Expr::LITERAL);
  e->set_allocated_constant(a);
  return e;
}

Mysqlx::Expr::Expr* Expr_builder::build_unary_op(const std::string& name, Mysqlx::Expr::Expr* param)
{
  Mysqlx::Expr::Expr* e = new Mysqlx::Expr::Expr();
  e->set_type(Mysqlx::Expr::Expr::OPERATOR);
  Mysqlx::Expr::Operator *op = e->mutable_operator_();
  op->mutable_param()->AddAllocated(param);
  op->set_name(name.c_str(), name.size());
  return e;
}

Token::Token(Token::TokenType type, const std::string& text) : _type(type), _text(text)
{
}

const std::string& Token::get_text() const
{
  return _text;
}

Token::TokenType Token::get_type() const
{
  return _type;
}

struct Tokenizer::maps map;

Tokenizer::Tokenizer(const std::string& input) : _input(input)
{
  _pos = 0;
}

bool Tokenizer::next_char_is(tokens_t::size_type i, int tok)
{
  return (i + 1) < _input.size() && _input[i + 1] == tok;
}

void Tokenizer::assert_cur_token(Token::TokenType type)
{
  assert_tok_position();
  Token::TokenType tok_type = _tokens.at(_pos).get_type();
  if (tok_type != type)
    throw Parser_error((boost::format("Expected token type %d at pos %d but found type %d.") % type % _pos % tok_type).str());
}

bool Tokenizer::cur_token_type_is(Token::TokenType type)
{
  return pos_token_type_is(_pos, type);
}

bool Tokenizer::next_token_type(Token::TokenType type)
{
  return pos_token_type_is(_pos + 1, type);
}

bool Tokenizer::pos_token_type_is(tokens_t::size_type pos, Token::TokenType type)
{
  return (pos < _tokens.size()) && (_tokens[pos].get_type() == type);
}

const std::string& Tokenizer::consume_token(Token::TokenType type)
{
  assert_cur_token(type);
  const std::string& v = _tokens[_pos++].get_text();
  return v;
}

const Token& Tokenizer::peek_token()
{
  assert_tok_position();
  Token& t = _tokens[_pos];
  return t;
}

void Tokenizer::unget_token()
{
  if (_pos == 0)
    throw Parser_error("Attempt to get back a token when already at first token (position 0).");
  --_pos;
}

void Tokenizer::get_tokens()
{
  for (size_t i = 0; i < _input.size(); ++i)
  {
    char c = _input[i];
    if (std::isspace(c))
    {
      // do nothing
      continue;
    }
    else if (std::isdigit(c))
    {
      // numerical literal
      int start = i;
      // floating grammar is
      // float -> int '.' (int | (int expo[sign] int))
      // int -> digit +
      // expo -> 'E' | 'e'
      // sign -> '-' | '+'
      while (i < _input.size() && std::isdigit(c = _input[i]))
        ++i;
      if (i < _input.size() && _input[i] == '.')
      {
        ++i;
        while (i < _input.size() && std::isdigit(_input[i]))
          ++i;
        if (i < _input.size() && std::toupper(_input[i]) == 'E')
        {
          ++i;
          if (i < _input.size() && ((c = _input[i]) == '-') || (c == '+'))
            ++i;
          int j = i;
          while (i < _input.size() && std::isdigit(_input[i]))
            i++;
          if (i == j)
            throw Parser_error((boost::format("Tokenizer: Missing exponential value for floating point at char %d") % i).str());
        }
      }
      _tokens.push_back(Token(Token::LNUM, std::string(_input, start, i - start)));
      if (i < _input.size())
        --i;
    }
    else if (!std::isalpha(c) && c != '_')
    {
      // # non-identifier, e.g. operator or quoted literal
      if (c == '?')
      {
        _tokens.push_back(Token(Token::PLACEHOLDER, std::string(1, c)));
      }
      else if (c == '+')
      {
        _tokens.push_back(Token(Token::PLUS, std::string(1, c)));
      }
      else if (c == '-')
      {
        _tokens.push_back(Token(Token::MINUS, std::string(1, c)));
      }
      else if (c == '*')
      {
        if (next_char_is(i, '*'))
        {
          ++i;
          _tokens.push_back(Token(Token::DOUBLESTAR, std::string("**")));
        }
        else
        {
          _tokens.push_back(Token(Token::MUL, std::string(1, c)));
        }
      }
      else if (c == '/')
      {
        _tokens.push_back(Token(Token::DIV, std::string(1, c)));
      }
      else if (c == '@')
      {
        _tokens.push_back(Token(Token::AT, std::string(1, c)));
      }
      else if (c == '%')
      {
        _tokens.push_back(Token(Token::MOD, std::string(1, c)));
      }
      else if (c == '=')
      {
        _tokens.push_back(Token(Token::EQ, std::string(1, c)));
      }
      else if (c == '&')
      {
        _tokens.push_back(Token(Token::BITAND, std::string(1, c)));
      }
      else if (c == '|')
      {
        _tokens.push_back(Token(Token::BITOR, std::string(1, c)));
      }
      else if (c == '(')
      {
        _tokens.push_back(Token(Token::LPAREN, std::string(1, c)));
      }
      else if (c == ')')
      {
        _tokens.push_back(Token(Token::RPAREN, std::string(1, c)));
      }
      else if (c == '[')
      {
        _tokens.push_back(Token(Token::LSQBRACKET, std::string(1, c)));
      }
      else if (c == ']')
      {
        _tokens.push_back(Token(Token::RSQBRACKET, std::string(1, c)));
      }
      else if (c == '~')
      {
        _tokens.push_back(Token(Token::NEG, std::string(1, c)));
      }
      else if (c == ',')
      {
        _tokens.push_back(Token(Token::COMMA, std::string(1, c)));
      }
      else if (c == '!')
      {
        if (next_char_is(i, '='))
        {
          ++i;
          _tokens.push_back(Token(Token::NE, std::string("!=")));
        }
        else
        {
          _tokens.push_back(Token(Token::BANG, std::string(1, c)));
        }
      }
      else if (c == '<')
      {
        if (next_char_is(i, '<'))
        {
          ++i;
          _tokens.push_back(Token(Token::LSHIFT, std::string("<<")));
        }
        else if (next_char_is(i, '='))
        {
          ++i;
          _tokens.push_back(Token(Token::LE, std::string("<=")));
        }
        else
        {
          _tokens.push_back(Token(Token::LT, std::string("<")));
        }
      }
      else if (c == '>')
      {
        if (next_char_is(i, '>'))
        {
          ++i;
          _tokens.push_back(Token(Token::RSHIFT, std::string(">>")));
        }
        else if (next_char_is(i, '='))
        {
          ++i;
          _tokens.push_back(Token(Token::GE, std::string(">=")));
        }
        else
        {
          _tokens.push_back(Token(Token::GT, std::string(1, c)));
        }
      }
      else if (c == '.')
      {
        if ((i + 1) < _input.size() && std::isdigit(_input[i + 1]))
        {
          size_t start = i;
          ++i;
          // floating grammar is
          // float -> '.' (int | (int expo[sign] int))
          // nint->digit +
          // expo -> 'E' | 'e'
          // sign -> '-' | '+'
          while (i < _input.size() && std::isdigit(_input[i]))
            ++i;
          if (i < _input.size() && std::toupper(_input[i]) == 'E')
          {
            if (i < _input.size() && ((c = _input[i]) == '+') || (c == '-'))
              ++i;
            size_t j = i;
            while (i < _input.size() && std::isdigit(_input[i]))
              ++i;
            if (i == j)
              throw Parser_error((boost::format("Tokenizer: Missing exponential value for floating point at char %d") % i).str());
          }
          _tokens.push_back(Token(Token::LNUM, std::string(_input, start, i - start)));
          if (i < _input.size())
            --i;
        }
        else
        {
          _tokens.push_back(Token(Token::DOT, std::string(1, c)));
        }
      }
      else if (c == '"' || c == '\'' || c == '`')
      {
        char quote_char = c;
        std::string val;
        size_t start = ++i;

        while (i < _input.size())
        {
          c = _input[i];
          if ((c == quote_char) && ((i + 1) < _input.size()) && (_input[i + 1] != quote_char))
          {
            // break if we have a quote char that's not double
            break;
          }
          else if ((c == quote_char) || (c == '\\'  && quote_char != '`'))
          {
            // && quote_char != '`'
            // this quote char has to be doubled
            if ((i + 1) >= _input.size())
              break;
            val.append(1, _input[++i]);
          }
          else
            val.append(1, c);
          ++i;
        }
        if ((i >= _input.size()) && (_input[i] != quote_char))
        {
          throw Parser_error((boost::format("Unterminated quoted string starting at %d") % start).str());
        }
        if (quote_char == '`')
        {
          _tokens.push_back(Token(Token::IDENT, val));
        }
        else
        {
          _tokens.push_back(Token(Token::LSTRING, val));
        }
      }
      else
      {
        throw Parser_error((boost::format("Unknown character at %d") % i).str());
      }
    }
    else
    {
      size_t start = i;
      while (i < _input.size() && (std::isalnum(_input[i]) || _input[i] == '_'))
        ++i;
      std::string val(_input, start, i - start);
      maps::reserved_words_t::const_iterator it = map.reserved_words.find(val);
      if (it != map.reserved_words.end())
      {
        _tokens.push_back(Token(it->second, val));
      }
      else
      {
        _tokens.push_back(Token(Token::IDENT, val));
      }
      --i;
    }
  }
}

void Tokenizer::inc_pos_token()
{
  ++_pos;
}

int Tokenizer::get_token_pos()
{
  return _pos;
}

const Token& Tokenizer::consume_any_token()
{
  assert_tok_position();
  Token& tok = _tokens[_pos];
  ++_pos;
  return tok;
}

void Tokenizer::assert_tok_position()
{
  if (_pos >= _tokens.size())
    throw Parser_error((boost::format("Expected at pos %d but no tokens left.") % _pos).str());
}

bool Tokenizer::tokens_available()
{
  return _pos < _tokens.size();
}

bool Tokenizer::is_interval_units_type()
{
  assert_tok_position();
  Token::TokenType type = _tokens[_pos].get_type();
  return map.interval_units.find(type) != map.interval_units.end();
}

bool Tokenizer::is_type_within_set(const std::set<Token::TokenType>& types)
{
  assert_tok_position();
  Token::TokenType type = _tokens[_pos].get_type();
  return types.find(type) != types.end();
}

bool Tokenizer::cmp_icase::operator()(const std::string& lhs, const std::string& rhs) const
{
  const char *c_lhs = lhs.c_str();
  const char *c_rhs = rhs.c_str();

  return _stricmp(c_lhs, c_rhs) < 0;
}

Expr_parser::Expr_parser(const std::string& expr_str, bool document_mode) : _document_mode(document_mode), _tokenizer(expr_str)
{
  _tokenizer.get_tokens();
}

void Expr_parser::paren_expr_list(::google::protobuf::RepeatedPtrField< ::Mysqlx::Expr::Expr >* expr_list)
{
  // Parse a paren-bounded expression list for function arguments or IN list and return a list of Expr objects
  _tokenizer.consume_token(Token::LPAREN);
  if (!_tokenizer.cur_token_type_is(Token::RPAREN))
  {
    std::auto_ptr<Mysqlx::Expr::Expr> ptr = expr();
    expr_list->AddAllocated(ptr.get());
    ptr.release();
    while (_tokenizer.cur_token_type_is(Token::COMMA))
    {
      _tokenizer.inc_pos_token();
      ptr = expr();
      expr_list->AddAllocated(ptr.get());
      ptr.release();
    }
  }
  _tokenizer.consume_token(Token::RPAREN);
}

std::auto_ptr<Mysqlx::Expr::Identifier> Expr_parser::identifier()
{
  _tokenizer.assert_cur_token(Token::IDENT);
  std::auto_ptr<Mysqlx::Expr::Identifier> id = std::auto_ptr<Mysqlx::Expr::Identifier>(new Mysqlx::Expr::Identifier());
  if (_tokenizer.next_token_type(Token::DOT))
  {
    const std::string& schema_name = _tokenizer.consume_token(Token::IDENT);
    id->set_name(schema_name.c_str(), schema_name.size());
    _tokenizer.consume_token(Token::DOT);
  }
  const std::string& name = _tokenizer.consume_token(Token::IDENT);
  id->set_name(name.c_str(), name.size());
  return id;
}

std::auto_ptr<Mysqlx::Expr::Expr> Expr_parser::function_call()
{
  std::auto_ptr<Mysqlx::Expr::Expr> e = std::auto_ptr<Mysqlx::Expr::Expr>(new Mysqlx::Expr::Expr());
  e->set_type(Mysqlx::Expr::Expr::FUNC_CALL);
  Mysqlx::Expr::FunctionCall* func = e->mutable_function_call();
  std::auto_ptr<Mysqlx::Expr::Identifier> id = identifier();
  func->set_allocated_name(id.get());
  id.release();
  paren_expr_list(func->mutable_param());
  return e;
}

void Expr_parser::docpath_member(Mysqlx::Expr::DocumentPathItem& item)
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

void Expr_parser::docpath_array_loc(Mysqlx::Expr::DocumentPathItem& item)
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

void Expr_parser::document_path(Mysqlx::Expr::ColumnIdentifier& colid)
{
  // Parse a JSON-style document path, like WL#7909, but prefix by @. instead of $.
  while (true)
  {
    if (_tokenizer.cur_token_type_is(Token::DOT))
    {
      docpath_member(*colid.mutable_document_path()->Add());
    }
    else if (_tokenizer.cur_token_type_is(Token::LSQBRACKET))
    {
      docpath_array_loc(*colid.mutable_document_path()->Add());
    }
    else if (_tokenizer.cur_token_type_is(Token::DOUBLESTAR))
    {
      _tokenizer.consume_token(Token::DOUBLESTAR);
      Mysqlx::Expr::DocumentPathItem* item = colid.mutable_document_path()->Add();
      item->set_type(Mysqlx::Expr::DocumentPathItem::DOUBLE_ASTERISK);
    }
    else
    {
      break;
    }
  }
  size_t size = colid.document_path_size();
  if (size > 0 && (colid.document_path(size - 1).type() == Mysqlx::Expr::DocumentPathItem::DOUBLE_ASTERISK))
  {
    throw Parser_error((boost::format("JSON path may not end in '**' at %d") % _tokenizer.get_token_pos()).str());
  }
}

std::auto_ptr<Mysqlx::Expr::Expr> Expr_parser::column_identifier()
{
  std::auto_ptr<Mysqlx::Expr::Expr> e = std::auto_ptr<Mysqlx::Expr::Expr>(new Mysqlx::Expr::Expr());
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
    Mysqlx::Expr::ColumnIdentifier* colid = e->mutable_identifier();
    std::vector<std::string>::reverse_iterator myend = parts.rend();
    int i = 0;
    for (std::vector<std::string>::reverse_iterator it = parts.rbegin(); it != myend; ++it, ++i)
    {
      std::string& s = *it;
      if (i == 0)
        colid->set_name(s.c_str(), s.size());
      else if (i == 1)
        colid->set_table_name(s.c_str(), s.size());
      else if (i == 2)
        colid->set_schema_name(s.c_str(), s.size());
    }
    if (_tokenizer.cur_token_type_is(Token::AT))
    {
      _tokenizer.consume_token(Token::AT);
      document_path(*colid);
    }
  }
  else
  {
    Mysqlx::Expr::ColumnIdentifier* colid = e->mutable_identifier();
    if (_tokenizer.cur_token_type_is(Token::IDENT))
    {
      Mysqlx::Expr::DocumentPathItem* item = colid->mutable_document_path()->Add();
      item->set_type(Mysqlx::Expr::DocumentPathItem::MEMBER);
      const std::string& value = _tokenizer.consume_token(Token::IDENT);
      item->set_value(value.c_str(), value.size());
    }
    document_path(*colid);
  }
  e->set_type(Mysqlx::Expr::Expr::IDENT);
  return e;
}

std::auto_ptr<Mysqlx::Expr::Expr> Expr_parser::atomic_expr()
{
  // Parse an atomic expression and return a protobuf Expr object
  const Token& t = _tokenizer.consume_any_token();
  int type = t.get_type();
  if (type == Token::PLACEHOLDER)
  {
    return std::auto_ptr<Mysqlx::Expr::Expr>(Expr_builder::build_literal_expr(Expr_builder::build_string_any("?")));
  }
  else if (type == Token::AT)
  {
    // TODO: make sure this doesn't interfere with un-prefixed JSON paths
    std::auto_ptr<Mysqlx::Expr::Expr> e = std::auto_ptr<Mysqlx::Expr::Expr>(new Mysqlx::Expr::Expr());
    e->set_type(Mysqlx::Expr::Expr::VARIABLE);
    const std::string& id = _tokenizer.consume_token(Token::IDENT);
    e->set_variable(id.c_str(), id.size());
    return e;
  }
  else if (type == Token::LPAREN)
  {
    std::auto_ptr<Mysqlx::Expr::Expr> e = expr();
    _tokenizer.consume_token(Token::RPAREN);
    return e;
  }
  else if (_tokenizer.cur_token_type_is(Token::LNUM) && ((type == Token::PLUS) || (type == Token::MINUS)))
  {
    const std::string& val = _tokenizer.consume_token(Token::LNUM);
    int sign = (type == Token::PLUS) ? 1 : -1;
    if (val.find(".") != std::string::npos)
    {
      return std::auto_ptr<Mysqlx::Expr::Expr>(Expr_builder::build_literal_expr(Expr_builder::build_double_any(boost::lexical_cast<double>(val.c_str()) * sign)));
    }
    else
    {
      return std::auto_ptr<Mysqlx::Expr::Expr>(Expr_builder::build_literal_expr(Expr_builder::build_int_any(boost::lexical_cast<int>(val.c_str()) * sign)));
    }
  }
  else if (type == Token::PLUS || type == Token::MINUS || type == Token::NOT || type == Token::NEG)
  {
    std::auto_ptr<Mysqlx::Expr::Expr> tmp = atomic_expr();
    std::auto_ptr<Mysqlx::Expr::Expr> result = std::auto_ptr<Mysqlx::Expr::Expr>(Expr_builder::build_unary_op(t.get_text(), tmp.get()));
    tmp.release();
    return result;
  }
  else if (type == Token::LSTRING)
  {
    return std::auto_ptr<Mysqlx::Expr::Expr>(Expr_builder::build_literal_expr(Expr_builder::build_string_any(t.get_text())));
  }
  else if (type == Token::T_NULL)
  {
    return std::auto_ptr<Mysqlx::Expr::Expr>(Expr_builder::build_literal_expr(Expr_builder::build_null_any()));
  }
  else if (type == Token::LNUM)
  {
    const std::string& val = t.get_text();
    if (val.find(".") != std::string::npos)
    {
      return std::auto_ptr<Mysqlx::Expr::Expr>(Expr_builder::build_literal_expr(Expr_builder::build_double_any(boost::lexical_cast<double>(val.c_str()))));
    }
    else
    {
      return std::auto_ptr<Mysqlx::Expr::Expr>(Expr_builder::build_literal_expr(Expr_builder::build_int_any(boost::lexical_cast<int>(val.c_str()))));
    }
  }
  else if (type == Token::TRUE_ || type == Token::FALSE_)
  {
    return std::auto_ptr<Mysqlx::Expr::Expr>(Expr_builder::build_literal_expr(Expr_builder::build_bool_any(type == Token::TRUE_)));
  }
  else if (type == Token::INTERVAL)
  {
    std::auto_ptr<Mysqlx::Expr::Expr> e = std::auto_ptr<Mysqlx::Expr::Expr>(new Mysqlx::Expr::Expr());
    std::auto_ptr<Mysqlx::Expr::Expr> operand(NULL);
    e->set_type(Mysqlx::Expr::Expr::OPERATOR);
    operand = expr();

    Mysqlx::Expr::Operator* op = e->mutable_operator_();
    op->set_name("interval");
    op->mutable_param()->AddAllocated(operand.get());
    operand.release();
    // validate the interval units
    if (_tokenizer.tokens_available() && _tokenizer.is_interval_units_type())
    {
      ;
    }
    else
    {
      throw Parser_error((boost::format("Expected interval units at %d") % _tokenizer.get_token_pos()).str());
    }
    const Token& val = _tokenizer.consume_any_token();
    std::auto_ptr<Mysqlx::Expr::Expr> param = std::auto_ptr<Mysqlx::Expr::Expr>(Expr_builder::build_literal_expr(Expr_builder::build_string_any(val.get_text())));
    e->mutable_operator_()->mutable_param()->AddAllocated(param.get());
    param.release();
    return e;
  }
  else if (type == Token::IDENT)
  {
    _tokenizer.unget_token();
    if (_tokenizer.next_token_type(Token::LPAREN) ||
      (_tokenizer.next_token_type(Token::DOT) && _tokenizer.pos_token_type_is(_tokenizer.get_token_pos() + 2, Token::IDENT) && _tokenizer.pos_token_type_is(_tokenizer.get_token_pos() + 3, Token::LPAREN)))
    {
      return function_call();
    }
    else
    {
      return column_identifier();
    }
  }
  throw Parser_error((boost::format("Unknown token type = %d when expecting atomic expression at %d") % type % _tokenizer.get_token_pos()).str());
}

std::auto_ptr<Mysqlx::Expr::Expr> Expr_parser::parse_left_assoc_binary_op_expr(std::set<Token::TokenType>& types, inner_parser_t inner_parser)
{
  // Given a `set' of types and an Expr-returning inner parser function, parse a left associate binary operator expression
  std::auto_ptr<Mysqlx::Expr::Expr> lhs = std::auto_ptr<Mysqlx::Expr::Expr>(inner_parser(this));
  while (_tokenizer.tokens_available() && _tokenizer.is_type_within_set(types))
  {
    std::auto_ptr<Mysqlx::Expr::Expr> e = std::auto_ptr<Mysqlx::Expr::Expr>(new Mysqlx::Expr::Expr());
    e->set_type(Mysqlx::Expr::Expr::OPERATOR);
    const Token &t = _tokenizer.consume_any_token();
    const std::string& op_val = t.get_text();
    Mysqlx::Expr::Operator* op = e->mutable_operator_();
    std::string& op_normalized = _tokenizer.map.operator_names.at(op_val);
    op->set_name(op_normalized.c_str(), op_normalized.size());
    op->mutable_param()->AddAllocated(lhs.get());
    lhs.release();
    std::auto_ptr<Mysqlx::Expr::Expr> tmp = inner_parser(this);
    op->mutable_param()->AddAllocated(tmp.get());
    tmp.release();
    lhs = e;
  }
  return lhs;
}

std::auto_ptr<Mysqlx::Expr::Expr> Expr_parser::mul_div_expr()
{
  std::set<Token::TokenType> types;
  types.insert(Token::MUL);
  types.insert(Token::DIV);
  types.insert(Token::MOD);
  return parse_left_assoc_binary_op_expr(types, &Expr_parser::atomic_expr);
}

std::auto_ptr<Mysqlx::Expr::Expr> Expr_parser::add_sub_expr()
{
  std::set<Token::TokenType> types;
  types.insert(Token::PLUS);
  types.insert(Token::MINUS);
  return parse_left_assoc_binary_op_expr(types, &Expr_parser::mul_div_expr);
}

std::auto_ptr<Mysqlx::Expr::Expr> Expr_parser::shift_expr()
{
  std::set<Token::TokenType> types;
  types.insert(Token::LSHIFT);
  types.insert(Token::RSHIFT);
  return parse_left_assoc_binary_op_expr(types, &Expr_parser::add_sub_expr);
}

std::auto_ptr<Mysqlx::Expr::Expr> Expr_parser::bit_expr()
{
  std::set<Token::TokenType> types;
  types.insert(Token::BITAND);
  types.insert(Token::BITOR);
  types.insert(Token::BITXOR);
  return parse_left_assoc_binary_op_expr(types, &Expr_parser::shift_expr);
}

std::auto_ptr<Mysqlx::Expr::Expr> Expr_parser::comp_expr()
{
  std::set<Token::TokenType> types;
  types.insert(Token::GE);
  types.insert(Token::GT);
  types.insert(Token::LE);
  types.insert(Token::LT);
  types.insert(Token::EQ);
  types.insert(Token::NE);
  return parse_left_assoc_binary_op_expr(types, &Expr_parser::bit_expr);
}

std::auto_ptr<Mysqlx::Expr::Expr> Expr_parser::ilri_expr()
{
  std::auto_ptr<Mysqlx::Expr::Expr> e = std::auto_ptr<Mysqlx::Expr::Expr>(new Mysqlx::Expr::Expr());
  std::auto_ptr<Mysqlx::Expr::Expr> lhs = comp_expr();
  bool is_not = false;
  if (_tokenizer.cur_token_type_is(Token::NOT))
  {
    is_not = true;
    _tokenizer.consume_token(Token::NOT);
  }
  if (_tokenizer.tokens_available())
  {
    ::google::protobuf::RepeatedPtrField< ::Mysqlx::Expr::Expr >* params = e->mutable_operator_()->mutable_param();
    const Token& op_name_tok = _tokenizer.peek_token();
    const std::string& op_name = op_name_tok.get_text();
    bool has_op_name = true;
    //boost::to_upper(op_name);

    if (_tokenizer.cur_token_type_is(Token::IS))
    {
      _tokenizer.consume_token(Token::IS);
      // for IS, NOT comes AFTER
      if (_tokenizer.cur_token_type_is(Token::NOT))
      {
        is_not = true;
        _tokenizer.consume_token(Token::NOT);
      }
      std::auto_ptr<Mysqlx::Expr::Expr> tmp = comp_expr();
      params->AddAllocated(lhs.get());
      params->AddAllocated(tmp.get());
      tmp.release();
    }
    else if (_tokenizer.cur_token_type_is(Token::IN_))
    {
      _tokenizer.consume_token(Token::IN_);
      params->AddAllocated(lhs.get());
      paren_expr_list(params);
    }
    else if (_tokenizer.cur_token_type_is(Token::LIKE))
    {
      _tokenizer.consume_token(Token::LIKE);
      std::auto_ptr<Mysqlx::Expr::Expr> tmp = comp_expr();
      params->AddAllocated(lhs.get());
      params->AddAllocated(tmp.get());
      tmp.release();
      if (_tokenizer.cur_token_type_is(Token::ESCAPE))
      {
        _tokenizer.consume_token(Token::ESCAPE);
        tmp = comp_expr();
        params->AddAllocated(tmp.get());
        tmp.release();
      }
    }
    else if (_tokenizer.cur_token_type_is(Token::BETWEEN))
    {
      _tokenizer.consume_token(Token::BETWEEN);
      params->AddAllocated(lhs.get());
      std::auto_ptr<Mysqlx::Expr::Expr> tmp = comp_expr();
      params->AddAllocated(tmp.get());
      tmp.release();
      _tokenizer.consume_token(Token::AND);
      tmp = comp_expr();
      params->AddAllocated(tmp.get());
      tmp.release();
    }
    else if (_tokenizer.cur_token_type_is(Token::REGEXP))
    {
      _tokenizer.consume_token(Token::REGEXP);
      std::auto_ptr<Mysqlx::Expr::Expr> tmp = comp_expr();
      params->AddAllocated(lhs.get());
      params->AddAllocated(tmp.get());
      tmp.release();
    }
    else
    {
      if (is_not)
        throw Parser_error((boost::format("Unknown token after NOT as pos %d") % _tokenizer.get_token_pos()).str());
      has_op_name = false;
    }
    if (has_op_name)
    {
      e->set_type(Mysqlx::Expr::Expr::OPERATOR);
      Mysqlx::Expr::Operator* op = e->mutable_operator_();
      op->set_name(op_name.c_str(), op_name.size());
      if (is_not)
      {
        // wrap if `NOT'-prefixed
        Mysqlx::Expr::Expr* expr = Expr_builder::build_unary_op("NOT", e.get());
        e.release();
        lhs.release();
        lhs.reset(expr);
      }
      else
      {
        lhs.release();
        lhs = e;
      }
    }
  }

  return lhs;
}

std::auto_ptr<Mysqlx::Expr::Expr> Expr_parser::and_expr()
{
  std::set<Token::TokenType> types;
  types.insert(Token::AND);
  return parse_left_assoc_binary_op_expr(types, &Expr_parser::ilri_expr);
}

std::auto_ptr<Mysqlx::Expr::Expr> Expr_parser::or_expr()
{
  std::set<Token::TokenType> types;
  types.insert(Token::OR);
  return parse_left_assoc_binary_op_expr(types, &Expr_parser::and_expr);
}

std::auto_ptr<Mysqlx::Expr::Expr> Expr_parser::expr()
{
  return or_expr();
}

std::string Expr_unparser::any_to_string(const Mysqlx::Datatypes::Any& a)
{
  if (a.type() == Mysqlx::Datatypes::Any::SCALAR)
  {
    return Expr_unparser::scalar_to_string(a.scalar());
  }
  else
  {
    // TODO: handle objects & array here
    throw Parser_error("Unknown type tag at Any" + a.DebugString());
  }
  return "";
}

std::string Expr_unparser::escape_literal(const std::string& s)
{
  std::string result = s;
  Expr_unparser::replace(result, "\"", "\"\"");
  return result;
}

std::string Expr_unparser::scalar_to_string(const Mysqlx::Datatypes::Scalar& s)
{
  switch (s.type())
  {
    case Mysqlx::Datatypes::Scalar::V_SINT:
      return (boost::format("%d") % s.v_signed_int()).str();
    case Mysqlx::Datatypes::Scalar::V_DOUBLE:
      return (boost::format("%f") % s.v_double()).str();
    case Mysqlx::Datatypes::Scalar::V_BOOL:
    {
      if (s.v_bool())
        return "TRUE";
      else
        return "FALSE";
    }
    case Mysqlx::Datatypes::Scalar::V_OCTETS:
    {
      const char* value = s.v_opaque().c_str();
      return "\"" + Expr_unparser::escape_literal(value) + "\"";
    }
    case Mysqlx::Datatypes::Scalar::V_NULL:
      return "NULL";
    default:
      throw Parser_error("Unknown type tag at Scalar: " + s.DebugString());
  }
}

std::string Expr_unparser::document_path_to_string(const ::google::protobuf::RepeatedPtrField< ::Mysqlx::Expr::DocumentPathItem >& dp)
{
  std::string s;
  std::vector<std::string> parts;
  for (int i = 0; i < dp.size(); ++i)
  {
    const Mysqlx::Expr::DocumentPathItem& dpi = dp.Get(i);
    switch (dpi.type())
    {
      case Mysqlx::Expr::DocumentPathItem::MEMBER:
        parts.push_back("." + dpi.value());
        break;
      case Mysqlx::Expr::DocumentPathItem::ARRAY_INDEX:
        parts.push_back((boost::format("[%d]") % dpi.index()).str());
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
  for (std::vector<std::string>::const_iterator it = parts.begin(); it != myend; ++it)
  {
    s = s + *it;
  }

  return s;
}

std::string Expr_unparser::column_identifier_to_string(const Mysqlx::Expr::ColumnIdentifier& colid)
{
  std::string s = Expr_unparser::quote_identifier(colid.name());
  if (colid.has_table_name())
  {
    s = Expr_unparser::quote_identifier(colid.table_name()) + "." + s;
  }
  if (colid.has_schema_name())
  {
    s = Expr_unparser::quote_identifier(colid.schema_name()) + "." + s;
  }
  std::string dp = Expr_unparser::document_path_to_string(colid.document_path());
  if (!dp.empty())
    s = s + "@" + dp;
  return s;
}

std::string Expr_unparser::function_call_to_string(const Mysqlx::Expr::FunctionCall& fc)
{
  std::string s = Expr_unparser::quote_identifier(fc.name().name()) + "(";
  if (fc.name().has_schema_name())
  {
    s = Expr_unparser::quote_identifier(fc.name().schema_name()) + "." + s;
  }
  for (int i = 0; i < fc.param().size(); ++i)
  {
    s = s + Expr_unparser::expr_to_string(fc.param().Get(i));
    if (i + 1 < fc.param().size())
    {
      s = s + ", ";
    }
  }
  return s + ")";
}

std::string Expr_unparser::operator_to_string(const Mysqlx::Expr::Operator& op)
{
  const ::google::protobuf::RepeatedPtrField< ::Mysqlx::Expr::Expr >& ps = op.param();
  std::string name = std::string(op.name());
  boost::to_upper(name);
  if (name == "IN")
  {
    std::string s = Expr_unparser::expr_to_string(ps.Get(0)) + " IN (";
    for (int i = 1; i < ps.size(); ++i)
    {
      s = s + Expr_unparser::expr_to_string(ps.Get(i));
      if (i + 1 < ps.size())
        s = s + ", ";
    }
    return s + ")";
  }
  else if (name == "INTERVAL")
  {
    std::string result = "INTERVAL " + Expr_unparser::expr_to_string(ps.Get(0)) + " ";
    std::string data = Expr_unparser::expr_to_string(ps.Get(1));
    Expr_unparser::replace(data, "\"", "");
    return result + data;
  }
  else if (name == "BETWEEN")
  {
    std::string result = Expr_unparser::expr_to_string(ps.Get(0));
    result += " BETWEEN ";
    result += Expr_unparser::expr_to_string(ps.Get(1));
    result += " AND ";
    result += Expr_unparser::expr_to_string(ps.Get(2));
    return result;
  }
  else if (name == "LIKE" && ps.size() == 3)
  {
    return Expr_unparser::expr_to_string(ps.Get(0)) + " LIKE " + Expr_unparser::expr_to_string(ps.Get(1)) + " ESCAPE " + Expr_unparser::expr_to_string(ps.Get(2));
  }
  else if (ps.size() == 2)
  {
    std::string result = "(" + Expr_unparser::expr_to_string(ps.Get(0)) + " " + op.name() + " " + Expr_unparser::expr_to_string(ps.Get(1)) + ")";
    return result;
  }
  else if (ps.size() == 1)
  {
    const std::string name = op.name();
    if (name.size() == 1)
    {
      return name + Expr_unparser::expr_to_string(ps.Get(0));
    }
    else
    {
      // something like NOT
      return name + " ( " + Expr_unparser::expr_to_string(ps.Get(0)) + ")";
    }
  }
  else
  {
    throw Parser_error((boost::format("Unknown operator structure %s") % op.name()).str());
  }

  return "";
}

void Expr_unparser::replace(std::string& target, const std::string& old_val, const std::string& new_val)
{
  size_t len_skip = std::abs((signed long)(old_val.size() - new_val.size() - 1));
  std::string result = target;
  std::string::size_type pos = 0;
  while ((pos = result.find(old_val, pos + len_skip)) != std::string::npos)
  {
    result = result.replace(pos, old_val.size(), new_val);
  }
  target = result;
}

std::string Expr_unparser::quote_identifier(const std::string& id)
{
  if (id.find("`") != std::string::npos || id.find("\"") != std::string::npos || id.find("'") != std::string::npos || id.find("@") != std::string::npos || id.find(".") != std::string::npos)
  {
    std::string result = id;
    Expr_unparser::replace(result, "`", "``");
    return "`" + result + "`";
  }
  else
    return id;
}

std::string Expr_unparser::expr_to_string(const Mysqlx::Expr::Expr& e)
{
  if (e.type() == Mysqlx::Expr::Expr::LITERAL)
  {
    return Expr_unparser::any_to_string(e.constant());
  }
  else if (e.type() == Mysqlx::Expr::Expr::IDENT)
  {
    return Expr_unparser::column_identifier_to_string(e.identifier());
  }
  else if (e.type() == Mysqlx::Expr::Expr::FUNC_CALL)
  {
    return Expr_unparser::function_call_to_string(e.function_call());
  }
  else if (e.type() == Mysqlx::Expr::Expr::OPERATOR)
  {
    return Expr_unparser::operator_to_string(e.operator_());
  }
  else if (e.type() == Mysqlx::Expr::Expr::VARIABLE)
  {
    return std::string("@") + Expr_unparser::quote_identifier(e.variable());
  }
  else
  {
    throw Parser_error((boost::format("Unknown expression type: %d") % e.type()).str());
  }

  return "";
}

std::string Expr_unparser::column_to_string(const Mysqlx::Crud::Column& c)
{
  std::string result = c.name();
  if (c.document_path_size() != 0)
    result += "@" + Expr_unparser::document_path_to_string(c.document_path());
  if (c.has_alias())
    result += " as " + c.alias();
  return result;
}

std::string Expr_unparser::column_list_to_string(std::vector<Mysqlx::Crud::Column*>& columns)
{
  std::string result("projection (");
  std::vector<Mysqlx::Crud::Column*>::const_iterator it, myend = columns.end();
  int i = 0;
  for (it = columns.begin(); it != myend; ++it, ++i)
  {
    std::string strcol = Expr_unparser::column_to_string(**it);
    result += strcol;
    if (i + 1 < columns.size())
      result += ", ";
  }
  result += ")";
  return result;
}