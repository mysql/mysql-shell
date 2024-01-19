//@<> Setup

// create a common CA
ca_path = testutil.sslCreateCa("myca", "/CN=Test_CA");

// needed to create sandbox folders
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});

// create server certificates
cert1_path = testutil.sslCreateCert("server", "myca", `/CN=${hostname}`, __mysql_sandbox_port1);
cert2_path = testutil.sslCreateCert("server", "myca", `/CN=${hostname}`, __mysql_sandbox_port2);
cert3_path = testutil.sslCreateCert("server", "myca", `/CN=${hostname}`, __mysql_sandbox_port3);

function update_conf(port, ca_path, cert_path) {
    testutil.changeSandboxConf(port, "ssl_ca", ca_path);
    testutil.changeSandboxConf(port, "ssl_cert", cert_path);
    testutil.changeSandboxConf(port, "ssl_key", cert_path.replace("-cert.pem", "-key.pem"));
    testutil.restartSandbox(port);
}

function check_instance_error(status, include_primary, expected_error) {
    if (include_primary) {
        EXPECT_TRUE("instanceErrors" in status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]);
        rr_error = status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["instanceErrors"];
        EXPECT_EQ(rr_error.length, 1);
        EXPECT_EQ(rr_error[0], expected_error);
    }

    EXPECT_TRUE("instanceErrors" in status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]);
    rr_error = status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["instanceErrors"];
    EXPECT_EQ(rr_error.length, 1);
    EXPECT_EQ(rr_error[0], expected_error);

    if (__version_num >= 80023) {
        EXPECT_TRUE("instanceErrors" in status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["readReplicas"][`${hostname}:${__mysql_sandbox_port3}`]);
        rr_error = status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["readReplicas"][`${hostname}:${__mysql_sandbox_port3}`]["instanceErrors"];
        EXPECT_EQ(rr_error.length, 1);
        EXPECT_EQ(rr_error[0], expected_error);
    }
};

function reset_instances() {
    if (__version_num >= 80023) {
        shell.connect(__sandbox_uri3);
        reset_instance(session);
    }
    shell.connect(__sandbox_uri2);
    reset_instance(session);
    shell.connect(__sandbox_uri1);
    reset_instance(session);

    /*
    * GR has a 5 second hardcoded timeout to update the information of group membership of managed channels. When
    *   dealing with read-replicas, after the AR channel is configured, there's a call to the
    *   asynchronous_connection_failover_delete_managed() UDF with information read from
    *   performance_schema.replication_asynchronous_connection_failover_managed, which, because of the 5 second
    *   timeout, might return a false positive. To avoid this, we wait 5 seconds after reseting the instances.
    */
    if (__version_num >= 80023) {
        os.sleep(5);
    }
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

function check_apis(sslMode, authType) {

    reset_instances();

    var cluster;
    if (authType.includes("_SUBJECT")) {
        EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", { gtidSetIsComplete: 1, memberSslMode: sslMode, memberAuthType: authType, certIssuer: "/CN=Test_CA", certSubject: `/CN=${hostname}` }); });
        EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2, { certSubject: `/CN=${hostname}` }); });
        if (__version_num >= 80023) {
            EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri3, { certSubject: `/CN=${hostname}` }); });
        }
    }
    else if (authType.includes("_ISSUER")) {
        EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", { gtidSetIsComplete: 1, memberSslMode: sslMode, memberAuthType: authType, certIssuer: "/CN=Test_CA" }); });
        EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2); });
        if (__version_num >= 80023) {
            EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri3); });
        }
    }
    else {
        EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", { gtidSetIsComplete: 1, memberSslMode: sslMode, memberAuthType: authType }); });
        EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2); });
        if (__version_num >= 80023) {
            EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri3); });
        }
    }

    // reboot
    testutil.stopGroup([__mysql_sandbox_port1,__mysql_sandbox_port2]);
    EXPECT_NO_THROWS(function(){ cluster = dba.rebootClusterFromCompleteOutage("cluster"); });
    EXPECT_EQ(authType, session.runSql("SELECT attributes->>'$.opt_memberAuthType' FROM mysql_innodb_cluster_metadata.clusters WHERE (cluster_name = 'cluster')").fetchOne()[0]);
    if (authType.includes("_ISSUER") || authType.includes("_SUBJECT")) {
        EXPECT_EQ("/CN=Test_CA", session.runSql("SELECT attributes->>'$.opt_certIssuer' FROM mysql_innodb_cluster_metadata.clusters WHERE (cluster_name = 'cluster')").fetchOne()[0]);
    }
    if (authType.includes("_SUBJECT")) {
        EXPECT_EQ(`/CN=${hostname}`, session.runSql(`SELECT attributes->>'$.opt_certSubject' FROM mysql_innodb_cluster_metadata.instances WHERE (address = '${hostname}:${__mysql_sandbox_port1}')`).fetchOne()[0]);
    }

    // rejoin
    disable_auto_rejoin(__mysql_sandbox_port2);

    testutil.stopSandbox(__mysql_sandbox_port2);
    testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING)");
    testutil.startSandbox(__mysql_sandbox_port2);
    EXPECT_NO_THROWS(function(){ cluster.rejoinInstance(__sandbox_uri2); });

    // rejoin read-replica
    if (__version_num >= 80023) {
        shell.connect(__sandbox_uri3);
        session.runSql("STOP replica");

        shell.connect(__sandbox_uri1);
        testutil.waitReplicationChannelState(__mysql_sandbox_port3, "read_replica_replication", "OFF");

        EXPECT_NO_THROWS(function(){ cluster.rejoinInstance(__sandbox_uri3); });
    }

    // setPrimaryInstance
    if (testutil.versionCheck(__version, ">=", "8.0.13")) {
        EXPECT_NO_THROWS(function(){ cluster.setPrimaryInstance(__sandbox_uri2); });
        EXPECT_NO_THROWS(function(){ cluster.setPrimaryInstance(__sandbox_uri1); });
    }

    // remove instance
    EXPECT_NO_THROWS(function(){ cluster.removeInstance(__sandbox_uri2); });
    if (authType.includes("_SUBJECT")) {
        EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2, { certSubject: `/CN=${hostname}` }); });
    }
    else {
        EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2); });
    }

    // remove read-replica instance
    if (__version_num >= 80023) {
        EXPECT_NO_THROWS(function(){ cluster.removeInstance(__sandbox_uri3); });
        if (authType.includes("_SUBJECT")) {
            EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri3, { certSubject: `/CN=${hostname}` }); });
        }
        else {
            EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri3); });
        }
    }

    // rejoin error out
    if (authType.includes("_SUBJECT")) {
        disable_auto_rejoin(__mysql_sandbox_port2);

        WIPE_STDOUT();

        testutil.stopSandbox(__mysql_sandbox_port2);
        testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING)");
        testutil.startSandbox(__mysql_sandbox_port2);
        session.runSql(`UPDATE mysql_innodb_cluster_metadata.instances SET attributes = json_remove(attributes, '$.opt_certSubject') WHERE (address = '${hostname}:${__mysql_sandbox_port2}');`);

        shell.connect(__sandbox_uri1);
        cluster = dba.getCluster();

        EXPECT_THROWS(function() {
            cluster.rejoinInstance(__sandbox_uri2);
        }, `The Cluster's SSL mode is set to 'CERT_SUBJECT_PASSWORD' but the 'certSubject' option for the instance isn't valid.`);
        EXPECT_OUTPUT_CONTAINS("The Cluster's SSL mode is set to 'CERT_SUBJECT_PASSWORD' but the instance being rejoined doesn't have the 'certSubject' option set. Please remove the instance with Cluster.removeInstance() and then add it back using Cluster.addInstance() with the appropriate authentication options.");

        if (__version_num >= 80023) {
            shell.connect(__sandbox_uri3);
            session.runSql("STOP replica");

            shell.connect(__sandbox_uri1);
            testutil.waitReplicationChannelState(__mysql_sandbox_port3, "read_replica_replication", "OFF");

            session.runSql(`UPDATE mysql_innodb_cluster_metadata.instances SET attributes = json_remove(attributes, '$.opt_certSubject') WHERE (address = '${hostname}:${__mysql_sandbox_port3}');`);

            EXPECT_THROWS(function() {
                cluster.rejoinInstance(__sandbox_uri3);
            }, `The Cluster's SSL mode is set to 'CERT_SUBJECT_PASSWORD' but the 'certSubject' option for the instance isn't valid.`);
            EXPECT_OUTPUT_CONTAINS("The Cluster's SSL mode is set to 'CERT_SUBJECT_PASSWORD' but the instance being rejoined doesn't have the 'certSubject' option set. Please remove the instance with Cluster.removeInstance() and then add it back using Cluster.addReplicaInstance() with the appropriate authentication options.");
        }
    }
}

//restart the servers with proper SSL support
update_conf(__mysql_sandbox_port1, ca_path, cert1_path);
update_conf(__mysql_sandbox_port2, ca_path, cert2_path);
update_conf(__mysql_sandbox_port3, ca_path, cert3_path);

//@<> FR1 create cluster (just password)
shell.connect(__sandbox_uri1);

EXPECT_THROWS(function() {
    dba.createCluster("cluster", { gtidSetIsComplete: 1, memberAuthType: "--" });
}, "Invalid value for 'memberAuthType' option. Supported values: PASSWORD, CERT_ISSUER, CERT_SUBJECT, CERT_ISSUER_PASSWORD, CERT_SUBJECT_PASSWORD");
EXPECT_THROWS(function() {
    dba.createCluster("cluster", { gtidSetIsComplete: 1, certIssuer: "" });
}, "Invalid value for 'certIssuer' option. Value cannot be an empty string.");
EXPECT_THROWS(function() {
    dba.createCluster("cluster", { gtidSetIsComplete: 1, certSubject: "" });
}, "Invalid value for 'certSubject' option. Value cannot be an empty string.");
EXPECT_THROWS(function() {
    dba.createCluster("cluster", { gtidSetIsComplete: 1, memberSslMode: "DISABLED", memberAuthType: "CERT_ISSUER" });
}, `Instance '${hostname}:${__mysql_sandbox_port1}' has the \"memberSslMode\" option with the value 'DISABLED', which means that \"memberAuthType\" only accepts the value 'PASSWORD'.`);

var cluster;
EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", { gtidSetIsComplete: 1, memberAuthType: "PASSWORD" }); });

EXPECT_EQ("PASSWORD", session.runSql("SELECT attributes->>'$.opt_memberAuthType' FROM mysql_innodb_cluster_metadata.clusters WHERE (cluster_name = 'cluster')").fetchOne()[0]);
EXPECT_EQ("", session.runSql("SELECT attributes->>'$.opt_certIssuer' FROM mysql_innodb_cluster_metadata.clusters WHERE (cluster_name = 'cluster')").fetchOne()[0]);
EXPECT_EQ("", session.runSql("SELECT ssl_type FROM mysql.user WHERE (user LIKE 'mysql_innodb_cluster_%');").fetchOne()[0]);
EXPECT_NE("", session.runSql("SELECT authentication_string FROM mysql.user WHERE (user LIKE 'mysql_innodb_cluster_%');").fetchOne()[0]);

EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2); });
if (__version_num >= 80023) {
    EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri3); });
}

check_users(session, true, false, false);

// FR14
options = cluster.options();
options;
check_option(options.defaultReplicaSet.globalOptions, "memberAuthType", "PASSWORD");
check_option(options.defaultReplicaSet.globalOptions, "certIssuer", "");
check_option(options.defaultReplicaSet.topology[`${hostname}:${__mysql_sandbox_port1}`], "certSubject", "");
check_option(options.defaultReplicaSet.topology[`${hostname}:${__mysql_sandbox_port2}`], "certSubject", "");
if (__version_num >= 80023) {
    check_option(options.defaultReplicaSet.topology[`${hostname}:${__mysql_sandbox_port3}`], "certSubject", "");
}

WIPE_OUTPUT();
EXPECT_NO_THROWS(function() { cluster.resetRecoveryAccountsPassword(); });
EXPECT_OUTPUT_NOT_CONTAINS("which doesn't assign a password for the recovery users.");

// cleanup cluster
cluster.dissolve();

//@<> FR1 create cluster (just password) unnecessary issuer and subject
shell.connect(__sandbox_uri1);

var cluster;
EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", { gtidSetIsComplete: 1, memberAuthType: "PASSWORD", certIssuer: "/CN=fooIssuer", certSubject: "/CN=fooSubject" }); });
EXPECT_OUTPUT_CONTAINS("The cluster's SSL mode is set to PASSWORD, which makes both the 'certIssuer' and 'certSubject' options not required. Both options will be ignored.");

EXPECT_EQ("PASSWORD", session.runSql("SELECT attributes->>'$.opt_memberAuthType' FROM mysql_innodb_cluster_metadata.clusters WHERE (cluster_name = 'cluster')").fetchOne()[0]);
EXPECT_EQ("/CN=fooIssuer", session.runSql("SELECT attributes->>'$.opt_certIssuer' FROM mysql_innodb_cluster_metadata.clusters WHERE (cluster_name = 'cluster')").fetchOne()[0]);
EXPECT_EQ("/CN=fooSubject", session.runSql(`SELECT attributes->>'$.opt_certSubject' FROM mysql_innodb_cluster_metadata.instances WHERE (address = '${hostname}:${__mysql_sandbox_port1}')`).fetchOne()[0]);
EXPECT_EQ("", session.runSql("SELECT ssl_type FROM mysql.user WHERE (user LIKE 'mysql_innodb_cluster_%');").fetchOne()[0]);
EXPECT_NE("", session.runSql("SELECT authentication_string FROM mysql.user WHERE (user LIKE 'mysql_innodb_cluster_%');").fetchOne()[0]);

WIPE_STDOUT();
EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2, { certSubject: "/CN=bar" }); });
EXPECT_OUTPUT_CONTAINS("NOTE: The cluster's SSL mode is set to PASSWORD, which makes the 'certSubject' option not required. The option will be ignored.");

if (__version_num >= 80023) {
    WIPE_STDOUT();
    EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri3, { certSubject: "/CN=bar" }); });
    EXPECT_OUTPUT_CONTAINS("NOTE: The cluster's SSL mode is set to PASSWORD, which makes the 'certSubject' option not required. The option will be ignored.");
}

check_users(session, true, false, false);

options = cluster.options();
check_option(options.defaultReplicaSet.globalOptions, "memberAuthType", "PASSWORD");
check_option(options.defaultReplicaSet.globalOptions, "certIssuer", "/CN=fooIssuer");
check_option(options.defaultReplicaSet.topology[`${hostname}:${__mysql_sandbox_port1}`], "certSubject", "/CN=fooSubject");
check_option(options.defaultReplicaSet.topology[`${hostname}:${__mysql_sandbox_port2}`], "certSubject", "/CN=bar");
if (__version_num >= 80023) {
    check_option(options.defaultReplicaSet.topology[`${hostname}:${__mysql_sandbox_port3}`], "certSubject", "/CN=bar");
}

// cleanup cluster
cluster.dissolve();

//@<> FR1 create cluster (issuer with no password)
shell.connect(__sandbox_uri1);

var cluster;
EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", { gtidSetIsComplete: 1, memberAuthType: "CERT_ISSUER", certIssuer: "/CN=Test_CA" }); });

EXPECT_EQ("CERT_ISSUER", session.runSql("SELECT attributes->>'$.opt_memberAuthType' FROM mysql_innodb_cluster_metadata.clusters WHERE (cluster_name = 'cluster')").fetchOne()[0]);
EXPECT_EQ("/CN=Test_CA", session.runSql("SELECT attributes->>'$.opt_certIssuer' FROM mysql_innodb_cluster_metadata.clusters WHERE (cluster_name = 'cluster')").fetchOne()[0]);
EXPECT_NE("", session.runSql("SELECT ssl_type FROM mysql.user WHERE (user LIKE 'mysql_innodb_cluster_%');").fetchOne()[0]);
EXPECT_EQ("", session.runSql("SELECT authentication_string FROM mysql.user WHERE (user LIKE 'mysql_innodb_cluster_%');").fetchOne()[0]);

//@<> FR3 add instance (issuer with no password)

EXPECT_THROWS(function() {
    cluster.addInstance(__sandbox_uri2, { certSubject: "" });
}, "Invalid value for 'certSubject' option. Value cannot be an empty string.");

EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2); });

check_users(session, false, true, false);
EXPECT_OUTPUT_CONTAINS("* Checking connectivity and SSL configuration...");

// add read-replica instance
if (__version_num >= 80023) {
    EXPECT_THROWS(function() {
        cluster.addReplicaInstance(__sandbox_uri3, { certSubject: "" });
    }, "Invalid value for 'certSubject' option. Value cannot be an empty string.");

    EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri3); });

    check_users(session, false, true, false);
    EXPECT_OUTPUT_CONTAINS("* Checking connectivity and SSL configuration...");
}

// FR14
options = cluster.options();
check_option(options.defaultReplicaSet.globalOptions, "memberAuthType", "CERT_ISSUER");
check_option(options.defaultReplicaSet.globalOptions, "certIssuer", "/CN=Test_CA");
check_option(options.defaultReplicaSet.topology[`${hostname}:${__mysql_sandbox_port1}`], "certSubject", "");
check_option(options.defaultReplicaSet.topology[`${hostname}:${__mysql_sandbox_port2}`], "certSubject", "");
if (__version_num >= 80023) {
    check_option(options.defaultReplicaSet.topology[`${hostname}:${__mysql_sandbox_port3}`], "certSubject", "");
}

WIPE_OUTPUT();
EXPECT_NO_THROWS(function() { cluster.resetRecoveryAccountsPassword(); });
EXPECT_OUTPUT_CONTAINS("The Cluster's authentication type is 'CERT_ISSUER', which doesn't assign a password for the recovery users.");

// cleanup cluster
cluster.dissolve();

//@<> FR1 create cluster (issuer with no password) + clone {VER(>= 8.0.17)}
shell.connect(__sandbox_uri1);

var cluster;
EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", { gtidSetIsComplete: 1, memberAuthType: "CERT_ISSUER", certIssuer: "/CN=Test_CA" }); });

session.runSql("CREATE SCHEMA test");
session.runSql("CREATE TABLE test.data (a int primary key auto_increment, data longtext)");
for (i = 0; i < 50; i++) {
    session.runSql("INSERT INTO test.data VALUES(default, repeat('x', 4*1024))");
}

EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2, { recoveryMethod: "clone"} ); });
if (__version_num >= 80023) {
    EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri3, { recoveryMethod: "clone"}); });
}

// cleanup cluster
cluster.dissolve();

//@<> FR1 create cluster (issuer)
shell.connect(__sandbox_uri1);

var cluster;
EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", { gtidSetIsComplete: 1, memberAuthType: "CERT_ISSUER_PASSWORD", certIssuer: "/CN=Test_CA", certSubject: "/CN=foo" }); });
EXPECT_OUTPUT_CONTAINS("The cluster's SSL mode is set to CERT_ISSUER_PASSWORD, which makes the 'certSubject' option not required. The option will be ignored.");
EXPECT_EQ("/CN=foo", session.runSql(`SELECT attributes->>'$.opt_certSubject' FROM mysql_innodb_cluster_metadata.instances WHERE (address = '${hostname}:${__mysql_sandbox_port1}')`).fetchOne()[0]);

EXPECT_EQ("CERT_ISSUER_PASSWORD", session.runSql("SELECT attributes->>'$.opt_memberAuthType' FROM mysql_innodb_cluster_metadata.clusters WHERE (cluster_name = 'cluster')").fetchOne()[0]);
EXPECT_EQ("/CN=Test_CA", session.runSql("SELECT attributes->>'$.opt_certIssuer' FROM mysql_innodb_cluster_metadata.clusters WHERE (cluster_name = 'cluster')").fetchOne()[0]);
EXPECT_NE("", session.runSql("SELECT ssl_type FROM mysql.user WHERE (user LIKE 'mysql_innodb_cluster_%');").fetchOne()[0]);
EXPECT_NE("", session.runSql("SELECT authentication_string FROM mysql.user WHERE (user LIKE 'mysql_innodb_cluster_%');").fetchOne()[0]);

//@<> FR3 add instance (issuer)

WIPE_OUTPUT();
EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2, { certSubject: "/CN=bar" }); });
EXPECT_OUTPUT_CONTAINS("The cluster's SSL mode is set to CERT_ISSUER_PASSWORD, which makes the 'certSubject' option not required. The option will be ignored.");
EXPECT_EQ("/CN=foo", session.runSql(`SELECT attributes->>'$.opt_certSubject' FROM mysql_innodb_cluster_metadata.instances WHERE (address = '${hostname}:${__mysql_sandbox_port1}')`).fetchOne()[0]);
EXPECT_EQ("/CN=bar", session.runSql(`SELECT attributes->>'$.opt_certSubject' FROM mysql_innodb_cluster_metadata.instances WHERE (address = '${hostname}:${__mysql_sandbox_port2}')`).fetchOne()[0]);

check_users(session, true, true, false);
EXPECT_OUTPUT_CONTAINS("* Checking connectivity and SSL configuration...");

// add read-replica instance
if (__version_num >= 80023) {

    WIPE_OUTPUT();
    EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri3, { certSubject: "/CN=bar2" }); });
    EXPECT_OUTPUT_CONTAINS("The cluster's SSL mode is set to CERT_ISSUER_PASSWORD, which makes the 'certSubject' option not required. The option will be ignored.");
    EXPECT_EQ("/CN=foo", session.runSql(`SELECT attributes->>'$.opt_certSubject' FROM mysql_innodb_cluster_metadata.instances WHERE (address = '${hostname}:${__mysql_sandbox_port1}')`).fetchOne()[0]);
    EXPECT_EQ("/CN=bar2", session.runSql(`SELECT attributes->>'$.opt_certSubject' FROM mysql_innodb_cluster_metadata.instances WHERE (address = '${hostname}:${__mysql_sandbox_port3}')`).fetchOne()[0]);

    check_users(session, true, true, false);
    EXPECT_OUTPUT_CONTAINS("* Checking connectivity and SSL configuration...");
}

// FR14
options = cluster.options();
check_option(options.defaultReplicaSet.globalOptions, "memberAuthType", "CERT_ISSUER_PASSWORD");
check_option(options.defaultReplicaSet.globalOptions, "certIssuer", "/CN=Test_CA");
check_option(options.defaultReplicaSet.topology[`${hostname}:${__mysql_sandbox_port1}`], "certSubject", "/CN=foo");
check_option(options.defaultReplicaSet.topology[`${hostname}:${__mysql_sandbox_port2}`], "certSubject", "/CN=bar");
if (__version_num >= 80023) {
    check_option(options.defaultReplicaSet.topology[`${hostname}:${__mysql_sandbox_port3}`], "certSubject", "/CN=bar2");
}

WIPE_OUTPUT();
EXPECT_NO_THROWS(function() { cluster.resetRecoveryAccountsPassword(); });
EXPECT_OUTPUT_NOT_CONTAINS("which doesn't assign a password for the recovery users.");

// cleanup cluster
cluster.dissolve();

//@<> FR1 create cluster (issuer) unnecessary subject
shell.connect(__sandbox_uri1);

var cluster;
EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", { gtidSetIsComplete: 1, memberAuthType: "CERT_ISSUER_PASSWORD", certIssuer: "/CN=Test_CA", certSubject: "/CN=foo" }); });
EXPECT_OUTPUT_CONTAINS("The cluster's SSL mode is set to CERT_ISSUER_PASSWORD, which makes the 'certSubject' option not required. The option will be ignored.");

EXPECT_EQ("CERT_ISSUER_PASSWORD", session.runSql("SELECT attributes->>'$.opt_memberAuthType' FROM mysql_innodb_cluster_metadata.clusters WHERE (cluster_name = 'cluster')").fetchOne()[0]);
EXPECT_EQ("/CN=Test_CA", session.runSql("SELECT attributes->>'$.opt_certIssuer' FROM mysql_innodb_cluster_metadata.clusters WHERE (cluster_name = 'cluster')").fetchOne()[0]);
EXPECT_EQ("/CN=foo", session.runSql(`SELECT attributes->>'$.opt_certSubject' FROM mysql_innodb_cluster_metadata.instances WHERE (address = '${hostname}:${__mysql_sandbox_port1}')`).fetchOne()[0]);
EXPECT_NE("", session.runSql("SELECT ssl_type FROM mysql.user WHERE (user LIKE 'mysql_innodb_cluster_%');").fetchOne()[0]);
EXPECT_NE("", session.runSql("SELECT authentication_string FROM mysql.user WHERE (user LIKE 'mysql_innodb_cluster_%');").fetchOne()[0]);

WIPE_STDOUT();
EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2, { certSubject: "/CN=bar" }); });
EXPECT_OUTPUT_CONTAINS("* Checking connectivity and SSL configuration...");

check_users(session, true, true, false);

if (__version_num >= 80023) {
    WIPE_STDOUT();
    EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri3, { certSubject: "/CN=bar" }); });
    EXPECT_OUTPUT_CONTAINS("* Checking connectivity and SSL configuration...");

    check_users(session, true, true, false);
}

options = cluster.options();
check_option(options.defaultReplicaSet.globalOptions, "memberAuthType", "CERT_ISSUER_PASSWORD");
check_option(options.defaultReplicaSet.globalOptions, "certIssuer", "/CN=Test_CA");
check_option(options.defaultReplicaSet.topology[`${hostname}:${__mysql_sandbox_port1}`], "certSubject", "/CN=foo");
check_option(options.defaultReplicaSet.topology[`${hostname}:${__mysql_sandbox_port2}`], "certSubject", "/CN=bar");
if (__version_num >= 80023) {
    check_option(options.defaultReplicaSet.topology[`${hostname}:${__mysql_sandbox_port3}`], "certSubject", "/CN=bar");
}

// cleanup cluster
cluster.dissolve();

//@<> FR1 create cluster (issuer + subject)
shell.connect(__sandbox_uri1);

var cluster;
EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", { gtidSetIsComplete: 1, memberAuthType: "CERT_SUBJECT_PASSWORD", certIssuer: "/CN=Test_CA", certSubject: `/CN=${hostname}` }); });

EXPECT_EQ("CERT_SUBJECT_PASSWORD", session.runSql("SELECT attributes->>'$.opt_memberAuthType' FROM mysql_innodb_cluster_metadata.clusters WHERE (cluster_name = 'cluster')").fetchOne()[0]);
EXPECT_EQ("/CN=Test_CA", session.runSql("SELECT attributes->>'$.opt_certIssuer' FROM mysql_innodb_cluster_metadata.clusters WHERE (cluster_name = 'cluster')").fetchOne()[0]);
EXPECT_EQ(`/CN=${hostname}`, session.runSql(`SELECT attributes->>'$.opt_certSubject' FROM mysql_innodb_cluster_metadata.instances WHERE (address = '${hostname}:${__mysql_sandbox_port1}')`).fetchOne()[0]);
EXPECT_NE("", session.runSql("SELECT ssl_type FROM mysql.user WHERE (user LIKE 'mysql_innodb_cluster_%');").fetchOne()[0]);
EXPECT_NE("", session.runSql("SELECT authentication_string FROM mysql.user WHERE (user LIKE 'mysql_innodb_cluster_%');").fetchOne()[0]);

//@<> FR3 add instance (issuer + subject)
EXPECT_THROWS(function() {
    cluster.addInstance(__sandbox_uri2);
}, "The cluster's SSL mode is set to CERT_SUBJECT_PASSWORD but the 'certSubject' option for the instance wasn't supplied.");

EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2, { certSubject: `/CN=${hostname}` }); });

check_users(session, true, true, true);
EXPECT_OUTPUT_CONTAINS("* Checking connectivity and SSL configuration...");

// add read-replica instance
if (__version_num >= 80023) {

    EXPECT_THROWS(function() {
        cluster.addReplicaInstance(__sandbox_uri3);
    }, "The cluster's SSL mode is set to CERT_SUBJECT_PASSWORD but the 'certSubject' option for the instance wasn't supplied.");

    EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri3, { certSubject: `/CN=${hostname}` }); });

    check_users(session, true, true, true);
    EXPECT_OUTPUT_CONTAINS("* Checking connectivity and SSL configuration...");
}

// FR14
options = cluster.options();
check_option(options.defaultReplicaSet.globalOptions, "memberAuthType", "CERT_SUBJECT_PASSWORD");
check_option(options.defaultReplicaSet.globalOptions, "certIssuer", "/CN=Test_CA");
check_option(options.defaultReplicaSet.topology[`${hostname}:${__mysql_sandbox_port1}`], "certSubject", `/CN=${hostname}`);
check_option(options.defaultReplicaSet.topology[`${hostname}:${__mysql_sandbox_port2}`], "certSubject", `/CN=${hostname}`);
if (__version_num >= 80023) {
    check_option(options.defaultReplicaSet.topology[`${hostname}:${__mysql_sandbox_port3}`], "certSubject", `/CN=${hostname}`);
}

WIPE_OUTPUT();
EXPECT_NO_THROWS(function() { cluster.resetRecoveryAccountsPassword(); });
EXPECT_OUTPUT_NOT_CONTAINS("which doesn't assign a password for the recovery users.");

//@<> FR1 create cluster (issuer + subject) with XCOM comm stack {VER(>=8.0.27)}

// cleanup previous cluster
cluster.dissolve();

shell.connect(__sandbox_uri1);

var cluster;
EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", { gtidSetIsComplete: 1, memberAuthType: "CERT_SUBJECT_PASSWORD", certIssuer: "/CN=Test_CA", certSubject: `/CN=${hostname}`, communicationStack: "XCOM" }); });
EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2, { certSubject: `/CN=${hostname}` }); });
if (__version_num >= 80023) {
    EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri3, { certSubject: `/CN=${hostname}` }); });
}

//@<> FR17 test if rescan with addInstances errors out
EXPECT_THROWS(function() {
    cluster.rescan({addInstances: [`${hostname}:${__mysql_sandbox_port2}`]});
}, "Can't automatically add unrecognized members to the cluster when memberAuthType is 'CERT_SUBJECT_PASSWORD'.");
EXPECT_OUTPUT_CONTAINS("Unrecognized members were detected in the group, but the cluster is configured to require SSL certificate authentication. Please stop GR on those members and then add them to the cluster using cluster.addInstance() with the appropriate authentication options.");

WIPE_OUTPUT();

EXPECT_THROWS(function() {
    cluster.rescan({addInstances: "auto"});
}, "Can't automatically add unrecognized members to the cluster when memberAuthType is 'CERT_SUBJECT_PASSWORD'.");
EXPECT_OUTPUT_CONTAINS("Unrecognized members were detected in the group, but the cluster is configured to require SSL certificate authentication. Please stop GR on those members and then add them to the cluster using cluster.addInstance() with the appropriate authentication options.");

// with prompt
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name=?", [`${hostname}:${__mysql_sandbox_port2}`]);

WIPE_OUTPUT();

shell.options.useWizards=1;

EXPECT_NO_THROWS(function() { cluster.rescan(); });
EXPECT_OUTPUT_CONTAINS("New instances were discovered in the cluster but ignore because the cluster requires SSL certificate authentication.");
EXPECT_OUTPUT_CONTAINS("Please stop GR on those members and then add them to the cluster using cluster.addInstance() with the appropriate authentication options.");

shell.options.useWizards=0;

status = cluster.status();
EXPECT_NE(status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["instanceErrors"].length, 0);
EXPECT_EQ(status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["instanceErrors"][0], "WARNING: Instance is not managed by InnoDB cluster. Use cluster.rescan() to repair.");

//@<> teste some APIs
check_apis("VERIFY_IDENTITY", "PASSWORD");
check_apis("REQUIRED", "CERT_ISSUER");
check_apis("VERIFY_CA", "CERT_SUBJECT_PASSWORD");
check_apis("VERIFY_IDENTITY", "CERT_SUBJECT_PASSWORD");

reset_instances();

//@<> FR1.2 cannot mix either 'certIssuer', 'certSubject' or 'memberAuthType' (with value other than "password") with 'adoptFromGR'
shell.connect(__sandbox_uri1);

EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", { gtidSetIsComplete: 1, memberAuthType: "CERT_SUBJECT_PASSWORD", certIssuer: "/CN=Test_CA", certSubject: `/CN=${hostname}` }); });
EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2, { certSubject: `/CN=${hostname}` }); });

dba.dropMetadataSchema({force: true});

EXPECT_THROWS(function() {
    dba.createCluster("cluster", { certIssuer: "foo", certSubject: "bar", adoptFromGR: true });
}, "Cannot use the options 'certIssuer' or 'certSubject' if 'adoptFromGR' is set to true.");
EXPECT_THROWS(function() {
    dba.createCluster("cluster", { certSubject: "bar", adoptFromGR: true });
}, "Cannot use the options 'certIssuer' or 'certSubject' if 'adoptFromGR' is set to true.");
EXPECT_THROWS(function() {
    dba.createCluster("cluster", { certIssuer: "foo", certSubject: "bar", adoptFromGR: true });
}, "Cannot use the options 'certIssuer' or 'certSubject' if 'adoptFromGR' is set to true.");

EXPECT_THROWS(function() {
    dba.createCluster("cluster", { memberAuthType: "CERT_ISSUER_PASSWORD", adoptFromGR: true });
}, "Cannot set 'memberAuthType' to a value other than 'PASSWORD' if 'adoptFromGR' is set to true.");

var cluster;
EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", { gtidSetIsComplete: 1, memberAuthType: "PASSWORD", adoptFromGR: true }); });
EXPECT_EQ("PASSWORD", session.runSql("SELECT attributes->>'$.opt_memberAuthType' FROM mysql_innodb_cluster_metadata.clusters WHERE (cluster_name = 'cluster')").fetchOne()[0]);

EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri3, { certSubject: `/CN=${hostname}` }); });

// cleanup cluster
cluster.dissolve();

//@<> FR9 create cluster with different memberSslMode
shell.connect(__sandbox_uri1);

var cluster;
EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", { gtidSetIsComplete: 1, memberSslMode: "VERIFY_IDENTITY" }); });
EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2); });
if (__version_num >= 80023) {
    EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri3); });
}

options = cluster.options();
check_option(options.defaultReplicaSet.globalOptions, "memberSslMode", "VERIFY_IDENTITY");

// cleanup cluster
cluster.dissolve();

//@<> check if status recognizes mix of different client versions (no certSubject)
shell.connect(__sandbox_uri1);

var cluster;

EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", { gtidSetIsComplete: 1, memberAuthType: "CERT_SUBJECT_PASSWORD", certIssuer: "/CN=Test_CA", certSubject: `/CN=${hostname}` }); });
EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2, { certSubject: `/CN=${hostname}` }); });
if (__version_num >= 80023) {
    EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri3, { certSubject: `/CN=${hostname}` }); });
}

session.runSql(`UPDATE mysql_innodb_cluster_metadata.instances SET attributes = json_remove(attributes, '$.opt_certSubject') WHERE (address = '${hostname}:${__mysql_sandbox_port2}');`);
session.runSql(`UPDATE mysql_innodb_cluster_metadata.instances SET attributes = json_remove(attributes, '$.opt_certSubject') WHERE (address = '${hostname}:${__mysql_sandbox_port3}');`);

var status = cluster.status();

check_instance_error(status, false, "WARNING: memberAuthType is set to 'CERT_SUBJECT_PASSWORD' but there's no 'certSubject' configured for this instance, which prevents all other instances from reaching it, compromising the Cluster. Please remove the instance from the Cluster and use the most recent version of the shell to re-add it back.");

//@<> FR12 check exception if SSL settings are incorrect

if (__version_num >= 80023) {
    ca3_path = testutil.sslCreateCa("myca3", "/CN=Test_CA3");
    cert3_ca3_path = testutil.sslCreateCert("server3", "myca3", `/CN=${hostname}`, __mysql_sandbox_port3);
    update_conf(__mysql_sandbox_port3, ca3_path, cert3_ca3_path);
}

ca2_path = testutil.sslCreateCa("myca2", "/CN=Test_CA2");
cert2_ca2_path = testutil.sslCreateCert("server2", "myca2", `/CN=${hostname}`, __mysql_sandbox_port2);
update_conf(__mysql_sandbox_port2, ca2_path, cert2_ca2_path);

reset_instances();

EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", { gtidSetIsComplete: 1, memberAuthType: "CERT_SUBJECT", certIssuer: "/CN=Test_CA", certSubject: `/CN=${hostname}` }); });

EXPECT_THROWS(function() {cluster.addInstance(__sandbox_uri2, { certSubject: `/CN=${hostname}` });}, "Authentication error during connection check");
EXPECT_OUTPUT_CONTAINS("* Checking connectivity and SSL configuration...");
EXPECT_OUTPUT_CONTAINS(`ERROR: Authentication error during connection check. Please ensure that the SSL certificate (@@ssl_cert) configured for '${hostname}:${__mysql_sandbox_port2}' was issued by '/CN=Test_CA' (certIssuer), is recognized at '${hostname}:${__mysql_sandbox_port2}' and that the value specified for certIssuer itself is correct.`);

if (__version_num >= 80023) {
    EXPECT_THROWS(function() {cluster.addReplicaInstance(__sandbox_uri3, { certSubject: `/CN=${hostname}` });}, "Authentication error during connection check");
    EXPECT_OUTPUT_CONTAINS("* Checking connectivity and SSL configuration...");
    EXPECT_OUTPUT_CONTAINS(`ERROR: Authentication error during connection check. Please ensure that the SSL certificate (@@ssl_cert) configured for '${hostname}:${__mysql_sandbox_port3}' was issued by '/CN=Test_CA' (certIssuer), is recognized at '${hostname}:${__mysql_sandbox_port3}' and that the value specified for certIssuer itself is correct.`);
}

//@<> check if status recognizes mix of different client versions (remove ssl_ variables) {!__dbug_off}
if (__version_num >= 80023) {
    update_conf(__mysql_sandbox_port3, ca_path, cert3_path);
}
update_conf(__mysql_sandbox_port2, ca_path, cert2_path);

reset_instances();

var cluster;

EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", { gtidSetIsComplete: 1, memberAuthType: "CERT_SUBJECT_PASSWORD", certIssuer: "/CN=Test_CA", certSubject: `/CN=${hostname}` }); });
EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2, { certSubject: `/CN=${hostname}` }); });
if (__version_num >= 80023) {
    EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri3, { certSubject: `/CN=${hostname}` }); });
}

testutil.dbugSet("+d,fail_recovery_user_status_check_subject");
var status = cluster.status();
testutil.dbugSet("");

status
check_instance_error(status, true, "WARNING: memberAuthType is set to 'CERT_SUBJECT_PASSWORD' but there's no 'certIssuer' and/or 'certSubject' configured for this instance recovery user, which prevents all other instances from reaching it, compromising the Cluster. Please remove the instance from the Cluster and use the most recent version of the shell to re-add it back.");

// just issuer

cluster.dissolve();
EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", { gtidSetIsComplete: 1, memberAuthType: "CERT_ISSUER", certIssuer: "/CN=Test_CA" }); });
EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2); });
if (__version_num >= 80023) {
    EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri3); });
}

testutil.dbugSet("+d,fail_recovery_user_status_check_issuer");
var status = cluster.status();
testutil.dbugSet("");

status
check_instance_error(status, true, "WARNING: memberAuthType is set to 'CERT_ISSUER' but there's no 'certIssuer' configured for this instance recovery user, which prevents all other instances from reaching it, compromising the Cluster. Please remove the instance from the Cluster and use the most recent version of the shell to re-add it back.");

// cleanup cluster
cluster.dissolve();

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
