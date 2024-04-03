#@ {has_oci_environment('OS') and __version_num >= 80400}

#@<> INCLUDE oci_utils.inc

#@<> INCLUDE dump_utils.inc

#@<> imports
import json
import os.path
import threading
import time

#@<> deploy source instance
testutil.deploy_raw_sandbox(__mysql_sandbox_port1, "root", {
    "local_infile": 1
})
testutil.wait_sandbox_alive(__sandbox_uri1)

#@<> deploy destination instance
testutil.deploy_raw_sandbox(__mysql_sandbox_port2, "root", {
    "skip-log-bin": "",
    "local_infile": 1
})
testutil.wait_sandbox_alive(__sandbox_uri2)

#@<> constants
RFC3339 = True
oci_config_file = os.path.join(OCI_CONFIG_HOME, "config")

test_schema = "wl15432"
test_table = "wl15432"
dump_dir = "wl15432"
load_src = None
required_privilege = "LOAD_FROM_URL"

src_session = shell.open_session(__sandbox_uri1)
dst_session = shell.open_session(__sandbox_uri2)

#@<> helpers
def grant_bulk_Load_privilege(s = dst_session):
    grant_privilege(s, required_privilege, "*.*")

def revoke_bulk_Load_privilege(s = dst_session):
    revoke_privilege(s, required_privilege, "*.*")

def init_bulk_load(s = dst_session):
    r = enable_bulk_load(s)
    if r:
        grant_bulk_Load_privilege(s)
    return r

def unload_bulk_load(s = dst_session):
    disable_bulk_load(s)

def DUMP(options = {}):
    prepare_empty_bucket(OS_BUCKET_NAME, OS_NAMESPACE)
    shell.connect(__sandbox_uri1)
    EXPECT_NO_THROWS(lambda: util.dump_schemas([ test_schema ], dump_dir, { "osBucketName": OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile": oci_config_file, "showProgress": False, **options }))
    global load_src
    load_src = create_par(OS_NAMESPACE, OS_BUCKET_NAME, "AnyObjectRead", "all-read-par", today_plus_days(1, RFC3339), f"{dump_dir}/", "ListObjects")

def TEST_LOAD(expect_bulk_loaded, target = __sandbox_uri2, options = {}, cleanup = True):
    if cleanup:
        remove_local_progress_file()
    shell.connect(target)
    if cleanup:
        wipeout_server(session)
    PREPARE_PAR_IS_SECRET_TEST()
    EXPECT_NO_THROWS(lambda: util.load_dump(load_src, { "progressFile": local_progress_file, "showProgress": False, **options }))
    EXPECT_PAR_IS_SECRET()
    msg = " loaded using BULK LOAD."
    if expect_bulk_loaded > 0:
        EXPECT_STDOUT_CONTAINS(f"{expect_bulk_loaded} table{' was' if 1 == expect_bulk_loaded else 's were'}{msg}")
    else:
        EXPECT_STDOUT_NOT_CONTAINS(msg)
    EXPECT_JSON_EQ(snapshot_schema(src_session, test_schema), snapshot_schema(session, test_schema))

def TEST_FAILED_LOAD(msg, target = __sandbox_uri2, options = {}):
    remove_local_progress_file()
    shell.connect(target)
    wipeout_server(session)
    PREPARE_PAR_IS_SECRET_TEST()
    EXPECT_THROWS(lambda: util.load_dump(load_src, { "progressFile": local_progress_file, "showProgress": False, **options }), msg)
    EXPECT_PAR_IS_SECRET()

def TEST_DUMP_AND_LOAD(expect_bulk_loaded, dump_options = {}, load_options = {}):
    DUMP(options = dump_options)
    TEST_LOAD(expect_bulk_loaded, options = load_options)

def prepare_test_table(create_definition, table_name = test_table, nrows = 1000):
    create_test_table(src_session, test_schema, table_name, create_definition, nrows)

#@<> Setup
bulk_load_supported = init_bulk_load()

#@<> Prepare dump for WL15432-TSFR_1_X tests
prepare_test_table("(f1 INT PRIMARY KEY)")
DUMP()

#@<> WL15432-TSFR_1_1 - BULK LOAD works {bulk_load_supported}
TEST_LOAD(expect_bulk_loaded = 1)
EXPECT_SHELL_LOG_CONTAINS("BULK LOAD is supported")

#@<> unsupported FS - using OCI's config file {bulk_load_supported}
par = load_src
load_src = dump_dir

TEST_LOAD(expect_bulk_loaded = 0, options = { "osBucketName": OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile": oci_config_file })
EXPECT_SHELL_LOG_CONTAINS("BULK LOAD: unsupported FS")

load_src = par

#@<> WL15432-TSFR_1_2 - unsupported version {bulk_load_supported and not __dbug_off}
testutil.dbug_set("+d,dump_loader_bulk_unsupported_version")

TEST_LOAD(expect_bulk_loaded = 0)
EXPECT_STDOUT_CONTAINS("Target is MySQL 8.3.0.")
EXPECT_SHELL_LOG_CONTAINS("BULK LOAD: unsupported version")

testutil.dbug_set("")

#@<> WL15432-TSFR_1_3 - missing component {bulk_load_supported}
unload_bulk_load()

TEST_LOAD(expect_bulk_loaded = 0)
EXPECT_SHELL_LOG_CONTAINS("BULK LOAD: component is not loaded")

#@<> WL15432-TSFR_1_3 - re-enable component {bulk_load_supported}
bulk_load_supported = init_bulk_load()

#@<> WL15432-TSFR_1_4 - binlog enabled {bulk_load_supported}
init_bulk_load(src_session)

TEST_LOAD(expect_bulk_loaded = 0, target = __sandbox_uri1)
EXPECT_SHELL_LOG_CONTAINS("BULK LOAD: binlog is enabled and 'skipBinlog' option is false")

#@<> WL15432-TSFR_1_4 - disable component {bulk_load_supported}
unload_bulk_load(src_session)

#@<> WL15432-TSFR_1_5 - binlog enabled and 'skipBinlog' = true {bulk_load_supported}
init_bulk_load(src_session)

TEST_LOAD(expect_bulk_loaded = 1, target = __sandbox_uri1, options = { "skipBinlog": True })

#@<> WL15432-TSFR_1_5 - disable component {bulk_load_supported}
unload_bulk_load(src_session)

#@<> WL15432-TSFR_1_6 - missing privilege {bulk_load_supported}
revoke_bulk_Load_privilege(dst_session)

TEST_LOAD(expect_bulk_loaded = 0)
EXPECT_SHELL_LOG_CONTAINS("BULK LOAD: OCI prefix PAR and 'LOAD_FROM_URL' privilege is missing")

#@<> WL15432-TSFR_1_6 - re-add privilege {bulk_load_supported}
grant_bulk_Load_privilege(dst_session)

#@<> WL15432-TSFR_1_8 - help text
help_text = """
      If target MySQL server supports BULK LOAD, the load operation of
      compatible tables can be offloaded to the target server, which
      parallelizes and loads data directly from the Cloud storage.
"""
EXPECT_TRUE(help_text in util.help("load_dump"))

help_text = """
      - disableBulkLoad: bool (default: false) - Do not use BULK LOAD feature
        to load the data, even when available.
"""
EXPECT_TRUE(help_text in util.help("load_dump"))

#@<> valid table for compression tests - all supported data types
prepare_test_table("(f1 INT PRIMARY KEY, f2 TINYINT, f3 SMALLINT, f4 MEDIUMINT, f5 BIGINT, f6 VARCHAR(10), f7 CHAR(10), f8 FLOAT, f9 DOUBLE, f10 DECIMAL(4,2), f11 DATETIME, f12 DATE, f13 TIME)")

#@<> WL15432-TSFR_1_2_1 - 'compression' = 'none' {bulk_load_supported}
TEST_DUMP_AND_LOAD(expect_bulk_loaded = 1, dump_options = { "compression": "none" })

# WL15432-TSFR_2_2_1 - 'disableBulkLoad' = false
TEST_LOAD(expect_bulk_loaded = 1, options = { "disableBulkLoad": False })

#@<> WL15432-TSFR_1_2_2 - 'compression' = 'zstd' {bulk_load_supported}
TEST_DUMP_AND_LOAD(expect_bulk_loaded = 1, dump_options = { "compression": "zstd" })

# WL15432-TSFR_2_2_2 - 'disableBulkLoad' = true
TEST_LOAD(expect_bulk_loaded = 0, options = { "disableBulkLoad": True })
EXPECT_SHELL_LOG_CONTAINS("BULK LOAD is explicitly disabled")

#@<> WL15432-TSFR_1_2_3 - 'compression' = 'gzip' - not supported {bulk_load_supported}
TEST_DUMP_AND_LOAD(expect_bulk_loaded = 0, dump_options = { "compression": "gzip" })
EXPECT_SHELL_LOG_CONTAINS(f"Table `{test_schema}`.`{test_table}` will not use BULK LOAD: unsupported compression: gzip")

#@<> WL15432-TSFR_1_2_4 - unsupported data type {bulk_load_supported}
prepare_test_table("(f1 INT PRIMARY KEY, f2 JSON)")
TEST_DUMP_AND_LOAD(expect_bulk_loaded = 0)
EXPECT_SHELL_LOG_CONTAINS(f"Table `{test_schema}`.`{test_table}` is not compatible with BULK LOAD: MySQL Error 3658 (HY000): Feature json column type is unsupported (LOAD DATA ALGORITHM = BULK)")
EXPECT_SHELL_LOG_CONTAINS(f"Table `{test_schema}`.`{test_table}` will not use BULK LOAD: not compatible")

#@<> WL15432-TSFR_1_2_4 - data type which is encoded and requires preprocessing when loading - not supported by BULK LOAD {bulk_load_supported}
prepare_test_table("(f1 INT PRIMARY KEY, f2 BINARY(3))")
TEST_DUMP_AND_LOAD(expect_bulk_loaded = 0)
EXPECT_SHELL_LOG_CONTAINS(f"Table `{test_schema}`.`{test_table}` is not compatible with BULK LOAD: MySQL Error 1221 (HY000): Incorrect usage of LOAD DATA with BULK Algorithm and column list specification")
EXPECT_SHELL_LOG_CONTAINS(f"Table `{test_schema}`.`{test_table}` will not use BULK LOAD: not compatible")

#@<> WL15432-TSFR_1_2_5 - setup
src_session.run_sql("DROP SCHEMA IF EXISTS !", [ test_schema ])
num_tables = 2

for i in range(num_tables):
    prepare_test_table("(f1 INT PRIMARY KEY, f2 CHAR(200))", table_name = f"{test_table}_{i}")

DUMP(options = { "bytesPerChunk": "128k" })

# download some files and remove them from the dump, simulating dump in progress
missing_files = [ f"{test_schema}@{test_table}_0@@1.tsv.zst", "@.done.json" ]

for f in missing_files:
    testutil.anycopy({ "osBucketName": OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile": oci_config_file, "name": f"{dump_dir}/{f}"}, f)
    delete_object(OS_BUCKET_NAME, f"{dump_dir}/{f}", OS_NAMESPACE)

def copy_missing_files():
    time.sleep(10)
    for f in missing_files:
        testutil.anycopy(f, { "osBucketName": OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile": oci_config_file, "name": f"{dump_dir}/{f}"})

#@<> WL15432-TSFR_1_2_5 - load dump which is ongoing {bulk_load_supported}
# copy the remaining files after a few secs
threading.Thread(target = copy_missing_files).start()

# both tables are compatible, but only one of them is complete when load starts
TEST_LOAD(expect_bulk_loaded = 1, options = { "waitDumpTimeout": 60 })

#@<> WL15432-TSFR_1_2_5 - cleanup
src_session.run_sql("DROP SCHEMA IF EXISTS !", [ test_schema ])

#@<> WL15432-TSFR_1_2_6 - load dump twice {bulk_load_supported}
prepare_test_table("(f1 INT PRIMARY KEY)")
TEST_DUMP_AND_LOAD(expect_bulk_loaded = 1)
TEST_LOAD(expect_bulk_loaded = 0, cleanup = False)

#@<> WL15432-TSFR_1_2_7 - resume interrupted load operation {bulk_load_supported and not __dbug_off}
prepare_test_table("(f1 INT PRIMARY KEY)")
DUMP()

# simulate an interrupted bulk load
testutil.dbug_set("+d,dump_loader_bulk_interrupt")
TEST_FAILED_LOAD("Error loading dump")
EXPECT_SHELL_LOG_CONTAINS(f"Table `{test_schema}`.`{test_table}` will use BULK LOAD")
EXPECT_STDOUT_CONTAINS("Aborting load...")

# resume operation - bulk load is still used
testutil.dbug_set("")
TEST_LOAD(expect_bulk_loaded = 1, cleanup = False)

#@<> a table for index tests
prepare_test_table("(f1 INT PRIMARY KEY, f2 INT, INDEX f2i (f2))")
DUMP()

#@<> WL15432-TSFR_1_2_8 - table with secondary index is not bulk loaded {bulk_load_supported}
TEST_LOAD(expect_bulk_loaded = 0)
EXPECT_SHELL_LOG_CONTAINS(f"Table `{test_schema}`.`{test_table}` is not compatible with BULK LOAD: MySQL Error 4132 (HY000): Table '{test_schema}/{test_table}' has indexes other than primary key.")
EXPECT_SHELL_LOG_CONTAINS(f"Table `{test_schema}`.`{test_table}` will not use BULK LOAD: not compatible")

#@<> WL15432-TSFR_1_2_9 - table with secondary index is bulk loaded when 'deferTableIndexes' = 'all' is given {bulk_load_supported}
TEST_LOAD(expect_bulk_loaded = 1, options = { "deferTableIndexes": "all" })

#@<> a table for partition tests
prepare_test_table("""(f1 INT PRIMARY KEY)
PARTITION BY RANGE (f1) (
    PARTITION p0 VALUES LESS THAN (250),
    PARTITION p1 VALUES LESS THAN (500),
    PARTITION p2 VALUES LESS THAN (750),
    PARTITION p3 VALUES LESS THAN (MAXVALUE)
)""")
DUMP()

#@<> WL15432-TSFR_1_3_1 - partitioned table {bulk_load_supported}
TEST_LOAD(expect_bulk_loaded = 1)

#@<> WL15432-TSFR_1_3_2 - resume interrupted load operation - partitioned table {bulk_load_supported and not __dbug_off}
# simulate an interrupted bulk load
testutil.dbug_set("+d,dump_loader_bulk_interrupt")
TEST_FAILED_LOAD("Error loading dump")
EXPECT_SHELL_LOG_CONTAINS(f"Table `{test_schema}`.`{test_table}` will use BULK LOAD")
EXPECT_STDOUT_CONTAINS("Aborting load...")

# target schema should contain just one table, no temporary ones created to handle partitioning
EXPECT_EQ(1, dst_session.run_sql("SELECT COUNT(*) FROM information_schema.tables WHERE TABLE_SCHEMA=?", [ test_schema ]).fetch_one()[0])

# create a table which uses the comment for temporary tables, it should be wiped while load is being done
dst_session.run_sql(f"CREATE TABLE !.`{test_table}-tmp` (a int) COMMENT='mysqlsh-tmp-218ffeec-b67b-4886-9a8c-d7812a1f93eb'", [ test_schema ])

# resume operation - bulk load is still used
testutil.dbug_set("")
TEST_LOAD(expect_bulk_loaded = 1, cleanup = False)

# target schema should contain just one table, artificially created table is gone
EXPECT_EQ(1, dst_session.run_sql("SELECT COUNT(*) FROM information_schema.tables WHERE TABLE_SCHEMA=?", [ test_schema ]).fetch_one()[0])

#@<> WL15432-TSFR_1_4_1 - setup
src_session.run_sql("DROP SCHEMA IF EXISTS !", [ test_schema ])
num_tables = 10

for i in range(num_tables):
    prepare_test_table("""(f1 INT PRIMARY KEY, f2 CHAR(200))
PARTITION BY RANGE (f1) (
    PARTITION p0 VALUES LESS THAN (1000),
    PARTITION p1 VALUES LESS THAN (2000),
    PARTITION p2 VALUES LESS THAN (3000),
    PARTITION p3 VALUES LESS THAN (MAXVALUE)
)""", table_name = f"{test_table}_{i}", nrows = 4000)

DUMP(options = { "bytesPerChunk": "128k" })

#@<> WL15432-TSFR_1_4_1 - save bulk_loader.concurrency {bulk_load_supported}
dst_session.run_sql("SET @saved_bulk_loader_concurrency = @@GLOBAL.bulk_loader.concurrency")
dst_session.run_sql("SET @saved_innodb_buffer_pool_size = @@GLOBAL.innodb_buffer_pool_size")
dst_session.run_sql("SET @@GLOBAL.innodb_buffer_pool_size = 1024 * 1024 * 1024")

#@<> WL15432-TSFR_1_4_1 - bulk_loader.concurrency << threads {bulk_load_supported}
dst_session.run_sql("SET @@GLOBAL.bulk_loader.concurrency = 2")
TEST_LOAD(expect_bulk_loaded = num_tables, options = { "threads": 16 })

#@<> WL15432-TSFR_1_4_1 - bulk_loader.concurrency = threads {bulk_load_supported}
dst_session.run_sql("SET @@GLOBAL.bulk_loader.concurrency = 4")
TEST_LOAD(expect_bulk_loaded = num_tables, options = { "threads": 4 })

#@<> WL15432-TSFR_1_4_1 - bulk_loader.concurrency > threads {bulk_load_supported}
dst_session.run_sql("SET @@GLOBAL.bulk_loader.concurrency = 8")
TEST_LOAD(expect_bulk_loaded = num_tables, options = { "threads": 4 })

#@<> WL15432-TSFR_1_4_1 - restore bulk_loader.concurrency {bulk_load_supported}
dst_session.run_sql("SET @@GLOBAL.bulk_loader.concurrency = @saved_bulk_loader_concurrency")
dst_session.run_sql("SET @@GLOBAL.innodb_buffer_pool_size = @saved_innodb_buffer_pool_size")

#@<> WL15432-TSFR_1_4_1 - cleanup
src_session.run_sql("DROP SCHEMA IF EXISTS !", [ test_schema ])

#@<> WL15432-TSFR_2_1_1 - 'disableBulkLoad' - invalid types
for opt in [(None, "Null"), ([], "Array"), ({}, "Map")]:
    EXPECT_THROWS(lambda: util.load_dump("", { "disableBulkLoad": opt[0] }), f"TypeError: Util.load_dump: Argument #2: Option 'disableBulkLoad' is expected to be of type Bool, but is {opt[1]}")

EXPECT_THROWS(lambda: util.load_dump("", { "disableBulkLoad": "str" }), f"TypeError: Util.load_dump: Argument #2: Option 'disableBulkLoad' Bool expected, but value is String")

#@<> Prepare dump for monitoring tests
prepare_test_table("(f1 INT PRIMARY KEY, f2 CHAR(200))", nrows = 10000)
DUMP()

# make sure monitoring is disabled
dst_session.run_sql("UPDATE performance_schema.setup_consumers SET ENABLED='NO' WHERE NAME='events_stages_current'")
dst_session.run_sql("UPDATE performance_schema.setup_instruments SET ENABLED='NO' WHERE NAME='stage/bulk_load_sorted/loading'")

#@<> monitoring is disabled {bulk_load_supported}
TEST_LOAD(expect_bulk_loaded = 1)
EXPECT_SHELL_LOG_CONTAINS("BULK LOAD monitoring: consumer is disabled")

#@<> monitoring - enable consumer - monitoring is still disabled {bulk_load_supported}
dst_session.run_sql("UPDATE performance_schema.setup_consumers SET ENABLED='YES' WHERE NAME='events_stages_current'")
TEST_LOAD(expect_bulk_loaded = 1)
EXPECT_SHELL_LOG_CONTAINS("BULK LOAD monitoring: instrumentation is disabled")

#@<> monitoring - enable instrumentation - monitoring is enabled {bulk_load_supported}
dst_session.run_sql("UPDATE performance_schema.setup_instruments SET ENABLED='YES' WHERE NAME='stage/bulk_load_sorted/loading'")
TEST_LOAD(expect_bulk_loaded = 1)
EXPECT_SHELL_LOG_CONTAINS("BULK LOAD monitoring is enabled")

# check progress file for table progress
validate_load_progress(local_progress_file)

bulk_load_entries = []

with open(local_progress_file) as f:
    for line in f:
        content = json.loads(line)
        if content["op"] == "TABLE-DATA-BULK":
            bulk_load_entries.append(content)

# there should be at least three entries, start and finish + some progress reports
EXPECT_LE(3, len(bulk_load_entries))

#@<> BUG#36477603 - loading a table which contains a virtual column generates a warning
prepare_test_table("(first_name VARCHAR(10), last_name VARCHAR(10), full_name VARCHAR(255) AS (CONCAT(first_name,' ',last_name)), address VARCHAR(10))")
TEST_DUMP_AND_LOAD(expect_bulk_loaded = 0)
EXPECT_STDOUT_NOT_CONTAINS("error 1261: Row 1 doesn't contain data for all columns")

#@<> Cleanup
testutil.destroy_sandbox(__mysql_sandbox_port1)
testutil.destroy_sandbox(__mysql_sandbox_port2)
