// Assumptions: smart deployment functions available

var number_of_rpl_users_query = "SELECT COUNT(*) FROM INFORMATION_SCHEMA.USER_PRIVILEGES WHERE GRANTEE REGEXP \"'mysql_innodb_cluster_r[0-9]{10}.*\"";

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

shell.connect(__sandbox_uri1);

//@ WL#12049: Unsupported server version {VER(<5.7.24)}
var c = dba.createCluster('test', {exitStateAction: "READ_ONLY"});

//@ WL#12049: Create cluster errors using exitStateAction option {VER(>=5.7.24)}
// F1.2 - The exitStateAction option shall be a string value.
// NOTE: GR validates the value, which is an Enumerator, and accepts the values
// `ABORT_SERVER` or `READ_ONLY`, or 1 or 0.
var c = dba.createCluster('test', {exitStateAction: ""});

var c = dba.createCluster('test', {exitStateAction: " "});

var c = dba.createCluster('test', {exitStateAction: ":"});

var c = dba.createCluster('test', {exitStateAction: "AB"});

var c = dba.createCluster('test', {exitStateAction: "10"});

//@ WL#12049: Create cluster specifying a valid value for exitStateAction (ABORT_SERVER) {VER(>=5.7.24)}
var c = dba.createCluster('test', {exitStateAction: "ABORT_SERVER"});

//@<> WL#12049: Confirm group_replication_exit_state_action is set correctly (ABORT_SERVER) {VER(>=5.7.24)}
// F1.1 - If the exitStateAction option is given, the
// group_replication_exit_state_action variable is set to the value given by the
// user.
EXPECT_EQ("ABORT_SERVER", get_sysvar(session, "group_replication_exit_state_action"));

//@ WL#12049: Dissolve cluster 1 {VER(>=5.7.24)}
c.dissolve({force: true});

//@ WL#12049: Create cluster specifying a valid value for exitStateAction (READ_ONLY) {VER(>=5.7.24)}
// NOTE: the server is in super-read-only since it was dissolved so we must
// disable it
var c = dba.createCluster('test', {clearReadOnly: true, exitStateAction: "READ_ONLY"});

//@<> WL#12049: Confirm group_replication_exit_state_action is set correctly (READ_ONLY) {VER(>=5.7.24)}
// F1.1 - If the exitStateAction option is given, the
// group_replication_exit_state_action variable is set to the value given by the
// user.
EXPECT_EQ("READ_ONLY", get_sysvar(session, "group_replication_exit_state_action"));

//@ WL#12049: Dissolve cluster 2 {VER(>=5.7.24)}
c.dissolve({force: true});

//@ WL#12049: Create cluster specifying a valid value for exitStateAction (1) {VER(>=5.7.24)}
// NOTE: the server is in super-read-only since it was dissolved so we must
// disable it
var c = dba.createCluster('test', {clearReadOnly: true, exitStateAction: "1"});

//@<> WL#12049: Confirm group_replication_exit_state_action is set correctly (1) {VER(>=5.7.24)}
// F1.1 - If the exitStateAction option is given, the
// group_replication_exit_state_action variable is set to the value given by the
// user.
EXPECT_EQ("ABORT_SERVER", get_sysvar(session, "group_replication_exit_state_action"));

//@ WL#12049: Dissolve cluster 3 {VER(>=5.7.24)}
c.dissolve({force: true});

//@ WL#12049: Create cluster specifying a valid value for exitStateAction (0) {VER(>=5.7.24)}
// NOTE: the server is in super-read-only since it was dissolved so we must
// disable it
var c = dba.createCluster('test', {clearReadOnly: true, exitStateAction: "0"});

//@<> WL#12049: Confirm group_replication_exit_state_action is set correctly (0) {VER(>=5.7.24)}
// F1.1 - If the exitStateAction option is given, the
// group_replication_exit_state_action variable is set to the value given by the
// user.
EXPECT_EQ("READ_ONLY", get_sysvar(session, "group_replication_exit_state_action"));

//@ WL#12049: Dissolve cluster 4 {VER(>=5.7.24)}
c.dissolve({force: true});

// Verify if exitStateAction is persisted on >= 8.0.12

// F2 - On a successful [dba.]createCluster() or [Cluster.]addInstance() call
// using the option exitStateAction, the GR sysvar
// group_replication_exit_state_action must be persisted using SET PERSIST at
// the target instance, if the MySQL Server instance version is >= 8.0.12.

//@ WL#12049: Create cluster {VER(>=8.0.12)}
var c = dba.createCluster('test', {clearReadOnly: true, groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc", exitStateAction: "READ_ONLY"});

//@<OUT> WL#12049: exitStateAction must be persisted on mysql >= 8.0.12 {VER(>=8.0.12)}
print(get_persisted_gr_sysvars(__mysql_sandbox_port1));

//@ WL#12049: Dissolve cluster 6 {VER(>=8.0.12)}
c.dissolve({force: true});

// Verify that group_replication_exit_state_action is not persisted when not used
// We need a clean instance for that because dissolve does not unset the previously set variables

//@ WL#12049: Initialize new instance
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});

shell.connect(__sandbox_uri1);

//@ WL#12049: Create cluster 2 {VER(>=8.0.12)}
var c = dba.createCluster('test', {groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc"});

//@<OUT> BUG#28701263: DEFAULT VALUE OF EXITSTATEACTION TOO DRASTIC {VER(>=8.0.12)}
print(get_persisted_gr_sysvars(__mysql_sandbox_port1));

//@ WL#12049: Finalization
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);


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

shell.connect(__sandbox_uri1);

//@ WL#11032: Unsupported server version {VER(<5.7.20)}
var c = dba.createCluster('test', {memberWeight: 25});

//@ WL#11032: Create cluster errors using memberWeight option {VER(>=5.7.20)}
// F1.2 - The memberWeight option shall be an integer value.
var c = dba.createCluster('test', {memberWeight: ""});

var c = dba.createCluster('test', {memberWeight: true});

var c = dba.createCluster('test', {memberWeight: "AB"});

var c = dba.createCluster('test', {memberWeight: 10.5});

//@ WL#11032: Create cluster specifying a valid value for memberWeight (25) {VER(>=5.7.20)}
var c = dba.createCluster('test', {memberWeight: 25});

//@<> WL#11032: Confirm group_replication_member_weight is set correctly (25) {VER(>=5.7.20)}
// F1.1 - If the memberWeight option is given, the
// group_replication_member_weight variable is set to the value given by the
// user.
EXPECT_EQ(25, get_sysvar(session, "group_replication_member_weight"));

//@ WL#11032: Dissolve cluster 1 {VER(>=5.7.20)}
c.dissolve({force: true});

//@ WL#11032: Create cluster specifying a valid value for memberWeight (100) {VER(>=5.7.20)}
// NOTE: the server is in super-read-only since it was dissolved so we must
// disable it
var c = dba.createCluster('test', {clearReadOnly: true, memberWeight: 100});

//@<> WL#11032: Confirm group_replication_member_weight is set correctly (100) {VER(>=5.7.20)}
// F1.1 - If the memberWeight option is given, the
// group_replication_member_weight variable is set to the value given by the
// user.
EXPECT_EQ(100, get_sysvar(session, "group_replication_member_weight"));

//@ WL#11032: Dissolve cluster 2 {VER(>=5.7.20)}
c.dissolve({force: true});

//@ WL#11032: Create cluster specifying a valid value for memberWeight (-50) {VER(>=5.7.20)}
// NOTE: the server is in super-read-only since it was dissolved so we must
// disable it
var c = dba.createCluster('test', {clearReadOnly: true, memberWeight: -50});

//@<> WL#11032: Confirm group_replication_member_weight is set correctly (0) {VER(>=5.7.20)}
// F1.1 - If the memberWeight option is given, the
// group_replication_member_weight variable is set to the value given by the
// user.
EXPECT_EQ(0, get_sysvar(session, "group_replication_member_weight"));

//@ WL#11032: Dissolve cluster 3 {VER(>=5.7.20)}
c.dissolve({force: true});

// Verify if memberWeight is persisted on >= 8.0.11

// F2 - On a successful [dba.]createCluster() or [Cluster.]addInstance() call
// using the option memberWeight, the GR sysvar
// group_replication_member_weight must be persisted using SET PERSIST at
// the target instance, if the MySQL Server instance version is >= 8.0.11.

//@ WL#11032: Create cluster {VER(>=8.0.11)}
var c = dba.createCluster('test', {clearReadOnly: true, groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc", memberWeight: 75});

//@<OUT> WL#11032: memberWeight must be persisted on mysql >= 8.0.11 {VER(>=8.0.11)}
print(get_persisted_gr_sysvars(__mysql_sandbox_port1));

//@ WL#11032: Dissolve cluster 6 {VER(>=8.0.11)}
c.dissolve({force: true});

// Verify that group_replication_member_weight is not persisted when not used
// We need a clean instance for that because dissolve does not unset the previously set variables

//@ WL#11032: Initialize new instance
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});

shell.connect(__sandbox_uri1);

//@ WL#11032: Create cluster 2 {VER(>=8.0.11)}
var c = dba.createCluster('test', {groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc"});

//@<OUT> WL#11032: memberWeight must not be persisted on mysql >= 8.0.11 if not set {VER(>=8.0.11)}
print(get_persisted_gr_sysvars(__mysql_sandbox_port1));

//@ WL#11032: Finalization
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);

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
//@ WL#12067: Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});

shell.connect(__sandbox_uri1);

//@ WL#12067: TSF1_6 Unsupported server version {VER(<8.0.14)}
var c = dba.createCluster('test', {consistency: "EVENTUAL"});

//@ WL#12067: Create cluster errors using consistency option {VER(>=8.0.14)}
// TSF1_4, TSF1_5 - The consistency option shall be a string value.
// NOTE: GR validates the value, which is an Enumerator, and accepts the values
// `BEFORE_ON_PRIMARY_FAILOVER` or `EVENTUAL`, or 1 or 0.
var c = dba.createCluster('test', {consistency: ""});

var c = dba.createCluster('test', {consistency: " "});

var c = dba.createCluster('test', {consistency: ":"});

var c = dba.createCluster('test', {consistency: "AB"});

var c = dba.createCluster('test', {consistency: "10"});

var c = dba.createCluster('test', {consistency: 1});

var c = dba.createCluster('test', {consistency: "1", failoverConsistency: "1"});

//@ WL#12067: TSF1_1 Create cluster using BEFORE_ON_PRIMARY_FAILOVER as value for consistency {VER(>=8.0.14)}
var c = dba.createCluster('test', {consistency: "BEFORE_ON_PRIMARY_FAILOVER"});

//@<> WL#12067: TSF1_1 Confirm group_replication_consistency is set correctly (BEFORE_ON_PRIMARY_FAILOVER) {VER(>=8.0.14)}
EXPECT_EQ("BEFORE_ON_PRIMARY_FAILOVER", get_sysvar(session, "group_replication_consistency"));

//@<> WL#12067: TSF1_1 Confirm group_replication_consistency was correctly persisted. {VER(>=8.0.14)}
EXPECT_EQ("BEFORE_ON_PRIMARY_FAILOVER", get_sysvar(session, "group_replication_consistency", "PERSISTED"));

//@ WL#12067: Dissolve cluster 1 {VER(>=8.0.14)}
c.dissolve({force: true});

//@ WL#12067: TSF1_2 Create cluster using EVENTUAL as value for consistency {VER(>=8.0.14)}
// NOTE: the server is in super-read-only since it was dissolved so we must disable it
var c = dba.createCluster('test', {clearReadOnly:true, consistency: "EVENTUAL"});

//@<> WL#12067: TSF1_2 Confirm group_replication_consistency is set correctly (EVENTUAL) {VER(>=8.0.14)}
EXPECT_EQ("EVENTUAL", get_sysvar(session, "group_replication_consistency"));

//@<> WL#12067: TSF1_2 Confirm group_replication_consistency was correctly persisted. {VER(>=8.0.14)}
EXPECT_EQ("EVENTUAL", get_sysvar(session, "group_replication_consistency", "PERSISTED"));

//@ WL#12067: Dissolve cluster 2 {VER(>=8.0.14)}
c.dissolve({force: true});

//@ WL#12067: TSF1_1 Create cluster using 1 as value for consistency {VER(>=8.0.14)}
var c = dba.createCluster('test', {clearReadOnly:true, consistency: "1"});

//@<> WL#12067: TSF1_1 Confirm group_replication_consistency is set correctly (1) {VER(>=8.0.14)}
EXPECT_EQ("BEFORE_ON_PRIMARY_FAILOVER", get_sysvar(session, "group_replication_consistency"));

//@ WL#12067: Dissolve cluster 3 {VER(>=8.0.14)}
c.dissolve({force: true});

//@ WL#12067: TSF1_2 Create cluster using 0 as value for consistency {VER(>=8.0.14)}
// NOTE: the server is in super-read-only since it was dissolved so we must disable it
var c = dba.createCluster('test', {clearReadOnly:true, consistency: "0"});

//@<> WL#12067: TSF1_2 Confirm group_replication_consistency is set correctly (0) {VER(>=8.0.14)}
EXPECT_EQ("EVENTUAL", get_sysvar(session, "group_replication_consistency"));

//@ WL#12067: Dissolve cluster 4 {VER(>=8.0.14)}
c.dissolve({force: true});

//@ WL#12067: TSF1_3 Create cluster using no value for consistency {VER(>=8.0.14)}
// NOTE: the server is in super-read-only since it was dissolved so we must disable it
var c = dba.createCluster('test', {clearReadOnly:true});

//@<> WL#12067: TSF1_3 Confirm without consistency group_replication_consistency is set to default (EVENTUAL) {VER(>=8.0.14)}
EXPECT_EQ("EVENTUAL", get_sysvar(session, "group_replication_consistency"));

//@ WL#12067: Dissolve cluster 5 {VER(>=8.0.14)}
c.dissolve({force: true});

//@ WL#12067: TSF1_7 Create cluster using evenTual as value for consistency throws no exception (case insensitive) {VER(>=8.0.14)}
// NOTE: the server is in super-read-only since it was dissolved so we must disable it
var c = dba.createCluster('test', {clearReadOnly:true, consistency: "EvenTual"});

//@<> WL#12067: TSF1_7 Confirm group_replication_consistency is set correctly (EVENTUAL) {VER(>=8.0.14)}
EXPECT_EQ("EVENTUAL", get_sysvar(session, "group_replication_consistency"));

//@ WL#12067: Dissolve cluster 6 {VER(>=8.0.14)}
c.dissolve({force: true});

//@ WL#12067: TSF1_8 Create cluster using Before_ON_PriMary_FailoveR as value for consistency throws no exception (case insensitive) {VER(>=8.0.14)}
// NOTE: the server is in super-read-only since it was dissolved so we must disable it
var c = dba.createCluster('test', {clearReadOnly:true, consistency: "Before_ON_PriMary_FailoveR"});

//@<> WL#12067: TSF1_8 Confirm group_replication_consistency is set correctly (BEFORE_ON_PRIMARY_FAILOVER) {VER(>=8.0.14)}
EXPECT_EQ("BEFORE_ON_PRIMARY_FAILOVER", get_sysvar(session, "group_replication_consistency"));

//@ WL#12067: Dissolve cluster 7 {VER(>=8.0.14)}
c.dissolve({force: true});

// Verify that group_replication_consistency is not persisted when not used
// We need a clean instance for that because dissolve does not unset the previously set variables
//@ WL#12067: Initialize new instance {VER(>=8.0.14)}
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});

shell.connect(__sandbox_uri1);

//@ WL#12067: Create cluster 2 {VER(>=8.0.14)}
var c = dba.createCluster('test');

//@<> WL#12067: consistency must not be persisted on mysql >= 8.0.14 if not set {VER(>=8.0.14)}
EXPECT_EQ("", get_sysvar(session, "group_replication_consistency", "PERSISTED"));

//@ WL#12067: Finalization
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);

// WL#12050 AdminAPI: Define the timeout for evicting previously active cluster members
//
// In 8.0.13, Group Replication introduced an option to allow defining
// the timeout for evicting previously active nodes.  In order to support
// defining such option, the AdminAPI was extended by introducing a new
// optional parameter, named 'expelTimeout', in the dba.createCluster()
// function.
//
//@ WL#12050: Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
shell.connect(__sandbox_uri1);

//@ WL#12050: TSF1_5 Unsupported server version {VER(<8.0.13)}
var c = dba.createCluster('test', {expelTimeout: 100});

//@ WL#12050: Create cluster errors using expelTimeout option {VER(>=8.0.13)}
// TSF1_3, TSF1_4, TSF1_6
var c = dba.createCluster('test', {expelTimeout:""});

var c = dba.createCluster('test', {expelTimeout: "10a"});

var c = dba.createCluster('test', {expelTimeout: 10.5});

var c = dba.createCluster('test', {expelTimeout: true});

var c = dba.createCluster('test', {expelTimeout: -1});

var c = dba.createCluster('test', {expelTimeout: 3601});

//@ WL#12050: TSF1_1 Create cluster using 12 as value for expelTimeout {VER(>=8.0.13)}
var c = dba.createCluster('test', {expelTimeout: 12});

//@<> WL#12050: TSF1_1 Confirm group_replication_member_expel_timeout is set correctly (12) {VER(>=8.0.13)}
EXPECT_EQ(12, get_sysvar(session, "group_replication_member_expel_timeout"));

//@<> WL#12050: TSF1_1 Confirm group_replication_consistency was correctly persisted. {VER(>=8.0.13)}
EXPECT_EQ("12", get_sysvar(session, "group_replication_member_expel_timeout", "PERSISTED"));

//@ WL#12050: Dissolve cluster 1 {VER(>=8.0.13)}
c.dissolve({force: true});

// Verify that group_replication_member_expel_timeout is not persisted when not used
// We need a clean instance for that because dissolve does not unset the previously set variables
//@ WL#12050: TSF1_2 Initialize new instance {VER(>=8.0.13)}
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
shell.connect(__sandbox_uri1);

//@ WL#12050: TSF1_2 Create cluster using no value for expelTimeout, confirm it has the default value {VER(>=8.0.13)}
// NOTE: the server is in super-read-only since it was dissolved so we must disable it
var c = dba.createCluster('test', {clearReadOnly:true});
c.disconnect();

//@<> WL#12050: TSF1_2 Confirm group_replication_member_expel_timeout is set correctly (0) {VER(>=8.0.13)}
EXPECT_EQ(0, get_sysvar(session, "group_replication_member_expel_timeout"));

//@<> WL#12050: TSF1_2 Confirm group_replication_member_expel_timeout was not persisted since no value was provided. {VER(>=8.0.13)}
EXPECT_EQ("", get_sysvar(session, "group_replication_member_expel_timeout", "PERSISTED"));

//@ WL#12050: Finalization
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);

// Regression test for BUG#25867733: CHECKINSTANCECONFIGURATION SUCCESS BUT CREATE CLUSTER FAILING IF PFS DISABLED
//@ BUG#25867733: Deploy instances (setting performance_schema value).
testutil.deploySandbox(__mysql_sandbox_port1, "root", {"performance_schema": "off", report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {"performance_schema": "on",report_host: hostname});
shell.connect(__sandbox_uri1);

//@ BUG#25867733: createCluster error with performance_schema=off
dba.createCluster("test_cluster");

session.close();
shell.connect(__sandbox_uri2);

//@ BUG#25867733: createCluster no error with performance_schema=on
var c = dba.createCluster("test_cluster");

//@ BUG#25867733: cluster.status() successful
c.status();

//@ BUG#25867733: finalization
c.disconnect();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);


// Regression test for BUG#29246110: CREATECLUSTER() OR ADDINSTANCE() FAILS IN DEBIAN-BASED PLATFORMS WITH LOCALHOST
//@ BUG#29246110: Deploy instances, setting non supported host: 127.0.1.1.
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: "127.0.1.1"});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});

//@ BUG#29246110: check instance error with non supported host.
dba.checkInstanceConfiguration(__sandbox_uri1);

//@ BUG#29246110: createCluster error with non supported host.
shell.connect(__sandbox_uri1);
var c = dba.createCluster("error");
session.close();

//@ BUG#29246110: create cluster succeed with supported host.
shell.connect(__sandbox_uri2);
var c = dba.createCluster("test_cluster");

//@ BUG#29246110: add instance error with non supported host.
c.addInstance(__sandbox_uri1);

// NOTE: Do not destroy sandbox 2 to be used on the next tests
//@ BUG#29246110: finalization
c.dissolve({force: true});
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);

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
var s1 = mysql.getSession(__sandbox_uri1);
var s2 = mysql.getSession(__sandbox_uri2);
shell.connect(__sandbox_uri1);

// FR1 - A new option autoRejoinTries shall be added to the [dba.]createCluster() and [cluster.]addInstance() to allow users to set the Group Replication (GR) system variable group_replication_autorejoin_tries.

//@ WL#12066: TSF1_4 Validate that an exception is thrown if the value specified is not an unsigned integer. {VER(>=8.0.16)}
var c = dba.createCluster('test', {autoRejoinTries: -1});

//@ WL#12066: TSF1_5 Validate that an exception is thrown if the value  is not in the range 0 to 2016. {VER(>=8.0.16)}
var c = dba.createCluster('test', {autoRejoinTries: 2017});

//@ WL#12066: TSF1_1 Validate that the functions [dba.]createCluster() and [cluster.]addInstance() support a new option named autoRejoinTries. {VER(>=8.0.16)}
var c = dba.createCluster('test', {autoRejoinTries: 2016});
session.close();
shell.connect(__sandbox_uri2);
var c2 = dba.createCluster('test2', {clearReadOnly: true});
session.close();

//@<> WL#12066: TSF1_3, TSF1_6 Validate that when calling the functions [dba.]createCluster() and [cluster.]addInstance(), the GR variable group_replication_autorejoin_tries is persisted with the value given by the user on the target instance.{VER(>=8.0.16)}
EXPECT_EQ(2016, get_sysvar(__mysql_sandbox_port1, "group_replication_autorejoin_tries"));
EXPECT_EQ(0, get_sysvar(__mysql_sandbox_port2, "group_replication_autorejoin_tries"));

//@<> WL#12066: TSF1_3, TSF1_6 Confirm group_replication_autorejoin_tries value was persisted {VER(>=8.0.16)}
EXPECT_EQ("2016", get_sysvar(__mysql_sandbox_port1, "group_replication_autorejoin_tries", "PERSISTED"));
EXPECT_EQ("", get_sysvar(__mysql_sandbox_port2, "group_replication_autorejoin_tries", "PERSISTED"));

//@ WL#12066: Dissolve cluster {VER(>=8.0.16)}
c.dissolve({force: true});
c2.dissolve({force: true});

//@ WL#12066: Finalization {VER(>=8.0.16)}
// NOTE: Do not destroy sandbox 2 to be used on the next tests
testutil.destroySandbox(__mysql_sandbox_port1);

// Regression test for BUG#29308037
//
// When dba.createCluster() fails and a rollback is performed to remove the
// created replication user, the account created at 'localhost' is not being
// removed

// Force a failure of the create_cluster function after the replication-user was created
//@<> BUG#29308037: Create cluster using an invalid localAddress
shell.connect(__sandbox_uri2);
EXPECT_THROWS(function() {dba.createCluster('test', {localAddress: '1a', clearReadOnly: true})}, "ERROR: Error starting cluster");

//@<OUT> BUG#29308037: Confirm that all replication users where removed
print(get_query_single_result(session, number_of_rpl_users_query) + "\n");

//@ BUG#29308037: Finalization
//NOTE: Do not destroy sandbox 2 to be used on the next tests
session.close();

// BUG#29305551: ADMINAPI FAILS TO DETECT INSTANCE IS RUNNING ASYNCHRONOUS REPLICATION
//
// dba.checkInstance() reports that a target instance which is running the Slave
// SQL and IO threads is valid for InnoDB cluster usage.
//
// As a consequence, the AdminAPI fails to detects that an instance has
// asynchronous replication running and both addInstance() and rejoinInstance()
// fail with useless/unfriendly errors on this scenario. There's not even
// information on how to overcome the issue.
//
// dba.createCluster() shall not fail if asynchronous replication is running

//@ BUG#29305551: Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});

//@<> BUG#29305551: Setup asynchronous replication
shell.connect(__sandbox_uri1);

// Create Replication user
session.runSql("CREATE USER 'repl'@'%' IDENTIFIED BY 'password' REQUIRE SSL");
session.runSql("GRANT REPLICATION SLAVE ON *.* TO 'repl'@'%';");

// Set async channel on instance2
session.close();
shell.connect(__sandbox_uri2);

session.runSql("CHANGE MASTER TO MASTER_HOST='" + hostname + "', MASTER_PORT=" + __mysql_sandbox_port1 + ", MASTER_USER='repl', MASTER_PASSWORD='password', MASTER_AUTO_POSITION=1, MASTER_SSL=1");
session.runSql("START SLAVE");

//@ Create cluster async replication OK
dba.createCluster('testAsync', {clearReadOnly: true});

//@ BUG#29305551: Finalization
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
