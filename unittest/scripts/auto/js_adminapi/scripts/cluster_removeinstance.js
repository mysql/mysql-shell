function get_all_users() {
  var result = session.runSql("select concat(user,'@',host) from mysql.user");
  return result.fetchAll();
}

function get_number_of_rpl_users() {
  var result = session.runSql(
      "SELECT COUNT(*) FROM INFORMATION_SCHEMA.USER_PRIVILEGES " +
      "WHERE GRANTEE REGEXP 'mysql_innodb_cluster_r[0-9]{10}.*'");
  var row = result.fetchOne();
  return row[0];
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

//@<> BUG#29617572: Get all users from instance 2
shell.connect(__sandbox_uri2);
var all_users_instance2 = get_all_users();
session.close();

//@<> BUG#29617572: Create cluster
shell.connect(__sandbox_uri1);
var c = dba.createCluster('test');

//@<> BUG#29617572: Add instance to the cluster
c.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> BUG#29617572: Get all users from instance 1
var all_users_instance1 = get_all_users();

//@<> BUG#29617572: Clear the mysql.slave_master_info table on instance 2
session.close();
shell.connect(__sandbox_uri2);
session.runSql('STOP GROUP_REPLICATION');
session.runSql('RESET SLAVE ALL');

//@<> BUG#29617572: Confirm that the replication users were created when adding the instance
EXPECT_EQ(4, get_number_of_rpl_users());

//@<> BUG#29617572: Remove instance 2 from the cluster
c.removeInstance(__sandbox_uri2, {force: true});

// TODO: Remove this validation as soon as BUG#29559303 is fixed
//@<> BUG#29617572: Verify that the recovery users were removed
EXPECT_EQ(0, get_number_of_rpl_users());

//@<> BUG#29617572: Verify that only the recovery users where removed on the target instance 2
var all_users_instance2_after = get_all_users();
EXPECT_EQ(all_users_instance2, all_users_instance2_after);

//@<> BUG#29617572: Verify that no users were removed from instance 1
session.close();
shell.connect(__sandbox_uri1);
var all_users_instance1_after = get_all_users();
EXPECT_EQ(all_users_instance1, all_users_instance1_after);

//@<> BUG#29617572: Finalization
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
