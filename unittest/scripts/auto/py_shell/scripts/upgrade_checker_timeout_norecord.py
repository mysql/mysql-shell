#@{VER(<8.0.0)}

#@<> SetUp
shell.connect(__mysql_uri)
session.run_sql("set @OLD_SQL_MODE = @@SQL_MODE;")
session.run_sql("set @@SQL_MODE = 'NO_AUTO_CREATE_USER,NO_FIELD_OPTIONS,NO_KEY_OPTIONS';")
session.run_sql("drop schema if exists test;")
session.run_sql("CREATE SCHEMA test;")
session.run_sql("USE test;")
session.run_sql("create table Clone(COMPONENT integer, cube int);")
session.run_sql("CREATE FUNCTION TEST_FUNC (s CHAR(20)) RETURNS CHAR(50) DETERMINISTIC RETURN CONCAT('Hello, ',s,'!');")

#@<> WL16757-TSFR_1_1_1
EXPECT_THROWS(lambda: util.check_for_server_upgrade(__mysql_uri, {"targetVersion": "9.0.0", "checkTimeout": 0}),
              "ValueError: Argument #2: Check timeout must be non-zero, positive value")
WIPE_OUTPUT()

EXPECT_THROWS(lambda: util.check_for_server_upgrade(__mysql_uri, {"targetVersion": "9.0.0", "checkTimeout": -1}),
              "TypeError: Argument #2: Option 'checkTimeout' UInteger expected, but Integer value is out of range")
WIPE_OUTPUT()

EXPECT_THROWS(lambda: util.check_for_server_upgrade(__mysql_uri, {"targetVersion": "9.0.0", "checkTimeout": -123555}),
              "TypeError: Argument #2: Option 'checkTimeout' UInteger expected, but Integer value is out of range")
WIPE_OUTPUT()

EXPECT_THROWS(lambda: util.check_for_server_upgrade(__mysql_uri, {"targetVersion": "9.0.0", "checkTimeout": "SomeValue"}),
              "TypeError: Argument #2: Option 'checkTimeout' UInteger expected, but value is String")
WIPE_OUTPUT()

EXPECT_THROWS(lambda: util.check_for_server_upgrade(__mysql_uri, {"targetVersion": "9.0.0", "checkTimeout": "-1"}),
              "TypeError: Argument #2: Option 'checkTimeout' UInteger expected, but value is String")
WIPE_OUTPUT()

#@<> WL16757-TSFR_2_1 {not __dbug_off}
testutil.dbug_set("+d,dbg_uc_sql_mode_sleep,dbg_uc_check_table_sleep")
EXPECT_NO_THROWS(lambda: util.check_for_server_upgrade(__mysql_uri, {"targetVersion": "9.0.0", "checkTimeout": 20, "include": ["oldTemporal", "obsoleteSqlModeFlags", "checkTableCommand"]}))
EXPECT_OUTPUT_NOT_CONTAINS("Warning: Check timed out and was interrupted.")
testutil.dbug_set("")

#@<> All checks are interrupted {not __dbug_off}
testutil.dbug_set("+d,dbg_uc_sql_mode_sleep,dbg_uc_check_table_sleep")
EXPECT_NO_THROWS(lambda: util.check_for_server_upgrade(__mysql_uri, {"targetVersion": "9.0.0", "checkTimeout": 1, "include": ["obsoleteSqlModeFlags", "checkTableCommand"]}))
EXPECT_OUTPUT_CONTAINS_MULTILINE('''Issues reported by 'check table x for upgrade' command (checkTableCommand)
   Warning: Check timed out and was interrupted.''')
EXPECT_OUTPUT_CONTAINS_MULTILINE('''Usage of obsolete sql_mode flags (obsoleteSqlModeFlags)
   Warning: Check timed out and was interrupted.''')
testutil.dbug_set("")

#@<> WL16757-TSFR_2_2 {not __dbug_off}
testutil.dbug_set("+d,dbg_uc_sql_mode_sleep,dbg_uc_check_table_sleep")
EXPECT_NO_THROWS(lambda: util.check_for_server_upgrade(__mysql_uri, {"targetVersion": "9.0.0", "checkTimeout": 10, "include": ["oldTemporal", "obsoleteSqlModeFlags", "checkTableCommand"]}))
EXPECT_OUTPUT_CONTAINS("Warning: Check timed out and was interrupted.")
EXPECT_OUTPUT_CONTAINS("No issues found")
testutil.dbug_set("")

#@<> WL16757-TSFR_3_1 {not __dbug_off}
testutil.dbug_set("+d,dbg_uc_sql_mode_sleep,dbg_uc_check_table_sleep")
EXPECT_NO_THROWS(lambda: util.check_for_server_upgrade(__mysql_uri, {"targetVersion": "9.0.0", "include": ["oldTemporal", "obsoleteSqlModeFlags", "checkTableCommand"]}))
EXPECT_OUTPUT_NOT_CONTAINS("Warning: Check timed out and was interrupted.")
EXPECT_OUTPUT_CONTAINS("No issues found")
testutil.dbug_set("")

#@<> Kill session test for check table {not __dbug_off}
testutil.dbug_set("+d,dbg_uc_check_table_sleep")
EXPECT_NO_THROWS(lambda: util.check_for_server_upgrade(__mysql_uri, {"targetVersion": "9.0.0", "checkTimeout": 2, "include": ["checkTableCommand"]}))
EXPECT_OUTPUT_CONTAINS("Warning: Check timed out and was interrupted.")
testutil.dbug_set("")

#@<> CleanUp
session.run_sql("drop schema test;");
session.run_sql("set @@SQL_MODE = @OLD_SQL_MODE;");
