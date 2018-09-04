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

function print_gr_member_weight() {
  var result = session.runSql('SELECT @@GLOBAL.group_replication_member_weight');
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
testutil.deploySandbox(__mysql_sandbox_port2, "root");

shell.connect(__sandbox_uri1);

// Test the option on the addInstance() command
//@ WL#12049: Create cluster 1 {VER(>=5.7.24)}
var c = dba.createCluster('test', {clearReadOnly: true})

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

// Verify if exitStateAction is persisted on >= 8.0.11

// F2 - On a successful [dba.]createCluster() or [Cluster.]addInstance() call
// using the option exitStateAction, the GR sysvar
// group_replication_exit_state_action must be persisted using SET PERSIST at
// the target instance, if the MySQL Server instance version is >= 8.0.11.

//@ WL#12049: Create cluster 2 {VER(>=8.0.11)}
var c = dba.createCluster('test', {clearReadOnly: true, groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc", exitStateAction: "READ_ONLY"});

//@ WL#12049: Add instance using a valid exitStateAction 2 {VER(>=8.0.11)}
c.addInstance(__sandbox_uri2, {exitStateAction: "READ_ONLY"})

session.close()
shell.connect(__sandbox_uri2);

//@<OUT> WL#12049: exitStateAction must be persisted on mysql >= 8.0.11 {VER(>=8.0.11)}
print_persisted_variables(session);

//@ WL#12049: Dissolve cluster 2 {VER(>=8.0.11)}
c.dissolve({force: true});

// Verify that group_replication_exit_state_action is not persisted when not used
// We need a clean instance for that because dissolve does not unset the previously set variables

//@ WL#12049: Initialize new instance
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");

shell.connect(__sandbox_uri1);

//@ WL#12049: Create cluster 3 {VER(>=8.0.11)}
var c = dba.createCluster('test', {groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc"});

//@ WL#12049: Add instance without using exitStateAction {VER(>=8.0.11)}
c.addInstance(__sandbox_uri2)

session.close()
shell.connect(__sandbox_uri2);

//@<OUT> WL#12049: exitStateAction must not be persisted on mysql >= 8.0.11 if not set {VER(>=8.0.11)}
print_persisted_variables(session);

//@ WL#12049: Finalization
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);


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
testutil.deploySandbox(__mysql_sandbox_port2, "root");

shell.connect(__sandbox_uri1);

// Test the option on the addInstance() command
//@ WL#11032: Create cluster 1 {VER(>=5.7.20)}
var c = dba.createCluster('test', {clearReadOnly: true})

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
var c = dba.createCluster('test', {clearReadOnly: true, groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc", memberWeight: 75});

//@ WL#11032: Add instance using a valid value for memberWeight (-50) {VER(>=8.0.11)}
c.addInstance(__sandbox_uri2, {memberWeight: -50})

session.close()
shell.connect(__sandbox_uri2);

//@<OUT> WL#11032: memberWeight must be persisted on mysql >= 8.0.11 {VER(>=8.0.11)}
print_persisted_variables(session);

//@ WL#11032: Dissolve cluster 2 {VER(>=8.0.11)}
c.dissolve({force: true});

// Verify that group_replication_member_weight is not persisted when not used
// We need a clean instance for that because dissolve does not unset the previously set variables

//@ WL#11032: Initialize new instance
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");

shell.connect(__sandbox_uri1);

//@ WL#11032: Create cluster 3 {VER(>=8.0.11)}
var c = dba.createCluster('test', {groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc"});

//@ WL#11032: Add instance without using memberWeight {VER(>=8.0.11)}
c.addInstance(__sandbox_uri2)

session.close()
shell.connect(__sandbox_uri2);

//@<OUT> WL#11032: memberWeight must not be persisted on mysql >= 8.0.11 if not set {VER(>=8.0.11)}
print_persisted_variables(session);

//@ WL#11032: Finalization
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
