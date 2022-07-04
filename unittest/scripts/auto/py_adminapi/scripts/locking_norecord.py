#@<> utils
import mysqlsh
import threading

#@<> Setup
allowlist = "127.0.0.1," + hostname_ip;

testutil.deploy_sandbox(__mysql_sandbox_port1, 'root', {'report_host': hostname})
testutil.deploy_sandbox(__mysql_sandbox_port2, 'root', {'report_host': hostname})
testutil.snapshot_sandbox_conf(__mysql_sandbox_port1)
testutil.snapshot_sandbox_conf(__mysql_sandbox_port2)

session1 = mysql.get_session(__sandbox_uri1)
session2 = mysql.get_session(__sandbox_uri2)

shell.connect(__sandbox_uri1)

if testutil.version_check(__version, '>=', '8.0.27'):
    cluster = dba.create_cluster('cluster', {"ipAllowlist": allowlist, "gtidSetIsComplete": True, "communicationStack": "XCOM"})
else:
    cluster = dba.create_cluster('cluster', {"ipAllowlist": allowlist, "gtidSetIsComplete": True})

session1.run_sql("create schema test")
session1.run_sql("create table test.data (a int primary key auto_increment, data longtext)")

cluster.add_instance(__sandbox_uri2, {"recoveryMethod": 'incremental', "ipAllowlist": allowlist})
testutil.wait_member_transactions(__mysql_sandbox_port2, __mysql_sandbox_port1);
cluster.remove_instance(__sandbox_uri2)

for _ in range(10):
    session1.run_sql("insert into test.data values (default, repeat('x', 4*1024*1024))")

#@<> exclusive cluster lock

def add_instance(cluster, instance):
    cluster.add_instance(instance, {"recoveryMethod": 'incremental', "ipAllowlist": allowlist})

# this will make the thread block
session2.run_sql("FLUSH TABLES test.data WITH READ LOCK");

cluster_thread = dba.get_cluster();
thread = threading.Thread(target=add_instance, args=(cluster_thread, __sandbox_uri2))
thread.start()

time.sleep(3) # give time to the thead to start and block

EXPECT_THROWS(lambda: cluster.add_instance(__sandbox_uri2, {"recoveryMethod": 'incremental', "ipAllowlist": allowlist}),
    f'Failed to acquire Cluster lock through primary member \'{hostname}:{__mysql_sandbox_port1}\'')
EXPECT_STDOUT_CONTAINS(f'The operation cannot be executed because it failed to acquire the Cluster lock through primary member \'{hostname}:{__mysql_sandbox_port1}\'. Another operation requiring access to the member is still in progress, please wait for it to finish and try again.')

EXPECT_THROWS(lambda: cluster.rejoin_instance(__sandbox_uri2, {"ipAllowlist": allowlist}),
    f'Failed to acquire Cluster lock through primary member \'{hostname}:{__mysql_sandbox_port1}\'')
EXPECT_STDOUT_CONTAINS(f'The operation cannot be executed because it failed to acquire the Cluster lock through primary member \'{hostname}:{__mysql_sandbox_port1}\'. Another operation requiring access to the member is still in progress, please wait for it to finish and try again.')

session2.run_sql("UNLOCK TABLES");
thread.join()

#@<> Cleanup
cluster_thread.disconnect()
cluster.disconnect()
session.close()
session1.close()
session2.close()

testutil.destroy_sandbox(__mysql_sandbox_port1)
testutil.destroy_sandbox(__mysql_sandbox_port2)
