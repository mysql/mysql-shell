// Assumptions: New sandboxes deployed with no server id in the config file.
// Regression for BUG#26818744 : MYSQL SHELL DOESN'T ADD THE SERVER_ID ANYMORE

//@ Configure instance on port 1.
var cnfPath1 = __sandbox_dir + __mysql_sandbox_port1 + "/my.cnf";
dba.configureLocalInstance("root@localhost:"+__mysql_sandbox_port1, {mycnfPath: cnfPath1, dbPassword:'root'});

//@ Configure instance on port 2.
var cnfPath2 = __sandbox_dir + __mysql_sandbox_port2 + "/my.cnf";
dba.configureLocalInstance("root@localhost:"+__mysql_sandbox_port2, {mycnfPath: cnfPath2, dbPassword:'root'});

//@ Configure instance on port 3.
var cnfPath3 = __sandbox_dir + __mysql_sandbox_port3 + "/my.cnf";
dba.configureLocalInstance("root@localhost:"+__mysql_sandbox_port3, {mycnfPath: cnfPath3, dbPassword:'root'});

//@ Restart instance on port 1 to apply new server id settings.
dba.stopSandboxInstance(__mysql_sandbox_port1, {password: 'root', sandboxDir: __sandbox_dir});
try_restart_sandbox(__mysql_sandbox_port1);

//@ Restart instance on port 2 to apply new server id settings.
dba.stopSandboxInstance(__mysql_sandbox_port2, {password: 'root', sandboxDir: __sandbox_dir});
try_restart_sandbox(__mysql_sandbox_port2);

//@ Restart instance on port 3 to apply new server id settings.
dba.stopSandboxInstance(__mysql_sandbox_port3, {password: 'root', sandboxDir: __sandbox_dir});
try_restart_sandbox(__mysql_sandbox_port3);

//@ Connect to instance on port 1.
shell.connect({host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

//@ Create cluster with success.
if (__have_ssl)
    var cluster = dba.createCluster('bug26818744', {memberSslMode: 'REQUIRED', clearReadOnly: true});
else
    var cluster = dba.createCluster('bug26818744', {memberSslMode: 'DISABLED', clearReadOnly: true});

//@ Add instance on port 2 to cluster with success.
add_instance_to_cluster(cluster, __mysql_sandbox_port2);
wait_slave_state(cluster, uri2, "ONLINE");

//@ Add instance on port 3 to cluster with success.
add_instance_to_cluster(cluster, __mysql_sandbox_port3);
wait_slave_state(cluster, uri3, "ONLINE");

//@ Remove instance on port 2 from cluster with success.
cluster.removeInstance('root:root@localhost:' + __mysql_sandbox_port2);

//@ Remove instance on port 3 from cluster with success.
cluster.removeInstance('root:root@localhost:' + __mysql_sandbox_port3);

//@ Dissolve cluster with success
cluster.dissolve({force: true});
