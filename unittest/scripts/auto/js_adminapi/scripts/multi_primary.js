// Tests to validate the correct handling of multi-primary cluster, starting
// it from the scratch or switching the mode of an existing cluster.
// Related with BUG#29794779 and BUG#29867483.
// - Metadata cannot have unsupported foreign Keys (e.g., CASCADE), etc.
// - The group_replication_enforce_update_everywhere_checks variable must be set
//   properly when creating a cluster and adding instances to it, depending on
//   the cluster topology mode (single-primary or multi-primary).

function print_gr_enforce_update_everywhere_checks(session) {
    var res = session.runSql("SHOW VARIABLES LIKE 'group_replication_enforce_update_everywhere_checks'");
    print(res.fetchOne()[1] + "\n");
}

//@<> Initialization.
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});

shell.connect(__sandbox_uri1);
var session2 = mysql.getSession(__sandbox_uri2);
var session3 = mysql.getSession(__sandbox_uri3);

//@ Create single-primary cluster.
var c = dba.createCluster("test", {gtidSetIsComplete: true});
c.addInstance(__sandbox_uri2);

//@ GR enforce update everywhere checks is OFF for single-primary cluster.
print_gr_enforce_update_everywhere_checks(session);

//@ Switch cluster to multi-primary. {VER(>=8.0.13)}
c.switchToMultiPrimaryMode();

//@ Add another instance to multi-primary cluster. {VER(>=8.0.13)}
c.addInstance(__sandbox_uri3);

//@ GR enforce update everywhere checks is ON on all instances. {VER(>=8.0.13)}
print_gr_enforce_update_everywhere_checks(session);
print_gr_enforce_update_everywhere_checks(session2);
print_gr_enforce_update_everywhere_checks(session3);

//@ Remove one instance from the cluster. {VER(>=8.0.13)}
c.removeInstance(__sandbox_uri3);

//@ GR enforce update everywhere checks must be OFF on removed instance (multi-primary). {VER(>=8.0.13)}
print_gr_enforce_update_everywhere_checks(session3);

//@ Dissolve multi-primary cluster.
c.dissolve();

//@ GR enforce update everywhere checks is OFF on all instances after dissolve (multi-primary). {VER(>=8.0.13)}
print_gr_enforce_update_everywhere_checks(session);
print_gr_enforce_update_everywhere_checks(session2);

//@ Create multi-primary cluster.
var c = dba.createCluster("cluster", {multiPrimary: true, force: true, gtidSetIsComplete: true});
c.addInstance(__sandbox_uri2);

//@ GR enforce update everywhere checks is ON for multi-primary cluster.
print_gr_enforce_update_everywhere_checks(session);

//@ Switch cluster to single-primary. {VER(>=8.0.13)}
c.switchToSinglePrimaryMode();

//@ Add another instance to single-primary cluster. {VER(>=8.0.13)}
c.addInstance(__sandbox_uri3);

//@ GR enforce update everywhere checks is OFF on all instances. {VER(>=8.0.13)}
print_gr_enforce_update_everywhere_checks(session);
print_gr_enforce_update_everywhere_checks(session2);
print_gr_enforce_update_everywhere_checks(session3);

//@ Remove one instance from single-primary cluster. {VER(>=8.0.13)}
c.removeInstance(__sandbox_uri3);

//@ GR enforce update everywhere checks must be OFF on removed instance (single-primary). {VER(>=8.0.13)}
print_gr_enforce_update_everywhere_checks(session3);

//@ Dissolve single-primary cluster.
c.dissolve();

//@ GR enforce update everywhere checks is OFF on all instances after dissolve (single-primary). {VER(>=8.0.13)}
print_gr_enforce_update_everywhere_checks(session);
print_gr_enforce_update_everywhere_checks(session2);

//@<> Clean-up.
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
