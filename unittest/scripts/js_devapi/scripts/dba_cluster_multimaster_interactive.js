// Assumptions: check_slave_online_multimaster and check_slave_offline_multimaster
// are defined

//@<OUT> Dba: createCluster multiMaster with interaction, cancel
dba.createCluster('devCluster', {multiMaster: true});

//@<OUT> Dba: createCluster multiMaster with interaction, ok
if (__have_ssl)
  dba.createCluster('devCluster', {multiMaster: true, memberSslMode: 'REQUIRED'});
else
  dba.createCluster('devCluster', {multiMaster: true});

var Cluster = dba.getCluster('devCluster');

//@ Cluster: addInstance with interaction, error
add_instance_options['port'] = __mysql_sandbox_port1;
Cluster.addInstance(add_instance_options, add_instance_extra_opts);

//@<OUT> Cluster: addInstance with interaction, ok
add_instance_to_cluster(Cluster, __mysql_sandbox_port2);

wait_slave_state(Cluster, uri2, "ONLINE")

//@<OUT> Cluster: addInstance 3 with interaction, ok
add_instance_to_cluster(Cluster, __mysql_sandbox_port3);

wait_slave_state(Cluster, uri3, "ONLINE")

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
add_instance_to_cluster(Cluster, __mysql_sandbox_port1);

wait_slave_state(Cluster, uri1, "ONLINE")

//@<OUT> Cluster: addInstance with interaction, ok 3
add_instance_to_cluster(Cluster, __mysql_sandbox_port2);

wait_slave_state(Cluster, uri2, "ONLINE")

//@<OUT> Cluster: addInstance with interaction, ok 4
add_instance_to_cluster(Cluster, __mysql_sandbox_port3);

wait_slave_state(Cluster, uri3, "ONLINE")

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
wait_slave_state(Cluster, uri3, ["OFFLINE", "UNREACHABLE"])

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
if (__have_ssl)
  Cluster.rejoinInstance({dbUser: "root", host: "localhost", port: __mysql_sandbox_port3, memberSslMode: 'REQUIRED'});
else
  Cluster.rejoinInstance({dbUser: "root", host: "localhost", port: __mysql_sandbox_port3});

wait_slave_state(Cluster, uri3, "ONLINE")

// Verify if the cluster is OK

//@<OUT> Cluster: status for rejoin: success
Cluster.status()


Cluster.dissolve({force: true})
