#@<> INCLUDE dump_utils.inc
#@<> INCLUDE copy_utils.inc

#@<> entry point

# helpers
def EXPECT_SUCCESS(connectionData, options = {}, src = __sandbox_uri1, setup = None):
    # WL15298_TSFR_1_3
    EXPECT_SUCCESS_IMPL(util.copy_instance, connectionData, options, src, setup)

def EXPECT_FAIL(error, msg, connectionData, options = {}, src = __sandbox_uri1, setup = None):
    EXPECT_FAIL_IMPL(util.copy_instance, error, msg, connectionData, options, src, setup)

#@<> Setup
setup_copy_tests(2)
create_test_user(src_session)

#@<> WL15298_TSFR_1_2
EXPECT_THROWS(lambda: util.copy_instance(), "ValueError: Invalid number of arguments, expected 1 to 2 but got 0")

#@<> WL15298_TSFR_1_1_1
for connection_options in [None, 1, [], False]:
    EXPECT_FAIL("TypeError", "Argument #1: Invalid connection options, expected either a URI or a Connection Options Dictionary", connection_options)

#@<> WL15298_TSFR_1_1_2
EXPECT_FAIL("DBError: MySQL Error (1045)", "Access denied for user 'root'@'localhost' (using password: YES)", { **shell.parse_uri(__sandbox_uri2), "password": "wrong" })

#@<> WL15298_TSFR_1_1_1_1
EXPECT_FAIL("ValueError", "Argument #1: Invalid SSH Identity file: Only absolute paths are accepted, the path 'wrong' looks like relative one.", { **shell.parse_uri(__sandbox_uri2), "ssh-identity-file": "wrong" })

#@<> WL15298_TSFR_1_2_1
EXPECT_SUCCESS(__sandbox_uri2)
EXPECT_STDOUT_CONTAINS("accounts were loaded")

#@<> WL15298_TSFR_1_2_2
for options in [1, [], "string"]:
    EXPECT_FAIL("TypeError", f"Argument #{options_arg_no} is expected to be a map", __sandbox_uri2, options)

#@<> WL15298_TSFR_1_2_1_2
EXPECT_SUCCESS(__sandbox_uri2, { "excludeSchemas": [ "sakila" ] })
EXPECT_STDOUT_CONTAINS("No data loaded.")

#@<> WL15298_TSFR_1_2_1_3
TEST_ARRAY_OF_STRINGS_OPTION("excludeSchemas")

#@<> WL15298_TSFR_1_2_1_4
EXPECT_SUCCESS(__sandbox_uri2, { "excludeSchemas": [ "wrong" ] })
EXPECT_STDOUT_NOT_CONTAINS("excludeSchemas")

#@<> WL15298 - test invalid values of includeSchemas option
TEST_ARRAY_OF_STRINGS_OPTION("includeSchemas")

#@<> WL15298 - test includeSchemas option
EXPECT_SUCCESS(__sandbox_uri2, { "includeSchemas": [ "sakila" ] })

if instance_supports_libraries:
    # WL16731-G3 - default value of the libraries option
    # WL16731-TSFR_1_2_1_2 - default value of the includeLibraries option
    # WL16731-TSFR_1_3_1_1 - default value of the excludeLibraries option
    EXPECT_STDOUT_MATCHES(re.compile(r"1 out of \d schemas will be dumped and within them 16 tables, 7 views, 1 event, 1 library, 8 routines, 6 triggers"))
    # WL16731-TSFR_1_1_2_2 - target version supports libraries, warning is not printed
    EXPECT_STDOUT_NOT_CONTAINS(warn_target_version_does_not_support_libraries(__version))
else:
    EXPECT_STDOUT_MATCHES(re.compile(r"1 out of \d schemas will be dumped and within them 16 tables, 7 views, 1 event, 6 routines, 6 triggers"))

EXPECT_STDOUT_CONTAINS("16 tables in 1 schemas were loaded")

#@<> WL15298_TSFR_1_2_1_5
EXPECT_SUCCESS(__sandbox_uri2, { "includeSchemas": [ "wrong" ] })
EXPECT_STDOUT_NOT_CONTAINS("includeSchemas")

#@<> WL15298 - test invalid values of users option
TEST_BOOL_OPTION("users")

#@<> WL15298_TSFR_1_2_1_6
EXPECT_SUCCESS(__sandbox_uri2, { "users": True })
EXPECT_STDOUT_CONTAINS("accounts were loaded")

#@<> WL15298 - test users option
EXPECT_SUCCESS(__sandbox_uri2, { "users": False })
EXPECT_STDOUT_NOT_CONTAINS("users")
EXPECT_STDOUT_NOT_CONTAINS("accounts")

#@<> WL15298 - test invalid values of excludeUsers option
TEST_ARRAY_OF_STRINGS_OPTION("excludeUsers")

#@<> WL15298_TSFR_1_2_1_7
EXPECT_FAIL("ValueError", f"Argument #{options_arg_no}: The 'excludeUsers' option cannot be used if the 'users' option is set to false.", __sandbox_uri2, { "users": False, "excludeUsers": [ "'root'@'%'" ] })

#@<> WL15298 - test invalid values of includeUsers option
TEST_ARRAY_OF_STRINGS_OPTION("includeUsers")

#@<> WL15298_TSFR_1_2_1_8
EXPECT_SUCCESS(__sandbox_uri2, { "includeUsers": [ test_user_account ], "ddlOnly": True})
EXPECT_STDOUT_MATCHES(re.compile(r"1 out of \d users will be dumped"))
EXPECT_STDOUT_CONTAINS("1 accounts were loaded")

#@<> WL15298_TSFR_1_3_1
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
# WL15298_TSFR_1_3_2
# src is an X session
EXPECT_SUCCESS(__sandbox_uri2, src = get_ssl_config(src_session, get_x_config(src_session, __sandbox_uri1)))
# WL15298_TSFR_1_3_3
# src is using SSL
EXPECT_SUCCESS(__sandbox_uri2, { "excludeUsers": [ "root" ] }, src = get_ssl_config(src_session, test_user_uri(__mysql_sandbox_port1)))
# WL15298_TSFR_4_1_2_1
EXPECT_STDOUT_CONTAINS("Running data dump using 4 threads")

#@<> WL15298_TSFR_4_1_3_2
# WL15298_TSFR_1_3_2
# tgt is an X session
EXPECT_SUCCESS(get_x_config(tgt_session, __sandbox_uri2))
# WL15298_TSFR_1_3_3
# tgt is using SSL
# the user is copied from the source server to make sure that the CREATE USER statements for validation are the same
create_user = src_session.run_sql(f"SHOW CREATE USER {test_user_account}").fetch_one()[0]
EXPECT_SUCCESS(get_ssl_config(tgt_session, test_user_uri(__mysql_sandbox_port2)), { "excludeUsers": [ "root" ] }, setup = lambda: tgt_session.run_sql(create_user) and tgt_session.run_sql(f"GRANT ALL ON *.* TO {test_user_account}"))

#@<> WL15298_TSFR_4_4_5
TEST_ARRAY_OF_STRINGS_OPTION("compatibility")

#@<> WL15298_TSFR_4_4_6
EXPECT_SUCCESS(__sandbox_uri2, { "compatibility": [] })

#@<> WL15298_TSFR_4_4_7
EXPECT_FAIL("ValueError", f"Argument #{options_arg_no}: Unknown compatibility option: unknown_compat_mode", __sandbox_uri2, { "compatibility": [ "unknown_compat_mode" ] })

#@<> WL15298_TSFR_4_4_8 {VER(>=8.0.24)}
# this tests that compatibility mode is recognized (there's no error)
EXPECT_SUCCESS(__sandbox_uri2, { "compatibility": [ "create_invisible_pks" ], "ddlOnly": True })

#@<> WL15298_TSFR_4_4_9
# WL15298_TSFR_4_4_10
# WL15298_TSFR_4_4_12
# WL15298_TSFR_4_4_13
# WL15298_TSFR_4_4_14
# WL15298_TSFR_4_4_15

# NOTE: targetVersion cannot be lower than 8.0.25 and higher than the current version
if __version_num < 80025:
    target_version = "8.0.25"
elif __version_num > __mysh_version_num:
    target_version = __mysh_version
else:
    target_version = __version

# this tests that compatibility mode is recognized and some of them are applied
EXPECT_SUCCESS(__sandbox_uri2, { "targetVersion": target_version, "compatibility": [ "force_innodb", "ignore_missing_pks", "ignore_wildcard_grants", "skip_invalid_accounts", "strip_definers", "strip_invalid_grants", "strip_restricted_grants", "strip_tablespaces" ] })
EXPECT_STDOUT_CONTAINS(f"User {test_user_account} had restricted privileges")
EXPECT_STDOUT_CONTAINS("Function `sakila`.`get_customer_balance` had definer clause removed")
EXPECT_STDOUT_CONTAINS("View `sakila`.`sales_by_store` had SQL SECURITY characteristic set to INVOKER")

# BUG#35740174 - note about targetVersion was printed even if target server did not support the SET_ANY_DEFINER privilege
if __version_num < 80000:
    EXPECT_STDOUT_NOT_CONTAINS("'targetVersion'")

#@<> WL15298_TSFR_4_4_19
EXPECT_SUCCESS(__sandbox_uri2, { "triggers": True, "ddlOnly": True })

#@<> WL15298_TSFR_4_4_20
EXPECT_SUCCESS(__sandbox_uri2, { "triggers": False, "ddlOnly": True })

#@<> WL15298_TSFR_4_4_21
TEST_BOOL_OPTION("triggers")

#@<> WL15298 - test invalid values of excludeTriggers option
TEST_ARRAY_OF_STRINGS_OPTION("excludeTriggers")

#@<> WL15298_TSFR_4_4_22
EXPECT_SUCCESS(__sandbox_uri2, { "excludeTriggers": [], "ddlOnly": True })

#@<> WL15298_TSFR_4_4_23
EXPECT_SUCCESS(__sandbox_uri2, { "excludeTriggers": [ "sakila.customer", "sakila.wrong", "wrong.wrong", "sakila.film.ins_film", "sakila.film.wrong" ], "ddlOnly": True })

#@<> WL15298 - test invalid values of includeTriggers option
TEST_ARRAY_OF_STRINGS_OPTION("includeTriggers")

#@<> WL15298_TSFR_4_4_27
EXPECT_SUCCESS(__sandbox_uri2, { "includeTriggers": [], "ddlOnly": True })

#@<> WL15298_TSFR_4_4_28
EXPECT_SUCCESS(__sandbox_uri2, { "includeTriggers": [ "sakila.customer", "sakila.wrong", "wrong.wrong", "sakila.film.ins_film", "sakila.film.wrong" ], "ddlOnly": True })

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
EXPECT_SUCCESS(__sandbox_uri2, { "dataOnly": True }, setup = lambda: util.copy_instance(__sandbox_uri2, { "ddlOnly": True, "triggers": False, "users": False, "showProgress": False }))

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
EXPECT_SUCCESS(__sandbox_uri2, { "maxBytesPerTransaction": "1M" }, setup = lambda: WIPE_SHELL_LOG())
# WL15298_TSFR_4_5_7
EXPECT_STDOUT_NOT_CONTAINS("Analyzing tables")
# WL15298_TSFR_4_6_1
p = re.compile(r"""
.*---
Dump_metadata:
  Binlog_file: .*
  Binlog_position: .*
  Executed_GTID_set: .*
""")
EXPECT_STDOUT_MATCHES(p)
# BUG#35883344 - binlog info should be written to the log file
EXPECT_SHELL_LOG_MATCHES(p)

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
EXPECT_STDOUT_CONTAINS("Building indexes")

#@<> WL15298_TSFR_4_5_15
EXPECT_SUCCESS(__sandbox_uri2, { "deferTableIndexes": "fulltext" })
# NOTE: functionality is checked in load tests
EXPECT_STDOUT_CONTAINS("Building indexes")

#@<> WL15298_TSFR_4_5_16
EXPECT_SUCCESS(__sandbox_uri2, { "deferTableIndexes": "off" })
EXPECT_STDOUT_NOT_CONTAINS("Building indexes")

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
EXPECT_NO_THROWS(lambda: util.copy_instance(__sandbox_uri2, { "users": False, "showProgress": False }), "copy should not throw")
EXPECT_NO_THROWS(lambda: util.copy_instance(__sandbox_uri2, { "ignoreExistingObjects": True, "users": False, "showProgress": False }), "copy should not throw")
EXPECT_STDOUT_CONTAINS("One or more objects in the dump already exist in the destination database but will be ignored because the 'ignoreExistingObjects' option was enabled.")

#@<> WL15298_TSFR_4_5_19
wipeout_server(tgt_session)
setup_session(__sandbox_uri1)
WIPE_STDOUT()
EXPECT_NO_THROWS(lambda: util.copy_instance(__sandbox_uri2, { "users": False, "showProgress": False }), "copy should not throw")
EXPECT_FAIL("Error: Shell Error (53021)", re.compile(r"While '.*': Duplicate objects found in destination database"), __sandbox_uri2, { "ignoreExistingObjects": "False" })

#@<> WL15298 - test invalid values of ignoreVersion option
TEST_BOOL_OPTION("ignoreVersion")

#@<> WL15298_TSFR_4_5_22
TEST_BOOL_OPTION("loadIndexes")

#@<> WL15298_TSFR_4_5_23
EXPECT_FAIL("ValueError", f"Argument #{options_arg_no} 'deferTableIndexes' option needs to be enabled when 'loadIndexes' option is disabled", __sandbox_uri2, { "loadIndexes": False, "deferTableIndexes": "off" })

#@<> WL15298_TSFR_4_5_24
EXPECT_SUCCESS(__sandbox_uri2, { "loadIndexes": False, "deferTableIndexes": "all" })
EXPECT_STDOUT_NOT_CONTAINS("Building indexes")

#@<> WL15298_TSFR_4_5_25
EXPECT_SUCCESS(__sandbox_uri2, { "loadIndexes": True, "deferTableIndexes": "all" })
EXPECT_STDOUT_CONTAINS("Building indexes")

#@<> WL15298_TSFR_4_5_26
TEST_STRING_OPTION("schema")

#@<> WL15298_TSFR_4_5_29
# WL16731 - we need to explicitly exclude the routines which are using a library, as we rewrite just some basic SQL statements, and not the `USING` clause of such routines
EXPECT_SUCCESS(__sandbox_uri2, { "includeSchemas": [ "sakila" ], "schema": "sakila-new", "excludeRoutines": ["sakila.existing_library_function", "sakila.existing_library_procedure"] })

if instance_supports_libraries:
    # WL16731 - with routines present, copy fails
    wipeout_server(tgt_session)
    EXPECT_FAIL("Error: Shell Error (53005)", "Error loading dump", __sandbox_uri2, { "includeSchemas": [ "sakila" ], "schema": "sakila-new", "users": False })
    EXPECT_STDOUT_CONTAINS("While executing DDL for routines of schema `sakila-new`: LIBRARY existing_library does not exist")

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
EXPECT_SUCCESS(__sandbox_uri2, { "excludeTables": [ "sakila.actor_info", "`sakila`.`actor2`", "wrong.wrong", "`sakila`.`sales_by_film_category`" ] })

#@<> WL15298_TSFR_5_1_6
EXPECT_FAIL("ValueError", f"Argument #{options_arg_no}: Failed to parse table to be excluded 'wrong.@': Invalid character in identifier", __sandbox_uri2, { "excludeTables": [ "wrong.@" ] })

#@<> WL15298_TSFR_5_1_8
TEST_ARRAY_OF_STRINGS_OPTION("excludeTables")

#@<> WL15298_TSFR_5_1_9
EXPECT_SUCCESS(__sandbox_uri2, { "includeTables": [] })

#@<> WL15298_TSFR_5_1_10
EXPECT_SUCCESS(__sandbox_uri2, { "includeTables": [ "sakila.customer", "sakila.address", "sakila.city", "sakila.country", "`sakila`.`actor2`", "wrong.wrong", "`sakila`.`customer_list`" ] })

#@<> WL15298 - test invalid values of includeTables option
TEST_ARRAY_OF_STRINGS_OPTION("includeTables")

#@<> WL15298_TSFR_5_1_11
EXPECT_SUCCESS(__sandbox_uri2, { "events": True, "ddlOnly": True })

#@<> WL15298_TSFR_5_1_12
EXPECT_SUCCESS(__sandbox_uri2, { "events": False, "ddlOnly": True })

#@<> WL15298_TSFR_5_1_13
TEST_BOOL_OPTION("events")

#@<> WL15298_TSFR_5_1_14
EXPECT_SUCCESS(__sandbox_uri2, { "excludeEvents": [], "ddlOnly": True })

#@<> WL15298_TSFR_5_1_16
EXPECT_FAIL("ValueError", f"Argument #{options_arg_no}: The event to be excluded must be in the following form: schema.event, with optional backtick quotes, wrong value: 'event'.", __sandbox_uri2, { "excludeEvents": [ "event" ] })

#@<> WL15298_TSFR_5_1_17
EXPECT_SUCCESS(__sandbox_uri2, { "excludeEvents": [ "sakila.existing_event", "`sakila`.`wrong`", "wrong.wrong" ], "ddlOnly": True })

#@<> WL15298 - test invalid values of excludeEvents option
TEST_ARRAY_OF_STRINGS_OPTION("excludeEvents")

#@<> WL15298_TSFR_5_1_20
EXPECT_SUCCESS(__sandbox_uri2, { "includeEvents": [], "ddlOnly": True })

#@<> WL15298_TSFR_5_1_21
EXPECT_SUCCESS(__sandbox_uri2, { "includeEvents": [ "sakila.existing_event", "`sakila`.`wrong`", "wrong.wrong" ], "ddlOnly": True })

#@<> WL15298 - test invalid values of includeEvents option
TEST_ARRAY_OF_STRINGS_OPTION("includeEvents")

#@<> WL15298_TSFR_5_1_23
EXPECT_SUCCESS(__sandbox_uri2, { "routines": True, "ddlOnly": True })

#@<> WL15298 - test routines option
EXPECT_SUCCESS(__sandbox_uri2, { "routines": False, "ddlOnly": True })

#@<> WL15298_TSFR_5_1_24
TEST_BOOL_OPTION("routines")

#@<> WL15298_TSFR_5_1_25
EXPECT_SUCCESS(__sandbox_uri2, { "excludeRoutines": [], "ddlOnly": True })

#@<> WL15298_TSFR_5_1_26
EXPECT_SUCCESS(__sandbox_uri2, { "excludeRoutines": [ "`sakila`.`rewards_report`", "sakila.get_customer_balance", "`sakila`.`wrong`", "wrong.wrong" ], "ddlOnly": True })

#@<> WL15298 - test invalid values of excludeRoutines option
TEST_ARRAY_OF_STRINGS_OPTION("excludeRoutines")

#@<> WL15298 - test includeRoutines option
EXPECT_SUCCESS(__sandbox_uri2, { "includeRoutines": [], "ddlOnly": True })

#@<> WL15298_TSFR_5_1_29
EXPECT_SUCCESS(__sandbox_uri2, { "includeRoutines": [ "`sakila`.`rewards_report`", "sakila.get_customer_balance", "`sakila`.`wrong`", "wrong.wrong" ], "ddlOnly": True })

#@<> WL15298 - test invalid values of includeRoutines option
TEST_ARRAY_OF_STRINGS_OPTION("includeRoutines")

#@<> WL16731 - help text
# WL16731-G1
# WL16731-TSFR_1_2_1
# WL16731-TSFR_1_3_1
help_text = """
      - libraries: bool (default: true) - Include library objects for each
        copied schema.
      - excludeLibraries: list of strings (default: empty) - List of library
        objects to be excluded from the copy in the format of schema.library.
      - includeLibraries: list of strings (default: empty) - List of library
        objects to be included in the copy in the format of schema.library.
"""
EXPECT_TRUE(help_text in util.help("copy_instance"))

#@<> WL16731 - enable libraries option
# WL16731-G2
# WL16731-TSFR_1_1_1 - test is executed regardless of server version
EXPECT_SUCCESS(__sandbox_uri2, { "libraries": True })

#@<> WL16731 - disable libraries option
# WL16731-G2
EXPECT_SUCCESS(__sandbox_uri2, { "libraries": False })

if instance_supports_libraries:
    # WL16731-TSFR_1_7_1
    EXPECT_STDOUT_CONTAINS(routine_references_excluded_library("Function", "sakila", "existing_library_function", "sakila", "existing_library").note(True))
    EXPECT_STDOUT_CONTAINS(routine_references_excluded_library("Procedure", "sakila", "existing_library_procedure", "sakila", "existing_library").note(True))

#@<> WL16731 - test invalid values of libraries option
# WL16731-G1
TEST_BOOL_OPTION("libraries")

#@<> WL16731 - test excludeLibraries option with an empty array
# WL16731-TSFR_1_3_1
# WL16731-TSFR_1_3_1_1
EXPECT_SUCCESS(__sandbox_uri2, { "excludeLibraries": [] })

#@<> WL16731 - test excludeLibraries option with existing and non-existing libraries
# WL16731-TSFR_1_3_1
# WL16731-TSFR_1_4_1
# WL16731-TSFR_1_5_1
EXPECT_SUCCESS(__sandbox_uri2, { "excludeLibraries": [ "`sakila`.`existing_library`", "`sakila`.`wrong`", "wrong.wrong" ] })

#@<> WL16731 - test invalid values of excludeLibraries option
# WL16731-TSFR_1_3_1
TEST_ARRAY_OF_STRINGS_OPTION("excludeLibraries")

#@<> WL16731-TSFR_1_4_2 - invalid format of excludeLibraries entries
EXPECT_FAIL("ValueError", "Argument #2: The library to be excluded must be in the following form: schema.library, with optional backtick quotes, wrong value: 'library'.", __sandbox_uri2, { "excludeLibraries": [ "library" ] })
EXPECT_FAIL("ValueError", "Argument #2: Failed to parse library to be excluded 'schema.@': Invalid character in identifier", __sandbox_uri2, { "excludeLibraries": [ "schema.@" ] })

#@<> WL16731 - test includeLibraries option
# WL16731-G4
# WL16731-TSFR_1_2_1_2
EXPECT_SUCCESS(__sandbox_uri2, { "includeLibraries": [] })

#@<> WL16731 - test includeLibraries option with existing and non-existing libraries
# WL16731-G4
# WL16731-TSFR_1_4_1
# WL16731-TSFR_1_5_1
EXPECT_SUCCESS(__sandbox_uri2, { "includeLibraries": [ "`sakila`.`existing_library`", "`sakila`.`wrong`", "wrong.wrong" ] })

#@<> WL16731 - test invalid values of includeLibraries option
# WL16731-TSFR_1_2_1
TEST_ARRAY_OF_STRINGS_OPTION("includeLibraries")

#@<> WL16731-TSFR_1_4_2 - invalid format of includeLibraries entries
EXPECT_FAIL("ValueError", "Argument #2: The library to be included must be in the following form: schema.library, with optional backtick quotes, wrong value: 'library'.", __sandbox_uri2, { "includeLibraries": [ "library" ] })
EXPECT_FAIL("ValueError", "Argument #2: Failed to parse library to be included 'schema.@': Invalid character in identifier", __sandbox_uri2, { "includeLibraries": [ "schema.@" ] })

#@<> WL16731-TSFR_1_6_1 - conflicting filtering options
EXPECT_FAIL("ValueError", "Argument #2: Conflicting filtering options", __sandbox_uri2, { "includeLibraries": [ "`sakila`.`existing_library`" ], "excludeLibraries": [ "sakila.existing_library" ] })
EXPECT_STDOUT_CONTAINS("Both includeLibraries and excludeLibraries options contain a library `sakila`.`existing_library`.")

#@<> WL16731-TSFR_1_7_2 - `libraries` - routine uses a library which is not included {instance_supports_libraries}
# constants
tested_schema = "wl16731"
tested_function = "my_function"
tested_procedure = "my_procedure"

tested_library_schema = "wl16731-lib"
tested_library = "my_library"

# setup
session.run_sql("DROP SCHEMA IF EXISTS !", [tested_schema])
session.run_sql("DROP SCHEMA IF EXISTS !", [tested_library_schema])
session.run_sql("CREATE SCHEMA !", [tested_schema])
session.run_sql("CREATE SCHEMA !", [tested_library_schema])
session.run_sql("CREATE LIBRARY !.! LANGUAGE JAVASCRIPT AS $$ $$", [tested_library_schema, tested_library])
session.run_sql("CREATE FUNCTION !.!() RETURNS INTEGER DETERMINISTIC LANGUAGE JAVASCRIPT USING (!.!) AS $$ $$", [tested_schema, tested_function, tested_library_schema, tested_library])
session.run_sql("CREATE PROCEDURE !.!() LANGUAGE JAVASCRIPT USING (!.!) AS $$ $$", [tested_schema, tested_procedure, tested_library_schema, tested_library])

# test
EXPECT_SUCCESS(__sandbox_uri2, { "includeSchemas": [tested_schema], "showProgress": False })

# SRC
EXPECT_STDOUT_CONTAINS(routine_references_excluded_library("Function", tested_schema, tested_function, tested_library_schema, tested_library).note(True))
EXPECT_STDOUT_CONTAINS(routine_references_excluded_library("Procedure", tested_schema, tested_procedure, tested_library_schema, tested_library).note(True))

# TGT
# WL16731-TSFR_2_6_1 - warn about unavailable MLE component
EXPECT_STDOUT_CONTAINS("WARNING: TGT: The dump contains library DDL, but the Multilingual Engine (MLE) component is not installed on the target instance. Routines which use these libraries will fail to execute, unless this component is installed.")

# routines are not loaded
EXPECT_STDOUT_CONTAINS(routine_uses_missing_library("function", tested_schema, tested_function, tested_library_schema, tested_library))
EXPECT_STDOUT_CONTAINS(routine_uses_missing_library("procedure", tested_schema, tested_procedure, tested_library_schema, tested_library))

EXPECT_STDOUT_CONTAINS(routine_skipped_due_to_missing_deps("function", tested_schema, tested_function))
EXPECT_STDOUT_CONTAINS(routine_skipped_due_to_missing_deps("procedure", tested_schema, tested_procedure))

# cleanup
session.run_sql("DROP SCHEMA IF EXISTS !", [tested_schema])
session.run_sql("DROP SCHEMA IF EXISTS !", [tested_library_schema])

#@<> WL16731-TSFR_1_7_1_2 - `libraries` - routine uses a library which does not exist {instance_supports_libraries}
# constants
tested_schema = "wl16731"
tested_library = "my_library"
tested_function = "my_function"
tested_procedure = "my_procedure"

# setup
session.run_sql("DROP SCHEMA IF EXISTS !", [tested_schema])
session.run_sql("CREATE SCHEMA !", [tested_schema])
session.run_sql("CREATE LIBRARY !.! LANGUAGE JAVASCRIPT AS $$ $$", [tested_schema, tested_library])
session.run_sql("CREATE FUNCTION !.!() RETURNS INTEGER DETERMINISTIC LANGUAGE JAVASCRIPT USING (!.!) AS $$ $$", [tested_schema, tested_function, tested_schema, tested_library])
session.run_sql("CREATE PROCEDURE !.!() LANGUAGE JAVASCRIPT USING (!.!) AS $$ $$", [tested_schema, tested_procedure, tested_schema, tested_library])
session.run_sql("DROP LIBRARY !.!", [tested_schema, tested_library])

# test
EXPECT_SUCCESS(__sandbox_uri2, { "includeSchemas": [tested_schema], "showProgress": False })

# SRC
EXPECT_STDOUT_CONTAINS(routine_references_missing_library("Function", tested_schema, tested_function, tested_schema, tested_library).warning(True))
EXPECT_STDOUT_CONTAINS(routine_references_missing_library("Procedure", tested_schema, tested_procedure, tested_schema, tested_library).warning(True))

# TGT
EXPECT_STDOUT_CONTAINS(routine_uses_missing_library("function", tested_schema, tested_function, tested_schema, tested_library))
EXPECT_STDOUT_CONTAINS(routine_uses_missing_library("procedure", tested_schema, tested_procedure, tested_schema, tested_library))

EXPECT_STDOUT_CONTAINS(routine_skipped_due_to_missing_deps("function", tested_schema, tested_function))
EXPECT_STDOUT_CONTAINS(routine_skipped_due_to_missing_deps("procedure", tested_schema, tested_procedure))

# cleanup
session.run_sql("DROP SCHEMA IF EXISTS !", [tested_schema])

#@<> WL15298 - unknown options
for param in [
        "dummy",
        "all",
        "compression",
        "ocimds",
        "targetVersion",
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
        "ociAuth",
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
account_name = get_test_user_account("sample_account")
role_name = get_test_user_account("sample_role")

account_names = [ account_name, role_name ]

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
src_session.run_sql(f"CREATE USER {account_name} IDENTIFIED BY 'pass'")
src_session.run_sql(f"CREATE ROLE {role_name}")
testutil.dbug_set("+d,copy_utils_force_mds")

#@<> WL15887-TSFR_3_1_1 - restricted accounts {not __dbug_off and VER(>=8.2.0)}
for account in ["mysql.infoschema", "mysql.session", "mysql.sys", "ociadmin", "ocidbm", "ocirpl"]:
    account = f"`{account}`@`localhost`"
    setup_db(account)
    WIPE_OUTPUT()
    EXPECT_FAIL("Error: Shell Error (52004)", "Compatibility issues were found", __sandbox_uri2, { "dryRun": True, "includeSchemas": [ schema_name ], "users": False, "showProgress": False })
    EXPECT_STDOUT_CONTAINS(definer_clause_uses_restricted_user_name(schema_name, test_schema_event, "Event", account).error(True))
    EXPECT_STDOUT_CONTAINS(definer_clause_uses_restricted_user_name(schema_name, test_schema_function, "Function", account).error(True))
    EXPECT_STDOUT_CONTAINS(definer_clause_uses_restricted_user_name(schema_name, test_schema_procedure, "Procedure", account).error(True))
    EXPECT_STDOUT_CONTAINS(definer_clause_uses_restricted_user_name(schema_name, test_table_trigger, "Trigger", account).error(True))
    EXPECT_STDOUT_CONTAINS(definer_clause_uses_restricted_user_name(schema_name, test_view, "View", account).error(True))

# restore schema
setup_db(test_user_account)

#@<> WL15887-TSFR_3_2_1 - valid account {not __dbug_off and VER(>=8.2.0)}
EXPECT_SUCCESS(__sandbox_uri2, { "dryRun": True, "includeSchemas": [ schema_name ], "users": False, "showProgress": False })

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

#@<> WL15887-TSFR_3_3_1 - user does not exist/is excluded {not __dbug_off and VER(>=8.2.0)}
for account in [ account_name, "`invalid-account`@`localhost`" ]:
    setup_db(account)
    WIPE_OUTPUT()
    EXPECT_FAIL("Error: Shell Error (52004)", "Compatibility issues were found", __sandbox_uri2, { "includeUsers": [ test_user_account ], "dryRun": True, "includeSchemas": [ schema_name ], "users": True, "showProgress": False })
    EXPECT_STDOUT_CONTAINS(definer_clause_uses_unknown_account(schema_name, test_schema_event, "Event", account).warning(True))
    EXPECT_STDOUT_CONTAINS(definer_clause_uses_unknown_account(schema_name, test_schema_function, "Function", account).warning(True))
    EXPECT_STDOUT_CONTAINS(definer_clause_uses_unknown_account(schema_name, test_schema_procedure, "Procedure", account).warning(True))
    EXPECT_STDOUT_CONTAINS(definer_clause_uses_unknown_account(schema_name, test_table_trigger, "Trigger", account).warning(True))
    EXPECT_STDOUT_CONTAINS(definer_clause_uses_unknown_account(schema_name, test_view, "View", account).warning(True))

# restore schema
setup_db(test_user_account)

#@<> WL15887-TSFR_4_1 - note about strip_definers {not __dbug_off and VER(>=8.2.0)}
EXPECT_SUCCESS(__sandbox_uri2, { "compatibility": [ "strip_definers" ], "dryRun": True, "includeSchemas": [ schema_name ], "users": False, "showProgress": False })
EXPECT_STDOUT_CONTAINS(f"NOTE: The 'targetVersion' option is set to {__version}. This version supports the SET_ANY_DEFINER privilege, using the 'strip_definers' compatibility option is unnecessary.")

#@<> WL15887-TSFR_5_1 - user/role with SET_ANY_DEFINER {not __dbug_off and VER(>=8.2.0)}
for account in account_names:
    src_session.run_sql(f"GRANT SET_ANY_DEFINER ON *.* TO {account}")
    WIPE_OUTPUT()
    EXPECT_SUCCESS(__sandbox_uri2, { "compatibility": [ "strip_restricted_grants" ], "includeUsers": [ account ], "dryRun": True, "includeSchemas": [ schema_name ], "users": True, "showProgress": False, "ddlOnly": True })
    EXPECT_STDOUT_NOT_CONTAINS("SET_ANY_DEFINER")
    src_session.run_sql(f"REVOKE SET_ANY_DEFINER ON *.* FROM {account}")

#@<> WL15887-TSFR_6_1 - user/role with SET_USER_ID {not __dbug_off and VER(>=8.2.0) and VER(<8.0.24)}
for account in account_names:
    src_session.run_sql(f"GRANT SET_USER_ID ON *.* TO {account}")
    WIPE_OUTPUT()
    EXPECT_SUCCESS(__sandbox_uri2, { "compatibility": [ "strip_restricted_grants" ], "includeUsers": [ account ], "dryRun": True, "includeSchemas": [ schema_name ], "users": True, "showProgress": False, "ddlOnly": True })
    EXPECT_STDOUT_CONTAINS(strip_restricted_grants_set_user_id_replaced(account).fixed(True))
    src_session.run_sql(f"REVOKE SET_USER_ID ON *.* FROM {account}")

#@<> WL15887 - cleanup {not __dbug_off and VER(>=8.2.0)}
src_session.run_sql("DROP SCHEMA IF EXISTS !;", [schema_name])
src_session.run_sql(f"DROP USER {account_name}")
src_session.run_sql(f"DROP ROLE {role_name}")
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
EXPECT_TRUE(help_text in util.help("copy_instance"))

#@<> WL15947-TSFR_3_1_1 - copy without checksum option
EXPECT_SUCCESS(__sandbox_uri2, { "includeSchemas": [ schema_name ], "users": False })
EXPECT_STDOUT_NOT_CONTAINS("checksum")

#@<> WL15947-TSFR_3_1_1 - copy with checksum option set to False
EXPECT_SUCCESS(__sandbox_uri2, { "checksum": False, "includeSchemas": [ schema_name ], "users": False  })
EXPECT_STDOUT_NOT_CONTAINS("checksum")

#@<> WL15947-TSFR_3_1_2 - option type
TEST_BOOL_OPTION("checksum")

#@<> WL15947-TSFR_3_2_2 - "checksum": True, "ddlOnly": False, "chunking": True, not partitioned table
wipeout_server(tgt_session)
EXPECT_FAIL("Error: Shell Error (53031)", "Checksum verification failed", __sandbox_uri2, { "checksum": True, "ddlOnly": False, "chunking": True, "includeTables": [ quote_identifier(schema_name, test_table_primary) ], "users": False })
EXPECT_STDOUT_CONTAINS(f"Checksum verification failed for: `{schema_name}`.`{test_table_primary}` (chunk 0) (boundary:")

#@<> WL15947-TSFR_3_2_3 - "checksum": True, "ddlOnly": False, "chunking": True, partitioned table
wipeout_server(tgt_session)
EXPECT_FAIL("Error: Shell Error (53031)", "Checksum verification failed", __sandbox_uri2, { "checksum": True, "ddlOnly": False, "chunking": True, "includeTables": [ quote_identifier(schema_name, test_table_partitioned) ], "users": False })
EXPECT_STDOUT_CONTAINS(f"Checksum verification failed for: `{schema_name}`.`{test_table_partitioned}` partition `p0` (chunk 0) (boundary:")

#@<> WL15947-TSFR_3_2_4 - "checksum": True, "ddlOnly": False, "chunking": False, not partitioned table
wipeout_server(tgt_session)
EXPECT_FAIL("Error: Shell Error (53031)", "Checksum verification failed", __sandbox_uri2, { "checksum": True, "ddlOnly": False, "chunking": False, "includeTables": [ quote_identifier(schema_name, test_table_primary) ], "users": False })
EXPECT_STDOUT_CONTAINS(f"Checksum verification failed for: `{schema_name}`.`{test_table_primary}`.")

#@<> WL15947-TSFR_3_2_5 - "checksum": True, "ddlOnly": False, "chunking": False, partitioned table
wipeout_server(tgt_session)
EXPECT_FAIL("Error: Shell Error (53031)", "Checksum verification failed", __sandbox_uri2, { "checksum": True, "ddlOnly": False, "chunking": False, "includeTables": [ quote_identifier(schema_name, test_table_partitioned) ], "users": False })
EXPECT_STDOUT_CONTAINS(f"Checksum verification failed for: `{schema_name}`.`{test_table_partitioned}` partition `p0`.")

#@<> WL15947-TSFR_3_2_6 - "checksum": True, "ddlOnly": True, not partitioned table
for chunking in [ True, False ]:
    wipeout_server(tgt_session)
    EXPECT_FAIL("Error: Shell Error (53031)", "Checksum verification failed", __sandbox_uri2, { "checksum": True, "ddlOnly": True, "chunking": chunking, "includeTables": [ quote_identifier(schema_name, test_table_primary) ], "users": False })
    EXPECT_STDOUT_CONTAINS(f"Checksum verification failed for: `{schema_name}`.`{test_table_primary}`. Mismatched number of rows, ")

#@<> WL15947-TSFR_3_2_6 - "checksum": True, "ddlOnly": True, partitioned table
for chunking in [ True, False ]:
    wipeout_server(tgt_session)
    EXPECT_FAIL("Error: Shell Error (53031)", "Checksum verification failed", __sandbox_uri2, { "checksum": True, "ddlOnly": True, "chunking": chunking, "includeTables": [ quote_identifier(schema_name, test_table_partitioned) ], "users": False })
    EXPECT_STDOUT_CONTAINS(f"Checksum verification failed for: `{schema_name}`.`{test_table_partitioned}` partition `p0`. Mismatched number of rows, ")

#@<> WL15947-TSFR_3_2_7 - "checksum": True, table without an index
wipeout_server(tgt_session)
EXPECT_FAIL("Error: Shell Error (53031)", "Checksum verification failed", __sandbox_uri2, { "checksum": True, "includeTables": [ quote_identifier(schema_name, test_table_no_index) ], "users": False })
EXPECT_STDOUT_CONTAINS(f"Checksum verification failed for: `{schema_name}`.`{test_table_no_index}`.")

#@<> WL15947-TSFR_3_2_8 - "checksum": True, table with a non-NULL unique index
wipeout_server(tgt_session)
EXPECT_FAIL("Error: Shell Error (53031)", "Checksum verification failed", __sandbox_uri2, { "checksum": True, "includeTables": [ quote_identifier(schema_name, test_table_unique) ], "users": False })
EXPECT_STDOUT_CONTAINS(f"Checksum verification failed for: `{schema_name}`.`{test_table_unique}` (chunk 0) (boundary:")

#@<> WL15947-TSFR_3_2_9 - "checksum": True, GIPK {gipk_supported}
src_session.run_sql("SET @@GLOBAL.show_gipk_in_create_table_and_information_schema = OFF")
wipeout_server(tgt_session)
EXPECT_FAIL("Error: Shell Error (53031)", "Checksum verification failed", __sandbox_uri2, { "checksum": True, "includeTables": [ quote_identifier(schema_name, test_table_gipk) ], "users": False })
EXPECT_STDOUT_CONTAINS(f"Checksum verification failed for: `{schema_name}`.`{test_table_gipk}`.")

#@<> WL15947 - restore show_gipk_in_create_table_and_information_schema variable {gipk_supported}
src_session.run_sql("SET @@GLOBAL.show_gipk_in_create_table_and_information_schema = ON")

#@<> WL15947-TSFR_3_2_10 - where option
EXPECT_SUCCESS(__sandbox_uri2, { "checksum": True, "where": { quote_identifier(schema_name, test_table_primary): "id = 1" }, "includeTables": [ quote_identifier(schema_name, test_table_primary) ], "users": False })
EXPECT_STDOUT_CONTAINS("checksum")

#@<> WL15947-TSFR_3_2_11 - dryRun
EXPECT_SUCCESS(__sandbox_uri2, { "checksum": True, "dryRun": True, "includeTables": [ quote_identifier(schema_name, test_table_primary) ], "users": False })
EXPECT_STDOUT_CONTAINS("Checksum information is not going to be verified, dryRun enabled.")

#@<> WL15947 - copying when target already has data
# prepare target server
wipeout_server(tgt_session)
EXPECT_NO_THROWS(lambda: util.copy_tables(schema_name, [test_table_primary], __sandbox_uri2, { "showProgress": False }))

# WL15947-TSFR_3_2_13 - target has the same data
# WL15947-TSFR_3_2_14 - target already has other data
WIPE_OUTPUT()
EXPECT_THROWS(lambda: util.copy_instance(__sandbox_uri2, { "checksum": True, "ddlOnly": True, "ignoreExistingObjects": True, "includeTables": [ quote_identifier(schema_name, test_table_primary) ], "showProgress": False, "users": False  }), "Error: Shell Error (53031): Checksum verification failed")
EXPECT_STDOUT_CONTAINS(f"Checksum verification failed for: `{schema_name}`.`{test_table_primary}`.")

# WL15947-TSFR_3_2_12 - tables are empty - checksum errors
WIPE_OUTPUT()
tgt_session.run_sql("TRUNCATE TABLE !.!", [ schema_name, test_table_primary ])
EXPECT_THROWS(lambda: util.copy_instance(__sandbox_uri2, { "checksum": True, "ddlOnly": True, "ignoreExistingObjects": True, "includeTables": [ quote_identifier(schema_name, test_table_primary) ], "showProgress": False, "users": False  }), "Error: Shell Error (53031): Checksum verification failed")
EXPECT_STDOUT_CONTAINS(f"Checksum verification failed for: `{schema_name}`.`{test_table_primary}`. Mismatched number of rows, expected: 4, actual: 0.")

#@<> WL15947-TSFR_3_2_16 - create primary key in target instance {gipk_supported}
EXPECT_SUCCESS(__sandbox_uri2, { "checksum": True, "compatibility": ["create_invisible_pks"], "where": { quote_identifier(schema_name, test_table_no_index): "id = 1" }, "includeTables": [ quote_identifier(schema_name, test_table_no_index) ], "users": False })

#@<> WL15947-TSFR_3_2_17 - timestamp and servers in different timezones
src_session.run_sql("SET @@GLOBAL.time_zone = '+9:00'")
EXPECT_SUCCESS(__sandbox_uri2, { "checksum": True, "includeTables": [ quote_identifier(schema_name, test_table_timestamp) ], "users": False })

#@<> WL15947 - restore time_zone variable
src_session.run_sql("SET @@GLOBAL.time_zone = SYSTEM")

#@<> WL15947 - cleanup
src_session.run_sql("DROP SCHEMA IF EXISTS !;", [schema_name])

#@<> BUG#36561962 - add 'dropExistingObjects' option to drop existing objects before copying the data
test_schema = "test_schema"
test_user = "test_user"

# setup
def create_objects(s, schema):
    s.run_sql(f"DROP USER IF EXISTS {test_user}")
    s.run_sql(f"CREATE USER {test_user}")
    s.run_sql("DROP SCHEMA IF EXISTS !", [ schema ])
    s.run_sql("CREATE SCHEMA !", [ schema ])
    s.run_sql("CREATE TABLE !.t (c INT)", [ schema ])
    s.run_sql("CREATE VIEW !.v AS SELECT * FROM !.t", [ schema, schema ])
    s.run_sql("CREATE TRIGGER !.tt BEFORE INSERT ON t FOR EACH ROW BEGIN END", [ schema ])
    s.run_sql("CREATE FUNCTION !.f() RETURNS INT DETERMINISTIC RETURN 1", [ schema ])
    s.run_sql("CREATE PROCEDURE !.p() BEGIN END", [ schema ])
    s.run_sql("CREATE EVENT !.e ON SCHEDULE AT CURRENT_TIMESTAMP + INTERVAL 1 HOUR DO BEGIN END", [ schema ])
    if instance_supports_libraries:
        s.run_sql("CREATE LIBRARY !.l LANGUAGE JAVASCRIPT AS $$ $$", [ schema ])

def alter_objects(s, schema):
    s.run_sql(f"ALTER USER {test_user} IDENTIFIED BY 'password1@'")
    s.run_sql("ALTER TABLE !.t COMMENT 'modified'", [ schema ])
    s.run_sql("ALTER VIEW !.v AS SELECT 'modified'", [ schema ])
    s.run_sql("DROP TRIGGER !.tt", [ schema ])
    s.run_sql("CREATE TRIGGER !.tt AFTER DELETE ON t FOR EACH ROW BEGIN END", [ schema ])
    s.run_sql("ALTER FUNCTION !.f COMMENT 'modified'", [ schema ])
    s.run_sql("ALTER PROCEDURE !.p COMMENT 'modified'", [ schema ])
    s.run_sql("ALTER EVENT !.e COMMENT 'modified'", [ schema ])
    # TODO(pawel): uncomment this once this feature is available
    # s.run_sql("/*!90300 ALTER LIBRARY !.l COMMENT 'modified' */", [ schema ])

create_objects(src_session, test_schema)

#@<> BUG#36561962 - 'dropExistingObjects' is mutually exclusive with 'ignoreExistingObjects'
EXPECT_FAIL("ValueError: Argument #2", "The 'dropExistingObjects' and 'ignoreExistingObjects' options cannot be both set to true.", __sandbox_uri2, { "dropExistingObjects": True, "ignoreExistingObjects": True })

#@<> BUG#36561962 - 'dropExistingObjects' is mutually exclusive with 'dataOnly'
EXPECT_FAIL("ValueError: Argument #2", "The 'dropExistingObjects' and 'dataOnly' options cannot be both set to true.", __sandbox_uri2, { "dropExistingObjects": True, "dataOnly": True })

#@<> BUG#36561962 - copy normally, then copy with 'dropExistingObjects': True
wipeout_server(tgt_session)
EXPECT_NO_THROWS(lambda: util.copy_instance(__sandbox_uri2, { "includeSchemas": [ test_schema ], "includeUsers": [ test_user ], "showProgress": False }), "Copy should not throw")

schemas = snapshot_schemas(tgt_session)
accounts = snapshot_accounts(tgt_session)

alter_objects(tgt_session, test_schema)

WIPE_OUTPUT()
EXPECT_NO_THROWS(lambda: util.copy_instance(__sandbox_uri2, { "dropExistingObjects": True, "includeSchemas": [ test_schema ], "includeUsers": [ test_user ], "showProgress": False }), "Copy should not throw")

EXPECT_STDOUT_CONTAINS("TGT: Checking for pre-existing objects...")
EXPECT_STDOUT_CONTAINS(f"NOTE: TGT: Account '{test_user}'@'%' already exists, dropping...")
EXPECT_STDOUT_CONTAINS(f"NOTE: TGT: Schema `{test_schema}` already contains a table named `t`, dropping...")
EXPECT_STDOUT_CONTAINS(f"NOTE: TGT: Schema `{test_schema}` already contains a view named `v`, dropping...")
EXPECT_STDOUT_CONTAINS(f"NOTE: TGT: Schema `{test_schema}` already contains a trigger named `tt`, dropping...")
EXPECT_STDOUT_CONTAINS(f"NOTE: TGT: Schema `{test_schema}` already contains a function named `f`, dropping...")
EXPECT_STDOUT_CONTAINS(f"NOTE: TGT: Schema `{test_schema}` already contains a procedure named `p`, dropping...")
EXPECT_STDOUT_CONTAINS(f"NOTE: TGT: Schema `{test_schema}` already contains an event named `e`, dropping...")
EXPECT_STDOUT_CONTAINS("NOTE: TGT: One or more objects in the dump already exist in the destination database but will be dropped because the 'dropExistingObjects' option was enabled.")

if instance_supports_libraries:
    EXPECT_STDOUT_CONTAINS(f"NOTE: TGT: Schema `{test_schema}` already contains a library named `l`, dropping...")

EXPECT_JSON_EQ(schemas, snapshot_schemas(tgt_session), "Verifying schemas")
EXPECT_JSON_EQ(accounts, snapshot_accounts(tgt_session), "Verifying accounts")

#@<> BUG#36561962 - cleanup
src_session.run_sql("DROP SCHEMA IF EXISTS !", [ test_schema ])
src_session.run_sql(f"DROP USER IF EXISTS {test_user}")

#@<> Cleanup
cleanup_copy_tests()
