// deploy sandbox
testutil.deploySandbox(__mysql_sandbox_port1, 'root');
shell.connect({user: 'root', password: 'root', host: 'localhost', port: __mysql_sandbox_port1});

// create user which doesn't have enough privileges to be a cluster admin
session.runSql("CREATE USER ic@localhost IDENTIFIED BY 'icpass'");
session.runSql("GRANT SELECT, RELOAD, SHUTDOWN, PROCESS, FILE, SUPER, REPLICATION SLAVE, REPLICATION CLIENT, CREATE USER ON *.* TO 'ic'@'localhost' WITH GRANT OPTION");
session.runSql("GRANT SELECT, INSERT, UPDATE, DELETE ON `mysql`.* TO 'ic'@'localhost' WITH GRANT OPTION");

// log in as created user
shell.connect({user: 'ic', password: 'icpass', host: 'localhost', port: __mysql_sandbox_port1});

//@# creation should fail without segmentation fault
var cluster = dba.createCluster('Cluster_R', {gtidSetIsComplete: true});

// Smart deployment cleanup
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
