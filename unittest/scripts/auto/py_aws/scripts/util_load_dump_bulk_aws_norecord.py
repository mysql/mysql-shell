#@ {has_aws_environment() and __version_num >= 80400}

#@<> INCLUDE aws_utils.inc

#@<> INCLUDE dump_utils.inc

#@<> imports
from contextlib import ExitStack
cleanup = ExitStack()

#@<> prepare script used by the credential process
creds_script = os.path.abspath("creds.py")

with open(creds_script, 'w', encoding="utf-8") as f:
    f.write(f"""# returns hardcoded credentials
import json

creds = {{
    'Version': 1,
    'AccessKeyId': '{aws_settings["aws_access_key_id"]}',
    'SecretAccessKey': '{aws_settings["aws_secret_access_key"]}',
    'SessionToken': '',
    'Expiration': ''
}}

print(json.dumps(creds))
""")

#@<> prepare config file compatible with BULK LOAD
bulk_load_profile = "dataimport"
cleanup.push(write_profile(local_aws_config_file, "profile " + bulk_load_profile, { "credential_process": f"{__mysqlsh} --py --file {creds_script}" }))

#@<> export path to the config file, it's going to be used by the component
cleanup.push(set_env_var("AWS_CONFIG_FILE", local_aws_config_file))

# prepare config file used by tests
cleanup.push(write_profile(local_aws_credentials_file, "profile " + local_aws_profile, aws_settings))

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
test_schema = "wl15432"
test_table = "wl15432"
dump_dir = "wl15432"
required_privilege = "LOAD_FROM_S3"

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
    clean_bucket()
    shell.connect(__sandbox_uri1)
    EXPECT_NO_THROWS(lambda: util.dump_schemas([ test_schema ], dump_dir, { "s3BucketName": MYSQLSH_S3_BUCKET_NAME, "s3Profile": local_aws_profile, "s3ConfigFile": local_aws_credentials_file, "showProgress": False, **options }))

def TEST_LOAD(expect_bulk_loaded, target = __sandbox_uri2, options = {}, cleanup = True):
    if cleanup:
        remove_local_progress_file()
    shell.connect(target)
    if cleanup:
        wipeout_server(session)
    WIPE_OUTPUT()
    WIPE_SHELL_LOG()
    EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "s3BucketName": MYSQLSH_S3_BUCKET_NAME, "s3Profile": local_aws_profile, "s3ConfigFile": local_aws_credentials_file, "progressFile": local_progress_file, "showProgress": False, **options }))
    msg = " loaded using BULK LOAD."
    if expect_bulk_loaded > 0:
        EXPECT_STDOUT_CONTAINS(f"{expect_bulk_loaded} table{' was' if 1 == expect_bulk_loaded else 's were'}{msg}")
    else:
        EXPECT_STDOUT_NOT_CONTAINS(msg)
    EXPECT_JSON_EQ(snapshot_schema(src_session, test_schema), snapshot_schema(session, test_schema))

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
# monitoring is disabled
EXPECT_SHELL_LOG_CONTAINS("BULK LOAD monitoring: consumer is disabled")

#@<> BUG#36297348 - bulk load should be disabled if s3EndpointOverride is used {bulk_load_supported}
# we're using a valid S3 endpoint, the same which would be used normally
TEST_LOAD(expect_bulk_loaded = 0, options = { "s3EndpointOverride": f"https://s3.{aws_settings.get('region', default_aws_region)}.amazonaws.com" })

#@<> WL15432-TSFR_1_7 - missing privilege {bulk_load_supported}
revoke_bulk_Load_privilege(dst_session)

TEST_LOAD(expect_bulk_loaded = 0)
EXPECT_SHELL_LOG_CONTAINS("BULK LOAD: AWS S3 and 'LOAD_FROM_S3' privilege is missing")

#@<> WL15432-TSFR_1_7 - re-add privilege {bulk_load_supported}
grant_bulk_Load_privilege(dst_session)

#@<> valid table for compression tests - all supported data types
prepare_test_table("(f1 INT PRIMARY KEY, f2 TINYINT, f3 SMALLINT, f4 MEDIUMINT, f5 BIGINT, f6 VARCHAR(10), f7 CHAR(10), f8 FLOAT, f9 DOUBLE, f10 DECIMAL(4,2), f11 DATETIME, f12 DATE, f13 TIME)")

#@<> WL15432-TSFR_1_2_1 - 'compression' = 'none' {bulk_load_supported}
TEST_DUMP_AND_LOAD(expect_bulk_loaded = 1, dump_options = { "compression": "none" })

#@<> WL15432-TSFR_1_2_2 - 'compression' = 'zstd' {bulk_load_supported}
TEST_DUMP_AND_LOAD(expect_bulk_loaded = 1, dump_options = { "compression": "zstd" })

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

#@<> Cleanup
cleanup.close()
cleanup_variables()
testutil.destroy_sandbox(__mysql_sandbox_port1)
testutil.destroy_sandbox(__mysql_sandbox_port2)
