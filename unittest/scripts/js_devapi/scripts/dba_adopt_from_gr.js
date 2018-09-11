// Assumptions: smart deployment rountines available

//@ Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");

function get_rpl_users() {
    var result = session.runSql(
        "SELECT GRANTEE FROM INFORMATION_SCHEMA.USER_PRIVILEGES " +
        "WHERE GRANTEE REGEXP \"'mysql_innodb_cluster_r[0-9]{10}.*\"");
    return result.fetchAll();
}

function has_new_rpl_users(rows) {
    var sql =
        "SELECT GRANTEE FROM INFORMATION_SCHEMA.USER_PRIVILEGES " +
        "WHERE GRANTEE REGEXP \"'mysql_innodb_cluster_r[0-9]{10}.*\" " +
        "AND GRANTEE NOT IN (";
    for (i = 0; i < rows.length; i++) {
        sql += "\"" + rows[i][0] + "\"";
        if (i != rows.length-1) {
            sql += ", ";
        }
    }
    sql += ")";
    var result = session.runSql(sql);
    var new_user_rows = result.fetchAll();
    if (new_user_rows.length == 0) {
        return false;
    } else {
        return true;
    }
}

// by default, root account can connect only via localhost, create 'root'@'%'
// so it's possible to connect via hostname
shell.connect(__sandbox_uri2);

session.runSql("SET sql_log_bin = 0");
session.runSql("CREATE USER 'root'@'%' IDENTIFIED BY 'root'");
session.runSql("GRANT ALL PRIVILEGES ON *.* to 'root'@'%' WITH GRANT OPTION");
session.runSql("SET sql_log_bin = 1");

shell.connect(__sandbox_uri1);

session.runSql("SET sql_log_bin = 0");
session.runSql("CREATE USER 'root'@'%' IDENTIFIED BY 'root'");
session.runSql("GRANT ALL PRIVILEGES ON *.* to 'root'@'%' WITH GRANT OPTION");
session.runSql("SET sql_log_bin = 1");

//@ it's not possible to adopt from GR without existing group replication
dba.createCluster('testCluster', {adoptFromGR: true});

// create cluster with two instances and drop metadata schema

//@ Create cluster
if (__have_ssl)
  var cluster = dba.createCluster('testCluster', {memberSslMode: 'REQUIRED'});
else
  var cluster = dba.createCluster('testCluster', {memberSslMode: 'DISABLED'});

//@ Adding instance to cluster
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
cluster.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

// To simulate an existing unmanaged replication group we simply drop the
// metadata schema

//@ Drop Metadata
dba.dropMetadataSchema({force: true})

//@ Check cluster status after drop metadata schema
cluster.status();

session.close();
cluster.disconnect();

// Establish a session using the real hostname
// because when adopting from GR, the information in the
// performance_schema.replication_group_members will have the real hostname
// and not 'localhost'
shell.connect({scheme:'mysql', host: real_hostname, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

//@ Get data about existing replication users before createCluster with adoptFromGR.
// Regression for BUG#28054500: replication users should be removed when adoptFromGR is used
var rpl_users_rows = get_rpl_users();

//@ Create cluster adopting from GR
var cluster = dba.createCluster('testCluster', {adoptFromGR: true});

//@<OUT> Confirm no new replication user was created.
// Regression for BUG#28054500: replication users should be removed when adoptFromGR is used
print(has_new_rpl_users(rpl_users_rows) + "\n");

//@<OUT> Check cluster status
cluster.status();

//@ dissolve the cluster
cluster.dissolve({force: true});

//@ it's not possible to adopt from GR when cluster was dissolved
dba.createCluster('testCluster', {adoptFromGR: true});

// Close session
session.close();

//@ Finalization
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
