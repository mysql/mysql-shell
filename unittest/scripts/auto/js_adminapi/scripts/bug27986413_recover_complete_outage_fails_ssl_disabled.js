//@<> Deploy cluster with ssl disabled
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3], {"memberSslMode": "DISABLED", gtidSetIsComplete: true});
var c = scene.cluster;

//@<> Reset gr_start_on_boot on all instances
disable_auto_rejoin(__mysql_sandbox_port1);
disable_auto_rejoin(__mysql_sandbox_port2);
disable_auto_rejoin(__mysql_sandbox_port3);

//@ Kill all cluster members
c.disconnect();
shell.connect(__sandbox_uri1);
testutil.killSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");
testutil.killSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "UNREACHABLE");
session.close();
testutil.killSandbox(__mysql_sandbox_port1);

//@ Start the members again
testutil.startSandbox(__mysql_sandbox_port1);
testutil.startSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port3);

//@ Reboot cluster from complete outage
shell.connect(__sandbox_uri1);
var uri2 = localhost + ":" + __mysql_sandbox_port2;
var uri3 = localhost + ":" + __mysql_sandbox_port3;
var uri1 = localhost + ":" + __mysql_sandbox_port1;
c = dba.rebootClusterFromCompleteOutage("cluster");
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");
session.close();

//@<> Check cluster status after reboot
var status = c.status();
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["status"])
EXPECT_EQ("PRIMARY", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["memberRole"])
EXPECT_EQ("SECONDARY", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["memberRole"])
EXPECT_EQ("SECONDARY", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["memberRole"])

//@<> Reset persisted gr_start_on_boot on all instances 2 {VER(>=8.0.11)}
disable_auto_rejoin(__mysql_sandbox_port1);
disable_auto_rejoin(__mysql_sandbox_port2);
disable_auto_rejoin(__mysql_sandbox_port3);

//@ Kill all cluster members again
c.disconnect();
shell.connect(__sandbox_uri1);
testutil.killSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");
testutil.killSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "UNREACHABLE");
session.close();
testutil.killSandbox(__mysql_sandbox_port1);

//@ Restart the members
testutil.startSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port1);
testutil.startSandbox(__mysql_sandbox_port3);

//@ Reboot cluster from complete outage using another member
shell.connect(__sandbox_uri2);
c = dba.rebootClusterFromCompleteOutage("cluster");
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<> Check cluster status after reboot 2
var status = c.status();
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["status"])
EXPECT_EQ("SECONDARY", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["memberRole"])
EXPECT_EQ("PRIMARY", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["memberRole"])
EXPECT_EQ("SECONDARY", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["memberRole"])

//@ Destroy sandboxes
c.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
