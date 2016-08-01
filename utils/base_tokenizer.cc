/*
 * Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
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

#include "base_tokenizer.h"

#include <stdexcept>
#include <memory>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include <cstdlib>

#ifndef WIN32
#  include <strings.h>
#  define _stricmp strcasecmp
#endif

// Avoid warnings from protobuf and rapidjson
#if defined __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wpedantic"
// TODO: Once expr parser does not have deps on std::auto_ptr
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#ifndef __has_warning
#define __has_warning(x) 0
#endif
#if __has_warning("-Wunused-local-typedefs")
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#endif
#elif defined _MSC_VER
#pragma warning (push)
#pragma warning (disable : 4018 4996) //TODO: add MSVC code for pedantic
#endif

#include <boost/format.hpp>
/*#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/case_conv.hpp>*/

#ifdef __GNUC__
#pragma GCC diagnostic pop
#elif defined _MSC_VER
#pragma warning (pop)
#endif

using namespace shcore;

BaseToken::BaseToken(const std::string& type, const std::string& text, int cur_pos) : _type(type), _text(text), _pos(cur_pos)
{
}

BaseTokenizer::BaseTokenizer() : _allow_spaces(true)
{
  _pos = 0;
  _parent_offset = 0;
}

void BaseTokenizer::process(const std::string& input, size_t offset)
{
  _parent_offset = offset;
  _pos = 0;
  _input = input;

  get_tokens();
}

bool BaseTokenizer::next_char_is(tokens_t::size_type i, int tok)
{
  return (i + 1) < _input.size() && _input[i + 1] == tok;
}

void BaseTokenizer::assert_cur_token(const std::string& type)
{
  assert_tok_position();
  const BaseToken& tok = _tokens.at(_pos);
  std::string tok_type = tok.get_type();
  if (tok_type != type)
    throw std::runtime_error((boost::format("Expected token type %1% at position %2% but found type %3% (%4%)") % type % tok.get_pos() % tok_type % tok.get_text()).str());
}

bool BaseTokenizer::cur_token_type_is(const std::string& type)
{
  return pos_token_type_is(_pos, type);
}

bool BaseTokenizer::next_token_type(const std::string& type, size_t pos)
{
  return pos_token_type_is(_pos + pos, type);
}

bool BaseTokenizer::pos_token_type_is(tokens_t::size_type pos, const std::string& type)
{
  return (pos < _tokens.size()) && (_tokens[pos].get_type() == type);
}

const std::string& BaseTokenizer::consume_token(const std::string& type)
{
  assert_cur_token(type);
  const std::string& v = _tokens[_pos++].get_text();
  return v;
}

const BaseToken& BaseTokenizer::peek_token()
{
  assert_tok_position();
  BaseToken& t = _tokens[_pos];
  return t;
}

const BaseToken* BaseTokenizer::peek_last_token()
{
  return _tokens.size() ? &_tokens[_tokens.size() - 1] : nullptr;
}

void BaseTokenizer::unget_token()
{
  if (_pos == 0)
    throw std::runtime_error("Attempt to get back a token when already at first token (position 0).");
  --_pos;
}

void BaseTokenizer::get_tokens()
{
  for (size_t i = 0; i < _input.size(); ++i)
  {
    if (std::isspace(_input[i]))
    {
      if (!_allow_spaces)
        throw std::runtime_error((boost::format("Illegal space found at position %1%") % (_parent_offset + i)).str());

      continue;
    }
    else
    {
      std::string type(&_input[i], 1);
      if (_base_tokens.find(type) != _base_tokens.end())
        _tokens.push_back(BaseToken(type, _base_tokens[type], i));
      else
      {
        bool found = false;
        for (auto token_type : _custom_tokens)
        {
          if (_token_functions.find(token_type) != _token_functions.end())
          {
            std::string text;
            size_t start = i;
            if (_token_functions[type](_input, i, text))
            {
              _tokens.push_back(BaseToken(token_type, text, start));
              found = true;
              break;
            }
            else
              i = start;
          }

          // Token sequences are to create a single token with many characters as long as they belong to the given sequence
          else if (_token_sequences.find(token_type) != _token_sequences.end())
          {
            size_t start = i;

            while (i < _input.length() && _token_sequences[token_type].find(_input[i]) != std::string::npos)
              i++;

            found = i > start;
            if (found)
            {
              _tokens.push_back(BaseToken(token_type, _input.substr(start, i - start), start));
              i--;
              break;
            }
          }

          else if (_token_vectors.find(token_type) != _token_vectors.end())
          {
            std::string text;
            size_t start = i;
            for (auto item : _token_vectors[token_type])
            {
              if (item.find(_input[i]) != std::string::npos)
              {
                text += _input[i];
                i++;
              }
              else
                break;
            }

            if ((i - start) == _token_vectors[token_type].size())
            {
              _tokens.push_back(BaseToken(token_type, text, start));
              found = true;
              i--;
              break;
            }
            else
              i = start;
          }
        }

        if (_final_group.find(type) != std::string::npos)
        {
          _tokens.push_back(BaseToken(_final_type, _input.substr(i), i));
          found = true;
          break;
        }

        if (!found)
          throw std::runtime_error((boost::format("Illegal character [%1%] found at position %2%") % _input[i] % (_parent_offset + i)).str());
      }
    }
  }
}

void BaseTokenizer::inc_pos_token()
{
  ++_pos;
}

const BaseToken& BaseTokenizer::consume_any_token()
{
  assert_tok_position();
  BaseToken& tok = _tokens[_pos];
  ++_pos;
  return tok;
}

void BaseTokenizer::assert_tok_position()
{
  if (_pos >= _tokens.size())
    throw std::runtime_error((boost::format("Expected token at position %d but no tokens left.") % _pos).str());
}

bool BaseTokenizer::tokens_available()
{
  return _pos < _tokens.size();
}

void BaseTokenizer::set_complex_token(const std::string &type, std::function<bool(const std::string& input, size_t&, std::string&)>function)
{
  if (std::find(_custom_tokens.begin(), _custom_tokens.end(), type) == _custom_tokens.end())
  {
    _custom_tokens.push_back(type);
    _token_functions[type] = function;
  }
}

void BaseTokenizer::set_complex_token(const std::string &type, const std::string& group)
{
  if (std::find(_custom_tokens.begin(), _custom_tokens.end(), type) == _custom_tokens.end())
  {
    _custom_tokens.push_back(type);
    _token_sequences[type] = group;
  }
}

void BaseTokenizer::set_complex_token(const std::string &type, const std::vector<std::string>& groups)
{
  if (std::find(_custom_tokens.begin(), _custom_tokens.end(), type) == _custom_tokens.end())
  {
    _custom_tokens.push_back(type);
    _token_vectors[type] = groups;
  }
}

void BaseTokenizer::remove_complex_token(const std::string& name)
{
  if (_token_functions.find(name) != _token_functions.end())
  {
    _token_functions.erase(name);
    _custom_tokens.erase(std::find(_custom_tokens.begin(), _custom_tokens.end(), name));
  }

  if (_token_vectors.find(name) != _token_vectors.end())
  {
    _token_vectors.erase(name);
    _custom_tokens.erase(std::find(_custom_tokens.begin(), _custom_tokens.end(), name));
  }
}