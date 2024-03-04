# imports
import itertools
import json
import os
import os.path
import re
import shutil
from typing import NamedTuple

# constants
dump_uri = __sandbox_uri1
dump_xuri = dump_uri.replace("mysql://", "mysqlx://") + "0"

load_uri = __sandbox_uri2
load_xuri = load_uri.replace("mysql://", "mysqlx://") + "0"

default_dump_dir = "dump_binlogs"
default_dump_dir_absolute = absolute_path_for_output(default_dump_dir)

incremental_dump_dir = "dump_binlogs_inc"
incremental_dump_dir_absolute = absolute_path_for_output(incremental_dump_dir)

default_since_dir = "since_dump"
default_since_dir_absolute = absolute_path_for_output(default_since_dir)

binlog_file_prefix = "my-binlog"
first_binlog_file = f"{binlog_file_prefix}.000001"
start_from_beginning = f"{first_binlog_file}:4"

if __version_num < 80000:
    position_after_description_event = 154
else:
    position_after_description_event = 158

tested_schema = "wl15977"
tested_table = "wl15977"

mysqld_options = {
    "log-bin": binlog_file_prefix,
    "enforce_gtid_consistency": "ON",
    "gtid_mode": "ON",
    "max_binlog_size": str(128 * 1024),  # small binlog size, so that it rotates often
    "innodb_doublewrite": "OFF",
}

# helpers
def setup_session(u):
    shell.connect(u)
    session.run_sql("SET NAMES 'utf8mb4';")

def count_subdirs(dir):
    if not os.path.isdir(dir):
        return 0
    with os.scandir(dir) as it:
        return sum(e.is_dir() for e in it)

def get_subdir(dir):
    with os.scandir(dir) as it:
        for e in it:
            if e.is_dir():
                path = os.path.join(dir, e.name)
                return (path, absolute_path_for_output(path))

def get_subdirs(dir):
    sub = []
    with os.scandir(dir) as it:
        for e in it:
            if e.is_dir():
                sub.append(os.path.join(dir, e.name))
    return sub

def create_default_test_table(s):
    create_test_table(s, tested_schema, tested_table, "(a INT PRIMARY KEY, b TEXT)")

def EXPECT_DUMP_SUCCESS(output_url=default_dump_dir, options={}, nothing_dumped=False):
    WIPE_STDOUT()
    dry_run = options.get("dryRun", False)
    if "showProgress" not in options:
        options["showProgress"] = False
    EXPECT_NO_THROWS(lambda: util.dump_binlogs(output_url, options), "dump_binlogs() should not throw")
    if dry_run:
        EXPECT_STDOUT_CONTAINS("NOTE: The dryRun option is enabled, no files will be created.")
    if nothing_dumped:
        EXPECT_STDOUT_CONTAINS("NOTE: Nothing to be dumped.")
    else:
        binlogs_dumped = "Binlogs dumped: "
        if dry_run:
            EXPECT_STDOUT_NOT_CONTAINS(binlogs_dumped)
        else:
            EXPECT_STDOUT_CONTAINS(binlogs_dumped)

def EXPECT_DUMP_FAIL(error, msg, output_url=default_dump_dir, options={}):
    WIPE_STDOUT()
    is_re = is_re_instance(msg)
    full_msg = f"{re.escape(error) if is_re else error}: {msg.pattern if is_re else msg}"
    if is_re:
        full_msg = re.compile("^" + full_msg)
    if "showProgress" not in options:
        options["showProgress"] = False
    EXPECT_THROWS(lambda: util.dump_binlogs(output_url, options), full_msg)

def EXPECT_LOAD_SUCCESS(input_url=default_dump_dir, options={}):
    WIPE_STDOUT()
    dry_run = options.get("dryRun", False)
    if "showProgress" not in options:
        options["showProgress"] = False
    EXPECT_NO_THROWS(lambda: util.load_binlogs(input_url, options), "load_binlogs() should not throw")
    if dry_run:
        EXPECT_STDOUT_CONTAINS("NOTE: The dryRun option is enabled, no data will be loaded.")

def EXPECT_LOAD_FAIL(error, msg, input_url=default_dump_dir, options={}):
    WIPE_STDOUT()
    is_re = is_re_instance(msg)
    full_msg = f"{re.escape(error) if is_re else error}: {msg.pattern if is_re else msg}"
    if is_re:
        full_msg = re.compile("^" + full_msg)
    if "showProgress" not in options:
        options["showProgress"] = False
    EXPECT_THROWS(lambda: util.load_binlogs(input_url, options), full_msg)

def TEST_BOOL_OPTION(option):
    EXPECT_FAIL("TypeError", f"Argument #2: Option '{option}' is expected to be of type Bool, but is Null", options={option: None})
    EXPECT_FAIL("TypeError", f"Argument #2: Option '{option}' Bool expected, but value is String", options={option: "dummy"})
    EXPECT_FAIL("TypeError", f"Argument #2: Option '{option}' is expected to be of type Bool, but is Array", options={option: []})
    EXPECT_FAIL("TypeError", f"Argument #2: Option '{option}' is expected to be of type Bool, but is Map", options={option: {}})

def TEST_STRING_OPTION(option):
    EXPECT_FAIL("TypeError", f"Argument #2: Option '{option}' is expected to be of type String, but is Null", options={option: None})
    EXPECT_FAIL("TypeError", f"Argument #2: Option '{option}' is expected to be of type String, but is Integer", options={option: 5})
    EXPECT_FAIL("TypeError", f"Argument #2: Option '{option}' is expected to be of type String, but is Integer", options={option: -5})
    EXPECT_FAIL("TypeError", f"Argument #2: Option '{option}' is expected to be of type String, but is Array", options={option: []})
    EXPECT_FAIL("TypeError", f"Argument #2: Option '{option}' is expected to be of type String, but is Map", options={option: {}})
    EXPECT_FAIL("TypeError", f"Argument #2: Option '{option}' is expected to be of type String, but is Bool", options={option: False})

def TEST_UINT_OPTION(option):
    EXPECT_FAIL("TypeError", f"Argument #2: Option '{option}' is expected to be of type UInteger, but is Null", options={option: None})
    EXPECT_FAIL("TypeError", f"Argument #2: Option '{option}' UInteger expected, but Integer value is out of range", options={option: -5})
    EXPECT_FAIL("TypeError", f"Argument #2: Option '{option}' UInteger expected, but value is String", options={option: "dummy"})
    EXPECT_FAIL("TypeError", f"Argument #2: Option '{option}' is expected to be of type UInteger, but is Array", options={option: []})
    EXPECT_FAIL("TypeError", f"Argument #2: Option '{option}' is expected to be of type UInteger, but is Map", options={option: {}})

def DUMP_INSTANCE(output_url=default_since_dir, options={}):
    EXPECT_NO_THROWS(lambda: util.dump_instance(output_url, {"showProgress": False, **options}))

def LOAD_DUMP(input_url=default_since_dir, options={}):
    EXPECT_NO_THROWS(lambda: util.load_dump(input_url, {"skipBinlog": __version_num < 80000, "updateGtidSet": "replace", "resetProgress": True, "showProgress": False, **options}))

class Binary_log_file(NamedTuple):
    file: str
    position: int

class Binary_log_status(NamedTuple):
    binlog: Binary_log_file
    executed_gtid_set: str
    do_db: str = ""
    ignore_db: str = ""

def binary_log_status(s):
    if __version_num < 80200:
        query = "SHOW MASTER STATUS"
    else:
        query = "SHOW BINARY LOG STATUS"
    r = s.run_sql(query).fetch_one()
    return Binary_log_status(Binary_log_file(r[0], r[1]), r[4], r[2], r[3])

def binary_logs(s):
    l = []
    for r in s.run_sql("SHOW BINARY LOGS").fetch_all():
        l.append(Binary_log_file(r[0], r[1]))
    return l

def gtid_subtract(s, l, r):
    return s.run_sql('SELECT GTID_SUBTRACT(?,?)', [l, r]).fetch_one()[0]

# r contains all GTIDs from l
def gtid_subset(s, l, r):
    return s.run_sql('SELECT GTID_SUBSET(?,?)', [l, r]).fetch_one()[0]
