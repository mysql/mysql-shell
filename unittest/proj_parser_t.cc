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

#include "proj_parser.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <memory>
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>

#include <gtest/gtest.h>
#include "../mysqlxtest/common/proj_parser.h"
#include "shellcore/types_cpp.h"

#include "mysqlx_datatypes.pb.h"
#include "mysqlx_expr.pb.h"

using namespace mysqlx;

namespace shcore
{
  namespace proj_parser_tests
  {
    void print_tokens(const Proj_parser& p, std::stringstream& out)
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

	  struct Wrap
	  {
		std::vector<Mysqlx::Crud::Column*> &cols;
		Wrap(std::vector<Mysqlx::Crud::Column*> &c) : cols(c) {}
		Mysqlx::Crud::Column *Add()
		{
		  Mysqlx::Crud::Column *tmp = new Mysqlx::Crud::Column();
		  cols.push_back(tmp);
		  return tmp;
		};
	  };
	  
    void parse_and_assert_expr(const std::string& input, const std::string& token_list, const std::string& unparsed, bool document_mode = false, bool allow_alias = true)
    {

      std::stringstream out, out_tokens;
      Proj_parser p(input, document_mode, allow_alias);
      print_tokens(p, out_tokens);
      std::string token_list2 = out_tokens.str();
      ASSERT_TRUE(token_list == token_list2);
      std::vector<Mysqlx::Crud::Column*> cols;
      Wrap wrapped_cols(cols);
      p.parse(wrapped_cols);
      std::string s = Expr_unparser::column_list_to_string(cols);
      out << s;
      std::string outstr = out.str();
      ASSERT_TRUE(unparsed == outstr);
    }

    TEST(Proj_parser_tests, x_test)
    {
      parse_and_assert_expr("a@.b[0][0].c**.d.\"a weird\\\"key name\"",
        "[19, 23, 22, 19, 8, 21, 9, 8, 21, 9, 22, 19, 54, 22, 19, 22, 20]", "projection (a@.b[0][0].c**.d.a weird\"key name)");
      parse_and_assert_expr("a, col1, b",
        "[19, 24, 19, 24, 19]", "projection (a, col1, b)");
      parse_and_assert_expr("a as x1, col1 as x2, b as x3",
        "[19, 56, 19, 24, 19, 56, 19, 24, 19, 56, 19]", "projection (a as x1, col1 as x2, b as x3)");
      EXPECT_ANY_THROW(parse_and_assert_expr("a as x1, col1 as x2, b as x3",
        "[19, 56, 19, 24, 19, 56, 19, 24, 19, 56, 19]", "projection (a as x1, col1 as x2, b as x3)", false, false));
      parse_and_assert_expr("x.a[1][0].c",
        "[19, 22, 19, 8, 21, 9, 8, 21, 9, 22, 19]", "projection (@.x.a[1][0].c)", true);
      parse_and_assert_expr("x.a[1][0].c as mycol",
        "[19, 22, 19, 8, 21, 9, 8, 21, 9, 22, 19, 56, 19]", "projection (@.x.a[1][0].c as mycol)", true);
      parse_and_assert_expr("x.a[1][0].c as mycol, col1 as alias1",
        "[19, 22, 19, 8, 21, 9, 8, 21, 9, 22, 19, 56, 19, 24, 19, 56, 19]", "projection (@.x.a[1][0].c as mycol, @.col1 as alias1)", true);
      EXPECT_ANY_THROW(parse_and_assert_expr("x.a[1][0].c as mycol, col1 as alias1",
        "[19, 22, 19, 8, 21, 9, 8, 21, 9, 22, 19, 56, 19, 24, 19, 56, 19]", "projection (@.x.a[1][0].c as mycol, @.col1 as alias1)", true, false));
    }
  };
};