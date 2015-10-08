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
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>

#include <gtest/gtest.h>
#include "../mysqlxtest/common/proj_parser.h"
#include "shellcore/types_cpp.h"
#include "shellcore/common.h"

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

    void parse_and_assert_expr(const std::string& input, const std::string& token_list, const std::string& unparsed, bool document_mode = false, bool allow_alias = true)
    {
      std::stringstream out, out_tokens;
      Proj_parser p(input, document_mode, allow_alias);
      print_tokens(p, out_tokens);
      std::string token_list2 = out_tokens.str();
      ASSERT_TRUE(token_list == token_list2);
      google::protobuf::RepeatedPtrField< ::Mysqlx::Crud::Projection > cols;
      p.parse(cols);
      std::string s = Expr_unparser::column_list_to_string(cols);
      out << s;
      std::string outstr = out.str();

      SCOPED_TRACE("ACTUAL: " + outstr);
      SCOPED_TRACE("EXPECT: " + unparsed);

      ASSERT_TRUE(unparsed == outstr);
    }

    TEST(Proj_parser_tests, x_test)
    {
      parse_and_assert_expr("$a.b[0][0].c**.d.\"a weird\\\"key name\"",
        "[77, 19, 22, 19, 8, 76, 9, 8, 76, 9, 22, 19, 54, 22, 19, 22, 20]", "projection ($.a.b[0][0].c**.d.a weird\"key name as $a.b[0][0].c**.d.\"a weird\\\"key name\")", true);

      parse_and_assert_expr("col1",
        "[19]", "projection (col1)");
      parse_and_assert_expr("a as x1",
        "[19, 56, 19]", "projection (a as x1)");
      EXPECT_ANY_THROW(parse_and_assert_expr("a as x1, col1 as x2, b as x3",
        "[19, 56, 19, 24, 19, 56, 19, 24, 19, 56, 19]", "projection (a as x1, col1 as x2, b as x3)", false, false));
      parse_and_assert_expr("$.x.a[1][0].c",
        "[77, 22, 19, 22, 19, 8, 76, 9, 8, 76, 9, 22, 19]", "projection ($.x.a[1][0].c as $.x.a[1][0].c)", true);
      parse_and_assert_expr("$.x.a[1][0].c as mycol",
        "[77, 22, 19, 22, 19, 8, 76, 9, 8, 76, 9, 22, 19, 56, 19]", "projection ($.x.a[1][0].c as mycol)", true);
      EXPECT_ANY_THROW(parse_and_assert_expr("x.a[1][0].c as mycol, col1 as alias1",
        "[19, 22, 19, 8, 76, 9, 8, 76, 9, 22, 19, 56, 19, 24, 19, 56, 19]", "projection ($.x.a[1][0].c as mycol, $.col1 as alias1)", true));
      EXPECT_ANY_THROW(parse_and_assert_expr("x.a[1][0].c as mycol, col1 as alias1",
        "[19, 22, 19, 8, 76, 9, 8, 76, 9, 22, 19, 56, 19, 24, 19, 56, 19]", "projection ($.x.a[1][0].c as mycol, $.col1 as alias1)", true, false));
      EXPECT_ANY_THROW(parse_and_assert_expr("$foo.bar", "[77, 19, 22, 19]", "projection ($.foo.bar)"));

      parse_and_assert_expr("bar->'$.foo.bar'", "[19, 82, 83, 77, 22, 19, 22, 19, 83]", "projection (bar$.foo.bar)");
      parse_and_assert_expr("bar->'$.foo.bar' as bla", "[19, 82, 83, 77, 22, 19, 22, 19, 83, 56, 19]", "projection (bar$.foo.bar as bla)");
      parse_and_assert_expr("bar->'$.foo.bar' bla", "[19, 82, 83, 77, 22, 19, 22, 19, 83, 19]", "projection (bar$.foo.bar as bla)");
      EXPECT_ANY_THROW(parse_and_assert_expr("bar$foo.bar bla", "[19, 77, 19, 22, 19, 19]", "projection ($.foo.bar)", true));
      parse_and_assert_expr("$.foo.bar", "[77, 22, 19, 22, 19]", "projection ($.foo.bar as $.foo.bar)", true);
      EXPECT_ANY_THROW(parse_and_assert_expr("foo.bar as a.b.c", "[19, 22, 19, 56, 19, 22, 19, 22, 19]", "projection ($.foo.bar as a.b.c)", true));
      parse_and_assert_expr("$.foo.bar as aaa", "[77, 22, 19, 22, 19, 56, 19]", "projection ($.foo.bar as aaa)", true);

      parse_and_assert_expr("$.foo.bar as `a.b.c`", "[77, 22, 19, 22, 19, 56, 19]", "projection ($.foo.bar as a.b.c)", true);
      parse_and_assert_expr("bla", "[19]", "projection ($.bla as bla)", true);
      EXPECT_ANY_THROW(parse_and_assert_expr("bla, ble", "[19, 24, 19]", "projection ($.foo.bar, ble)", true));

      parse_and_assert_expr("$foo.bar", "[77, 19, 22, 19]", "projection ($.foo.bar as $foo.bar)", true);

      // Some negative cases for ->
      EXPECT_ANY_THROW(parse_and_assert_expr("foo->$.foo ", "[19, 82, 77, 22, 19]", "projection ($.foo.bar, ble)", false));
      EXPECT_ANY_THROW(parse_and_assert_expr("foo->$", "[19, 82, 77]", "projection ($.foo.bar, ble)", false));
      EXPECT_ANY_THROW(parse_and_assert_expr("foo->", "[19, 82]", "projection ($.foo.bar, ble)", false));
      EXPECT_ANY_THROW(parse_and_assert_expr("foo->$$", "[19, 82, 77, 77]", "projection ($.foo.bar, ble)", false));
      EXPECT_ANY_THROW(parse_and_assert_expr("->", "[82]", "projection ($.foo.bar, ble)", false));
      EXPECT_ANY_THROW(parse_and_assert_expr("-->'$.foo'", "[37, 82, 83, 77, 22, 19, 83]", "projection ($.foo.bar, ble)", false));
      EXPECT_ANY_THROW(parse_and_assert_expr("->>'$.foo'", "[82, 27, 83, 77, 22, 19, 83]", "projection ($.foo.bar, ble)", false));

      parse_and_assert_expr("concat(foo, ' ', bar)", "[19, 6, 19, 24, 20, 24, 19, 7]", "projection (concat($.foo, \" \", $.bar) as concat(foo, ' ', bar))", true);

      parse_and_assert_expr("a > now() + interval (2 + x) MiNuTe",
        "[19, 27, 19, 6, 7, 36, 16, 6, 76, 36, 19, 7, 46]", "projection ((a > (now() + INTERVAL (2 + x) MiNuTe)))");
      parse_and_assert_expr("a between 1 and 2",
        "[19, 10, 76, 2, 76]", "projection (a BETWEEN 1 AND 2)");
      parse_and_assert_expr("(1 + 3) in (3, 4, 5)",
        "[6, 76, 36, 76, 7, 14, 6, 76, 24, 76, 24, 76, 7]", "projection ((1 + 3) IN (3, 4, 5))");
    }
  };
};