/* Copyright (c) 2014, 2019, Oracle and/or its affiliates. All rights reserved.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License, version 2.0,
 as published by the Free Software Foundation.

 This program is also distributed with certain software (including
 but not limited to OpenSSL) that is licensed under separate terms, as
 designated in a particular file or component or in included license
 documentation.  The authors of MySQL hereby grant you an additional
 permission to link the program and your derivative works with the
 separately licensed software that they have included with MySQL.
 This program is distributed in the hope that it will be useful,  but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 the GNU General Public License, version 2.0, for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA */

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "db/mysqlx/mysqlxclient_clean.h"
#include "db/mysqlx/orderby_parser.h"
#include "gtest_clean.h"
#include "scripting/common.h"
#include "scripting/types_cpp.h"

using namespace mysqlx;

namespace shcore {
namespace Orderby_parser_tests {
void print_tokens(const Orderby_parser &p, std::stringstream &out) {
  bool first = true;
  out << "[";
  for (std::vector<Token>::const_iterator it = p.begin(); it != p.end(); ++it) {
    if (first)
      first = false;
    else
      out << ", ";
    out << static_cast<int>(it->get_type());
  }
  out << "]";
}

void parse_and_assert_expr(const std::string &input,
                           const std::string &token_list,
                           const std::string &unparsed,
                           bool document_mode = false) {
  SCOPED_TRACE(input);
  std::stringstream out, out_tokens;
  Orderby_parser p(input, document_mode);
  print_tokens(p, out_tokens);
  ASSERT_EQ(token_list, out_tokens.str());
  google::protobuf::RepeatedPtrField<::Mysqlx::Crud::Order> cols;
  p.parse(cols);
  std::string s = Expr_unparser::order_list_to_string(cols);
  out << s;
  std::string outstr = out.str();
  ASSERT_TRUE(unparsed == outstr);
}

TEST(Orderby_parser_tests, simple) {
  parse_and_assert_expr("col1 desc", "[19, 58]", "orderby (col1 desc)");
  parse_and_assert_expr("col1 asc", "[19, 57]", "orderby (col1 asc)");
  parse_and_assert_expr("a", "[19]", "orderby (a asc)");
  parse_and_assert_expr("`asc` asc", "[19, 57]", "orderby (asc asc)");
  parse_and_assert_expr("asc asc", "[57, 57]", "orderby (asc asc)");
  EXPECT_THROW_MSG(parse_and_assert_expr("a asc, col1 desc, b asc",
                                         "[19, 57, 24, 19, 58, 24, 19, 57]",
                                         "orderby (a asc, col1 desc, b asc)"),
                   Parser_error,
                   "Expected end of expression, at position 5,\n"
                   "in: a asc, col1 desc, b asc\n"
                   "         ^                 ");
}

}  // namespace Orderby_parser_tests
}  // namespace shcore
