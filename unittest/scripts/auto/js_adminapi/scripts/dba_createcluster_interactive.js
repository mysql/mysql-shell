// Assumptions: smart deployment functions available

// WL#12011: AdminAPI: Refactor dba.createCluster()
//@ WL#12011: Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);

shell.connect(__sandbox_uri1);

//@<> WL#12011: FR2-03 - no interactive option (default: interactive).
testutil.expectPrompt("Confirm [y/N]:", "n");
var c = dba.createCluster('test', {multiPrimary: true});

//@<> WL#12011: FR2-02 - interactive = false.
shell.options.useWizards=0;
var c = dba.createCluster('test', {multiPrimary: true, force: true});
shell.options.useWizards=1;

//@ WL#12011: Finalization.
c.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);

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
// INTERACTIVE test

//@ WL#12049: Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});

shell.connect(__sandbox_uri1);

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

//@ WL#12049: Finalization
c.disconnect();
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

//@ WL#11032: Create cluster errors using memberWeight option {VER(>=5.7.20)}
// F1.2 - The memberWeight option shall be an integer value.
var c = dba.createCluster('test', {memberWeight: ""});

var c = dba.createCluster('test', {memberWeight: true});

var c = dba.createCluster('test', {memberWeight: "AB"});

var c = dba.createCluster('test', {memberWeight: 10.5});

//@ WL#11032: Create cluster specifying a valid value for memberWeight (integer) {VER(>=5.7.20)}
var c = dba.createCluster('test', {memberWeight: 25});

//@ WL#11032: Finalization
c.disconnect();
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

//@ WL#12067: TSF1_1 Create cluster using a valid as value for consistency {VER(>=8.0.14)}
var c = dba.createCluster('test', {consistency: "BEFORE_ON_PRIMARY_FAILOVER"});

//@<> Dissolve the cluster {VER(>=8.0.14)}
shell.options.useWizards=0;
c.dissolve({force: true})
shell.options.useWizards=1;

//@<> Create cluster using a valid value for consistency {VER(>=8.0.14)}
var c;
EXPECT_NO_THROWS(function(){ c = dba.createCluster('test', {consistency: "BEFORE_ON_PRIMARY_FAILOVER"}); });

//@ WL#12067: Finalization
c.disconnect();
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

//@<> WL#12050: Create cluster errors using expelTimeout option {VER(>=8.0.13)}
// TSF1_3, TSF1_4, TSF1_6
EXPECT_THROWS(function(){dba.createCluster('test', {expelTimeout:""});}, "Option 'expelTimeout' Integer expected, but value is String");
EXPECT_THROWS(function(){dba.createCluster('test', {expelTimeout: "10a"});}, "Option 'expelTimeout' Integer expected, but value is String");
EXPECT_THROWS(function(){dba.createCluster('test', {expelTimeout: 10.5});}, "Option 'expelTimeout' is expected to be of type Integer, but is Float");
EXPECT_THROWS(function(){dba.createCluster('test', {expelTimeout: true});}, "Option 'expelTimeout' is expected to be of type Integer, but is Bool");
EXPECT_THROWS(function(){dba.createCluster('test', {expelTimeout: -1});}, "Invalid value for 'expelTimeout': integer value must be >= 0");

//@<> WL#12050: Create cluster errors using expelTimeout option (v8.0.13) {VER(==8.0.13)}
EXPECT_THROWS(function(){dba.createCluster('test', {expelTimeout: 3601});}, "Invalid value for 'expelTimeout': integer value must be in the range [0, 3600]");

//@ WL#12050: TSF1_1 Create cluster using a valid value for expelTimeout {VER(>=8.0.13)}
var c = dba.createCluster('test', {expelTimeout: 12});

//@ WL#12050: Finalization
c.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);

// BUG#29361352: multiprimary warning and prompt displayed with multiPrimary:false
//
// The warning an prompt for creating multi-primary cluster should only be
// displayed if the multiPrimary option is set to true.
// This is a regression issue.

//@ BUG#29361352: Initialization.
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
shell.connect(__sandbox_uri1);

//@<> BUG#29361352: no warning or prompt for multi-primary (multiPrimary: false).
var c = dba.createCluster('test', {multiPrimary: false, force: false});

//@ BUG#29361352: Finalization.
c.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
