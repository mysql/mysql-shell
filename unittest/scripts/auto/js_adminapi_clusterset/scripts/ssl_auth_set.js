//@ {VER(>=8.0.27)}

//@<> INCLUDE clusterset_utils.inc

//@<> Setup

// create a common CA
ca_path = testutil.sslCreateCa("myca", "/CN=Test_CA");

// needed to create sandbox folders
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});

// create server certificates
cert1_path = testutil.sslCreateCert("server", "myca", `/CN=${hostname}`, __mysql_sandbox_port1);
cert2_path = testutil.sslCreateCert("server", "myca", `/CN=${hostname}`, __mysql_sandbox_port2);

function update_conf(port, ca_path, cert_path) {
    testutil.changeSandboxConf(port, "ssl_ca", ca_path);
    testutil.changeSandboxConf(port, "ssl_cert", cert_path);
    testutil.changeSandboxConf(port, "ssl_key", cert_path.replace("-cert.pem", "-key.pem"));
    testutil.restartSandbox(port);
}

function check_users(session, has_pwd, has_issuer, has_subject) {
    var num_users = session.runSql("SELECT count(*) FROM mysql.user WHERE (user like 'mysql_innodb%')").fetchOne()[0];
    EXPECT_NE(num_users, 0, "No users named 'mysql_innodb' present");

    if (has_pwd) {
        EXPECT_EQ(0, session.runSql("SELECT count(*) FROM mysql.user WHERE (user like 'mysql_innodb%') AND (length(authentication_string) = 0)").fetchOne()[0], "accounts without password present");
    }
    else {
        EXPECT_EQ(0, session.runSql("SELECT count(*) FROM mysql.user WHERE (user like 'mysql_innodb%') AND (length(authentication_string) <> 0)").fetchOne()[0], "accounts with password present");
    }

    if (has_issuer) {
        EXPECT_EQ(0, session.runSql("SELECT count(*) FROM mysql.user WHERE (user like 'mysql_innodb%') AND (length(x509_issuer) = 0)").fetchOne()[0], "accounts without issuer present");
    }
    else {
        EXPECT_EQ(0, session.runSql("SELECT count(*) FROM mysql.user WHERE (user like 'mysql_innodb%') AND (length(x509_issuer) <> 0)").fetchOne()[0], "accounts with issuer present");
    }

    if (has_subject) {
        EXPECT_EQ(0, session.runSql("SELECT count(*) FROM mysql.user WHERE (user like 'mysql_innodb%') AND (length(x509_subject) = 0)").fetchOne()[0], "accounts without issuer present");
    }
    else {
        EXPECT_EQ(0, session.runSql("SELECT count(*) FROM mysql.user WHERE (user like 'mysql_innodb%') AND (length(x509_subject) <> 0)").fetchOne()[0], "accounts with issuer present");
    }
}

function check_option(options, option_name, option_value) {
    for(i in options) {
        var option = options[i]
        if (option["option"] == option_name) {
            EXPECT_EQ(option_value, option["value"], option_value);
            break;
        }
    }
}

//restart the servers with proper SSL support
update_conf(__mysql_sandbox_port1, ca_path, cert1_path);
update_conf(__mysql_sandbox_port2, ca_path, cert2_path);

var session1 = mysql.getSession(__sandbox_uri1);
var session2 = mysql.getSession(__sandbox_uri2);

//@<> FR1.1 create cluster set (just password) / create replica cluster
shell.connect(__sandbox_uri1);

var cluster;
EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", { gtidSetIsComplete: 1, memberAuthType: "PASSWORD" }); });

var cset;
EXPECT_NO_THROWS(function() { cset = cluster.createClusterSet("cset"); });

EXPECT_EQ("PASSWORD", session.runSql("SELECT attributes->>'$.opt_memberAuthType' FROM mysql_innodb_cluster_metadata.clustersets WHERE (domain_name = 'cset')").fetchOne()[0]);
EXPECT_EQ("", session.runSql("SELECT attributes->>'$.opt_certIssuer' FROM mysql_innodb_cluster_metadata.clustersets WHERE (domain_name = 'cset')").fetchOne()[0]);
EXPECT_EQ("REQUIRED", session.runSql("SELECT attributes->>'$.opt_clusterSetReplicationSslMode' FROM mysql_innodb_cluster_metadata.clustersets WHERE (domain_name = 'cset')").fetchOne()[0]);

check_option(cset.options().clusterSet.globalOptions, "clusterSetReplicationSslMode", "REQUIRED");

WIPE_OUTPUT();

var rcluster;
EXPECT_NO_THROWS(function() { rcluster = cset.createReplicaCluster(__sandbox_uri2, "cluster2", {certSubject: "/CN=foo" }); });
EXPECT_OUTPUT_CONTAINS("The cluster's SSL mode (inherited from the ClusterSet) is set to PASSWORD, which makes the 'certSubject' option not required. The option will be ignored.");
EXPECT_OUTPUT_CONTAINS("Cluster \"memberAuthType\" is set to 'PASSWORD' (inherited from the ClusterSet).");

check_users(session, true, false, false);

status = cset.status({extended: 1});
EXPECT_CONTAINS("TLS_AES_", status.clusters.cluster2.clusterSetReplication.replicationSsl);
EXPECT_EQ("REQUIRED", status.clusters.cluster2.clusterSetReplication.replicationSslMode);

// cleanup cluster
reset_instance(session1);
reset_instance(session2);

//@<> FR1.1 create cluster (issuer) / create replica cluster
shell.connect(__sandbox_uri1);

var cluster;
EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", { gtidSetIsComplete: 1, memberAuthType: "CERT_ISSUER_PASSWORD", certIssuer: "/CN=Test_CA" }); });

var cset;
EXPECT_NO_THROWS(function() { cset = cluster.createClusterSet("cset"); });

EXPECT_EQ("CERT_ISSUER_PASSWORD", session.runSql("SELECT attributes->>'$.opt_memberAuthType' FROM mysql_innodb_cluster_metadata.clustersets WHERE (domain_name = 'cset')").fetchOne()[0]);
EXPECT_EQ("/CN=Test_CA", session.runSql("SELECT attributes->>'$.opt_certIssuer' FROM mysql_innodb_cluster_metadata.clustersets WHERE (domain_name = 'cset')").fetchOne()[0]);

check_option(cset.options().clusterSet.globalOptions, "clusterSetReplicationSslMode", "REQUIRED");

var rcluster;
EXPECT_NO_THROWS(function() { rcluster = cset.createReplicaCluster(__sandbox_uri2, "cluster2"); });
EXPECT_OUTPUT_CONTAINS("Cluster \"memberAuthType\" is set to 'CERT_ISSUER_PASSWORD' (inherited from the ClusterSet).");

check_users(session, true, true, false);
EXPECT_OUTPUT_CONTAINS("* Checking connectivity and SSL configuration...");

status = cset.status({extended: 1});
EXPECT_CONTAINS("TLS_AES_", status.clusters.cluster2.clusterSetReplication.replicationSsl);
EXPECT_EQ("REQUIRED", status.clusters.cluster2.clusterSetReplication.replicationSslMode);

// cleanup cluster
reset_instance(session1);
reset_instance(session2);

//@<> FR1.1 create cluster (issuer no password) / create replica cluster
shell.connect(__sandbox_uri1);

var cluster;
EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", { gtidSetIsComplete: 1, memberAuthType: "CERT_ISSUER", certIssuer: "/CN=Test_CA" }); });

var cset;
EXPECT_NO_THROWS(function() { cset = cluster.createClusterSet("cset"); });

EXPECT_EQ("CERT_ISSUER", session.runSql("SELECT attributes->>'$.opt_memberAuthType' FROM mysql_innodb_cluster_metadata.clustersets WHERE (domain_name = 'cset')").fetchOne()[0]);
EXPECT_EQ("/CN=Test_CA", session.runSql("SELECT attributes->>'$.opt_certIssuer' FROM mysql_innodb_cluster_metadata.clustersets WHERE (domain_name = 'cset')").fetchOne()[0]);

check_option(cset.options().clusterSet.globalOptions, "clusterSetReplicationSslMode", "REQUIRED");

var rcluster;
EXPECT_NO_THROWS(function() { rcluster = cset.createReplicaCluster(__sandbox_uri2, "cluster2"); });

check_users(session, false, true, false);
EXPECT_OUTPUT_CONTAINS("* Checking connectivity and SSL configuration...");

status = cset.status({extended: 1});
EXPECT_CONTAINS("TLS_AES_", status.clusters.cluster2.clusterSetReplication.replicationSsl);
EXPECT_EQ("REQUIRED", status.clusters.cluster2.clusterSetReplication.replicationSslMode);

// cleanup cluster
reset_instance(session1);
reset_instance(session2);

//@<> FR1.1 create cluster (issuer no password) / create replica cluster with clone
shell.connect(__sandbox_uri1);

var cluster;
EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", { gtidSetIsComplete: 1, memberAuthType: "CERT_ISSUER", certIssuer: "/CN=Test_CA" }); });

var cset;
EXPECT_NO_THROWS(function() { cset = cluster.createClusterSet("cset"); });

var rcluster;
EXPECT_NO_THROWS(function() { rcluster = cset.createReplicaCluster(__sandbox_uri2, "cluster2", { recoveryMethod: "clone" }); });

// cleanup cluster
reset_instance(session1);
var session2 = mysql.getSession(__sandbox_uri2);
reset_instance(session2);

//@<> FR1.1 / FR4 create cluster set (issuer + subject) / create replica cluster
shell.connect(__sandbox_uri1);

var cluster;
EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", { gtidSetIsComplete: 1, memberAuthType: "CERT_SUBJECT_PASSWORD", certIssuer: "/CN=Test_CA", certSubject: `/CN=${hostname}` }); });

var cset;
EXPECT_NO_THROWS(function() { cset = cluster.createClusterSet("cset"); });

EXPECT_EQ("CERT_SUBJECT_PASSWORD", session.runSql("SELECT attributes->>'$.opt_memberAuthType' FROM mysql_innodb_cluster_metadata.clustersets WHERE (domain_name = 'cset')").fetchOne()[0]);
EXPECT_EQ("/CN=Test_CA", session.runSql("SELECT attributes->>'$.opt_certIssuer' FROM mysql_innodb_cluster_metadata.clustersets WHERE (domain_name = 'cset')").fetchOne()[0]);

check_option(cset.options().clusterSet.globalOptions, "clusterSetReplicationSslMode", "REQUIRED");

EXPECT_THROWS(function() {
    cset.createReplicaCluster(__sandbox_uri2, "cluster2");
}, "The cluster's SSL mode (inherited from the ClusterSet) is set to CERT_SUBJECT_PASSWORD but the 'certSubject' option for the instance wasn't supplied.");

EXPECT_THROWS(function() {
    cset.createReplicaCluster(__sandbox_uri2, "cluster2", { certSubject: "" });
}, "Invalid value for 'certSubject' option. Value cannot be an empty string.");

var rcluster;
EXPECT_NO_THROWS(function() { rcluster = cset.createReplicaCluster(__sandbox_uri2, "cluster2", { certSubject: `/CN=${hostname}` }); });

check_users(session, true, true, true);
EXPECT_OUTPUT_CONTAINS("* Checking connectivity and SSL configuration...");

status = cset.status({extended: 1});
EXPECT_CONTAINS("TLS_AES_", status.clusters.cluster2.clusterSetReplication.replicationSsl);
EXPECT_EQ("REQUIRED", status.clusters.cluster2.clusterSetReplication.replicationSslMode);

// cleanup cluster
reset_instance(session1);
reset_instance(session2);

//@<> FR1.1 / FR4 create cluster set (issuer + subject) / create replica cluster with clone
shell.connect(__sandbox_uri1);

var cluster;
EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", { gtidSetIsComplete: 1, memberAuthType: "CERT_SUBJECT_PASSWORD", certIssuer: "/CN=Test_CA", certSubject: `/CN=${hostname}` }); });

var cset;
EXPECT_NO_THROWS(function() { cset = cluster.createClusterSet("cset"); });

var rcluster;
EXPECT_NO_THROWS(function() { rcluster = cset.createReplicaCluster(__sandbox_uri2, "cluster2", { certSubject: `/CN=${hostname}`, recoveryMethod: "clone" }); });

status = cset.status({extended: 1});
EXPECT_CONTAINS("TLS_AES_", status.clusters.cluster2.clusterSetReplication.replicationSsl);
EXPECT_EQ("REQUIRED", status.clusters.cluster2.clusterSetReplication.replicationSslMode);

// cleanup cluster
reset_instance(session1);
var session2 = mysql.getSession(__sandbox_uri2);
reset_instance(session2);

//@<> FR10 'clusterSetReplicationSslMode' option in cluster.createClusterSet()
shell.connect(__sandbox_uri1);

var cluster;
EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", { gtidSetIsComplete: 1, memberAuthType: "PASSWORD" }); });

var cset;
EXPECT_NO_THROWS(function() { cset = cluster.createClusterSet("cset", { clusterSetReplicationSslMode: "VERIFY_CA" }); });

EXPECT_EQ("PASSWORD", session.runSql("SELECT attributes->>'$.opt_memberAuthType' FROM mysql_innodb_cluster_metadata.clustersets WHERE (domain_name = 'cset')").fetchOne()[0]);
EXPECT_EQ("VERIFY_CA", session.runSql("SELECT attributes->>'$.opt_clusterSetReplicationSslMode' FROM mysql_innodb_cluster_metadata.clustersets WHERE (domain_name = 'cset')").fetchOne()[0]);

var rcluster;
EXPECT_NO_THROWS(function() { rcluster = cset.createReplicaCluster(__sandbox_uri2, "cluster2"); });

status = cset.status({extended: 1});
EXPECT_CONTAINS("TLS_AES_", status.clusters.cluster2.clusterSetReplication.replicationSsl);
EXPECT_EQ("VERIFY_CA", status.clusters.cluster2.clusterSetReplication.replicationSslMode);

// FR15
options = cset.options();
check_option(options.clusterSet.globalOptions, "clusterSetReplicationSslMode", "VERIFY_CA");

// cleanup cluster
reset_instance(session1);
reset_instance(session2);

//@<> FR10 'clusterSetReplicationSslMode' option in cluster.createClusterSet() + memberAuthType
shell.connect(__sandbox_uri1);

var cluster;
EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", { gtidSetIsComplete: 1, memberAuthType: "CERT_SUBJECT_PASSWORD", certIssuer: "/CN=Test_CA", certSubject: `/CN=${hostname}` }); });

var cset;
EXPECT_NO_THROWS(function() { cset = cluster.createClusterSet("cset", { clusterSetReplicationSslMode: "VERIFY_IDENTITY" }); });

EXPECT_EQ("CERT_SUBJECT_PASSWORD", session.runSql("SELECT attributes->>'$.opt_memberAuthType' FROM mysql_innodb_cluster_metadata.clustersets WHERE (domain_name = 'cset')").fetchOne()[0]);
EXPECT_EQ("VERIFY_IDENTITY", session.runSql("SELECT attributes->>'$.opt_clusterSetReplicationSslMode' FROM mysql_innodb_cluster_metadata.clustersets WHERE (domain_name = 'cset')").fetchOne()[0]);
EXPECT_EQ("/CN=Test_CA", session.runSql("SELECT attributes->>'$.opt_certIssuer' FROM mysql_innodb_cluster_metadata.clustersets WHERE (domain_name = 'cset')").fetchOne()[0]);

var rcluster;
EXPECT_NO_THROWS(function() { rcluster = cset.createReplicaCluster(__sandbox_uri2, "cluster2", { certSubject: `/CN=${hostname}` }); });
EXPECT_EQ(`/CN=${hostname}`, session.runSql(`SELECT attributes->>'$.opt_certSubject' FROM mysql_innodb_cluster_metadata.instances WHERE (address = '${hostname}:${__mysql_sandbox_port2}')`).fetchOne()[0]);

check_users(session, true, true, true);
EXPECT_OUTPUT_CONTAINS("* Checking connectivity and SSL configuration...");

status = cset.status({extended: 1});
EXPECT_CONTAINS("TLS_AES_", status.clusters.cluster2.clusterSetReplication.replicationSsl);
EXPECT_EQ("VERIFY_IDENTITY", status.clusters.cluster2.clusterSetReplication.replicationSslMode);

// FR15
options = cset.options();
check_option(options.clusterSet.globalOptions, "clusterSetReplicationSslMode", "VERIFY_IDENTITY");

// cleanup cluster
reset_instance(session1);
reset_instance(session2);

//@<> FR10 'clusterSetReplicationSslMode' should default to cluster's 'memberSslMode'
shell.connect(__sandbox_uri1);

var cluster;
EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", { gtidSetIsComplete: 1, memberSslMode: "VERIFY_IDENTITY" }); });

var cset;
EXPECT_NO_THROWS(function() { cset = cluster.createClusterSet("cset"); });

EXPECT_EQ("VERIFY_IDENTITY", session.runSql("SELECT attributes->>'$.opt_clusterSetReplicationSslMode' FROM mysql_innodb_cluster_metadata.clustersets WHERE (domain_name = 'cset')").fetchOne()[0]);

var rcluster;
EXPECT_NO_THROWS(function() { rcluster = cset.createReplicaCluster(__sandbox_uri2, "cluster2", { memberSslMode: "VERIFY_CA" }); });

status = cset.status({extended: 1});
EXPECT_CONTAINS("TLS_AES_", status.clusters.cluster2.clusterSetReplication.replicationSsl);
EXPECT_EQ("VERIFY_IDENTITY", status.clusters.cluster2.clusterSetReplication.replicationSslMode);

// cleanup cluster
reset_instance(session1);
reset_instance(session2);

//@<> FR10 'clusterSetReplicationSslMode' with DISABLED
shell.connect(__sandbox_uri1);

var cluster;
EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", { gtidSetIsComplete: 1, memberSslMode: "VERIFY_IDENTITY" }); });

var cset;
EXPECT_NO_THROWS(function() { cset = cluster.createClusterSet("cset", { clusterSetReplicationSslMode: "DISABLED" }); });

EXPECT_EQ("DISABLED", session.runSql("SELECT attributes->>'$.opt_clusterSetReplicationSslMode' FROM mysql_innodb_cluster_metadata.clustersets WHERE (domain_name = 'cset')").fetchOne()[0]);

var rcluster;
EXPECT_NO_THROWS(function() { rcluster = cset.createReplicaCluster(__sandbox_uri2, "cluster2", { memberSslMode: "VERIFY_CA" }); });

status = cset.status({extended: 1});
EXPECT_EQ(null, status.clusters.cluster2.clusterSetReplication.replicationSsl);
EXPECT_EQ("DISABLED", status.clusters.cluster2.clusterSetReplication.replicationSslMode);

// cleanup cluster
reset_instance(session1);
reset_instance(session2);

//@<> FR9 create replica cluster with different memberSslMode
shell.connect(__sandbox_uri1);

var cluster;
EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", { gtidSetIsComplete: 1, memberSslMode: "VERIFY_IDENTITY" }); });

var cset;
EXPECT_NO_THROWS(function() { cset = cluster.createClusterSet("cset"); });

var rcluster;
EXPECT_NO_THROWS(function() { rcluster = cset.createReplicaCluster(__sandbox_uri2, "cluster2", { memberSslMode: "VERIFY_CA" }); });

options = cluster.options();
check_option(options.defaultReplicaSet.globalOptions, "memberSslMode", "VERIFY_IDENTITY");
options = rcluster.options();
check_option(options.defaultReplicaSet.globalOptions, "memberSslMode", "VERIFY_CA");

status = cset.status({extended: 3});
EXPECT_EQ("VERIFY_IDENTITY", status.clusters.cluster.ssl);
EXPECT_EQ("VERIFY_CA", status.clusters.cluster2.ssl);
EXPECT_CONTAINS("TLS_AES_", status.clusters.cluster2.clusterSetReplication.replicationSsl);
EXPECT_EQ("VERIFY_IDENTITY", status.clusters.cluster2.clusterSetReplication.replicationSslMode);

// cleanup cluster
reset_instance(session1);
reset_instance(session2);

//@<> FR12 check exception if SSL settings are incorrect

ca2_path = testutil.sslCreateCa("myca2", "/CN=Test_CA2");
cert2_ca2_path = testutil.sslCreateCert("server2", "myca2", `/CN=${hostname}`, __mysql_sandbox_port2);
update_conf(__mysql_sandbox_port2, ca2_path, cert2_ca2_path);

var session1 = mysql.getSession(__sandbox_uri1);
var session2 = mysql.getSession(__sandbox_uri2);

reset_instance(session1);
reset_instance(session2);

var cluster;
EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", { gtidSetIsComplete: 1, memberAuthType: "CERT_SUBJECT", certIssuer: "/CN=Test_CA", certSubject: `/CN=${hostname}` }); });

var cset;
EXPECT_NO_THROWS(function() { cset = cluster.createClusterSet("cset", { clusterSetReplicationSslMode: "VERIFY_IDENTITY" }); });

EXPECT_THROWS(function() {
    cset.createReplicaCluster(__sandbox_uri2, "cluster2", { certSubject: `/CN=${hostname}` });
}, "Authentication error during connection check");
EXPECT_OUTPUT_CONTAINS("* Checking connectivity and SSL configuration...");
EXPECT_OUTPUT_CONTAINS("Authentication error during connection check.");

// cleanup cluster
reset_instance(session1);
reset_instance(session2);

//@<> check reverted remove cluster {!__dbug_off}

update_conf(__mysql_sandbox_port2, ca_path, cert2_path);

var session1 = mysql.getSession(__sandbox_uri1);
var session2 = mysql.getSession(__sandbox_uri2);

shell.connect(__sandbox_uri1);

var cluster;
EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", { gtidSetIsComplete: 1, memberAuthType: "CERT_SUBJECT_PASSWORD", certIssuer: "/CN=Test_CA", certSubject: `/CN=${hostname}` }); });

var cset;
EXPECT_NO_THROWS(function() { cset = cluster.createClusterSet("cset", { clusterSetReplicationSslMode: "VERIFY_IDENTITY" }); });

EXPECT_EQ("CERT_SUBJECT_PASSWORD", session.runSql("SELECT attributes->>'$.opt_memberAuthType' FROM mysql_innodb_cluster_metadata.clustersets WHERE (domain_name = 'cset')").fetchOne()[0]);
EXPECT_EQ("VERIFY_IDENTITY", session.runSql("SELECT attributes->>'$.opt_clusterSetReplicationSslMode' FROM mysql_innodb_cluster_metadata.clustersets WHERE (domain_name = 'cset')").fetchOne()[0]);
EXPECT_EQ("/CN=Test_CA", session.runSql("SELECT attributes->>'$.opt_certIssuer' FROM mysql_innodb_cluster_metadata.clustersets WHERE (domain_name = 'cset')").fetchOne()[0]);

var rcluster;
EXPECT_NO_THROWS(function() { rcluster = cset.createReplicaCluster(__sandbox_uri2, "cluster2", { certSubject: `/CN=${hostname}` }); });
EXPECT_EQ(`/CN=${hostname}`, session.runSql(`SELECT attributes->>'$.opt_certSubject' FROM mysql_innodb_cluster_metadata.instances WHERE (address = '${hostname}:${__mysql_sandbox_port2}')`).fetchOne()[0]);

testutil.dbugSet("+d,dba_remove_cluster_fail_disable_skip_replica_start");

EXPECT_THROWS_TYPE(function(){ cset.removeCluster("cluster2"); }, "debug", "LogicError");

check_users(session, true, true, true);
EXPECT_EQ("CERT_SUBJECT_PASSWORD", session.runSql("SELECT attributes->>'$.opt_memberAuthType' FROM mysql_innodb_cluster_metadata.clusters WHERE (cluster_name = 'cluster2')").fetchOne()[0]);
EXPECT_EQ("/CN=Test_CA", session.runSql("SELECT attributes->>'$.opt_certIssuer' FROM mysql_innodb_cluster_metadata.clusters WHERE (cluster_name = 'cluster2')").fetchOne()[0]);
EXPECT_EQ(`/CN=${hostname}`, session.runSql(`SELECT attributes->>'$.opt_certSubject' FROM mysql_innodb_cluster_metadata.instances WHERE (address = '${hostname}:${__mysql_sandbox_port1}')`).fetchOne()[0]);
EXPECT_EQ(`/CN=${hostname}`, session.runSql(`SELECT attributes->>'$.opt_certSubject' FROM mysql_innodb_cluster_metadata.instances WHERE (address = '${hostname}:${__mysql_sandbox_port2}')`).fetchOne()[0]);

// cleanup cluster
reset_instance(session1);
reset_instance(session2);

//@<> Cleanup
session1.close();
session2.close();

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
