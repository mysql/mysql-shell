/* Copyright (c) 2014, 2015, Oracle and/or its affiliates. All rights reserved.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; version 2 of the License.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */


#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <memory>
#include <boost/lexical_cast.hpp>

#include <gtest/gtest.h>
#include "../mysqlxtest/common/orderby_parser.h"
#include "shellcore/types_cpp.h"
#include "shellcore/common.h"

#include "mysqlx_datatypes.pb.h"
#include "mysqlx_expr.pb.h"

using namespace mysqlx;

namespace shcore
{
  namespace Orderby_parser_tests
  {
    void print_tokens(const Orderby_parser& p, std::stringstream& out)
    {
      bool first = true;
      out << "[";
      for (std::vector<Token>::const_iterator it = p.begin(); it != p.end(); ++it)
      {
        if (first)
          first = false;
        else
          out << ", ";
        out << it->get_type();
      }
      out << "]";
    }

    void parse_and_assert_expr(const std::string& input, const std::string& token_list, const std::string& unparsed, bool document_mode = false)
    {
      std::stringstream out, out_tokens;
      Orderby_parser p(input, document_mode);
      print_tokens(p, out_tokens);
      std::string token_list2 = out_tokens.str();
      ASSERT_TRUE(token_list == token_list2);
      google::protobuf::RepeatedPtrField< ::Mysqlx::Crud::Order> cols;
      p.parse(cols);
      std::string s = Expr_unparser::order_list_to_string(cols);
      out << s;
      std::string outstr = out.str();
      ASSERT_TRUE(unparsed == outstr);
    }

    TEST(Orderby_parser_tests, simple)
    {
      parse_and_assert_expr("col1 desc", "[19, 58]", "orderby (col1 desc)");
      parse_and_assert_expr("col1 asc", "[19, 57]", "orderby (col1 asc)");
      parse_and_assert_expr("a", "[19]", "orderby (a asc)");
      EXPECT_ANY_THROW(parse_and_assert_expr("a asc, col1 desc, b asc",
        "[19, 57, 24, 19, 58, 24, 19, 57]", "orderby (a asc, col1 desc, b asc)"));
    }
  };
};
