/*
 * Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include <iostream>
#include <string>
#include <vector>
#include <cstdio>

#include "../mysqlxtest/common/expr_parser.h"

#include "mysqlx_expr.pb.h"
#include <google/protobuf/text_format.h>


int main(int argc, char **argv)
{
  if (argc != 3)
  {
    printf("%s tab|col <expr>\n", argv[0]);
    return 1;
  }

  const char *type = argv[1];
  const char *expr = argv[2];

  try
  {
    mysqlx::Expr_parser p(expr, strcmp(type, "col") == 0);

    Mysqlx::Expr::Expr *expr = p.expr().release();
    std::string out;
    google::protobuf::TextFormat::PrintToString(*expr, &out);
    delete expr;

    std::cout << "OK\n";
    std::cout << out << "\n";
  }
  catch (mysqlx::Parser_error &e)
  {
    std::cout << "ERROR\n";
    std::cout << e.what() << "\n";
    // must exit with 0
  }
  catch (std::exception &e)
  {
    std::cout << "UNEXPECTED ERROR\n";
    std::cout << e.what() << "\n";
    return 1;
  }

  return 0;
}
