// Regression for BUG#29265869: DBA.REBOOTCLUSTERFROMCOMPLETEOUTAGE OVERRIDES SOME OF THE PERSISTED GR SETTINGS
// - rebootClusterFromCompleteOutage() should not change any GR setting.

//@ BUG29265869 - Deploy sandboxes.
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
var expel_timeout = 1;
var exit_state = "ABORT_SERVER";
var consistency = "BEFORE_ON_PRIMARY_FAILOVER";
var local_address1 = hostname + ":" + (__mysql_sandbox_port3 * 10 + 1).toString();
var ip_white_list = hostname_ip;
var member_weight1 = 15;
var grp_name = "a1efe13d-20c3-11e9-9b77-3c6aa7197def";
var local_address2 = hostname + ":" + (__mysql_sandbox_port3).toString();
var member_weight2 = 75;
var uri1 = hostname + ":" + __mysql_sandbox_port1;
var uri2 = hostname + ":" + __mysql_sandbox_port2;

//@ BUG29265869 - Create cluster with custom GR settings. {VER(>=8.0.16)}
shell.connect(__sandbox_uri1);
var c = dba.createCluster("test", {expelTimeout: expel_timeout, exitStateAction: exit_state, failoverConsistency: consistency, localAddress: local_address1, ipWhitelist: ip_white_list, memberWeight: member_weight1, groupName: grp_name});
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");

//@ BUG29265869 - Create cluster with custom GR settings for 5.7. {VER(<8.0.0)}
shell.connect(__sandbox_uri1);
var c = dba.createCluster("test", {localAddress: local_address1, ipWhitelist: ip_white_list, groupName: grp_name});
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");

//@ BUG29265869 - Add instance with custom GR settings. {VER(>=8.0.16)}
c.addInstance(__sandbox_uri2, {exitStateAction: exit_state, localAddress: local_address2, ipWhitelist: ip_white_list, memberWeight: member_weight2});
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@ BUG29265869 - Add instance with custom GR settings for 5.7. {VER(<8.0.0)}
c.addInstance(__sandbox_uri2, {localAddress: local_address2, ipWhitelist: ip_white_list, memberWeight: member_weight2});
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@ BUG29265869 - Persist GR settings for 5.7. {VER(<8.0.0)}
var sandbox_cnf1 = testutil.getSandboxConfPath(__mysql_sandbox_port1);
dba.configureLocalInstance(__sandbox_uri1, {mycnfPath: sandbox_cnf1});
var sandbox_cnf2 = testutil.getSandboxConfPath(__mysql_sandbox_port2);
dba.configureLocalInstance(__sandbox_uri2, {mycnfPath: sandbox_cnf2});

//@<OUT> BUG29265869 - Show initial cluster options.
c.options();

//@ BUG29265869 - Kill all cluster members.
c.disconnect();
testutil.killSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "UNREACHABLE");
session.close();
testutil.killSandbox(__mysql_sandbox_port1);

//@ BUG29265869 - Start the members again.
testutil.startSandbox(__mysql_sandbox_port2);
testutil.waitForDelayedGRStart(__mysql_sandbox_port2, 'root', 0);
testutil.startSandbox(__mysql_sandbox_port1);
testutil.waitForDelayedGRStart(__mysql_sandbox_port1, 'root', 0);

//@ BUG29265869 - connect to instance.
shell.connect(__sandbox_uri1);

//@ BUG29265869 - Reboot cluster from complete outage.
var c = dba.rebootClusterFromCompleteOutage("test", {rejoinInstances: [uri2]});

// Waiting for the instances to become online
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<OUT> BUG29265869 - Show cluster options after reboot.
c.options();

//@ BUG29265869 - clean-up (destroy sandboxes).
c.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port1);
