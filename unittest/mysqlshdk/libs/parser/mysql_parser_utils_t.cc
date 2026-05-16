/*
 * Copyright (c) 2022, 2026, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <algorithm>
#include <string>

#include "unittest/gtest_clean.h"

#include "mysqlshdk/libs/db/charset.h"
#include "mysqlshdk/libs/parser/ddl/mysql_object_parser.h"
#include "mysqlshdk/libs/parser/mysql_parser_utils.h"

namespace mysqlshdk {
namespace parser {

using Version = mysqlshdk::utils::Version;

namespace {

class Terminal_collector final : public IParser_listener {
 public:
  void on_rule(const AST_rule_node &, bool) override {}

  void on_terminal(const AST_terminal_node &node) override {
    terminals.push_back(node.name);
  }

  void on_error(const AST_error_node &node) override { errors.push_back(node); }

  std::vector<std::string> terminals;
  std::vector<AST_error_node> errors;
};

std::vector<std::string> collect_token_types(std::string_view stmt,
                                             const Parser_config &config) {
  std::vector<std::string> token_types;

  EXPECT_TRUE(tokenize_statement(
      stmt, config,
      [&token_types](std::string_view type, std::string_view /*token*/) {
        token_types.emplace_back(type);
        return true;
      }));

  return token_types;
}

shcore::Dictionary_t parse_ddl_for_test(const std::string &sql,
                                        const std::string &schema = "") {
  const auto context = create_parser_context({Version(8, 0, 27)});
  return parse_ddl_script(context, sql, schema, false);
}

shcore::Dictionary_t get_single_change(const shcore::Dictionary_t &result,
                                       const std::string &operation) {
  const auto changes = result->get_array("changes");
  if (changes->size() != 1) {
    ADD_FAILURE() << "Expected one DDL change, got " << changes->size();
    return {};
  }

  const auto change = changes->at(0).as_map();
  if (!change->has_key(operation)) {
    ADD_FAILURE() << "Expected DDL operation: " << operation;
    return {};
  }

  return change->get_map(operation);
}

}  // namespace

TEST(MysqlParserUtils, check_syntax_ok) {
  EXPECT_NO_THROW(check_sql_syntax(R"*(select 1, '1', "1")*", {}));

  EXPECT_NO_THROW(check_sql_syntax(R"*(
CREATE TABLE reservedTestTable (
      `ROWS` binary(16) NOT NULL,
      `ROW` binary(16) NOT NULL,
      C datetime NOT NULL,
      D varchar(32) NOT NULL,
      E longtext NOT NULL,
      PRIMARY KEY (`ROWS`,`C`),
      KEY `BD_idx` (`ROW`,`D`)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;
)*",
                                   {}));

  EXPECT_NO_THROW(check_sql_syntax(R"*(
DELIMITER $$
select 1$$
select 2$$
create procedure a() begin
select 1;
select 2;
end$$
select 3$$
    )*",
                                   {}));

  EXPECT_NO_THROW(check_sql_syntax(R"*(
DELIMITER $$
CREATE PROCEDURE GetTotalOrder()
BEGIN
  DECLARE `rows` INT DEFAULT 0;
  DECLARE `ROW`  INT DEFAULT 4;

  CREATE TABLE reservedTestTable (
      `ROWS` binary(16) NOT NULL,
      `ROW` binary(16) NOT NULL,
      C datetime NOT NULL,
      D varchar(32) NOT NULL,
      E longtext NOT NULL,
      PRIMARY KEY (`ROWS`,`C`),
      KEY `BD_idx` (`ROW`,`D`)
  ) ENGINE=InnoDB DEFAULT CHARSET=utf8;

  select now();
END$$

DELIMITER ;
)*",
                                   {}));
}

TEST(MysqlParserUtils, syntax_deprecated_but_not_removed) {
  EXPECT_NO_THROW(
      check_sql_syntax("show master status", Parser_config{Version(8, 3, 0)}));
  EXPECT_THROW(
      check_sql_syntax("show master status", Parser_config{Version(8, 4, 0)}),
      mysqlshdk::parser::Sql_syntax_error);

  EXPECT_NO_THROW(check_sql_syntax("change master to master_auto_position=1",
                                   Parser_config{Version(8, 3, 0)}));
  EXPECT_THROW(check_sql_syntax("change master to master_auto_position=1",
                                Parser_config{Version(8, 4, 0)}),
               mysqlshdk::parser::Sql_syntax_error);
}

TEST(MysqlParserUtils, check_syntax_bad) {
  // ROWS is reserved
  EXPECT_THROW(check_sql_syntax(R"*(
CREATE TABLE reservedTestTable (
      ROWS binary(16) NOT NULL,
      ROW binary(16) NOT NULL,
      C datetime NOT NULL,
      D varchar(32) NOT NULL,
      E longtext NOT NULL,
      PRIMARY KEY (ROWS,`C`),
      KEY `BD_idx` (ROW,`D`)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;
)*",
                                {}),
               Sql_syntax_error);

  // ROWS is reserved
  EXPECT_THROW(check_sql_syntax(R"*(
DELIMITER $$
CREATE PROCEDURE GetTotalOrder()
BEGIN
  DECLARE rows INT DEFAULT 0;
  DECLARE ROW  INT DEFAULT 4;

  CREATE TABLE reservedTestTable (
      ROWS binary(16) NOT NULL,
      ROW binary(16) NOT NULL,
      C datetime NOT NULL,
      D varchar(32) NOT NULL,
      E longtext NOT NULL,
      PRIMARY KEY (ROWS,`C`),
      KEY `BD_idx` (ROW,`D`)
  ) ENGINE=InnoDB DEFAULT CHARSET=utf8;

  select now();
END$$

DELIMITER ;
)*",
                                {}),
               Sql_syntax_error);
}

TEST(MysqlParserUtils, respects_ansi_quotes_mode) {
  Parser_config ansi_quotes_on;
  ansi_quotes_on.ansi_quotes = true;

  EXPECT_NO_THROW(check_sql_syntax(R"*(select "a")*", {}));
  EXPECT_THROW(check_sql_syntax(R"*(select "a" from "t")*", {}),
               Sql_syntax_error);
  EXPECT_NO_THROW(check_sql_syntax(R"*(select "a" from "t")*", ansi_quotes_on));
}

TEST(MysqlParserUtils, respects_no_backslash_escapes_mode) {
  Parser_config no_backslash_escapes_on;
  no_backslash_escapes_on.no_backslash_escapes = true;

  EXPECT_NO_THROW(check_sql_syntax(R"(select 'a\'b')", {}));
  EXPECT_THROW(check_sql_syntax(R"(select 'a\'b')", no_backslash_escapes_on),
               Sql_syntax_error);
  EXPECT_NO_THROW(
      check_sql_syntax(R"(select 'a''b')", no_backslash_escapes_on));
}

TEST(MysqlParserUtils, uses_string_terminals_when_ansi_quotes_is_disabled) {
  Terminal_collector collector;

  EXPECT_NO_THROW(traverse_statement_ast(R"*(select "a")*", {}, &collector));
  EXPECT_TRUE(collector.errors.empty());
  EXPECT_NE(std::find(collector.terminals.begin(), collector.terminals.end(),
                      "SINGLE_QUOTED_TEXT"),
            collector.terminals.end());
  EXPECT_EQ(std::find(collector.terminals.begin(), collector.terminals.end(),
                      "DOUBLE_QUOTED_TEXT"),
            collector.terminals.end());
}

TEST(MysqlParserUtils, uses_quoted_identifier_terminals_with_ansi_quotes) {
  Terminal_collector collector;
  Parser_config ansi_quotes_on;
  ansi_quotes_on.ansi_quotes = true;

  EXPECT_NO_THROW(traverse_statement_ast(R"*(select "a" from "t")*",
                                         ansi_quotes_on, &collector));
  EXPECT_TRUE(collector.errors.empty());
  EXPECT_NE(std::find(collector.terminals.begin(), collector.terminals.end(),
                      "BACK_TICK_QUOTED_ID"),
            collector.terminals.end());
  EXPECT_EQ(std::find(collector.terminals.begin(), collector.terminals.end(),
                      "SINGLE_QUOTED_TEXT"),
            collector.terminals.end());
}

TEST(MysqlParserUtils, tokenizes_double_quotes_according_to_ansi_quotes_mode) {
  Parser_config ansi_quotes_on;
  ansi_quotes_on.ansi_quotes = true;

  const auto text_tokens = collect_token_types(R"*(select "a")*", {});
  EXPECT_NE(
      std::find(text_tokens.begin(), text_tokens.end(), "SINGLE_QUOTED_TEXT"),
      text_tokens.end());
  EXPECT_EQ(
      std::find(text_tokens.begin(), text_tokens.end(), "BACK_TICK_QUOTED_ID"),
      text_tokens.end());

  const auto identifier_tokens =
      collect_token_types(R"*(select "a")*", ansi_quotes_on);
  EXPECT_NE(std::find(identifier_tokens.begin(), identifier_tokens.end(),
                      "BACK_TICK_QUOTED_ID"),
            identifier_tokens.end());
  EXPECT_EQ(std::find(identifier_tokens.begin(), identifier_tokens.end(),
                      "SINGLE_QUOTED_TEXT"),
            identifier_tokens.end());
}

TEST(MysqlParserUtils, tokenizes_charset_introducers) {
  const Parser_config config{Version(8, 0, 27)};

  for (const auto introducer : {"_utf8", "_latin1", "_binary"}) {
    SCOPED_TRACE(introducer);
    const auto tokens = collect_token_types(
        std::string("SELECT ") + introducer + " 'active'", config);

    EXPECT_NE(std::find(tokens.begin(), tokens.end(), "UNDERSCORE_CHARSET"),
              tokens.end());
  }
}

TEST(MysqlParserUtils, parse_ddl_charset_introducers) {
  const auto charsets = mysqlshdk::db::charset::charset_names();
  ASSERT_FALSE(charsets.empty());

  for (const auto charset : charsets) {
    const auto introducer = "_" + std::string{charset};
    const auto sql = "CREATE VIEW v AS SELECT IF(1, " + introducer +
                     "'active', " + introducer + "'')";
    SCOPED_TRACE(sql);

    const auto create = get_single_change(parse_ddl_for_test(sql), "create");
    ASSERT_TRUE(create);
    EXPECT_EQ("view", create->get_string("_type"));
    EXPECT_EQ("SELECT IF(1, " + introducer + "'active', " + introducer + "'')",
              create->get_string("sqlBody"));
  }
}

TEST(MysqlParserUtils, parse_type_definition) {
  Data_type_definition definition;

  ASSERT_TRUE(parse_type_definition("DECIMAL(10,2) UNSIGNED", &definition));

  EXPECT_EQ("DECIMAL", definition.type);
  ASSERT_TRUE(definition.precision);
  ASSERT_TRUE(definition.scale);
  EXPECT_EQ(10, *definition.precision);
  EXPECT_EQ(2, *definition.scale);
  EXPECT_FALSE(definition.length);
  EXPECT_TRUE(definition.explicit_params.empty());
  ASSERT_EQ(1, definition.flags.size());
  EXPECT_EQ("UNSIGNED", definition.flags[0]);
}

TEST(MysqlParserUtils, parse_ddl_qualified_creates_use_schema_identity) {
  struct Test_case {
    const char *sql;
    const char *type;
    const char *name;
  };

  const Test_case test_cases[] = {
      {"CREATE TABLE s.t (id INT)", "table", "t"},
      {"CREATE FUNCTION s.f() RETURNS INT RETURN 1", "function", "f"},
      {"CREATE VIEW s.v AS SELECT 1", "view", "v"},
      {"CREATE EVENT s.e ON SCHEDULE EVERY 1 DAY DO SELECT 1", "event", "e"},
  };

  for (const auto &test_case : test_cases) {
    SCOPED_TRACE(test_case.sql);

    const auto create =
        get_single_change(parse_ddl_for_test(test_case.sql), "create");
    ASSERT_TRUE(create);

    EXPECT_EQ(test_case.type, create->get_string("_type"));
    EXPECT_EQ("s", create->get_string("_schema"));
    EXPECT_EQ(test_case.name, create->get_string("name"));
  }
}

TEST(MysqlParserUtils, parse_statement_grant_user_targets) {
  const auto context = create_parser_context({Version(8, 0, 27)});

  const auto grant = parse_statement(context, "GRANT SELECT ON s.t TO 'u'@'h'");
  const auto users = grant->get_map("users");
  ASSERT_TRUE(users);

  const auto user = users->get_map("'u'@'h'");
  ASSERT_TRUE(user);
  EXPECT_EQ("u", user->get_string("user"));
  EXPECT_EQ("h", user->get_string("host"));

  const auto proxy =
      parse_statement(context, "GRANT PROXY ON 'proxied'@'h1' TO u@h2");
  const auto proxy_user = proxy->get_map("proxyUser");
  ASSERT_TRUE(proxy_user);
  EXPECT_EQ("proxied", proxy_user->get_string("user"));
  EXPECT_EQ("h1", proxy_user->get_string("host"));
}

TEST(MysqlParserUtils, parse_ddl_javascript_routine_options) {
  const auto context = create_parser_context({Version(9, 2, 0)});

  const auto create = get_single_change(
      parse_ddl_script(context,
                       "CREATE PROCEDURE p() LANGUAGE JAVASCRIPT "
                       "USING (s.lib AS mylib, lib2) AS $$ $$",
                       "current", false),
      "create");
  ASSERT_TRUE(create);

  EXPECT_EQ("procedure", create->get_string("_type"));
  EXPECT_EQ("JAVASCRIPT", create->get_string("language"));

  const auto libraries = create->get_array("libraries");
  ASSERT_TRUE(libraries);
  ASSERT_EQ(2, libraries->size());

  const auto first = libraries->at(0).as_map();
  EXPECT_EQ("s", first->get_string("_schema"));
  EXPECT_EQ("lib", first->get_string("name"));
  EXPECT_EQ("mylib", first->get_string("alias"));

  const auto second = libraries->at(1).as_map();
  EXPECT_EQ("current", second->get_string("_schema"));
  EXPECT_EQ("lib2", second->get_string("name"));
  EXPECT_FALSE(second->has_key("alias"));

  const auto alter = get_single_change(
      parse_ddl_script(context, "ALTER PROCEDURE p LANGUAGE JAVASCRIPT",
                       "current", false),
      "alter");
  ASSERT_TRUE(alter);

  EXPECT_EQ("procedure", alter->get_string("_type"));
  EXPECT_EQ("JAVASCRIPT", alter->get_string("language"));
}

TEST(MysqlParserUtils, parse_ddl_table_partitioning_alter) {
  const auto alter_partition = get_single_change(
      parse_ddl_for_test("ALTER TABLE s.t PARTITION BY HASH(id) PARTITIONS 4"),
      "alter");
  ASSERT_TRUE(alter_partition);

  EXPECT_EQ("table", alter_partition->get_string("_type"));
  EXPECT_EQ("s", alter_partition->get_string("_schema"));
  EXPECT_EQ("t", alter_partition->get_string("_name"));
  EXPECT_EQ("PARTITION BY HASH(id) PARTITIONS 4",
            alter_partition->get_string("partitionClauseSql"));

  const auto add_partition = get_single_change(
      parse_ddl_for_test("ALTER TABLE s.t ADD PARTITION PARTITIONS 2"),
      "alter");
  ASSERT_TRUE(add_partition);

  EXPECT_EQ("table", add_partition->get_string("_type"));
  EXPECT_EQ("s", add_partition->get_string("_schema"));
  EXPECT_EQ("t", add_partition->get_string("_name"));
  EXPECT_EQ("ADD PARTITION PARTITIONS 2",
            add_partition->get_string("partitionOperationSql"));

  const auto remove_partitioning = get_single_change(
      parse_ddl_for_test("ALTER TABLE s.t REMOVE PARTITIONING"), "alter");
  ASSERT_TRUE(remove_partitioning);

  EXPECT_EQ("table", remove_partitioning->get_string("_type"));
  EXPECT_EQ("s", remove_partitioning->get_string("_schema"));
  EXPECT_EQ("t", remove_partitioning->get_string("_name"));
  EXPECT_TRUE(remove_partitioning->is_null("partitionClauseSql"));
}

TEST(MysqlParserUtils, parse_ddl_routine_libraries_use_routine_schema) {
  const auto context = create_parser_context({Version(9, 2, 0)});

  const auto create = get_single_change(
      parse_ddl_script(context,
                       "CREATE PROCEDURE s.p() LANGUAGE JAVASCRIPT "
                       "USING (lib) AS ''",
                       "current", false),
      "create");
  ASSERT_TRUE(create);

  const auto libraries = create->get_array("libraries");
  ASSERT_TRUE(libraries);
  ASSERT_EQ(1, libraries->size());

  EXPECT_EQ("s", libraries->at(0).as_map()->get_string("_schema"));
  EXPECT_EQ("lib", libraries->at(0).as_map()->get_string("name"));
}

TEST(MysqlParserUtils, parse_ddl_create_and_drop_library) {
  const auto context = create_parser_context({Version(9, 2, 0)});

  const auto create = get_single_change(
      parse_ddl_script(context,
                       "CREATE LIBRARY s.lib LANGUAGE JAVASCRIPT AS $$ $$", "",
                       false),
      "create");
  ASSERT_TRUE(create);

  EXPECT_EQ("library", create->get_string("_type"));
  EXPECT_EQ("s", create->get_string("_schema"));
  EXPECT_EQ("lib", create->get_string("name"));
  EXPECT_EQ("JAVASCRIPT", create->get_string("language"));
  EXPECT_EQ("AS $$ $$", create->get_string("sqlBody"));

  const auto drop = get_single_change(
      parse_ddl_script(context, "DROP LIBRARY s.lib", "", false), "drop");
  ASSERT_TRUE(drop);

  EXPECT_EQ("library", drop->get_string("_type"));
  EXPECT_EQ("s", drop->get_string("_schema"));
  EXPECT_EQ("lib", drop->get_string("_name"));

  EXPECT_THROW(
      parse_ddl_script(context, "DROP LIBRARY s.lib garbage", "", false),
      shcore::Exception);
  EXPECT_THROW(
      parse_ddl_script(context, "DROP LIBRARY s.lib, other", "", false),
      shcore::Exception);
}

TEST(MysqlParserUtils, parse_ddl_drop_library_requires_supported_version) {
  const auto context = create_parser_context({Version(8, 0, 27)});

  EXPECT_THROW(parse_ddl_script(context, "DROP LIBRARY s.lib", "", false),
               shcore::Exception);
}

TEST(MysqlParserUtils, parse_ddl_version_predicate_error_is_descriptive) {
  const auto context = create_parser_context({Version(8, 0, 13)});

  EXPECT_THROW_MSG_CONTAINS(
      parse_ddl_script(context, "ALTER UNDO TABLESPACE uts SET ACTIVE", "",
                       false),
      shcore::Exception, "for this server version");
}

TEST(MysqlParserUtils, parse_ddl_reports_unclassified_statements) {
  const auto context = create_parser_context({Version(8, 0, 27)});
  const auto result =
      parse_ddl_script(context, "NOT A SQL STATEMENT", "", true);

  const auto errors = result->get_array("errors");
  ASSERT_TRUE(errors);
  ASSERT_EQ(1, errors->size());
  EXPECT_NE(std::string::npos,
            errors->at(0).get_string().find("Cannot determine query type"));
}

TEST(MysqlParserUtils, parse_ddl_json_duality_view) {
  const auto context = create_parser_context({Version(9, 7, 0)});

  const auto create = get_single_change(
      parse_ddl_script(context, "CREATE JSON DUALITY VIEW s.jv AS SELECT 1", "",
                       false),
      "create");
  ASSERT_TRUE(create);

  EXPECT_EQ("view", create->get_string("_type"));
  EXPECT_EQ("s", create->get_string("_schema"));
  EXPECT_EQ("jv", create->get_string("name"));
  EXPECT_EQ("JSON_DUALITY", create->get_string("viewType"));
  EXPECT_EQ("SELECT 1", create->get_string("sqlBody"));

  const auto definer_create = get_single_change(
      parse_ddl_script(context,
                       "CREATE DEFINER = CURRENT_USER JSON DUALITY VIEW jv2 "
                       "AS SELECT 1",
                       "current", false),
      "create");
  ASSERT_TRUE(definer_create);

  EXPECT_EQ("view", definer_create->get_string("_type"));
  EXPECT_EQ("current", definer_create->get_string("_schema"));
  EXPECT_EQ("jv2", definer_create->get_string("name"));
  EXPECT_EQ("JSON_DUALITY", definer_create->get_string("viewType"));

  const auto definer_security_create = get_single_change(
      parse_ddl_script(
          context,
          "CREATE DEFINER = CURRENT_USER SQL SECURITY INVOKER JSON DUALITY "
          "VIEW jv3 AS SELECT 1",
          "current", false),
      "create");
  ASSERT_TRUE(definer_security_create);

  EXPECT_EQ("view", definer_security_create->get_string("_type"));
  EXPECT_EQ("current", definer_security_create->get_string("_schema"));
  EXPECT_EQ("jv3", definer_security_create->get_string("name"));
  EXPECT_EQ("JSON_DUALITY", definer_security_create->get_string("viewType"));

  const auto alter = get_single_change(
      parse_ddl_script(context,
                       "ALTER JSON RELATIONAL DUALITY VIEW s.jv AS SELECT 2",
                       "", false),
      "alter");
  ASSERT_TRUE(alter);

  EXPECT_EQ("view", alter->get_string("_type"));
  EXPECT_EQ("s", alter->get_string("_schema"));
  EXPECT_EQ("jv", alter->get_string("_name"));
  EXPECT_EQ("JSON_DUALITY", alter->get_string("viewType"));
  EXPECT_EQ("SELECT 2", alter->get_string("sqlBody"));

  const auto definer_alter = get_single_change(
      parse_ddl_script(context,
                       "ALTER DEFINER = CURRENT_USER JSON DUALITY VIEW s.jv "
                       "AS SELECT 3",
                       "", false),
      "alter");
  ASSERT_TRUE(definer_alter);

  EXPECT_EQ("view", definer_alter->get_string("_type"));
  EXPECT_EQ("s", definer_alter->get_string("_schema"));
  EXPECT_EQ("jv", definer_alter->get_string("_name"));
  EXPECT_EQ("JSON_DUALITY", definer_alter->get_string("viewType"));
  EXPECT_EQ("SELECT 3", definer_alter->get_string("sqlBody"));

  const auto definer_security_alter = get_single_change(
      parse_ddl_script(
          context,
          "ALTER DEFINER = CURRENT_USER SQL SECURITY DEFINER JSON DUALITY "
          "VIEW s.jv AS SELECT 4",
          "", false),
      "alter");
  ASSERT_TRUE(definer_security_alter);

  EXPECT_EQ("view", definer_security_alter->get_string("_type"));
  EXPECT_EQ("s", definer_security_alter->get_string("_schema"));
  EXPECT_EQ("jv", definer_security_alter->get_string("_name"));
  EXPECT_EQ("JSON_DUALITY", definer_security_alter->get_string("viewType"));
  EXPECT_EQ("SELECT 4", definer_security_alter->get_string("sqlBody"));

  EXPECT_THROW(parse_ddl_script(context,
                                "CREATE JSON DUALITY VIEW s.jv garbage AS "
                                "SELECT 1",
                                "", false),
               shcore::Exception);
  EXPECT_THROW(parse_ddl_script(context,
                                "ALTER JSON DUALITY VIEW s.jv garbage AS "
                                "SELECT 1",
                                "", false),
               shcore::Exception);
}

TEST(MysqlParserUtils, parse_ddl_create_drop_roles) {
  const auto result =
      parse_ddl_for_test("CREATE ROLE r1, r2; DROP ROLE r1, r2");
  const auto changes = result->get_array("changes");
  ASSERT_TRUE(changes);
  ASSERT_EQ(4, changes->size());

  const auto create1 = changes->at(0).as_map()->get_map("create");
  const auto create2 = changes->at(1).as_map()->get_map("create");
  const auto drop1 = changes->at(2).as_map()->get_map("drop");
  const auto drop2 = changes->at(3).as_map()->get_map("drop");
  ASSERT_TRUE(create1);
  ASSERT_TRUE(create2);
  ASSERT_TRUE(drop1);
  ASSERT_TRUE(drop2);

  EXPECT_EQ("role", create1->get_string("_type"));
  EXPECT_EQ("r1", create1->get_string("name"));
  EXPECT_EQ("role", create2->get_string("_type"));
  EXPECT_EQ("r2", create2->get_string("name"));
  EXPECT_EQ("role", drop1->get_string("_type"));
  EXPECT_EQ("r1", drop1->get_string("_name"));
  EXPECT_EQ("role", drop2->get_string("_type"));
  EXPECT_EQ("r2", drop2->get_string("_name"));

  const auto quoted_result =
      parse_ddl_for_test("CREATE ROLE 'r'@'h'; DROP ROLE 'r'@'h'");
  const auto quoted_changes = quoted_result->get_array("changes");
  ASSERT_TRUE(quoted_changes);
  ASSERT_EQ(2, quoted_changes->size());

  const auto quoted_create = quoted_changes->at(0).as_map()->get_map("create");
  const auto quoted_drop = quoted_changes->at(1).as_map()->get_map("drop");
  ASSERT_TRUE(quoted_create);
  ASSERT_TRUE(quoted_drop);

  EXPECT_EQ("'r'@'h'", quoted_create->get_string("name"));
  EXPECT_EQ("'r'@'h'", quoted_drop->get_string("_name"));
}

TEST(MysqlParserUtils, parse_ddl_spatial_reference_system) {
  const auto create = get_single_change(
      parse_ddl_for_test("CREATE SPATIAL REFERENCE SYSTEM 9999 NAME TEXT "
                         "'test' DEFINITION TEXT 'LOCAL_CS[\"test\"]'"),
      "create");
  ASSERT_TRUE(create);

  EXPECT_EQ("spatialReferenceSystem", create->get_string("_type"));
  EXPECT_EQ("9999", create->get_string("name"));

  const auto drop = get_single_change(
      parse_ddl_for_test("DROP SPATIAL REFERENCE SYSTEM 9999"), "drop");
  ASSERT_TRUE(drop);

  EXPECT_EQ("spatialReferenceSystem", drop->get_string("_type"));
  EXPECT_EQ("9999", drop->get_string("_name"));
}

TEST(MysqlParserUtils, parse_ddl_resource_group) {
  const auto create = get_single_change(
      parse_ddl_for_test("CREATE RESOURCE GROUP rg TYPE = USER VCPU = 0-1, 3 "
                         "THREAD_PRIORITY = 5 ENABLE"),
      "create");
  ASSERT_TRUE(create);

  EXPECT_EQ("resourceGroup", create->get_string("_type"));
  EXPECT_EQ("rg", create->get_string("name"));
  EXPECT_EQ("USER", create->get_string("type"));
  EXPECT_EQ("0-1, 3", create->get_string("vcpu"));
  EXPECT_EQ(5, create->get_int("threadPriority"));
  EXPECT_EQ(true, create->get_bool("enabled"));

  const auto alter = get_single_change(
      parse_ddl_for_test("ALTER RESOURCE GROUP rg VCPU = 2 THREAD_PRIORITY = 7 "
                         "DISABLE FORCE"),
      "alter");
  ASSERT_TRUE(alter);

  EXPECT_EQ("resourceGroup", alter->get_string("_type"));
  EXPECT_EQ("rg", alter->get_string("_name"));
  EXPECT_EQ("2", alter->get_string("vcpu"));
  EXPECT_EQ(7, alter->get_int("threadPriority"));
  EXPECT_EQ(false, alter->get_bool("enabled"));
  EXPECT_EQ(true, alter->get_bool("force"));

  const auto drop = get_single_change(
      parse_ddl_for_test("DROP RESOURCE GROUP rg FORCE"), "drop");
  ASSERT_TRUE(drop);

  EXPECT_EQ("resourceGroup", drop->get_string("_type"));
  EXPECT_EQ("rg", drop->get_string("_name"));
  EXPECT_EQ(true, drop->get_bool("force"));
}

TEST(MysqlParserUtils, parse_ddl_undo_tablespace) {
  const auto create = get_single_change(
      parse_ddl_for_test(
          "CREATE UNDO TABLESPACE uts ADD DATAFILE 'uts.ibu' ENGINE = InnoDB"),
      "create");
  ASSERT_TRUE(create);

  EXPECT_EQ("undoTablespace", create->get_string("_type"));
  EXPECT_EQ("uts", create->get_string("name"));
  EXPECT_EQ("uts.ibu", create->get_string("dataFile"));

  const auto alter = get_single_change(
      parse_ddl_for_test("ALTER UNDO TABLESPACE uts SET INACTIVE"), "alter");
  ASSERT_TRUE(alter);

  EXPECT_EQ("undoTablespace", alter->get_string("_type"));
  EXPECT_EQ("uts", alter->get_string("_name"));
  EXPECT_EQ("INACTIVE", alter->get_string("state"));

  const auto drop =
      get_single_change(parse_ddl_for_test("DROP UNDO TABLESPACE uts"), "drop");
  ASSERT_TRUE(drop);

  EXPECT_EQ("undoTablespace", drop->get_string("_type"));
  EXPECT_EQ("uts", drop->get_string("_name"));
  EXPECT_FALSE(drop->has_key("_schema"));
}

TEST(MysqlParserUtils, parse_ddl_tablespace_sizes_accept_hex_and_suffixes) {
  const auto create = get_single_change(
      parse_ddl_for_test("CREATE TABLESPACE ts ADD DATAFILE 't.dat' "
                         "INITIAL_SIZE = 0x1000 FILE_BLOCK_SIZE = 4M"),
      "create");
  ASSERT_TRUE(create);

  EXPECT_EQ("tablespace", create->get_string("_type"));
  EXPECT_EQ(4096, create->get_uint("initialSize"));
  EXPECT_EQ(4194304, create->get_uint("fileBlockSize"));
}

TEST(MysqlParserUtils, parse_ddl_alter_library) {
  const auto context = create_parser_context({Version(9, 7, 0)});

  const auto alter = get_single_change(
      parse_ddl_script(context, "ALTER LIBRARY s.lib COMMENT 'changed'", "",
                       false),
      "alter");
  ASSERT_TRUE(alter);

  EXPECT_EQ("library", alter->get_string("_type"));
  EXPECT_EQ("s", alter->get_string("_schema"));
  EXPECT_EQ("lib", alter->get_string("_name"));
  EXPECT_EQ("changed", alter->get_string("comment"));

  EXPECT_THROW(parse_ddl_script(
                   context, "ALTER LIBRARY IF EXISTS s.lib COMMENT 'changed'",
                   "", false),
               shcore::Exception);
  EXPECT_THROW(
      parse_ddl_script(context, "ALTER LIBRARY s.lib garbage COMMENT 'changed'",
                       "", false),
      shcore::Exception);
  EXPECT_THROW(
      parse_ddl_script(context, "ALTER LIBRARY s.lib COMMENT 'changed' garbage",
                       "", false),
      shcore::Exception);
  EXPECT_THROW(
      parse_ddl_script(context, "ALTER LIBRARY s.lib COMMENT 1", "", false),
      shcore::Exception);
}

TEST(MysqlParserUtils, parse_ddl_unsupported_storage_ddl_is_ignored) {
  const char *ignored_statements[] = {
      "ALTER LOGFILE GROUP lg ADD UNDOFILE 'u.dat'",
      "ALTER SERVER s OPTIONS (HOST 'h')",
      "ALTER TABLESPACE ts ADD DATAFILE 't.dat'",
      "DROP LOGFILE GROUP lg",
      "DROP SERVER s",
      "DROP TABLESPACE ts",
  };

  for (const auto statement : ignored_statements) {
    SCOPED_TRACE(statement);

    const auto result = parse_ddl_for_test(statement);
    const auto changes = result->get_array("changes");
    ASSERT_TRUE(changes);
    EXPECT_TRUE(changes->empty());
  }
}

TEST(MysqlParserUtils, parse_ddl_create_udf_is_classified_as_udf) {
  const auto create = get_single_change(
      parse_ddl_for_test("CREATE FUNCTION f RETURNS STRING SONAME 'x.so'"),
      "create");
  ASSERT_TRUE(create);

  EXPECT_EQ("udf", create->get_string("_type"));
  EXPECT_EQ("f", create->get_string("name"));
  EXPECT_EQ("STRING", create->get_string("returnDataType"));
}

TEST(MysqlParserUtils, parse_ddl_rename_table_uses_source_schema_identity) {
  const auto result = parse_ddl_for_test("RENAME TABLE s.old TO s2.new");
  const auto changes = result->get_array("changes");
  ASSERT_EQ(1, changes->size());

  const auto change = changes->at(0).as_map();
  const auto alter = change->get_map("alter");
  ASSERT_TRUE(alter);
  EXPECT_FALSE(change->has_key("changes"));

  EXPECT_EQ("table", alter->get_string("_type"));
  EXPECT_EQ("s", alter->get_string("_schema"));
  EXPECT_EQ("old", alter->get_string("_name"));
  EXPECT_EQ("s2", alter->get_string("schema"));
  EXPECT_EQ("new", alter->get_string("name"));
}

TEST(MysqlParserUtils, parse_ddl_alter_table_rename_keeps_target_schema) {
  const auto alter = get_single_change(
      parse_ddl_for_test("ALTER TABLE s.old RENAME TO s2.new"), "alter");
  ASSERT_TRUE(alter);

  EXPECT_EQ("table", alter->get_string("_type"));
  EXPECT_EQ("s", alter->get_string("_schema"));
  EXPECT_EQ("old", alter->get_string("_name"));
  EXPECT_EQ("s2", alter->get_string("schema"));
  EXPECT_EQ("new", alter->get_string("name"));
}

TEST(MysqlParserUtils, parse_ddl_create_server_options) {
  shcore::Dictionary_t create;
  ASSERT_NO_THROW(create = get_single_change(
                      parse_ddl_for_test(
                          "CREATE SERVER s FOREIGN DATA WRAPPER mysql OPTIONS "
                          "(HOST 'h', DATABASE 'd', USER 'u', PASSWORD 'p')"),
                      "create"));
  ASSERT_TRUE(create);

  EXPECT_EQ("server", create->get_string("_type"));
  EXPECT_EQ("s", create->get_string("name"));
  EXPECT_EQ("h", create->get_string("host"));
  EXPECT_EQ("d", create->get_string("schema"));
  EXPECT_EQ("u", create->get_string("user"));
  EXPECT_EQ("p", create->get_string("password"));

  const auto concatenated = get_single_change(
      parse_ddl_for_test("CREATE SERVER s FOREIGN DATA WRAPPER mysql OPTIONS "
                         "(HOST 'h' 'ost')"),
      "create");
  ASSERT_TRUE(concatenated);

  EXPECT_EQ("host", concatenated->get_string("host"));

  const auto context =
      create_parser_context({mysqlshdk::utils::k_shell_version});
  context->update_sql_mode("NO_BACKSLASH_ESCAPES");

  const auto no_backslash_escapes = get_single_change(
      parse_ddl_script(context,
                       "CREATE SERVER s FOREIGN DATA WRAPPER mysql OPTIONS "
                       "(HOST 'a\\b')",
                       "", false),
      "create");
  ASSERT_TRUE(no_backslash_escapes);

  EXPECT_EQ("a\\b", no_backslash_escapes->get_string("host"));
}

TEST(MysqlParserUtils, parse_ddl_concatenates_comment_text_literals) {
  const auto column =
      get_single_change(parse_ddl_for_test("CREATE TABLE t ("
                                           "c INT COMMENT 'col' 'umn')"),
                        "create")
          ->get_array("columns")
          ->at(0)
          .as_map();
  EXPECT_EQ("column", column->get_string("comment"));

  const auto index =
      get_single_change(parse_ddl_for_test("CREATE TABLE t ("
                                           "c INT, INDEX i (c) COMMENT 'i' "
                                           "'dx')"),
                        "create")
          ->get_array("indexes")
          ->at(0)
          .as_map();
  EXPECT_EQ("idx", index->get_string("comment"));

  const auto routine = get_single_change(
      parse_ddl_for_test("CREATE FUNCTION f() RETURNS INT COMMENT 'fun' "
                         "'ction' RETURN 1"),
      "create");
  EXPECT_EQ("function", routine->get_string("comment"));

  const auto event = get_single_change(
      parse_ddl_for_test("CREATE EVENT e ON SCHEDULE EVERY 1 DAY COMMENT 'ev' "
                         "'ent' DO SELECT 1"),
      "create");
  EXPECT_EQ("event", event->get_string("comment"));
}

TEST(MysqlParserUtils, parse_ddl_preserves_timestamp_default_precision) {
  const auto column =
      get_single_change(
          parse_ddl_for_test("CREATE TABLE t (c TIMESTAMP(6) DEFAULT "
                             "CURRENT_TIMESTAMP(6) ON UPDATE "
                             "CURRENT_TIMESTAMP(6))"),
          "create")
          ->get_array("columns")
          ->at(0)
          .as_map();

  EXPECT_EQ("CURRENT_TIMESTAMP(6) ON UPDATE CURRENT_TIMESTAMP(6)",
            column->get_string("defaultValue"));
}

TEST(MysqlParserUtils, parse_ddl_comments_preserve_no_backslash_escapes) {
  const auto context =
      create_parser_context({mysqlshdk::utils::k_shell_version});
  context->update_sql_mode("NO_BACKSLASH_ESCAPES");

  const auto create =
      get_single_change(parse_ddl_script(context,
                                         "CREATE TABLE t ("
                                         "c INT COMMENT 'a\\nb') "
                                         "COMMENT 'table\\ncomment'",
                                         "", false),
                        "create");

  EXPECT_EQ("table\\ncomment", create->get_string("comment"));
  EXPECT_EQ("a\\nb", create->get_array("columns")->at(0).as_map()->get_string(
                         "comment"));
}

TEST(MysqlParserUtils, parse_ddl_create_table_union_option_does_not_abort) {
  shcore::Dictionary_t create;
  ASSERT_NO_THROW(
      create = get_single_change(
          parse_ddl_for_test("CREATE TABLE t (id INT) ENGINE=MRG_MYISAM "
                             "UNION=(t1,t2)"),
          "create"));
  ASSERT_TRUE(create);

  EXPECT_EQ("table", create->get_string("_type"));
  EXPECT_EQ("t", create->get_string("name"));
}

TEST(MysqlParserUtils, parse_ddl_respects_ansi_quotes_mode) {
  const auto context =
      create_parser_context({mysqlshdk::utils::k_shell_version});
  context->update_sql_mode("ANSI_QUOTES");

  const auto create = get_single_change(
      parse_ddl_script(context, R"*(CREATE TABLE "quoted table" (id INT))*", "",
                       false),
      "create");
  ASSERT_TRUE(create);

  EXPECT_EQ("table", create->get_string("_type"));
  EXPECT_EQ("quoted table", create->get_string("name"));
}

TEST(MysqlParserUtils, parse_ddl_respects_no_backslash_escapes_mode) {
  const std::string sql =
      "CREATE EVENT e ON SCHEDULE EVERY 1 DAY DO SELECT 'a\\''";
  const auto context =
      create_parser_context({mysqlshdk::utils::k_shell_version});

  EXPECT_NO_THROW(parse_ddl_script(context, sql, "", false));

  context->update_sql_mode("NO_BACKSLASH_ESCAPES");
  EXPECT_THROW(parse_ddl_script(context, sql, "", false), shcore::Exception);
}

TEST(MysqlParserUtils, extract_table_references) {
  const auto EXPECT = [](const std::string &stmt,
                         const std::vector<Table_reference> &expected) {
    SCOPED_TRACE(stmt);

    std::vector<Table_reference> actual;
    EXPECT_NO_THROW(actual = extract_table_references(
                        stmt, mysqlshdk::utils::k_shell_version));
    EXPECT_EQ(expected, actual);
  };

  EXPECT("SELECT * FROM DUAL", {});

  EXPECT("SELECT * FROM t", {{"", "t"}});
  EXPECT("SELECT * FROM s.t", {{"s", "t"}});

  EXPECT("SELECT * FROM `t`", {{"", "t"}});
  EXPECT("SELECT * FROM `s`.t", {{"s", "t"}});
  EXPECT("SELECT * FROM s.`t`", {{"s", "t"}});
  EXPECT("SELECT * FROM `s`.`t`", {{"s", "t"}});

  EXPECT("SELECT * FROM s.t WHERE 1=0", {{"s", "t"}});
  EXPECT("SELECT * FROM s.t, t2", {{"s", "t"}, {"", "t2"}});
  EXPECT("SELECT * FROM s.t, s2.t2 WHERE 1=0", {{"s", "t"}, {"s2", "t2"}});

  EXPECT("SELECT * FROM t PARTITION (p1, p2) AS a FORCE KEY (i)", {{"", "t"}});
  EXPECT("SELECT * FROM t PARTITION (p) FORCE KEY FOR JOIN (i)", {{"", "t"}});
  EXPECT("SELECT * FROM t FORCE KEY FOR ORDER BY (i1, i2)", {{"", "t"}});
  EXPECT("SELECT * FROM t FORCE KEY FOR GROUP BY (PRIMARY)", {{"", "t"}});

  EXPECT("SELECT * FROM t FORCE INDEX (i)", {{"", "t"}});
  EXPECT("SELECT * FROM t FORCE INDEX FOR JOIN (i)", {{"", "t"}});
  EXPECT("SELECT * FROM t FORCE INDEX FOR ORDER BY (i1, i2)", {{"", "t"}});
  EXPECT("SELECT * FROM t FORCE INDEX FOR GROUP BY (PRIMARY)", {{"", "t"}});

  EXPECT("SELECT * FROM t IGNORE KEY (i)", {{"", "t"}});
  EXPECT("SELECT * FROM t IGNORE KEY FOR JOIN (i)", {{"", "t"}});
  EXPECT("SELECT * FROM t IGNORE KEY FOR ORDER BY (i1, i2)", {{"", "t"}});
  EXPECT("SELECT * FROM t IGNORE KEY FOR GROUP BY (PRIMARY)", {{"", "t"}});

  EXPECT("SELECT * FROM t IGNORE INDEX (i)", {{"", "t"}});
  EXPECT("SELECT * FROM t IGNORE INDEX FOR JOIN (i)", {{"", "t"}});
  EXPECT("SELECT * FROM t IGNORE INDEX FOR ORDER BY (i1, i2)", {{"", "t"}});
  EXPECT("SELECT * FROM t IGNORE INDEX FOR GROUP BY (PRIMARY)", {{"", "t"}});

  EXPECT("SELECT * FROM t USE KEY (i)", {{"", "t"}});
  EXPECT("SELECT * FROM t USE KEY FOR JOIN (i)", {{"", "t"}});
  EXPECT("SELECT * FROM t USE KEY FOR ORDER BY (i1, i2)", {{"", "t"}});
  EXPECT("SELECT * FROM t USE KEY FOR GROUP BY (PRIMARY)", {{"", "t"}});

  EXPECT("SELECT * FROM t USE INDEX (i)", {{"", "t"}});
  EXPECT("SELECT * FROM t USE INDEX FOR JOIN (i)", {{"", "t"}});
  EXPECT("SELECT * FROM t USE INDEX FOR ORDER BY (i1, i2)", {{"", "t"}});
  EXPECT("SELECT * FROM t USE INDEX FOR GROUP BY (PRIMARY)", {{"", "t"}});

  EXPECT("SELECT * FROM t USE INDEX ()", {{"", "t"}});

  EXPECT("SELECT * FROM t USE KEY (i1) USE KEY (i2)", {{"", "t"}});

  EXPECT("SELECT * FROM (t)", {{"", "t"}});
  EXPECT("SELECT * FROM ((t))", {{"", "t"}});
  EXPECT("SELECT * FROM (t USE KEY (i))", {{"", "t"}});

  EXPECT("SELECT * FROM (SELECT * FROM t) AS s", {{"", "t"}});
  EXPECT("SELECT * FROM LATERAL (SELECT * FROM t) AS s (c1, c2)", {{"", "t"}});

  EXPECT("SELECT * FROM (t1 JOIN t2)", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM ((t1 JOIN t2))", {{"", "t1"}, {"", "t2"}});

  EXPECT("SELECT * FROM (t1, t2)", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM ((t1), (t2))", {{"", "t1"}, {"", "t2"}});

  EXPECT(
      "SELECT * FROM JSON_TABLE('[{\"c1\":null}]','$[*]' COLUMNS(c1 INT PATH "
      "'$.c1' ERROR ON ERROR)) AS jt, t",
      {{"", "t"}});

  EXPECT("SELECT * FROM t1 JOIN t2", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 INNER JOIN t2", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 CROSS JOIN t2", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 STRAIGHT_JOIN t2", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 JOIN t2 ON c1=c2", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 JOIN t2 USING (c)", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 JOIN t2 USING (c1, c2)", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 JOIN t2 JOIN t3",
         {{"", "t1"}, {"", "t2"}, {"", "t3"}});
  EXPECT("SELECT * FROM t1 JOIN t2 JOIN { OJ t3 }",
         {{"", "t1"}, {"", "t2"}, {"", "t3"}});

  EXPECT("SELECT * FROM t1 LEFT JOIN t2 ON c1=c2", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 LEFT OUTER JOIN t2 USING (c)",
         {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 RIGHT JOIN t2 USING (c1, c2)",
         {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 RIGHT OUTER JOIN t2 ON c1=c2",
         {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 LEFT JOIN t2 USING (c) JOIN t3",
         {{"", "t1"}, {"", "t2"}, {"", "t3"}});
  EXPECT("SELECT * FROM t1 LEFT JOIN t2 USING (c1, c2) JOIN { OJ t3 }",
         {{"", "t1"}, {"", "t2"}, {"", "t3"}});

  EXPECT("SELECT * FROM t1 NATURAL JOIN t2", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 NATURAL INNER JOIN t2", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 NATURAL RIGHT JOIN t2", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 NATURAL RIGHT OUTER JOIN t2",
         {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 NATURAL LEFT JOIN t2", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 NATURAL LEFT OUTER JOIN t2",
         {{"", "t1"}, {"", "t2"}});

  EXPECT("SELECT * FROM { OJ t }", {{"", "t"}});
  EXPECT("SELECT * FROM { OJ t1 JOIN t2 }", {{"", "t1"}, {"", "t2"}});

  EXPECT("TABLE t", {{"", "t"}});
  EXPECT("TABLE t ORDER BY c LIMIT 1 OFFSET 2", {{"", "t"}});

  EXPECT("WITH cte AS (SELECT c FROM t) SELECT c FROM cte", {{"", "t"}});
  EXPECT(
      "WITH cte (c1, c2) AS (SELECT 1, 2 UNION ALL SELECT 3, 4) SELECT c1, c2 "
      "FROM cte;",
      {});
}

TEST(MysqlParserUtils, extract_table_references_respects_ansi_quotes_mode) {
  Parser_config ansi_quotes_on;
  ansi_quotes_on.mysql_version = mysqlshdk::utils::k_shell_version;
  ansi_quotes_on.ansi_quotes = true;

  const auto stmt = R"*(SELECT * FROM "test"."quoted table")*";
  const auto expected = std::vector<Table_reference>{{"test", "quoted table"}};

  std::vector<Table_reference> actual;
  EXPECT_NO_THROW(actual = extract_table_references(stmt, ansi_quotes_on));
  EXPECT_EQ(expected, actual);
}

TEST(MysqlParserUtils, extract_table_references_class) {
  Extract_table_references etr{mysqlshdk::utils::k_shell_version};

  const auto EXPECT = [&etr](const std::string &stmt,
                             const std::vector<Table_reference> &expected) {
    SCOPED_TRACE(stmt);

    std::vector<Table_reference> actual;
    EXPECT_NO_THROW(actual = etr.run(stmt));
    EXPECT_EQ(expected, actual);
  };

  EXPECT("SELECT * FROM DUAL", {});

  EXPECT("SELECT * FROM t", {{"", "t"}});
  EXPECT("SELECT * FROM s.t", {{"s", "t"}});

  EXPECT("SELECT * FROM `t`", {{"", "t"}});
  EXPECT("SELECT * FROM `s`.t", {{"s", "t"}});
  EXPECT("SELECT * FROM s.`t`", {{"s", "t"}});
  EXPECT("SELECT * FROM `s`.`t`", {{"s", "t"}});

  EXPECT("SELECT * FROM s.t WHERE 1=0", {{"s", "t"}});
  EXPECT("SELECT * FROM s.t, t2", {{"s", "t"}, {"", "t2"}});
  EXPECT("SELECT * FROM s.t, s2.t2 WHERE 1=0", {{"s", "t"}, {"s2", "t2"}});

  EXPECT("SELECT * FROM t PARTITION (p1, p2) AS a FORCE KEY (i)", {{"", "t"}});
  EXPECT("SELECT * FROM t PARTITION (p) FORCE KEY FOR JOIN (i)", {{"", "t"}});
  EXPECT("SELECT * FROM t FORCE KEY FOR ORDER BY (i1, i2)", {{"", "t"}});
  EXPECT("SELECT * FROM t FORCE KEY FOR GROUP BY (PRIMARY)", {{"", "t"}});

  EXPECT("SELECT * FROM t FORCE INDEX (i)", {{"", "t"}});
  EXPECT("SELECT * FROM t FORCE INDEX FOR JOIN (i)", {{"", "t"}});
  EXPECT("SELECT * FROM t FORCE INDEX FOR ORDER BY (i1, i2)", {{"", "t"}});
  EXPECT("SELECT * FROM t FORCE INDEX FOR GROUP BY (PRIMARY)", {{"", "t"}});

  EXPECT("SELECT * FROM t IGNORE KEY (i)", {{"", "t"}});
  EXPECT("SELECT * FROM t IGNORE KEY FOR JOIN (i)", {{"", "t"}});
  EXPECT("SELECT * FROM t IGNORE KEY FOR ORDER BY (i1, i2)", {{"", "t"}});
  EXPECT("SELECT * FROM t IGNORE KEY FOR GROUP BY (PRIMARY)", {{"", "t"}});

  EXPECT("SELECT * FROM t IGNORE INDEX (i)", {{"", "t"}});
  EXPECT("SELECT * FROM t IGNORE INDEX FOR JOIN (i)", {{"", "t"}});
  EXPECT("SELECT * FROM t IGNORE INDEX FOR ORDER BY (i1, i2)", {{"", "t"}});
  EXPECT("SELECT * FROM t IGNORE INDEX FOR GROUP BY (PRIMARY)", {{"", "t"}});

  EXPECT("SELECT * FROM t USE KEY (i)", {{"", "t"}});
  EXPECT("SELECT * FROM t USE KEY FOR JOIN (i)", {{"", "t"}});
  EXPECT("SELECT * FROM t USE KEY FOR ORDER BY (i1, i2)", {{"", "t"}});
  EXPECT("SELECT * FROM t USE KEY FOR GROUP BY (PRIMARY)", {{"", "t"}});

  EXPECT("SELECT * FROM t USE INDEX (i)", {{"", "t"}});
  EXPECT("SELECT * FROM t USE INDEX FOR JOIN (i)", {{"", "t"}});
  EXPECT("SELECT * FROM t USE INDEX FOR ORDER BY (i1, i2)", {{"", "t"}});
  EXPECT("SELECT * FROM t USE INDEX FOR GROUP BY (PRIMARY)", {{"", "t"}});

  EXPECT("SELECT * FROM t USE INDEX ()", {{"", "t"}});

  EXPECT("SELECT * FROM t USE KEY (i1) USE KEY (i2)", {{"", "t"}});

  EXPECT("SELECT * FROM (t)", {{"", "t"}});
  EXPECT("SELECT * FROM ((t))", {{"", "t"}});
  EXPECT("SELECT * FROM (t USE KEY (i))", {{"", "t"}});

  EXPECT("SELECT * FROM (SELECT * FROM t) AS s", {{"", "t"}});
  EXPECT("SELECT * FROM LATERAL (SELECT * FROM t) AS s (c1, c2)", {{"", "t"}});

  EXPECT("SELECT * FROM (t1 JOIN t2)", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM ((t1 JOIN t2))", {{"", "t1"}, {"", "t2"}});

  EXPECT("SELECT * FROM (t1, t2)", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM ((t1), (t2))", {{"", "t1"}, {"", "t2"}});

  EXPECT(
      "SELECT * FROM JSON_TABLE('[{\"c1\":null}]','$[*]' COLUMNS(c1 INT PATH "
      "'$.c1' ERROR ON ERROR)) AS jt, t",
      {{"", "t"}});

  EXPECT("SELECT * FROM t1 JOIN t2", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 INNER JOIN t2", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 CROSS JOIN t2", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 STRAIGHT_JOIN t2", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 JOIN t2 ON c1=c2", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 JOIN t2 USING (c)", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 JOIN t2 USING (c1, c2)", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 JOIN t2 JOIN t3",
         {{"", "t1"}, {"", "t2"}, {"", "t3"}});
  EXPECT("SELECT * FROM t1 JOIN t2 JOIN { OJ t3 }",
         {{"", "t1"}, {"", "t2"}, {"", "t3"}});

  EXPECT("SELECT * FROM t1 LEFT JOIN t2 ON c1=c2", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 LEFT OUTER JOIN t2 USING (c)",
         {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 RIGHT JOIN t2 USING (c1, c2)",
         {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 RIGHT OUTER JOIN t2 ON c1=c2",
         {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 LEFT JOIN t2 USING (c) JOIN t3",
         {{"", "t1"}, {"", "t2"}, {"", "t3"}});
  EXPECT("SELECT * FROM t1 LEFT JOIN t2 USING (c1, c2) JOIN { OJ t3 }",
         {{"", "t1"}, {"", "t2"}, {"", "t3"}});

  EXPECT("SELECT * FROM t1 NATURAL JOIN t2", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 NATURAL INNER JOIN t2", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 NATURAL RIGHT JOIN t2", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 NATURAL RIGHT OUTER JOIN t2",
         {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 NATURAL LEFT JOIN t2", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 NATURAL LEFT OUTER JOIN t2",
         {{"", "t1"}, {"", "t2"}});

  EXPECT("SELECT * FROM { OJ t }", {{"", "t"}});
  EXPECT("SELECT * FROM { OJ t1 JOIN t2 }", {{"", "t1"}, {"", "t2"}});

  EXPECT("TABLE t", {{"", "t"}});
  EXPECT("TABLE t ORDER BY c LIMIT 1 OFFSET 2", {{"", "t"}});

  EXPECT("WITH cte AS (SELECT c FROM t) SELECT c FROM cte", {{"", "t"}});
  EXPECT(
      "WITH cte (c1, c2) AS (SELECT 1, 2 UNION ALL SELECT 3, 4) SELECT c1, c2 "
      "FROM cte;",
      {});
}

TEST(MysqlParserUtils,
     extract_table_references_class_respects_ansi_quotes_mode) {
  Parser_config ansi_quotes_on;
  ansi_quotes_on.mysql_version = mysqlshdk::utils::k_shell_version;
  ansi_quotes_on.ansi_quotes = true;

  Extract_table_references etr{ansi_quotes_on};

  const auto stmt = R"*(SELECT * FROM "test"."quoted table")*";
  const auto expected = std::vector<Table_reference>{{"test", "quoted table"}};

  std::vector<Table_reference> actual;
  EXPECT_NO_THROW(actual = etr.run(stmt));
  EXPECT_EQ(expected, actual);
}

TEST(MysqlParserUtils, bug_37393439) {
  // grammar of SHOW FUNCTION/PROCEDURE STATUS/CODE was not correct
  EXPECT_NO_THROW(check_sql_syntax("SHOW FUNCTION STATUS", {}));
  EXPECT_NO_THROW(check_sql_syntax("SHOW FUNCTION STATUS LIKE 'func%'", {}));
  EXPECT_NO_THROW(
      check_sql_syntax("SHOW FUNCTION STATUS WHERE Name LIKE 'func%'", {}));

  EXPECT_NO_THROW(check_sql_syntax("SHOW PROCEDURE STATUS", {}));
  EXPECT_NO_THROW(check_sql_syntax("SHOW PROCEDURE STATUS LIKE 'proc%'", {}));
  EXPECT_NO_THROW(
      check_sql_syntax("SHOW PROCEDURE STATUS WHERE Name LIKE 'proc%'", {}));

  EXPECT_NO_THROW(check_sql_syntax("SHOW FUNCTION CODE func", {}));

  EXPECT_NO_THROW(check_sql_syntax("SHOW PROCEDURE CODE proc", {}));
}

}  // namespace parser
}  // namespace mysqlshdk
