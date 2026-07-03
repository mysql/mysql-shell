//@ {DEF(MYSQLD_SECONDARY_SERVER_A) && VER(>=8.0.27) && testutil.versionCheck(MYSQLD_SECONDARY_SERVER_A.version, "between", "[8.0.27,9.0.0)") && __dbug}

//@<> INCLUDE clusterset_utils.inc

//@<> Setup
var pwdAdmin = "C0mPL1CAT3D_pa22w0rd_adm1n";
var native_password_status_warning =
    "uses the 'mysql_native_password' authentication plugin";
var native_password_status_warning_expected =
    testutil.versionCheck(MYSQLD_SECONDARY_SERVER_A.version, ">=", "8.0.34");

function expect_native_password_status_warnings(value, api_class) {
    var value_json = JSON.stringify(value) || "";
    if (native_password_status_warning_expected) {
        EXPECT_CONTAINS(native_password_status_warning, value_json);
        EXPECT_CONTAINS(
            api_class + ".resetReplicationAccountsPassword({recreate: true})",
            value_json);
    } else {
        EXPECT_NOT_CONTAINS(native_password_status_warning, value_json);
    }
}

testutil.deployRawSandbox(__mysql_sandbox_port1, __secure_password,
    {report_host: hostname},
    {mysqldPath: MYSQLD_SECONDARY_SERVER_A.path});
testutil.deploySandbox(__mysql_sandbox_port2, __secure_password,
    {report_host: hostname},
    {mysqldPath: MYSQLD_SECONDARY_SERVER_A.path});
testutil.deploySandbox(__mysql_sandbox_port3, __secure_password,
    {report_host: hostname},
    {mysqldPath: MYSQLD_SECONDARY_SERVER_A.path});
testutil.deploySandbox(__mysql_sandbox_port4, __secure_password,
    {report_host: hostname},
    {mysqldPath: MYSQLD_SECONDARY_SERVER_A.path});
testutil.deploySandbox(__mysql_sandbox_port5, __secure_password,
    {report_host: hostname},
    {mysqldPath: MYSQLD_SECONDARY_SERVER_A.path});

// All instances in this test use the secondary binary (<9.0).
testutil.changeSandboxConf(__mysql_sandbox_port1, "loose_mysql_native_password", "ON");
testutil.changeSandboxConf(__mysql_sandbox_port2, "loose_mysql_native_password", "ON");
testutil.changeSandboxConf(__mysql_sandbox_port3, "loose_mysql_native_password", "ON");
testutil.changeSandboxConf(__mysql_sandbox_port4, "loose_mysql_native_password", "ON");
testutil.changeSandboxConf(__mysql_sandbox_port5, "loose_mysql_native_password", "ON");

EXPECT_NO_THROWS(function() {
    dba.configureInstance(__sandbox_uri_secure_password1,
        {clusterAdmin: "admin", clusterAdminPassword: pwdAdmin});
});
EXPECT_NO_THROWS(function() {
    dba.configureInstance(__sandbox_uri_secure_password2,
        {clusterAdmin: "admin", clusterAdminPassword: pwdAdmin});
});
EXPECT_NO_THROWS(function() {
    dba.configureInstance(__sandbox_uri_secure_password3,
        {clusterAdmin: "admin", clusterAdminPassword: pwdAdmin});
});
EXPECT_NO_THROWS(function() {
    dba.configureReplicaSetInstance(__sandbox_uri_secure_password4,
        {clusterAdmin: "admin", clusterAdminPassword: pwdAdmin});
});
EXPECT_NO_THROWS(function() {
    dba.configureReplicaSetInstance(__sandbox_uri_secure_password5,
        {clusterAdmin: "admin", clusterAdminPassword: pwdAdmin});
});

var __sandbox_uri1 = `mysql://admin:${pwdAdmin}@localhost:${__mysql_sandbox_port1}`;
var __sandbox_uri2 = `mysql://admin:${pwdAdmin}@localhost:${__mysql_sandbox_port2}`;
var __sandbox_uri3 = `mysql://admin:${pwdAdmin}@localhost:${__mysql_sandbox_port3}`;
var __sandbox_uri4 = `mysql://admin:${pwdAdmin}@localhost:${__mysql_sandbox_port4}`;
var __sandbox_uri5 = `mysql://admin:${pwdAdmin}@localhost:${__mysql_sandbox_port5}`;

testutil.restartSandbox(__mysql_sandbox_port1);
testutil.restartSandbox(__mysql_sandbox_port2);
testutil.restartSandbox(__mysql_sandbox_port3);
testutil.restartSandbox(__mysql_sandbox_port4);
testutil.restartSandbox(__mysql_sandbox_port5);

//@<> Create Cluster + ClusterSet + ReplicaCluster
shell.connect(__sandbox_uri1);
testutil.dbugSet("+d,dba_create_user_force_mysql_native_password");
var cluster = dba.createCluster("cluster", {"ipAllowlist":"127.0.0.1," + hostname_ip, "communicationStack": "XCOM"});

var status = cluster.status({extended: 1});
expect_native_password_status_warnings(
    status["defaultReplicaSet"]["topology"][__endpoint1]["instanceErrors"],
    "Cluster");

var cs = cluster.createClusterSet("clusterset");

var rc = cs.createReplicaCluster(__sandbox_uri2, "replicacluster",
    {recoveryMethod: "incremental", "ipAllowlist":"127.0.0.1," + hostname_ip, "communicationStack": "XCOM"});
EXPECT_NO_THROWS(function() {
    cluster.addReplicaInstance(__sandbox_uri3,
        {recoveryMethod: "incremental"});
});
testutil.waitReadReplicaState(__mysql_sandbox_port3, "ONLINE");

CHECK_PRIMARY_CLUSTER([__sandbox_uri1], cluster);
CHECK_REPLICA_CLUSTER([__sandbox_uri2], cluster, rc, undefined, __secure_password);

//@<> Create ReplicaSet
shell.connect(__sandbox_uri4);
var rs = dba.createReplicaSet("replicaset");
EXPECT_NO_THROWS(function() {
    rs.addInstance(__sandbox_uri5, {recoveryMethod: "incremental"});
});

status = rs.status({extended: 1});
expect_native_password_status_warnings(
    status["replicaSet"]["topology"][__endpoint4]["instanceErrors"],
    "ReplicaSet");
expect_native_password_status_warnings(
    status["replicaSet"]["topology"][__endpoint5]["instanceErrors"],
    "ReplicaSet");

testutil.dbugSet("");

//@<> Validate migration from mysql_native_password to caching_sha2_password
shell.connect(__sandbox_uri1);

var native_pwd_accounts = session.runSql(
  "SELECT COUNT(*) FROM mysql.user " +
  "WHERE user LIKE 'mysql_innodb_c%' AND plugin='mysql_native_password'"
).fetchOne()[0];

var all_internal_accounts = session.runSql(
  "SELECT COUNT(*) FROM mysql.user " +
  "WHERE user LIKE 'mysql_innodb_c%'"
).fetchOne()[0];

// All accounts are using mysql_native_password
EXPECT_EQ(4, all_internal_accounts);
EXPECT_EQ(all_internal_accounts, native_pwd_accounts);
status = cluster.status({extended: 1});
expect_native_password_status_warnings(
    status["defaultReplicaSet"]["topology"][__endpoint1]["instanceErrors"],
    "ClusterSet");
expect_native_password_status_warnings(
    status["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint3]["instanceErrors"],
    "ClusterSet");

// Do not warn about a metadata account that is not used by the Read Replica.
var member_account = session.runSql(
    "SELECT attributes->>'$.recoveryAccountUser', " +
    "attributes->>'$.recoveryAccountHost' " +
    "FROM mysql_innodb_cluster_metadata.instances " +
    "WHERE mysql_server_uuid = @@server_uuid").fetchOne();
var read_replica_account = session.runSql(
    "SELECT attributes->>'$.readReplicaReplicationAccountUser', " +
    "attributes->>'$.readReplicaReplicationAccountHost' " +
    "FROM mysql_innodb_cluster_metadata.instances WHERE address = ?",
    [__endpoint3]).fetchOne();
session.runSql(
    "UPDATE mysql_innodb_cluster_metadata.instances SET attributes = " +
    "JSON_SET(attributes, '$.readReplicaReplicationAccountUser', ?, " +
    "'$.readReplicaReplicationAccountHost', ?) WHERE address = ?",
    [member_account[0], member_account[1], __endpoint3]);
status = cluster.status({extended: 1});
EXPECT_NOT_CONTAINS(native_password_status_warning, JSON.stringify(
    status["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint3]["instanceErrors"]) || "");
session.runSql(
    "UPDATE mysql_innodb_cluster_metadata.instances SET attributes = " +
    "JSON_SET(attributes, '$.readReplicaReplicationAccountUser', ?, " +
    "'$.readReplicaReplicationAccountHost', ?) WHERE address = ?",
    [read_replica_account[0], read_replica_account[1], __endpoint3]);

status = rc.status({extended: 1});
expect_native_password_status_warnings(
    status["defaultReplicaSet"]["topology"][__endpoint2]["instanceErrors"],
    "ClusterSet");

status = cs.status({extended: 1});
expect_native_password_status_warnings(
    status["clusters"]["cluster"]["clusterErrors"], "ClusterSet");
expect_native_password_status_warnings(
    status["clusters"]["replicacluster"]["clusterErrors"], "ClusterSet");

// Do not warn about a metadata account that is not used by a Replica Cluster.
var primary_cluster_account = session.runSql(
    "SELECT attributes->>'$.replicationAccountUser', " +
    "attributes->>'$.replicationAccountHost' " +
    "FROM mysql_innodb_cluster_metadata.clusters WHERE cluster_name = 'cluster'").fetchOne();
var replica_cluster_account = session.runSql(
    "SELECT attributes->>'$.replicationAccountUser', " +
    "attributes->>'$.replicationAccountHost' " +
    "FROM mysql_innodb_cluster_metadata.clusters " +
    "WHERE cluster_name = 'replicacluster'").fetchOne();
session.runSql(
    "UPDATE mysql_innodb_cluster_metadata.clusters SET attributes = " +
    "JSON_SET(attributes, '$.replicationAccountUser', ?, " +
    "'$.replicationAccountHost', ?) WHERE cluster_name = 'replicacluster'",
    [primary_cluster_account[0], primary_cluster_account[1]]);
status = cs.status({extended: 1});
EXPECT_NOT_CONTAINS(native_password_status_warning, JSON.stringify(
    status["clusters"]["replicacluster"]["clusterErrors"]) || "");
session.runSql(
    "UPDATE mysql_innodb_cluster_metadata.clusters SET attributes = " +
    "JSON_SET(attributes, '$.replicationAccountUser', ?, " +
    "'$.replicationAccountHost', ?) WHERE cluster_name = 'replicacluster'",
    [replica_cluster_account[0], replica_cluster_account[1]]);

EXPECT_NO_THROWS(function() { cs.resetReplicationAccountsPassword({recreate:true}); });
EXPECT_NO_THROWS(function() { rs.resetReplicationAccountsPassword({recreate:true}); });

shell.connect(__sandbox_uri1);

var native_pwd_accounts = session.runSql(
  "SELECT COUNT(*) FROM mysql.user " +
  "WHERE user LIKE 'mysql_innodb_c%' AND plugin='mysql_native_password'"
).fetchOne()[0];

var caching_sha2_accounts = session.runSql(
  "SELECT COUNT(*) FROM mysql.user " +
  "WHERE user LIKE 'mysql_innodb_c%' AND plugin='caching_sha2_password'"
).fetchOne()[0];

var all_internal_accounts = session.runSql(
  "SELECT COUNT(*) FROM mysql.user " +
  "WHERE user LIKE 'mysql_innodb_c%'"
).fetchOne()[0];

// All accounts are using caching_sha2_password
EXPECT_EQ(4, all_internal_accounts);
EXPECT_EQ(0, native_pwd_accounts);
EXPECT_EQ(4, caching_sha2_accounts);
EXPECT_EQ(0, session.runSql(
  "SELECT COUNT(*) FROM mysql.user " +
  "WHERE user LIKE 'mysql_innodb_replica_%' AND plugin='mysql_native_password'"
).fetchOne()[0]);
EXPECT_EQ(1, session.runSql(
  "SELECT COUNT(*) FROM mysql.user " +
  "WHERE user LIKE 'mysql_innodb_replica_%' AND plugin='caching_sha2_password'"
).fetchOne()[0]);
EXPECT_NOT_CONTAINS(native_password_status_warning,
    JSON.stringify(cluster.status({extended: 1})));
EXPECT_NOT_CONTAINS(native_password_status_warning,
    JSON.stringify(cs.status({extended: 1})));
EXPECT_NOT_CONTAINS(native_password_status_warning,
    JSON.stringify(rs.status({extended: 1})));

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
testutil.destroySandbox(__mysql_sandbox_port5);
