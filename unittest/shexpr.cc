/*
 * Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include "db/mysqlx/expr_parser.h"

#include <google/protobuf/text_format.h>

int main(int argc, char **argv) {
  if (argc != 3) {
    printf("%s tab|col <expr>\n", argv[0]);
    return 1;
  }

  const char *type = argv[1];
  const char *expr = argv[2];

  try {
    mysqlx::Expr_parser p(expr, strcmp(type, "col") == 0);

    Mysqlx::Expr::Expr *expr = p.expr().release();
    std::string out;
    google::protobuf::TextFormat::PrintToString(*expr, &out);
    delete expr;

    std::cout << "OK\n";
    std::cout << out << "\n";
  } catch (mysqlx::Parser_error &e) {
    std::cout << "ERROR\n";
    std::cout << e.what() << "\n";
    // must exit with 0
  } catch (std::exception &e) {
    std::cout << "UNEXPECTED ERROR\n";
    std::cout << e.what() << "\n";
    return 1;
  }

  return 0;
}
