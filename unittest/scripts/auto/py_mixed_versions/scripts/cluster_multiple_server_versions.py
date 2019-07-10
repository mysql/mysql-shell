#@ {DEF(MYSQLD57_PATH)}

#@<> Deploy sandbox using base server
def deploy_base_sb():
    testutil.deploy_sandbox(__mysql_sandbox_port1, "root", { "report_host": hostname })
    testutil.snapshot_sandbox_conf(__mysql_sandbox_port1)

EXPECT_NO_THROWS(deploy_base_sb, "Deploying 8.0 Sandbox")

#@<> Deploy sandbox using 5.7 server
def deploy_57_sb():
    testutil.deploy_sandbox(__mysql_sandbox_port2, "root", { "report_host": hostname }, {"mysqldPath": MYSQLD57_PATH})
    testutil.snapshot_sandbox_conf(__mysql_sandbox_port2)

EXPECT_NO_THROWS(deploy_57_sb, "Deploying 5.7 Sandbox")

#@<> Create the cluster on the 5.7 instance
shell.connect(__sandbox_uri2)
EXPECT_NO_THROWS(lambda: dba.create_cluster("testCluster"), "Creating the 5.7 Cluster")
cluster = dba.get_cluster()

#@<> Add an 8.0 instance to the cluster
EXPECT_NO_THROWS(lambda:cluster.add_instance(__sandbox_uri1, {"recoveryMethod":'incremental'}), "Adding an 8.0 Instance")

#@ get cluster status
cluster.describe();

#@<> Get version for the 5.7 Server
session.run_sql("select @@version");
EXPECT_STDOUT_CONTAINS("5.7");


#@<> Get Version for the 8.0 Server
shell.connect(__sandbox_uri1);
session.run_sql("select @@version");
EXPECT_STDOUT_CONTAINS("8.0");

#@<> Finalize
def finalize():
    session.close()
    cluster.disconnect()
    testutil.destroy_sandbox(__mysql_sandbox_port1);
    testutil.destroy_sandbox(__mysql_sandbox_port2);

EXPECT_NO_THROWS(finalize, "Cleaning Up Test Case")
