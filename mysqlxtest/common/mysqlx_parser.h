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

#include "proj_parser.h"

#include <string>

namespace mysqlx
{
  namespace parser
  {
    inline std::auto_ptr<Mysqlx::Expr::Expr> parse_collection_filter(const std::string &source)
    {
      Expr_parser parser(source, true);
      return parser.expr();
    }

    inline std::auto_ptr<Mysqlx::Expr::Expr> parse_table_filter(const std::string &source)
    {
      Expr_parser parser(source);
      return parser.expr();
    }

    template<typename Container>
    void parse_collection_column_list(Container &container, const std::string &source)
    {
      Proj_parser parser(source, true, false);
      parser.parse(container);
    }

    template<typename Container>
    void parse_collection_column_list_with_alias(Container &container, const std::string &source)
    {
      Proj_parser parser(source, true, true);
      parser.parse(container);
    }

    template<typename Container>
    void parse_table_column_list(Container &container, const std::string &source)
    {
      Proj_parser parser(source, false, false);
      parser.parse(container);
    }

    template<typename Container>
    void parse_table_column_list_with_alias(Container &container, const std::string &source)
    {
      Proj_parser parser(source, false, true);
      parser.parse(container);
    }
  };
};

#endif
