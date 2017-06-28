// Assumptions: smart deployment rountines available
//@ Initialization
var deployed_here = reset_or_deploy_sandboxes();

shell.connect({scheme: 'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

//@ create cluster
if (__have_ssl)
  var cluster = dba.createCluster('devCluster', {memberSslMode:'REQUIRED'});
else
  var cluster = dba.createCluster('devCluster');

//@ Validates the createCluster successfully configured the grLocal member of the instance addresses
var gr_local_port = __mysql_sandbox_port1 + 10000;
var res = session.runSql('select json_unquote(addresses->\'$.grLocal\') from mysql_innodb_cluster_metadata.instances where addresses->\'$.mysqlClassic\' = \'localhost:' + __mysql_sandbox_port1 + '\'');
var row = res.fetchOne();
print (row[0]);


//@ Adding the remaining instances
add_instance_to_cluster(cluster, __mysql_sandbox_port2, 'second_sandbox');
add_instance_to_cluster(cluster, __mysql_sandbox_port3, 'third_sandbox');
wait_slave_state(cluster, 'second_sandbox', "ONLINE");
wait_slave_state(cluster, 'third_sandbox', "ONLINE");

//@<OUT> Healthy cluster status
cluster.status()

//@ Kill instance, will not auto join after start
dba.killSandboxInstance(__mysql_sandbox_port3, {sandboxDir:__sandbox_dir});
wait_slave_state(cluster, 'third_sandbox', ["UNREACHABLE", "OFFLINE"]);

//@ Start instance, will not auto join the cluster
dba.startSandboxInstance(__mysql_sandbox_port3, {sandboxDir: __sandbox_dir});
wait_slave_state(cluster, 'third_sandbox', ["OFFLINE", "(MISSING)"]);

//@<OUT> Still missing 3rd instance
os.sleep(5)
cluster.status()

//@#: Rejoins the instance
cluster.rejoinInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3}, {memberSslMode: "AUTO", "password": "root"});
wait_slave_state(cluster, 'third_sandbox', "ONLINE");

//@<OUT> Instance is back
cluster.status()

//@ Persist the GR configuration
var cnfPath = __sandbox_dir + __mysql_sandbox_port3 + "/my.cnf";
var result = dba.configureLocalInstance('root@localhost:' + __mysql_sandbox_port3, {mycnfPath: cnfPath, dbPassword:'root'});
print (result.status)

//@ Kill instance, will auto join after start
dba.killSandboxInstance(__mysql_sandbox_port3, {sandboxDir:__sandbox_dir});
wait_slave_state(cluster, 'third_sandbox', ["UNREACHABLE", "OFFLINE"]);

//@ Start instance, will auto join the cluster
dba.startSandboxInstance(__mysql_sandbox_port3, {sandboxDir: __sandbox_dir});
wait_slave_state(cluster, 'third_sandbox', "ONLINE");

//@<OUT> Check saved auto_inc settings are restored
shell.connect({scheme: 'mysql', host: localhost, port: __mysql_sandbox_port3, user: 'root', password: 'root'});
session.runSql("show global variables like 'auto_increment_%'").fetchAll();

//@ Finalization
if (deployed_here)
  cleanup_sandbox(__mysql_sandbox_port1);
