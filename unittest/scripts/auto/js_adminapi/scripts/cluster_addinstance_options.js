// WL#11032 AdminAPI: Configure member weight for automatic primary election on
// failover
//
// In MySQL 8.0.2 and 5.7.20, Group Replication introduces an option to control
// the outcome of the primary election algorithm in single-primary mode. With
// this option, the user can influence the primary member election by providing
// a member weight value for each member node. That weight value is used for
// electing the primary member instead of the member uuid which is the default
// method used for the election.
//
// In order to support defining such option, the AdminAPI was extended by
// introducing a new optional parameter, named 'memberWeight', in the
// following functions:
//
// - dba.createCluster()
// - Cluster.addInstance()
//

//@ WL#11032: Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});

shell.connect(__sandbox_uri1);

// Test the option on the addInstance() command
//@ WL#11032: Create cluster 1 {VER(>=5.7.20)}
var c = dba.createCluster('test', {clearReadOnly: true, gtidSetIsComplete: true})

//@ WL#11032: addInstance() errors using memberWeight option {VER(>=5.7.20)}
// F1.2 - The memberWeight option shall be an integer value.
c.addInstance(__sandbox_uri2, {memberWeight: ""});

c.addInstance(__sandbox_uri2, {memberWeight: true});

c.addInstance(__sandbox_uri2, {memberWeight: "AB"});

c.addInstance(__sandbox_uri2, {memberWeight: 10.5});

//@ WL#11032: Add instance using a valid value for memberWeight (25) {VER(>=5.7.20)}
c.addInstance(__sandbox_uri2, {memberWeight: 25});

//@ WL#11032: Dissolve cluster 1 {VER(>=5.7.20)}
c.dissolve({force: true});

// Verify if memberWeight is persisted on >= 8.0.11

// F2 - On a successful [dba.]createCluster() or [Cluster.]addInstance() call
// using the option memberWeight, the GR sysvar
// group_replication_member_weight must be persisted using SET PERSIST at
// the target instance, if the MySQL Server instance version is >= 8.0.11.

//@ WL#11032: Create cluster 2 {VER(>=8.0.11)}
var c = dba.createCluster('test', {clearReadOnly: true, groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc", memberWeight: 75, gtidSetIsComplete: true});

//@ WL#11032: Add instance using a valid value for memberWeight (-50) {VER(>=8.0.11)}
c.addInstance(__sandbox_uri2, {memberWeight: -50})

//@<> WL#11032: memberWeight must be persisted on mysql >= 8.0.11 {VER(>=8.0.11)}
EXPECT_EQ("0", get_sysvar(__mysql_sandbox_port2, "group_replication_member_weight", "PERSISTED"));

//@ WL#11032: Initialize new instance
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});

shell.connect(__sandbox_uri1);

//@ WL#11032: Create cluster 3 {VER(>=8.0.11)}
var c = dba.createCluster('test', {groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc", gtidSetIsComplete: true});

//@ WL#11032: Add instance without using memberWeight {VER(>=8.0.11)}
c.addInstance(__sandbox_uri2)

//@<> WL#11032: memberWeight must not be persisted on mysql >= 8.0.11 if not set {VER(>=8.0.11)}
EXPECT_EQ("", get_sysvar(__mysql_sandbox_port2, "group_replication_member_weight", "PERSISTED"));

//@ WL#11032: Finalization
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);


// WL#12049 AdminAPI: option to shutdown server when dropping out of the
// cluster
//
// In 8.0.12 and 5.7.24, Group Replication introduced an option to allow
// defining the behaviour of a cluster member whenever it drops the cluster
// unintentionally, i.e. not manually removed by the user (DBA). The
// behaviour defined can be either automatically shutting itself down or
// switching itself to super-read-only (current behaviour). The option is:
// group_replication_exit_state_action.
//
// In order to support defining such option, the AdminAPI was extended by
// introducing a new optional parameter, named 'exitStateAction', in the
// following functions:
//
// - dba.createCluster()
// - Cluster.addInstance()
//

//@ WL#12049: Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});

shell.connect(__sandbox_uri1);

// Test the option on the addInstance() command
//@ WL#12049: Create cluster 1 {VER(>=5.7.24)}
var c = dba.createCluster('test', {clearReadOnly: true, gtidSetIsComplete: true})

//@ WL#12049: addInstance() errors using exitStateAction option {VER(>=5.7.24)}
// F1.2 - The exitStateAction option shall be a string value.
// NOTE: GR validates the value, which is an Enumerator, and accepts the values
// `ABORT_SERVER` or `READ_ONLY`, or 1 or 0.
c.addInstance(__sandbox_uri2, {exitStateAction: ""});

c.addInstance(__sandbox_uri2, {exitStateAction: " "});

c.addInstance(__sandbox_uri2, {exitStateAction: ":"});

c.addInstance(__sandbox_uri2, {exitStateAction: "AB"});

c.addInstance(__sandbox_uri2, {exitStateAction: "10"});

//@ WL#12049: Add instance using a valid exitStateAction 1 {VER(>=5.7.24)}
c.addInstance(__sandbox_uri2, {exitStateAction: "ABORT_SERVER"});

//@ WL#12049: Dissolve cluster 1 {VER(>=5.7.24)}
c.dissolve({force: true});

// Verify if exitStateAction is persisted on >= 8.0.12

// F2 - On a successful [dba.]createCluster() or [Cluster.]addInstance() call
// using the option exitStateAction, the GR sysvar
// group_replication_exit_state_action must be persisted using SET PERSIST at
// the target instance, if the MySQL Server instance version is >= 8.0.12.

//@ WL#12049: Create cluster 2 {VER(>=8.0.12)}
var c = dba.createCluster('test', {clearReadOnly: true, groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc", exitStateAction: "READ_ONLY", gtidSetIsComplete: true});

//@ WL#12049: Add instance using a valid exitStateAction 2 {VER(>=8.0.12)}
c.addInstance(__sandbox_uri2, {exitStateAction: "READ_ONLY"})

session.close()
shell.connect(__sandbox_uri2);

//@<> WL#12049: exitStateAction must be persisted on mysql >= 8.0.12 {VER(>=8.0.12)}
EXPECT_EQ("READ_ONLY", get_sysvar(session,  "group_replication_exit_state_action", "PERSISTED"));

//@ WL#12049: Initialize new instance
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});

shell.connect(__sandbox_uri1);

//@ WL#12049: Create cluster 3 {VER(>=8.0.12)}
var c = dba.createCluster('test', {groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc", gtidSetIsComplete: true});

//@ WL#12049: Add instance without using exitStateAction {VER(>=8.0.12)}
c.addInstance(__sandbox_uri2)

//@<> BUG#28701263: DEFAULT VALUE OF EXITSTATEACTION TOO DRASTIC {VER(>=8.0.12)}
EXPECT_EQ("READ_ONLY", get_sysvar(session, "group_replication_exit_state_action"));

//@ WL#12049: Finalization
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

// WL#12067 AdminAPI: Define failover consistency
//
// In 8.0.14, Group Replication introduces an option to specify the failover
// guarantees (eventual or read_your_writes) when a primary failover happens in
// single-primary mode). The new option defines the behavior of a new fencing
// mechanism when a new primary is being promoted in a group. The fencing will
// restrict connections from writing and reading from the new primary until it
// has applied all the pending backlog of changes that came from the old
// primary (read_your_writes). Applications will not see time going backward for
// a short period of time (during the new primary promotion).

// In order to support defining such option, the AdminAPI was extended by
// introducing a new optional parameter, named 'consistency', in the
// dba.createCluster function.
//
//@ WL#12067: Initialization {VER(>=8.0.14)}
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
var s1 = mysql.getSession(__sandbox_uri1);
var s2 = mysql.getSession(__sandbox_uri2);

shell.connect(__sandbox_uri1);

// FR2 The value for group_replication_consistency must be the same on all cluster members that support the option
//@ WL#12067: TSF2_1 The value for group_replication_consistency must be the same on all cluster members (single-primary) {VER(>=8.0.14)}
var c = dba.createCluster('test', {consistency: "1", gtidSetIsComplete: true});
c.addInstance(__sandbox_uri2);
session.close();

//@<> WL#12067: TSF2_1 Confirm group_replication_consistency value is the same on all cluster members (single-primary) {VER(>=8.0.14)}
EXPECT_EQ("BEFORE_ON_PRIMARY_FAILOVER", get_sysvar(__mysql_sandbox_port1, "group_replication_consistency"));
EXPECT_EQ("BEFORE_ON_PRIMARY_FAILOVER", get_sysvar(__mysql_sandbox_port2, "group_replication_consistency"));

//@<> WL#12067: TSF2_1 Confirm group_replication_consistency value was persisted (single-primary) {VER(>=8.0.14)}
EXPECT_EQ("BEFORE_ON_PRIMARY_FAILOVER", get_sysvar(__mysql_sandbox_port1, "group_replication_consistency", "PERSISTED"));
EXPECT_EQ("BEFORE_ON_PRIMARY_FAILOVER", get_sysvar(__mysql_sandbox_port2, "group_replication_consistency", "PERSISTED"));

// BUG#30394258: CLUSTER.ADD_INSTANCE: THE OPTION GROUP_REPLICATION_CONSISTENCY CANNOT BE USED
// Regression test to ensure that when a consistency level of "BEFORE", "AFTER" or
// "BEFORE_AND_AFTER" was set in the cluster any instance will be successfully added

//@<> BUG#30394258: prepare {VER(>=8.0.14)}
 c.setOption("consistency", "BEFORE");

//@<> BUG#30394258: add instance to the cluster {VER(>=8.0.14)}
c.addInstance(__sandbox_uri3);

//@ WL#12067: Dissolve cluster 1{VER(>=8.0.14)}
c.dissolve({force: true});

// FR2 The value for group_replication_consistency must be the same on all cluster members that support the option
//@ WL#12067: TSF2_2 The value for group_replication_consistency must be the same on all cluster members (multi-primary) {VER(>=8.0.14)}
shell.connect(__sandbox_uri1);
var c = dba.createCluster('test', { clearReadOnly:true, consistency: "1", multiPrimary:true, force: true, gtidSetIsComplete: true});
c.addInstance(__sandbox_uri2);
session.close();

//@<> WL#12067: TSF2_2 Confirm group_replication_consistency value is the same on all cluster members (multi-primary) {VER(>=8.0.14)}
EXPECT_EQ("BEFORE_ON_PRIMARY_FAILOVER", get_sysvar(__mysql_sandbox_port1, "group_replication_consistency"));
EXPECT_EQ("BEFORE_ON_PRIMARY_FAILOVER", get_sysvar(__mysql_sandbox_port2, "group_replication_consistency"));

//@<> WL#12067: TSF2_2 Confirm group_replication_consistency value was persisted (multi-primary) {VER(>=8.0.14)}
EXPECT_EQ("BEFORE_ON_PRIMARY_FAILOVER", get_sysvar(__mysql_sandbox_port1, "group_replication_consistency", "PERSISTED"));
EXPECT_EQ("BEFORE_ON_PRIMARY_FAILOVER", get_sysvar(__mysql_sandbox_port2, "group_replication_consistency", "PERSISTED"));

//@ WL#12067: Finalization {VER(>=8.0.14)}
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);

// WL#12050 AdminAPI: Define the timeout for evicting previously active cluster members
//
// In 8.0.13, Group Replication introduced an option to allow defining
// the timeout for evicting previously active nodes.  In order to support
// defining such option, the AdminAPI was extended by introducing a new
// optional parameter, named 'expelTimeout', in the dba.createCluster()
// function.

//@ WL#12050: Initialization {VER(>=8.0.13)}
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
var s1 = mysql.getSession(__sandbox_uri1);
var s2 = mysql.getSession(__sandbox_uri2);

shell.connect(__sandbox_uri1);

// FR2 The value for group_replication_member_expel_timeout must be the same on all cluster members that support the option
//@ WL#12050: TSF2_1 The value for group_replication_member_expel_timeout must be the same on all cluster members (single-primary) {VER(>=8.0.13)}
var c = dba.createCluster('test', {expelTimeout: 100, gtidSetIsComplete: true});
c.addInstance(__sandbox_uri2);
session.close();

//@<> WL#12050: TSF2_1 Confirm group_replication_member_expel_timeout value is the same on all cluster members (single-primary) {VER(>=8.0.13)}
EXPECT_EQ(100, get_sysvar(__mysql_sandbox_port1, "group_replication_member_expel_timeout"));
EXPECT_EQ(100, get_sysvar(__mysql_sandbox_port2, "group_replication_member_expel_timeout"));

//@<> WL#12050: TSF2_1 Confirm group_replication_member_expel_timeout value was persisted (single-primary) {VER(>=8.0.13)}
EXPECT_EQ("100", get_sysvar(__mysql_sandbox_port1, "group_replication_member_expel_timeout", "PERSISTED"));
EXPECT_EQ("100", get_sysvar(__mysql_sandbox_port2, "group_replication_member_expel_timeout", "PERSISTED"));

//@ WL#12050: Dissolve cluster 1 {VER(>=8.0.13)}
c.dissolve({force: true});

// FR2 The value for group_replication_member_expel_timeout must be the same on all cluster members that support the option
//@ WL#12050: TSF2_2 The value for group_replication_member_expel_timeout must be the same on all cluster members (multi-primary) {VER(>=8.0.13)}
shell.connect(__sandbox_uri1);
var c = dba.createCluster('test', { clearReadOnly:true, expelTimeout: 200, multiPrimary:true, force: true, gtidSetIsComplete: true});
c.addInstance(__sandbox_uri2);
session.close();

//@<> WL#12050: TSF2_2 Confirm group_replication_member_expel_timeout value is the same on all cluster members (multi-primary) {VER(>=8.0.13)}
EXPECT_EQ(200, get_sysvar(__mysql_sandbox_port1, "group_replication_member_expel_timeout"));
EXPECT_EQ(200, get_sysvar(__mysql_sandbox_port2, "group_replication_member_expel_timeout"));

//@<> WL#12050: TSF2_2 Confirm group_replication_member_expel_timeout value was persisted (multi-primary) {VER(>=8.0.13)}
EXPECT_EQ("200", get_sysvar(__mysql_sandbox_port1, "group_replication_member_expel_timeout", "PERSISTED"));
EXPECT_EQ("200", get_sysvar(__mysql_sandbox_port2, "group_replication_member_expel_timeout", "PERSISTED"));

//@ WL#12050: Finalization {VER(>=8.0.13)}
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);


// WL#12066 AdminAPI: option to define the number of auto-rejoins
//
// In 8.0.16, Group Replication introduced an option to allow setting the
// number of auto-rejoin attempts an instance will do after being
// expelled from the group.  In order to support defining such option,
// the AdminAPI was extended by introducing a new optional parameter,
// named 'autoRejoinTries', in the dba.createCluster(),
// cluster.addInstance, cluster.setOption and cluster.setInstanceOption
// functions.

//@ WL#12066: Initialization {VER(>=8.0.16)}
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
var s1 = mysql.getSession(__sandbox_uri1);
var s2 = mysql.getSession(__sandbox_uri2);
var s3 = mysql.getSession(__sandbox_uri3);
shell.connect(__sandbox_uri1);
var c = dba.createCluster('test', {autoRejoinTries: 2, gtidSetIsComplete: true});

// FR1 - A new option autoRejoinTries shall be added to the [dba.]createCluster() and [cluster.]addInstance() to allow users to set the Group Replication (GR) system variable group_replication_autorejoin_tries.
//@ WL#12066: TSF1_4 Validate that an exception is thrown if the value specified is not an unsigned integer. {VER(>=8.0.16)}
c.addInstance(__sandbox_uri2, {autoRejoinTries: -5});

//@ WL#12066: TSF1_5 Validate that an exception is thrown if the value  is not in the range 0 to 2016. {VER(>=8.0.16)}
c.addInstance(__sandbox_uri2, {autoRejoinTries: 2017});

//@ WL#12066: TSF1_1, TSF6_1 Validate that the functions [dba.]createCluster() and [cluster.]addInstance() support a new option named autoRejoinTries. {VER(>=8.0.16)}
c.addInstance(__sandbox_uri2, {autoRejoinTries: 2016});
c.addInstance(__sandbox_uri3);
session.close();

//@<> WL#12066: TSF1_3, TSF1_6 Validate that when calling the functions [dba.]createCluster() and [cluster.]addInstance(), the GR variable group_replication_autorejoin_tries is persisted with the value given by the user on the target instance.{VER(>=8.0.16)}
EXPECT_EQ(2, get_sysvar(__mysql_sandbox_port1, "group_replication_autorejoin_tries"));
EXPECT_EQ(2016, get_sysvar(__mysql_sandbox_port2, "group_replication_autorejoin_tries"));
EXPECT_EQ(__default_gr_auto_rejoin_tries, get_sysvar(__mysql_sandbox_port3, "group_replication_autorejoin_tries"));

//@<> WL#12066: TSF1_3, TSF1_6 Confirm group_replication_autorejoin_tries value was persisted {VER(>=8.0.16)}
EXPECT_EQ("2", get_sysvar(__mysql_sandbox_port1, "group_replication_autorejoin_tries", "PERSISTED"));
EXPECT_EQ("2016", get_sysvar(__mysql_sandbox_port2, "group_replication_autorejoin_tries", "PERSISTED"));
EXPECT_EQ("", get_sysvar(__mysql_sandbox_port3, "group_replication_autorejoin_tries", "PERSISTED"));

//@ WL#12066: Finalization {VER(>=8.0.16)}
// NOTE: re-use the cluster for the next test. Remove only instances 2 and 3
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);