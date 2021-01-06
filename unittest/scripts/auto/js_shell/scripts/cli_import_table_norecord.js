//@<> Initialization
function callMysqlsh(additional_args) {
    base_args = [__mysqluripwd, "--quiet-start=2", "--result-format=json/raw"]
    return testutil.callMysqlsh(base_args.concat(additional_args), "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])
}

function validate_and_cleanup(schema, table, expected_content) {
    callMysqlsh(["--sql", "-e", `select * from ${schema}.${table}`]);
    EXPECT_OUTPUT_CONTAINS(`${expected_content}`);
    session.runSql(`drop table ${schema}.${table}`);
    WIPE_OUTPUT();
}

shell.connect(__mysqluripwd)

//@<> CLI import table - TSFR_3_3_1 - 1
schema = "test";
table = "table_a";
session.runSql("set global local_infile=ON");
session.runSql(`drop schema if exists ${schema}`);
session.runSql(`create schema ${schema}`);
session.runSql(`create table ${schema}.${table} (\`column_1\` VARCHAR(10), \`:int=1\` VARCHAR(10))`);
testutil.createFile(`${table}.dump`, "john\tdoe");
rc = callMysqlsh([`--schema=${schema}`, "--", "util", "import-table", `${table}.dump`, `--table=${table}`, "--columns:str=column_1,:int=1"]);
EXPECT_EQ(1, rc);
EXPECT_OUTPUT_CONTAINS("Unknown column 'column_1,:int=1' in 'field list'")
WIPE_OUTPUT();

//@<> CLI import table - TSFR_3_3_1 - 1.1 - Same as 3_3_1 without the str user type
rc = callMysqlsh([`--schema=${schema}`, "--", "util", "import-table", `${table}.dump`, `--table=${table}`, "--columns=column_1,:int=1"]);
EXPECT_EQ(0, rc);
validate_and_cleanup(schema, table, '{"column_1":"john",":int=1":"doe"}');

//@<> CLI import table - TSFR_3_3_1 - 1.1 - Same as 3_3_1 if the column 'column_1,:int=1' existed
session.runSql(`create table ${schema}.${table} (\`column_1,:int=1\` VARCHAR(10))`);
testutil.createFile(`${table}.dump`, "john");
rc = callMysqlsh([`--schema=${schema}`, "--", "util", "import-table", `${table}.dump`, `--table=${table}`, "--columns:str=column_1,:int=1"]);
EXPECT_EQ(0, rc);
validate_and_cleanup(schema, table, '{"column_1,:int=1":"john"}');

//@<> CLI import table - TSFR_3_3_1 - 2
session.runSql(`create table ${schema}.${table} (\`column_1\` VARCHAR(10))`);
testutil.createFile(`${table}.dump`, "john");
rc = callMysqlsh([`--schema=${schema}`, "--", "util", "import-table", `${table}.dump`, `--table=${table}`, "--columns:str=column_1", ":int=1"]);
// BUG #32286186 - IMPORT TABLE FROM CLI REPORTS WRONG RETURN CODE ON FAILURE WITH MULTIPLE FILES
//EXPECT_EQ(1, rc);
EXPECT_OUTPUT_CONTAINS('ERROR: File');
EXPECT_OUTPUT_CONTAINS(':int=1 does not exists.');
validate_and_cleanup(schema, table, '{"column_1":"john"}');

//@<> CLI import table - TSFR_3_3_1 - 3
session.runSql(`create table ${schema}.${table} (\`[:str=column_1\` VARCHAR(10), \`:int=1]\` VARCHAR(10))`);
testutil.createFile(`${table}.dump`, "john\tdoe");
rc = callMysqlsh([`--schema=${schema}`, "--", "util", "import-table", `${table}.dump`, `--table=${table}`, "--columns=[:str=column_1,:int=1]"]);
EXPECT_EQ(0, rc);
validate_and_cleanup(schema, table, '{"[:str=column_1":"john",":int=1]":"doe"}');

//@<> CLI import table - TSFR_3_3_1 - 4
session.runSql(`create table ${schema}.${table} (\`column_1\` VARCHAR(10))`);
testutil.createFile(`${table}.dump`, "john");
rc = callMysqlsh([`--schema=${schema}`, "--", "util", "import-table", `${table}.dump`, `--table=${table}`, "--columns=column_1"]);
EXPECT_EQ(0, rc);
validate_and_cleanup(schema, table, '{"column_1":"john"}');

//@<> CLI import table - TSFR_3_3_1 - 5
session.runSql(`create table ${schema}.${table} (\`column_1\` VARCHAR(10), \`column_2\` VARCHAR(10))`);
testutil.createFile(`${table}.dump`, "john\tdoe");
rc = callMysqlsh([`--schema=${schema}`, "--", "util", "import-table", `${table}.dump`, `--table=${table}`, "--columns=column_1,'column_2'"]);
EXPECT_EQ(0, rc);
validate_and_cleanup(schema, table, '{"column_1":"john","column_2":"doe"}');


//@<> CLI import table - TSFR_3_3_2 - 1
schema = "schema_a";
session.runSql(`drop schema if exists ${schema}`);
session.runSql(`create schema ${schema}`);
session.runSql(`create table ${schema}.${table} (\`:test=column_1\` VARCHAR(10))`);
testutil.createFile(`${table}.dump`, "john");
rc = callMysqlsh([`--schema=${schema}`, "--", "util", "import-table", `${table}.dump`, `--table=${table}`, "--columns=:test=column_1", "--schema=schema_a"]);
EXPECT_EQ(0, rc);
validate_and_cleanup(schema, table, '{":test=column_1":"john"}');

//@<> CLI import table - TSFR_3_3_2 - 2
session.runSql(`create table ${schema}.${table} (\`:::=column_1\` VARCHAR(10))`);
testutil.createFile(`${table}.dump`, "john");
rc = callMysqlsh([`--schema=${schema}`, "--", "util", "import-table", `${table}.dump`, `--table=${table}`, "--columns=:::=column_1", "--schema=schema_a"]);
EXPECT_EQ(0, rc);
validate_and_cleanup(schema, table, '{":::=column_1":"john"}');

//@<> CLI import table - TSFR_3_3_2 - 3
session.runSql(`create table ${schema}.${table} (\`---=column_1\` VARCHAR(10))`);
testutil.createFile(`${table}.dump`, "john");
rc = callMysqlsh([`--schema=${schema}`, "--", "util", "import-table", `${table}.dump`, `--table=${table}`, "--columns=---=column_1", "--schema=schema_a"]);
EXPECT_EQ(0, rc);
validate_and_cleanup(schema, table, '{"---=column_1":"john"}');

//@<> CLI import table - TSFR_3_3_2 - 4
session.runSql(`create table ${schema}.${table} (\`:str=column_1\` VARCHAR(10))`);
testutil.createFile(`${table}.dump`, "john");
rc = callMysqlsh([`--schema=${schema}`, "--", "util", "import-table", `${table}.dump`, `--table=${table}`, "--columns=:str=column_1", "--schema=schema_a"]);
EXPECT_EQ(0, rc);
validate_and_cleanup(schema, table, '{":str=column_1":"john"}');

//@<> CLI import table - TSFR_3_3_2 - 5
session.runSql(`create table ${schema}.${table} (\`:str=column_1\` VARCHAR(10), \`:test=1\` VARCHAR(10))`);
testutil.createFile(`${table}.dump`, "john\tdoe");
rc = callMysqlsh([`--schema=${schema}`, "--", "util", "import-table", `${table}.dump`, `--table=${table}`, "--columns=:str=column_1,:test=1"]);
EXPECT_EQ(0, rc);
validate_and_cleanup(schema, table, '{":str=column_1":"john",":test=1":"doe"}');

//@<> CLI import table - TSFR_3_3_2 - 6
session.runSql(`create table ${schema}.${table} (\`:int="1"\` VARCHAR(10))`);
testutil.createFile(`${table}.dump`, "john");
rc = callMysqlsh([`--schema=${schema}`, "--", "util", "import-table", `${table}.dump`, `--table=${table}`, "--columns=:int=\"1\"", "--schema=schema_a"]);
EXPECT_EQ(0, rc);
validate_and_cleanup(schema, table, '{":int=\\\"1\\\"":"john"}');

//@<> CLI import table - TSFR_3_3_2 - 7
session.runSql(`create table ${schema}.${table} (\`[:test=column_1\` VARCHAR(10), \`:int=1]\` VARCHAR(10))`);
testutil.createFile(`${table}.dump`, "john\tdoe");
rc = callMysqlsh([`--schema=${schema}`, "--", "util", "import-table", `${table}.dump`, `--table=${table}`, "--columns=[:test=column_1,:int=1]"]);
EXPECT_EQ(0, rc);
validate_and_cleanup(schema, table, '{"[:test=column_1":"john",":int=1]":"doe"}');

//@<> Cleanup
session.runSql("drop schema test");
session.runSql("drop schema schema_a");
session.close()