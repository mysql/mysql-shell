//@<> Setup

// create a common CA
ca_path = testutil.sslCreateCa("myca", "/CN=Test_CA");

// needed to create sandbox folders
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname, server_id:11});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname, server_id:22});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname, server_id:33});

// create server certificates
cert1_path = testutil.sslCreateCert("server", "myca", `/CN=${hostname}/L=machine1`, __mysql_sandbox_port1);
cert2_path = testutil.sslCreateCert("server", "myca", `/CN=${hostname}/L=machine2`, __mysql_sandbox_port2);
cert3_path = testutil.sslCreateCert("server", "myca", `/CN=${hostname}/L=machine3`, __mysql_sandbox_port3);

function update_conf(port, ca_path, cert_path) {
    testutil.changeSandboxConf(port, "ssl_ca", ca_path);
    testutil.changeSandboxConf(port, "ssl_cert", cert_path);
    testutil.changeSandboxConf(port, "ssl_key", cert_path.replace("-cert.pem", "-key.pem"));
    testutil.restartSandbox(port);
}

function test_add_instance(commStack, recoverMethod) {

    if (recoverMethod.toLowerCase() == "clone" && (__version_num < 80017)) {
        return;
    }
    if (commStack.toLowerCase() == "mysql" && (__version_num < 80027)){
        return;
    }

    shell.connect(__sandbox_uri1);

    var cluster;
    if ((commStack.toLowerCase() == "xcom") && (__version_num < 80027)) {
        EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", { memberAuthType: "CERT_SUBJECT_PASSWORD", certIssuer: "/CN=Test_CA", certSubject: `/CN=${hostname}/L=machine1` }); });
    } else {
        EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", { communicationStack: commStack, memberAuthType: "CERT_SUBJECT_PASSWORD", certIssuer: "/CN=Test_CA", certSubject: `/CN=${hostname}/L=machine1` }); });
    }

    EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2, { recoveryMethod: recoverMethod, certSubject: `/CN=${hostname}/L=machine2` }); });

    testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

    if (commStack.toLowerCase() == "xcom" && (__version_num >= 80027)) {

        testutil.stopGroup([__mysql_sandbox_port1, __mysql_sandbox_port2]);
        EXPECT_NO_THROWS(function(){ cluster = dba.rebootClusterFromCompleteOutage("cluster", {switchCommunicationStack: "mysql"}); });

        EXPECT_NE("", session.runSql("SELECT ssl_type FROM mysql.user WHERE (user LIKE 'mysql_innodb_cluster_%');").fetchOne()[0]);
        EXPECT_NE("", session.runSql("SELECT authentication_string FROM mysql.user WHERE (user LIKE 'mysql_innodb_cluster_%');").fetchOne()[0]);
    }

    cluster.dissolve();
}

//restart the servers with proper SSL support
update_conf(__mysql_sandbox_port1, ca_path, cert1_path);
update_conf(__mysql_sandbox_port2, ca_path, cert2_path);
update_conf(__mysql_sandbox_port3, ca_path, cert3_path);

//@<> create cluster comm stack mysql and no clone {VER(>= 8.0.27)}
test_add_instance("mysql", "incremental");

//@<> create cluster comm stack mysql and clone {VER(>= 8.0.27)}
test_add_instance("mysql", "clone");

//@<> create cluster comm stack xcom and no clone
test_add_instance("xcom", "incremental");

//@<> create cluster comm stack xcom and clone {VER(>= 8.0.17)}
test_add_instance("xcom", "clone");

//@<> mid cleanup
shell.connect(__sandbox_uri3);
reset_instance(session);
shell.connect(__sandbox_uri2);
reset_instance(session);
shell.connect(__sandbox_uri1);
reset_instance(session);

//@<> create cluster comm stack xcom with primary offline
shell.connect(__sandbox_uri1);

var cluster;
if (__version_num < 80027) {
    EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", { memberAuthType: "CERT_SUBJECT_PASSWORD", certIssuer: "/CN=Test_CA", certSubject: `/CN=${hostname}/L=machine1` }); });
} else {
    EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", { communicationStack: "xcom", memberAuthType: "CERT_SUBJECT_PASSWORD", certIssuer: "/CN=Test_CA", certSubject: `/CN=${hostname}/L=machine1` }); });
}
EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2, { recoveryMethod: "incremental", certSubject: `/CN=${hostname}/L=machine2` }); });
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

session.runSql("STOP GROUP_REPLICATION;")

shell.connect(__sandbox_uri2);

cluster = dba.getCluster();

EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri3, { recoveryMethod: "incremental", certSubject: `/CN=${hostname}/L=machine3` }); });
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

testutil.stopGroup([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3]);
shell.connect(__sandbox_uri3);
reset_instance(session);
shell.connect(__sandbox_uri2);
reset_instance(session);
shell.connect(__sandbox_uri1);
reset_instance(session);

//@<> create cluster comm stack mysql with primary offline {VER(>= 8.0.27)}
shell.connect(__sandbox_uri1);

var cluster;
EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", { communicationStack: "mysql", memberAuthType: "CERT_SUBJECT_PASSWORD", certIssuer: "/CN=Test_CA", certSubject: `/CN=${hostname}/L=machine1` }); });
EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2, { recoveryMethod: "incremental", certSubject: `/CN=${hostname}/L=machine2` }); });
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

session.runSql("STOP GROUP_REPLICATION;")

shell.connect(__sandbox_uri2);

cluster = dba.getCluster();

EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri3, { recoveryMethod: "incremental", certSubject: `/CN=${hostname}/L=machine3` }); });
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

testutil.stopGroup([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3]);
shell.connect(__sandbox_uri3);
reset_instance(session);
shell.connect(__sandbox_uri2);
reset_instance(session);
shell.connect(__sandbox_uri1);
reset_instance(session);

//@<> create cluster comm stack mysql with primary offline with clone {VER(>= 8.0.27)}
shell.connect(__sandbox_uri1);

var cluster;
EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", { communicationStack: "mysql", memberAuthType: "CERT_SUBJECT_PASSWORD", certIssuer: "/CN=Test_CA", certSubject: `/CN=${hostname}/L=machine1` }); });
EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2, { recoveryMethod: "clone", certSubject: `/CN=${hostname}/L=machine2` }); });
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

session.runSql("STOP GROUP_REPLICATION;")

shell.connect(__sandbox_uri2);

cluster = dba.getCluster();

EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri3, { recoveryMethod: "clone", certSubject: `/CN=${hostname}/L=machine3` }); });
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

testutil.stopGroup([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3]);
shell.connect(__sandbox_uri3);
reset_instance(session);
shell.connect(__sandbox_uri2);
reset_instance(session);
shell.connect(__sandbox_uri1);
reset_instance(session);

//@<> create cluster comm stack mysql and clone and check if accounts aren't recreated {VER(>= 8.0.17)}
shell.connect(__sandbox_uri1);

var cluster;
EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", { communicationStack: "xcom", memberAuthType: "CERT_SUBJECT", certIssuer: "/CN=Test_CA", certSubject: `/CN=${hostname}/L=machine1` }); });
EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2, { recoveryMethod: "clone", certSubject: `/CN=${hostname}/L=machine2` }); });
EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri3, { recoveryMethod: "clone", certSubject: `/CN=${hostname}/L=machine3` }); });

testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

EXPECT_SHELL_LOG_CONTAINS("Restoring recovery accounts with certificates only.");

testutil.stopGroup([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3]);
shell.connect(__sandbox_uri3);
reset_instance(session);
shell.connect(__sandbox_uri2);
reset_instance(session);
shell.connect(__sandbox_uri1);
reset_instance(session);

//@<> Make sure that variables are reverted in case the command fails (createCluster) {VER(>= 8.0.17)}

shell.connect(__sandbox_uri1);

shell.options["dba.connectivityChecks"] = false;

set_sysvar(session, "auto_increment_increment", 56);
set_sysvar(session, "auto_increment_offset", 77);
set_sysvar(session, "group_replication_ssl_mode", "DISABLED");
set_sysvar(session, "group_replication_recovery_use_ssl", "OFF");
old_group_replication_group_name = get_sysvar(session, "group_replication_group_name");
old_group_replication_communication_stack = get_sysvar(session, "group_replication_communication_stack");

EXPECT_THROWS(function() {
    dba.createCluster("cluster", { memberAuthType: "CERT_SUBJECT", certIssuer: "/CN=foo", certSubject: `/CN=bar` });
}, "The server is not configured properly to be an active member of the group. Please see more details on error log.");

EXPECT_SHELL_LOG_CONTAINS("createCluster() failed: Trying to revert changes...");

EXPECT_EQ(56, get_sysvar(session, "auto_increment_increment"));
EXPECT_EQ(77, get_sysvar(session, "auto_increment_offset"));
EXPECT_EQ("DISABLED", get_sysvar(session, "group_replication_ssl_mode"));
EXPECT_EQ(0, get_sysvar(session, "group_replication_recovery_use_ssl"));
EXPECT_EQ(old_group_replication_group_name, get_sysvar(session, "group_replication_group_name"));
EXPECT_EQ(old_group_replication_communication_stack, get_sysvar(session, "group_replication_communication_stack"));

reset_instance(session);

//@<> Make sure that variables are reverted in case the command fails (addInstance) {VER(>= 8.0.17)}

shell.connect(__sandbox_uri1);

var cluster;
EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", { memberAuthType: "CERT_SUBJECT", certIssuer: "/CN=Test_CA", certSubject: `/CN=${hostname}/L=machine1` }); });

shell.options["dba.connectivityChecks"] = false;

shell.connect(__sandbox_uri2);

set_sysvar(session, "auto_increment_increment", 57);
set_sysvar(session, "auto_increment_offset", 78);
set_sysvar(session, "group_replication_ssl_mode", "DISABLED");
set_sysvar(session, "group_replication_recovery_use_ssl", "OFF");
old_group_replication_group_name = get_sysvar(session, "group_replication_group_name");
old_group_replication_communication_stack = get_sysvar(session, "group_replication_communication_stack");

EXPECT_THROWS(function() {
    cluster.addInstance(__sandbox_uri2, { recoveryMethod: "clone", certSubject: `/CN=foo` });
}, "The server is not configured properly to be an active member of the group. Please see more details on error log.");

EXPECT_EQ(57, get_sysvar(session, "auto_increment_increment"));
EXPECT_EQ(78, get_sysvar(session, "auto_increment_offset"));
EXPECT_EQ("DISABLED", get_sysvar(session, "group_replication_ssl_mode"));
EXPECT_EQ(0, get_sysvar(session, "group_replication_recovery_use_ssl"));
EXPECT_EQ(old_group_replication_group_name, get_sysvar(session, "group_replication_group_name"));
EXPECT_EQ(old_group_replication_communication_stack, get_sysvar(session, "group_replication_communication_stack"));

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
