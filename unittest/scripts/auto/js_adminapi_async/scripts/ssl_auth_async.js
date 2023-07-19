//@ {VER(>=8.0.11)}

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

function check_apis(sslMode, authType) {

    var session1 = mysql.getSession(__sandbox_uri1);
    var session2 = mysql.getSession(__sandbox_uri2);

    reset_instance(session1);
    reset_instance(session2);

    shell.connect(__sandbox_uri1);

    var rset;
    if (authType.includes("_SUBJECT")) {
        EXPECT_NO_THROWS(function(){ rset = dba.createReplicaSet("rset", { gtidSetIsComplete: 1, replicationSslMode: sslMode, memberAuthType: authType, certIssuer: "/CN=Test_CA", certSubject: `/CN=${hostname}` }); });
        EXPECT_NO_THROWS(function(){ rset.addInstance(__sandbox_uri2, { certSubject: `/CN=${hostname}` }); });
    }
    else if (authType.includes("_ISSUER")) {
        EXPECT_NO_THROWS(function(){ rset = dba.createReplicaSet("rset", { gtidSetIsComplete: 1, replicationSslMode: sslMode, memberAuthType: authType, certIssuer: "/CN=Test_CA" }); });
        EXPECT_NO_THROWS(function(){ rset.addInstance(__sandbox_uri2); });
    }
    else {
        EXPECT_NO_THROWS(function(){ rset = dba.createReplicaSet("rset", { gtidSetIsComplete: 1, replicationSslMode: sslMode, memberAuthType: authType }); });
        EXPECT_NO_THROWS(function(){ rset.addInstance(__sandbox_uri2); });
    }

    options = rset.options();
    check_option(options.replicaSet.globalOptions, "replicationSslMode", sslMode);

    // rejoin
    session2.runSql("STOP SLAVE SQL_THREAD");
    EXPECT_NO_THROWS(function(){ rset.rejoinInstance(__sandbox_uri2); });

    // setPrimaryInstance
    EXPECT_NO_THROWS(function(){ rset.setPrimaryInstance(__sandbox_uri2); });
    EXPECT_NO_THROWS(function(){ rset.setPrimaryInstance(__sandbox_uri1); });

    // forcePrimaryInstance
    testutil.stopSandbox(__mysql_sandbox_port1);

    shell.connect(__sandbox_uri2);
    rset = dba.getReplicaSet();
    EXPECT_NO_THROWS(function(){ rset.forcePrimaryInstance(__sandbox_uri2); });

    testutil.startSandbox(__mysql_sandbox_port1);
    EXPECT_NO_THROWS(function(){ rset.rejoinInstance(__sandbox_uri1); });
    EXPECT_NO_THROWS(function(){ rset.setPrimaryInstance(__sandbox_uri1); });

    // removeInstance
    EXPECT_NO_THROWS(function(){ rset.removeInstance(__sandbox_uri2); });
}

//restart the servers with proper SSL support
update_conf(__mysql_sandbox_port1, ca_path, cert1_path);
update_conf(__mysql_sandbox_port2, ca_path, cert2_path);

var session1 = mysql.getSession(__sandbox_uri1);
var session2 = mysql.getSession(__sandbox_uri2);

//@<> FR2 create replica set (just password)
shell.connect(__sandbox_uri1);

EXPECT_THROWS(function() {
    dba.createReplicaSet("rset", { gtidSetIsComplete: 1, memberAuthType: "--" });
}, "Invalid value for 'memberAuthType' option. Supported values: PASSWORD, CERT_ISSUER, CERT_SUBJECT, CERT_ISSUER_PASSWORD, CERT_SUBJECT_PASSWORD");
EXPECT_THROWS(function() {
    dba.createReplicaSet("rset", { gtidSetIsComplete: 1, certIssuer: "" });
}, "Invalid value for 'certIssuer' option. Value cannot be an empty string.");
EXPECT_THROWS(function() {
    dba.createReplicaSet("rset", { gtidSetIsComplete: 1, certSubject: "" });
}, "Invalid value for 'certSubject' option. Value cannot be an empty string.");
EXPECT_THROWS(function() {
    dba.createReplicaSet("rset", { gtidSetIsComplete: 1, replicationSslMode: "DISABLED", memberAuthType: "CERT_ISSUER" });
}, `Instance '${hostname}:${__mysql_sandbox_port1}' has the \"replicationSslMode\" option with the value 'DISABLED', which means that \"memberAuthType\" only accepts the value 'PASSWORD'.`);

var rset;
EXPECT_NO_THROWS(function() { rset = dba.createReplicaSet("rset", { gtidSetIsComplete: 1, memberAuthType: "PASSWORD" }); });

EXPECT_EQ("PASSWORD", session.runSql("SELECT attributes->>'$.opt_memberAuthType' FROM mysql_innodb_cluster_metadata.clusters WHERE (cluster_name = 'rset')").fetchOne()[0]);
EXPECT_EQ("", session.runSql("SELECT attributes->>'$.opt_certIssuer' FROM mysql_innodb_cluster_metadata.clusters WHERE (cluster_name = 'rset')").fetchOne()[0]);
EXPECT_EQ("", session.runSql("SELECT ssl_type FROM mysql.user WHERE (user LIKE 'mysql_innodb_rs_%');").fetchOne()[0]);
EXPECT_NE("", session.runSql("SELECT authentication_string FROM mysql.user WHERE (user LIKE 'mysql_innodb_rs_%');").fetchOne()[0]);

WIPE_STDOUT();
EXPECT_NO_THROWS(function() { rset.addInstance(__sandbox_uri2); });

// FR16
status = rset.status();
EXPECT_EQ(status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["replication"]["replicationSsl"], "TLS_AES_256_GCM_SHA384 TLSv1.3");
EXPECT_EQ(status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["replication"]["replicationSslMode"], "REQUIRED");

check_users(session, true, false, false);

// FR14
options = rset.options();
check_option(options.replicaSet.globalOptions, "replicationSslMode", "REQUIRED");
check_option(options.replicaSet.globalOptions, "memberAuthType", "PASSWORD");
check_option(options.replicaSet.globalOptions, "certIssuer", "");
check_option(options.replicaSet.topology[`${hostname}:${__mysql_sandbox_port1}`], "certSubject", "");
check_option(options.replicaSet.topology[`${hostname}:${__mysql_sandbox_port2}`], "certSubject", "");

// cleanup replica set
reset_instance(session1);
reset_instance(session2);

//@<> FR2 create replica set (just password) unnecessary issuer and subject
shell.connect(__sandbox_uri1);

var rset;
EXPECT_NO_THROWS(function() { rset = dba.createReplicaSet("rset", { gtidSetIsComplete: 1, memberAuthType: "PASSWORD", certIssuer: "/CN=fooIssuer", certSubject: "/CN=fooSubject" }); });
EXPECT_OUTPUT_CONTAINS("The replicaset's SSL mode is set to PASSWORD, which makes both the 'certIssuer' and 'certSubject' options not required. Both options will be ignored.");

EXPECT_EQ("PASSWORD", session.runSql("SELECT attributes->>'$.opt_memberAuthType' FROM mysql_innodb_cluster_metadata.clusters WHERE (cluster_name = 'rset')").fetchOne()[0]);
EXPECT_EQ("/CN=fooIssuer", session.runSql("SELECT attributes->>'$.opt_certIssuer' FROM mysql_innodb_cluster_metadata.clusters WHERE (cluster_name = 'rset')").fetchOne()[0]);
EXPECT_EQ("/CN=fooSubject", session.runSql(`SELECT attributes->>'$.opt_certSubject' FROM mysql_innodb_cluster_metadata.instances WHERE (address = '${hostname}:${__mysql_sandbox_port1}')`).fetchOne()[0]);
EXPECT_EQ("", session.runSql("SELECT ssl_type FROM mysql.user WHERE (user LIKE 'mysql_innodb_rs_%');").fetchOne()[0]);
EXPECT_NE("", session.runSql("SELECT authentication_string FROM mysql.user WHERE (user LIKE 'mysql_innodb_rs_%');").fetchOne()[0]);

WIPE_STDOUT();
EXPECT_NO_THROWS(function() { rset.addInstance(__sandbox_uri2, { certSubject: "/CN=bar" }); });

check_users(session, true, false, false);

options = rset.options();
check_option(options.replicaSet.globalOptions, "replicationSslMode", "REQUIRED");
check_option(options.replicaSet.globalOptions, "memberAuthType", "PASSWORD");
check_option(options.replicaSet.globalOptions, "certIssuer", "/CN=fooIssuer");
check_option(options.replicaSet.topology[`${hostname}:${__mysql_sandbox_port1}`], "certSubject", "/CN=fooSubject");
check_option(options.replicaSet.topology[`${hostname}:${__mysql_sandbox_port2}`], "certSubject", "/CN=bar");

// cleanup replica set
reset_instance(session1);
reset_instance(session2);

//@<> FR2 create replica set (issuer with no password)
shell.connect(__sandbox_uri1);

// FR2.1 cannot mix 'memberAuthType' (with value other than "password") with 'adoptFromAR'
EXPECT_THROWS(function() {
    dba.createReplicaSet("rset", { gtidSetIsComplete: 1, memberAuthType: "CERT_ISSUER", certIssuer: "/CN=Test_CA", adoptFromAR: true });
}, "Cannot set 'memberAuthType' to a value other than 'PASSWORD' if 'adoptFromAR' is set to true.");

var rset;
EXPECT_NO_THROWS(function() { rset = dba.createReplicaSet("rset", { gtidSetIsComplete: 1, memberAuthType: "CERT_ISSUER", certIssuer: "/CN=Test_CA" }); });

EXPECT_EQ("CERT_ISSUER", session.runSql("SELECT attributes->>'$.opt_memberAuthType' FROM mysql_innodb_cluster_metadata.clusters WHERE (cluster_name = 'rset')").fetchOne()[0]);
EXPECT_EQ("/CN=Test_CA", session.runSql("SELECT attributes->>'$.opt_certIssuer' FROM mysql_innodb_cluster_metadata.clusters WHERE (cluster_name = 'rset')").fetchOne()[0]);
EXPECT_NE("", session.runSql("SELECT ssl_type FROM mysql.user WHERE (user LIKE 'mysql_innodb_rs_%');").fetchOne()[0]);
EXPECT_EQ("", session.runSql("SELECT authentication_string FROM mysql.user WHERE (user LIKE 'mysql_innodb_rs_%');").fetchOne()[0]);

//@<> FR5 add instance (issuer with no password)
EXPECT_THROWS(function() {
    rset.addInstance(__sandbox_uri2, { certSubject: "" });
}, "Invalid value for 'certSubject' option. Value cannot be an empty string.");

shell.options.verbose=1
shell.options['dba.logSql']=1
EXPECT_NO_THROWS(function() { rset.addInstance(__sandbox_uri2); });
shell.options.verbose=0
EXPECT_OUTPUT_CONTAINS("* Checking connectivity and SSL configuration...");

// FR16
status = rset.status();
EXPECT_EQ(status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["replication"]["replicationSsl"], "TLS_AES_256_GCM_SHA384 TLSv1.3");
EXPECT_EQ(status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["replication"]["replicationSslMode"], "REQUIRED");

check_users(session, false, true, false);

// FR14
options = rset.options();
check_option(options.replicaSet.globalOptions, "replicationSslMode", "REQUIRED");
check_option(options.replicaSet.globalOptions, "memberAuthType", "CERT_ISSUER");
check_option(options.replicaSet.globalOptions, "certIssuer", "/CN=Test_CA");
check_option(options.replicaSet.topology[`${hostname}:${__mysql_sandbox_port1}`], "certSubject", "");
check_option(options.replicaSet.topology[`${hostname}:${__mysql_sandbox_port2}`], "certSubject", "");

// cleanup replica set
reset_instance(session1);
reset_instance(session2);
WIPE_STDERR()
WIPE_STDOUT()

//@<> FR2 create replica set (issuer with no password) unnecessary subject
shell.connect(__sandbox_uri1);

var rset;
EXPECT_NO_THROWS(function() { rset = dba.createReplicaSet("rset", { gtidSetIsComplete: 1, memberAuthType: "CERT_ISSUER", certIssuer: "/CN=Test_CA", certSubject: "/CN=foo" }); });
EXPECT_OUTPUT_CONTAINS("The replicaset's SSL mode is set to CERT_ISSUER, which makes the 'certSubject' option not required. The option will be ignored.");

EXPECT_EQ("CERT_ISSUER", session.runSql("SELECT attributes->>'$.opt_memberAuthType' FROM mysql_innodb_cluster_metadata.clusters WHERE (cluster_name = 'rset')").fetchOne()[0]);
EXPECT_EQ("/CN=Test_CA", session.runSql("SELECT attributes->>'$.opt_certIssuer' FROM mysql_innodb_cluster_metadata.clusters WHERE (cluster_name = 'rset')").fetchOne()[0]);
EXPECT_EQ("/CN=foo", session.runSql(`SELECT attributes->>'$.opt_certSubject' FROM mysql_innodb_cluster_metadata.instances WHERE (address = '${hostname}:${__mysql_sandbox_port1}')`).fetchOne()[0]);
EXPECT_NE("", session.runSql("SELECT ssl_type FROM mysql.user WHERE (user LIKE 'mysql_innodb_rs_%');").fetchOne()[0]);
EXPECT_EQ("", session.runSql("SELECT authentication_string FROM mysql.user WHERE (user LIKE 'mysql_innodb_rs_%');").fetchOne()[0]);

WIPE_STDOUT();
EXPECT_NO_THROWS(function() { rset.addInstance(__sandbox_uri2, { certSubject: "/CN=bar" }); });
EXPECT_OUTPUT_CONTAINS("* Checking connectivity and SSL configuration...");

status = rset.status();
EXPECT_EQ(status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["replication"]["replicationSsl"], "TLS_AES_256_GCM_SHA384 TLSv1.3");
EXPECT_EQ(status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["replication"]["replicationSslMode"], "REQUIRED");

check_users(session, false, true, false);

options = rset.options();
check_option(options.replicaSet.globalOptions, "replicationSslMode", "REQUIRED");
check_option(options.replicaSet.globalOptions, "memberAuthType", "CERT_ISSUER");
check_option(options.replicaSet.globalOptions, "certIssuer", "/CN=Test_CA");
check_option(options.replicaSet.topology[`${hostname}:${__mysql_sandbox_port1}`], "certSubject", "/CN=foo");
check_option(options.replicaSet.topology[`${hostname}:${__mysql_sandbox_port2}`], "certSubject", "/CN=bar");

// cleanup replica set
reset_instance(session1);
reset_instance(session2);

//@<> FR2 create replica set (issuer with no password) with clone
shell.connect(__sandbox_uri1);

var rset;
EXPECT_NO_THROWS(function() { rset = dba.createReplicaSet("rset", { gtidSetIsComplete: 1, memberAuthType: "CERT_ISSUER", certIssuer: "/CN=Test_CA" }); });

EXPECT_NO_THROWS(function() { rset.addInstance(__sandbox_uri2, { recoveryMethod: "clone" }); });

// cleanup replica set
reset_instance(session1);
var session2 = mysql.getSession(__sandbox_uri2);
reset_instance(session2);

//@<> FR2 create replica set (issuer)
shell.connect(__sandbox_uri1);

// FR2.1 cannot mix 'memberAuthType' (with value other than "password") with 'adoptFromAR'
EXPECT_THROWS(function() {
    dba.createReplicaSet("rset", { gtidSetIsComplete: 1, memberAuthType: "CERT_ISSUER_PASSWORD", certIssuer: "/CN=Test_CA", adoptFromAR: true });
}, "Cannot set 'memberAuthType' to a value other than 'PASSWORD' if 'adoptFromAR' is set to true.");

var rset;
EXPECT_NO_THROWS(function() { rset = dba.createReplicaSet("rset", { gtidSetIsComplete: 1, memberAuthType: "CERT_ISSUER_PASSWORD", certIssuer: "/CN=Test_CA" }); });

EXPECT_EQ("CERT_ISSUER_PASSWORD", session.runSql("SELECT attributes->>'$.opt_memberAuthType' FROM mysql_innodb_cluster_metadata.clusters WHERE (cluster_name = 'rset')").fetchOne()[0]);
EXPECT_EQ("/CN=Test_CA", session.runSql("SELECT attributes->>'$.opt_certIssuer' FROM mysql_innodb_cluster_metadata.clusters WHERE (cluster_name = 'rset')").fetchOne()[0]);
EXPECT_NE("", session.runSql("SELECT ssl_type FROM mysql.user WHERE (user LIKE 'mysql_innodb_rs_%');").fetchOne()[0]);
EXPECT_NE("", session.runSql("SELECT authentication_string FROM mysql.user WHERE (user LIKE 'mysql_innodb_rs_%');").fetchOne()[0]);

//@<> FR5 add instance (issuer)
EXPECT_NO_THROWS(function() { rset.addInstance(__sandbox_uri2); });

EXPECT_OUTPUT_CONTAINS("* Checking connectivity and SSL configuration...");

// FR16
status = rset.status();
EXPECT_EQ(status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["replication"]["replicationSsl"], "TLS_AES_256_GCM_SHA384 TLSv1.3");
EXPECT_EQ(status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["replication"]["replicationSslMode"], "REQUIRED");

check_users(session, true, true, false);

// FR14
options = rset.options();
check_option(options.replicaSet.globalOptions, "replicationSslMode", "REQUIRED");
check_option(options.replicaSet.globalOptions, "memberAuthType", "CERT_ISSUER_PASSWORD");
check_option(options.replicaSet.globalOptions, "certIssuer", "/CN=Test_CA");
check_option(options.replicaSet.topology[`${hostname}:${__mysql_sandbox_port1}`], "certSubject", "");
check_option(options.replicaSet.topology[`${hostname}:${__mysql_sandbox_port2}`], "certSubject", "");

// cleanup replica set
reset_instance(session1);
reset_instance(session2);

//@<> FR2 create replica set (issuer + subject)
shell.connect(__sandbox_uri1);

// FR2.1 cannot mix 'memberAuthType' (with value other than "password") with 'adoptFromAR'
EXPECT_THROWS(function() {
    dba.createReplicaSet("rset", { gtidSetIsComplete: 1, memberAuthType: "CERT_SUBJECT_PASSWORD", certIssuer: "/CN=Test_CA", certSubject: `/CN=${hostname}`, adoptFromAR: true });
}, "Cannot set 'memberAuthType' to a value other than 'PASSWORD' if 'adoptFromAR' is set to true.");

var rset;
EXPECT_NO_THROWS(function() { rset = dba.createReplicaSet("rset", { gtidSetIsComplete: 1, memberAuthType: "CERT_SUBJECT_PASSWORD", certIssuer: "/CN=Test_CA", certSubject: `/CN=${hostname}` }); });

EXPECT_EQ("CERT_SUBJECT_PASSWORD", session.runSql("SELECT attributes->>'$.opt_memberAuthType' FROM mysql_innodb_cluster_metadata.clusters WHERE (cluster_name = 'rset')").fetchOne()[0]);
EXPECT_EQ("/CN=Test_CA", session.runSql("SELECT attributes->>'$.opt_certIssuer' FROM mysql_innodb_cluster_metadata.clusters WHERE (cluster_name = 'rset')").fetchOne()[0]);
EXPECT_EQ(`/CN=${hostname}`, session.runSql(`SELECT attributes->>'$.opt_certSubject' FROM mysql_innodb_cluster_metadata.instances WHERE (address = '${hostname}:${__mysql_sandbox_port1}')`).fetchOne()[0]);
EXPECT_NE("", session.runSql("SELECT ssl_type FROM mysql.user WHERE (user LIKE 'mysql_innodb_rs_%');").fetchOne()[0]);
EXPECT_NE("", session.runSql("SELECT authentication_string FROM mysql.user WHERE (user LIKE 'mysql_innodb_rs_%');").fetchOne()[0]);

//@<> FR5 add instance (issuer + subject)
EXPECT_THROWS(function() {
    rset.addInstance(__sandbox_uri2);
}, "The replicaset's SSL mode is set to CERT_SUBJECT_PASSWORD but the 'certSubject' option for the instance wasn't supplied.");

EXPECT_NO_THROWS(function() { rset.addInstance(__sandbox_uri2, { certSubject: `/CN=${hostname}` }); });

EXPECT_OUTPUT_CONTAINS("* Checking connectivity and SSL configuration...");

// FR16
status = rset.status();
EXPECT_EQ(status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["replication"]["replicationSsl"], "TLS_AES_256_GCM_SHA384 TLSv1.3");
EXPECT_EQ(status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["replication"]["replicationSslMode"], "REQUIRED");

check_users(session, true, true, true);

// FR14
options = rset.options();
check_option(options.replicaSet.globalOptions, "replicationSslMode", "REQUIRED");
check_option(options.replicaSet.globalOptions, "memberAuthType", "CERT_SUBJECT_PASSWORD");
check_option(options.replicaSet.globalOptions, "certIssuer", "/CN=Test_CA");
check_option(options.replicaSet.topology[`${hostname}:${__mysql_sandbox_port1}`], "certSubject", `/CN=${hostname}`);
check_option(options.replicaSet.topology[`${hostname}:${__mysql_sandbox_port2}`], "certSubject", `/CN=${hostname}`);

// cleanup replica set
reset_instance(session1);
reset_instance(session2);

//@<> FR2 create replica set (issuer + subject) with clone
shell.connect(__sandbox_uri1);

var rset;
EXPECT_NO_THROWS(function(){ rset = dba.createReplicaSet("rset", { gtidSetIsComplete: 1, memberAuthType: "CERT_SUBJECT_PASSWORD", certIssuer: "/CN=Test_CA", certSubject: `/CN=${hostname}` }); });
EXPECT_NO_THROWS(function(){ rset.addInstance(__sandbox_uri2, { certSubject: `/CN=${hostname}`, recoveryMethod: "clone" }); });

var session1 = mysql.getSession(__sandbox_uri1);
var session2 = mysql.getSession(__sandbox_uri2);

reset_instance(session1);
reset_instance(session2);

//@<> FR2/FR11 create replica set (replicationSslMode)
shell.connect(__sandbox_uri1);

EXPECT_THROWS(function() {
    rset = dba.createReplicaSet("rset", { gtidSetIsComplete: 1, replicationSslMode: 12 });
}, "Option 'replicationSslMode' is expected to be of type String, but is Integer");

EXPECT_THROWS(function() {
    rset = dba.createReplicaSet("rset", { gtidSetIsComplete: 1, replicationSslMode: "fff" });
}, "Invalid value for 'replicationSslMode' option. Supported values: DISABLED, REQUIRED, VERIFY_CA, VERIFY_IDENTITY, AUTO.");

var rset;
EXPECT_NO_THROWS(function() { rset = dba.createReplicaSet("rset", { gtidSetIsComplete: 1, replicationSslMode: "DISABLED" }); });
EXPECT_EQ("DISABLED", session.runSql("SELECT attributes->>'$.opt_replicationSslMode' FROM mysql_innodb_cluster_metadata.clusters WHERE (cluster_name = 'rset')").fetchOne()[0]);
EXPECT_NO_THROWS(function() { rset.addInstance(__sandbox_uri2); });

options = rset.options();
check_option(options.replicaSet.globalOptions, "replicationSslMode", "DISABLED");

// FR16
status = rset.status();
EXPECT_EQ(status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["replication"]["replicationSsl"], null);
EXPECT_EQ(status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["replication"]["replicationSslMode"], "DISABLED");

reset_instance(session1);
reset_instance(session2);

EXPECT_NO_THROWS(function() { rset = dba.createReplicaSet("rset", { gtidSetIsComplete: 1, replicationSslMode: "REQUIRED" }); });
EXPECT_EQ("REQUIRED", session.runSql("SELECT attributes->>'$.opt_replicationSslMode' FROM mysql_innodb_cluster_metadata.clusters WHERE (cluster_name = 'rset')").fetchOne()[0]);
EXPECT_NO_THROWS(function() { rset.addInstance(__sandbox_uri2); });

options = rset.options();
check_option(options.replicaSet.globalOptions, "replicationSslMode", "REQUIRED");

status = rset.status();
EXPECT_CONTAINS("TLS_AES_", status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["replication"]["replicationSsl"]);
EXPECT_EQ("REQUIRED", status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["replication"]["replicationSslMode"]);

reset_instance(session1);
reset_instance(session2);

EXPECT_NO_THROWS(function() { rset = dba.createReplicaSet("rset", { gtidSetIsComplete: 1, replicationSslMode: "AUTO" }); });
EXPECT_EQ("REQUIRED", session.runSql("SELECT attributes->>'$.opt_replicationSslMode' FROM mysql_innodb_cluster_metadata.clusters WHERE (cluster_name = 'rset')").fetchOne()[0]);
EXPECT_NO_THROWS(function() { rset.addInstance(__sandbox_uri2); });

options = rset.options();
check_option(options.replicaSet.globalOptions, "replicationSslMode", "REQUIRED");

status = rset.status();
EXPECT_CONTAINS("TLS_AES_", status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["replication"]["replicationSsl"]);
EXPECT_EQ("REQUIRED", status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["replication"]["replicationSslMode"]);

reset_instance(session1);
reset_instance(session2);

EXPECT_NO_THROWS(function() { rset = dba.createReplicaSet("rset", { gtidSetIsComplete: 1, replicationSslMode: "VERIFY_CA" }); });
EXPECT_EQ("VERIFY_CA", session.runSql("SELECT attributes->>'$.opt_replicationSslMode' FROM mysql_innodb_cluster_metadata.clusters WHERE (cluster_name = 'rset')").fetchOne()[0]);
EXPECT_NO_THROWS(function() { rset.addInstance(__sandbox_uri2); });

options = rset.options();
check_option(options.replicaSet.globalOptions, "replicationSslMode", "VERIFY_CA");

status = rset.status();
EXPECT_CONTAINS("TLS_AES_", status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["replication"]["replicationSsl"]);
EXPECT_EQ("VERIFY_CA", status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["replication"]["replicationSslMode"]);

reset_instance(session1);
reset_instance(session2);

EXPECT_NO_THROWS(function() { rset = dba.createReplicaSet("rset", { gtidSetIsComplete: 1, replicationSslMode: "VERIFY_IDENTITY" }); });
EXPECT_EQ("VERIFY_IDENTITY", session.runSql("SELECT attributes->>'$.opt_replicationSslMode' FROM mysql_innodb_cluster_metadata.clusters WHERE (cluster_name = 'rset')").fetchOne()[0]);
EXPECT_NO_THROWS(function() { rset.addInstance(__sandbox_uri2); });

options = rset.options();
check_option(options.replicaSet.globalOptions, "replicationSslMode", "VERIFY_IDENTITY");

status = rset.status();
EXPECT_CONTAINS("TLS_AES_", status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["replication"]["replicationSsl"]);
EXPECT_EQ("VERIFY_IDENTITY", status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["replication"]["replicationSslMode"]);

//@<> teste some APIs

check_apis("VERIFY_IDENTITY", "PASSWORD");
check_apis("REQUIRED", "CERT_ISSUER");
check_apis("VERIFY_CA", "CERT_SUBJECT_PASSWORD");
check_apis("VERIFY_IDENTITY", "CERT_SUBJECT_PASSWORD");

var session1 = mysql.getSession(__sandbox_uri1);
var session2 = mysql.getSession(__sandbox_uri2);

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

shell.connect(__sandbox_uri1);

EXPECT_NO_THROWS(function() { rset = dba.createReplicaSet("rset", { gtidSetIsComplete: 1, memberAuthType: "CERT_SUBJECT", certIssuer: "/CN=Test_CA", certSubject: `/CN=${hostname}` }); });

EXPECT_THROWS(function() {rset.addInstance(__sandbox_uri2, { certSubject: `/CN=${hostname}` });}, "Authentication error during connection check");
EXPECT_OUTPUT_CONTAINS("* Checking connectivity and SSL configuration...");
EXPECT_OUTPUT_CONTAINS(`ERROR: Authentication error during connection check. Please ensure that the SSL certificate (@@ssl_cert) configured for '${hostname}:${__mysql_sandbox_port2}' was issued by '/CN=Test_CA' (certIssuer), is recognized at '${hostname}:${__mysql_sandbox_port2}' and that the value specified for certIssuer itself is correct.`);

//@<> Cleanup
session1.close();
session2.close();

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
