//@<> Initialization
function callMysqlsh(additional_args) {
    base_args = [__mysqluripwd, "--quiet-start=2", "--log-level=debug"]
    testutil.callMysqlsh(base_args.concat(additional_args), "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=."])
}

shell.connect(__mysqluripwd)

schema = "wl14297-TSFR_5_1_1";
//testutil.rmdir(schema, true);
tables = ['table_a', 'table_b', 'table_c', 'table_d', 'table_e']
session.runSql(`drop schema if exists \`${schema}\``);
session.runSql(`create schema \`${schema}\``);
for (index in tables) {
    session.runSql(`create table \`${schema}\`.${tables[index]} (id INT PRIMARY KEY, \`column_1\` VARCHAR(10), \`column_2\` VARCHAR(10))`);
    session.runSql(`insert into \`${schema}\`.${tables[index]} values (1, 'john','doe')`);
}

// All of the tests should contain the same content on the dump
// at wl14297-TSFR_5_1_1/wl14297-TSFR_5_1_1.json
function validate() {
    EXPECT_STDOUT_CONTAINS("Writing DDL for table `wl14297-TSFR_5_1_1`.`table_a`");
    EXPECT_STDOUT_CONTAINS("Writing DDL for table `wl14297-TSFR_5_1_1`.`table_b`");
    EXPECT_STDOUT_CONTAINS("Writing DDL for table `wl14297-TSFR_5_1_1`.`table_c`");
    EXPECT_STDOUT_CONTAINS("Data dump for table `wl14297-TSFR_5_1_1`.`table_a` will be written to 1 file");
    EXPECT_STDOUT_CONTAINS("Data dump for table `wl14297-TSFR_5_1_1`.`table_b` will be written to 1 file");
    EXPECT_STDOUT_CONTAINS("Data dump for table `wl14297-TSFR_5_1_1`.`table_c` will be written to 1 file");
    WIPE_OUTPUT();
}
//@<> CLI import table - WL14297 - TSFR_5_1_1 - 1
callMysqlsh(["--", "util", "dump-tables", `${schema}`, '[\"table_a\",\"table_b\",\"table_c\"]', `--outputUrl=${schema}`]);
println(os.loadTextFile(os.path.join(`${schema}`, `${schema}.json`)));
validate();
testutil.rmdir(schema, true);

//@<> CLI import table - WL14297 - TSFR_5_1_1 - 2
callMysqlsh(["--", "util", "dump-tables", `${schema}`, '\"table_a\",\"table_b\",\"table_c\"', `--outputUrl=${schema}`]);
println(os.loadTextFile(os.path.join(`${schema}`, `${schema}.json`)));
validate();
testutil.rmdir(schema, true);

//@<> CLI import table - WL14297 - TSFR_5_1_1 - 3
callMysqlsh(["--", "util", "dump-tables", `${schema}`, '\"table_a\"', '\"table_b\"', '"table_c\"', `--outputUrl=${schema}`]);
println(os.loadTextFile(os.path.join(`${schema}`, `${schema}.json`)));
validate();
testutil.rmdir(schema, true);

//@<> CLI import table - WL14297 - Unquoted List
callMysqlsh(["--", "util", "dump-tables", `${schema}`, 'table_a,table_b,table_c', `--outputUrl=${schema}`]);
println(os.loadTextFile(os.path.join(`${schema}`, `${schema}.json`)));
validate();
testutil.rmdir(schema, true);

//@<> CLI import table - WL14297 - Unquoted Arguments
callMysqlsh(["--", "util", "dump-tables", `${schema}`, 'table_a', 'table_b' ,'table_c', `--outputUrl=${schema}`]);
println(os.loadTextFile(os.path.join(`${schema}`, `${schema}.json`)));
validate();
testutil.rmdir(schema, true);

//@<> Cleanup
session.runSql(`drop schema \`${schema}\``);
session.close()