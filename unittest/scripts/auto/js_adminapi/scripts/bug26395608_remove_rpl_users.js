// Deploy instances.
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.deploySandbox(__mysql_sandbox_port3, "root");

function get_number_of_rpl_users() {
    var result = session.runSql(
        "SELECT COUNT(*) FROM INFORMATION_SCHEMA.USER_PRIVILEGES " +
        "WHERE GRANTEE REGEXP \"'mysql_innodb_cluster_r[0-9]{10}.*\"");
    var row = result.fetchOne();
    return row[0];
}

//@ Create a cluster with 3 members.
shell.connect(__sandbox_uri1);
var cluster = dba.createCluster("test_cluster");

cluster.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

cluster.addInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@ Get initial number of replication users.
var init_num_rpl_users = get_number_of_rpl_users();

//@ Remove last added instance.
cluster.removeInstance(__sandbox_uri3);

//@ Connect to removed instance.
session.close();
cluster.disconnect();
shell.connect(__sandbox_uri3);

//@<OUT> Confirm that all replication users where removed.
print(get_number_of_rpl_users() + "\n");

//@ Connect back to primary and get cluster.
session.close();
shell.connect(__sandbox_uri1);
var cluster = dba.getCluster("test_cluster");

//@<OUT> Confirm that some replication user was removed.
var less_rpl_users = init_num_rpl_users > get_number_of_rpl_users();
print(less_rpl_users + "\n");

//@ Dissolve cluster.
cluster.dissolve({force: true});

//@<OUT> Confirm that replication users where removed on primary.
print(get_number_of_rpl_users() + "\n");

//@ Connect to remaining instance.
session.close();
cluster.disconnect();
shell.connect(__sandbox_uri2);

//@<OUT> Confirm that replication users where removed on remaining instance.
print(get_number_of_rpl_users() + "\n");

// Clean-up deployed instances.
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);