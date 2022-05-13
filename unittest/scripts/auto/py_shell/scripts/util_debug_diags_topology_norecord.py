import zipfile
import yaml
import os
import re
import shutil

#@<> INCLUDE diags_common.inc

#@<> Init
testutil.deploy_sandbox(__mysql_sandbox_port1, "root")
testutil.deploy_sandbox(__mysql_sandbox_port2, "root")
testutil.deploy_sandbox(__mysql_sandbox_port3, "root")
testutil.deploy_sandbox(__mysql_sandbox_port4, "root")

session1 = mysql.get_session(__sandbox_uri1)
session2 = mysql.get_session(__sandbox_uri2)
session3 = mysql.get_session(__sandbox_uri3)
session4 = mysql.get_session(__sandbox_uri4)

session1.run_sql("create schema test")

#@<> ReplicaSet {VER(>8.0.0)}
shell.connect(__sandbox_uri1)

r = dba.create_replica_set("replicaset", {"gtidSetIsComplete":1})
r.add_instance(__sandbox_uri2)

outpath = run_collect(__sandbox_uri1, None, allMembers=1)
CHECK_DIAGPACK(outpath, [(1, session1), (2, session2)], is_cluster=True, innodbMutex=False, localTarget=True)

outpath = run_collect_hl(__sandbox_uri1, None)
CHECK_DIAGPACK(outpath, [(None, session1)], localTarget=True)

outpath = run_collect_sq(__sandbox_uri1, None)
CHECK_DIAGPACK(outpath, [(None, session1)], localTarget=True)

session.run_sql("drop schema mysql_innodb_cluster_metadata")

session2.close()
testutil.destroy_sandbox(__mysql_sandbox_port2)
testutil.deploy_sandbox(__mysql_sandbox_port2, "root")
session2 = mysql.get_session(__sandbox_uri2)

r.disconnect()

session1.run_sql("reset master")

#@<> Setup InnoDB Cluster
shell.connect(__sandbox_uri1)
c = dba.create_cluster("cluster", {"gtidSetIsComplete":1})
c.add_instance(__sandbox_uri2)
c.add_instance(__sandbox_uri3)

testutil.wait_member_transactions(__mysql_sandbox_port3, __mysql_sandbox_port1)

#@<> highload and slowQuery 
outpath = run_collect_hl(__sandbox_uri1, None)
CHECK_DIAGPACK(outpath, [(None, session1)], localTarget=True)

outpath = run_collect_sq(__sandbox_uri1, None)
CHECK_DIAGPACK(outpath, [(None, session1)], localTarget=True)

#@<> cluster with innodbMutex + schemaStats
testutil.expect_password("Password for root: ", "root")
outpath = run_collect(hostname_uri, None, innodbMutex=1, schemaStats=1, allMembers=1)
CHECK_DIAGPACK(outpath, [(1, session1), (2, session2), (3, session3)], is_cluster=True, innodbMutex=True, schemaStats=True)

#@<> cluster with allMembers=False
outpath = run_collect(hostname_uri, None, schemaStats=1, allMembers=0)
EXPECT_STDOUT_NOT_CONTAINS("Password for root:")
CHECK_DIAGPACK(outpath, [(1, session1), (2, session2), (3, session3)], is_cluster=True, schemaStats=True, allMembers=False)

#@<> with bad password
outpath = run_collect(hostname_uri, None, password="bla", schemaStats=1, allMembers=1)
EXPECT_STDOUT_CONTAINS("Access denied for user 'root'@'localhost'")
EXPECT_NO_FILE(outpath)

#@<> minimal privs with innodb cluster
run_collect("minimal:@localhost:"+str(__mysql_sandbox_port1), None, allMembers=1, password="")
EXPECT_STDOUT_CONTAINS("Access denied")

#@<> Shutdown instance
session2.close()
testutil.stop_sandbox(__mysql_sandbox_port2, {"wait":1})

outpath = run_collect(hostname_uri, None, allMembers=1)
CHECK_DIAGPACK(outpath, [(1, session1), (2, "MySQL Error (2003): mysql.get_session: Can't connect to MySQL server on"), (3, session3)], is_cluster=True, innodbMutex=False)

#@<> Take offline
session3.run_sql("stop group_replication")

outpath = run_collect(hostname_uri, None, allMembers=1)
CHECK_DIAGPACK(outpath, [(1, session1), (2, "MySQL Error (2003): mysql.get_session: Can't connect to MySQL server on"), (3, session3)], is_cluster=True, innodbMutex=False)

session1.run_sql("stop group_replication")

outpath = run_collect(hostname_uri, None, allMembers=1)
CHECK_DIAGPACK(outpath, [(1, session1), (2, "MySQL Error (2003): mysql.get_session: Can't connect to MySQL server on"), (3, session3)], is_cluster=True, innodbMutex=False)

#@<> Expand to ClusterSet {VER(>8.0.0)}
testutil.start_sandbox(__mysql_sandbox_port2)
session2 = mysql.get_session(__sandbox_uri2)
c = dba.reboot_cluster_from_complete_outage()
c.rejoin_instance(__sandbox_uri2)
c.rejoin_instance(__sandbox_uri3)

cs = c.create_cluster_set("cset")
c2 = cs.create_replica_cluster(__sandbox_uri4, "cluster2")

testutil.wait_member_transactions(__mysql_sandbox_port4, __mysql_sandbox_port1)
testutil.wait_member_transactions(__mysql_sandbox_port3, __mysql_sandbox_port1)
testutil.wait_member_transactions(__mysql_sandbox_port2, __mysql_sandbox_port1)

outpath = run_collect(hostname_uri, None, allMembers=1)
CHECK_DIAGPACK(outpath, [(1, session1), (2, session2), (3, session3), (4, session4)], is_cluster=True, innodbMutex=False)

outpath = run_collect_hl(__sandbox_uri1, None)
CHECK_DIAGPACK(outpath, [(None, session1)], localTarget=True)

outpath = run_collect_sq(__sandbox_uri1, None)
CHECK_DIAGPACK(outpath, [(None, session1)], localTarget=True)

c.disconnect()
cs.disconnect()
c2.disconnect()

#@<> Cleanup
session1.close()
session2.close()
session3.close()
session4.close()

testutil.destroy_sandbox(__mysql_sandbox_port1)
testutil.destroy_sandbox(__mysql_sandbox_port2)
testutil.destroy_sandbox(__mysql_sandbox_port3)
testutil.destroy_sandbox(__mysql_sandbox_port4)

shutil.rmtree(outdir)
