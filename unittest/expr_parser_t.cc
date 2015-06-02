
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
#include "../mysqlxtest/expr_parser.h"
#include "shellcore/types_cpp.h"

#include "mysqlx_datatypes.pb.h"
#include "mysqlx_expr.pb.h"

using namespace mysqlx;

namespace shcore
{
  namespace expr_parser_tests
  {

    void print_tokens(const Expr_parser& p, std::stringstream& out)
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

    void parse_and_assert_expr(const std::string& input, const std::string& token_list, const std::string& unparsed)
    {
      std::stringstream out, out_tokens;
      Expr_parser p(input);
      print_tokens(p, out_tokens);
      ASSERT_TRUE(token_list == out_tokens.str());
      std::auto_ptr<Mysqlx::Expr::Expr> e = p.expr();
      std::string s = Expr_unparser::expr_to_string(*(e.get()));
      out << s;
      ASSERT_TRUE(unparsed == out.str());
    }

    TEST(Expr_parser_tests, x_test)
    {
      parse_and_assert_expr("now () - interval -2 day",
        "[19, 6, 7, 37, 16, 37, 21, 48]", "(now() - INTERVAL -2 day)");
      parse_and_assert_expr("1",
        "[21]", "1");
      parse_and_assert_expr("10+1", 
        "[21, 36, 21]", "(10 + 1)");
      parse_and_assert_expr("(abc = 1)", 
        "[6, 19, 25, 21, 7]", "(abc == 1)");
      parse_and_assert_expr("(func(abc)=1)", 
        "[6, 19, 6, 19, 7, 25, 21, 7]", "(func(abc) == 1)");
      parse_and_assert_expr("(abc = \"jess\")", 
        "[6, 19, 25, 20, 7]", "(abc == \"jess\")");
      parse_and_assert_expr("(abc = \"with \\\"\")", 
        "[6, 19, 25, 20, 7]", "(abc == \"with \"\"\")");
      parse_and_assert_expr("(abc != .10)", 
        "[6, 19, 26, 21, 7]", "(abc != 0.100000)");
      parse_and_assert_expr("(abc != \"xyz\")", 
        "[6, 19, 26, 20, 7]", "(abc != \"xyz\")");
      parse_and_assert_expr("a + b * c + d", 
        "[19, 36, 19, 38, 19, 36, 19]", "((a + (b * c)) + d)");
      parse_and_assert_expr("(a + b) * c + d", 
        "[6, 19, 36, 19, 7, 38, 19, 36, 19]", "(((a + b) * c) + d)");
      parse_and_assert_expr("(field not in ('a',func('b', 2.0),'c'))", 
        "[6, 19, 1, 14, 6, 20, 24, 19, 6, 20, 24, 21, 7, 24, 20, 7, 7]", "NOT ( field IN (\"a\", func(\"b\", 2.000000), \"c\"))");
      parse_and_assert_expr("jess.age between 30 and death", 
        "[19, 22, 19, 10, 21, 2, 19]", "jess.age BETWEEN 30 AND death");
      parse_and_assert_expr("a + b * c + d", 
        "[19, 36, 19, 38, 19, 36, 19]", "((a + (b * c)) + d)");
      parse_and_assert_expr("x > 10 and Y >= -20", 
        "[19, 27, 21, 2, 19, 28, 37, 21]", "((x > 10) && (Y >= -20))");
      parse_and_assert_expr("a is true and b is null and C + 1 > 40 and (time = now() or hungry())", 
        "[19, 5, 11, 2, 19, 5, 12, 2, 19, 36, 21, 27, 21, 2, 6, 19, 25, 19, 6, 7, 3, 19, 6, 7, 7]", "((((a is TRUE) && (b is NULL)) && ((C + 1) > 40)) && ((time == now()) || hungry()))");
      parse_and_assert_expr("a + b + -c > 2", 
        "[19, 36, 19, 36, 37, 19, 27, 21]", "(((a + b) + -c) > 2)");
      parse_and_assert_expr("now () + b + c > 2", 
        "[19, 6, 7, 36, 19, 36, 19, 27, 21]", "(((now() + b) + c) > 2)");
      parse_and_assert_expr("now () + @b + c > 2", 
        "[19, 6, 7, 36, 23, 19, 36, 19, 27, 21]", "(((now() + @b) + c) > 2)");
      parse_and_assert_expr("now () - interval +2 day > some_other_time() or something_else IS NOT NULL", 
        "[19, 6, 7, 37, 16, 36, 21, 48, 27, 19, 6, 7, 3, 19, 5, 1, 12]", "(((now() - INTERVAL 2 day) > some_other_time()) || NOT ( (something_else IS NULL)))");
      parse_and_assert_expr("\"two quotes to one\"\"\"", 
        "[20]", "\"two quotes to one\"\"\"");
      parse_and_assert_expr("'two quotes to one'''", 
        "[20]", "\"two quotes to one'\"");
      parse_and_assert_expr("'different quote \"'", 
        "[20]", "\"different quote \"\"\"");
      parse_and_assert_expr("\"different quote '\"", 
        "[20]", "\"different quote '\"");
      parse_and_assert_expr("`ident`", 
        "[19]", "ident");
      parse_and_assert_expr("`ident```", 
        "[19]", "`ident```");
      parse_and_assert_expr("`ident\"'`", 
        "[19]", "`ident\"'`");
      parse_and_assert_expr("? > x and func(?, ?, ?)", 
        "[53, 27, 19, 2, 19, 6, 53, 24, 53, 24, 53, 7]", "((\"?\" > x) && func(\"?\", \"?\", \"?\"))");
      parse_and_assert_expr("a > now() + interval (2 + x) MiNuTe", 
        "[19, 27, 19, 6, 7, 36, 16, 6, 21, 36, 19, 7, 46]", "(a > (now() + INTERVAL (2 + x) MiNuTe))");
      parse_and_assert_expr("a between 1 and 2", 
        "[19, 10, 21, 2, 21]", "a BETWEEN 1 AND 2");
      parse_and_assert_expr("a not between 1 and 2", 
        "[19, 1, 10, 21, 2, 21]", "NOT ( a BETWEEN 1 AND 2)");
      parse_and_assert_expr("a in (1,2,a.b(3),4,5,x)", 
        "[19, 14, 6, 21, 24, 21, 24, 19, 22, 19, 6, 21, 7, 24, 21, 24, 21, 24, 19, 7]", "a IN (1, 2, b(3), 4, 5, x)");
      parse_and_assert_expr("a not in (1,2,3,4,5,@x)", 
        "[19, 1, 14, 6, 21, 24, 21, 24, 21, 24, 21, 24, 21, 24, 23, 19, 7]", "NOT ( a IN (1, 2, 3, 4, 5, @x))");
      parse_and_assert_expr("a like b escape c", 
        "[19, 15, 19, 18, 19]", "a LIKE b ESCAPE c");
      parse_and_assert_expr("a not like b escape c", 
        "[19, 1, 15, 19, 18, 19]", "NOT ( a LIKE b ESCAPE c)");
      parse_and_assert_expr("(1 + 3) in (3, 4, 5)", 
        "[6, 21, 36, 21, 7, 14, 6, 21, 24, 21, 24, 21, 7]", "(1 + 3) IN (3, 4, 5)");
      parse_and_assert_expr("`a crazy \"function\"``'name'`(1 + 3) in (3, 4, 5)", 
        "[19, 6, 21, 36, 21, 7, 14, 6, 21, 24, 21, 24, 21, 7]", "`a crazy \"function\"``'name'`((1 + 3)) IN (3, 4, 5)");
      parse_and_assert_expr("`a crazy \"function\"``'name'`(1 + 3) in (3, 4, 5)", 
        "[19, 6, 21, 36, 21, 7, 14, 6, 21, 24, 21, 24, 21, 7]", "`a crazy \"function\"``'name'`((1 + 3)) IN (3, 4, 5)");
      parse_and_assert_expr("a@.b", 
        "[19, 23, 22, 19]", "a@.b");
      parse_and_assert_expr("a@.b[0][0].c**.d.\"a weird\\\"key name\"", 
        "[19, 23, 22, 19, 8, 21, 9, 8, 21, 9, 22, 19, 54, 22, 19, 22, 20]", "a@.b[0][0].c**.d.a weird\"key name");
      parse_and_assert_expr("a@.*", 
        "[19, 23, 22, 38]", "a@.*");
      parse_and_assert_expr("a@[0].*", 
        "[19, 23, 8, 21, 9, 22, 38]", "a@[0].*");
      parse_and_assert_expr("a@**[0].*", 
        "[19, 23, 54, 8, 21, 9, 22, 38]", "a@**[0].*");
      parse_and_assert_expr("a + 314.1592e-2", 
        "[19, 36, 21]", "(a + 3.141592)");
      parse_and_assert_expr("a + 0.0271e+2", 
        "[19, 36, 21]", "(a + 2.710000)");
      parse_and_assert_expr("a + 0.0271e2", 
        "[19, 36, 21]", "(a + 2.710000)");
      EXPECT_ANY_THROW(parse_and_assert_expr("`ident\\``", "", ""));
    }
  };
};
