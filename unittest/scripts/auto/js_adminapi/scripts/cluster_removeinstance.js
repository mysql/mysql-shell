function get_all_users() {
  var result = session.runSql("select concat(user,'@',host) from mysql.user");
  return result.fetchAll();
}

// BUG#29617572: DBA.REMOVEINSTANCE() WITH EMPTY MYSQL.SLAVE_MASTER_INFO DROPS ALL USERS
//
// Removing an instance from a cluster when the instance to be removed has no
// user defined for channel group_replication_recovery in
// mysql.slave_master_info results in dropping ALL users from mysql.user on the
// remaining instances of the cluster.

//@<> BUG#29617572: Initialization
testutil.deploySandbox(__mysql_sandbox_port1, 'root', {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, 'root', {report_host: hostname});

//@<> BUG#29617572: Create cluster
shell.connect(__sandbox_uri1);
var c = dba.createCluster('test');

var all_users_instance1 = get_all_users();

//@<> BUG#29617572: Add instance to the cluster
c.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> BUG#29617572: Clear the mysql.slave_master_info table on instance 2
session.close();
shell.connect(__sandbox_uri2);
session.runSql('STOP GROUP_REPLICATION');
session.runSql('RESET SLAVE ALL');
c.removeInstance(__sandbox_uri2, {force:true});

//@<> BUG#29617572: Verify that no extra users were removed from instance 1
session.close();
shell.connect(__sandbox_uri1);
EXPECT_EQ(all_users_instance1, get_all_users());
session.close();

//@<> Finalization
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
