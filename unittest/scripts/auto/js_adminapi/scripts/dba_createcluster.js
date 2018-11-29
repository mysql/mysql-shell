// Assumptions: smart deployment functions available

function print_gr_exit_state_action() {
  var result = session.runSql('SELECT @@GLOBAL.group_replication_exit_state_action');
  var row = result.fetchOne();
  print(row[0] + "\n");
}

function print_persisted_variables(session) {
    var res = session.runSql("SELECT * from performance_schema.persisted_variables WHERE Variable_name like '%group_replication%'").fetchAll();
    for (var i = 0; i < res.length; i++) {
        print(res[i][0] + " = " + res[i][1] + "\n");
    }
    print("\n");
}

function print_persisted_variables_like(session, pattern) {
    var res = session.runSql("SELECT * from performance_schema.persisted_variables WHERE Variable_name like '%" + pattern + "%'").fetchAll();
    for (var i = 0; i < res.length; i++) {
        print(res[i][0] + " = " + res[i][1] + "\n");
    }
    print("\n");
}

function print_gr_member_weight() {
  var result = session.runSql('SELECT @@GLOBAL.group_replication_member_weight');
  var row = result.fetchOne();
  print(row[0] + "\n");
}

function print_auto_increment_variables() {
  var res = session.runSql('SHOW VARIABLES like "auto_increment%"').fetchAll();
  for (var i = 0; i < 2; i++) {
        print(res[i][0] + " = " + res[i][1] + "\n");
  }
  print("\n");
}

function print_gr_failover_consistency() {
    var result = session.runSql('SELECT @@GLOBAL.group_replication_consistency');
    var row = result.fetchOne();
    print(row[0] + "\n");
}

function print_gr_expel_timeout() {
    var result = session.runSql('SELECT @@GLOBAL.group_replication_member_expel_timeout');
    var row = result.fetchOne();
    print(row[0] + "\n");
}

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
testutil.deploySandbox(__mysql_sandbox_port1, "root");

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

//@<OUT> WL#12049: Confirm group_replication_exit_state_action is set correctly (ABORT_SERVER) {VER(>=5.7.24)}
// F1.1 - If the exitStateAction option is given, the
// group_replication_exit_state_action variable is set to the value given by the
// user.
print_gr_exit_state_action();

//@ WL#12049: Dissolve cluster 1 {VER(>=5.7.24)}
c.dissolve({force: true});

//@ WL#12049: Create cluster specifying a valid value for exitStateAction (READ_ONLY) {VER(>=5.7.24)}
// NOTE: the server is in super-read-only since it was dissolved so we must
// disable it
var c = dba.createCluster('test', {clearReadOnly: true, exitStateAction: "READ_ONLY"});

//@<OUT> WL#12049: Confirm group_replication_exit_state_action is set correctly (READ_ONLY) {VER(>=5.7.24)}
// F1.1 - If the exitStateAction option is given, the
// group_replication_exit_state_action variable is set to the value given by the
// user.
print_gr_exit_state_action();

//@ WL#12049: Dissolve cluster 2 {VER(>=5.7.24)}
c.dissolve({force: true});

//@ WL#12049: Create cluster specifying a valid value for exitStateAction (1) {VER(>=5.7.24)}
// NOTE: the server is in super-read-only since it was dissolved so we must
// disable it
var c = dba.createCluster('test', {clearReadOnly: true, exitStateAction: "1"});

//@<OUT> WL#12049: Confirm group_replication_exit_state_action is set correctly (1) {VER(>=5.7.24)}
// F1.1 - If the exitStateAction option is given, the
// group_replication_exit_state_action variable is set to the value given by the
// user.
print_gr_exit_state_action();

//@ WL#12049: Dissolve cluster 3 {VER(>=5.7.24)}
c.dissolve({force: true});

//@ WL#12049: Create cluster specifying a valid value for exitStateAction (0) {VER(>=5.7.24)}
// NOTE: the server is in super-read-only since it was dissolved so we must
// disable it
var c = dba.createCluster('test', {clearReadOnly: true, exitStateAction: "0"});

//@<OUT> WL#12049: Confirm group_replication_exit_state_action is set correctly (0) {VER(>=5.7.24)}
// F1.1 - If the exitStateAction option is given, the
// group_replication_exit_state_action variable is set to the value given by the
// user.
print_gr_exit_state_action();

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
print_persisted_variables(session);

//@ WL#12049: Dissolve cluster 6 {VER(>=8.0.12)}
c.dissolve({force: true});

// Verify that group_replication_exit_state_action is not persisted when not used
// We need a clean instance for that because dissolve does not unset the previously set variables

//@ WL#12049: Initialize new instance
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port1, "root");

shell.connect(__sandbox_uri1);

//@ WL#12049: Create cluster 2 {VER(>=8.0.12)}
var c = dba.createCluster('test', {groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc"});

//@<OUT> BUG#28701263: DEFAULT VALUE OF EXITSTATEACTION TOO DRASTIC {VER(>=8.0.12)}
print_persisted_variables(session);

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
testutil.deploySandbox(__mysql_sandbox_port1, "root");

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

//@<OUT> WL#11032: Confirm group_replication_member_weight is set correctly (25) {VER(>=5.7.20)}
// F1.1 - If the memberWeight option is given, the
// group_replication_member_weight variable is set to the value given by the
// user.
print_gr_member_weight();

//@ WL#11032: Dissolve cluster 1 {VER(>=5.7.20)}
c.dissolve({force: true});

//@ WL#11032: Create cluster specifying a valid value for memberWeight (100) {VER(>=5.7.20)}
// NOTE: the server is in super-read-only since it was dissolved so we must
// disable it
var c = dba.createCluster('test', {clearReadOnly: true, memberWeight: 100});

//@<OUT> WL#11032: Confirm group_replication_member_weight is set correctly (100) {VER(>=5.7.20)}
// F1.1 - If the memberWeight option is given, the
// group_replication_member_weight variable is set to the value given by the
// user.
print_gr_member_weight();

//@ WL#11032: Dissolve cluster 2 {VER(>=5.7.20)}
c.dissolve({force: true});

//@ WL#11032: Create cluster specifying a valid value for memberWeight (-50) {VER(>=5.7.20)}
// NOTE: the server is in super-read-only since it was dissolved so we must
// disable it
var c = dba.createCluster('test', {clearReadOnly: true, memberWeight: -50});

//@<OUT> WL#11032: Confirm group_replication_member_weight is set correctly (0) {VER(>=5.7.20)}
// F1.1 - If the memberWeight option is given, the
// group_replication_member_weight variable is set to the value given by the
// user.
print_gr_member_weight();

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
print_persisted_variables(session);

//@ WL#11032: Dissolve cluster 6 {VER(>=8.0.11)}
c.dissolve({force: true});

// Verify that group_replication_member_weight is not persisted when not used
// We need a clean instance for that because dissolve does not unset the previously set variables

//@ WL#11032: Initialize new instance
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port1, "root");

shell.connect(__sandbox_uri1);

//@ WL#11032: Create cluster 2 {VER(>=8.0.11)}
var c = dba.createCluster('test', {groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc"});

//@<OUT> WL#11032: memberWeight must not be persisted on mysql >= 8.0.11 if not set {VER(>=8.0.11)}
print_persisted_variables(session);

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
// introducing a new optional parameter, named 'failoverConsistency', in the
// dba.createCluster function.
//
//@ WL#12067: Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root");

shell.connect(__sandbox_uri1);

//@ WL#12067: TSF1_6 Unsupported server version {VER(<8.0.14)}
var c = dba.createCluster('test', {failoverConsistency: "EVENTUAL"});

//@ WL#12067: Create cluster errors using failoverConsistency option {VER(>=8.0.14)}
// TSF1_4, TSF1_5 - The failoverConsistency option shall be a string value.
// NOTE: GR validates the value, which is an Enumerator, and accepts the values
// `BEFORE_ON_PRIMARY_FAILOVER` or `EVENTUAL`, or 1 or 0.
var c = dba.createCluster('test', {failoverConsistency: ""});

var c = dba.createCluster('test', {failoverConsistency: " "});

var c = dba.createCluster('test', {failoverConsistency: ":"});

var c = dba.createCluster('test', {failoverConsistency: "AB"});

var c = dba.createCluster('test', {failoverConsistency: "10"});

var c = dba.createCluster('test', {failoverConsistency: 1});

//@ WL#12067: TSF1_1 Create cluster using BEFORE_ON_PRIMARY_FAILOVER as value for failoverConsistency {VER(>=8.0.14)}
var c = dba.createCluster('test', {failoverConsistency: "BEFORE_ON_PRIMARY_FAILOVER"});

//@<OUT> WL#12067: TSF1_1 Confirm group_replication_consistency is set correctly (BEFORE_ON_PRIMARY_FAILOVER) {VER(>=8.0.14)}
print_gr_failover_consistency();

//@<OUT> WL#12067: TSF1_1 Confirm group_replication_consistency was correctly persisted. {VER(>=8.0.14)}
print_persisted_variables_like(session, "group_replication_consistency");

//@ WL#12067: Dissolve cluster 1 {VER(>=8.0.14)}
c.dissolve({force: true});

//@ WL#12067: TSF1_2 Create cluster using EVENTUAL as value for failoverConsistency {VER(>=8.0.14)}
// NOTE: the server is in super-read-only since it was dissolved so we must disable it
var c = dba.createCluster('test', {clearReadOnly:true, failoverConsistency: "EVENTUAL"});

//@<OUT> WL#12067: TSF1_2 Confirm group_replication_consistency is set correctly (EVENTUAL) {VER(>=8.0.14)}
print_gr_failover_consistency();

//@<OUT> WL#12067: TSF1_2 Confirm group_replication_consistency was correctly persisted. {VER(>=8.0.14)}
print_persisted_variables_like(session, "group_replication_consistency");

//@ WL#12067: Dissolve cluster 2 {VER(>=8.0.14)}
c.dissolve({force: true});

//@ WL#12067: TSF1_1 Create cluster using 1 as value for failoverConsistency {VER(>=8.0.14)}
var c = dba.createCluster('test', {clearReadOnly:true, failoverConsistency: "1"});

//@<OUT> WL#12067: TSF1_1 Confirm group_replication_consistency is set correctly (1) {VER(>=8.0.14)}
print_gr_failover_consistency();

//@ WL#12067: Dissolve cluster 3 {VER(>=8.0.14)}
c.dissolve({force: true});

//@ WL#12067: TSF1_2 Create cluster using 0 as value for failoverConsistency {VER(>=8.0.14)}
// NOTE: the server is in super-read-only since it was dissolved so we must disable it
var c = dba.createCluster('test', {clearReadOnly:true, failoverConsistency: "0"});

//@<OUT> WL#12067: TSF1_2 Confirm group_replication_consistency is set correctly (0) {VER(>=8.0.14)}
print_gr_failover_consistency();

//@ WL#12067: Dissolve cluster 4 {VER(>=8.0.14)}
c.dissolve({force: true});

//@ WL#12067: TSF1_3 Create cluster using no value for failoverConsistency {VER(>=8.0.14)}
// NOTE: the server is in super-read-only since it was dissolved so we must disable it
var c = dba.createCluster('test', {clearReadOnly:true});

//@<OUT> WL#12067: TSF1_3 Confirm without failoverConsistency group_replication_consistency is set to default (EVENTUAL) {VER(>=8.0.14)}
print_gr_failover_consistency();

//@ WL#12067: Dissolve cluster 5 {VER(>=8.0.14)}
c.dissolve({force: true});

//@ WL#12067: TSF1_7 Create cluster using evenTual as value for failoverConsistency throws no exception (case insensitive) {VER(>=8.0.14)}
// NOTE: the server is in super-read-only since it was dissolved so we must disable it
var c = dba.createCluster('test', {clearReadOnly:true, failoverConsistency: "EvenTual"});

//@<OUT> WL#12067: TSF1_7 Confirm group_replication_consistency is set correctly (EVENTUAL) {VER(>=8.0.14)}
print_gr_failover_consistency();

//@ WL#12067: Dissolve cluster 6 {VER(>=8.0.14)}
c.dissolve({force: true});

//@ WL#12067: TSF1_8 Create cluster using Before_ON_PriMary_FailoveR as value for failoverConsistency throws no exception (case insensitive) {VER(>=8.0.14)}
// NOTE: the server is in super-read-only since it was dissolved so we must disable it
var c = dba.createCluster('test', {clearReadOnly:true, failoverConsistency: "Before_ON_PriMary_FailoveR"});

//@<OUT> WL#12067: TSF1_8 Confirm group_replication_consistency is set correctly (BEFORE_ON_PRIMARY_FAILOVER) {VER(>=8.0.14)}
print_gr_failover_consistency();

//@ WL#12067: Dissolve cluster 7 {VER(>=8.0.14)}
c.dissolve({force: true});

// Verify that group_replication_consistency is not persisted when not used
// We need a clean instance for that because dissolve does not unset the previously set variables
//@ WL#12067: Initialize new instance {VER(>=8.0.14)}
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port1, "root");

shell.connect(__sandbox_uri1);

//@ WL#12067: Create cluster 2 {VER(>=8.0.14)}
var c = dba.createCluster('test');

//@<OUT> WL#12067: failoverConsistency must not be persisted on mysql >= 8.0.14 if not set {VER(>=8.0.14)}
print_persisted_variables_like(session, "group_replication_consistency");

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
testutil.deploySandbox(__mysql_sandbox_port1, "root");
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

//@ WL#12050: TSF1_1 Confirm group_replication_member_expel_timeout is set correctly (12) {VER(>=8.0.13)}
print_gr_expel_timeout();

//@<OUT> WL#12050: TSF1_1 Confirm group_replication_consistency was correctly persisted. {VER(>=8.0.13)}
print_persisted_variables_like(session, "group_replication_member_expel_timeout");

//@ WL#12050: Dissolve cluster 1 {VER(>=8.0.13)}
c.dissolve({force: true});

// Verify that group_replication_member_expel_timeout is not persisted when not used
// We need a clean instance for that because dissolve does not unset the previously set variables
//@ WL#12050: TSF1_2 Initialize new instance {VER(>=8.0.13)}
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port1, "root");
shell.connect(__sandbox_uri1);

//@ WL#12050: TSF1_2 Create cluster using no value for expelTimeout, confirm it has the default value {VER(>=8.0.13)}
// NOTE: the server is in super-read-only since it was dissolved so we must disable it
var c = dba.createCluster('test', {clearReadOnly:true});
c.disconnect();

//@ WL#12050: TSF1_2 Confirm group_replication_member_expel_timeout is set correctly (0) {VER(>=8.0.13)}
print_gr_expel_timeout();

//@<OUT> WL#12050: TSF1_2 Confirm group_replication_member_expel_timeout was not persisted since no value was provided. {VER(>=8.0.13)}
print_persisted_variables_like(session, "group_replication_member_expel_timeout");

//@ WL#12050: Finalization
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
