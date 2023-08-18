//@ {VER(>=8.0.11)}

//@<> Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});

//@<> Create replica set
shell.connect(__sandbox_uri1);
var rset = dba.createReplicaSet('rset', {gtidSetIsComplete: true});
var desc = undefined;

//@<> Manually change the name
session.runSql("UPDATE mysql_innodb_cluster_metadata.clusters SET cluster_name = \"newName\"");

rset = dba.getReplicaSet();
EXPECT_NO_THROWS(function() { desc = rset.describe(); });

desc;
EXPECT_EQ(2, Object.keys(desc).length);
EXPECT_TRUE(desc.hasOwnProperty('name'));
EXPECT_TRUE(desc.hasOwnProperty('topology'));

EXPECT_EQ("newName", desc["name"]);
EXPECT_EQ(1, desc["topology"].length);
EXPECT_EQ(`${hostname}:${__mysql_sandbox_port1}`, desc["topology"][0]["address"]);
EXPECT_EQ(`${hostname}:${__mysql_sandbox_port1}`, desc["topology"][0]["label"]);
EXPECT_EQ("PRIMARY", desc["topology"][0]["instanceRole"]);

//@<> Manually change back the name to the correct one
session.runSql("UPDATE mysql_innodb_cluster_metadata.clusters SET cluster_name = \"rset\"");

rset = dba.getReplicaSet();
EXPECT_NO_THROWS(function() { desc = rset.describe(); });

desc;
EXPECT_EQ(2, Object.keys(desc).length);
EXPECT_TRUE(desc.hasOwnProperty('name'));
EXPECT_TRUE(desc.hasOwnProperty('topology'));

EXPECT_EQ("rset", desc["name"]);
EXPECT_EQ(1, desc["topology"].length);
EXPECT_EQ(`${hostname}:${__mysql_sandbox_port1}`, desc["topology"][0]["address"]);
EXPECT_EQ(`${hostname}:${__mysql_sandbox_port1}`, desc["topology"][0]["label"]);
EXPECT_EQ("PRIMARY", desc["topology"][0]["instanceRole"]);

//@<> Add an instance to the replicaset
rset.addInstance(__sandbox_uri2, {label: "zzLabel"});
testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);

EXPECT_NO_THROWS(function() { desc = rset.describe(); });

desc;
EXPECT_EQ(2, Object.keys(desc).length);
EXPECT_TRUE(desc.hasOwnProperty('name'));
EXPECT_TRUE(desc.hasOwnProperty('topology'));

EXPECT_EQ("rset", desc["name"]);
EXPECT_EQ(2, desc["topology"].length);
EXPECT_EQ(`${hostname}:${__mysql_sandbox_port1}`, desc["topology"][0]["address"]);
EXPECT_EQ(`${hostname}:${__mysql_sandbox_port1}`, desc["topology"][0]["label"]);
EXPECT_EQ("PRIMARY", desc["topology"][0]["instanceRole"]);
EXPECT_EQ(`${hostname}:${__mysql_sandbox_port2}`, desc["topology"][1]["address"]);
EXPECT_EQ("zzLabel", desc["topology"][1]["label"]);
EXPECT_EQ("REPLICA", desc["topology"][1]["instanceRole"]);

//@<> Invalidate primary and check description
testutil.killSandbox(__mysql_sandbox_port1);

shell.connect(__sandbox_uri2);
rset = dba.getReplicaSet();
rset.forcePrimaryInstance(__sandbox_uri2);

EXPECT_NO_THROWS(function(){ desc = rset.describe(); });

desc;
EXPECT_EQ(2, Object.keys(desc).length);
EXPECT_TRUE(desc.hasOwnProperty('name'));
EXPECT_TRUE(desc.hasOwnProperty('topology'));

EXPECT_EQ("rset", desc["name"]);
EXPECT_EQ(2, desc["topology"].length);
EXPECT_EQ(`${hostname}:${__mysql_sandbox_port1}`, desc["topology"][0]["address"]);
EXPECT_EQ(`${hostname}:${__mysql_sandbox_port1}`, desc["topology"][0]["label"]);
EXPECT_EQ(null, desc["topology"][0]["instanceRole"]);
EXPECT_EQ(`${hostname}:${__mysql_sandbox_port2}`, desc["topology"][1]["address"]);
EXPECT_EQ("zzLabel", desc["topology"][1]["label"]);
EXPECT_EQ("PRIMARY", desc["topology"][1]["instanceRole"]);

//@<> Must work even if primary is down

//rejoin invalidated member
testutil.startSandbox(__mysql_sandbox_port1);
EXPECT_NO_THROWS(function(){ rset.rejoinInstance(__sandbox_uri1); });

testutil.killSandbox(__mysql_sandbox_port2);
shell.connect(__sandbox_uri1);

rset = dba.getReplicaSet();

WIPE_OUTPUT();
EXPECT_NO_THROWS(function(){ desc = rset.describe(); });

EXPECT_OUTPUT_CONTAINS("WARNING: Unable to connect to the PRIMARY of the ReplicaSet 'rset'");
EXPECT_OUTPUT_CONTAINS("ReplicaSet change operations will not be possible unless the PRIMARY can be reached.");

desc;
EXPECT_EQ(2, Object.keys(desc).length);
EXPECT_TRUE(desc.hasOwnProperty('name'));
EXPECT_TRUE(desc.hasOwnProperty('topology'));

EXPECT_EQ("rset", desc["name"]);
EXPECT_EQ(2, desc["topology"].length);
EXPECT_EQ(`${hostname}:${__mysql_sandbox_port1}`, desc["topology"][0]["address"]);
EXPECT_EQ(`${hostname}:${__mysql_sandbox_port1}`, desc["topology"][0]["label"]);
EXPECT_EQ("REPLICA", desc["topology"][0]["instanceRole"]);
EXPECT_EQ(`${hostname}:${__mysql_sandbox_port2}`, desc["topology"][1]["address"]);
EXPECT_EQ("zzLabel", desc["topology"][1]["label"]);
EXPECT_EQ("PRIMARY", desc["topology"][1]["instanceRole"]);

//@<> Finalization
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
