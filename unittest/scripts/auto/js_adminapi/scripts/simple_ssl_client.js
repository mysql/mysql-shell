// Plain positive test case for SSL client certificates

// Includes test for

//@<> Setup

testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.deploySandbox(__mysql_sandbox_port3, "root");

testutil.snapshotSandboxConf(__mysql_sandbox_port3);

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
ca1_path = testutil.sslCreateCa("myca", "/CN=Test_CA");

// Create client certificates
user1_cert = testutil.sslCreateCert("client-user1", "myca", "/CN=user1@%"); // client cert specific for user1@%
user2_cert = testutil.sslCreateCert("client-user2", "myca", "/CN=user2@%"); // client cert specific for user2@%
router_cert = testutil.sslCreateCert("client-router", "myca", "/CN=router@%"); // client cert specific for router@%

client1_cert = testutil.sslCreateCert("client1", "myca", "/CN=MySandboxServer1Client");
client2_cert = testutil.sslCreateCert("client2", "myca", "/CN=MySandboxServer2Client");
client3_cert = testutil.sslCreateCert("client3", "myca", "/CN=MySandboxServer3Client");

testutil.sslCreateCert("server", "myca", "/CN=MySandboxServer1", __mysql_sandbox_port1);
testutil.sslCreateCert("server", "myca", "/CN=MySandboxServer2", __mysql_sandbox_port2);
testutil.sslCreateCert("server", "myca", "/CN=MySandboxServer3", __mysql_sandbox_port3);

testutil.changeSandboxConf(__mysql_sandbox_port1, "require_secure_transport", "1");
testutil.changeSandboxConf(__mysql_sandbox_port2, "require_secure_transport", "1");

testutil.changeSandboxConf(__mysql_sandbox_port1, "ssl_ca", ca1_path);
testutil.changeSandboxConf(__mysql_sandbox_port1, "ssl_cert", client1_cert);
testutil.changeSandboxConf(__mysql_sandbox_port1, "ssl_key", client1_cert.replace("-cert.pem", "-key.pem"));

testutil.changeSandboxConf(__mysql_sandbox_port2, "ssl_ca", ca1_path);
testutil.changeSandboxConf(__mysql_sandbox_port2, "ssl_cert", client1_cert);
testutil.changeSandboxConf(__mysql_sandbox_port2, "ssl_key", client1_cert.replace("-cert.pem", "-key.pem"));

testutil.restartSandbox(__mysql_sandbox_port1);
testutil.restartSandbox(__mysql_sandbox_port2);
testutil.restartSandbox(__mysql_sandbox_port3);

function cert_uri(user, sbport, ca_path, cert_path) {
    key_path = cert_path.replace("-cert.pem", "-key.pem");
    return `${user}@127.0.0.1:${sbport}?ssl-ca=(${ca_path})&ssl-cert=(${cert_path})&ssl-key=(${key_path})`;
}

ssl_sandbox_uri1 = cert_uri("admin", __mysql_sandbox_port1, ca1_path, client1_cert);
ssl_sandbox_uri2 = cert_uri("admin", __mysql_sandbox_port2, ca1_path, client2_cert);
ssl_sandbox_uri3 = cert_uri("admin", __mysql_sandbox_port3, ca1_path, client3_cert);

session1 = mysql.getSession(ssl_sandbox_uri1);
session2 = mysql.getSession(ssl_sandbox_uri2);
session3 = mysql.getSession(ssl_sandbox_uri3);

//@ createCluster
shell.connect(ssl_sandbox_uri1);
var cluster = dba.createCluster("clus", { gtidSetIsComplete: 1 });

//@<> add a Read-Replica {VER(>=8.0.23)}
EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__endpoint2); });

// Clean-up
EXPECT_NO_THROWS(function() { cluster.removeInstance(__endpoint2); });

//@<> create accounts for certificate authentication
cluster.setupAdminAccount("user1", {requireCertIssuer:"/CN=Test_CA", requireCertSubject:"/CN=user1@%", password:""});

cluster.setupRouterAccount("router", {requireCertIssuer:"/CN=Test_CA", requireCertSubject:"/CN=router@%", password:""});

s = mysql.getSession(cert_uri("user1", __mysql_sandbox_port1, ca1_path, user1_cert));
EXPECT_EQ("user1@%", s.runSql("select current_user()").fetchOne()[0]);

s = mysql.getSession(cert_uri("router", __mysql_sandbox_port1, ca1_path, router_cert));
EXPECT_EQ("router@%", s.runSql("select current_user()").fetchOne()[0]);

//@<> getCluster
shell.connect(cert_uri("user1", __mysql_sandbox_port1, ca1_path, user1_cert));
cluster = dba.getCluster();

//@<> addInstance overriding the user
cluster.addInstance("root:root@127.0.0.1:"+__mysql_sandbox_port2);

//@<> addInstance with cert

EXPECT_THROWS(function(){cluster.addInstance("127.0.0.1:"+__mysql_sandbox_port3);}, "Access denied for user 'user1'@'localhost'");

dba.configureInstance("root:root@127.0.0.1:"+__mysql_sandbox_port3, {mycnfPath: testutil.getSandboxConfPath(__mysql_sandbox_port3), clusterAdmin:"user1@'%'", clusterAdminPassword:"", clusterAdminCertIssuer:"/CN=Test_CA", clusterAdminCertSubject:"/CN=user1@%"});
cluster.addInstance("127.0.0.1:"+__mysql_sandbox_port3);

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

shell.connect(ssl_sandbox_uri2);
cluster = dba.getCluster();

cluster.forceQuorumUsingPartitionOf(ssl_sandbox_uri2);
shell.dumpRows(session.runSql("SELECT * FROM performance_schema.replication_group_members"));
cluster.rejoinInstance(ssl_sandbox_uri1);

testutil.startSandbox(__mysql_sandbox_port3);
session3 = mysql.getSession(ssl_sandbox_uri3);

//@<> forceQuorum (rejoin restarted sandbox without persisted start_on_boot) {VER(<8.0.0)}
cluster.rejoinInstance(ssl_sandbox_uri3);

//@ rebootCluster
testutil.stopGroup([__mysql_sandbox_port1,__mysql_sandbox_port2,__mysql_sandbox_port3]);

shell.connect(ssl_sandbox_uri1);
cluster = dba.rebootClusterFromCompleteOutage("clus");
cluster.status();

//@ rescan
// remove an instance from the MD
session.runSql("delete from mysql_innodb_cluster_metadata.instances where instance_name=?", ["127.0.0.1:" + __mysql_sandbox_port2]);

cluster.rescan();

//@ dissolve
cluster.dissolve();

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
