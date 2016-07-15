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

#include "uri_parser.h"
#include <cctype>
#include <boost/format.hpp>
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

#ifdef __GNUC__
#pragma GCC diagnostic pop
#elif defined _MSC_VER
#pragma warning (pop)
#endif

#include <boost/lexical_cast.hpp>
using namespace shcore;

std::string Uri_parser::DELIMITERS = ":/?#[]@";
std::string Uri_parser::SUBDELIMITERS = "!$&'()*+,;=";
std::string Uri_parser::ALPHA = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
std::string Uri_parser::DIGIT = "0123456789";
std::string Uri_parser::HEXDIG = "ABCDEFabcdef0123456789";
std::string Uri_parser::ALPHANUMERIC = ALPHA + DIGIT;
std::string Uri_parser::RESERVED = DELIMITERS + SUBDELIMITERS;
std::string Uri_parser::UNRESERVED = ALPHANUMERIC + "-._~";

#define IS_DELIMITER(x) DELIMITERS.find(x)!=std::string::npos
#define IS_SUBDELIMITER(x) SUBDELIMITERS.find(x)!=std::string::npos
#define IS_ALPHA(x) ALPHA.find(x)!=std::string::npos
#define IS_DIGIT(x) DIGIT.find(x)!=std::string::npos
#define IS_HEXDIG(x) HEXDIG.find(x)!=std::string::npos
#define IS_ALPHANUMERIC(x) ALPHANUMERIC.find(x)!=std::string::npos
#define IS_RESERVED(x) RESERVED.find(x)!=std::string::npos
#define IS_UNRESERVED(x) UNRESERVED.find(x)!=std::string::npos

Uri_parser::Uri_parser() :_has_password(false), _has_port(false), _port(0)
{
}

void Uri_parser::parse_scheme()
{
  if (_components.find("scheme") != _components.end())
  {
    std::string full_scheme = _components["scheme"];

    if (full_scheme.empty())
      throw Parser_error("Scheme is missing");

    // Does schema parsing based on RFC3986
    BaseTokenizer tok;
    tok.set_allow_spaces(false);

    // RFC3986 Defines the next valid chars: +.-
    // However, in the Specification we only consider alphanumerics and + for extensions
    tok.set_simple_tokens("+");
    tok.set_complex_token("alphanumeric", ALPHANUMERIC);

    tok.process(full_scheme);

    _scheme = tok.consume_token("alphanumeric");

    if (tok.tokens_available())
    {
      tok.consume_token("+");
      _scheme_ext = tok.consume_token("alphanumeric");

      if (tok.tokens_available())
        throw Parser_error((boost::format("Invalid scheme format [%1%], only one extension is supported") % full_scheme).str());
      else
        throw Parser_error((boost::format("Scheme extension [%1%] is not supported") % _scheme_ext).str());
    }

    // Validate on unique supported schema formats
    // In the future we may support additional stuff, like extensions
    if (_scheme != "mysql" && _scheme != "mysqlx")
      throw Parser_error((boost::format("Invalid scheme [%1%], supported schemes include: mysql, mysqlx") % _scheme).str());
  }
}

void Uri_parser::parse_userinfo()
{
  if (_components.find("userinfo") != _components.end())
  {
    std::string userinfo = _components["userinfo"];

    if (!userinfo.empty())
    {
      BaseTokenizer tok;
      tok.set_allow_spaces(false);
      tok.set_complex_token("pct-encoded", { "%", HEXDIG, HEXDIG });
      tok.set_complex_token("unreserved", UNRESERVED);
      tok.set_complex_token("sub-delims", SUBDELIMITERS);
      tok.set_final_token_group("password", ":");

      tok.process(userinfo, _offsets["userinfo"]);

      auto last_token = tok.peek_last_token();
      if (last_token->get_type() == "password")
      {
        if (last_token->get_pos() == 0)
          throw Parser_error("Missing user information");
        else
        {
          _user = userinfo.substr(0, last_token->get_pos());
          _password = tok.peek_last_token()->get_text().substr(1);
          _has_password = true;
        }
      }
      else
        _user = userinfo;
    }
    else
      throw Parser_error("Missing user information");
  }
}

void Uri_parser::parse_host()
{
  if (_components.find("hostinfo") != _components.end())
  {
    std::string& hostinfo = _components["hostinfo"];

    if (!hostinfo.empty())
    {
      BaseTokenizer tok;
      size_t hostoffset = _offsets["hostinfo"];
      tok.set_allow_spaces(false);
      tok.set_simple_tokens(".:");
      tok.set_complex_token("pct-encoded", { "%", HEXDIG, HEXDIG });
      tok.set_complex_token("digits", DIGIT);
      tok.set_complex_token("sub-delims", SUBDELIMITERS);
      tok.set_complex_token("unreserved", UNRESERVED);
      tok.set_final_token_group("rest", "/");
      tok.process(hostinfo, hostoffset);

      // If the rest token is found, it means multiple levels were defined on the
      // path component
      if (tok.peek_last_token()->get_type() == "rest")
        throw Parser_error("Multiple levels defined on the path component");

      if (tok.cur_token_type_is("digits") &&
          tok.next_token_type(".") &&
          tok.next_token_type("digits", 2) &&
          tok.next_token_type(".", 3) &&
          tok.next_token_type("digits", 4) &&
          tok.next_token_type(".", 5) &&
          tok.next_token_type("digits", 6))
      {
        std::string text;
        std::string octect;

        std::vector<std::string> octets;

        octets.push_back(tok.consume_token("digits"));
        for (size_t index = 0; index < 3; index++)
        {
          tok.consume_token(".");
          octets.push_back(tok.consume_token("digits"));
        }

        for (auto octet : octets)
        {
          int value = boost::lexical_cast<int>(octet);
          if (value < 0 || value >255)
            throw Parser_error("Octect value out of bounds, valid range for IPv4 is 0 to 255");
        }
      }
      else
      {
        while (tok.tokens_available() && !tok.cur_token_type_is(":"))
          tok.consume_any_token();
      }

      if (tok.tokens_available())
      {
        _host = hostinfo.substr(0, tok.peek_token().get_pos());

        if (tok.cur_token_type_is(":"))
        {
          hostoffset += _host.length();

          // Offset because of the : in hoat:port
          hostoffset++;

          tok.consume_any_token();

          if (tok.tokens_available())
          {
            if (tok.cur_token_type_is("digits"))
            {
              std::string port = tok.consume_any_token().get_text();
              _port = boost::lexical_cast<int>(port);

              if (_port < 0 || _port > 65536)
                throw Parser_error("Port is out of the valid range: 0 - 65536");

              _has_port = true;

              hostoffset += port.length();

              if (tok.tokens_available())
                throw Parser_error((boost::format("Illegal character after port definition at position %1%") % hostoffset).str());
            }
            else
              throw Parser_error("Invalid port definition");
          }
          else
            throw Parser_error("Missing port number");
        }
        else
          throw Parser_error((boost::format("Unexpected character found at position %1%") % hostoffset).str());
      }
      else
        _host = hostinfo;
    }
    else
      throw Parser_error("Missing host information");
  }
}

void Uri_parser::parse_path()
{
  if (_components.find("path") != _components.end())
  {
    BaseTokenizer tok;
    tok.set_allow_spaces(false);
    tok.set_simple_tokens(":@");
    tok.set_complex_token("pct-encoded", { "%", HEXDIG, HEXDIG });
    tok.set_complex_token("unreserved", UNRESERVED);
    tok.set_complex_token("sub-delims", SUBDELIMITERS);

    tok.process(_components["path"], _offsets["path"]);

    _db = _components["path"];
  }
}

void Uri_parser::parse_query()
{
  if (_components.find("query") != _components.end())
  {
    std::string query = _components["query"];

    parse_attribute(query, _offsets["query"]);
  }
}

void Uri_parser::parse_attribute(const std::string& remaining, size_t offset)
{
  BaseTokenizer tok;
  tok.set_allow_spaces(false);
  tok.set_complex_token("pct-encoded", { "%", HEXDIG, HEXDIG });
  tok.set_complex_token("unreserved", UNRESERVED);
  tok.set_complex_token("sub-delims", SUBDELIMITERS);
  tok.set_final_token_group("pause", "=&");

  // Pauses the processing on the presense of the next tokens
  tok.process(remaining.substr(1), offset);

  auto last_token = tok.peek_last_token();
  if (last_token->get_type() == "pause")
  {
    std::string attribute = remaining.substr(1, last_token->get_pos());
    if (last_token->get_text()[0] == '&')
    {
      _attributes[attribute] = "";
      parse_attribute(remaining.substr(attribute.length() + 1), offset + attribute.length() + 1);
    }
    else
      parse_value(attribute, remaining.substr(attribute.length() + 2), offset + attribute.length() + 2);
  }
  else
    _attributes[remaining] = "";
}

void Uri_parser::parse_value(const std::string& attribute, const std::string& remaining, size_t offset)
{
  BaseTokenizer tok;
  tok.set_complex_token("unreserved", UNRESERVED);

  if (remaining[0] == '(')
  {
    tok.set_simple_tokens("()");
    tok.set_complex_token("delims", DELIMITERS);
    tok.set_final_token_group("next", "&");
    tok.process(remaining, offset);

    tok.consume_token("(");
    while (tok.tokens_available() && !tok.cur_token_type_is(")"))
      tok.consume_any_token();

    tok.consume_token(")");
  }
  else
  {
    tok.set_complex_token("pct-encoded", { "%", HEXDIG, HEXDIG });
    std::string delims = "!$'()*+;=";
    tok.set_complex_token("delims", delims);

    if (remaining[0] == '[')
    {
      tok.set_simple_tokens("[]");
      tok.process(remaining, offset);

      tok.consume_token("[");
      while (tok.tokens_available() && !tok.cur_token_type_is("]"))
        tok.consume_any_token();

      tok.consume_token("]");
    }
    else
    {
      tok.process(remaining, offset);

      while (tok.tokens_available() && !tok.cur_token_type_is("next"))
        tok.consume_any_token();
    }

    // Sets the attributes and starts a new loop
    if (tok.tokens_available())
    {
      _attributes[attribute] = remaining.substr(0, tok.peek_token().get_pos());
      parse_attribute(tok.peek_token().get_text(), offset + tok.peek_token().get_pos());
    }
    else
      _attributes[attribute] = remaining;
  }
}

bool Uri_parser::parse(const std::string& input)
{
  std::string remaining(input);
  // Starts by splitting the major components

  // 1) Trims from the input the scheme component if found
  int offset = 0;
  size_t position = input.find("://");
  if (position != std::string::npos)
  {
    _components["scheme"] = remaining.substr(0, position);
    _offsets["scheme"] = 0;

    // offset for the remaining components
    offset = position + 3;

    remaining = remaining.substr(offset);
  }

  // 2) Trimming the query component
  // We need to identify whether we have a query component
  // which start is marked by a question mark, however a question
  // mark can be part of a password.
  // The @ is allowed as:
  // - Part of the query component
  // - Delimiter between userinfo and hostinfo
  // So if a question mark is found to the right of the last @ symbol
  // we can assume it is the delimiter for the query component
  size_t last_at_position = remaining.rfind('@');
  size_t last_qm_position = remaining.rfind('?');
  if (last_qm_position != std::string::npos)
  {
    // Either there's no userinfo or the question mark is found at the right of @
    // which means is not part of the password
    if (last_at_position == std::string::npos || last_at_position < last_qm_position)
    {
      _offsets["query"] = last_qm_position + offset;

      // Includes the ? symbol to ease further parsing
      _components["query"] = remaining.substr(last_qm_position);

      remaining = remaining.substr(0, last_qm_position);
    }
  }

  // 3) Trims from the remaining string the path component
  // Assumption here is that the last / found on the URI marks the start of the path component
  // NOTE: This will fail in a scenario where there's no path component but the / has been used on the password
  // WORKAROUND: f / is part of the password specify an empty path
  position = remaining.rfind("/");
  if (position != std::string::npos)
  {
    // Strips the / already
    _components["path"] = remaining.substr(position + 1);
    _offsets["path"] = position + offset + 1;

    remaining = remaining.substr(0, position);
  }

  // 4) Trims from the remaining the user info if found
  // rfind is used to allow using @ as part of the password
  position = remaining.rfind("@");
  if (position != std::string::npos)
  {
    _components["userinfo"] = remaining.substr(0, position);
    _offsets["userinfo"] = offset;

    offset += position + 1;

    // excludes the @
    remaining = remaining.substr(position + 1);
  }

  // 5) The rest is host info
  _components["hostinfo"] = remaining;
  _offsets["hostinfo"] = offset;

  parse_scheme();
  parse_userinfo();
  parse_host();
  parse_path();
  parse_query();

  return true;
}