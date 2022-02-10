//@<> Initialization
function callMysqlsh(additional_args) {
    base_args = [__sandbox_uri1, "--quiet-start=2", "--log-level=debug"]
    return testutil.callMysqlsh(base_args.concat(additional_args), "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])
}

testutil.deploySandbox(__mysql_sandbox_port1, 'root');
shell.connect(__sandbox_uri1)

//testutil.rmdir(schema, true);
schemas = ['schema_a', 'schema_b', 'schema_c']
tables = ['table_a', 'table_b', 'table_c']
output_url =  "wl14297-TSFR_6_1_1"
for (s_index in schemas) {
    schema = schemas[s_index]
    session.runSql(`drop schema if exists \`${schema}\``);
    session.runSql(`create schema \`${schema}\``);
    for (index in tables) {
        session.runSql(`create table \`${schema}\`.${tables[index]} (id INT PRIMARY KEY, \`column_1\` VARCHAR(10), \`column_2\` VARCHAR(10))`);
        session.runSql(`insert into \`${schema}\`.${tables[index]} values (1, 'john','doe')`);
        session.runSql(`CREATE SQL SECURITY DEFINER VIEW \`${schema}\`.${tables[index]}_v AS SELECT * FROM ${schema}.${tables[index]}`);
    }
}

//@<> CLI dump-instance - WL14297 - TSFR_6_1_1 - 1
WIPE_SHELL_LOG()
var rc = callMysqlsh(["--", "util", "dump-instance", `${output_url}`, '--excludeSchemas=\"schema_a\",\"schema_b\"']);
EXPECT_EQ(0, rc);
EXPECT_SHELL_LOG_NOT_CONTAINS("Writing DDL for schema `schema_a`")
EXPECT_SHELL_LOG_NOT_CONTAINS("Writing DDL for schema `schema_b`")
EXPECT_SHELL_LOG_CONTAINS("Writing DDL for schema `schema_c`")
testutil.rmdir(output_url, true);

//@<> CLI dump-instance - WL14297 - TSFR_6_1_1 - 2
WIPE_SHELL_LOG()
var rc = callMysqlsh(["--", "util", "dump-instance", `${output_url}`, '--excludeTables=\"schema_a.table_a\",\"schema_b.table_b\"']);
EXPECT_EQ(0, rc);
EXPECT_SHELL_LOG_CONTAINS("Writing DDL for schema `schema_a`")
EXPECT_SHELL_LOG_CONTAINS("Writing DDL for schema `schema_b`")
EXPECT_SHELL_LOG_CONTAINS("Writing DDL for schema `schema_c`")
EXPECT_SHELL_LOG_NOT_CONTAINS("Writing DDL for table `schema_a`.`table_a`")
EXPECT_SHELL_LOG_CONTAINS("Writing DDL for table `schema_a`.`table_b`")
EXPECT_SHELL_LOG_CONTAINS("Writing DDL for table `schema_a`.`table_c`")
EXPECT_SHELL_LOG_CONTAINS("Writing DDL for table `schema_b`.`table_a`")
EXPECT_SHELL_LOG_NOT_CONTAINS("Writing DDL for table `schema_b`.`table_b`")
EXPECT_SHELL_LOG_CONTAINS("Writing DDL for table `schema_b`.`table_c`")
EXPECT_SHELL_LOG_CONTAINS("Writing DDL for table `schema_c`.`table_a`")
EXPECT_SHELL_LOG_CONTAINS("Writing DDL for table `schema_c`.`table_b`")
EXPECT_SHELL_LOG_CONTAINS("Writing DDL for table `schema_c`.`table_c`")
testutil.rmdir(output_url, true);

//@<> CLI dump-instance - WL14297 - TSFR_6_1_1 - 3
var rc = callMysqlsh(["--", "util", "dump-instance", `${output_url}`, '--compatibility=[\"force_innodb\",\"strip_definers\"]']);
EXPECT_EQ(0, rc);
EXPECT_OUTPUT_CONTAINS("NOTE: View `schema_a`.`table_c_v` had definer clause removed");
EXPECT_OUTPUT_CONTAINS("NOTE: View `schema_a`.`table_c_v` had SQL SECURITY characteristic set to INVOKER");
EXPECT_OUTPUT_CONTAINS("NOTE: View `schema_b`.`table_c_v` had definer clause removed");
EXPECT_OUTPUT_CONTAINS("NOTE: View `schema_b`.`table_c_v` had SQL SECURITY characteristic set to INVOKER");
EXPECT_OUTPUT_CONTAINS("NOTE: View `schema_c`.`table_c_v` had definer clause removed");
EXPECT_OUTPUT_CONTAINS("NOTE: View `schema_c`.`table_c_v` had SQL SECURITY characteristic set to INVOKER");
testutil.rmdir(output_url, true);

//@<> CLI dump-instance - WL14297 - TSFR_6_1_1 - 4
var rc = callMysqlsh(["--", "util", "dump-instance", `${output_url}`, '--compatibility', '["\\\"force_innodb\\\"","\\\"strip_definers\\\""]']);
EXPECT_EQ(1, rc);
EXPECT_OUTPUT_CONTAINS('ERROR: Argument options: Unknown compatibility option: "force_innodb"');

//@<> CLI dump-instance - WL14297 - TSFR_6_1_1 - 5
WIPE_SHELL_LOG()
var rc = callMysqlsh(["--", "util", "dump-instance", `${output_url}`, "--excludeTables='schema_a.table_a'", "--excludeTables='schema_b.table_b'"]);
EXPECT_EQ(0, rc);
EXPECT_SHELL_LOG_CONTAINS("Writing DDL for schema `schema_a`")
EXPECT_SHELL_LOG_CONTAINS("Writing DDL for schema `schema_b`")
EXPECT_SHELL_LOG_CONTAINS("Writing DDL for schema `schema_c`")
EXPECT_SHELL_LOG_NOT_CONTAINS("Writing DDL for table `schema_a`.`table_a`")
EXPECT_SHELL_LOG_CONTAINS("Writing DDL for table `schema_a`.`table_b`")
EXPECT_SHELL_LOG_CONTAINS("Writing DDL for table `schema_a`.`table_c`")
EXPECT_SHELL_LOG_CONTAINS("Writing DDL for table `schema_b`.`table_a`")
EXPECT_SHELL_LOG_NOT_CONTAINS("Writing DDL for table `schema_b`.`table_b`")
EXPECT_SHELL_LOG_CONTAINS("Writing DDL for table `schema_b`.`table_c`")
EXPECT_SHELL_LOG_CONTAINS("Writing DDL for table `schema_c`.`table_a`")
EXPECT_SHELL_LOG_CONTAINS("Writing DDL for table `schema_c`.`table_b`")
EXPECT_SHELL_LOG_CONTAINS("Writing DDL for table `schema_c`.`table_c`")

testutil.rmdir(output_url, true);

//@<> CLI dump-instance - WL14297 - TSFR_6_1_1 - 6
var rc = callMysqlsh(["--", "util", "dump-instance", `${output_url}`, "--excludeTables=\"\\\"schema_a.table_a\\\"\"", "--excludeTables=\"schema_b.table_b\""]);
EXPECT_EQ(1, rc);

// This error is the same produced if calling util.dumpInstance(<path>, {excludeTables:["\"schema_a.table_a\"", "schema_b.table_b"]})
// Which indicates the data is passed as expected to the API call
EXPECT_OUTPUT_CONTAINS("ERROR: Argument options: Failed to parse table to be excluded '\\\"schema_a.table_a\\\"': Invalid character in identifier")

//@<> CLI dump-instance - WL14297 - TSFR_8_1_1
var rc = callMysqlsh(["--", "util", "dump-instance", `${output_url}`, "--exclude--Tables", "table_a", "table_b", "table_c"]);
EXPECT_EQ(10, rc);

// This error is the same produced if calling util.dumpInstance(<path>, {excludeTables:["\"schema_a.table_a\"", "schema_b.table_b"]})
// Which indicates the data is passed as expected to the API call
EXPECT_OUTPUT_CONTAINS("ERROR: The following option is invalid: --exclude--Tables")

//@<> CLI dump-instance - WL14297 - TSFR_8_1_1 - 2
var rc = callMysqlsh(["--", "util", "dump-instance", `${output_url}`, "--exclu-deTab-les", "table_a", "table_b", "table_c"]);
EXPECT_EQ(10, rc);

// This error is the same produced if calling util.dumpInstance(<path>, {excludeTables:["\"schema_a.table_a\"", "schema_b.table_b"]})
// Which indicates the data is passed as expected to the API call
EXPECT_OUTPUT_CONTAINS("ERROR: The following option is invalid: --exclu-deTab-les")

//@<> Cleanup
session.runSql(`drop schema \`${schema}\``);
session.close()

testutil.destroySandbox(__mysql_sandbox_port1);
