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

#ifndef _MYSQLX_PARSER_H_
#define _MYSQLX_PARSER_H_

#include "expr_parser.h"
#include "proj_parser.h"

#include <string>

namespace mysqlx
{
  namespace parser
  {
    std::auto_ptr<Mysqlx::Expr::Expr> parse_collection_filter(const std::string &source)
    {
      Expr_parser parser(source, true);
      return parser.expr();
    }

    std::auto_ptr<Mysqlx::Expr::Expr> parse_table_filter(const std::string &source)
    {
      Expr_parser parser(source);
      return parser.expr();
    }

    std::vector<Mysqlx::Crud::Column*> parse_collection_column_list(const std::string &source)
    {
      Proj_parser parser(source, true, false);
      return parser.projection();
    }

    std::vector<Mysqlx::Crud::Column*> parse_collection_column_list_with_alias(const std::string &source)
    {
      Proj_parser parser(source, true, true);
      return parser.projection();
    }

    std::vector<Mysqlx::Crud::Column*> parse_table_column_list(const std::string &source)
    {
      Proj_parser parser(source, false, false);
      return parser.projection();
    }

    std::vector<Mysqlx::Crud::Column*> parse_table_column_list_with_alias(const std::string &source)
    {
      Proj_parser parser(source, false, true);
      return parser.projection();
    }
  };
};

#endif
