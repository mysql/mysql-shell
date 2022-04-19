# Python integration test for the AdminAPI:
#
# General test to cover the AdminAPI commands in "happy path" scenarios
#
# TODOS:
#
# - Use raw_sandboxes
# - Extend testutils.start_sandbox() to allow passing the myCnf path

# dba.check_instance_configuration():
#
# 1. Test dba.check_instance_configuration() for an instance not ready for cluster usage
# 2. Test dba.check_instance_configuration() for an instance ready for cluster usage

# This test will only be executed when required, i.e. when recording or when there's
# no JavaScript, reason is that it only tests the AdminAPI, when there's JavaScript
# it is thoroughly tested by JS tests

#@ {not (__have_javascript and __test_execution_mode == "replay")}

#@ Initialization
testutil.deploy_sandbox(__mysql_sandbox_port1, 'root', {'report_host': hostname, 'binlog_checksum': 'CRC32', 'gtid_mode': 'OFF', 'server_id': '0'})
testutil.snapshot_sandbox_conf(__mysql_sandbox_port1)
testutil.deploy_sandbox(__mysql_sandbox_port2, 'root', {'report_host': hostname})
testutil.snapshot_sandbox_conf(__mysql_sandbox_port2)
testutil.deploy_sandbox(__mysql_sandbox_port3, 'root', {'report_host': hostname})
testutil.snapshot_sandbox_conf(__mysql_sandbox_port3)

__sandbox1_cnf_path = testutil.get_sandbox_conf_path(__mysql_sandbox_port1)
__sandbox2_cnf_path = testutil.get_sandbox_conf_path(__mysql_sandbox_port2)
__sandbox3_cnf_path = testutil.get_sandbox_conf_path(__mysql_sandbox_port3)

#@<OUT> check_instance_configuration() - instance not valid for cluster usage
dba.check_instance_configuration(__sandbox_uri1, {'mycnfPath': __sandbox1_cnf_path})

#@<OUT> check_instance_configuration() - instance valid for cluster usage 2
dba.check_instance_configuration(__sandbox_uri2, {'mycnfPath': __sandbox2_cnf_path})

#@<OUT> check_instance_configuration() - instance valid for cluster usage 3
dba.check_instance_configuration(__sandbox_uri3, {'mycnfPath': __sandbox3_cnf_path})

# dba.configure_instance():
#
# 1. Test the configuration of an instance not ready for cluster usage
# 2. Test the configuration of an instance ready for cluster usage

#@<OUT> configure_instance() - instance not valid for cluster usage {VER(>=8.0.11)}
testutil.expect_prompt("Do you want to perform the required configuration changes?", "y")
testutil.expect_prompt("Do you want to restart the instance after configuring it?", "n")
dba.configure_instance(__sandbox_uri1, {'clusterAdmin': 'myAdmin', 'clusterAdminPassword': 'myPwd', 'mycnfPath': __sandbox1_cnf_path})

#@<OUT> configure_instance() - instance not valid for cluster usage {VER(<8.0.11)}
testutil.expect_prompt("Do you want to perform the required configuration changes?", "y")
dba.configure_instance(__sandbox_uri1, {'clusterAdmin': 'myAdmin', 'clusterAdminPassword': 'myPwd', 'mycnfPath': __sandbox1_cnf_path})

#@<OUT> configure_instance() - create admin account 2
dba.configure_instance(__sandbox_uri2, {'clusterAdmin': 'myAdmin', 'clusterAdminPassword': 'myPwd', 'mycnfPath': __sandbox2_cnf_path})

#@<OUT> configure_instance() - create admin account 3
dba.configure_instance(__sandbox_uri3, {'clusterAdmin': 'myAdmin', 'clusterAdminPassword': 'myPwd', 'mycnfPath': __sandbox3_cnf_path})

#@<OUT> configure_instance() - check if configure_instance() was actually successful by double-checking with check_instance_configuration()
# Restart the instance first since there were settings changed that required a restart
testutil.restart_sandbox(__mysql_sandbox_port1)
# Call the check function
dba.check_instance_configuration(__sandbox_uri1)

#@<OUT> configure_instance() - instance already valid for cluster usage
# Restart the instance first since there were settings changed that required a restart
testutil.restart_sandbox(__mysql_sandbox_port2)
dba.configure_instance(__sandbox_uri2)

#@ configure_instance() 2 - instance already valid for cluster usage
# Restart the instance first since there were settings changed that required a restart
testutil.restart_sandbox(__mysql_sandbox_port3)
dba.configure_instance(__sandbox_uri3)

# dba.createCluster():
#
# 1. Create a cluster on __sandbox_uri1, using some of the available options

# Create a test dataset so that RECOVERY takes a while and we ensure right monitoring messages for addInstance
shell.connect(__sandbox_uri1)
session.run_sql("create schema test");
session.run_sql("create table test.data (a int primary key auto_increment, data longtext)");
for x in range(0, 20):
  session.run_sql("insert into test.data values (default, repeat('x', 4*1024*1024))");

session.close()

# Establish a session to the instance on which the cluster will be created
shell.connect({'host': hostname, 'port': __mysql_sandbox_port1, 'user': 'myAdmin', 'password': 'myPwd'})

#@<OUT> create_cluster()
if __version_num < 80027:
  c = dba.create_cluster("testCluster", {'groupName': 'ca94447b-e6fc-11e7-b69d-4485005154dc', 'ipWhitelist': '255.255.255.255/32,127.0.0.1,' + hostname_ip, 'memberSslMode': 'DISABLED', 'exitStateAction': 'READ_ONLY', 'gtidSetIsComplete': True})
else:
  c = dba.create_cluster("testCluster", {'groupName': 'ca94447b-e6fc-11e7-b69d-4485005154dc', 'memberSslMode': 'DISABLED', 'exitStateAction': 'READ_ONLY', 'gtidSetIsComplete': True})

# Cluster.addInstance():
#
# 1. Add 2 instances to the created cluster and check both .status() and .describe()

#@<OUT> add_instance() 1 clone {VER(>=8.0.17)}
c.add_instance({'host': hostname, 'port': __mysql_sandbox_port2, 'user': 'myAdmin', 'password': 'myPwd'}, {'exitStateAction': 'READ_ONLY'})

#@<OUT> add_instance() 1 {VER(<8.0.11)}
c.add_instance({'host': hostname, 'port': __mysql_sandbox_port2, 'user': 'myAdmin', 'password': 'myPwd'}, {'exitStateAction': 'READ_ONLY'})

#@ add_instance() 2
c.add_instance({'host': hostname, 'port': __mysql_sandbox_port3, 'user': 'myAdmin', 'password': 'myPwd'}, {'exitStateAction': 'READ_ONLY'})

# Cluster.describe():
#
#@<OUT> Verify the output of cluster.describe()
c.describe()

# Cluster.status():
#
#@<OUT> Verify the output of cluster.status()
c.status()

# Cluster.remove_instance():
#
# 1. Test the removal of an instance
# 2. Verify the cluster.status() to check if the instance is not there anymore

#@ Remove instance
c.remove_instance({'host': hostname, 'port': __mysql_sandbox_port3})

# get_cluster():
#
# 1. Use get_cluster() to obtain a handle to the cluster, without a session established to the primary instance
# 2. Verify the cluster.status() to check if the instance previously removed is actually not there anymore

# Disconnect the cluster session
c.disconnect()
# Close the active shell session
session.close()
#Establish a session to the non primary member
shell.connect({'host': hostname, 'port': __mysql_sandbox_port2, 'user': 'myAdmin', 'password': 'myPwd'})

#@ get_cluster():
c = dba.get_cluster()

#@<OUT> Verify the output of cluster.status() after the instance removal
c.status()

#@ Add the instance back to the cluster
c.add_instance({'host': hostname, 'port': __mysql_sandbox_port3, 'user': 'myAdmin', 'password': 'myPwd'}, {'exitStateAction': 'READ_ONLY'})
testutil.wait_member_state(__mysql_sandbox_port3, "ONLINE")

# Cluster.rejoin_instance()
#
# 1. Test rejoin instance on 5.7, whenever the settings weren't persisted

#@ Kill instance 3 {VER(<8.0.11)}
testutil.kill_sandbox(__mysql_sandbox_port3, True)
testutil.wait_member_state(__mysql_sandbox_port3, "(MISSING)")

#@ Restart instance 3 {VER(<8.0.11)}
testutil.start_sandbox(__mysql_sandbox_port3)

#@ Rejoin instance 3 {VER(<8.0.11)}
c.rejoin_instance({'host': hostname, 'port': __mysql_sandbox_port3, 'user': 'myAdmin', 'password': 'myPwd'})
testutil.wait_member_state(__mysql_sandbox_port3, "ONLINE")

#@<OUT> Verify the output of cluster.status() after the instance rejoined {VER(<8.0.11)}
c.status()

# Persist settings in 5.7:
#
# 1. If we're running the tests with MySQL 5.7, we must use
# dba.configure_local_instance() to persist the GR settings and achieve automatic rejoin

#@ dba_configure_local_instance() 1: {VER(<8.0.11)}
dba.configure_local_instance({'host': hostname, 'port': __mysql_sandbox_port1, 'user': 'myAdmin', 'password': 'myPwd'}, {'mycnfPath': __sandbox1_cnf_path})

#@ dba_configure_local_instance() 2: {VER(<8.0.11)}
dba.configure_local_instance({'host': hostname, 'port': __mysql_sandbox_port2, 'user': 'myAdmin', 'password': 'myPwd'}, {'mycnfPath': __sandbox2_cnf_path})

#@ dba_configure_local_instance() 3: {VER(<8.0.11)}
dba.configure_local_instance({'host': hostname, 'port': __mysql_sandbox_port3, 'user': 'myAdmin', 'password': 'myPwd'}, {'mycnfPath': __sandbox3_cnf_path})

# Test automatic rejoin
#
# 1. Shutdown one of the cluster members and restart it
# 2. Verify it automatically rejoins the cluster

# Disconnect the cluster session
c.disconnect()
# Close the active shell session
session.close()

#@ Kill instance 2
testutil.kill_sandbox(__mysql_sandbox_port2, True)

#Establish a session to the primary member
shell.connect({'host': hostname, 'port': __mysql_sandbox_port1, 'user': 'myAdmin', 'password': 'myPwd'})

testutil.wait_member_state(__mysql_sandbox_port2, "(MISSING)")

#@ Restart instance 2
testutil.start_sandbox(__mysql_sandbox_port2)
testutil.wait_member_state(__mysql_sandbox_port2, "ONLINE")

# Get back the cluster handle
c = dba.get_cluster()

#@<OUT> Verify the output of cluster.status() after the instance automatically rejoined
c.status()

# Restore from quorum-loss
#
# Simulate a quorum-loss scenario and restore it using Cluster.force_quorum_using_partition_of()
#
# 1. Kill 2 members of the cluster to simulate the quorum-loss
# 2. Restore the quorum using Cluster.force_quorum_using_partition_of()
# 3. Let the instances rejoin the cluster automatically
# 4. Verify the final cluster status

#@ Kill instance 2 - quorum-loss
testutil.kill_sandbox(__mysql_sandbox_port2, True)
testutil.wait_member_state(__mysql_sandbox_port2, "(MISSING)")

#@ Kill instance 3 - quorum-loss
testutil.kill_sandbox(__mysql_sandbox_port3, True)
testutil.wait_member_state(__mysql_sandbox_port3, "UNREACHABLE")

#@<OUT> Verify the cluster lost the quorum
c.status()

#@<OUT> Restore the cluster quorum
c.force_quorum_using_partition_of({'host': hostname, 'port': __mysql_sandbox_port1, 'user': 'myAdmin', 'password': 'myPwd'})

#@<OUT> Check the cluster status after restoring the quorum
c.status()

#@ Restart instance 2 - quorum-loss
testutil.start_sandbox(__mysql_sandbox_port2)
testutil.wait_for_delayed_g_r_start(__mysql_sandbox_port2, 'root')
testutil.wait_member_state(__mysql_sandbox_port2, "ONLINE")

#@ Restart instance 3 - quorum-loss
testutil.start_sandbox(__mysql_sandbox_port3)
testutil.wait_for_delayed_g_r_start(__mysql_sandbox_port3, 'root')
testutil.wait_member_state(__mysql_sandbox_port3, "ONLINE")

#@<OUT> Verify the final cluster status
c.status()

# Reboot cluster from complete outage
#
# Simulate a complete outage scenario and restore the cluster using Cluster.reboot_cluster_from_complete_outage()
#
# 1. Kill all members of the cluster to simulate the complete outage
# 2. Restore the quorum using Cluster.reboot_cluster_from_complete_outage()
# 3. Rejoin the instances back to the cluster by accepting the interactive prompts to rejoin
# 4. Verify the final cluster status

#@<> Disable auto-rejoin in all members {VER(>=8.0.11)}
shell.connect(__sandbox_uri3)
session.run_sql("SET PERSIST group_replication_start_on_boot=OFF")
session.close()

shell.connect(__sandbox_uri2)
session.run_sql("SET PERSIST group_replication_start_on_boot=OFF")
session.close()

shell.connect(__sandbox_uri1)
session.run_sql("SET PERSIST group_replication_start_on_boot=OFF")

#@<> Disable auto-rejoin config in all members {VER(<8.0.11)}
testutil.change_sandbox_conf(__mysql_sandbox_port1, 'group_replication_start_on_boot', 'OFF')
testutil.change_sandbox_conf(__mysql_sandbox_port2, 'group_replication_start_on_boot', 'OFF')
testutil.change_sandbox_conf(__mysql_sandbox_port3, 'group_replication_start_on_boot', 'OFF')

#@ Kill instance 2 - complete-outage
testutil.kill_sandbox(__mysql_sandbox_port2, True)
testutil.wait_member_state(__mysql_sandbox_port2, "(MISSING)")

#@ Kill instance 3 - complete-outage
testutil.kill_sandbox(__mysql_sandbox_port3, True)
testutil.wait_member_state(__mysql_sandbox_port3, "UNREACHABLE")

#@ Kill instance 1 - complete-outage
testutil.kill_sandbox(__mysql_sandbox_port1, True)

#@ Restart instance 2 - complete-outage
testutil.start_sandbox(__mysql_sandbox_port2)

#@ Restart instance 3 - complete-outage
testutil.start_sandbox(__mysql_sandbox_port3)

#@ Restart instance 1 - complete-outage
testutil.start_sandbox(__mysql_sandbox_port1)

# Re-establish the session
shell.connect({'host': hostname, 'port': __mysql_sandbox_port1, 'user': 'myAdmin', 'password': 'myPwd'})

#@ Verify that it's not possible to get the cluster handle due to complete outage
c = dba.get_cluster()

#@ Reboot cluster from complete outage
testutil.expect_prompt("Would you like to rejoin it to the cluster?", "y")
testutil.expect_prompt("Would you like to rejoin it to the cluster?", "y")
c = dba.reboot_cluster_from_complete_outage()
testutil.wait_member_state(__mysql_sandbox_port2, "ONLINE")
testutil.wait_member_state(__mysql_sandbox_port3, "ONLINE")

#@<OUT> Verify the final cluster status after reboot
c.status()

# Rescan cluster
#
# Simulate an instance that belongs to the GR group but is not in the metadata
# and an instance that is on the metadata but not in the GR group.
# Then use Cluster.rescan() to act accordingly
#
# 1. Remove instance 3 from the MD schema
# 2. Stop GR on instance 2
# 3. Use Cluster.rescan() to add instance 2 and 3 to the cluster
# 4. Verify the final cluster status

# Connect to instance 1
session.close()
shell.connect({'host': hostname, 'port': __mysql_sandbox_port1, 'user': 'myAdmin', 'password': 'myPwd'})

#@ Remove instance 3 from the MD schema
session.run_sql("delete from mysql_innodb_cluster_metadata.instances where instance_name=?", [hostname + ":" + str(__mysql_sandbox_port2)])

# Connect to instance 3
session.close()
shell.connect({'host': hostname, 'port': __mysql_sandbox_port3, 'user': 'myAdmin', 'password': 'myPwd'})

#@ Stop GR on instance 2
session.run_sql("STOP group_replication")

# Connect to instance 1
session.close()
shell.connect({'host': hostname, 'port': __mysql_sandbox_port1, 'user': 'myAdmin', 'password': 'myPwd'})

#@ Get the cluster handle back
c = dba.get_cluster()

#@ Rescan the cluster
testutil.expect_prompt("Would you like to add it to the cluster metadata?", "y")
testutil.expect_password("Please provide the password for", "myPwd")
testutil.expect_prompt("Would you like to remove it from the cluster metadata?", "y")
c.rescan()

#@<OUT> Verify the final cluster status after rescan
c.status()

# Dissolve cluster
#
# 1. Completely dissolve the cluster using Cluster.dissolve({force: true})

#@<OUT> Dissolve cluster
testutil.expect_prompt("Are you sure you want to dissolve the cluster?", "y")
c.dissolve({'force': True})

#@ Finalization
session.close()
testutil.destroy_sandbox(__mysql_sandbox_port1)
testutil.destroy_sandbox(__mysql_sandbox_port2)
testutil.destroy_sandbox(__mysql_sandbox_port3)
