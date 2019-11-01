// {VER(>8.0.5)}
// Test various things in a cluster while an instance is autorejoining
// Not a thorough test because this would require a kill/start of a member
// for every test (to try being reliable), which would be very slow.

//@<> Setup

testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.deploySandbox(__mysql_sandbox_port3, "root");

shell.connect(__sandbox_uri1);

var cluster = dba.createCluster("clus", {gtidSetIsComplete:1});
cluster.addInstance(__sandbox_uri2);


session1 = mysql.getSession(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);
session3 = mysql.getSession(__sandbox_uri3);

//@ rebootCluster on an auto-rejoining instance {VER(>=8.0.0)}
// Bug #30501590 REBOOTCLUSTER(), STATUS() ETC FAIL IF TARGET IS AUTO-REJOINING
// rebootCluster() used to fail if the instance being rebooted is auto-rejoining
// A restart of the only member of the cluster would cause it to be stuck
// trying to auto-rejoin until timeout. Trying to reboot during that would fail
// and be confusing, wasting precious minutes during an outage. It's also not
// always clear the error is temporary.
// Another problem is that reboot was doing the busy XCOM port check, which will
// obviously be busy in an auto-rejoining member and abort the reboot.

// force complete outage
testutil.stopSandbox(__mysql_sandbox_port1);
session2.runSql("stop group_replication");

// restart the target member, so that it gets stuck trying to auto-rejoin
testutil.startSandbox(__mysql_sandbox_port1);

shell.connect(__sandbox_uri1);
cluster = dba.rebootClusterFromCompleteOutage();

//@<> complete the 3 member cluster {VER(>=8.0.0)}
cluster.rejoinInstance(__sandbox_uri2);

cluster.addInstance(__sandbox_uri3);

//@ status while the target is autorejoining (should pass)

testutil.killSandbox(__mysql_sandbox_port1);
testutil.startSandbox(__mysql_sandbox_port1);

// At this point, the primary is offline, but the other members including
// sandbox2 which is being queried haven't noticed that yet. Normally
// getCluster() would redirect to the primary, but since the primary is down,
// it should stay connected to sandbox2, even if status() still thinks
// sandbox1 is ONLINE and PRIMARY.

shell.connect(__sandbox_uri2);
cluster = dba.getCluster();

EXPECT_NO_THROWS(function(){status = cluster.status();});
println(status);

EXPECT_EQ("127.0.0.1:"+__mysql_sandbox_port2, status.groupInformationSourceMember);


//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
