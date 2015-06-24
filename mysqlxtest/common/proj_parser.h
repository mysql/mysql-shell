
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

#ifndef _EXPR_PARSER_PROJ_H_
#define _EXPR_PARSER_PROJ_H_

#define _EXPR_PARSER_HAS_PROJECTION_KEYWORDS_ 1

#include "expr_parser.h"
#include "mysqlx_crud.pb.h"

#include <memory>

namespace mysqlx
{

class Proj_tokenizer : public Tokenizer
{
public:
  Proj_tokenizer(const std::string& input) : Tokenizer(input) 
  { 
    map.reserved_words["as"] = Token::AS;
  }
};

class Proj_parser
{
public:
  Proj_parser(const std::string& expr_str, bool document_mode = false, bool allow_alias = true);

  template<typename Container>
  void parse(Container &result)
  {
//    Mysqlx::Crud::Projection *proj = result.Add();
//    docpath_member(*proj);

    while (_tokenizer.cur_token_type_is(Token::COMMA))
    {
      _tokenizer.consume_token(Token::COMMA);
//      colid = result.Add();
//      column_identifier(*proj);
    }
  }

  void column_identifier(Mysqlx::Crud::Column &column);
  void docpath_member(Mysqlx::Expr::DocumentPathItem& item);
  void docpath_array_loc(Mysqlx::Expr::DocumentPathItem& item);
  void document_path(Mysqlx::Crud::Column& col);

  std::vector<Token>::const_iterator begin() const { return _tokenizer.begin(); }
  std::vector<Token>::const_iterator end() const { return _tokenizer.end(); }

private:
  Proj_tokenizer _tokenizer;
  bool _document_mode;
  bool _allow_alias;
};

};
#endif
