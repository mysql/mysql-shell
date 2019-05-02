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

#include "db/mysqlx/expr_parser.h"
#include "gtest_clean.h"
#include "scripting/types_cpp.h"

using namespace mysqlx;

namespace shcore {
namespace expr_parser_tests {
void print_tokens(const Expr_parser &p, std::stringstream &out) {
  bool first = true;
  out << "[";
  for (std::vector<Token>::const_iterator it = p.begin(); it != p.end(); ++it) {
    if (first)
      first = false;
    else
      out << ", ";
    out << it->get_type();
  }
  out << "]";
}

void parse_and_assert_expr(const std::string &input,
                           const std::string &token_list,
                           const std::string &unparsed,
                           bool document_mode = false,
                           Mysqlx::Expr::Expr **expr = NULL) {
  std::stringstream out, out_tokens;
  std::unique_ptr<Expr_parser> p;
  SCOPED_TRACE(input);
  p.reset(new Expr_parser(input, document_mode));
  print_tokens(*p, out_tokens);
  ASSERT_EQ(token_list, out_tokens.str());
  std::unique_ptr<Mysqlx::Expr::Expr> e;
  e = p->expr();
  std::string s = Expr_unparser::expr_to_string(*(e.get()));
  if (expr != NULL) *expr = e.release();
  out << s;
  EXPECT_EQ(unparsed, out.str());
}

void assert_member_type(Mysqlx::Expr::Expr *expr,
                        Mysqlx::Expr::DocumentPathItem_Type type_expected) {
  EXPECT_TRUE(expr->has_identifier());
  const Mysqlx::Expr::ColumnIdentifier &colid = expr->identifier();
  EXPECT_TRUE(colid.document_path_size() == 1);
  for (int i = 0; i < colid.document_path_size(); i++) {
    const Mysqlx::Expr::DocumentPathItem &item = colid.document_path(i);
    Mysqlx::Expr::DocumentPathItem_Type type = item.type();
    EXPECT_EQ(type, type_expected);
  }
}

TEST(Expr_parser_tests, x_test) {
  parse_and_assert_expr("1", "[76]", "1");
  parse_and_assert_expr("10+1", "[76, 36, 76]", "(10 + 1)");
  parse_and_assert_expr("(abc = 1)", "[6, 19, 25, 76, 7]", "(abc == 1)");
  parse_and_assert_expr("(func(abc)=1)", "[6, 19, 6, 19, 7, 25, 76, 7]",
                        "(func(abc) == 1)");
  parse_and_assert_expr("(abc = \"jess\")", "[6, 19, 25, 20, 7]",
                        "(abc == \"jess\")");
  parse_and_assert_expr("(abc = \"with \\\"\")", "[6, 19, 25, 20, 7]",
                        "(abc == \"with \"\"\")");
  parse_and_assert_expr("(abc != .10)", "[6, 19, 26, 21, 7]",
                        "(abc != 0.100000)");
  parse_and_assert_expr("(abc != \"xyz\")", "[6, 19, 26, 20, 7]",
                        "(abc != \"xyz\")");
  parse_and_assert_expr("a + b * c + d", "[19, 36, 19, 38, 19, 36, 19]",
                        "((a + (b * c)) + d)");
  parse_and_assert_expr("(a + b) * c + d", "[6, 19, 36, 19, 7, 38, 19, 36, 19]",
                        "(((a + b) * c) + d)");
  parse_and_assert_expr(
      "(field not in ('a',func('b', 2.0),'c'))",
      "[6, 19, 1, 14, 6, 20, 24, 19, 6, 20, 24, 21, 7, 24, 20, 7, 7]",
      "NOT ( field IN (\"a\", func(\"b\", 2.000000), \"c\"))");
  parse_and_assert_expr("jess.age between 30 and death",
                        "[19, 22, 19, 10, 76, 2, 19]",
                        "jess.age BETWEEN 30 AND death");
  parse_and_assert_expr("a + b * c + d", "[19, 36, 19, 38, 19, 36, 19]",
                        "((a + (b * c)) + d)");
  parse_and_assert_expr("x > 10 and Y >= -20",
                        "[19, 27, 76, 2, 19, 28, 37, 76]",
                        "((x > 10) && (Y >= -20))");
  parse_and_assert_expr("a->'$.b'", "[19, 82, 83, 77, 22, 19, 83]", "a$.b");
}

TEST(Expr_parser_tests, x_test_cast) {
  parse_and_assert_expr("cast( ( mycol +  1 ) as signed integer )",
                        "[59, 6, 6, 19, 36, 76, 7, 56, 73, 75, 7]",
                        "cast((mycol + 1), \"signed integer\")");
  parse_and_assert_expr("cast( concat( \"Hello\", \" World\" ) as char(11) )",
                        "[59, 6, 19, 6, 20, 24, 20, 7, 56, 67, 6, 76, 7, 7]",
                        "cast(concat(\"Hello\", \" World\"), \"char(11)\")");
  parse_and_assert_expr(
      "cast(doc->'$.foo' as char(30))",
      "[59, 6, 19, 82, 83, 77, 22, 19, 83, 56, 67, 6, 76, 7, 7]",
      "cast(doc$.foo, \"char(30)\")");
  parse_and_assert_expr("cast(1234 as unsigned int)",
                        "[59, 6, 76, 56, 74, 75, 7]",
                        "cast(1234, \"unsigned int\")");
  parse_and_assert_expr("cast(bla as decimal(4,3))",
                        "[59, 6, 19, 56, 72, 6, 76, 24, 76, 7, 7]",
                        "cast(bla, \"decimal(4, 3)\")");
  parse_and_assert_expr(
      "cast( ( now() - interval + 2 day > some_other_time() or cast( ( "
      "something_else IS NOT NULL ) as signed integer )) as int )",
      "[59, 6, 6, 19, 6, 7, 37, 16, 36, 76, 48, 27, 19, 6, 7, 3, 59, 6, 6, 19, "
      "5, 1, 12, 7, 56, 73, 75, 7, 7, 56, 75, 7]",
      "cast((((now() - INTERVAL 2 day) > some_other_time()) || cast(NOT ( "
      "(something_else IS NULL)), \"signed integer\")), \"int\")");
  parse_and_assert_expr(
      "cast( ( a is true and b is null and C + 1 > 40 and cast(mytime = now() "
      "as int ) or hungry()) as char(1) )",
      "[59, 6, 6, 19, 5, 11, 2, 19, 5, 12, 2, 19, 36, 76, 27, 76, 2, 59, 6, "
      "19, 25, 19, 6, 7, 56, 75, 7, 3, 19, 6, 7, 7, 56, 67, 6, 76, 7, 7]",
      "cast((((((a IS TRUE) && (b IS NULL)) && ((C + 1) > 40)) && cast((mytime "
      "== now()), \"int\")) || hungry()), \"char(1)\")");
  parse_and_assert_expr("cast( '1/5/6' as datetime(6))",
                        "[59, 6, 20, 56, 70, 6, 76, 7, 7]",
                        "cast(\"1/5/6\", \"datetime(6)\")");
  parse_and_assert_expr("cast( '1:5' as time(5))",
                        "[59, 6, 20, 56, 71, 6, 76, 7, 7]",
                        "cast(\"1:5\", \"time(5)\")");
  parse_and_assert_expr(
      "cast( concat( \"Hello\", \" World\" ) as char(20) binary charset utf8)",
      "[59, 6, 19, 6, 20, 24, 20, 7, 56, 67, 6, 76, 7, 66, 62, 19, 7]",
      "cast(concat(\"Hello\", \" World\"), \"char(20) binary charset utf8\")");
  parse_and_assert_expr(
      "cast( concat( \"Hello\", \" World\" ) as char(30) binary charset binary "
      ")",
      "[59, 6, 19, 6, 20, 24, 20, 7, 56, 67, 6, 76, 7, 66, 62, 66, 7]",
      "cast(concat(\"Hello\", \" World\"), \"char(30) binary charset "
      "binary\")");
  parse_and_assert_expr("binary mycol +  1", "[66, 19, 36, 76]",
                        "binary((mycol + 1))");
  parse_and_assert_expr(
      "binary cast( ( a is true and b is null and C + 1 > 40 and cast(mytime = "
      "now() as int ) or hungry()) as char(1) )",
      "[66, 59, 6, 6, 19, 5, 11, 2, 19, 5, 12, 2, 19, 36, 76, 27, 76, 2, 59, "
      "6, 19, 25, 19, 6, 7, 56, 75, 7, 3, 19, 6, 7, 7, 56, 67, 6, 76, 7, 7]",
      "binary(cast((((((a IS TRUE) && (b IS NULL)) && ((C + 1) > 40)) && "
      "cast((mytime == now()), \"int\")) || hungry()), \"char(1)\"))");
}

TEST(Expr_parser_tests, x_test_2) {
  parse_and_assert_expr(
      "a is true and b is null and C + 1 > 40 and (mytime = now() or hungry())",
      "[19, 5, 11, 2, 19, 5, 12, 2, 19, 36, 76, 27, 76, 2, 6, 19, 25, 19, 6, "
      "7, 3, 19, 6, 7, 7]",
      "((((a IS TRUE) && (b IS NULL)) && ((C + 1) > 40)) && ((mytime == now()) "
      "|| hungry()))");
  parse_and_assert_expr("a + b + -c > 2", "[19, 36, 19, 36, 37, 19, 27, 76]",
                        "(((a + b) + -c) > 2)");
  parse_and_assert_expr("now () + b + c > 2",
                        "[19, 6, 7, 36, 19, 36, 19, 27, 76]",
                        "(((now() + b) + c) > 2)");
  parse_and_assert_expr(
      "now () - interval +2 day > some_other_time() or something_else IS NOT "
      "NULL",
      "[19, 6, 7, 37, 16, 36, 76, 48, 27, 19, 6, 7, 3, 19, 5, 1, 12]",
      "(((now() - INTERVAL 2 day) > some_other_time()) || NOT ( "
      "(something_else IS NULL)))");
  parse_and_assert_expr("\"two quotes to one\"\"\"", "[20]",
                        "\"two quotes to one\"\"\"");
  parse_and_assert_expr("'two quotes to one'''", "[20]",
                        "\"two quotes to one'\"");
  parse_and_assert_expr("'different quote \"'", "[20]",
                        "\"different quote \"\"\"");
  parse_and_assert_expr("\"different quote '\"", "[20]",
                        "\"different quote '\"");
  parse_and_assert_expr("`ident`", "[19]", "ident");
  parse_and_assert_expr("`ident```", "[19]", "`ident```");
  parse_and_assert_expr("`ident\"'`", "[19]", "`ident\"'`");
  parse_and_assert_expr("? > x and func(?, ?, ?)",
                        "[53, 27, 19, 2, 19, 6, 53, 24, 53, 24, 53, 7]",
                        "((\"?\" > x) && func(\"?\", \"?\", \"?\"))");
}

TEST(Expr_parser_tests, x_test_3) {
  parse_and_assert_expr("a > now() + interval (2 + x) MiNuTe",
                        "[19, 27, 19, 6, 7, 36, 16, 6, 76, 36, 19, 7, 46]",
                        "(a > (now() + INTERVAL (2 + x) MiNuTe))");
  parse_and_assert_expr("a between 1 and 2", "[19, 10, 76, 2, 76]",
                        "a BETWEEN 1 AND 2");
  parse_and_assert_expr("a not between 1 and 2", "[19, 1, 10, 76, 2, 76]",
                        "NOT ( a BETWEEN 1 AND 2)");
  parse_and_assert_expr("a in (1,2,a.b(3),4,5,x)",
                        "[19, 14, 6, 76, 24, 76, 24, 19, 22, 19, 6, 76, 7, 24, "
                        "76, 24, 76, 24, 19, 7]",
                        "a IN (1, 2, a.b(3), 4, 5, x)");
  parse_and_assert_expr(
      "a not in (1,2,3,4,5,x)",
      "[19, 1, 14, 6, 76, 24, 76, 24, 76, 24, 76, 24, 76, 24, 19, 7]",
      "NOT ( a IN (1, 2, 3, 4, 5, x))");
  parse_and_assert_expr("a like b escape c", "[19, 15, 19, 18, 19]",
                        "a LIKE b ESCAPE c");
  parse_and_assert_expr("a not like b escape c", "[19, 1, 15, 19, 18, 19]",
                        "NOT ( a LIKE b ESCAPE c)");
  parse_and_assert_expr("(1 + 3) in (3, 4, 5)",
                        "[6, 76, 36, 76, 7, 14, 6, 76, 24, 76, 24, 76, 7]",
                        "(1 + 3) IN (3, 4, 5)");
  parse_and_assert_expr("`a crazy \"function\"``'name'`(1 + 3) in (3, 4, 5)",
                        "[19, 6, 76, 36, 76, 7, 14, 6, 76, 24, 76, 24, 76, 7]",
                        "`a crazy \"function\"``'name'`((1 + 3)) IN (3, 4, 5)");
  parse_and_assert_expr("`a crazy \"function\"``'name'`(1 + 3) in (3, 4, 5)",
                        "[19, 6, 76, 36, 76, 7, 14, 6, 76, 24, 76, 24, 76, 7]",
                        "`a crazy \"function\"``'name'`((1 + 3)) IN (3, 4, 5)");
  parse_and_assert_expr("a + 314.1592e-2", "[19, 36, 21]", "(a + 3.141592)");
  parse_and_assert_expr("a + 0.0271e+2", "[19, 36, 21]", "(a + 2.710000)");
  parse_and_assert_expr("a + 0.0271e2", "[19, 36, 21]", "(a + 2.710000)");
  EXPECT_ANY_THROW(parse_and_assert_expr("`ident\\``", "", ""));
  parse_and_assert_expr("*", "[38]", "*");
  parse_and_assert_expr("table1.*", "[19, 22, 38]", "table1.*");
  parse_and_assert_expr("schema.table1.*", "[19, 22, 19, 22, 38]",
                        "schema.table1.*");
}

TEST(Expr_parser_tests, x_test_4) {
  parse_and_assert_expr("a->'$.b'", "[19, 82, 83, 77, 22, 19, 83]", "a$.b");
  parse_and_assert_expr("a->'$.b[0][0].c**.d.\"a weird\\\"key name\"'",
                        "[19, 82, 83, 77, 22, 19, 8, 76, 9, 8, 76, 9, 22, 19, "
                        "54, 22, 19, 22, 20, 83]",
                        "a$.b[0][0].c**.d.a weird\"key name");
  parse_and_assert_expr("a->'$.*'", "[19, 82, 83, 77, 22, 38, 83]", "a$.*");
  parse_and_assert_expr("a->'$.foo[*]'",
                        "[19, 82, 83, 77, 22, 19, 8, 38, 9, 83]", "a$.foo[*]");
  parse_and_assert_expr("a->'$.foo[*].bar'",
                        "[19, 82, 83, 77, 22, 19, 8, 38, 9, 22, 19, 83]",
                        "a$.foo[*].bar");
  parse_and_assert_expr("a->'$[0].*'", "[19, 82, 83, 77, 8, 76, 9, 22, 38, 83]",
                        "a$[0].*");
  parse_and_assert_expr("a->'$**[0].*'",
                        "[19, 82, 83, 77, 54, 8, 76, 9, 22, 38, 83]",
                        "a$**[0].*");

  parse_and_assert_expr("bla->'$.foo.bar'",
                        "[19, 82, 83, 77, 22, 19, 22, 19, 83]", "bla$.foo.bar");
  parse_and_assert_expr("bla->'$.\"foo\".bar'",
                        "[19, 82, 83, 77, 22, 20, 22, 19, 83]", "bla$.foo.bar");
  parse_and_assert_expr(".foo", "[22, 19]", "$.foo", true);
  parse_and_assert_expr(".\"foo\"", "[22, 20]", "$.foo", true);
  parse_and_assert_expr(".*", "[22, 38]", "$.*", true);

  Mysqlx::Expr::Expr *expr;
  parse_and_assert_expr(".*", "[22, 38]", "$.*", true, &expr);
  assert_member_type(expr, Mysqlx::Expr::DocumentPathItem_Type_MEMBER_ASTERISK);

  parse_and_assert_expr(".\"*\"", "[22, 20]", "$.*", true, &expr);
  assert_member_type(expr, Mysqlx::Expr::DocumentPathItem_Type_MEMBER);

  // Some regression bugs
  parse_and_assert_expr("colId + .1e-3", "[19, 36, 21]", "(colId + 0.000100)",
                        false);
}

TEST(Expr_parser_tests, x_test_5) {
  // json
  parse_and_assert_expr("{'mykey' : 1, 'myvalue' : \"hello world\" }",
                        "[80, 20, 79, 76, 24, 20, 79, 20, 81]",
                        "{ 'mykey' : 1, 'myvalue' : \"hello world\" }");
  parse_and_assert_expr("{}", "[80, 81]", "{  }");
  // placeholders
  parse_and_assert_expr("name = :1", "[19, 25, 79, 76]", "(name == :0)");
  parse_and_assert_expr(
      ":1 > now() + interval (2 + :x) MiNuTe",
      "[79, 76, 27, 19, 6, 7, 36, 16, 6, 76, 36, 79, 19, 7, 46]",
      "(:0 > (now() + INTERVAL (2 + :1) MiNuTe))");
}

TEST(Expr_parser_tests, arrow_operator) {
  parse_and_assert_expr("c->'$.foo.bar'",
                        "[19, 82, 83, 77, 22, 19, 22, 19, 83]", "c$.foo.bar");
  parse_and_assert_expr("`foo`->'$.bla.bla.bla'",
                        "[19, 82, 83, 77, 22, 19, 22, 19, 22, 19, 83]",
                        "foo$.bla.bla.bla");
  parse_and_assert_expr("doc->'$.\"a b\"'=42",
                        "[19, 82, 83, 77, 22, 20, 83, 25, 76]",
                        "(doc$.a b == 42)");

  EXPECT_ANY_THROW(parse_and_assert_expr("`ident\\``", "", ""));
}

TEST(Expr_parser_tests, x_test_6) {
  parse_and_assert_expr("count(*)", "[19, 6, 38, 7]", "count(*)");
  parse_and_assert_expr("[]", "[8, 9]", "[  ]");
  parse_and_assert_expr("[\"item1\", \"item2\", \"item3\"]",
                        "[8, 20, 24, 20, 24, 20, 9]",
                        "[ \"item1\", \"item2\", \"item3\" ]");

  parse_and_assert_expr(
      "$.geography.Region = :geo and $.geography.SurfaceArea = :area",
      "[77, 22, 19, 22, 19, 25, 79, 19, 2, 77, 22, 19, 22, 19, 25, 79, 19]",
      "(($.geography.Region == :0) && ($.geography.SurfaceArea == :1))", true);
  parse_and_assert_expr(
      "geography.Region = :geo and geography.SurfaceArea = :area",
      "[19, 22, 19, 25, 79, 19, 2, 19, 22, 19, 25, 79, 19]",
      "(($.geography.Region == :0) && ($.geography.SurfaceArea == :1))", true);

  EXPECT_ANY_THROW(parse_and_assert_expr(
      "$.geography.Region = :geo and $.geography.SurfaceArea = :area",
      "[77, 22, 19, 22, 19, 25, 79, 19, 2, 77, 22, 19, 22, 19, 25, 79, 19]",
      "(($.geography.Region == :0) && ($.geography.SurfaceArea == :1))",
      false));
  parse_and_assert_expr(
      "geography.Region = :geo and geography.SurfaceArea = :area",
      "[19, 22, 19, 25, 79, 19, 2, 19, 22, 19, 25, 79, 19]",
      "((geography.Region == :0) && (geography.SurfaceArea == :1))", false);
}

TEST(Expr_parser_tests, regression) {
  // Bug#25754078
  {
    Expr_parser p("1 in 1", true);
    EXPECT_NO_THROW(p.expr());  // no crash = ok
  }
  {
    Expr_parser p("foo[*]", true);
    EXPECT_NO_THROW(p.expr());
  }
}

TEST(Expr_parser_tests, json_in) {
  parse_and_assert_expr("1 in (foo)", "[76, 14, 6, 19, 7]", "1 IN ($.foo)",
                        true);

  parse_and_assert_expr("1 in foo", "[76, 14, 19]", "(1 CONT_IN $.foo)", true);

  parse_and_assert_expr("bar in foo", "[19, 14, 19]", "($.bar CONT_IN $.foo)",
                        true);

  parse_and_assert_expr("1 in [foo]", "[76, 14, 8, 19, 9]",
                        "(1 CONT_IN [ $.foo ])", true);

  parse_and_assert_expr("1 in [1]", "[76, 14, 8, 76, 9]", "(1 CONT_IN [ 1 ])",
                        true);

  parse_and_assert_expr("1 in null", "[76, 14, 12]", "(1 CONT_IN NULL)", true);

  parse_and_assert_expr("1 in {'foo':1}", "[76, 14, 80, 20, 79, 76, 81]",
                        "(1 CONT_IN { 'foo' : 1 })", true);

  parse_and_assert_expr("1 in {\"foo\":[1]}",
                        "[76, 14, 80, 20, 79, 8, 76, 9, 81]",
                        "(1 CONT_IN { 'foo' : [ 1 ] })", true);

  parse_and_assert_expr("1 not in foo", "[76, 1, 14, 19]",
                        "(1 NOT_CONT_IN $.foo)", true);

  parse_and_assert_expr("1 not in [foo]", "[76, 1, 14, 8, 19, 9]",
                        "(1 NOT_CONT_IN [ $.foo ])", true);

  parse_and_assert_expr("1 not in [1]", "[76, 1, 14, 8, 76, 9]",
                        "(1 NOT_CONT_IN [ 1 ])", true);

  parse_and_assert_expr("1 not in null", "[76, 1, 14, 12]",
                        "(1 NOT_CONT_IN NULL)", true);

  parse_and_assert_expr("1 not in {'foo':1}", "[76, 1, 14, 80, 20, 79, 76, 81]",
                        "(1 NOT_CONT_IN { 'foo' : 1 })", true);

  parse_and_assert_expr("1 not in {\"foo\":[1]}",
                        "[76, 1, 14, 80, 20, 79, 8, 76, 9, 81]",
                        "(1 NOT_CONT_IN { 'foo' : [ 1 ] })", true);

  parse_and_assert_expr("1 in bla[*]", "[76, 14, 19, 8, 38, 9]",
                        "(1 CONT_IN $.bla[*])", true);
}

TEST(Expr_parser_tests, keywords_as_doc_fields) {
  // Any keyword should produce a column expression unless it's one of the
  // following exceptions:
  // - Unary Operator: not
  // - Cast Operator: cast
  // - Literals: false, true, null
  // - Binary Function: binary
  // - Interval Definition: interval
  std::set<std::string> incomplete = {"not", "cast", "interval", "binary"};
  std::set<std::string> literals = {"false", "true", "null"};

  for (const auto &kwd : Tokenizer::map.reserved_words) {
    SCOPED_TRACE(kwd.first);
    std::unique_ptr<Expr_parser> p;
    std::stringstream out, out_tokens;

    if (incomplete.find(kwd.first) != incomplete.end()) {
      p.reset(new Expr_parser(kwd.first, true));
      std::unique_ptr<Mysqlx::Expr::Expr> e;
      EXPECT_THROW(e = p->expr(), Parser_error);
    } else {
      p.reset(new Expr_parser(kwd.first, true));
      std::unique_ptr<Mysqlx::Expr::Expr> e;
      EXPECT_NO_THROW(e = p->expr());
      if (e) {
        std::string actual = Expr_unparser::expr_to_string(*(e.get()));

        std::string expected;
        if (literals.find(kwd.first) != literals.end())
          expected = shcore::str_upper(kwd.first);
        else
          expected = "$." + kwd.first;

        EXPECT_STREQ(expected.c_str(), actual.c_str());
      }
    }
  }
}

TEST(Expr_parser_tests, keywords_in_expressions) {
  std::vector<std::tuple<std::string, std::string, std::string, std::string>>
      data{
          std::make_tuple("`in` in :in", "[19, 14, 79, 14]",
                          "($.in CONT_IN :0)", "(in CONT_IN :0)"),
          std::make_tuple("in in :in", "[14, 14, 79, 14]", "($.in CONT_IN :0)",
                          "(in CONT_IN :0)"),
          std::make_tuple("in not in :in", "[14, 1, 14, 79, 14]",
                          "($.in NOT_CONT_IN :0)", "(in NOT_CONT_IN :0)"),
          std::make_tuple("`is` is :is", "[19, 5, 79, 5]", "($.is IS :0)",
                          "(is IS :0)"),
          std::make_tuple("is is :is", "[5, 5, 79, 5]", "($.is IS :0)",
                          "(is IS :0)"),
          std::make_tuple("is is not :is", "[5, 5, 1, 79, 5]",
                          "NOT ( ($.is IS :0))", "NOT ( (is IS :0))"),
          std::make_tuple("`in` like `like`", "[19, 15, 19]",
                          "($.in LIKE $.like)", "(in LIKE like)"),
          std::make_tuple("in like like", "[14, 15, 15]", "($.in LIKE $.like)",
                          "(in LIKE like)"),
          std::make_tuple("in not like like", "[14, 1, 15, 15]",
                          "NOT ( ($.in LIKE $.like))", "NOT ( (in LIKE like))"),
          std::make_tuple("`like` and `and`", "[19, 2, 19]",
                          "($.like && $.and)", "(like && and)"),
          std::make_tuple("like and and", "[15, 2, 2]", "($.like && $.and)",
                          "(like && and)"),
          std::make_tuple("not like and not and", "[1, 15, 2, 1, 2]",
                          "(NOT ( $.like) && NOT ( $.and))",
                          "(NOT ( like) && NOT ( and))"),
          std::make_tuple("`and` or `or`", "[19, 3, 19]", "($.and || $.or)",
                          "(and || or)"),
          std::make_tuple("and or or", "[2, 3, 3]", "($.and || $.or)",
                          "(and || or)"),
          std::make_tuple(
              "`between` between `and` and `or`", "[19, 10, 19, 2, 19]",
              "$.between BETWEEN $.and AND $.or", "between BETWEEN and AND or"),
          std::make_tuple("between between and and or", "[10, 10, 2, 2, 3]",
                          "$.between BETWEEN $.and AND $.or",
                          "between BETWEEN and AND or"),
          std::make_tuple("between between between and between",
                          "[10, 10, 10, 2, 10]",
                          "$.between BETWEEN $.between AND $.between",
                          "between BETWEEN between AND between"),
          std::make_tuple("between not between between and between",
                          "[10, 1, 10, 10, 2, 10]",
                          "NOT ( $.between BETWEEN $.between AND $.between)",
                          "NOT ( between BETWEEN between AND between)"),
          std::make_tuple("`overlaps` overlaps `overlaps`", "[19, 84, 19]",
                          "($.overlaps OVERLAPS $.overlaps)",
                          "(overlaps OVERLAPS overlaps)"),
          std::make_tuple("overlaps overlaps overlaps", "[84, 84, 84]",
                          "($.overlaps OVERLAPS $.overlaps)",
                          "(overlaps OVERLAPS overlaps)"),
          std::make_tuple("`overlaps` not overlaps `overlaps`",
                          "[19, 1, 84, 19]",
                          "NOT ( ($.overlaps OVERLAPS $.overlaps))",
                          "NOT ( (overlaps OVERLAPS overlaps))"),
          std::make_tuple("overlaps not overlaps overlaps", "[84, 1, 84, 84]",
                          "NOT ( ($.overlaps OVERLAPS $.overlaps))",
                          "NOT ( (overlaps OVERLAPS overlaps))"),
          std::make_tuple("`regexp` regexp :regexp", "[19, 17, 79, 17]",
                          "($.regexp REGEXP :0)", "(regexp REGEXP :0)"),
          std::make_tuple("regexp regexp :regexp", "[17, 17, 79, 17]",
                          "($.regexp REGEXP :0)", "(regexp REGEXP :0)"),
          std::make_tuple("regexp not regexp :regexp", "[17, 1, 17, 79, 17]",
                          "NOT ( ($.regexp REGEXP :0))",
                          "NOT ( (regexp REGEXP :0))"),
          std::make_tuple("month(`month`)", "[50, 6, 19, 7]", "month($.month)",
                          "month(month)"),
          std::make_tuple("month(month)", "[50, 6, 50, 7]", "month($.month)",
                          "month(month)"),
      };

  for (const auto &tup : data) {
    SCOPED_TRACE("Document Expressions");
    parse_and_assert_expr(std::get<0>(tup), std::get<1>(tup), std::get<2>(tup),
                          true);
  }

  for (const auto &tup : data) {
    SCOPED_TRACE("Table Expressions");
    parse_and_assert_expr(std::get<0>(tup), std::get<1>(tup), std::get<3>(tup));
  }
}  // namespace expr_parser_tests

// WL12767-TS4_1
TEST(Expr_parser_tests, overlaps_negative) {
  std::vector<std::string> input{"overlaps `field`", "`field` overlaps",
                                 "overlaps overlaps"};

  for (const auto &value : input) {
    std::unique_ptr<Expr_parser> p;
    SCOPED_TRACE(value);
    p.reset(new Expr_parser(value, true));
    std::unique_ptr<Mysqlx::Expr::Expr> e;
    EXPECT_THROW(e = p->expr(), Parser_error);
  }
}

};  // namespace expr_parser_tests
};  // namespace shcore
