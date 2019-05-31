function get_all_users() {
  var result = session.runSql(
    "SELECT concat(user,'@',host) FROM mysql.user " +
    "WHERE user NOT REGEXP 'mysql_innodb_cluster_r[0-9]{10}.*'");
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
var c = dba.createCluster('test', {gtidSetIsComplete: true});

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

// WL#13208 AdminAPI: Automatic handling of Clone GR integration

//Since clone copies all the data, including mysql.slave_master_info (where the CHANGE MASTER credentials are stored),
// the following issue may happen:
//
//1) Group is bootstrapped on server1.
//    user_account:  mysql_innodb_cluster_1
//2) server2 is added to the group
//    user_account:  mysql_innodb_cluster_2
//    But server2 is cloned from server1, which means its recovery account
//    will be mysql_innodb_cluster_1
//3) server3 is added to the group
//    user_account:  mysql_innodb_cluster_3
//    But server3 is cloned from server1, which means its recovery account
//    will be mysql_innodb_cluster_1
//4) server1 is removed from the group
//    mysql_innodb_cluster_1 account is dropped on all group servers.
//
//To avoid such situation, we will re-issue the CHANGE MASTER query after clone to ensure the right account is used.
//On top of that, we will make sure that the account is not dropped if any other cluster member is using it.
//
// To verify this fix, we do the following:
// 1 - create a cluster
// 2 - add an instance using clone
// 3 - add another instance using clone
// 4 - Remove the seed instance
// 5 - Re-add seed instance
// 6 - Remove seed instance
// 7 - Dissolve cluster

//@<> WL#13208: Initialization {VER(>=8.0.17)}
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
shell.connect(__sandbox_uri1);
var c = dba.createCluster('test');

//@<> WL#13208: Add instance 2 using clone {VER(>=8.0.17)}
c.addInstance(__sandbox_uri2, {recoveryMethod: "clone"});

//@<> WL#13208: Add instance 3 using clone {VER(>=8.0.17)}
c.addInstance(__sandbox_uri3, {recoveryMethod: "clone"});

//@<> WL#13208: Remove the seed instance {VER(>=8.0.17)}
c.removeInstance(__sandbox_uri1);

//@<> WL#13208: Re-add seed instance {VER(>=8.0.17)}
session.close();
shell.connect(__sandbox_uri2);
var c = dba.getCluster()
c.addInstance(__sandbox_uri1, {recoveryMethod: "incremental"});

//@ WL#13208: Cluster status {VER(>=8.0.17)}
c.status();

//@<> WL#13208: Re-remove seed instance {VER(>=8.0.17)}
c.removeInstance(__sandbox_uri1);

//@ WL#13208: Cluster status 2 {VER(>=8.0.17)}
c.status();

//@<> WL#13208: Dissolve cluster {VER(>=8.0.17)}
c.dissolve({force: true});

//@<> WL#13208: Finalization {VER(>=8.0.17)}
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
