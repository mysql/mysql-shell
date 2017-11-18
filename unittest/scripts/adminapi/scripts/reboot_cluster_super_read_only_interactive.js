// Tests for super_read_only handling after cluster is down
// Non-interactive tests


//@ Make a cluster with a single node then stop GR to simulate a dead cluster
testutil.deploySandbox(__mysql_sandbox_port1, "root");
shell.connect("mysql://root:root@localhost:"+__mysql_sandbox_port1);
c = dba.createCluster("dev");

//@<OUT> status before stop GR
c.status();

session.runSql("stop group_replication");
//@# status after stop GR - error
c.status();

//c.disconnect();

// metadata should be there now, but GR is not running and since this is a
// single node, we have full outage
// super_read_only is also supposed to be on
//@# getCluster() - error
dba.getCluster();

// Reboot with no prompt
// Note: this test was originally checking for the case where there are no
// connected sessions, but in reality there will always be at least one from
// the session doing the reboot
//
// shell.connect("mysql://root:root@localhost:"+__mysql_sandbox_port1);
//
// //@<OUT> Check reboot
// var c = dba.rebootClusterFromCompleteOutage('dev');
//
// //c.disconnect();
// session.close();
//
// clean_dead_cluster();


//@<OUT> No flag, yes on prompt
testutil.expectPrompt("Do you want to disable super_read_only and continue? [y|N]: ", "y");

var c = dba.rebootClusterFromCompleteOutage('dev');

// Kill cluster back
session.runSql("stop group_replication");

//@<OUT> No flag, no on prompt
testutil.expectPrompt("Do you want to disable super_read_only and continue? [y|N]: ", "n");

var c = dba.rebootClusterFromCompleteOutage('dev');


//@# Invalid flag value
var c = dba.rebootClusterFromCompleteOutage('dev', {clearReadOnly: 'NotABool'});

//@# Flag false
var c = dba.rebootClusterFromCompleteOutage('dev', {clearReadOnly: false});


//@<OUT> Flag true
var c = dba.rebootClusterFromCompleteOutage('dev', {clearReadOnly: true});

session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
