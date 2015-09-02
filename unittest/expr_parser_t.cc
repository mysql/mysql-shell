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
#include "../mysqlxtest/common/expr_parser.h"
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

    void parse_and_assert_expr(const std::string& input, const std::string& token_list, const std::string& unparsed, bool document_mode = false, Mysqlx::Expr::Expr** expr = NULL)
    {
      std::stringstream out, out_tokens;
      Expr_parser p(input, document_mode);
      print_tokens(p, out_tokens);
      ASSERT_TRUE(token_list == out_tokens.str());
      std::auto_ptr<Mysqlx::Expr::Expr> e(p.expr());
      std::string s = Expr_unparser::expr_to_string(*(e.get()));
      if (expr != NULL)
        *expr = e.release();
      out << s;
      ASSERT_TRUE(unparsed == out.str());
    }

    void assert_member_type(Mysqlx::Expr::Expr* expr, Mysqlx::Expr::DocumentPathItem_Type type_expected)
    {
      EXPECT_TRUE(expr->has_identifier());
      const Mysqlx::Expr::ColumnIdentifier& colid = expr->identifier();
      EXPECT_TRUE(colid.document_path_size() == 1);
      for (int i = 0; i < colid.document_path_size(); i++)
      {
        const Mysqlx::Expr::DocumentPathItem& item = colid.document_path(i);
        Mysqlx::Expr::DocumentPathItem_Type type = item.type();
        EXPECT_TRUE(type == type_expected);
      }
    }

    TEST(Expr_parser_tests, x_test)
    {
      parse_and_assert_expr("1", "[76]", "1");
      parse_and_assert_expr("10+1",
        "[76, 36, 76]", "(10 + 1)");
      parse_and_assert_expr("(abc = 1)",
        "[6, 19, 25, 76, 7]", "(abc == 1)");
      parse_and_assert_expr("(func(abc)=1)",
        "[6, 19, 6, 19, 7, 25, 76, 7]", "(func(abc) == 1)");
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
                "[19, 22, 19, 10, 76, 2, 19]", "jess.age BETWEEN 30 AND death");
      parse_and_assert_expr("a + b * c + d",
        "[19, 36, 19, 38, 19, 36, 19]", "((a + (b * c)) + d)");
      parse_and_assert_expr("x > 10 and Y >= -20",
        "[19, 27, 76, 2, 19, 28, 37, 76]", "((x > 10) && (Y >= -20))");
      parse_and_assert_expr("a$.b", "[19, 77, 22, 19]", "a$.b");
}

    TEST(Expr_parser_tests, x_test_cast)
    {
      parse_and_assert_expr("cast( ( mycol +  1 ) as signed integer )",
        "[59, 6, 6, 19, 36, 76, 7, 56, 73, 75, 7]", "cast((mycol + 1), \"signed integer\")");
      parse_and_assert_expr("cast( concat( \"Hello\", \" World\" ) as char(11) )",
        "[59, 6, 19, 6, 20, 24, 20, 7, 56, 67, 6, 76, 7, 7]", "cast(concat(\"Hello\", \" World\"), \"char(11)\")");
      parse_and_assert_expr("cast(doc$foo as char(30))", "[59, 6, 19, 77, 19, 56, 67, 6, 76, 7, 7]", "cast(doc$.foo, \"char(30)\")");
      parse_and_assert_expr("cast(1234 as unsigned int)", "[59, 6, 76, 56, 74, 75, 7]", "cast(1234, \"unsigned int\")");
      parse_and_assert_expr("cast(bla as decimal(4,3))", "[59, 6, 19, 56, 72, 6, 76, 24, 76, 7, 7]", "cast(bla, \"decimal(4, 3)\")");
      parse_and_assert_expr("cast( ( now() - interval + 2 day > some_other_time() or cast( ( something_else IS NOT NULL ) as signed integer )) as int )",
        "[59, 6, 6, 19, 6, 7, 37, 16, 36, 76, 48, 27, 19, 6, 7, 3, 59, 6, 6, 19, 5, 1, 12, 7, 56, 73, 75, 7, 7, 56, 75, 7]",
        "cast((((now() - INTERVAL 2 day) > some_other_time()) || cast(NOT ( (something_else IS NULL)), \"signed integer\")), \"int\")");
      parse_and_assert_expr("cast( ( a is true and b is null and C + 1 > 40 and cast(mytime = now() as int ) or hungry()) as char(1) )",
        "[59, 6, 6, 19, 5, 11, 2, 19, 5, 12, 2, 19, 36, 76, 27, 76, 2, 59, 6, 19, 25, 19, 6, 7, 56, 75, 7, 3, 19, 6, 7, 7, 56, 67, 6, 76, 7, 7]",
        "cast((((((a is TRUE) && (b is NULL)) && ((C + 1) > 40)) && cast((mytime == now()), \"int\")) || hungry()), \"char(1)\")");
      parse_and_assert_expr("cast( '1/5/6' as datetime(6))", "[59, 6, 20, 56, 70, 6, 76, 7, 7]", "cast(\"1/5/6\", \"datetime(6)\")");
      parse_and_assert_expr("cast( '1:5' as time(5))", "[59, 6, 20, 56, 71, 6, 76, 7, 7]", "cast(\"1:5\", \"time(5)\")");
      parse_and_assert_expr("cast( concat( \"Hello\", \" World\" ) as char(20) binary charset utf8)",
        "[59, 6, 19, 6, 20, 24, 20, 7, 56, 67, 6, 76, 7, 66, 62, 19, 7]", "cast(concat(\"Hello\", \" World\"), \"char(20) binary charset utf8\")");
      parse_and_assert_expr("cast( concat( \"Hello\", \" World\" ) as char(30) binary charset binary )",
        "[59, 6, 19, 6, 20, 24, 20, 7, 56, 67, 6, 76, 7, 66, 62, 66, 7]", "cast(concat(\"Hello\", \" World\"), \"char(30) binary charset binary\")");
      parse_and_assert_expr("binary mycol +  1", "[66, 19, 36, 76]", "binary((mycol + 1))");
      parse_and_assert_expr("binary cast( ( a is true and b is null and C + 1 > 40 and cast(mytime = now() as int ) or hungry()) as char(1) )",
        "[66, 59, 6, 6, 19, 5, 11, 2, 19, 5, 12, 2, 19, 36, 76, 27, 76, 2, 59, 6, 19, 25, 19, 6, 7, 56, 75, 7, 3, 19, 6, 7, 7, 56, 67, 6, 76, 7, 7]",
        "binary(cast((((((a is TRUE) && (b is NULL)) && ((C + 1) > 40)) && cast((mytime == now()), \"int\")) || hungry()), \"char(1)\"))");
    }

    TEST(Expr_parser_tests, x_test_2)
    {
      parse_and_assert_expr("a is true and b is null and C + 1 > 40 and (mytime = now() or hungry())",
        "[19, 5, 11, 2, 19, 5, 12, 2, 19, 36, 76, 27, 76, 2, 6, 19, 25, 19, 6, 7, 3, 19, 6, 7, 7]", "((((a is TRUE) && (b is NULL)) && ((C + 1) > 40)) && ((mytime == now()) || hungry()))");
      parse_and_assert_expr("a + b + -c > 2",
        "[19, 36, 19, 36, 37, 19, 27, 76]", "(((a + b) + -c) > 2)");
      parse_and_assert_expr("now () + b + c > 2",
        "[19, 6, 7, 36, 19, 36, 19, 27, 76]", "(((now() + b) + c) > 2)");
      parse_and_assert_expr("now () + $b + c > 2",
        "[19, 6, 7, 36, 77, 19, 36, 19, 27, 76]", "(((now() + $b) + c) > 2)");
      parse_and_assert_expr("now () - interval +2 day > some_other_time() or something_else IS NOT NULL",
        "[19, 6, 7, 37, 16, 36, 76, 48, 27, 19, 6, 7, 3, 19, 5, 1, 12]", "(((now() - INTERVAL 2 day) > some_other_time()) || NOT ( (something_else IS NULL)))");
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
    }

    TEST(Expr_parser_tests, x_test_3)
    {
      parse_and_assert_expr("a > now() + interval (2 + x) MiNuTe",
        "[19, 27, 19, 6, 7, 36, 16, 6, 76, 36, 19, 7, 46]", "(a > (now() + INTERVAL (2 + x) MiNuTe))");
      parse_and_assert_expr("a between 1 and 2",
        "[19, 10, 76, 2, 76]", "a BETWEEN 1 AND 2");
      parse_and_assert_expr("a not between 1 and 2",
        "[19, 1, 10, 76, 2, 76]", "NOT ( a BETWEEN 1 AND 2)");
      parse_and_assert_expr("a in (1,2,a.b(3),4,5,x)",
        "[19, 14, 6, 76, 24, 76, 24, 19, 22, 19, 6, 76, 7, 24, 76, 24, 76, 24, 19, 7]", "a IN (1, 2, a.b(3), 4, 5, x)");
      parse_and_assert_expr("a not in (1,2,3,4,5,$x)",
        "[19, 1, 14, 6, 76, 24, 76, 24, 76, 24, 76, 24, 76, 24, 77, 19, 7]", "NOT ( a IN (1, 2, 3, 4, 5, $x))");
      parse_and_assert_expr("a like b escape c",
        "[19, 15, 19, 18, 19]", "a LIKE b ESCAPE c");
      parse_and_assert_expr("a not like b escape c",
        "[19, 1, 15, 19, 18, 19]", "NOT ( a LIKE b ESCAPE c)");
      parse_and_assert_expr("(1 + 3) in (3, 4, 5)",
        "[6, 76, 36, 76, 7, 14, 6, 76, 24, 76, 24, 76, 7]", "(1 + 3) IN (3, 4, 5)");
      parse_and_assert_expr("`a crazy \"function\"``'name'`(1 + 3) in (3, 4, 5)",
        "[19, 6, 76, 36, 76, 7, 14, 6, 76, 24, 76, 24, 76, 7]", "`a crazy \"function\"``'name'`((1 + 3)) IN (3, 4, 5)");
      parse_and_assert_expr("`a crazy \"function\"``'name'`(1 + 3) in (3, 4, 5)",
        "[19, 6, 76, 36, 76, 7, 14, 6, 76, 24, 76, 24, 76, 7]", "`a crazy \"function\"``'name'`((1 + 3)) IN (3, 4, 5)");
      parse_and_assert_expr("a + 314.1592e-2",
        "[19, 36, 21]", "(a + 3.141592)");
      parse_and_assert_expr("a + 0.0271e+2",
        "[19, 36, 21]", "(a + 2.710000)");
      parse_and_assert_expr("a + 0.0271e2",
        "[19, 36, 21]", "(a + 2.710000)");
      EXPECT_ANY_THROW(parse_and_assert_expr("`ident\\``", "", ""));
      parse_and_assert_expr("*", "[38]", "*");
      parse_and_assert_expr("table1.*", "[19, 22, 38]", "table1.*");
      parse_and_assert_expr("schema.table1.*", "[19, 22, 19, 22, 38]", "schema.table1.*");
}

    TEST(Expr_parser_tests, x_test_4)
    {
      parse_and_assert_expr("a$.b",
        "[19, 77, 22, 19]", "a$.b");
      parse_and_assert_expr("a$.b[0][0].c**.d.\"a weird\\\"key name\"",
        "[19, 77, 22, 19, 8, 76, 9, 8, 76, 9, 22, 19, 54, 22, 19, 22, 20]", "a$.b[0][0].c**.d.a weird\"key name");
      parse_and_assert_expr("a$.*",
        "[19, 77, 22, 38]", "a$.*");
      parse_and_assert_expr("a$[0].*",
        "[19, 77, 8, 76, 9, 22, 38]", "a$[0].*");
      parse_and_assert_expr("a$**[0].*",
        "[19, 77, 54, 8, 76, 9, 22, 38]", "a$**[0].*");

      parse_and_assert_expr("bla$.foo.bar", "[19, 77, 22, 19, 22, 19]", "bla$.foo.bar");
      parse_and_assert_expr("bla$.\"foo\".bar", "[19, 77, 22, 20, 22, 19]", "bla$.foo.bar");
      parse_and_assert_expr(".foo", "[22, 19]", "$.foo", true);
      parse_and_assert_expr(".\"foo\"", "[22, 20]", "$.foo", true);
      parse_and_assert_expr(".*", "[22, 38]", "$.*", true);

      Mysqlx::Expr::Expr *expr;
      parse_and_assert_expr(".*", "[22, 38]", "$.*", true, &expr);
      assert_member_type(expr, Mysqlx::Expr::DocumentPathItem_Type_MEMBER_ASTERISK);

      parse_and_assert_expr(".\"*\"", "[22, 20]", "$.*", true, &expr);
      assert_member_type(expr, Mysqlx::Expr::DocumentPathItem_Type_MEMBER);
    }

    TEST(Expr_parser_tests, x_test_5)
    {
      // json
      parse_and_assert_expr("{'mykey' : 1, 'myvalue' : \"hello world\" }",
        "[80, 20, 79, 76, 24, 20, 79, 20, 81]", "{ 'mykey' : 1, 'myvalue' : \"hello world\" }");
      // placeholders
      parse_and_assert_expr("name = :1",
        "[19, 25, 79, 76]", "(name == :0)");
      parse_and_assert_expr(":1 > now() + interval (2 + :x) MiNuTe",
        "[79, 76, 27, 19, 6, 7, 36, 16, 6, 76, 36, 79, 19, 7, 46]", "(:0 > (now() + INTERVAL (2 + :1) MiNuTe))");
    }
  };
};