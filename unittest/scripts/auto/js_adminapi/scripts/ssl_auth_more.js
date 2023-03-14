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

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
