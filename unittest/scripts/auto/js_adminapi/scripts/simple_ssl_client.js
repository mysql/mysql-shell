// Plain positive test case for SSL client certificates

// Includes test for

//@<> Setup

testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.deploySandbox(__mysql_sandbox_port3, "root");

// Create admin accounts that require a client cert to connect
session1 = mysql.getSession(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);
session3 = mysql.getSession(__sandbox_uri3);

session1.runSql("set sql_log_bin=0");
session1.runSql("create user admin@'%' require x509");
session1.runSql("grant all on *.* to admin@'%' with grant option");
session2.runSql("set sql_log_bin=0");
session2.runSql("create user admin@'%' require x509");
session2.runSql("grant all on *.* to admin@'%' with grant option");
session3.runSql("set sql_log_bin=0");
session3.runSql("create user admin@'%' require x509");
session3.runSql("grant all on *.* to admin@'%' with grant option");

// Create a common CA
testutil.sslCreateCa("myca");

// Create client certificates
testutil.sslCreateCerts(__mysql_sandbox_port1, "myca", "MySandboxServer1", "MySandboxServer1Client");
testutil.sslCreateCerts(__mysql_sandbox_port2, "myca", "MySandboxServer2", "MySandboxServer2Client");
testutil.sslCreateCerts(__mysql_sandbox_port3, "myca", "MySandboxServer3", "MySandboxServer3Client");

testutil.restartSandbox(__mysql_sandbox_port1);
testutil.restartSandbox(__mysql_sandbox_port2);
testutil.restartSandbox(__mysql_sandbox_port3);


function sandbox_ssl_opts(sbport) {
    var s = mysql.getSession("root:root@localhost:"+sbport);
    sbdir = s.runSql("SELECT @@datadir").fetchOne()[0];
    s.close();
    return "ssl-ca=("+sbdir+"/ca.pem)&ssl-cert=("+sbdir+"/client-cert.pem)&ssl-key=("+sbdir+"/client-key.pem)";
}

ssl_sandbox_uri1 = "admin@localhost:"+__mysql_sandbox_port1+"?"+sandbox_ssl_opts(__mysql_sandbox_port1);
ssl_sandbox_uri2 = "admin@localhost:"+__mysql_sandbox_port2+"?"+sandbox_ssl_opts(__mysql_sandbox_port2);
ssl_sandbox_uri3 = "admin@localhost:"+__mysql_sandbox_port3+"?"+sandbox_ssl_opts(__mysql_sandbox_port3);

session1 = mysql.getSession(ssl_sandbox_uri1);
session2 = mysql.getSession(ssl_sandbox_uri2);
session3 = mysql.getSession(ssl_sandbox_uri3);

//@ createCluster
shell.connect(ssl_sandbox_uri1);
var cluster = dba.createCluster("clus", {gtidSetIsComplete:1});

//@ addInstance
cluster.addInstance(ssl_sandbox_uri2);
cluster.addInstance(ssl_sandbox_uri3);

//@ status
cluster.status();

//@ rejoinInstance
session1.runSql("stop group_replication");

shell.connect(ssl_sandbox_uri2);
cluster = dba.getCluster();

cluster.rejoinInstance(ssl_sandbox_uri1);

//@ describe
cluster.describe();

//@ listRouters
cluster.listRouters();

//@ removeInstance
cluster.removeInstance(ssl_sandbox_uri3);

//@ setPrimaryInstance {VER(>=8.0.0)}
cluster.setPrimaryInstance(ssl_sandbox_uri1);
cluster.setPrimaryInstance(ssl_sandbox_uri2);

//@ options
cluster.options();

//@ setOption {VER(>=8.0.0)}
cluster.setOption("expelTimeout", 5);

//@ setInstanceOption
cluster.setInstanceOption(ssl_sandbox_uri1, "memberWeight", 42);

cluster.options();

//@ forceQuorum
// TODO these 2 lines shouldn't be needed, but without them the addInstance
// fails with Cluster.addInstance: This function is not available through a session to a read only instance (RuntimeError)
// everything should be fixed to always work through the PRIMARY
shell.connect(ssl_sandbox_uri1);
cluster = dba.getCluster();
// <--

cluster.addInstance(ssl_sandbox_uri3);
cluster.status();
session1.runSql("stop group_replication");

testutil.killSandbox(__mysql_sandbox_port3);
wait_member_state_from(session2, __mysql_sandbox_port3, "UNREACHABLE");
testutil.startSandbox(__mysql_sandbox_port3);
session3 = mysql.getSession(ssl_sandbox_uri3);

shell.connect(ssl_sandbox_uri2);
cluster = dba.getCluster();

cluster.forceQuorumUsingPartitionOf(ssl_sandbox_uri2);
cluster.rejoinInstance(ssl_sandbox_uri1);

//@<> forceQuorum (rejoin restarted sandbox without persisted start_on_boot) {VER(<8.0.0)}
cluster.rejoinInstance(ssl_sandbox_uri3);

//@ rebootCluster
session3.runSql("stop group_replication");
session2.runSql("stop group_replication");
session1.runSql("stop group_replication");

shell.connect(ssl_sandbox_uri1);
cluster = dba.rebootClusterFromCompleteOutage("clus", {rejoinInstances: ["localhost:"+__mysql_sandbox_port2, "localhost:"+__mysql_sandbox_port3]});

cluster.status();

//@ rescan
// remove an instance from the MD
session.runSql("delete from mysql_innodb_cluster_metadata.instances where instance_name=?", ["127.0.0.1:"+__mysql_sandbox_port2]);

cluster.rescan();

//@ dissolve
cluster.dissolve();

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
