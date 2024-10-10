#@ {VER(>=8.0.0)}

#@<> INCLUDE dump_utils.inc
#@<> INCLUDE dump_binlogs_utils.inc

#@<> entry point
# helpers
def EXPECT_SUCCESS(*args, **kwargs):
    EXPECT_DUMP_SUCCESS(*args, **kwargs)

def EXPECT_FAIL(*args, **kwargs):
    EXPECT_DUMP_FAIL(*args, **kwargs)

def EXPECT_SOURCE_HAS_CHANGED(previous, current):
    EXPECT_STDOUT_MATCHES(re.compile(f"NOTE: Previous dump was taken from '.*:{previous}', current instance is: '.*:{current}', scanning binary log files for starting position"))

#@<> deploy sandboxes
testutil.deploy_raw_sandbox(__mysql_sandbox_port1, "root", {
    "server-id": str(random.randint(1, 4294967295)),
    **mysqld_options,
})
testutil.wait_sandbox_alive(__sandbox_uri1)

testutil.deploy_raw_sandbox(__mysql_sandbox_port2, "root", {
    "server-id": str(random.randint(1, 4294967295)),
    **mysqld_options,
})
testutil.wait_sandbox_alive(__sandbox_uri2)

testutil.deploy_raw_sandbox(__mysql_sandbox_port3, "root", {
    "server-id": str(random.randint(1, 4294967295)),
    **mysqld_options,
})
testutil.wait_sandbox_alive(__sandbox_uri3)

dump_instance_dir = random_string(10)
dump_binlogs_dir = random_string(10)
working_binlogs_dir = random_string(10)

#@<> WL15977-FR2.4.2 - different source instances
setup_session(__sandbox_uri1)
EXPECT_SUCCESS(dump_binlogs_dir, options={"startFrom": start_from_beginning})

setup_session(__sandbox_uri2)
EXPECT_FAIL("ValueError", "The source instance is different from the one used in the previous dump.", dump_binlogs_dir)

wipe_dir(dump_binlogs_dir)

#@<> create replica set
setup_session(__sandbox_uri1)
create_default_test_table(session)
rs = dba.create_replica_set("rs")

setup_session(__sandbox_uri2)
create_default_test_table(session)
rs = dba.create_replica_set("rs")
rs.add_instance(__sandbox_uri3, {"recoveryMethod": "clone"})

# dump from one instance
EXPECT_NO_THROWS(lambda: util.dump_instance(dump_instance_dir, {"showProgress": False}))
EXPECT_SUCCESS(dump_binlogs_dir, options={"startFrom": start_from_beginning})

# add more data
create_default_test_table(session)
testutil.wait_member_transactions(__mysql_sandbox_port3, __mysql_sandbox_port2)

expected_gtid_set = binary_log_status(session).executed_gtid_set

#@<> `since` dump_instance() - replica set
setup_session(__sandbox_uri3)

EXPECT_SUCCESS(working_binlogs_dir, options={"since": dump_instance_dir})
EXPECT_SOURCE_HAS_CHANGED(__mysql_sandbox_port2, __mysql_sandbox_port3)

wipe_dir(working_binlogs_dir)

#@<> WL15977-FR2.4.2 - wrong instance - `since` dump_instance() - replica set
setup_session(__sandbox_uri1)

EXPECT_FAIL("ValueError", "The source instance is different from the one used in the previous dump and they do not belong to the same replication/managed topology.", working_binlogs_dir, options={"since": dump_instance_dir})

#@<> `since` dump_binlogs() - replica set
setup_session(__sandbox_uri3)

EXPECT_SUCCESS(working_binlogs_dir, options={"since": dump_binlogs_dir})
EXPECT_SOURCE_HAS_CHANGED(__mysql_sandbox_port2, __mysql_sandbox_port3)

wipe_dir(working_binlogs_dir)

#@<> WL15977-FR2.4.2 - wrong instance - `since` dump_binlogs() - replica set
setup_session(__sandbox_uri1)

EXPECT_FAIL("ValueError", "The source instance is different from the one used in the previous dump and they do not belong to the same replication/managed topology.", working_binlogs_dir, options={"since": dump_binlogs_dir})

#@<> WL15977-FR2.4.2 - wrong instance - incremental dump - replica set
setup_session(__sandbox_uri1)

EXPECT_FAIL("ValueError", "The source instance is different from the one used in the previous dump and they do not belong to the same replication/managed topology.", dump_binlogs_dir)

#@<> incremental dump - replica set
setup_session(__sandbox_uri3)

EXPECT_SUCCESS(dump_binlogs_dir)
EXPECT_SOURCE_HAS_CHANGED(__mysql_sandbox_port2, __mysql_sandbox_port3)

#@<> incremental dump - nothing more to be dumped - replica set
setup_session(__sandbox_uri2)

EXPECT_SUCCESS(dump_binlogs_dir, nothing_dumped=True)

#@<> WL15977-FR2.3.3 - warn that instance is not ONLINE - replica set
setup_session(__sandbox_uri3)
session.run_sql("STOP REPLICA")
testutil.wait_replication_channel_state(__mysql_sandbox_port3, "read_replica_replication", "OFF")

EXPECT_SUCCESS(dump_binlogs_dir, nothing_dumped=True)
EXPECT_STDOUT_CONTAINS("WARNING: The source instance is part of a replication/managed topology, but it's not currently ONLINE.")

#@<> load the binlog dump - replica set
setup_session(__sandbox_uri1)
rs = dba.get_replica_set()
rs.dissolve({'force': True})
reset_instance(session)
wipeout_server(session)

EXPECT_LOAD_SUCCESS(dump_binlogs_dir)
EXPECT_TRUE(gtid_subset(session, expected_gtid_set, binary_log_status(session).executed_gtid_set))

wipeout_server(session)

#@<> dissolve replica set
setup_session(__sandbox_uri2)
rs = dba.get_replica_set()
rs.dissolve({'force': True})
reset_instance(session)
wipeout_server(session)

setup_session(__sandbox_uri3)
reset_instance(session)
wipeout_server(session)

wipe_dir(dump_instance_dir)
wipe_dir(dump_binlogs_dir)
wipe_dir(working_binlogs_dir)

#@<> create cluster
setup_session(__sandbox_uri1)
create_default_test_table(session)
c = dba.create_cluster("C")

setup_session(__sandbox_uri2)
create_default_test_table(session)
c = dba.create_cluster("C")
c.add_instance(__sandbox_uri3, {"recoveryMethod": "clone"})

# dump from one instance
EXPECT_NO_THROWS(lambda: util.dump_instance(dump_instance_dir, {"showProgress": False}))
EXPECT_SUCCESS(dump_binlogs_dir, options={"startFrom": start_from_beginning})

# add more data
create_default_test_table(session)
testutil.wait_member_transactions(__mysql_sandbox_port3, __mysql_sandbox_port2)

expected_gtid_set = binary_log_status(session).executed_gtid_set

#@<> `since` dump_instance() - cluster
setup_session(__sandbox_uri3)

EXPECT_SUCCESS(working_binlogs_dir, options={"since": dump_instance_dir})
EXPECT_SOURCE_HAS_CHANGED(__mysql_sandbox_port2, __mysql_sandbox_port3)

wipe_dir(working_binlogs_dir)

#@<> WL15977-FR2.4.2 - wrong instance - `since` dump_instance() - cluster
setup_session(__sandbox_uri1)

EXPECT_FAIL("ValueError", "The source instance is different from the one used in the previous dump and they do not belong to the same replication/managed topology.", working_binlogs_dir, options={"since": dump_instance_dir})

#@<> `since` dump_binlogs() - cluster
setup_session(__sandbox_uri3)

EXPECT_SUCCESS(working_binlogs_dir, options={"since": dump_binlogs_dir})
EXPECT_SOURCE_HAS_CHANGED(__mysql_sandbox_port2, __mysql_sandbox_port3)

wipe_dir(working_binlogs_dir)

#@<> WL15977-FR2.4.2 - wrong instance - `since` dump_binlogs() - cluster
setup_session(__sandbox_uri1)

EXPECT_FAIL("ValueError", "The source instance is different from the one used in the previous dump and they do not belong to the same replication/managed topology.", working_binlogs_dir, options={"since": dump_binlogs_dir})

#@<> WL15977-FR2.4.2 - wrong instance - incremental dump - cluster
setup_session(__sandbox_uri1)

EXPECT_FAIL("ValueError", "The source instance is different from the one used in the previous dump and they do not belong to the same replication/managed topology.", dump_binlogs_dir)

#@<> incremental dump - cluster
setup_session(__sandbox_uri3)

EXPECT_SUCCESS(dump_binlogs_dir)
EXPECT_SOURCE_HAS_CHANGED(__mysql_sandbox_port2, __mysql_sandbox_port3)

#@<> incremental dump - nothing more to be dumped - cluster
setup_session(__sandbox_uri2)

EXPECT_SUCCESS(dump_binlogs_dir, nothing_dumped=True)

#@<> WL15977-FR2.3.3 - warn that instance is not ONLINE - cluster
setup_session(__sandbox_uri3)
session.run_sql("STOP GROUP_REPLICATION")

setup_session(__sandbox_uri2)
testutil.wait_member_state(__mysql_sandbox_port3, "(MISSING)")

setup_session(__sandbox_uri3)

EXPECT_SUCCESS(dump_binlogs_dir, nothing_dumped=True)
EXPECT_STDOUT_CONTAINS("WARNING: The source instance is part of a replication/managed topology, but it's not currently ONLINE.")

#@<> load the binlog dump - cluster
setup_session(__sandbox_uri1)
c = dba.get_cluster()
c.dissolve({'force': True})
reset_instance(session)
wipeout_server(session)

EXPECT_LOAD_SUCCESS(dump_binlogs_dir)
EXPECT_TRUE(gtid_subset(session, expected_gtid_set, binary_log_status(session).executed_gtid_set))

wipeout_server(session)

#@<> dissolve cluster
setup_session(__sandbox_uri2)
c = dba.get_cluster()
c.dissolve({'force': True})
reset_instance(session)
wipeout_server(session)

setup_session(__sandbox_uri3)
reset_instance(session)
wipeout_server(session)

wipe_dir(dump_instance_dir)
wipe_dir(dump_binlogs_dir)
wipe_dir(working_binlogs_dir)

#@<> create cluster set
setup_session(__sandbox_uri1)
create_default_test_table(session)
c = dba.create_cluster("C1")
cs = c.create_cluster_set("cs")

setup_session(__sandbox_uri2)
create_default_test_table(session)
c = dba.create_cluster("C1")
cs = c.create_cluster_set("cs")
cs.create_replica_cluster(__sandbox_uri3, "C2", {"recoveryMethod": "clone"})

# dump from one instance
EXPECT_NO_THROWS(lambda: util.dump_instance(dump_instance_dir, {"showProgress": False}))
EXPECT_SUCCESS(dump_binlogs_dir, options={"startFrom": start_from_beginning})

# add more data
create_default_test_table(session)
testutil.wait_member_transactions(__mysql_sandbox_port3, __mysql_sandbox_port2)

expected_gtid_set = binary_log_status(session).executed_gtid_set

#@<> `since` dump_instance() - cluster set
setup_session(__sandbox_uri3)

EXPECT_SUCCESS(working_binlogs_dir, options={"since": dump_instance_dir})
EXPECT_SOURCE_HAS_CHANGED(__mysql_sandbox_port2, __mysql_sandbox_port3)

wipe_dir(working_binlogs_dir)

#@<> WL15977-FR2.4.2 - wrong instance - `since` dump_instance() - cluster set
setup_session(__sandbox_uri1)

EXPECT_FAIL("ValueError", "The source instance is different from the one used in the previous dump and they do not belong to the same replication/managed topology.", working_binlogs_dir, options={"since": dump_instance_dir})

#@<> `since` dump_binlogs() - cluster set
setup_session(__sandbox_uri3)

EXPECT_SUCCESS(working_binlogs_dir, options={"since": dump_binlogs_dir})
EXPECT_SOURCE_HAS_CHANGED(__mysql_sandbox_port2, __mysql_sandbox_port3)

wipe_dir(working_binlogs_dir)

#@<> WL15977-FR2.4.2 - wrong instance - `since` dump_binlogs() - cluster set
setup_session(__sandbox_uri1)

EXPECT_FAIL("ValueError", "The source instance is different from the one used in the previous dump and they do not belong to the same replication/managed topology.", working_binlogs_dir, options={"since": dump_binlogs_dir})

#@<> WL15977-FR2.4.2 - wrong instance - incremental dump - cluster set
setup_session(__sandbox_uri1)

EXPECT_FAIL("ValueError", "The source instance is different from the one used in the previous dump and they do not belong to the same replication/managed topology.", dump_binlogs_dir)

#@<> incremental dump - cluster set
setup_session(__sandbox_uri3)

EXPECT_SUCCESS(dump_binlogs_dir)
EXPECT_SOURCE_HAS_CHANGED(__mysql_sandbox_port2, __mysql_sandbox_port3)

#@<> incremental dump - nothing more to be dumped - cluster set
setup_session(__sandbox_uri2)

EXPECT_SUCCESS(dump_binlogs_dir, nothing_dumped=True)

#@<> WL15977-FR2.3.3 - warn that instance is not ONLINE - cluster set
setup_session(__sandbox_uri3)
session.run_sql("STOP GROUP_REPLICATION")

setup_session(__sandbox_uri2)
testutil.wait_member_state(__mysql_sandbox_port3, "(MISSING)")

setup_session(__sandbox_uri3)

EXPECT_SUCCESS(dump_binlogs_dir, nothing_dumped=True)
EXPECT_STDOUT_CONTAINS("WARNING: The source instance is part of a replication/managed topology, but it's not currently ONLINE.")

#@<> load the binlog dump - cluster set
setup_session(__sandbox_uri1)
cs = dba.get_cluster_set()
cs.dissolve({'force': True})
reset_instance(session)
wipeout_server(session)

EXPECT_LOAD_SUCCESS(dump_binlogs_dir)
EXPECT_TRUE(gtid_subset(session, expected_gtid_set, binary_log_status(session).executed_gtid_set))

wipeout_server(session)

#@<> dissolve cluster set
setup_session(__sandbox_uri2)
cs = dba.get_cluster_set()
cs.dissolve({'force': True})
reset_instance(session)
wipeout_server(session)

setup_session(__sandbox_uri3)
reset_instance(session)
wipeout_server(session)

wipe_dir(dump_instance_dir)
wipe_dir(dump_binlogs_dir)
wipe_dir(working_binlogs_dir)

#@<> Cleanup
testutil.destroy_sandbox(__mysql_sandbox_port1)
testutil.destroy_sandbox(__mysql_sandbox_port2)
testutil.destroy_sandbox(__mysql_sandbox_port3)
