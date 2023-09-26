#@<> INCLUDE dump_utils.inc
#@<> INCLUDE copy_utils.inc

#@<> entry point

# helpers
def EXPECT_SUCCESS(connectionData, options = {}, src = __sandbox_uri1, setup = None, schemas = [ "sakila" ]):
    # WL15298_TSFR_2_2
    EXPECT_SUCCESS_IMPL(util.copy_schemas, connectionData, options, src, setup, schemas)

def EXPECT_FAIL(error, msg, connectionData, options = {}, src = __sandbox_uri1, setup = None, schemas = [ "sakila" ]):
    EXPECT_FAIL_IMPL(util.copy_schemas, error, msg, connectionData, options, src, setup, schemas)

#@<> Setup
setup_copy_tests(3)
create_test_user(src_session)

#@<> WL15298 - invalid number of arguments
EXPECT_THROWS(lambda: util.copy_schemas(), "ValueError: Util.copy_schemas: Invalid number of arguments, expected 2 to 3 but got 0")

#@<> WL15298_TSFR_2_1_1
# use an additional sandbox to not mess up with the remaining tests
testutil.deploy_sandbox(__mysql_sandbox_port3, "root", { "local_infile": 1 })

EXPECT_SUCCESS(__sandbox_uri3, { "ignoreExistingObjects": True }, schemas = [ "mysql" ])
EXPECT_STDOUT_CONTAINS("Schema `mysql` already contains a table named ")
EXPECT_STDOUT_CONTAINS("One or more objects in the dump already exist in the destination database but will be ignored because the 'ignoreExistingObjects' option was enabled.")

testutil.destroy_sandbox(__mysql_sandbox_port3)

#@<> WL15298_TSFR_2_1_2
for schemas in [None, 1, "", False, {}]:
    EXPECT_FAIL("TypeError", "Argument #1 is expected to be an array", __sandbox_uri2, schemas = schemas)

#@<> WL15298_TSFR_2_1_3
EXPECT_FAIL("Error: Shell Error (53021)", re.compile(r"While '.*': Duplicate objects found in destination database"), __sandbox_uri2, schemas = [ "sakila", "information_schema", "performance_schema" ])
EXPECT_STDOUT_CONTAINS("Schema `performance_schema` already contains a table named ")

#@<> WL15298_TSFR_2_1_1_1
EXPECT_FAIL("ValueError", "The 'schemas' parameter cannot be an empty list.", __sandbox_uri2, schemas = [])

#@<> WL15298_TSFR_2_1_2_1
EXPECT_FAIL("ValueError", "Following schemas were not found in the database: 'non_existent'", __sandbox_uri2, schemas = [ "non_existent" ])

#@<> WL15298_TSFR_2_2_1
EXPECT_FAIL("ValueError", "Argument #2: Invalid URI: Invalid address", "mysql://")

#@<> WL15298 - connection options - invalid types
for connection_options in [None, 1, [], False]:
    EXPECT_FAIL("TypeError", "Argument #2: Invalid connection options, expected either a URI or a Connection Options Dictionary", connection_options)

#@<> WL15298 - connecting with wrong password
EXPECT_FAIL("DBError: MySQL Error (1045)", "Access denied for user 'root'@'localhost' (using password: YES)", { **shell.parse_uri(__sandbox_uri2), "password": "wrong" })

#@<> WL15298 - connecting with invalid SSH data
EXPECT_FAIL("ValueError", "Argument #2: Invalid SSH Identity file: Only absolute paths are accepted, the path 'wrong' looks like relative one.", { **shell.parse_uri(__sandbox_uri2), "ssh-identity-file": "wrong" })

#@<> WL15298_TSFR_2_3_1
for options in [1, [], "string"]:
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no} is expected to be a map", __sandbox_uri2, options)

#@<> WL15298_TSFR_2_4_1
EXPECT_FAIL("RuntimeError", "An open session is required to perform this operation.", __sandbox_uri2, setup = lambda: session.close())

#@<> WL15298_TSFR_4_1_1
TEST_UINT_OPTION("threads")

#@<> WL15298_TSFR_4_1_2
# WL15298_TSFR_4_4_4
EXPECT_SUCCESS(__sandbox_uri2, { "threads": 2 })
EXPECT_STDOUT_CONTAINS("Running data dump using 2 threads")

#@<> WL15298_TSFR_4_1_1_1
EXPECT_FAIL("ValueError", f"Argument #{options_arg_no}: The value of 'threads' option must be greater than 0.", __sandbox_uri2, { "threads": 0 })

#@<> WL15298_TSFR_4_1_3_1
# src is an X session
EXPECT_SUCCESS(__sandbox_uri2, src = get_ssl_config(src_session, get_x_config(src_session, __sandbox_uri1)))
# src is using SSL
EXPECT_SUCCESS(__sandbox_uri2, src = get_ssl_config(src_session, test_user_uri(__mysql_sandbox_port1)))
# WL15298_TSFR_4_1_2_1
EXPECT_STDOUT_CONTAINS("Running data dump using 4 threads")

#@<> WL15298_TSFR_4_1_3_2
# tgt is an X session
EXPECT_SUCCESS(get_x_config(tgt_session, __sandbox_uri2))
# tgt is using SSL
# the user is copied from the source server to make sure that the CREATE USER statements for validation are the same
create_user = src_session.run_sql(f"SHOW CREATE USER {test_user_account}").fetch_one()[0]
EXPECT_SUCCESS(get_ssl_config(tgt_session, test_user_uri(__mysql_sandbox_port2)), setup = lambda: tgt_session.run_sql(create_user) and tgt_session.run_sql(f"GRANT ALL ON *.* TO {test_user_account}"))

#@<> WL15298_TSFR_4_4_5
TEST_ARRAY_OF_STRINGS_OPTION("compatibility")

#@<> WL15298_TSFR_4_4_6
EXPECT_SUCCESS(__sandbox_uri2, { "compatibility": [] })

#@<> WL15298_TSFR_4_4_7
EXPECT_FAIL("ValueError", f"Argument #{options_arg_no}: Unknown compatibility option: unknown_compat_mode", __sandbox_uri2, { "compatibility": [ "unknown_compat_mode" ] })

#@<> WL15298_TSFR_4_4_8 {VER(>=8.0.24)}
# this tests that compatibility mode is recognized (there's no error)
EXPECT_SUCCESS(__sandbox_uri2, { "compatibility": [ "create_invisible_pks" ] })

#@<> WL15298_TSFR_4_4_9
# WL15298_TSFR_4_4_10
# WL15298_TSFR_4_4_12
# WL15298_TSFR_4_4_13
# WL15298_TSFR_4_4_14
# WL15298_TSFR_4_4_15
# this tests that compatibility mode is recognized and some of them are applied
EXPECT_SUCCESS(__sandbox_uri2, { "compatibility": [ "force_innodb", "ignore_missing_pks", "ignore_wildcard_grants", "skip_invalid_accounts", "strip_definers", "strip_invalid_grants", "strip_restricted_grants", "strip_tablespaces" ] })
EXPECT_STDOUT_NOT_CONTAINS(f"User {test_user_account} had restricted privileges")
EXPECT_STDOUT_CONTAINS("Function `sakila`.`get_customer_balance` had definer clause removed")
EXPECT_STDOUT_CONTAINS("View `sakila`.`sales_by_store` had SQL SECURITY characteristic set to INVOKER")

#@<> WL15298_TSFR_4_4_19
EXPECT_SUCCESS(__sandbox_uri2, { "triggers": True })

#@<> WL15298_TSFR_4_4_20
EXPECT_SUCCESS(__sandbox_uri2, { "triggers": False })

#@<> WL15298_TSFR_4_4_21
TEST_BOOL_OPTION("triggers")

#@<> WL15298 - test invalid values of excludeTriggers option
TEST_ARRAY_OF_STRINGS_OPTION("excludeTriggers")

#@<> WL15298_TSFR_4_4_22
EXPECT_SUCCESS(__sandbox_uri2, { "excludeTriggers": [] })

#@<> WL15298_TSFR_4_4_23
EXPECT_SUCCESS(__sandbox_uri2, { "excludeTriggers": [ "sakila.customer", "sakila.wrong", "sakila.film.ins_film", "sakila.film.wrong" ] })

#@<> WL15298 - test invalid values of includeTriggers option
TEST_ARRAY_OF_STRINGS_OPTION("includeTriggers")

#@<> WL15298_TSFR_4_4_27
EXPECT_SUCCESS(__sandbox_uri2, { "includeTriggers": [] })

#@<> WL15298_TSFR_4_4_28
EXPECT_SUCCESS(__sandbox_uri2, { "includeTriggers": [ "sakila.customer", "sakila.wrong", "sakila.film.ins_film", "sakila.film.wrong" ] })

#@<> WL15298_TSFR_4_4_31
EXPECT_SUCCESS(__sandbox_uri2, { "tzUtc": True })

#@<> WL15298_TSFR_4_4_32
EXPECT_SUCCESS(__sandbox_uri2, { "tzUtc": False })

#@<> WL15298_TSFR_4_4_33
TEST_BOOL_OPTION("tzUtc")

#@<> WL15298_TSFR_4_4_34
EXPECT_SUCCESS(__sandbox_uri2, { "consistent": True })

#@<> WL15298 - test consistent option
EXPECT_SUCCESS(__sandbox_uri2, { "consistent": False })
EXPECT_STDOUT_CONTAINS("The dumped value of gtid_executed is not guaranteed to be consistent")

#@<> WL15298 - test invalid values of consistent option
TEST_BOOL_OPTION("consistent")

#@<> WL15298_TSFR_4_4_39
EXPECT_SUCCESS(__sandbox_uri2, { "ddlOnly": True })

#@<> WL15298_TSFR_4_4_40
EXPECT_SUCCESS(__sandbox_uri2, { "ddlOnly": False })

#@<> WL15298_TSFR_4_4_41
TEST_BOOL_OPTION("ddlOnly")

#@<> WL15298_TSFR_4_4_42
wipeout_server(tgt_session)
EXPECT_FAIL("Error: Shell Error (53005)", "Error loading dump", __sandbox_uri2, { "dataOnly": True })
EXPECT_STDOUT_CONTAINS("Unknown database 'sakila'")

#@<> WL15298_TSFR_4_4_43
# there are some triggers that can interfere with the load, disable those
EXPECT_SUCCESS(__sandbox_uri2, { "dataOnly": True }, setup = lambda: util.copy_schemas([ "sakila" ], __sandbox_uri2, { "ddlOnly": True, "triggers": False, "showProgress": False }))

#@<> WL15298_TSFR_4_4_44
EXPECT_FAIL("ValueError", f"Argument #{options_arg_no}: The 'ddlOnly' and 'dataOnly' options cannot be both set to true.", __sandbox_uri2, { "ddlOnly": True, "dataOnly": True })

#@<> WL15298_TSFR_4_4_45
TEST_BOOL_OPTION("dataOnly")

#@<> WL15298 - test dryRun option
EXPECT_SUCCESS(__sandbox_uri2, { "dryRun": True })
EXPECT_STDOUT_CONTAINS("dryRun enabled, no locks will be acquired and no files will be created.")
EXPECT_STDOUT_CONTAINS("dryRun enabled, no changes will be made.")

EXPECT_SUCCESS(__sandbox_uri2, { "dryRun": False })
EXPECT_STDOUT_NOT_CONTAINS("dryRun")

#@<> WL15298 - test invalid values of dryRun option
TEST_BOOL_OPTION("dryRun")

#@<> WL15298_TSFR_4_4_46
WIPE_SHELL_LOG()
EXPECT_SUCCESS(__sandbox_uri2, { "chunking": True })
EXPECT_SHELL_LOG_CONTAINS("Chunking `sakila`.`film`")

#@<> WL15298_TSFR_4_4_47
WIPE_SHELL_LOG()
EXPECT_SUCCESS(__sandbox_uri2, { "chunking": False })
EXPECT_SHELL_LOG_NOT_CONTAINS("Chunking `sakila`.`film`")

#@<> WL15298_TSFR_4_4_48
TEST_BOOL_OPTION("chunking")

#@<> WL15298_TSFR_4_4_49
EXPECT_SUCCESS(__sandbox_uri2, { "bytesPerChunk": "128000" })

#@<> WL15298_TSFR_4_4_50
TEST_STRING_OPTION("bytesPerChunk")
EXPECT_FAIL("ValueError", f'Argument #{options_arg_no}: Wrong input number "2Mhz"', __sandbox_uri2, { "bytesPerChunk": "2Mhz" })

#@<> WL15298_TSFR_4_4_51
EXPECT_SUCCESS(__sandbox_uri2, { "maxRate": "1G" })

#@<> WL15298_TSFR_4_4_52
EXPECT_SUCCESS(__sandbox_uri2, { "maxRate": "" })

#@<> WL15298_TSFR_4_4_53
TEST_STRING_OPTION("maxRate")
EXPECT_FAIL("ValueError", f'Argument #{options_arg_no}: Wrong input number "2Mhz"', __sandbox_uri2, { "maxRate": "2Mhz" })

#@<> WL15298_TSFR_4_4_54
EXPECT_SUCCESS(__sandbox_uri2, { "showProgress": True })
# if progress is shown, progress information (like the one below) is not captured from stdout
EXPECT_STDOUT_NOT_CONTAINS("Writing schema metadata")

#@<> WL15298_TSFR_4_4_55
EXPECT_SUCCESS(__sandbox_uri2, { "showProgress": False })
# if progress is hidden, progress information is printed to stdout
EXPECT_STDOUT_CONTAINS("Writing schema metadata")

#@<> WL15298 - test invalid values of showProgress option
TEST_BOOL_OPTION("showProgress")

#@<> WL15298_TSFR_4_4_56
EXPECT_SUCCESS(__sandbox_uri2, { "defaultCharacterSet": "latin1" })

#@<> WL15298_TSFR_4_4_58
EXPECT_FAIL("DBError: MySQL Error (1115)", "Unknown character set: 'wrong'", __sandbox_uri2, { "defaultCharacterSet": "wrong" })

#@<> WL15298 - test invalid values of defaultCharacterSet option
TEST_STRING_OPTION("defaultCharacterSet")

#@<> WL15298 - test skipConsistencyChecks option
EXPECT_SUCCESS(__sandbox_uri2, { "skipConsistencyChecks": True })
EXPECT_SUCCESS(__sandbox_uri2, { "skipConsistencyChecks": False })

#@<> WL15298 - test invalid values of skipConsistencyChecks option
TEST_BOOL_OPTION("skipConsistencyChecks")

#@<> WL15298 - test where option
EXPECT_SUCCESS(__sandbox_uri2, { "where": { "sakila.actor": "actor_id > 20" } })

#@<> WL15298 - test invalid values of where option
TEST_MAP_OF_STRINGS_OPTION("where")

#@<> WL15298 - test partitions option
src_session.run_sql("""CREATE TABLE `sakila`.`actor_part`
(`actor_id` SMALLINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
 `first_name` VARCHAR(45) NOT NULL,
 `last_name` VARCHAR(45) NOT NULL)
PARTITION BY RANGE (`actor_id`)
(PARTITION x0 VALUES LESS THAN (50),
 PARTITION x1 VALUES LESS THAN (100),
 PARTITION x2 VALUES LESS THAN (150),
 PARTITION x3 VALUES LESS THAN MAXVALUE)""")
src_session.run_sql("INSERT INTO `sakila`.`actor_part` SELECT actor_id, first_name, last_name FROM `sakila`.`actor`")

EXPECT_SUCCESS(__sandbox_uri2, { "partitions": { "sakila.actor_part": [ "x1", "x2" ] } })

src_session.run_sql("DROP TABLE `sakila`.`actor_part`")

#@<> WL15298 - test invalid values of partitions option
TEST_MAP_OF_ARRAY_OF_STRINGS_OPTION("partitions")

#@<> WL15298_TSFR_4_5_1
EXPECT_FAIL("ValueError", f"Argument #{options_arg_no}: The value of 'maxBytesPerTransaction' option must be greater than or equal to 4096 bytes.", __sandbox_uri2, { "maxBytesPerTransaction": "4000" })

#@<> WL15298_TSFR_4_5_2
# WL15298_TSFR_5_1_2
EXPECT_SUCCESS(__sandbox_uri2, { "maxBytesPerTransaction": "1M" })
# WL15298_TSFR_4_5_7
EXPECT_STDOUT_NOT_CONTAINS("Analyzing tables")
# WL15298_TSFR_4_6_1
EXPECT_STDOUT_MATCHES(re.compile(r"""
---
Dump_metadata:
  Binlog_file: .*
  Binlog_position: .*
  Executed_GTID_set: .*
"""))

#@<> WL15298 - test invalid values of maxBytesPerTransaction option
TEST_STRING_OPTION("maxBytesPerTransaction")
EXPECT_FAIL("ValueError", f'Argument #{options_arg_no}: Wrong input number "2Mhz"', __sandbox_uri2, { "maxBytesPerTransaction": "2Mhz" })

#@<> WL15298_TSFR_4_5_8
TEST_STRING_OPTION("analyzeTables")

#@<> WL15298_TSFR_4_5_9
EXPECT_FAIL("ValueError", f"Argument #{options_arg_no}: Invalid value '' for analyzeTables option, allowed values: 'histogram', 'off' and 'on'.", __sandbox_uri2, { "analyzeTables": "" })
EXPECT_FAIL("ValueError", f"Argument #{options_arg_no}: Invalid value 'wrong' for analyzeTables option, allowed values: 'histogram', 'off' and 'on'.", __sandbox_uri2, { "analyzeTables": "wrong" })

#@<> WL15298_TSFR_4_5_10
EXPECT_SUCCESS(__sandbox_uri2, { "analyzeTables": "off" })
EXPECT_STDOUT_NOT_CONTAINS("Analyzing tables")

#@<> WL15298_TSFR_4_5_11
EXPECT_SUCCESS(__sandbox_uri2, { "analyzeTables": "histogram" })
# NOTE: functionality is checked in load tests
EXPECT_STDOUT_CONTAINS("Analyzing tables")

#@<> WL15298 - test analyzeTables option
EXPECT_SUCCESS(__sandbox_uri2, { "analyzeTables": "on" })
# NOTE: functionality is checked in load tests
EXPECT_STDOUT_CONTAINS("Analyzing tables")

#@<> WL15298_TSFR_4_5_12
EXPECT_FAIL("ValueError", f"Argument #{options_arg_no}: Invalid value '' for deferTableIndexes option, allowed values: 'all', 'fulltext' and 'off'.", __sandbox_uri2, { "deferTableIndexes": "" })
EXPECT_FAIL("ValueError", f"Argument #{options_arg_no}: Invalid value 'wrong' for deferTableIndexes option, allowed values: 'all', 'fulltext' and 'off'.", __sandbox_uri2, { "deferTableIndexes": "wrong" })

#@<> WL15298_TSFR_4_5_13
TEST_STRING_OPTION("deferTableIndexes")

#@<> WL15298_TSFR_4_5_14
EXPECT_SUCCESS(__sandbox_uri2, { "deferTableIndexes": "all" })
# NOTE: functionality is checked in load tests
EXPECT_STDOUT_CONTAINS("Recreating indexes")

#@<> WL15298_TSFR_4_5_15
EXPECT_SUCCESS(__sandbox_uri2, { "deferTableIndexes": "fulltext" })
# NOTE: functionality is checked in load tests
EXPECT_STDOUT_CONTAINS("Recreating indexes")

#@<> WL15298_TSFR_4_5_16
EXPECT_SUCCESS(__sandbox_uri2, { "deferTableIndexes": "off" })
EXPECT_STDOUT_NOT_CONTAINS("Recreating indexes")

#@<> WL15298 - test invalid values of handleGrantErrors option
TEST_STRING_OPTION("handleGrantErrors")
EXPECT_FAIL("ValueError", f"Argument #{options_arg_no}: The value of the 'handleGrantErrors' option must be set to one of: 'abort', 'drop_account', 'ignore'.", __sandbox_uri2, { "handleGrantErrors": "" })
EXPECT_FAIL("ValueError", f"Argument #{options_arg_no}: The value of the 'handleGrantErrors' option must be set to one of: 'abort', 'drop_account', 'ignore'.", __sandbox_uri2, { "handleGrantErrors": "wrong" })

#@<> WL15298 - test handleGrantErrors option
EXPECT_SUCCESS(__sandbox_uri2, { "handleGrantErrors": "abort" })
EXPECT_SUCCESS(__sandbox_uri2, { "handleGrantErrors": "drop_account" })
EXPECT_SUCCESS(__sandbox_uri2, { "handleGrantErrors": "ignore" })

#@<> WL15298_TSFR_4_5_17
TEST_BOOL_OPTION("ignoreExistingObjects")

#@<> WL15298_TSFR_4_5_18
wipeout_server(tgt_session)
setup_session(__sandbox_uri1)
WIPE_STDOUT()
# we're not using EXPECT_SUCCESS() here, because second copy updates timestamp columns and verification will fail
EXPECT_NO_THROWS(lambda: util.copy_schemas([ "sakila" ], __sandbox_uri2, { "showProgress": False }), "copy should not throw")
EXPECT_NO_THROWS(lambda: util.copy_schemas([ "sakila" ], __sandbox_uri2, { "ignoreExistingObjects": True, "showProgress": False }), "copy should not throw")
EXPECT_STDOUT_CONTAINS("One or more objects in the dump already exist in the destination database but will be ignored because the 'ignoreExistingObjects' option was enabled.")

#@<> WL15298_TSFR_4_5_19
wipeout_server(tgt_session)
setup_session(__sandbox_uri1)
WIPE_STDOUT()
EXPECT_NO_THROWS(lambda: util.copy_schemas([ "sakila" ], __sandbox_uri2, { "showProgress": False }), "copy should not throw")
EXPECT_FAIL("Error: Shell Error (53021)", re.compile(r"While '.*': Duplicate objects found in destination database"), __sandbox_uri2, { "ignoreExistingObjects": "False" })

#@<> WL15298 - test invalid values of ignoreVersion option
TEST_BOOL_OPTION("ignoreVersion")

#@<> WL15298_TSFR_4_5_22
TEST_BOOL_OPTION("loadIndexes")

#@<> WL15298_TSFR_4_5_23
EXPECT_FAIL("ValueError", f"Argument #{options_arg_no} 'deferTableIndexes' option needs to be enabled when 'loadIndexes' option is disabled", __sandbox_uri2, { "loadIndexes": False, "deferTableIndexes": "off" })

#@<> WL15298_TSFR_4_5_24
EXPECT_SUCCESS(__sandbox_uri2, { "loadIndexes": False, "deferTableIndexes": "all" })
EXPECT_STDOUT_NOT_CONTAINS("Recreating indexes")

#@<> WL15298_TSFR_4_5_25
EXPECT_SUCCESS(__sandbox_uri2, { "loadIndexes": True, "deferTableIndexes": "all" })
EXPECT_STDOUT_CONTAINS("Recreating indexes")

#@<> WL15298_TSFR_4_5_26
TEST_STRING_OPTION("schema")

#@<> WL15298_TSFR_4_5_29
EXPECT_SUCCESS(__sandbox_uri2, { "schema": "sakila-new" })

#@<> WL15298_TSFR_4_5_30
TEST_ARRAY_OF_STRINGS_OPTION("sessionInitSql")

#@<> WL15298_TSFR_4_5_31
EXPECT_FAIL("RuntimeError", "Error while executing sessionInitSql: MySQL Error 1064 (42000): You have an error in your SQL syntax; check the manual that corresponds to your MySQL server version for the right syntax to use near 'wrong' at line 1", __sandbox_uri2, { "sessionInitSql": [ "wrong" ] })

#@<> WL15298 - test sessionInitSql option
EXPECT_SUCCESS(__sandbox_uri2, { "sessionInitSql": [ "INSERT INTO ver.t VALUES (1)" ] }, setup = lambda: tgt_session.run_sql('CREATE SCHEMA ver') and tgt_session.run_sql('CREATE TABLE ver.t (a INT)'))

#@<> WL15298_TSFR_4_5_32
TEST_BOOL_OPTION("skipBinlog")

#@<> WL15298_TSFR_4_5_33
EXPECT_SUCCESS(__sandbox_uri2, { "skipBinlog": False })

#@<> WL15298_TSFR_4_5_34
EXPECT_SUCCESS(__sandbox_uri2, { "skipBinlog": True })

#@<> WL15298_TSFR_4_5_35
TEST_STRING_OPTION("updateGtidSet")
EXPECT_FAIL("ValueError", f"Argument #{options_arg_no}: Invalid value '' for updateGtidSet option, allowed values: 'append', 'off' and 'replace'.", __sandbox_uri2, { "updateGtidSet": "" })
EXPECT_FAIL("ValueError", f"Argument #{options_arg_no}: Invalid value 'wrong' for updateGtidSet option, allowed values: 'append', 'off' and 'replace'.", __sandbox_uri2, { "updateGtidSet": "wrong" })

#@<> WL15298_TSFR_4_5_36 {VER(>=8.0.0)}
EXPECT_SUCCESS(__sandbox_uri2, { "updateGtidSet": "replace" })

#@<> WL15298_TSFR_4_5_37 {VER(>=8.0.0)}
EXPECT_SUCCESS(__sandbox_uri2, { "updateGtidSet": "append" })

#@<> WL15298_TSFR_4_5_38
EXPECT_SUCCESS(__sandbox_uri2, { "updateGtidSet": "off" })

#@<> WL15298_TSFR_4_5_39 {VER(<8.0.0)}
EXPECT_FAIL("Error: Shell Error (53013)", "Target MySQL server does not support updateGtidSet:'append'.", __sandbox_uri2, { "updateGtidSet": "append" })
EXPECT_FAIL("Error: Shell Error (53014)", "The updateGtidSet option on MySQL 5.7 target server can only be used if the skipBinlog option is enabled.", __sandbox_uri2, { "updateGtidSet": "replace" })

#@<> WL15298_TSFR_5_1_3
# WL15298_TSFR_5_1_4
# WL15298_TSFR_5_1_5
# WL15298_TSFR_5_1_7
EXPECT_SUCCESS(__sandbox_uri2, { "excludeTables": [ "sakila.actor_info", "`sakila`.`actor2`", "`sakila`.`sales_by_film_category`" ] })

#@<> WL15298_TSFR_5_1_6
EXPECT_FAIL("ValueError", f"Argument #{options_arg_no}: Failed to parse table to be excluded 'wrong.@': Invalid character in identifier", __sandbox_uri2, { "excludeTables": [ "wrong.@" ] })

#@<> WL15298_TSFR_5_1_8
TEST_ARRAY_OF_STRINGS_OPTION("excludeTables")

#@<> WL15298_TSFR_5_1_9
EXPECT_SUCCESS(__sandbox_uri2, { "includeTables": [] })

#@<> WL15298_TSFR_5_1_10
EXPECT_SUCCESS(__sandbox_uri2, { "includeTables": [ "sakila.customer", "sakila.address", "sakila.city", "sakila.country", "`sakila`.`actor2`", "`sakila`.`customer_list`" ] })

#@<> WL15298 - test invalid values of includeTables option
TEST_ARRAY_OF_STRINGS_OPTION("includeTables")

#@<> WL15298_TSFR_5_1_11
EXPECT_SUCCESS(__sandbox_uri2, { "events": True })

#@<> WL15298_TSFR_5_1_12
EXPECT_SUCCESS(__sandbox_uri2, { "events": False })

#@<> WL15298_TSFR_5_1_13
TEST_BOOL_OPTION("events")

#@<> WL15298_TSFR_5_1_14
EXPECT_SUCCESS(__sandbox_uri2, { "excludeEvents": [] })

#@<> WL15298_TSFR_5_1_16
EXPECT_FAIL("ValueError", f"Argument #{options_arg_no}: The event to be excluded must be in the following form: schema.event, with optional backtick quotes, wrong value: 'event'.", __sandbox_uri2, { "excludeEvents": [ "event" ] })

#@<> WL15298_TSFR_5_1_17
EXPECT_SUCCESS(__sandbox_uri2, { "excludeEvents": [ "sakila.existing_event", "`sakila`.`wrong`" ] })

#@<> WL15298 - test invalid values of excludeEvents option
TEST_ARRAY_OF_STRINGS_OPTION("excludeEvents")

#@<> WL15298_TSFR_5_1_20
EXPECT_SUCCESS(__sandbox_uri2, { "includeEvents": [] })

#@<> WL15298_TSFR_5_1_21
EXPECT_SUCCESS(__sandbox_uri2, { "includeEvents": [ "sakila.existing_event", "`sakila`.`wrong`" ] })

#@<> WL15298 - test invalid values of includeEvents option
TEST_ARRAY_OF_STRINGS_OPTION("includeEvents")

#@<> WL15298_TSFR_5_1_23
EXPECT_SUCCESS(__sandbox_uri2, { "routines": True })

#@<> WL15298 - test routines option
EXPECT_SUCCESS(__sandbox_uri2, { "routines": False })

#@<> WL15298_TSFR_5_1_24
TEST_BOOL_OPTION("routines")

#@<> WL15298_TSFR_5_1_25
EXPECT_SUCCESS(__sandbox_uri2, { "excludeRoutines": [] })

#@<> WL15298_TSFR_5_1_26
EXPECT_SUCCESS(__sandbox_uri2, { "excludeRoutines": [ "`sakila`.`rewards_report`", "sakila.get_customer_balance", "`sakila`.`wrong`" ] })

#@<> WL15298 - test invalid values of excludeRoutines option
TEST_ARRAY_OF_STRINGS_OPTION("excludeRoutines")

#@<> WL15298 - test includeRoutines option
EXPECT_SUCCESS(__sandbox_uri2, { "includeRoutines": [] })

#@<> WL15298_TSFR_5_1_29
EXPECT_SUCCESS(__sandbox_uri2, { "includeRoutines": [ "`sakila`.`rewards_report`", "sakila.get_customer_balance", "`sakila`.`wrong`" ] })

#@<> WL15298 - test invalid values of includeRoutines option
TEST_ARRAY_OF_STRINGS_OPTION("includeRoutines")

#@<> WL15298 - unknown options
for param in [
        "dummy",
        "all",
        "compression",
        "excludeSchemas",
        "excludeUsers",
        "includeSchemas",
        "includeUsers",
        "ocimds",
        "targetVersion",
        "users",
        "backgroundThreads",
        "characterSet",
        "createInvisiblePKs",
        "loadData",
        "loadDdl",
        "loadUsers",
        "progressFile",
        "resetProgress",
        "showMetadata",
        "waitDumpTimeout",
        "osBucketName",
        "osNamespace",
        "ociConfigFile",
        "ociProfile",
        "ociParManifest",
        "ociParExpireTime",
        "s3BucketName",
        "s3CredentialsFile",
        "s3ConfigFile",
        "s3Profile",
        "s3Region",
        "s3EndpointOverride",
        "azureContainerName",
        "azureConfigFile",
        "azureStorageAccount",
        "azureStorageSasToken",
        "dialect",
        "fieldsTerminatedBy",
        "fieldsEnclosedBy",
        "fieldsOptionallyEnclosed",
        "fieldsEscapedBy",
        "linesTerminatedBy"
        ]:
    EXPECT_FAIL("ValueError", f"Argument #{options_arg_no}: Invalid options: {param}", __sandbox_uri2, { param: "fails" })

#@<> WL15887 - setup {not __dbug_off and VER(>=8.2.0)}
schema_name = "wl15887"
test_table_primary = "ttp"
test_schema_event = "tse"
test_schema_function = "tsf"
test_schema_procedure = "tsp"
test_table_trigger = "ttt"
test_view = "tv"

def setup_db(account):
    src_session.run_sql("DROP SCHEMA IF EXISTS !", [schema_name])
    src_session.run_sql("CREATE SCHEMA !", [schema_name])
    src_session.run_sql("CREATE TABLE !.! (`id` MEDIUMINT NOT NULL AUTO_INCREMENT PRIMARY KEY) ENGINE=InnoDB;", [ schema_name, test_table_primary ])
    src_session.run_sql(f"CREATE DEFINER={account} EVENT !.! ON SCHEDULE EVERY 1 HOUR DO BEGIN END;", [ schema_name, test_schema_event ])
    src_session.run_sql(f"CREATE DEFINER={account} FUNCTION !.!(s CHAR(20)) RETURNS CHAR(50) DETERMINISTIC RETURN CONCAT('Hello, ',s,'!');", [ schema_name, test_schema_function ])
    src_session.run_sql(f"CREATE DEFINER={account} PROCEDURE !.!() SQL SECURITY DEFINER BEGIN END", [ schema_name, test_schema_procedure ])
    src_session.run_sql(f"CREATE DEFINER={account} TRIGGER !.! BEFORE UPDATE ON ! FOR EACH ROW BEGIN END;", [ schema_name, test_table_trigger, test_table_primary ])
    src_session.run_sql(f"CREATE DEFINER={account} SQL SECURITY INVOKER VIEW !.! AS SELECT * FROM !.!;", [ schema_name, test_view, schema_name, test_table_primary ])

setup_db(test_user_account)
testutil.dbug_set("+d,copy_utils_force_mds")

#@<> WL15887-TSFR_3_1_1 - restricted accounts {not __dbug_off and VER(>=8.2.0)}
for account in ["mysql.infoschema", "mysql.session", "mysql.sys", "ociadmin", "ocidbm", "ocirpl"]:
    account = f"`{account}`@`localhost`"
    setup_db(account)
    WIPE_OUTPUT()
    EXPECT_FAIL("Error: Shell Error (52004)", "While 'Validating MySQL HeatWave Service compatibility': Compatibility issues were found", __sandbox_uri2, { "dryRun": True, "showProgress": False }, schemas = [ schema_name ])
    EXPECT_STDOUT_CONTAINS(definer_clause_uses_restricted_user_name(schema_name, test_schema_event, "Event", account).error(True))
    EXPECT_STDOUT_CONTAINS(definer_clause_uses_restricted_user_name(schema_name, test_schema_function, "Function", account).error(True))
    EXPECT_STDOUT_CONTAINS(definer_clause_uses_restricted_user_name(schema_name, test_schema_procedure, "Procedure", account).error(True))
    EXPECT_STDOUT_CONTAINS(definer_clause_uses_restricted_user_name(schema_name, test_table_trigger, "Trigger", account).error(True))
    EXPECT_STDOUT_CONTAINS(definer_clause_uses_restricted_user_name(schema_name, test_view, "View", account).error(True))

# restore schema
setup_db(test_user_account)

#@<> WL15887-TSFR_3_2_1 - valid account {not __dbug_off and VER(>=8.2.0)}
EXPECT_SUCCESS(__sandbox_uri2, { "dryRun": True, "showProgress": False }, schemas = [ schema_name ])

# no warnings about DEFINER=
EXPECT_STDOUT_NOT_CONTAINS(strip_definers_definer_clause(schema_name, test_schema_event, "Event", test_user_account).warning(True))
EXPECT_STDOUT_NOT_CONTAINS(strip_definers_definer_clause(schema_name, test_schema_function, "Function", test_user_account).warning(True))
EXPECT_STDOUT_NOT_CONTAINS(strip_definers_definer_clause(schema_name, test_schema_procedure, "Procedure", test_user_account).warning(True))
EXPECT_STDOUT_NOT_CONTAINS(strip_definers_definer_clause(schema_name, test_table_trigger, "Trigger", test_user_account).warning(True))
EXPECT_STDOUT_NOT_CONTAINS(strip_definers_definer_clause(schema_name, test_view, "View", test_user_account).warning(True))

# WL15887-TSFR_3_5_1 - no warnings about SQL SECURITY
EXPECT_STDOUT_NOT_CONTAINS(strip_definers_security_clause(schema_name, test_schema_function, "Function").warning(True))
EXPECT_STDOUT_NOT_CONTAINS(strip_definers_security_clause(schema_name, test_schema_procedure, "Procedure").warning(True))
EXPECT_STDOUT_NOT_CONTAINS(strip_definers_security_clause(schema_name, test_view, "View").warning(True))

# WL15887-TSFR_3_3_1 - no account is not included in the dump
EXPECT_STDOUT_CONTAINS(definer_clause_uses_unknown_account_once().warning(True))

#@<> WL15887-TSFR_4_1 - note about strip_definers {not __dbug_off and VER(>=8.2.0)}
EXPECT_SUCCESS(__sandbox_uri2, { "compatibility": [ "strip_definers" ], "dryRun": True, "showProgress": False }, schemas = [ schema_name ])
EXPECT_STDOUT_CONTAINS(f"NOTE: The 'targetVersion' option is set to {__mysh_version_no_extra}. This version supports the SET_ANY_DEFINER privilege, using the 'strip_definers' compatibility option is unnecessary.")

#@<> WL15887 - cleanup {not __dbug_off and VER(>=8.2.0)}
src_session.run_sql("DROP SCHEMA IF EXISTS !;", [schema_name])
testutil.dbug_set("")

#@<> WL15947 - setup
schema_name = "wl15947"
test_table_primary = "pk"
test_table_unique = "uni"
test_table_unique_null = "uni-null"
test_table_no_index = "no-index"
test_table_partitioned = "part"
test_table_empty = "empty"
test_table_gipk = "gipk"
test_table_timestamp = "ts"
gipk_supported = __version_num >= 80030

def setup_db():
    src_session.run_sql("DROP SCHEMA IF EXISTS !", [schema_name])
    src_session.run_sql("CREATE SCHEMA !", [schema_name])
    src_session.run_sql("CREATE TABLE !.! (id INT NOT NULL PRIMARY KEY AUTO_INCREMENT, datecol DATE NOT NULL)", [ schema_name, test_table_primary ])
    src_session.run_sql("CREATE TABLE !.! (id INT NOT NULL UNIQUE KEY, datecol DATE NOT NULL)", [ schema_name, test_table_unique ])
    src_session.run_sql("CREATE TABLE !.! (id INT UNIQUE KEY, datecol DATE NOT NULL)", [ schema_name, test_table_unique_null ])
    src_session.run_sql("CREATE TABLE !.! (id INT, datecol DATE NOT NULL)", [ schema_name, test_table_no_index ])
    src_session.run_sql("CREATE TABLE !.! (id INT NOT NULL PRIMARY KEY, datecol DATE NOT NULL) PARTITION BY RANGE (`id`) (PARTITION p0 VALUES LESS THAN (500), PARTITION p1 VALUES LESS THAN (1000), PARTITION p2 VALUES LESS THAN MAXVALUE);", [ schema_name, test_table_partitioned ])
    src_session.run_sql("CREATE TABLE !.! (id INT, datecol DATE NOT NULL)", [ schema_name, test_table_empty ])
    src_session.run_sql("CREATE TABLE !.! (id INT NOT NULL PRIMARY KEY, ts TIMESTAMP DEFAULT CURRENT_TIMESTAMP)", [ schema_name, test_table_timestamp ])
    # WL15947-ET_1 - columns with invalid values
    src_session.run_sql("SET @saved_sql_mode = @@sql_mode")
    src_session.run_sql("SET @@sql_mode = 'ALLOW_INVALID_DATES,NO_AUTO_VALUE_ON_ZERO'")
    src_session.run_sql("INSERT INTO !.! (id, datecol) VALUES (0, '2004-04-28')", [ schema_name, test_table_primary ])
    src_session.run_sql("INSERT INTO !.! (datecol) VALUES ('2004-04-29'), ('2004-04-30'), ('2004-04-31')", [ schema_name, test_table_primary ])
    src_session.run_sql("INSERT INTO !.! SELECT * FROM !.!;", [ schema_name, test_table_unique, schema_name, test_table_primary ])
    src_session.run_sql("INSERT INTO !.! SELECT * FROM !.!;", [ schema_name, test_table_unique_null, schema_name, test_table_primary ])
    src_session.run_sql("INSERT INTO !.! SELECT * FROM !.!;", [ schema_name, test_table_no_index, schema_name, test_table_primary ])
    src_session.run_sql("INSERT INTO !.! SELECT * FROM !.!;", [ schema_name, test_table_partitioned, schema_name, test_table_primary ])
    src_session.run_sql("INSERT INTO !.! (id) VALUES (1), (2), (3)", [ schema_name, test_table_timestamp ])
    src_session.run_sql("ANALYZE TABLE !.!;", [ schema_name, test_table_primary ])
    src_session.run_sql("ANALYZE TABLE !.!;", [ schema_name, test_table_unique ])
    src_session.run_sql("ANALYZE TABLE !.!;", [ schema_name, test_table_unique_null ])
    src_session.run_sql("ANALYZE TABLE !.!;", [ schema_name, test_table_no_index ])
    src_session.run_sql("ANALYZE TABLE !.!;", [ schema_name, test_table_partitioned ])
    src_session.run_sql("ANALYZE TABLE !.!;", [ schema_name, test_table_timestamp ])
    if gipk_supported:
        src_session.run_sql("SET @@SESSION.sql_generate_invisible_primary_key=ON")
        src_session.run_sql("CREATE TABLE !.! (id INT, datecol DATE NOT NULL) ENGINE=InnoDB;", [ schema_name, test_table_gipk ])
        src_session.run_sql("INSERT INTO !.! SELECT * FROM !.!;", [ schema_name, test_table_gipk, schema_name, test_table_primary ])
        src_session.run_sql("ANALYZE TABLE !.!;", [ schema_name, test_table_gipk ])
        src_session.run_sql("SET @@SESSION.sql_generate_invisible_primary_key=OFF")
    src_session.run_sql("SET @@sql_mode = @saved_sql_mode")

setup_db()

#@<> WL15947-TSFR_3_1 - help text
help_text = """
      - checksum: bool (default: false) - Compute checksums of the data and
        verify tables in the target instance against these checksums.
"""
EXPECT_TRUE(help_text in util.help("copy_schemas"))

#@<> WL15947-TSFR_3_1_1 - copy without checksum option
EXPECT_SUCCESS(__sandbox_uri2, {}, schemas = [ schema_name ])
EXPECT_STDOUT_NOT_CONTAINS("checksum")

#@<> WL15947-TSFR_3_1_1 - copy with checksum option set to False
EXPECT_SUCCESS(__sandbox_uri2, { "checksum": False }, schemas = [ schema_name ])
EXPECT_STDOUT_NOT_CONTAINS("checksum")

#@<> WL15947-TSFR_3_1_2 - option type
TEST_BOOL_OPTION("checksum")

#@<> WL15947-TSFR_3_2_2 - "checksum": True, "ddlOnly": False, "chunking": True, not partitioned table
wipeout_server(tgt_session)
EXPECT_FAIL("Error: Shell Error (53031)", "Checksum verification failed", __sandbox_uri2, { "checksum": True, "ddlOnly": False, "chunking": True, "includeTables": [ quote_identifier(schema_name, test_table_primary) ] }, schemas = [ schema_name ])
EXPECT_STDOUT_CONTAINS(f"Checksum verification failed for: `{schema_name}`.`{test_table_primary}` (chunk 0) (boundary:")

#@<> WL15947-TSFR_3_2_3 - "checksum": True, "ddlOnly": False, "chunking": True, partitioned table
wipeout_server(tgt_session)
EXPECT_FAIL("Error: Shell Error (53031)", "Checksum verification failed", __sandbox_uri2, { "checksum": True, "ddlOnly": False, "chunking": True, "includeTables": [ quote_identifier(schema_name, test_table_partitioned) ] }, schemas = [ schema_name ])
EXPECT_STDOUT_CONTAINS(f"Checksum verification failed for: `{schema_name}`.`{test_table_partitioned}` partition `p0` (chunk 0) (boundary:")

#@<> WL15947-TSFR_3_2_4 - "checksum": True, "ddlOnly": False, "chunking": False, not partitioned table
wipeout_server(tgt_session)
EXPECT_FAIL("Error: Shell Error (53031)", "Checksum verification failed", __sandbox_uri2, { "checksum": True, "ddlOnly": False, "chunking": False, "includeTables": [ quote_identifier(schema_name, test_table_primary) ] }, schemas = [ schema_name ])
EXPECT_STDOUT_CONTAINS(f"Checksum verification failed for: `{schema_name}`.`{test_table_primary}`.")

#@<> WL15947-TSFR_3_2_5 - "checksum": True, "ddlOnly": False, "chunking": False, partitioned table
wipeout_server(tgt_session)
EXPECT_FAIL("Error: Shell Error (53031)", "Checksum verification failed", __sandbox_uri2, { "checksum": True, "ddlOnly": False, "chunking": False, "includeTables": [ quote_identifier(schema_name, test_table_partitioned) ] }, schemas = [ schema_name ])
EXPECT_STDOUT_CONTAINS(f"Checksum verification failed for: `{schema_name}`.`{test_table_partitioned}` partition `p0`.")

#@<> WL15947-TSFR_3_2_6 - "checksum": True, "ddlOnly": True, not partitioned table
for chunking in [ True, False ]:
    wipeout_server(tgt_session)
    EXPECT_FAIL("Error: Shell Error (53031)", "Checksum verification failed", __sandbox_uri2, { "checksum": True, "ddlOnly": True, "chunking": chunking, "includeTables": [ quote_identifier(schema_name, test_table_primary) ] }, schemas = [ schema_name ])
    EXPECT_STDOUT_CONTAINS(f"Checksum verification failed for: `{schema_name}`.`{test_table_primary}`. Mismatched number of rows, ")

#@<> WL15947-TSFR_3_2_6 - "checksum": True, "ddlOnly": True, partitioned table
for chunking in [ True, False ]:
    wipeout_server(tgt_session)
    EXPECT_FAIL("Error: Shell Error (53031)", "Checksum verification failed", __sandbox_uri2, { "checksum": True, "ddlOnly": True, "chunking": chunking, "includeTables": [ quote_identifier(schema_name, test_table_partitioned) ] }, schemas = [ schema_name ])
    EXPECT_STDOUT_CONTAINS(f"Checksum verification failed for: `{schema_name}`.`{test_table_partitioned}` partition `p0`. Mismatched number of rows, ")

#@<> WL15947-TSFR_3_2_7 - "checksum": True, table without an index
wipeout_server(tgt_session)
EXPECT_FAIL("Error: Shell Error (53031)", "Checksum verification failed", __sandbox_uri2, { "checksum": True, "includeTables": [ quote_identifier(schema_name, test_table_no_index) ] }, schemas = [ schema_name ])
EXPECT_STDOUT_CONTAINS(f"Checksum verification failed for: `{schema_name}`.`{test_table_no_index}`.")

#@<> WL15947-TSFR_3_2_8 - "checksum": True, table with a non-NULL unique index
wipeout_server(tgt_session)
EXPECT_FAIL("Error: Shell Error (53031)", "Checksum verification failed", __sandbox_uri2, { "checksum": True, "includeTables": [ quote_identifier(schema_name, test_table_unique) ] }, schemas = [ schema_name ])
EXPECT_STDOUT_CONTAINS(f"Checksum verification failed for: `{schema_name}`.`{test_table_unique}` (chunk 0) (boundary:")

#@<> WL15947-TSFR_3_2_9 - "checksum": True, GIPK {gipk_supported}
src_session.run_sql("SET @@GLOBAL.show_gipk_in_create_table_and_information_schema = OFF")
wipeout_server(tgt_session)
EXPECT_FAIL("Error: Shell Error (53031)", "Checksum verification failed", __sandbox_uri2, { "checksum": True, "includeTables": [ quote_identifier(schema_name, test_table_gipk) ] }, schemas = [ schema_name ])
EXPECT_STDOUT_CONTAINS(f"Checksum verification failed for: `{schema_name}`.`{test_table_gipk}`.")

#@<> WL15947 - restore show_gipk_in_create_table_and_information_schema variable {gipk_supported}
src_session.run_sql("SET @@GLOBAL.show_gipk_in_create_table_and_information_schema = ON")

#@<> WL15947-TSFR_3_2_10 - where option
EXPECT_SUCCESS(__sandbox_uri2, { "checksum": True, "where": { quote_identifier(schema_name, test_table_primary): "id = 1" }, "includeTables": [ quote_identifier(schema_name, test_table_primary) ] }, schemas = [ schema_name ])
EXPECT_STDOUT_CONTAINS("checksum")

#@<> WL15947-TSFR_3_2_11 - dryRun
EXPECT_SUCCESS(__sandbox_uri2, { "checksum": True, "dryRun": True, "includeTables": [ quote_identifier(schema_name, test_table_primary) ] }, schemas = [ schema_name ])
EXPECT_STDOUT_CONTAINS("Checksum information is not going to be verified, dryRun enabled.")

#@<> WL15947 - copying when target already has data
# prepare target server
wipeout_server(tgt_session)
EXPECT_NO_THROWS(lambda: util.copy_tables(schema_name, [test_table_primary], __sandbox_uri2, { "showProgress": False }))

# WL15947-TSFR_3_2_13 - target has the same data
# WL15947-TSFR_3_2_14 - target already has other data
WIPE_OUTPUT()
EXPECT_THROWS(lambda: util.copy_schemas([ schema_name ], __sandbox_uri2, { "checksum": True, "ddlOnly": True, "ignoreExistingObjects": True, "includeTables": [ quote_identifier(schema_name, test_table_primary) ], "showProgress": False  }), "Error: Shell Error (53031): Util.copy_schemas: Checksum verification failed")
EXPECT_STDOUT_CONTAINS(f"Checksum verification failed for: `{schema_name}`.`{test_table_primary}`.")

# WL15947-TSFR_3_2_12 - tables are empty - checksum errors
WIPE_OUTPUT()
tgt_session.run_sql("TRUNCATE TABLE !.!", [ schema_name, test_table_primary ])
EXPECT_THROWS(lambda: util.copy_schemas([ schema_name ], __sandbox_uri2, { "checksum": True, "ddlOnly": True, "ignoreExistingObjects": True, "includeTables": [ quote_identifier(schema_name, test_table_primary) ], "showProgress": False  }), "Error: Shell Error (53031): Util.copy_schemas: Checksum verification failed")
EXPECT_STDOUT_CONTAINS(f"Checksum verification failed for: `{schema_name}`.`{test_table_primary}`. Mismatched number of rows, expected: 4, actual: 0.")

#@<> WL15947-TSFR_3_2_16 - create primary key in target instance {gipk_supported}
EXPECT_SUCCESS(__sandbox_uri2, { "checksum": True, "compatibility": ["create_invisible_pks"], "where": { quote_identifier(schema_name, test_table_no_index): "id = 1" }, "includeTables": [ quote_identifier(schema_name, test_table_no_index) ] }, schemas = [ schema_name ])

#@<> WL15947-TSFR_3_2_17 - timestamp and servers in different timezones
src_session.run_sql("SET @@GLOBAL.time_zone = '+9:00'")
EXPECT_SUCCESS(__sandbox_uri2, { "checksum": True, "includeTables": [ quote_identifier(schema_name, test_table_timestamp) ] }, schemas = [ schema_name ])

#@<> WL15947 - restore time_zone variable
src_session.run_sql("SET @@GLOBAL.time_zone = SYSTEM")

#@<> WL15947 - cleanup
src_session.run_sql("DROP SCHEMA IF EXISTS !;", [schema_name])

#@<> Cleanup
cleanup_copy_tests()
