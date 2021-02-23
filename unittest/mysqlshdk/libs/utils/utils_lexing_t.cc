/*
 * Copyright (c) 2017, 2021, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/utils/utils_lexing.h"
#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"

namespace mysqlshdk {
namespace utils {

TEST(Utils_lexing, span_quoted_string_dq) {
  EXPECT_EQ(2, span_quoted_string_dq("\"\"", 0));
  EXPECT_EQ(3, span_quoted_string_dq("\"a\"", 0));
  EXPECT_EQ(3, span_quoted_string_dq("\"a\"a", 0));
  EXPECT_EQ(3, span_quoted_string_dq("\"a\"aa", 0));
  EXPECT_EQ(4, span_quoted_string_dq("\"aa\"aa", 0));
  EXPECT_EQ(4, span_quoted_string_dq("\"aa\"a", 0));

  EXPECT_EQ(2, span_quoted_string_dq("\"\"\"", 0));
  EXPECT_EQ(3, span_quoted_string_dq("\"a\"\"", 0));

  EXPECT_EQ(1 + 2, span_quoted_string_dq("\"\"\"", 1));
  EXPECT_EQ(1 + 3, span_quoted_string_dq("\"\"a\"", 1));
  EXPECT_EQ(1 + 3, span_quoted_string_dq("\"\"a\"a", 1));
  EXPECT_EQ(1 + 3, span_quoted_string_dq("\"\"a\"aa", 1));
  EXPECT_EQ(1 + 4, span_quoted_string_dq("\"\"aa\"aa", 1));
  EXPECT_EQ(1 + 4, span_quoted_string_dq("\"\"aa\"a", 1));

  EXPECT_EQ(10, span_quoted_string_dq("\"foo 'bar\"x", 0));
  EXPECT_EQ(11, span_quoted_string_dq("\"foo \\'bar\"x", 0));
  EXPECT_EQ(11, span_quoted_string_dq("\"foo \\\"bar\"x", 0));
  EXPECT_EQ(12, span_quoted_string_dq("\"foo \\\"\\bar\"x", 0));
  EXPECT_EQ(12, span_quoted_string_dq("\"foo \\\"\\bar\"x", 0));

  EXPECT_EQ(6, span_quoted_string_dq("\"foo \"\\bar\"x", 0));

  EXPECT_EQ(9, span_quoted_string_dq(std::string("\"foo\0bar\"x", 10), 0));
  EXPECT_EQ(6, span_quoted_string_dq(std::string("\"\0\0\0\0\"x", 7), 0));

  EXPECT_EQ(std::string::npos, span_quoted_string_dq("\"foo", 0));
  EXPECT_EQ(std::string::npos, span_quoted_string_dq("\"foo\\", 0));

  EXPECT_EQ(std::string::npos, span_quoted_string_dq("\"", 0));
  EXPECT_EQ(std::string::npos, span_quoted_string_dq("\"\\", 0));
  EXPECT_EQ(std::string::npos, span_quoted_string_dq("\"\\\"", 0));
}

TEST(Utils_lexing, span_quoted_string_sq) {
  EXPECT_EQ(2, span_quoted_string_sq("''", 0));
  EXPECT_EQ(3, span_quoted_string_sq("'a'", 0));
  EXPECT_EQ(3, span_quoted_string_sq("'a'a", 0));
  EXPECT_EQ(3, span_quoted_string_sq("'a'aa", 0));
  EXPECT_EQ(4, span_quoted_string_sq("'aa'aa", 0));
  EXPECT_EQ(4, span_quoted_string_sq("'aa'a", 0));

  EXPECT_EQ(2, span_quoted_string_sq("'''", 0));
  EXPECT_EQ(3, span_quoted_string_sq("'a''", 0));

  EXPECT_EQ(1 + 2, span_quoted_string_sq("'''", 1));
  EXPECT_EQ(1 + 3, span_quoted_string_sq("''a'", 1));
  EXPECT_EQ(1 + 3, span_quoted_string_sq("''a'a", 1));
  EXPECT_EQ(1 + 3, span_quoted_string_sq("''a'aa", 1));
  EXPECT_EQ(1 + 4, span_quoted_string_sq("''aa'aa", 1));
  EXPECT_EQ(1 + 4, span_quoted_string_sq("''aa'a", 1));

  EXPECT_EQ(10, span_quoted_string_sq("'foo \"bar'x", 0));
  EXPECT_EQ(11, span_quoted_string_sq("'foo \\'bar'x", 0));
  EXPECT_EQ(11, span_quoted_string_sq("'foo \\'bar'x", 0));
  EXPECT_EQ(12, span_quoted_string_sq("'foo \\'\\bar'x", 0));
  EXPECT_EQ(12, span_quoted_string_sq("'foo \\'\\bar'x", 0));

  EXPECT_EQ(6, span_quoted_string_sq("'foo '\\bar'x", 0));

  EXPECT_EQ(9, span_quoted_string_sq(std::string("'foo\0bar'x", 10), 0));
  EXPECT_EQ(6, span_quoted_string_sq(std::string("'\0\0\0\0'x", 7), 0));
  char arr[] = {'\'',
                '\0',
                static_cast<char>(255),
                static_cast<char>(255),
                static_cast<char>(255),
                '\'',
                'x'};
  EXPECT_EQ(6, span_quoted_string_sq(std::string(arr, 7), 0));

  EXPECT_EQ(std::string::npos, span_quoted_string_sq("'foo", 0));
  EXPECT_EQ(std::string::npos, span_quoted_string_sq("'foo\\", 0));

  EXPECT_EQ(std::string::npos, span_quoted_string_sq("'", 0));
  EXPECT_EQ(std::string::npos, span_quoted_string_sq("'\\", 0));
  EXPECT_EQ(std::string::npos, span_quoted_string_sq("'\\'", 0));
}

TEST(Utils_lexing, span_cstyle_comment_plain) {
  EXPECT_EQ(9, span_cstyle_sql_comment("/* foo */", 0));
  EXPECT_EQ(10, span_cstyle_sql_comment("*/* foo */", 1));
  EXPECT_EQ(7, span_cstyle_sql_comment("/*foo*/", 0));
  EXPECT_EQ(10, span_cstyle_sql_comment("/* foo* */", 0));
  EXPECT_EQ(9, span_cstyle_sql_comment("/* foo */*", 0));
  EXPECT_EQ(10, span_cstyle_sql_comment("/* foo/ */*", 0));
  EXPECT_EQ(4, span_cstyle_sql_comment("/**/", 0));
  EXPECT_EQ(6, span_cstyle_sql_comment("/****/", 0));

  EXPECT_EQ(std::string::npos, span_cstyle_sql_comment("/* foo", 0));
  EXPECT_EQ(std::string::npos, span_cstyle_sql_comment("/*/", 0));

  EXPECT_EQ(6, span_cstyle_sql_comment("/* '*/' */", 0));
  EXPECT_EQ(6, span_cstyle_sql_comment("/* \"*/\" */", 0));
  EXPECT_EQ(6, span_cstyle_sql_comment("/* `*/` */", 0));
  EXPECT_EQ(9, span_cstyle_sql_comment("/* \n-- */ \n */", 0));

  EXPECT_EQ(9, span_cstyle_comment("/* foo */", 0));
  EXPECT_EQ(10, span_cstyle_comment("*/* foo */", 1));
  EXPECT_EQ(7, span_cstyle_comment("/*foo*/", 0));
  EXPECT_EQ(10, span_cstyle_comment("/* foo* */", 0));
  EXPECT_EQ(9, span_cstyle_comment("/* foo */*", 0));
  EXPECT_EQ(10, span_cstyle_comment("/* foo/ */*", 0));
  EXPECT_EQ(4, span_cstyle_comment("/**/", 0));
  EXPECT_EQ(6, span_cstyle_comment("/****/", 0));

  EXPECT_EQ(std::string::npos, span_cstyle_comment("/* foo", 0));
  EXPECT_EQ(std::string::npos, span_cstyle_comment("/*/", 0));

  EXPECT_EQ(6, span_cstyle_comment("/* '*/' */", 0));
  EXPECT_EQ(6, span_cstyle_comment("/* \"*/\" */", 0));
  EXPECT_EQ(6, span_cstyle_comment("/* `*/` */", 0));
  EXPECT_EQ(9, span_cstyle_comment("/* \n-- */\n */", 0));
}

TEST(Utils_lexing, span_cstyle_comment_hint) {
  // Tests hints in the form /*+ ... */, which work just like regular C comments

  EXPECT_EQ(10, span_cstyle_sql_comment("/*+ foo */", 0));
  EXPECT_EQ(11, span_cstyle_sql_comment("*/*+ foo */", 1));
  EXPECT_EQ(8, span_cstyle_sql_comment("/*+foo*/", 0));
  EXPECT_EQ(11, span_cstyle_sql_comment("/*+ foo* */", 0));
  EXPECT_EQ(10, span_cstyle_sql_comment("/*+ foo */*", 0));
  EXPECT_EQ(11, span_cstyle_sql_comment("/*+ foo/ */*", 0));
  EXPECT_EQ(5, span_cstyle_sql_comment("/*+*/", 0));
  EXPECT_EQ(7, span_cstyle_sql_comment("/*+***/", 0));

  EXPECT_EQ(std::string::npos, span_cstyle_sql_comment("/*+ foo", 0));
  EXPECT_EQ(std::string::npos, span_cstyle_sql_comment("/*+/", 0));

  EXPECT_EQ(7, span_cstyle_sql_comment("/*+ '*/' */", 0));
  EXPECT_EQ(7, span_cstyle_sql_comment("/*+ \"*/\" */", 0));
  EXPECT_EQ(7, span_cstyle_sql_comment("/*+ `*/` */", 0));
  EXPECT_EQ(10, span_cstyle_sql_comment("/*+ \n-- */ \n */", 0));

  EXPECT_EQ(10, span_cstyle_comment("/*+ foo */", 0));
  EXPECT_EQ(11, span_cstyle_comment("*/*+ foo */", 1));
  EXPECT_EQ(8, span_cstyle_comment("/*+foo*/", 0));
  EXPECT_EQ(11, span_cstyle_comment("/*+ foo* */", 0));
  EXPECT_EQ(10, span_cstyle_comment("/*+ foo */*", 0));
  EXPECT_EQ(11, span_cstyle_comment("/*+ foo/ */*", 0));
  EXPECT_EQ(5, span_cstyle_comment("/*+*/", 0));
  EXPECT_EQ(7, span_cstyle_comment("/*+***/", 0));

  EXPECT_EQ(std::string::npos, span_cstyle_comment("/*+ foo", 0));
  EXPECT_EQ(std::string::npos, span_cstyle_comment("/*+/", 0));

  EXPECT_EQ(7, span_cstyle_comment("/*+ '*/' */", 0));
  EXPECT_EQ(7, span_cstyle_comment("/*+ \"*/\" */", 0));
  EXPECT_EQ(7, span_cstyle_comment("/*+ `*/` */", 0));
  EXPECT_EQ(10, span_cstyle_comment("/*+ \n-- */\n */", 0));
}

TEST(Utils_lexing, span_cstyle_comment_conditional) {
  EXPECT_EQ(10, span_cstyle_sql_comment("/*! foo */", 0));
  EXPECT_EQ(11, span_cstyle_sql_comment("//*! foo */", 1));
  EXPECT_EQ(8, span_cstyle_sql_comment("/*!foo*/", 0));
  EXPECT_EQ(11, span_cstyle_sql_comment("/*! foo* */", 0));
  EXPECT_EQ(10, span_cstyle_sql_comment("/*! foo */*", 0));
  EXPECT_EQ(11, span_cstyle_sql_comment("/*! foo/ */*", 0));
  EXPECT_EQ(5, span_cstyle_sql_comment("/*!*/", 0));
  EXPECT_EQ(7, span_cstyle_sql_comment("/*!***/", 0));
  EXPECT_EQ(11, span_cstyle_sql_comment("/*! foo/ *//*!12345 */", 0));

  EXPECT_EQ(std::string::npos, span_cstyle_sql_comment("/*! foo", 0));
  EXPECT_EQ(std::string::npos, span_cstyle_sql_comment("/*!/", 0));

  // */ inside a string has to be ignored
  EXPECT_EQ(11, span_cstyle_sql_comment("/*! '*/' */", 0));
  EXPECT_EQ(11, span_cstyle_sql_comment("/*! \"*/\" */", 0));
  EXPECT_EQ(11, span_cstyle_sql_comment("/*! `*/` */", 0));
  EXPECT_EQ(11, span_cstyle_sql_comment("/*! '*/' *//*! */", 0));
  EXPECT_EQ(11, span_cstyle_sql_comment("/*! \"*/\" *//*! */", 0));
  EXPECT_EQ(11, span_cstyle_sql_comment("/*! `*/` *//*! */", 0));

  EXPECT_EQ(std::string::npos, span_cstyle_sql_comment("/*! '*/ */", 0));
  EXPECT_EQ(std::string::npos, span_cstyle_sql_comment("/*! \"*/ */", 0));
  EXPECT_EQ(std::string::npos, span_cstyle_sql_comment("/*! `*/ */", 0));

  // */ inside a line comment has to be ignored
  EXPECT_EQ(14, span_cstyle_sql_comment("/*! \n--  */\n*/", 0));
  EXPECT_EQ(16, span_cstyle_sql_comment("/*! \n--  `*/`\n*/", 0));
  EXPECT_EQ(16, span_cstyle_sql_comment("/*! \n--  '*/'\n*/", 0));
  EXPECT_EQ(16, span_cstyle_sql_comment("/*! \n--  \"*/\"\n*/", 0));

  // not really line comments
  EXPECT_EQ(15, span_cstyle_sql_comment("/*! '\n-- */' */", 0));
  EXPECT_EQ(15, span_cstyle_sql_comment("/*! \"\n-- */\" */", 0));
  EXPECT_EQ(15, span_cstyle_sql_comment("/*! `\n-- */` */", 0));
}

TEST(Utils_lexing, span_sql_identifier) {
  EXPECT_EQ(5, span_quoted_sql_identifier_bt("`foo` bar", 0));
  EXPECT_EQ(2, span_quoted_sql_identifier_bt("`` bar", 0));
  EXPECT_EQ(6, span_quoted_sql_identifier_bt("`f``o` bar", 0));
  EXPECT_EQ(5, span_quoted_sql_identifier_bt("`f``` bar", 0));
  EXPECT_EQ(5, span_quoted_sql_identifier_bt("```f` bar", 0));
  EXPECT_EQ(4, span_quoted_sql_identifier_bt("```` bar", 0));

  EXPECT_EQ(std::string::npos, span_quoted_sql_identifier_bt("`foo", 0));
}

TEST(Utils_lexing, SQL_iterator) {
  std::string ts("# foo * \ns'* '/* foo*  */s\"* \"s-- * \ns");
  SQL_iterator it(ts);
  EXPECT_EQ('s', *(it++));
  EXPECT_EQ('s', *it);
  EXPECT_EQ('s', *(++it));
  EXPECT_EQ('s', *(++it));
  EXPECT_EQ(ts.length(), ++it);
  EXPECT_THROW(++it, std::out_of_range);

  std::string ts1(
      "# foo * \nselect '* '/* foo*  */select \"* \" from -- * \n *");
  SQL_iterator it1(ts1);
  for (std::size_t i = 0; i < 17; ++i) EXPECT_NO_THROW(++it1);
  EXPECT_THROW(++it, std::out_of_range);

  std::string ts2("# test");
  SQL_iterator it2(ts2);
  EXPECT_EQ(ts2.length(), it2);
  EXPECT_THROW(++it2, std::out_of_range);

  std::string ts3("/* foo; bar */a-- 'foo'\nb 'foo\"'`bar'\"z`c d");
  SQL_iterator it3(ts3);

  EXPECT_EQ('a', *(it3++));
  EXPECT_EQ('b', *(it3++));
  EXPECT_EQ(' ', *(it3++));
  EXPECT_EQ('c', *(it3++));
  EXPECT_EQ(' ', *(it3++));
  EXPECT_EQ('d', *(it3++));
  EXPECT_THROW(++it3, std::out_of_range);
}

TEST(Utils_lexing, SQL_string_iterator_get_next_sql_token) {
  std::string sql{
      "/*!50100 create trigger*/# psikus\ngenre_summary_asc AFTER\tINSERT on "
      "movies for "
      "each\nrow INSERT INTO genre_summary (genre, count, time) select genre, "
      "\"qoted\", now() from movies group/* psikus */by genre\nasc;"};
  SQL_iterator it(sql);

  EXPECT_EQ("create", it.get_next_token());
  EXPECT_EQ("trigger", it.get_next_token());
  EXPECT_EQ("genre_summary_asc", it.get_next_token());
  EXPECT_EQ("AFTER", it.get_next_token());
  EXPECT_EQ("INSERT", it.get_next_token());
  EXPECT_EQ("on", it.get_next_token());
  EXPECT_EQ("movies", it.get_next_token());
  EXPECT_EQ("for", it.get_next_token());
  EXPECT_EQ("each", it.get_next_token());
  EXPECT_EQ("row", it.get_next_token());
  EXPECT_EQ("INSERT", it.get_next_token());
  EXPECT_EQ("INTO", it.get_next_token());
  EXPECT_EQ("genre_summary", it.get_next_token());
  EXPECT_EQ("(", it.get_next_token());
  EXPECT_EQ("genre", it.get_next_token());
  EXPECT_EQ(",", it.get_next_token());
  EXPECT_EQ("count", it.get_next_token());
  EXPECT_EQ(",", it.get_next_token());
  EXPECT_EQ("time", it.get_next_token());
  EXPECT_EQ(")", it.get_next_token());
  EXPECT_EQ("select", it.get_next_token());
  EXPECT_EQ("genre", it.get_next_token());
  EXPECT_EQ(",", it.get_next_token());
  EXPECT_EQ(",", it.get_next_token());
  EXPECT_EQ("now", it.get_next_token());
  EXPECT_EQ("(", it.get_next_token());
  EXPECT_EQ(")", it.get_next_token());
  EXPECT_EQ("from", it.get_next_token());
  EXPECT_EQ("movies", it.get_next_token());
  EXPECT_EQ("group", it.get_next_token());
  EXPECT_EQ("by", it.get_next_token());
  EXPECT_EQ("genre", it.get_next_token());
  EXPECT_EQ("asc", it.get_next_token());
  EXPECT_EQ(";", it.get_next_token());
  EXPECT_TRUE(it.get_next_sql_function().empty());
  EXPECT_FALSE(it.valid());

  std::string grant{
      "GRANT `app_delete`@`%`,`app_read`@`%`, `app_write` @ `%` ,`combz`@`%` "
      "TO `combined`\n@\n`%`;"};
  SQL_iterator itg(grant);

  EXPECT_EQ("GRANT", itg.get_next_token());
  EXPECT_EQ("@", itg.get_next_token());
  EXPECT_EQ(",", itg.get_next_token());
  EXPECT_EQ("@", itg.get_next_token());
  EXPECT_EQ(",", itg.get_next_token());
  EXPECT_EQ("@", itg.get_next_token());
  EXPECT_EQ(",", itg.get_next_token());
  EXPECT_EQ("@", itg.get_next_token());
  EXPECT_EQ("TO", itg.get_next_token());
  EXPECT_EQ("@", itg.get_next_token());
  EXPECT_EQ(";", itg.get_next_token());
  EXPECT_TRUE(itg.get_next_sql_function().empty());
  EXPECT_FALSE(itg.valid());

  SQL_iterator git(grant, 0, false);
  EXPECT_EQ("GRANT", git.get_next_token());
  EXPECT_EQ("`app_delete`", git.get_next_token());
  EXPECT_EQ("@", git.get_next_token());
  EXPECT_EQ("`%`", git.get_next_token());
  EXPECT_EQ(",", git.get_next_token());
  EXPECT_EQ("`app_read`", git.get_next_token());
  EXPECT_EQ("@", git.get_next_token());
  EXPECT_EQ("`%`", git.get_next_token());
  EXPECT_EQ(",", git.get_next_token());
  EXPECT_EQ("`app_write`", git.get_next_token());
  EXPECT_EQ("@", git.get_next_token());
  EXPECT_EQ("`%`", git.get_next_token());
  EXPECT_EQ(",", git.get_next_token());
  EXPECT_EQ("`combz`", git.get_next_token());
  EXPECT_EQ("@", git.get_next_token());
  EXPECT_EQ("`%`", git.get_next_token());
  EXPECT_EQ("TO", git.get_next_token());
  EXPECT_EQ("`combined`", git.get_next_token());
  EXPECT_EQ("@", git.get_next_token());
  EXPECT_EQ("`%`", git.get_next_token());
  EXPECT_EQ(";", git.get_next_token());
  EXPECT_TRUE(git.get_next_sql_function().empty());
  EXPECT_FALSE(git.valid());

  std::string ct =
      "CREATE TABLE t(i int) ENCRYPTION = 'N' DATA DIRECTORY = '\tmp', "
      "/*!50100 TABLESPACE `t s 1` */ ENGINE = InnoDB;";
  SQL_iterator cit(ct, 0, false);
  EXPECT_EQ("CREATE", cit.get_next_token());
  EXPECT_EQ("TABLE", cit.get_next_token());
  EXPECT_EQ("t", cit.get_next_token());
  EXPECT_EQ("(", cit.get_next_token());
  EXPECT_EQ("i", cit.get_next_token());
  EXPECT_EQ("int", cit.get_next_token());
  EXPECT_EQ(")", cit.get_next_token());
  EXPECT_EQ("ENCRYPTION", cit.get_next_token());
  EXPECT_EQ("=", cit.get_next_token());
  EXPECT_EQ("'N'", cit.get_next_token());
  EXPECT_EQ("DATA", cit.get_next_token());
  EXPECT_EQ("DIRECTORY", cit.get_next_token());
  EXPECT_EQ("=", cit.get_next_token());
  EXPECT_EQ("'\tmp'", cit.get_next_token());
  EXPECT_EQ(",", cit.get_next_token());
  EXPECT_EQ("TABLESPACE", cit.get_next_token());
  EXPECT_EQ("`t s 1`", cit.get_next_token());
  EXPECT_EQ("ENGINE", cit.get_next_token());
  EXPECT_EQ("=", cit.get_next_token());
  EXPECT_EQ("InnoDB", cit.get_next_token());
  EXPECT_EQ(";", cit.get_next_token());
  EXPECT_TRUE(cit.get_next_sql_function().empty());
  EXPECT_FALSE(cit.valid());

  std::string proc = R"(delimiter //

CREATE procedure yourdatabase.for_loop_example()
wholeblock:BEGIN
  DECLARE x INT;
  DECLARE str VARCHAR(255);
  SET x = -5;
  SET str = '';

  loop_label: LOOP
    IF x > 0 THEN
      LEAVE loop_label;
    END IF;
    SET str = CONCAT(str,x,',');
    SET x = x + 1;
    ITERATE loop_label;
  END LOOP;

  SELECT str;

END//)";
  SQL_iterator pit(proc, 0, false);
  EXPECT_EQ("delimiter", pit.get_next_token());
  EXPECT_EQ("//", pit.get_next_token());
  EXPECT_EQ("CREATE", pit.get_next_token());
  EXPECT_EQ("procedure", pit.get_next_token());
  EXPECT_EQ("yourdatabase.for_loop_example", pit.get_next_token());
  EXPECT_EQ("(", pit.get_next_token());
  EXPECT_EQ(")", pit.get_next_token());
  EXPECT_EQ("wholeblock:", pit.get_next_token());
  EXPECT_EQ("BEGIN", pit.get_next_token());
  EXPECT_EQ("DECLARE", pit.get_next_token());
  EXPECT_EQ("x", pit.get_next_token());
  EXPECT_EQ("INT", pit.get_next_token());
  EXPECT_EQ(";", pit.get_next_token());
  EXPECT_EQ("DECLARE", pit.get_next_token());
  EXPECT_EQ("str", pit.get_next_token());
  EXPECT_EQ("VARCHAR", pit.get_next_token());
  EXPECT_EQ("(", pit.get_next_token());
  EXPECT_EQ("255", pit.get_next_token());
  EXPECT_EQ(")", pit.get_next_token());
  EXPECT_EQ(";", pit.get_next_token());
  EXPECT_EQ("SET", pit.get_next_token());
  EXPECT_EQ("x", pit.get_next_token());
  EXPECT_EQ("=", pit.get_next_token());
  EXPECT_EQ("-5", pit.get_next_token());
  EXPECT_EQ(";", pit.get_next_token());
  EXPECT_EQ("SET", pit.get_next_token());
  EXPECT_EQ("str", pit.get_next_token());
  EXPECT_EQ("=", pit.get_next_token());
  EXPECT_EQ("''", pit.get_next_token());
  EXPECT_EQ(";", pit.get_next_token());
  EXPECT_EQ("loop_label:", pit.get_next_token());
  EXPECT_EQ("LOOP", pit.get_next_token());
  EXPECT_EQ("IF", pit.get_next_token());
  EXPECT_EQ("x", pit.get_next_token());
  EXPECT_EQ(">", pit.get_next_token());
  EXPECT_EQ("0", pit.get_next_token());
  EXPECT_EQ("THEN", pit.get_next_token());
  EXPECT_EQ("LEAVE", pit.get_next_token());
  EXPECT_EQ("loop_label", pit.get_next_token());
  EXPECT_EQ(";", pit.get_next_token());
  EXPECT_EQ("END", pit.get_next_token());
  EXPECT_EQ("IF", pit.get_next_token());
  EXPECT_EQ(";", pit.get_next_token());
  EXPECT_EQ("SET", pit.get_next_token());
  EXPECT_EQ("str", pit.get_next_token());
  EXPECT_EQ("=", pit.get_next_token());
  EXPECT_EQ("CONCAT", pit.get_next_token());
  EXPECT_EQ("(", pit.get_next_token());
  EXPECT_EQ("str", pit.get_next_token());
  EXPECT_EQ(",", pit.get_next_token());
  EXPECT_EQ("x", pit.get_next_token());
  EXPECT_EQ(",", pit.get_next_token());
  EXPECT_EQ("','", pit.get_next_token());
  EXPECT_EQ(")", pit.get_next_token());
  EXPECT_EQ(";", pit.get_next_token());
  EXPECT_EQ("SET", pit.get_next_token());
  EXPECT_EQ("x", pit.get_next_token());
  EXPECT_EQ("=", pit.get_next_token());
  EXPECT_EQ("x", pit.get_next_token());
  EXPECT_EQ("+", pit.get_next_token());
  EXPECT_EQ("1", pit.get_next_token());
  EXPECT_EQ(";", pit.get_next_token());
  EXPECT_EQ("ITERATE", pit.get_next_token());
  EXPECT_EQ("loop_label", pit.get_next_token());
  EXPECT_EQ(";", pit.get_next_token());
  EXPECT_EQ("END", pit.get_next_token());
  EXPECT_EQ("LOOP", pit.get_next_token());
  EXPECT_EQ(";", pit.get_next_token());
  EXPECT_EQ("SELECT", pit.get_next_token());
  EXPECT_EQ("str", pit.get_next_token());
  EXPECT_EQ(";", pit.get_next_token());
  EXPECT_EQ("END//", pit.get_next_token());
  EXPECT_TRUE(pit.get_next_sql_function().empty());
  EXPECT_FALSE(pit.valid());

  std::string select =
      "select'abra', * from information_schema.columns where data_type in "
      "(\"enum\",\"set\") and table_schema not in (\"information_schema\");";

  SQL_iterator sit(select, 0, false);
  EXPECT_EQ("select", sit.get_next_token());
  EXPECT_EQ("'abra'", sit.get_next_token());
  EXPECT_EQ(",", sit.get_next_token());
  EXPECT_EQ("*", sit.get_next_token());
  EXPECT_EQ("from", sit.get_next_token());
  EXPECT_EQ("information_schema.columns", sit.get_next_token());
  EXPECT_EQ("where", sit.get_next_token());
  EXPECT_EQ("data_type", sit.get_next_token());
  EXPECT_EQ("in", sit.get_next_token());
  EXPECT_EQ("(", sit.get_next_token());
  EXPECT_EQ("\"enum\"", sit.get_next_token());
  EXPECT_EQ(",", sit.get_next_token());
  EXPECT_EQ("\"set\"", sit.get_next_token());
  EXPECT_EQ(")", sit.get_next_token());
  EXPECT_EQ("and", sit.get_next_token());
  EXPECT_EQ("table_schema", sit.get_next_token());
  EXPECT_EQ("not", sit.get_next_token());
  EXPECT_EQ("in", sit.get_next_token());
  EXPECT_EQ("(", sit.get_next_token());
  EXPECT_EQ("\"information_schema\"", sit.get_next_token());
  EXPECT_EQ(")", sit.get_next_token());
  EXPECT_EQ(";", sit.get_next_token());
  EXPECT_TRUE(sit.get_next_sql_function().empty());
  EXPECT_FALSE(sit.valid());

  // TODO(alfredo) these are wrong
  // std::string objects = "*, *.*, `foo`.*, `foo`.`bar`, foo.bar, *.foo,
  // foo.*";

  // SQL_iterator oit(objects, 0, false);
  // EXPECT_EQ("*", oit.get_next_token());
  // EXPECT_EQ(",", oit.get_next_token());

  // EXPECT_EQ("*.*", oit.get_next_token());
  // EXPECT_EQ(",", oit.get_next_token());

  // EXPECT_EQ("`foo`.*", oit.get_next_token());
  // EXPECT_EQ(",", oit.get_next_token());

  // EXPECT_EQ("`foo`.`bar`", oit.get_next_token());
  // EXPECT_EQ(",", oit.get_next_token());

  // EXPECT_EQ("foo.bar", oit.get_next_token());
  // EXPECT_EQ(",", oit.get_next_token());

  // EXPECT_EQ("*.foo", oit.get_next_token());
  // EXPECT_EQ(",", oit.get_next_token());

  // EXPECT_EQ("foo.*", oit.get_next_token());

  // TODO(alfredo): lexer is not breaking these correctly
  // std::string sql2("!+foo@localhost foo@'%' (x+y/zzz-q*y&x)");
  // SQL_iterator it2(sql2, 0, false);
  // EXPECT_EQ("!", it2.get_next_token());
  // EXPECT_EQ("+", it2.get_next_token());
  // EXPECT_EQ("foo", it2.get_next_token());
  // EXPECT_EQ("@", it2.get_next_token());
  // EXPECT_EQ("localhost", it2.get_next_token());
  // EXPECT_EQ("foo", it2.get_next_token());
  // EXPECT_EQ("@", it2.get_next_token());
  // EXPECT_EQ("'%'", it2.get_next_token());
  // EXPECT_EQ("(", it2.get_next_token());
  // EXPECT_EQ("x", it2.get_next_token());
  // EXPECT_EQ("+", it2.get_next_token());
  // EXPECT_EQ("y", it2.get_next_token());
  // EXPECT_EQ("/", it2.get_next_token());
  // EXPECT_EQ("zzz", it2.get_next_token());
  // EXPECT_EQ("-", it2.get_next_token());
  // EXPECT_EQ("q", it2.get_next_token());
  // EXPECT_EQ("*", it2.get_next_token());
  // EXPECT_EQ("y", it2.get_next_token());
  // EXPECT_EQ("&", it2.get_next_token());
  // EXPECT_EQ("x", it2.get_next_token());
  // EXPECT_FALSE(it2.valid());
}

TEST(Utils_lexing, SQL_string_iterator_get_next_sql_function) {
  std::string sql(
      "create procedure contains_proc(p1 geometry,p2 geometry) begin select "
      "col1, 'Y()' from tab1 where col2=@p1 and col3=@p2 and contains(p1,p2) "
      "and TOUCHES(p1, p2);\n"
      "-- This is a test NUMGEOMETRIES ()\n"
      "# This is a test GLENGTH()\n"
      "/* just a comment X() */end;");

  SQL_iterator it(sql);
  EXPECT_EQ("contains_proc", it.get_next_sql_function());
  EXPECT_EQ("contains", it.get_next_sql_function());
  EXPECT_EQ("TOUCHES", it.get_next_sql_function());
  EXPECT_TRUE(it.get_next_sql_function().empty());

  std::string sql1(
      "SELECT `AAA_TEST_REMOVED_FUNCTIONS`.`GEOTAB1`.`COL1` AS "
      "`COL1`,`AAA_TEST_REMOVED_FUNCTIONS`.`GEOTAB1`.`COL2` AS "
      "`COL2`,`AAA_TEST_REMOVED_FUNCTIONS`.`GEOTAB1`.`COL3` AS "
      "`COL3`,`AAA_TEST_REMOVED_FUNCTIONS`.`GEOTAB1`.`COL4` AS "
      "`COL4`,ST_TOUCHES(`AAA_TEST_REMOVED_FUNCTIONS`.`GEOTAB1`.`COL2`,`AAA_"
      "TEST_REMOVED_FUNCTIONS`.`GEOTAB1`.`COL3`) AS "
      "`TOUCHES(``COL2``,``COL3``)` FROM "
      "`AAA_TEST_REMOVED_FUNCTIONS`.`GEOTAB1`");

  SQL_iterator it1(sql1);
  EXPECT_EQ("ST_TOUCHES", it1.get_next_sql_function());
  EXPECT_TRUE(it1.get_next_sql_function().empty());
}

}  // namespace utils
}  // namespace mysqlshdk
