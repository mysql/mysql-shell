#@ {has_oci_environment('OS')}

#@<> INCLUDE dump_utils.inc
#@<> INCLUDE dump_binlogs_utils.inc

#@<> INCLUDE oci_utils.inc

#@<> entry point
# constants
local_since_dir = f"file://{default_since_dir}"

# helpers
def REMOTE_DUMP_SUCCESS(*args, **kwargs):
    PREPARE_PAR_IS_SECRET_TEST()
    EXPECT_DUMP_SUCCESS(*args, **kwargs)
    EXPECT_PAR_IS_SECRET()

def REMOTE_DUMP_FAIL(*args, **kwargs):
    PREPARE_PAR_IS_SECRET_TEST()
    EXPECT_DUMP_FAIL(*args, **kwargs)
    EXPECT_PAR_IS_SECRET()

def REMOTE_LOAD_SUCCESS(*args, **kwargs):
    PREPARE_PAR_IS_SECRET_TEST()
    EXPECT_LOAD_SUCCESS(*args, **kwargs)
    EXPECT_PAR_IS_SECRET()

def REMOTE_LOAD_FAIL(*args, **kwargs):
    PREPARE_PAR_IS_SECRET_TEST()
    EXPECT_LOAD_FAIL(*args, **kwargs)
    EXPECT_PAR_IS_SECRET()

def REMOTE_DUMP_INSTANCE(*args, **kwargs):
    PREPARE_PAR_IS_SECRET_TEST()
    DUMP_INSTANCE(*args, **kwargs)
    EXPECT_PAR_IS_SECRET()

def wipe_bucket():
    prepare_empty_bucket(OS_BUCKET_NAME, OS_NAMESPACE)

def create_test_par():
    return create_par(OS_NAMESPACE, OS_BUCKET_NAME, "AnyObjectReadWrite", "all-read-write-par", today_plus_days(1, True), "binlogs-test/", "ListObjects")

#@<> deploy sandboxes
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

#@<> setup
wipe_dir(default_dump_dir)
wipe_dir(default_since_dir)
wipe_dir(incremental_dump_dir)
wipe_bucket()

#@<> since + local dump instance
setup_session(dump_uri)
create_default_test_table(session)

DUMP_INSTANCE(local_since_dir)

create_default_test_table(session)

REMOTE_DUMP_SUCCESS(create_test_par(), options={"since": local_since_dir})

wipe_bucket()
wipe_dir(default_since_dir)
wipeout_server(session)

#@<> since + local dump binlogs
setup_session(dump_uri)
create_default_test_table(session)

EXPECT_DUMP_SUCCESS(local_since_dir, options={"startFrom": start_from_beginning})

create_default_test_table(session)

REMOTE_DUMP_SUCCESS(create_test_par(), options={"since": local_since_dir})

wipe_bucket()
wipe_dir(default_since_dir)
wipeout_server(session)

#@<> incremental dump + load
dump_par = create_test_par()

setup_session(dump_uri)
create_default_test_table(session)

REMOTE_DUMP_SUCCESS(dump_par, options={"startFrom": start_from_beginning})

create_default_test_table(session)

REMOTE_DUMP_SUCCESS(dump_par)

wipeout_server(session)
setup_session(dump_uri)

REMOTE_LOAD_SUCCESS(dump_par)

wipe_bucket()
wipeout_server(session)

#@<> multiple binlogs
setup_session(dump_uri)

for i in range(10):
    create_default_test_table(session)

REMOTE_DUMP_SUCCESS(create_test_par(), options={"startFrom": start_from_beginning})

wipe_bucket()

#@<> WL15977-FR2.4.6 - first dump fails {not __dbug_off}
# this reuses server state from the previous dump
testutil.set_trap("binlog", [f"file == {binlog_file_prefix}.000003", "++match_counter > 10"], { "msg": "Internal error." })

REMOTE_DUMP_FAIL("RuntimeError", "Internal error.", create_test_par(), options={"threads": 2, "startFrom": start_from_beginning})
EXPECT_STDOUT_CONTAINS("WARNING: Skipping clean up, it's not possible to remove files using the OCI prefix PAR.")
EXPECT_STDOUT_NOT_CONTAINS("Removing files...")

testutil.clear_traps("binlog")
wipe_bucket()

#@<> WL15977-FR2.4.6 - second dump fails {not __dbug_off}
# this reuses server state from the previous dump
dump_par = create_test_par()

REMOTE_DUMP_SUCCESS(dump_par, options={"startFrom": start_from_beginning})

for i in range(10):
    create_default_test_table(session)

testutil.set_trap("binlog", [f"file == {binlog_file_prefix}.000009", "++match_counter > 10"], { "msg": "Internal error." })

REMOTE_DUMP_FAIL("RuntimeError", "Internal error.", dump_par, options={"threads": 2})
EXPECT_STDOUT_CONTAINS("WARNING: Skipping clean up, it's not possible to remove files using the OCI prefix PAR.")
EXPECT_STDOUT_NOT_CONTAINS("Removing files...")

testutil.clear_traps("binlog")
wipeout_server(session)
wipe_bucket()

#@<> Cleanup
testutil.destroy_sandbox(__mysql_sandbox_port1)
testutil.destroy_sandbox(__mysql_sandbox_port2)
