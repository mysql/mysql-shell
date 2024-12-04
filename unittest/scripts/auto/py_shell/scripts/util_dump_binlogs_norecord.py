#@<> INCLUDE dump_utils.inc
#@<> INCLUDE dump_binlogs_utils.inc

#@<> entry point
# helpers
def EXPECT_SUCCESS(*args, **kwargs):
    output_url = None
    if len(args) > 0:
        output_url = args[0]
    if output_url is None:
        output_url = kwargs.setdefault("output_url", default_dump_dir)
    dry_run = kwargs.get("options", {}).get("dryRun", False)
    nothing_dumped = kwargs.get("nothing_dumped", False)
    data_dumped = not dry_run and not nothing_dumped
    root_exists = os.path.isdir(output_url)
    expected_root_exists = True if data_dumped else root_exists
    dumps = count_subdirs(output_url)
    expected_dumps = dumps + 1 if data_dumped else dumps
    EXPECT_DUMP_SUCCESS(*args, **kwargs)
    EXPECT_EQ(expected_root_exists, os.path.isdir(output_url), f"Output directory '{output_url}' should{'' if expected_root_exists else ' NOT'} be created.")
    EXPECT_EQ(expected_dumps, count_subdirs(output_url), f"Output directory '{output_url}' should have {expected_dumps} subdirectories.")


def EXPECT_FAIL(*args, **kwargs):
    output_url = None
    if len(args) > 2:
        output_url = args[2]
    if output_url is None:
        output_url = kwargs.setdefault("output_url", default_dump_dir)
    root_exists = os.path.isdir(output_url)
    dumps = count_subdirs(output_url)
    EXPECT_DUMP_FAIL(*args, **kwargs)
    EXPECT_EQ(root_exists, os.path.isdir(output_url), f"Output directory '{output_url}' should not be {'deleted' if root_exists else 'created'}.")
    EXPECT_EQ(dumps, count_subdirs(output_url), f"Output directory '{output_url}' should have {dumps} subdirectories.")

#@<> WL15977-FR2.3 - an exception shall be thrown if there is no global session
EXPECT_FAIL("RuntimeError", "An open session is required to perform this operation.")

#@<> deploy sandbox
testutil.deploy_raw_sandbox(__mysql_sandbox_port1, "root", {
    "server-id": str(random.randint(1, 4294967295)),
    **mysqld_options,
})
testutil.wait_sandbox_alive(__sandbox_uri1)

#@<> WL15977-FR2.3 - an exception shall be thrown if there is no open global session
shell.connect(dump_uri)
session.close()
EXPECT_FAIL("RuntimeError", "An open session is required to perform this operation.")

#@<> Setup
setup_session(dump_uri)
wipe_dir(default_dump_dir)
wipe_dir(default_since_dir)

#@<> WL15977-FR2.1 - first parameter is a string
EXPECT_THROWS(lambda: util.dump_binlogs(None),  "TypeError: Argument #1 is expected to be a string")
EXPECT_THROWS(lambda: util.dump_binlogs(1),     "TypeError: Argument #1 is expected to be a string")
EXPECT_THROWS(lambda: util.dump_binlogs([]),    "TypeError: Argument #1 is expected to be a string")
EXPECT_THROWS(lambda: util.dump_binlogs({}),    "TypeError: Argument #1 is expected to be a string")
EXPECT_THROWS(lambda: util.dump_binlogs(False), "TypeError: Argument #1 is expected to be a string")

#@<> WL15977-FR2.1.1 - output directory does not exist
EXPECT_SUCCESS(options={"startFrom": start_from_beginning})

create_default_test_table(session)

EXPECT_SUCCESS()

wipe_dir(default_dump_dir)
wipeout_server(session)

#@<> WL15977-FR2.1.3 - output directory exists and is empty
os.mkdir(default_dump_dir)

EXPECT_SUCCESS(options={"startFrom": start_from_beginning})

create_default_test_table(session)

EXPECT_SUCCESS()

wipe_dir(default_dump_dir)
wipeout_server(session)

#@<> WL15977-FR2.1.3 - output directory exists and is not empty
os.mkdir(default_dump_dir)
open(os.path.join(default_dump_dir, "tmp"), "a").close()

EXPECT_FAIL("ValueError", f"The '{default_dump_dir_absolute}' points to an existing directory but it does not contain a binary log dump.", options={"startFrom": start_from_beginning})

wipe_dir(default_dump_dir)

#@<> WL15977-FR2.1.4 - consecutive dumps in the same directory
prev_binlog = binary_log_status(session)

# server is empty, this will have an empty GTID set
EXPECT_SUCCESS(options={"startFrom": start_from_beginning})
EXPECT_STDOUT_CONTAINS(f"Starting from binary log file: {start_from_beginning}")
EXPECT_STDOUT_CONTAINS(f"Will finish at binary log file: {prev_binlog.binlog.file}:{prev_binlog.binlog.position}")
EXPECT_STDOUT_CONTAINS("GTID set dumped: \n")

create_default_test_table(session)
binlog = binary_log_status(session)

# whole server is dumped, the dumped GTID set is the same as gtid_executed
EXPECT_SUCCESS()
EXPECT_STDOUT_CONTAINS(f"Starting from binary log file: {prev_binlog.binlog.file}:{prev_binlog.binlog.position}")
EXPECT_STDOUT_CONTAINS(f"Will finish at binary log file: {binlog.binlog.file}:{binlog.binlog.position}")
EXPECT_STDOUT_CONTAINS(f"GTID set dumped: {binlog.executed_gtid_set}")

create_default_test_table(session)
new_binlog = binary_log_status(session)

# incremental dump, the difference between gtid_executed and previous dump was dumped
EXPECT_SUCCESS()
EXPECT_STDOUT_CONTAINS(f"GTID set dumped: {gtid_subtract(session, new_binlog.executed_gtid_set, binlog.executed_gtid_set)}")

# nothing has changed, dump succeeds
EXPECT_SUCCESS(nothing_dumped=True)

wipe_dir(default_dump_dir)
wipeout_server(session)

#@<> WL15977-TSFR_2_1_1 - manipulate metadata so that loading a JSON file fails
EXPECT_SUCCESS(options={"startFrom": start_from_beginning})

full_path, dir_name = get_subdir(default_dump_dir)
md_file = os.path.join(full_path, "@.binlog.json")

md = read_json(md_file)
del md["endAt"]
write_json(md_file, md)

EXPECT_FAIL("RuntimeError", f"Failed to read '{absolute_path_for_output(md_file)}': JSON object should contain a 'endAt' key with an object value")

wipe_dir(default_dump_dir)

#@<> WL15977-FR2.2 - second parameter is a map
EXPECT_THROWS(lambda: util.dump_binlogs("", 1),        "TypeError: Argument #2 is expected to be a map")
EXPECT_THROWS(lambda: util.dump_binlogs("", "string"), "TypeError: Argument #2 is expected to be a map")
EXPECT_THROWS(lambda: util.dump_binlogs("", []),       "TypeError: Argument #2 is expected to be a map")

#@<> WL15977-TSFR_2_2_1 - dump without options
EXPECT_THROWS(lambda: util.dump_binlogs(default_dump_dir), f"One of the 'since' or 'startFrom' options must be set because the destination directory '{default_dump_dir_absolute}' does not contain any dumps yet.")

EXPECT_SUCCESS(options={"startFrom": start_from_beginning})

EXPECT_NO_THROWS(lambda: util.dump_binlogs(default_dump_dir))
EXPECT_STDOUT_CONTAINS("NOTE: Nothing to be dumped.")

wipe_dir(default_dump_dir)

#@<> WL15977-FR2.2.1 - `since` is a string option
TEST_STRING_OPTION("since")

#@<> WL15977-FR2.2.1 - `since` cannot be empty
EXPECT_FAIL("ValueError", "Argument #2: The 'since' option cannot be set to an empty string.", options={"since": ""})

#@<> WL15977-FR2.2.1.1.1 - `since` - directory does not exist
EXPECT_FAIL("ValueError", f"Directory '{default_since_dir_absolute}' specified in option 'since' does not exist.", options={"since": default_since_dir})

#@<> WL15977-FR2.2.1.1.2 - `since` - directory exists and is not empty
os.mkdir(default_since_dir)
open(os.path.join(default_since_dir, "tmp"), "a").close()

EXPECT_FAIL("ValueError", f"Directory '{default_since_dir_absolute}' specified in option 'since' is not a valid dump directory.", options={"since": default_since_dir})

wipe_dir(default_since_dir)

#@<> WL15977-FR2.2.1.1.2 - `since` - directory was created by dump_schemas()
create_default_test_table(session)
EXPECT_NO_THROWS(lambda: util.dump_schemas([tested_schema], default_since_dir, {"showProgress": False}))

EXPECT_FAIL("ValueError", f"Directory '{default_since_dir_absolute}' specified in option 'since' is not a valid dump directory.", options={"since": default_since_dir})

wipe_dir(default_since_dir)
wipeout_server(session)

#@<> WL15977-FR2.2.1.1.2 - `since` - directory was created by dump_tables()
create_default_test_table(session)
EXPECT_NO_THROWS(lambda: util.dump_tables(tested_schema, [tested_table], default_since_dir, {"showProgress": False}))

EXPECT_FAIL("ValueError", f"Directory '{default_since_dir_absolute}' specified in option 'since' is not a valid dump directory.", options={"since": default_since_dir})

wipe_dir(default_since_dir)
wipeout_server(session)

#@<> WL15977-FR2.2.1.1.2 - `since` - directory created by dump_binlogs() is incomplete
EXPECT_SUCCESS(default_since_dir, options={"startFrom": start_from_beginning})

full_path, dir_name = get_subdir(default_since_dir)
os.remove(os.path.join(full_path, "@.binlog.done.json"))

EXPECT_FAIL("ValueError", f"Directory '{dir_name}' is incomplete and is not a valid binary log dump directory.", options={"since": default_since_dir})

wipe_dir(default_since_dir)

#@<> WL15977-FR2.2.1.1.2 - `since` - directory created by dump_instance() is incomplete
DUMP_INSTANCE()

os.remove(os.path.join(default_since_dir, "@.done.json"))

EXPECT_FAIL("ValueError", f"Directory '{default_since_dir_absolute}' specified in option 'since' is incomplete and is not a valid dump directory.", options={"since": default_since_dir})

wipe_dir(default_since_dir)

#@<> WL15977-FR2.2.1.1.3 - `since` - directory created by Shell older than 9.2.0
# WL15977-TSFR_1_1
DUMP_INSTANCE()

md_file = os.path.join(default_since_dir, "@.json")
md = read_json(md_file)
# this entry was added by WL#15977
del md["source"]["topology"]
write_json(md_file, md)

EXPECT_FAIL("ValueError", "The 'since' option should be set to a directory which contains a dump created by MySQL Shell 9.2.0 or newer.", options={"since": default_since_dir})

wipe_dir(default_since_dir)

#@<> WL15977-FR2.2.1.1.4 - `since` - directory incompatible with the current version of Shell
DUMP_INSTANCE()

md_file = os.path.join(default_since_dir, "@.json")
md = read_json(md_file)
md["version"] = "100.0.0"
write_json(md_file, md)

EXPECT_FAIL("Error: Shell Error (53006)", "Unsupported dump version", options={"since": default_since_dir})
EXPECT_STDOUT_CONTAINS("ERROR: Dump format has version 100.0.0 which is not supported by this version of MySQL Shell. Please upgrade MySQL Shell to use it.")

wipe_dir(default_since_dir)

#@<> WL15977-FR2.2.1.1.5 - `since` - dump contains partial information - exceptions
# WL15977-TSFR_2_2_1_1_5_1
session.run_sql("CREATE SCHEMA !", [tested_schema])

for option in [
    {"events": False},
    {"routines": False},
    {"users": False},
    {"triggers": False},
    {"where": {"s.t": "a > 0"}},
    {"partitions": {"s.t": ["p1"]}},
    {"ddlOnly": True},
    {"dataOnly": True},
    {"includeSchemas": ["s"]},
    {"includeTables": ["s.t"]},
    {"includeEvents": ["s.t"]},
    {"includeRoutines": ["s.t"]},
    {"includeUsers": ["u@h"]},
    {"excludeUsers": ["u@h"]},
    {"includeTriggers": ["s.t"]},
    {"excludeSchemas": ["s.t"]},
    {"excludeTables": ["s.t"]},
    {"excludeEvents": ["s.t"]},
    {"excludeRoutines": ["s.t"]},
    {"excludeTriggers": ["s.t.t"]},
]:
    print("--> option:", option)
    DUMP_INSTANCE(options=option)
    EXPECT_FAIL("ValueError", "The 'since' option is set to a directory which contains a partial dump of an instance.", options={"since": default_since_dir})
    wipe_dir(default_since_dir)

#@<> WL15977-FR2.2.1.1.5 - `since` - dump contains partial information - OK
for option in [
    {"events": True},
    {"routines": True},
    {"users": True},
    {"triggers": True},
    {"where": {}},
    {"partitions": {}},
    {"ddlOnly": False},
    {"dataOnly": False},
    {"includeSchemas": []},
    {"includeTables": []},
    {"includeEvents": []},
    {"includeRoutines": []},
    {"includeUsers": []},
    {"excludeUsers": []},
    {"includeTriggers": []},
    *[{p[0]: [p[1] + ".t"]} for p in itertools.product([
            "excludeSchemas",
            "excludeTables",
            "excludeEvents",
            "excludeRoutines",
            "excludeTriggers",
        ], [
            "INFORMATION_SCHEMA",
            "PERFORMANCE_SCHEMA",
            "mysql",
            "sys",
        ])]
]:
    print("--> option:", option)
    DUMP_INSTANCE(options=option)
    create_default_test_table(session)
    EXPECT_SUCCESS(options={"since": default_since_dir})
    wipe_dir(default_dump_dir)
    wipe_dir(default_since_dir)
    wipeout_server(session)

#@<> WL15977-FR2.2.1.1.6 - `since` - dump contains modified DDL - exceptions
# WL15977-TSFR_2_2_1_1_6_1
for option in [
    {"ocimds": True, "compatibility": ["strip_restricted_grants", "skip_invalid_accounts"]},
    {"compatibility": ["force_innodb"]},
]:
    print("--> option:", option)
    DUMP_INSTANCE(options=option)
    EXPECT_FAIL("ValueError", "The 'since' option is set to a directory which contains a dump with modified DDL. Enable the 'ignoreDdlChanges' option to dump anyway.", options={"since": default_since_dir})
    wipe_dir(default_since_dir)

#@<> WL15977-FR2.2.1.1.6 - `since` - dump contains modified DDL - OK
for option in [
    {"ocimds": False},
    {"compatibility": []},
]:
    print("--> option:", option)
    DUMP_INSTANCE(options=option)
    create_default_test_table(session)
    EXPECT_SUCCESS(options={"since": default_since_dir})
    wipe_dir(default_dump_dir)
    wipe_dir(default_since_dir)
    wipeout_server(session)

#@<> WL15977-FR2.2.1.1.7 - `since` - dump is inconsistent
DUMP_INSTANCE(options={"consistent": False})
EXPECT_FAIL("ValueError", "The 'since' option is set to a directory which contains an inconsistent dump.", options={"since": default_since_dir})
wipe_dir(default_since_dir)

#@<> WL15977-FR2.2.1.1.7 - `since` - dump is consistent
DUMP_INSTANCE(options={"consistent": True})
create_default_test_table(session)
EXPECT_SUCCESS(options={"since": default_since_dir})
wipe_dir(default_dump_dir)
wipe_dir(default_since_dir)
wipeout_server(session)

#@<> WL15977-FR2.2.1.2 - `since` with a valid directory - dump_instance()
create_default_test_table(session)
binlog = binary_log_status(session)

DUMP_INSTANCE()

create_default_test_table(session)
new_binlog = binary_log_status(session)

EXPECT_SUCCESS(options={"since": default_since_dir})
EXPECT_STDOUT_CONTAINS(f"GTID set dumped: {gtid_subtract(session, new_binlog.executed_gtid_set, binlog.executed_gtid_set)}")

wipe_dir(default_dump_dir)
wipe_dir(default_since_dir)
wipeout_server(session)

#@<> WL15977-FR2.2.1.2 - `since` with a valid directory - dump_binlogs()
create_default_test_table(session)
binlog = binary_log_status(session)

EXPECT_SUCCESS(default_since_dir, options={"startFrom": start_from_beginning})

create_default_test_table(session)
new_binlog = binary_log_status(session)

EXPECT_SUCCESS(options={"since": default_since_dir})
EXPECT_STDOUT_CONTAINS(f"GTID set dumped: {gtid_subtract(session, new_binlog.executed_gtid_set, binlog.executed_gtid_set)}")

wipe_dir(default_dump_dir)
wipe_dir(default_since_dir)
wipeout_server(session)

#@<> WL15977-FR2.2.2 - `startFrom` is a string option
TEST_STRING_OPTION("startFrom")

#@<> WL15977-FR2.2.2 - `startFrom` cannot be empty
EXPECT_FAIL("ValueError", "Argument #2: The 'startFrom' option cannot be set to an empty string.", options={"startFrom": ""})

#@<> WL15977-FR2.2.2.1, WL15977-FR2.2.2.2 - `startFrom` - invalid position
for log_name in [
    "log:s",
    "log:-1",
]:
    print("--> log:", log_name)
    EXPECT_FAIL("ValueError", "Argument #2: The 'startFrom' option must be in the form <binlog-file>[:<binlog-position>], but binlog-position could not be converted to a number.", options={"startFrom": "log:s"})

#@<> WL15977-FR2.2.3 - `startFrom` - log file does not exist
EXPECT_FAIL("ValueError", f"Found final binary log file '{first_binlog_file}' before finding the starting binary log file 'log'", options={"startFrom": "log"})

#@<> WL15977-FR2.2.4 - `startFrom` - whole log file
# WL15977-TSFR_2_2_2_1
for i in range(10):
    create_default_test_table(session)

EXPECT_SUCCESS(options={"startFrom": first_binlog_file})

wipe_dir(default_dump_dir)
wipeout_server(session)

#@<> WL15977-FR2.2.5 - `startFrom` - log file + position
# WL15977-TSFR_2_2_2_1
for i in range(10):
    create_default_test_table(session)

EXPECT_SUCCESS(options={"startFrom": f"{first_binlog_file}:{position_after_description_event}"})

wipe_dir(default_dump_dir)
wipeout_server(session)

#@<> WL15977-FR2.2.2.5.1 - `startFrom` - position outside of range
# WL15977-TSFR_2_2_2_2
EXPECT_FAIL("ValueError", f"The starting binary log file '{first_binlog_file}' does not contain the 100000 position.", options={"startFrom": f"{first_binlog_file}:100000"})

#@<> WL15977-FR2.2.2.5 - `startFrom` - position below 4
# WL15977-TSFR_2_2_2_3
EXPECT_SUCCESS(options={"startFrom": f"{first_binlog_file}:2"})
wipe_dir(default_dump_dir)

#@<> WL15977-FR2.2.2.5.1 - `startFrom` - position inside of an event
# WL15977-TSFR_2_2_2_4
EXPECT_FAIL("RuntimeError", f"While reading the '{first_binlog_file}' binary log file: failed to fetch a binary log file event", options={"startFrom": f"{first_binlog_file}:5"})

#@<> WL15977-FR2.2.2.6 - `since` and `startFrom` cannot be used at the same time
EXPECT_FAIL("ValueError", "Argument #2: The 'since' and 'startFrom' options cannot be both set.", options={"since": "since", "startFrom": "log"})

#@<> WL15977-FR2.2.3 - output directory does not exist, `since` or `startFrom` is required
EXPECT_FAIL("ValueError", f"One of the 'since' or 'startFrom' options must be set because the destination directory '{default_dump_dir_absolute}' does not contain any dumps yet.")

#@<> WL15977-FR2.2.3 - output directory exists and is empty, `since` or `startFrom` is required
os.mkdir(default_dump_dir)

EXPECT_FAIL("ValueError", f"One of the 'since' or 'startFrom' options must be set because the destination directory '{default_dump_dir_absolute}' does not contain any dumps yet.")

wipe_dir(default_dump_dir)

#@<> WL15977-FR2.2.4 - output directory contains a valid dump, neither `since` nor `startFrom` is allowed
EXPECT_SUCCESS(options={"startFrom": start_from_beginning})

for option in [
    "since",
    "startFrom",
]:
    print("--> option:", option)
    EXPECT_FAIL("ValueError", f"The '{option}' option cannot be used because '{default_dump_dir_absolute}' points to a pre-existing binary log dump directory. You may omit that option to dump starting from the most recent entry there.", options={option: "value"})

wipe_dir(default_dump_dir)

#@<> WL15977-FR2.2.5 - `threads` is an unsigned int option
TEST_UINT_OPTION("threads")

#@<> WL15977-FR2.2.5, WL15977-FR2.2.5.1 - `threads` cannot be set to zero
EXPECT_FAIL("ValueError", "Argument #2: The value of 'threads' option must be greater than 0.", options={"threads": 0})

#@<> WL15977-FR2.2.5 - `threads` - valid value
EXPECT_SUCCESS(options={"threads": 2, "startFrom": start_from_beginning})

wipe_dir(default_dump_dir)

#@<> WL15977-FR2.2.6 - `dryRun` is a boolean option
TEST_BOOL_OPTION("dryRun")

#@<> WL15977-FR2.2.6 - `dryRun` + `startFrom`
EXPECT_SUCCESS(options={"dryRun": True, "startFrom": start_from_beginning})

#@<> WL15977-FR2.2.6 - `dryRun` + `since` util.dump_instance()
DUMP_INSTANCE()

create_default_test_table(session)

EXPECT_SUCCESS(options={"dryRun": True, "since": default_since_dir})

wipe_dir(default_dump_dir)
wipe_dir(default_since_dir)
wipeout_server(session)

#@<> WL15977-FR2.2.6 - `dryRun` + `since` util.dump_binlogs()
EXPECT_SUCCESS(default_since_dir, options={"startFrom": start_from_beginning})

create_default_test_table(session)

EXPECT_SUCCESS(options={"dryRun": True, "since": default_since_dir})

wipe_dir(default_dump_dir)
wipe_dir(default_since_dir)
wipeout_server(session)

#@<> WL15977-FR2.2.6 - `dryRun` + reusing output directory
EXPECT_SUCCESS(options={"startFrom": start_from_beginning})

create_default_test_table(session)

EXPECT_SUCCESS(options={"dryRun": True})

wipe_dir(default_dump_dir)
wipe_dir(default_since_dir)
wipeout_server(session)

#@<> WL15977-FR2.2.7 - `compression` is a string option
TEST_STRING_OPTION("compression")

#@<> WL15977-FR2.2.7 - `compression` cannot be empty
EXPECT_FAIL("ValueError", "Argument #2: The 'compression' option cannot be set to an empty string.", options={"compression": ""})

#@<> WL15977-FR2.2.7 - `compression` - unknown value
EXPECT_FAIL("ValueError", "Argument #2: Unknown compression type: unknown", options={"compression": "unknown"})

#@<> WL15977-FR2.2.7.1 - `compression` - allowed values
create_default_test_table(session)

for compression in [
    "none",
    "gzip",
    "gzip;level=8",
    "zstd",
    "zstd;level=8",
]:
    print("--> compression:", compression)
    EXPECT_SUCCESS(options={"compression": compression, "startFrom": start_from_beginning})
    wipe_dir(default_dump_dir)

wipeout_server(session)

#@<> WL15977-FR2.2.8 - `showProgress` is a boolean option
TEST_BOOL_OPTION("showProgress")

#@<> WL15977-FR2.2.8.1 - `showProgress` - progress information - current throughput + compressed throughput
create_default_test_table(session)

EXPECT_EQ(0, testutil.call_mysqlsh([dump_uri, "--", "util", "dump-binlogs", default_dump_dir, "--show-progress", "true", "--start-from", start_from_beginning]))
EXPECT_TRUE(os.path.isdir(default_dump_dir))

EXPECT_STDOUT_MATCHES(re.compile(r'\d+% \(\d+\.?\d* [TGMK]?B / \d+\.?\d* [TGMK]?B\), \d+\.?\d* [TGMK]?B/s, \d+\.?\d* [TGMK]?B/s compressed, [01] / 1 binlogs done', re.MULTILINE))

wipe_dir(default_dump_dir)
wipeout_server(session)

#@<> WL15977-FR2.2.9 - `ignoreDdlChanges` is a boolean option
TEST_BOOL_OPTION("ignoreDdlChanges")

#@<> WL15977-FR2.2.9.1 - `ignoreDdlChanges` is set to true
for option in [
    {"ocimds": True, "compatibility": ["strip_restricted_grants", "skip_invalid_accounts"]},
    {"compatibility": ["force_innodb"]},
]:
    print("--> option:", option)
    DUMP_INSTANCE(options=option)
    create_default_test_table(session)
    EXPECT_SUCCESS(options={"ignoreDdlChanges": True, "since": default_since_dir})
    EXPECT_STDOUT_CONTAINS("WARNING: The 'since' option is set to a directory which contains a dump with modified DDL.")
    EXPECT_STDOUT_CONTAINS("NOTE: The 'ignoreDdlChanges' option is set, continuing.")
    wipe_dir(default_dump_dir)
    wipe_dir(default_since_dir)
    wipeout_server(session)

#@<> WL15977-FR2.2.9.2 - `ignoreDdlChanges` cannot be set if `since` is not set
for value in [
    True,
    False,
]:
    print("--> value:", value)
    EXPECT_FAIL("ValueError", "Argument #2: The 'ignoreDdlChanges' option cannot be used when the 'since' option is not set.", options={"ignoreDdlChanges": value})

#@<> WL15977-FR2.3.1 - source instance has binlog disabled {not __dbug_off}
testutil.dbug_set("+d,dump_binlogs_binlog_disabled")

EXPECT_FAIL("ValueError", "The binary logging on the source instance is disabled.", options={"startFrom": start_from_beginning})

testutil.dbug_set("")

#@<> WL15977-FR2.3.2 - source instance has GTID mode set to OFF {not __dbug_off}
testutil.dbug_set("+d,dump_binlogs_gtid_disabled")

EXPECT_FAIL("ValueError", "The 'gtid_mode' system variable on the source instance is set to 'OFF'. This utility requires GTID support to be enabled.", options={"startFrom": start_from_beginning})

testutil.dbug_set("")

#@<> WL15977-FR2.3.4 - source instance was started with --binlog-do-db {not __dbug_off}
testutil.dbug_set("+d,dump_binlogs_has_do_db")

EXPECT_SUCCESS(options={"startFrom": start_from_beginning})
EXPECT_STDOUT_CONTAINS("NOTE: The source instance was started with the --binlog-do-db option.")
EXPECT_STDOUT_CONTAINS("NOTE: The binary log on the source instance is filtered and does not contain all transactions.")

wipe_dir(default_dump_dir)
testutil.dbug_set("")

#@<> WL15977-FR2.3.4 - source instance was started with --binlog-ignore-db {not __dbug_off}
testutil.dbug_set("+d,dump_binlogs_has_ignore_db")

EXPECT_SUCCESS(options={"startFrom": start_from_beginning})
EXPECT_STDOUT_CONTAINS("NOTE: The source instance was started with the --binlog-ignore-db option.")
EXPECT_STDOUT_CONTAINS("NOTE: The binary log on the source instance is filtered and does not contain all transactions.")

wipe_dir(default_dump_dir)
testutil.dbug_set("")

#@<> WL15977-FR2.3.4 - source instance was started with both --binlog-do-db and --binlog-ignore-db {not __dbug_off}
testutil.dbug_set("+d,dump_binlogs_has_do_db,dump_binlogs_has_ignore_db")

EXPECT_SUCCESS(options={"startFrom": start_from_beginning})
EXPECT_STDOUT_CONTAINS("NOTE: The source instance was started with the --binlog-do-db option.")
EXPECT_STDOUT_CONTAINS("NOTE: The source instance was started with the --binlog-ignore-db option.")
EXPECT_STDOUT_CONTAINS("NOTE: The binary log on the source instance is filtered and does not contain all transactions.")

wipe_dir(default_dump_dir)
testutil.dbug_set("")

#@<> WL15977-FR2.4.1 - incremental dump - previous dump is incomplete
EXPECT_SUCCESS(options={"startFrom": start_from_beginning})

full_path, dir_name = get_subdir(default_dump_dir)
os.remove(os.path.join(full_path, "@.binlog.done.json"))

EXPECT_FAIL("ValueError", f"Directory '{dir_name}' is incomplete and is not a valid binary log dump directory.")

wipe_dir(default_dump_dir)

#@<> WL15977-FR2.4.3 - previous dump has more information than current instance - dump_instance()
create_default_test_table(session)

DUMP_INSTANCE()
wipeout_server(session)

EXPECT_FAIL("ValueError", "The base dump contains transactions that are not available in the source instance.", options={"since": default_since_dir})

wipe_dir(default_since_dir)

#@<> WL15977-FR2.4.3 - previous dump has more information than current instance - dump_binlogs()
create_default_test_table(session)

EXPECT_SUCCESS(options={"startFrom": start_from_beginning})
wipeout_server(session)

EXPECT_FAIL("ValueError", "The base dump contains transactions that are not available in the source instance.")

wipe_dir(default_dump_dir)

#@<> WL15977-FR2.4.4 - binlogs were purged
create_default_test_table(session)

EXPECT_SUCCESS(options={"startFrom": start_from_beginning})

create_default_test_table(session)
create_default_test_table(session)

session.run_sql("PURGE BINARY LOGS TO ?", [binary_log_status(session).binlog.file])

EXPECT_FAIL("ValueError", "Some of the transactions to be dumped have already been purged.")

wipeout_server(session)
wipe_dir(default_dump_dir)

#@<> multiple binlogs
for i in range(10):
    create_default_test_table(session)

EXPECT_SUCCESS(options={"startFrom": start_from_beginning})

wipe_dir(default_dump_dir)

#@<> WL15977-FR2.4.6 - first dump fails {not __dbug_off}
# this reuses server state from the previous dump
testutil.set_trap("binlog", [f"file == {binlog_file_prefix}.000003", "++match_counter > 10"], { "msg": "Internal error." })

EXPECT_FAIL("RuntimeError", "Internal error.", options={"threads": 2, "startFrom": start_from_beginning})
EXPECT_STDOUT_CONTAINS("Removing files...")

testutil.clear_traps("binlog")

#@<> WL15977-FR2.4.6 - second dump fails {not __dbug_off}
# this reuses server state from the previous dump
EXPECT_SUCCESS(options={"startFrom": start_from_beginning})

for i in range(10):
    create_default_test_table(session)

testutil.set_trap("binlog", [f"file == {binlog_file_prefix}.000009", "++match_counter > 10"], { "msg": "Internal error." })

EXPECT_FAIL("RuntimeError", "Internal error.", options={"threads": 2})
EXPECT_STDOUT_CONTAINS("Removing files...")

testutil.clear_traps("binlog")

# there are no leftovers, another dump succeeds
EXPECT_SUCCESS()

wipeout_server(session)
wipe_dir(default_dump_dir)

#@<> BUG#37338679 - check version compatibility with the previous dump - dump_instance()
# MAJOR.MINOR
current_version = __version[:__version.rfind('.')]

create_default_test_table(session)
DUMP_INSTANCE()
create_default_test_table(session)

md_file = os.path.join(default_since_dir, "@.json")
md = read_json(md_file)

# wrong version
md["source"]["sysvars"]["version"] = "100.0.0"

with backup_file(md_file) as backup:
    write_json(md_file, md)
    EXPECT_FAIL("ValueError", f"Version of the source instance ({current_version}) is incompatible with version of the instance used in the previous dump (100.0).", options={"since": default_since_dir})

# patch difference is OK
md["source"]["sysvars"]["version"] = current_version + ".100"

with backup_file(md_file) as backup:
    write_json(md_file, md)
    EXPECT_SUCCESS(options={"since": default_since_dir})

wipeout_server(session)
wipe_dir(default_since_dir)
wipe_dir(default_dump_dir)

#@<> BUG#37338679 - check version compatibility with the previous dump - dump_binlogs()
# MAJOR.MINOR
current_version = __version[:__version.rfind('.')]

create_default_test_table(session)
EXPECT_SUCCESS(default_since_dir, options={"startFrom": start_from_beginning})
create_default_test_table(session)

full_path, dir_name = get_subdir(default_since_dir)
md_file = os.path.join(full_path, "@.binlog.json")
md = read_json(md_file)

# wrong version
md["source"]["sysvars"]["version"] = "100.0.0"

with backup_file(md_file) as backup:
    write_json(md_file, md)
    EXPECT_FAIL("ValueError", f"Version of the source instance ({current_version}) is incompatible with version of the instance used in the previous dump (100.0).", options={"since": default_since_dir})

# patch difference is OK
md["source"]["sysvars"]["version"] = current_version + ".100"

with backup_file(md_file) as backup:
    write_json(md_file, md)
    EXPECT_SUCCESS(options={"since": default_since_dir})

wipeout_server(session)
wipe_dir(default_since_dir)
wipe_dir(default_dump_dir)

#@<> BUG#37338679 - check version compatibility with the previous dump - incremental dump
# MAJOR.MINOR
current_version = __version[:__version.rfind('.')]

create_default_test_table(session)
EXPECT_SUCCESS(options={"startFrom": start_from_beginning})
create_default_test_table(session)

full_path, dir_name = get_subdir(default_dump_dir)
md_file = os.path.join(full_path, "@.binlog.json")
md = read_json(md_file)

# wrong version
md["source"]["sysvars"]["version"] = "100.0.0"

with backup_file(md_file) as backup:
    write_json(md_file, md)
    EXPECT_FAIL("ValueError", f"Version of the source instance ({current_version}) is incompatible with version of the instance used in the previous dump (100.0).")

# patch difference is OK
md["source"]["sysvars"]["version"] = current_version + ".100"

with backup_file(md_file) as backup:
    write_json(md_file, md)
    EXPECT_SUCCESS()

# multiple dumps in the dir
create_default_test_table(session)

full_path = sorted(get_subdirs(default_dump_dir))[1]
md_file = os.path.join(full_path, "@.binlog.json")
md = read_json(md_file)

# wrong version - multiple dumps
md["source"]["sysvars"]["version"] = "100.0.0"

with backup_file(md_file) as backup:
    write_json(md_file, md)
    EXPECT_FAIL("ValueError", f"Version of the source instance ({current_version}) is incompatible with version of the instance used in the previous dump (100.0).")

# patch difference is OK - multiple dumps
md["source"]["sysvars"]["version"] = current_version + ".200"

with backup_file(md_file) as backup:
    write_json(md_file, md)
    EXPECT_SUCCESS()

wipeout_server(session)
wipe_dir(default_dump_dir)

#@<> Cleanup
session.close()
testutil.destroy_sandbox(__mysql_sandbox_port1)
