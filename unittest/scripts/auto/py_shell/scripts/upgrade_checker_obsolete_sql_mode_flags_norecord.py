#@{VER(<8.0.0)}

#@<> Setup
testutil.deploy_raw_sandbox(port=__mysql_sandbox_port1, rootpass="root")

obsolete_modes = ["DB2","MSSQL","MYSQL323","MYSQL40","NO_AUTO_CREATE_USER","NO_FIELD_OPTIONS","NO_KEY_OPTIONS","NO_TABLE_OPTIONS","ORACLE","POSTGRESQL"]

shell.connect(__sandbox_uri1)

session.run_sql("CREATE DATABASE IF NOT EXISTS SAMPLE")
session.run_sql("USE SAMPLE")
session.run_sql("CREATE TABLE SOME(COMPONENT integer, cube int)")

for mode in obsolete_modes:
    session.run_sql(f"SET SESSION sql_mode = '{mode}';")
    session.run_sql(f"CREATE FUNCTION FN_{mode} (s CHAR(20)) RETURNS CHAR(50) DETERMINISTIC RETURN CONCAT('Hello, ',s,'!');")
    session.run_sql(f"CREATE TRIGGER TR_{mode} AFTER INSERT ON SOME FOR EACH ROW DELETE FROM SOME WHERE COMPONENT<0;")
    session.run_sql(f"CREATE EVENT EV_{mode} ON SCHEDULE AT CURRENT_TIMESTAMP + INTERVAL 1 HOUR DO UPDATE SOME SET COMPONENT = COMPONENT + 1;")


session.run_sql(f"SET GLOBAL sql_mode = '{",".join(obsolete_modes)}';")


session.run_sql(f"SHOW VARIABLES LIKE 'sql_mode'")


def CHECK_DB_OBJECT_NOTICES():
    for mode in obsolete_modes:
        # Just skips because this one is a multiline message (no big deal on skipping)
        if mode != "NO_AUTO_CREATE_USER":
            EXPECT_OUTPUT_CONTAINS_IGNORE_CASE(f"- SAMPLE.SOME.TR_{mode}: TRIGGER uses obsolete {mode}")
        EXPECT_OUTPUT_CONTAINS_IGNORE_CASE(f"- sample.EV_{mode}: EVENT uses obsolete {mode}")
        EXPECT_OUTPUT_CONTAINS_IGNORE_CASE(f"- sample.FN_{mode}: FUNCTION uses obsolete {mode}")    

#@<> Check upgrade without passing configuration file
WIPE_OUTPUT()
rc = testutil.call_mysqlsh([__sandbox_uri1, "--", "util", "check-for-server-upgrade", "--include", "obsoleteSqlModeFlags"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQL_HOME=."])
EXPECT_EQ(0, rc)

EXPECT_OUTPUT_CONTAINS_MULTILINE('''1) Usage of obsolete sql_mode flags (obsoleteSqlModeFlags)
The following DB objects have obsolete options persisted for sql_mode.

Notice: The following flags will be cleared during the upgrade.''')


CHECK_DB_OBJECT_NOTICES()


EXPECT_OUTPUT_CONTAINS_MULTILINE('''Warning: Ensure the following flags are not persisted in the configuration
file as they will prevent the target server from loading.''')

for mode in obsolete_modes:
    EXPECT_OUTPUT_CONTAINS(f"- @@global.sql_mode: defined using obsolete {mode} option")


EXPECT_OUTPUT_CONTAINS_MULTILINE('''Errors:   0
Warnings: 10
Notices:  69

NOTE: No fatal errors were found that would prevent an upgrade, but some potential issues were detected. Please ensure that the reported issues are not significant before upgrading.''')




#@<> Check upgrade passing configuration file
WIPE_OUTPUT()
config_path = testutil.get_sandbox_conf_path(__mysql_sandbox_port1)
rc = testutil.call_mysqlsh([__sandbox_uri1, "--", "util", "check-for-server-upgrade", "--include", "obsoleteSqlModeFlags", "--config-path", config_path], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQL_HOME=."])
EXPECT_EQ(0, rc)


EXPECT_OUTPUT_CONTAINS_MULTILINE('''1) Usage of obsolete sql_mode flags (obsoleteSqlModeFlags)
The following DB objects have obsolete options persisted for sql_mode.

Notice: The following flags will be cleared during the upgrade.''')

for mode in obsolete_modes:
    EXPECT_OUTPUT_CONTAINS_IGNORE_CASE(f"- @@global.sql_mode: defined using obsolete {mode} option")


CHECK_DB_OBJECT_NOTICES()

EXPECT_OUTPUT_CONTAINS_MULTILINE('''Errors:   0
Warnings: 0
Notices:  79

NOTE: No fatal errors were found that would prevent an upgrade, but some potential issues were detected. Please ensure that the reported issues are not significant before upgrading.''')



#@<> Check upgrade passing configuration file with persisted sql_mode
WIPE_OUTPUT()
testutil.change_sandbox_conf(__mysql_sandbox_port1, "sql_mode", ','.join(obsolete_modes), "mysqld")
testutil.stop_sandbox(__mysql_sandbox_port1, {"wait":1})
testutil.start_sandbox(__mysql_sandbox_port1)
rc = testutil.call_mysqlsh([__sandbox_uri1, "--", "util", "check-for-server-upgrade", "--include", "obsoleteSqlModeFlags", "--config-path", config_path], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQL_HOME=."])
EXPECT_EQ(0, rc)


EXPECT_OUTPUT_CONTAINS_MULTILINE('''1) Usage of obsolete sql_mode flags (obsoleteSqlModeFlags)
The following DB objects have obsolete options persisted for sql_mode.

Notice: The following flags will be cleared during the upgrade.''')


CHECK_DB_OBJECT_NOTICES()


EXPECT_OUTPUT_CONTAINS_MULTILINE('''Error: The following flags are persisted in the configuration file which
will prevent the target server from loading, they must be removed.''')

for mode in obsolete_modes:
    EXPECT_OUTPUT_CONTAINS_IGNORE_CASE(f"- @@global.sql_mode: defined using obsolete {mode} option")


EXPECT_OUTPUT_CONTAINS_MULTILINE('''Errors:   10
Warnings: 0
Notices:  69

NOTE: No fatal errors were found that would prevent an upgrade, but some potential issues were detected. Please ensure that the reported issues are not significant before upgrading.''')


#@<> TearDown
testutil.destroy_sandbox(port=__mysql_sandbox_port1)
