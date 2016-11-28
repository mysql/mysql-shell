// Assumptions: check_slave_online_multimaster and check_slave_offline_multimaster
// are defined

//@<OUT> Dba: createCluster multiMaster with interaction, cancel
dba.createCluster('devCluster', {multiMaster: true});

//@<OUT> Dba: createCluster multiMaster with interaction, ok
dba.createCluster('devCluster', {multiMaster: true});

var Cluster = dba.getCluster('devCluster');

var uri1 = "localhost:" + __mysql_sandbox_port1;
var uri2 = "localhost:" + __mysql_sandbox_port2;
var uri3 = "localhost:" + __mysql_sandbox_port3;

//@ Cluster: addInstance with interaction, error
Cluster.addInstance({host: "localhost", port:__mysql_sandbox_port1});

//@<OUT> Cluster: addInstance with interaction, ok
Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2});

check_slave_online_multimaster(Cluster, uri2);

//@<OUT> Cluster: addInstance 3 with interaction, ok
Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3});

check_slave_online_multimaster(Cluster, uri3);

//@<OUT> Cluster: describe1
Cluster.describe()

//@<OUT> Cluster: status1
Cluster.status()

//@ Cluster: removeInstance
Cluster.removeInstance({host: "localhost", port:__mysql_sandbox_port2})

//@<OUT> Cluster: describe2
Cluster.describe()

//@<OUT> Cluster: status2
Cluster.status()

//@ Cluster: remove_instance 3
Cluster.removeInstance({host: "localhost", port:__mysql_sandbox_port3})

//@ Cluster: remove_instance last
Cluster.removeInstance({host: "localhost", port:__mysql_sandbox_port1})

//@<OUT> Cluster: describe3
Cluster.describe()

//@<OUT> Cluster: status3
Cluster.status()

//@<OUT> Cluster: addInstance with interaction, ok 2
Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port1});

//@<OUT> Cluster: addInstance with interaction, ok 3
Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2});

check_slave_online_multimaster(Cluster, uri2);

//@<OUT> Cluster: addInstance with interaction, ok 4
Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3});

check_slave_online_multimaster(Cluster, uri3);

//@<OUT> Cluster: status: success
Cluster.status()

// Rejoin tests

//@# Dba: kill instance 3
if (__sandbox_dir)
  dba.killSandboxInstance(__mysql_sandbox_port3, {sandboxDir:__sandbox_dir})
else
  dba.killSandboxInstance(__mysql_sandbox_port3)

// XCOM needs time to kick out the member of the group. The GR team has a patch to fix this
// But won't be available for the GA release. So we need to wait until the instance is reported
// as offline
check_slave_offline_multimaster(Cluster, uri3);

//@# Dba: start instance 3
if (__sandbox_dir)
  dba.startSandboxInstance(__mysql_sandbox_port3, {sandboxDir: __sandbox_dir})
else
  dba.startSandboxInstance(__mysql_sandbox_port3)

//@: Cluster: rejoinInstance errors
Cluster.rejoinInstance();
Cluster.rejoinInstance(1,2,3);
Cluster.rejoinInstance(1);
Cluster.rejoinInstance({host: "localhost"});
Cluster.rejoinInstance({host: "localhost", schema: "abs", authMethod:56});
Cluster.rejoinInstance("somehost:3306");

//@<OUT> Cluster: rejoinInstance with interaction, ok
Cluster.rejoinInstance({dbUser: "root", host: "localhost", port: __mysql_sandbox_port3});

check_slave_online_multimaster(Cluster, uri3);

// Verify if the cluster is OK

//@<OUT> Cluster: status for rejoin: success
Cluster.status()


// We cannot test the output of dissolve because it will crash the rejoined instance, hitting the bug:
// BUG#24818604: MYSQLD CRASHES WHILE STARTING GROUP REPLICATION FOR A NODE IN RECOVERY PROCESS
// As soon as the bug is fixed, dissolve will work fine and we can remove the above workaround to do a clean-up

//Cluster.dissolve({force: true})

//Cluster.describe()

//Cluster.status()
