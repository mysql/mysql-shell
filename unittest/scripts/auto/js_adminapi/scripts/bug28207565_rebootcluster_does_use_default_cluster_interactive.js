//@ Deploy sandboxes
testutil.deploySandbox(__mysql_sandbox_port1, "root", {loose_group_replication_exit_state_action: "READ_ONLY"});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {loose_group_replication_exit_state_action: "READ_ONLY"});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {loose_group_replication_exit_state_action: "READ_ONLY"});

//@ Deploy cluster
shell.connect(__sandbox_uri1);
var c = dba.createCluster("myCluster", {memberSslMode: 'DISABLED'});
c.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
c.addInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<OUT> Check cluster status before reboot
c.status();

//@ Kill all cluster members
c.disconnect();
testutil.killSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");
testutil.killSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "UNREACHABLE");
session.close();
testutil.killSandbox(__mysql_sandbox_port1);

//@ Start the members again
testutil.startSandbox(__mysql_sandbox_port2);
testutil.waitForDelayedGRStart(__mysql_sandbox_port2, 'root', 0);
testutil.startSandbox(__mysql_sandbox_port1);
testutil.startSandbox(__mysql_sandbox_port3);
testutil.waitForDelayedGRStart(__mysql_sandbox_port1, 'root', 0);
testutil.waitForDelayedGRStart(__mysql_sandbox_port3, 'root', 0);

// Reboot cluster from complete outage, not specifying the name

shell.connect(__sandbox_uri1);
testutil.expectPrompt("Would you like to rejoin it to the cluster? [y/N]: ", "y");
testutil.expectPrompt("Would you like to rejoin it to the cluster? [y/N]: ", "y");

//@ Reboot cluster from complete outage, not specifying the name
c = dba.rebootClusterFromCompleteOutage();
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<OUT> Check cluster status after reboot
c.status();

//@ Finalization
c.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
