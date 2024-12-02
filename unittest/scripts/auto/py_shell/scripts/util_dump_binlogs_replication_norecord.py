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

def dissolve_topology(uri, method):
    setup_session(uri)
    topology = dba[method]()
    topology.dissolve({"force": True})
    reset_instance(session)
    wipeout_server(session)

def dissolve_replica_set(uri):
    dissolve_topology(uri, "get_replica_set")

def dissolve_cluster(uri):
    dissolve_topology(uri, "get_cluster")

def dissolve_cluster_set(uri):
    dissolve_topology(uri, "get_cluster_set")

def wipe_replica(uri):
    setup_session(uri)
    reset_instance(session)
    wipeout_server(session)

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
EXPECT_FAIL("ValueError", "The source instance is different from the one used in the previous dump and it's not part of an InnoDB Cluster group.", dump_binlogs_dir)

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

EXPECT_FAIL("ValueError", "The source instance is different from the one used in the previous dump and it's not part of an InnoDB Cluster group.", working_binlogs_dir, options={"since": dump_instance_dir})

wipe_dir(working_binlogs_dir)

#@<> WL15977-FR2.4.2 - wrong instance - `since` dump_instance() - replica set
setup_session(__sandbox_uri1)

EXPECT_FAIL("ValueError", "The source instance is different from the one used in the previous dump and it's not part of an InnoDB Cluster group.", working_binlogs_dir, options={"since": dump_instance_dir})

#@<> `since` dump_binlogs() - replica set
setup_session(__sandbox_uri3)

EXPECT_FAIL("ValueError", "The source instance is different from the one used in the previous dump and it's not part of an InnoDB Cluster group.", working_binlogs_dir, options={"since": dump_binlogs_dir})

wipe_dir(working_binlogs_dir)

#@<> WL15977-FR2.4.2 - wrong instance - `since` dump_binlogs() - replica set
setup_session(__sandbox_uri1)

EXPECT_FAIL("ValueError", "The source instance is different from the one used in the previous dump and it's not part of an InnoDB Cluster group.", working_binlogs_dir, options={"since": dump_binlogs_dir})

#@<> WL15977-FR2.4.2 - wrong instance - incremental dump - replica set
setup_session(__sandbox_uri1)

EXPECT_FAIL("ValueError", "The source instance is different from the one used in the previous dump and it's not part of an InnoDB Cluster group.", dump_binlogs_dir)

#@<> dissolve replica set
dissolve_replica_set(__sandbox_uri1)
dissolve_replica_set(__sandbox_uri2)
wipe_replica(__sandbox_uri3)

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

EXPECT_FAIL("ValueError", "The source instance is different from the one used in the previous dump and they do not belong to the same InnoDB Cluster group.", working_binlogs_dir, options={"since": dump_instance_dir})

#@<> `since` dump_binlogs() - cluster
setup_session(__sandbox_uri3)

EXPECT_SUCCESS(working_binlogs_dir, options={"since": dump_binlogs_dir})
EXPECT_SOURCE_HAS_CHANGED(__mysql_sandbox_port2, __mysql_sandbox_port3)

wipe_dir(working_binlogs_dir)

#@<> WL15977-FR2.4.2 - wrong instance - `since` dump_binlogs() - cluster
setup_session(__sandbox_uri1)

EXPECT_FAIL("ValueError", "The source instance is different from the one used in the previous dump and they do not belong to the same InnoDB Cluster group.", working_binlogs_dir, options={"since": dump_binlogs_dir})

#@<> WL15977-FR2.4.2 - wrong instance - incremental dump - cluster
setup_session(__sandbox_uri1)

EXPECT_FAIL("ValueError", "The source instance is different from the one used in the previous dump and they do not belong to the same InnoDB Cluster group.", dump_binlogs_dir)

#@<> incremental dump - cluster
setup_session(__sandbox_uri3)

EXPECT_SUCCESS(dump_binlogs_dir)
EXPECT_SOURCE_HAS_CHANGED(__mysql_sandbox_port2, __mysql_sandbox_port3)

#@<> incremental dump - nothing more to be dumped - cluster
setup_session(__sandbox_uri2)

EXPECT_SUCCESS(dump_binlogs_dir, nothing_dumped=True)

#@<> instance is not ONLINE, but it's the same as previous dump - cluster
setup_session(__sandbox_uri3)
session.run_sql("STOP GROUP_REPLICATION")

setup_session(__sandbox_uri2)
testutil.wait_member_state(__mysql_sandbox_port3, "(MISSING)")

setup_session(__sandbox_uri3)

EXPECT_SUCCESS(dump_binlogs_dir, nothing_dumped=True)

#@<> load the binlog dump - cluster
dissolve_cluster(__sandbox_uri1)

EXPECT_LOAD_SUCCESS(dump_binlogs_dir)
EXPECT_TRUE(gtid_subset(session, expected_gtid_set, binary_log_status(session).executed_gtid_set))

wipeout_server(session)

#@<> instance is not ONLINE and it's different from previous dump - cluster
c.add_instance(__sandbox_uri1, {"recoveryMethod": "clone"})

setup_session(__sandbox_uri1)
session.run_sql("STOP GROUP_REPLICATION")

setup_session(__sandbox_uri2)
testutil.wait_member_state(__mysql_sandbox_port1, "(MISSING)")

setup_session(__sandbox_uri1)

EXPECT_FAIL("ValueError", "The source instance is different from the one used in the previous dump, it's part of the same InnoDB Cluster group, but is not currently ONLINE.", dump_binlogs_dir)

wipe_replica(__sandbox_uri1)
c.remove_instance(__sandbox_uri1, {"force": True})

#@<> BUG#37334759 - former member - cluster
c.add_instance(__sandbox_uri1, {"recoveryMethod": "clone"})
c.remove_instance(__sandbox_uri1)

setup_session(__sandbox_uri1)

EXPECT_FAIL("ValueError", "The source instance is different from the one used in the previous dump, it's part of the same InnoDB Cluster group, but is not currently ONLINE.", dump_binlogs_dir)

wipe_replica(__sandbox_uri1)
#@<> BUG#37335360 - read replica + cluster
c.add_replica_instance(__sandbox_uri1, {"recoveryMethod": "incremental"})
setup_session(__sandbox_uri1)

EXPECT_FAIL("ValueError", "The source instance is different from the one used in the previous dump and is not an ONLINE member of the same InnoDB Cluster replication group.", dump_binlogs_dir)

c.remove_instance(__sandbox_uri1)
wipe_replica(__sandbox_uri1)

#@<> cluster without metadata + previous instance which is no longer part of the group
c.add_instance(__sandbox_uri1, {"recoveryMethod": "incremental"})

setup_session(__sandbox_uri1)
dba.drop_metadata_schema({"force": True})

EXPECT_SUCCESS(dump_binlogs_dir)

wipe_replica(__sandbox_uri1)

#@<> dissolve cluster
wipe_replica(__sandbox_uri2)
wipe_replica(__sandbox_uri3)

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

EXPECT_FAIL("ValueError", "The source instance is different from the one used in the previous dump and they do not belong to the same InnoDB Cluster group.", working_binlogs_dir, options={"since": dump_instance_dir})

wipe_dir(working_binlogs_dir)

#@<> WL15977-FR2.4.2 - wrong instance - `since` dump_instance() - cluster set
setup_session(__sandbox_uri1)

EXPECT_FAIL("ValueError", "The source instance is different from the one used in the previous dump and they do not belong to the same InnoDB Cluster group.", working_binlogs_dir, options={"since": dump_instance_dir})

#@<> `since` dump_binlogs() - cluster set
setup_session(__sandbox_uri3)

EXPECT_FAIL("ValueError", "The source instance is different from the one used in the previous dump and they do not belong to the same InnoDB Cluster group.", working_binlogs_dir, options={"since": dump_binlogs_dir})

wipe_dir(working_binlogs_dir)

#@<> WL15977-FR2.4.2 - wrong instance - `since` dump_binlogs() - cluster set
setup_session(__sandbox_uri1)

EXPECT_FAIL("ValueError", "The source instance is different from the one used in the previous dump and they do not belong to the same InnoDB Cluster group.", working_binlogs_dir, options={"since": dump_binlogs_dir})

#@<> WL15977-FR2.4.2 - wrong instance - incremental dump - cluster set
setup_session(__sandbox_uri1)

EXPECT_FAIL("ValueError", "The source instance is different from the one used in the previous dump and they do not belong to the same InnoDB Cluster group.", dump_binlogs_dir)

#@<> dissolve cluster set
dissolve_cluster_set(__sandbox_uri1)
dissolve_cluster_set(__sandbox_uri2)
wipe_replica(__sandbox_uri3)

wipe_dir(dump_instance_dir)
wipe_dir(dump_binlogs_dir)
wipe_dir(working_binlogs_dir)

#@<> Cleanup
testutil.destroy_sandbox(__mysql_sandbox_port1)
testutil.destroy_sandbox(__mysql_sandbox_port2)
testutil.destroy_sandbox(__mysql_sandbox_port3)
