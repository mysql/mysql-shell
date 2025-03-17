#@ {has_oci_environment('OS')}

#@<> INCLUDE dump_utils.inc
#@<> INCLUDE dump_binlogs_utils.inc

#@<> INCLUDE oci_utils.inc

#@<> entry point
# constants
local_since_dir = f"file://{default_since_dir}"

# helpers
def inject_remote_options(options):
    options["osBucketName"] = OS_BUCKET_NAME
    options["osNamespace"] = OS_NAMESPACE
    options["ociConfigFile"] = oci_config_file

def REMOTE_DUMP_SUCCESS(*args, **kwargs):
    inject_remote_options(kwargs.setdefault("options", {}))
    EXPECT_DUMP_SUCCESS(*args, **kwargs)

def REMOTE_DUMP_FAIL(*args, **kwargs):
    inject_remote_options(kwargs.setdefault("options", {}))
    EXPECT_DUMP_FAIL(*args, **kwargs)

def REMOTE_LOAD_SUCCESS(*args, **kwargs):
    inject_remote_options(kwargs.setdefault("options", {}))
    EXPECT_LOAD_SUCCESS(*args, **kwargs)

def REMOTE_LOAD_FAIL(*args, **kwargs):
    inject_remote_options(kwargs.setdefault("options", {}))
    EXPECT_LOAD_FAIL(*args, **kwargs)

def REMOTE_DUMP_INSTANCE(*args, **kwargs):
    inject_remote_options(kwargs.setdefault("options", {}))
    DUMP_INSTANCE(*args, **kwargs)

def wipe_bucket():
    prepare_empty_bucket(OS_BUCKET_NAME, OS_NAMESPACE)

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

REMOTE_DUMP_SUCCESS(options={"since": local_since_dir})

wipe_bucket()
wipe_dir(default_since_dir)
wipeout_server(session)

#@<> since + local dump binlogs
setup_session(dump_uri)
create_default_test_table(session)

EXPECT_DUMP_SUCCESS(local_since_dir, options={"startFrom": start_from_beginning})

create_default_test_table(session)

REMOTE_DUMP_SUCCESS(options={"since": local_since_dir})

wipe_bucket()
wipe_dir(default_since_dir)
wipeout_server(session)

#@<> incremental dump + load
setup_session(dump_uri)
create_default_test_table(session)

REMOTE_DUMP_SUCCESS(options={"startFrom": start_from_beginning})

create_default_test_table(session)

REMOTE_DUMP_SUCCESS()

wipeout_server(session)
setup_session(dump_uri)

REMOTE_LOAD_SUCCESS()

wipe_bucket()
wipeout_server(session)

#@<> multiple binlogs
setup_session(dump_uri)

for i in range(10):
    create_default_test_table(session)

REMOTE_DUMP_SUCCESS(options={"startFrom": start_from_beginning})

wipe_bucket()

#@<> WL15977-FR2.4.6 - first dump fails {not __dbug_off}
# this reuses server state from the previous dump
testutil.set_trap("binlog", [f"file == {binlog_file_prefix}.000003", "++match_counter > 10"], { "msg": "Internal error." })

REMOTE_DUMP_FAIL("RuntimeError", "Internal error.", options={"threads": 2, "startFrom": start_from_beginning})
EXPECT_STDOUT_CONTAINS("Removing files...")

testutil.clear_traps("binlog")

#@<> WL15977-FR2.4.6 - second dump fails {not __dbug_off}
# this reuses server state from the previous dump
REMOTE_DUMP_SUCCESS(options={"startFrom": start_from_beginning})

for i in range(10):
    create_default_test_table(session)

testutil.set_trap("binlog", [f"file == {binlog_file_prefix}.000009", "++match_counter > 10"], { "msg": "Internal error." })

REMOTE_DUMP_FAIL("RuntimeError", "Internal error.", options={"threads": 2})
EXPECT_STDOUT_CONTAINS("Removing files...")

testutil.clear_traps("binlog")

# there are no leftovers, another dump succeeds
REMOTE_DUMP_SUCCESS()

wipeout_server(session)
wipe_bucket()

#@<> Cleanup
testutil.destroy_sandbox(__mysql_sandbox_port1)
testutil.destroy_sandbox(__mysql_sandbox_port2)
