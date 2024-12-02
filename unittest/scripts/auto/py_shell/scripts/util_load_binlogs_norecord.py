#@<> INCLUDE dump_utils.inc
#@<> INCLUDE dump_binlogs_utils.inc

#@<> entry point
# constants
empty_dump_dir = "empty_dump"

# helpers
def EXPECT_SUCCESS(*args, **kwargs):
    EXPECT_LOAD_SUCCESS(*args, **kwargs)

def EXPECT_FAIL(*args, **kwargs):
    EXPECT_LOAD_FAIL(*args, **kwargs)

#@<> WL15977-FR3.3 - an exception shall be thrown if there is no global session
EXPECT_FAIL("RuntimeError", "An open session is required to perform this operation.")

#@<> deploy sandbox
testutil.deploy_raw_sandbox(__mysql_sandbox_port1, "root", {
    "server-id": str(random.randint(1, 4294967295)),
    **mysqld_options,
})
testutil.wait_sandbox_alive(__sandbox_uri1)

testutil.deploy_raw_sandbox(__mysql_sandbox_port2, "root", {
    "server-id": str(random.randint(1, 4294967295)),
    "local_infile": 1,
    **mysqld_options,
})
testutil.wait_sandbox_alive(__sandbox_uri2)

#@<> WL15977-FR3.3 - an exception shall be thrown if there is no open global session
shell.connect(load_uri)
session.close()
EXPECT_FAIL("RuntimeError", "An open session is required to perform this operation.")

#@<> Setup
setup_session(dump_uri)

wipe_dir(default_dump_dir)
wipe_dir(default_since_dir)
wipe_dir(incremental_dump_dir)
wipe_dir(empty_dump_dir)

EXPECT_DUMP_SUCCESS(empty_dump_dir, options={"startFrom": start_from_beginning})

create_default_test_table(session)

DUMP_INSTANCE()
EXPECT_DUMP_SUCCESS(incremental_dump_dir, options={"startFrom": start_from_beginning})

create_default_test_table(session)

EXPECT_DUMP_SUCCESS(default_dump_dir, options={"since": default_since_dir})
EXPECT_DUMP_SUCCESS(incremental_dump_dir)

for i in range(3):
    create_default_test_table(session)
    EXPECT_DUMP_SUCCESS(default_dump_dir)
    EXPECT_DUMP_SUCCESS(incremental_dump_dir)

expected_gtid_set = binary_log_status(session).executed_gtid_set
server_uuid = expected_gtid_set[0:expected_gtid_set.find(':')]
source_binary_logs = binary_logs(session)

setup_session(load_uri)

#@<> WL15977-FR3.1 - first parameter is a string
EXPECT_THROWS(lambda: util.load_binlogs(None),  "TypeError: Argument #1 is expected to be a string")
EXPECT_THROWS(lambda: util.load_binlogs(1),     "TypeError: Argument #1 is expected to be a string")
EXPECT_THROWS(lambda: util.load_binlogs([]),    "TypeError: Argument #1 is expected to be a string")
EXPECT_THROWS(lambda: util.load_binlogs({}),    "TypeError: Argument #1 is expected to be a string")
EXPECT_THROWS(lambda: util.load_binlogs(False), "TypeError: Argument #1 is expected to be a string")

#@<> WL15977-FR3.1.1.1 - input directory does not exist
missing_dir = "unknown"
absolute_missing_dir = absolute_path_for_output(missing_dir)
EXPECT_FAIL("ValueError", f"Directory '{absolute_missing_dir}' does not exist.", missing_dir)

#@<> WL15977-FR3.1.1.2 - input directory is empty
os.mkdir(missing_dir)

EXPECT_FAIL("ValueError", f"Directory '{absolute_missing_dir}' is not a valid binary log dump directory.", missing_dir)

wipe_dir(missing_dir)

#@<> WL15977-FR3.1.1.2 - input directory does not contain a dump - random file
os.mkdir(missing_dir)
open(os.path.join(missing_dir, "tmp"), "a").close()

EXPECT_FAIL("ValueError", f"Directory '{absolute_missing_dir}' is not a valid binary log dump directory.", missing_dir)

wipe_dir(missing_dir)

#@<> WL15977-FR3.1.1.2 - input directory does not contain a binlog dump
EXPECT_FAIL("ValueError", f"Directory '{default_since_dir_absolute}' is not a valid binary log dump directory.", default_since_dir)

#@<> WL15977-FR3.1.1.3 - dump is incompatible with the current version of Shell - root metadata file
md_file = os.path.join(default_dump_dir, "@.binlog.info.json")
md = read_json(md_file)
md["version"] = "100.0.0"

with backup_file(md_file) as backup:
    write_json(md_file, md)
    EXPECT_FAIL("Error: Shell Error (53006)", "Unsupported dump version", default_dump_dir)
    EXPECT_STDOUT_CONTAINS("ERROR: Dump format has version 100.0.0 which is not supported by this version of MySQL Shell. Please upgrade MySQL Shell to use it.")

#@<> WL15977-FR3.1.1.3 - dump is incompatible with the current version of Shell - one of the dumps
full_path, dir_name = get_subdir(default_dump_dir)
md_file = os.path.join(full_path, "@.binlog.json")
md = read_json(md_file)
md["version"] = "100.0.0"

with backup_file(md_file) as backup:
    write_json(md_file, md)
    EXPECT_FAIL("RuntimeError", f"Failed to read '{absolute_path_for_output(md_file)}': Unsupported dump version", default_dump_dir)
    EXPECT_STDOUT_CONTAINS("ERROR: Dump format has version 100.0.0 which is not supported by this version of MySQL Shell. Please upgrade MySQL Shell to use it.")

#@<> WL15977-FR3.1.1.4 - dump is incomplete
full_path, dir_name = get_subdir(default_dump_dir)
md_file = os.path.join(full_path, "@.binlog.json")

with backup_file(md_file) as backup:
    EXPECT_FAIL("ValueError", f"Directory '{dir_name}' is not a valid binary log dump directory.", default_dump_dir)

#@<> WL15977-FR3.1.1.5 - dump is not contiguous
# WL15977-TSFR_3_1_1_3
full_path = sorted(get_subdirs(default_dump_dir))[1]

# move second dump, creating a gap
shutil.move(full_path, __tmp_dir)

EXPECT_FAIL("ValueError", f"The binary log dump in '{default_dump_dir_absolute}' directory is not contiguous.", default_dump_dir)

# restore the dump
shutil.move(os.path.join(__tmp_dir, os.path.basename(full_path)), full_path)

#@<> WL15977-FR3.1.1.5 - dump is contiguous but directories are in reverse order
# WL15977-TSFR_3_1_1_2
dirs = sorted(get_subdirs(incremental_dump_dir))

i = 0
for d in dirs:
    shutil.move(d, os.path.join(incremental_dump_dir, chr(ord('z') - i)))
    i += 1

EXPECT_SUCCESS(incremental_dump_dir)

i = 0
for d in dirs:
    shutil.move(os.path.join(incremental_dump_dir, chr(ord('z') - i)), d)
    i += 1

wipeout_server(session)

#@<> WL15977-FR3.1.1.6 - target instance is missing some transactions
EXPECT_FAIL("ValueError", f"The target instance is missing some transactions which are not available in the dump: {server_uuid}:1-5. Enable the 'ignoreGtidGap' option to load anyway.", default_dump_dir)

#@<> WL15977-FR3.2.2.1 - target instance is missing some transactions - `ignoreGtidGap` is true
EXPECT_SUCCESS(default_dump_dir, options={"ignoreGtidGap": True})
EXPECT_STDOUT_CONTAINS(f"WARNING: The target instance is missing some transactions which are not available in the dump: {server_uuid}:1-5.")
EXPECT_STDOUT_CONTAINS("NOTE: The 'ignoreGtidGap' option is set, continuing.")

wipeout_server(session)

#@<> WL15977-FR3.1.1.6 - target instance has some transaction, but is missing transactions from the dump
create_default_test_table(session)

EXPECT_FAIL("ValueError", f"The target instance is missing some transactions which are not available in the dump: {server_uuid}:1-5. Enable the 'ignoreGtidGap' option to load anyway.", default_dump_dir)

wipeout_server(session)

#@<> WL15977-FR3.1 - load the instance dump, then binlog dump
# WL15977-TSFR_2_1
LOAD_DUMP()
EXPECT_SUCCESS(default_dump_dir)

EXPECT_TRUE(gtid_subset(session, expected_gtid_set, binary_log_status(session).executed_gtid_set))

wipeout_server(session)

#@<> WL15977-FR3.1 - load the binlog dump
EXPECT_SUCCESS(incremental_dump_dir)

EXPECT_TRUE(gtid_subset(session, expected_gtid_set, binary_log_status(session).executed_gtid_set))

wipeout_server(session)

# WL15977-FR3.4 - summary
EXPECT_STDOUT_CONTAINS("Total duration: ")
EXPECT_STDOUT_CONTAINS("Binlogs loaded: ")
EXPECT_STDOUT_CONTAINS("Uncompressed data size: ")
EXPECT_STDOUT_CONTAINS("Compressed data size: ")
EXPECT_STDOUT_CONTAINS("Statements executed: ")
EXPECT_STDOUT_CONTAINS("Average uncompressed throughput: ")
EXPECT_STDOUT_CONTAINS("Average compressed throughput: ")
EXPECT_STDOUT_CONTAINS("Average statement throughput: ")

#@<> WL15977-FR3.1 - load the binlog dump - target instance has some transactions
create_default_test_table(session)

EXPECT_SUCCESS(incremental_dump_dir)

EXPECT_TRUE(gtid_subset(session, expected_gtid_set, binary_log_status(session).executed_gtid_set))

wipeout_server(session)

#@<> WL15977-FR3.1.2 - load an instance dump, then full dump from beginning, some transactions are skipped
LOAD_DUMP()

EXPECT_SUCCESS(incremental_dump_dir)
EXPECT_STDOUT_CONTAINS(f"      Found starting GTID: {server_uuid}:6")
EXPECT_TRUE(gtid_subset(session, expected_gtid_set, binary_log_status(session).executed_gtid_set))

wipeout_server(session)

#@<> WL15977-FR3.1.2 - load twice, all transactions are skipped
EXPECT_SUCCESS(incremental_dump_dir)

EXPECT_SUCCESS(incremental_dump_dir)
EXPECT_STDOUT_CONTAINS("Nothing to be loaded.")

EXPECT_TRUE(gtid_subset(session, expected_gtid_set, binary_log_status(session).executed_gtid_set))

wipeout_server(session)

#@<> WL15977-FR3.2 - second parameter is a map
EXPECT_THROWS(lambda: util.load_binlogs("", 1),        "TypeError: Argument #2 is expected to be a map")
EXPECT_THROWS(lambda: util.load_binlogs("", "string"), "TypeError: Argument #2 is expected to be a map")
EXPECT_THROWS(lambda: util.load_binlogs("", []),       "TypeError: Argument #2 is expected to be a map")

#@<> WL15977-FR3.2.1 - `ignoreVersion` is a boolean option
TEST_BOOL_OPTION("ignoreVersion")

#@<> WL15977-FR3.2.2 - `ignoreGtidGap` is a boolean option
TEST_BOOL_OPTION("ignoreGtidGap")

#@<> WL15977-FR3.2.3 - `stopBefore` is a string option
TEST_STRING_OPTION("stopBefore")

#@<> WL15977-FR3.2.3 - `stopBefore` cannot be empty
EXPECT_FAIL("ValueError", "Argument #2: The 'stopBefore' option cannot be set to an empty string.", options={"stopBefore": ""})

#@<> WL15977-FR3.2.3.2 - `stopBefore` - no colon
EXPECT_FAIL("ValueError", "Argument #2: Invalid value of the 'stopBefore' option, expected a GTID in format: source_id:[tag:]:transaction_id", options={"stopBefore": "3e11fa47-71ca-11e1-9e33-c80aa9429562"})

#@<> WL15977-FR3.2.3.2 - `stopBefore` - negative transaction ID
EXPECT_FAIL("ValueError", "Argument #2: Invalid value of the 'stopBefore' option, expected a GTID in format: source_id:[tag:]:transaction_id", options={"stopBefore": "3e11fa47-71ca-11e1-9e33-c80aa9429562:-1"})

#@<> WL15977-FR3.2.3.2 - `stopBefore` - transaction ID set to 0
EXPECT_FAIL("ValueError", "Argument #2: Invalid value of the 'stopBefore' option, expected a GTID in format: source_id:[tag:]:transaction_id", options={"stopBefore": "3e11fa47-71ca-11e1-9e33-c80aa9429562:0"})

#@<> WL15977-FR3.2.3.2 - `stopBefore` - could not find GTID
EXPECT_FAIL("ValueError", "Could not find the final binary log file to be loaded that contains GTID '3e11fa47-71ca-11e1-9e33-c80aa9429562:1'", incremental_dump_dir, options={"stopBefore": "3e11fa47-71ca-11e1-9e33-c80aa9429562:1"})

#@<> WL15977-FR3.2.3.2 - `stopBefore` GTID - range
EXPECT_FAIL("ValueError", "Argument #2: Invalid value of the 'stopBefore' option, expected a GTID in format: source_id:[tag:]:transaction_id", options={"stopBefore": "3e11fa47-71ca-11e1-9e33-c80aa9429562:22-57"})

#@<> WL15977-FR3.2.3.2 - `stopBefore` GTID - multiple GTIDs
EXPECT_FAIL("ValueError", "Argument #2: Invalid value of the 'stopBefore' option, expected a GTID in format: source_id:[tag:]:transaction_id", options={"stopBefore": "3e11fa47-71ca-11e1-9e33-c80aa9429562:21:25"})

#@<> WL15977-FR3.2.3.3 - `stopBefore` GTID
stop_before_gtid = f"{server_uuid}:10"

EXPECT_SUCCESS(incremental_dump_dir, options={"stopBefore": stop_before_gtid})
EXPECT_STDOUT_CONTAINS(f"Stopped before GTID: {stop_before_gtid}")

EXPECT_FALSE(gtid_subset(session, expected_gtid_set, binary_log_status(session).executed_gtid_set))
EXPECT_FALSE(gtid_subset(session, stop_before_gtid, binary_log_status(session).executed_gtid_set))

wipeout_server(session)

#@<> WL15977-FR3.2.3.5 - `stopAfter` is a string option
TEST_STRING_OPTION("stopAfter")

#@<> WL15977-FR3.2.3.5 - `stopAfter` cannot be empty
EXPECT_FAIL("ValueError", "Argument #2: The 'stopAfter' option cannot be set to an empty string.", options={"stopAfter": ""})

#@<> WL15977-FR3.2.3.5 - `stopAfter` - no colon
EXPECT_FAIL("ValueError", "Argument #2: Invalid value of the 'stopAfter' option, expected a GTID in format: source_id:[tag:]:transaction_id", options={"stopAfter": "3e11fa47-71ca-11e1-9e33-c80aa9429562"})

#@<> WL15977-FR3.2.3.5 - `stopAfter` - negative transaction ID
EXPECT_FAIL("ValueError", "Argument #2: Invalid value of the 'stopAfter' option, expected a GTID in format: source_id:[tag:]:transaction_id", options={"stopAfter": "3e11fa47-71ca-11e1-9e33-c80aa9429562:-1"})

#@<> WL15977-FR3.2.3.5 - `stopAfter` - transaction ID set to 0
EXPECT_FAIL("ValueError", "Argument #2: Invalid value of the 'stopAfter' option, expected a GTID in format: source_id:[tag:]:transaction_id", options={"stopAfter": "3e11fa47-71ca-11e1-9e33-c80aa9429562:0"})

#@<> WL15977-FR3.2.3.5 - `stopAfter` - could not find GTID
EXPECT_FAIL("ValueError", "Could not find the final binary log file to be loaded that contains GTID '3e11fa47-71ca-11e1-9e33-c80aa9429562:1'", incremental_dump_dir, options={"stopAfter": "3e11fa47-71ca-11e1-9e33-c80aa9429562:1"})

#@<> WL15977-FR3.2.3.5 - `stopAfter` GTID - range
EXPECT_FAIL("ValueError", "Argument #2: Invalid value of the 'stopAfter' option, expected a GTID in format: source_id:[tag:]:transaction_id", options={"stopAfter": "3e11fa47-71ca-11e1-9e33-c80aa9429562:22-57"})

#@<> WL15977-FR3.2.3.5 - `stopAfter` GTID - multiple GTIDs
EXPECT_FAIL("ValueError", "Argument #2: Invalid value of the 'stopAfter' option, expected a GTID in format: source_id:[tag:]:transaction_id", options={"stopAfter": "3e11fa47-71ca-11e1-9e33-c80aa9429562:21:25"})

#@<> WL15977-FR3.2.3.5 - `stopAfter` GTID
stop_after_gtid = f"{server_uuid}:11"

EXPECT_SUCCESS(incremental_dump_dir, options={"stopAfter": stop_after_gtid})
EXPECT_STDOUT_CONTAINS(f"Stopped after GTID: {stop_after_gtid}")

EXPECT_FALSE(gtid_subset(session, expected_gtid_set, binary_log_status(session).executed_gtid_set))
EXPECT_TRUE(gtid_subset(session, stop_after_gtid, binary_log_status(session).executed_gtid_set))

wipeout_server(session)

#@<> WL15977-FR3.2.3.6 - `stopBefore` + `stopAfter`
EXPECT_FAIL("ValueError", "Argument #2: The 'stopBefore' and 'stopAfter' options cannot be both set.", options={"stopBefore": stop_before_gtid, "stopAfter": stop_after_gtid})

#@<> WL15977-FR3.2.4 - `dryRun` is a boolean option
TEST_BOOL_OPTION("dryRun")

#@<> WL15977-FR3.2.4.1 - load with `dryRun`
EXPECT_SUCCESS(incremental_dump_dir, options={"dryRun": True})
EXPECT_EQ("", binary_log_status(session).executed_gtid_set)

# WL15977-FR3.2.4.1.1 - information printed
EXPECT_STDOUT_CONTAINS(f"Opening dump '{incremental_dump_dir_absolute}'")

for full_path in get_subdirs(incremental_dump_dir):
    EXPECT_STDOUT_CONTAINS(f"  Loading dump '{os.path.basename(full_path)}' created at ")

EXPECT_STDOUT_CONTAINS(f"    Loading binary log file '{first_binlog_file}', GTID set: {server_uuid}:1-5")
EXPECT_STDOUT_CONTAINS(f"      Found starting GTID: {server_uuid}:1")

#@<> WL15977-FR3.2.4.1 - load with `dryRun` + `stopBefore` GTID
EXPECT_SUCCESS(incremental_dump_dir, options={"dryRun": True, "stopBefore": stop_before_gtid})
EXPECT_STDOUT_CONTAINS(f"Stopped before GTID: {stop_before_gtid}")
EXPECT_EQ("", binary_log_status(session).executed_gtid_set)

#@<> WL15977-FR3.2.4.1 - load with `dryRun` + `stopAfter` GTID
EXPECT_SUCCESS(incremental_dump_dir, options={"dryRun": True, "stopAfter": stop_after_gtid})
EXPECT_STDOUT_CONTAINS(f"Stopped after GTID: {stop_after_gtid}")
EXPECT_EQ("", binary_log_status(session).executed_gtid_set)


#@<> WL15977-FR3.2.5 - `showProgress` is a boolean option
TEST_BOOL_OPTION("showProgress")

#@<> WL15977-FR3.2.5.1 - `showProgress` - progress information - current throughput + compressed throughput
EXPECT_EQ(0, testutil.call_mysqlsh([load_uri, "--", "util", "load-binlogs", incremental_dump_dir, "--show-progress", "true"]))

EXPECT_STDOUT_MATCHES(re.compile(r'\d+% \(\d+\.?\d* [TGMK]?B / \d+\.?\d* [TGMK]?B\), \d+\.?\d* [TGMK]?B/s, \d+\.?\d* [TGMK]?B/s compressed, \d+\.?\d* stmts/s, [01234567] / 7 binlogs done', re.MULTILINE))

wipeout_server(session)

#@<> WL15977-FR3.3.1 - target instance has binlog disabled {not __dbug_off}
testutil.dbug_set("+d,load_binlogs_binlog_disabled")

EXPECT_FAIL("ValueError", "The binary logging on the target instance is disabled.")

testutil.dbug_set("")

#@<> WL15977-FR3.3.2 - target instance has GTID mode set to OFF {not __dbug_off}
testutil.dbug_set("+d,load_binlogs_gtid_disabled")

EXPECT_FAIL("ValueError", "The 'gtid_mode' system variable on the target instance is set to 'OFF'. This utility requires GTID support to be enabled.")

testutil.dbug_set("")

#@<> WL15977-FR3.3.3 - target instance has an incompatible version {not __dbug_off}
testutil.dbug_set("+d,load_binlogs_unsupported_version")

EXPECT_FAIL("ValueError", f"Version of the source instance ({__version}) is incompatible with version of the target instance (100.0.0). Enable the 'ignoreVersion' option to load anyway.", incremental_dump_dir)

testutil.dbug_set("")

#@<> WL15977-FR3.3.3 - target instance has an incompatible version + `ignoreVersion` {not __dbug_off}
testutil.dbug_set("+d,load_binlogs_unsupported_version")

EXPECT_SUCCESS(incremental_dump_dir, options={"ignoreVersion": True})
EXPECT_STDOUT_CONTAINS(f"WARNING: Version of the source instance ({__version}) is incompatible with version of the target instance (100.0.0).")
EXPECT_STDOUT_CONTAINS("NOTE: The 'ignoreVersion' option is set, continuing.")

testutil.dbug_set("")
wipeout_server(session)

#@<> WL15977-TSFR_3_1_1_1 - empty dump
EXPECT_SUCCESS(empty_dump_dir)
wipeout_server(session)

#@<> BUG#37331023 - load freezes if there's an error loading a dump
session.run_sql("SET @@GLOBAL.super_read_only=ON")

EXPECT_FAIL("RuntimeError", "Loading the binary log files has failed", incremental_dump_dir)
EXPECT_STDOUT_CONTAINS("ERROR: While replaying the binary log events: The MySQL server is running with the --super-read-only option so it cannot execute this statement")

#@<> BUG#37331023 - clean up
session.run_sql("SET @@GLOBAL.super_read_only=OFF")

#@<> Cleanup
session.close()
testutil.destroy_sandbox(__mysql_sandbox_port1)
testutil.destroy_sandbox(__mysql_sandbox_port2)
