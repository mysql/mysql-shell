// Assumptions: smart deployment routines available
//@ Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});

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

shell.connect(__sandbox_uri2);

// Create root@<hostname> account with all privileges, required to create a
// cluster.
session.runSql("SET sql_log_bin = 0");
session.runSql("CREATE USER 'root'@'"+hostname+"' IDENTIFIED BY 'root'");
session.runSql("GRANT ALL PRIVILEGES ON *.* to 'root'@'"+hostname+"' WITH GRANT OPTION");
session.runSql("CREATE USER 'root'@'"+hostname_ip+"' IDENTIFIED BY 'root'");
session.runSql("GRANT ALL PRIVILEGES ON *.* to 'root'@'"+hostname_ip+"' WITH GRANT OPTION");
session.runSql("SET sql_log_bin = 1");

shell.connect(__sandbox_uri1);

// Create root@<hostname> account with all privileges, required to create a
// cluster.
session.runSql("SET sql_log_bin = 0");
session.runSql("CREATE USER 'root'@'"+hostname+"' IDENTIFIED BY 'root'");
session.runSql("GRANT ALL PRIVILEGES ON *.* to 'root'@'"+hostname+"' WITH GRANT OPTION");
session.runSql("CREATE USER 'root'@'"+hostname_ip+"' IDENTIFIED BY 'root'");
session.runSql("GRANT ALL PRIVILEGES ON *.* to 'root'@'"+hostname_ip+"' WITH GRANT OPTION");
session.runSql("SET sql_log_bin = 1");

//@ it's not possible to adopt from GR without existing group replication
dba.createCluster('testCluster', {adoptFromGR: true});

//@ Create cluster
var cluster = dba.createCluster('testCluster', {memberSslMode: 'DISABLED'});

//@ Adding instance to cluster
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
cluster.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

// To simulate an existing unmanaged replication group we simply drop the
// metadata schema

//@<OUT> Drop Metadata
dba.dropMetadataSchema()

//@ Check cluster status after drop metadata schema
cluster.status();

session.close();

// Establish a session using the hostname
// because when adopting from GR, the information in the
// performance_schema.replication_group_members will have the hostname
// and not 'localhost'
shell.connect({scheme:'mysql', host: hostname, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

cluster.disconnect();

//@ Get data about existing replication users before createCluster with adoptFromGR.
// Regression for BUG#28054500: replication users should be removed when adoptFromGR is used
var rpl_users_rows = get_rpl_users();

//@<OUT> Create cluster adopting from GR - answer 'yes' to prompt
var cluster = dba.createCluster('testCluster');

//@<OUT> Confirm no new replication user was created.
// Regression for BUG#28054500: replication users should be removed when adoptFromGR is used
print(has_new_rpl_users(rpl_users_rows) + "\n");

//@<OUT> Check cluster status - success
cluster.status();

// To simulate an existing unmanaged replication group we simply drop the
// metadata schema
dba.dropMetadataSchema()

// Close session
session.close();
cluster.disconnect();

// Establish a session using the hostname
// because when adopting from GR, the information in the
// performance_schema.replication_group_members will have the hostname
// and not 'localhost'
shell.connect({scheme:'mysql', host: hostname, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

//@ Create cluster adopting from GR - answer 'no' to prompt
var cluster = dba.createCluster('testCluster');
//@ Check cluster status - failure
cluster.status();

// Close session
session.close();
cluster.disconnect();

// Establish a session using the hostname
// because when adopting from GR, the information in the
// performance_schema.replication_group_members will have the hostname
// and not 'localhost'
shell.connect({scheme:'mysql', host: hostname, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

//@<OUT> Create cluster adopting from GR - use 'adoptFromGR' option
var cluster = dba.createCluster('testCluster', {adoptFromGR: true});
//@<OUT> Check cluster status - success - 'adoptFromGR'
cluster.status();

// Dismantle the cluster
cluster.dissolve({force: true})

// Close session
session.close();
cluster.disconnect();

// create cluster in multi-primary mode
shell.connect(__sandbox_uri1);

var cluster = dba.createCluster('testCluster', {multiPrimary: true, memberSslMode: __ssl_mode, clearReadOnly: true, force: true});

testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
cluster.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

// To simulate an existing unmanaged replication group we simply drop the
// metadata schema
dba.dropMetadataSchema()

// Close session
session.close();
cluster.disconnect();

// Establish a session using the hostname
// because when adopting from GR, the information in the
// performance_schema.replication_group_members will have the hostname
// and not 'localhost'
shell.connect({scheme:'mysql', host: hostname, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

//@<OUT> Create cluster adopting from multi-primary GR - use 'adoptFromGR' option
var cluster = dba.createCluster('testCluster', {adoptFromGR: true, force: true});
//@<OUT> Check cluster status - success - adopt from multi-primary
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
