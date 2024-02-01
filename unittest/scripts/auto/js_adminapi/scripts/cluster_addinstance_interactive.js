// Assumptions: smart deployment functions available

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
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});

shell.connect(__sandbox_uri1);

// Test the option on the addInstance() command
//@ WL#12049: Create cluster 1 {VER(>=5.7.24)}
var c = dba.createCluster('test', {gtidSetIsComplete: true});

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

//@ WL#12049: Finalization cluster {VER(>=5.7.24)}
c.disconnect();

//@ WL#12049: Finalization
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

// WL#12049 AdminAPI: Configure member weight for automatic primary election on
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
var c = dba.createCluster('test', {gtidSetIsComplete: true});

//@ WL#11032: addInstance() errors using memberWeight option {VER(>=5.7.20)}
// F1.2 - The memberWeight option shall be an integer value.
c.addInstance(__sandbox_uri2, {memberWeight: ""});

c.addInstance(__sandbox_uri2, {memberWeight: true});

c.addInstance(__sandbox_uri2, {memberWeight: "AB"});

c.addInstance(__sandbox_uri2, {memberWeight: 10.5});

//@ WL#11032: Add instance using a valid memberWeight (integer) {VER(>=5.7.20)}
c.addInstance(__sandbox_uri2, {memberWeight: 25});

//@ WL#11032: Finalization
c.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
