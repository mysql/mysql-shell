// deploy sandbox
testutil.deploySandbox(__mysql_sandbox_port1, 'root');
testutil.deploySandbox(__mysql_sandbox_port2, 'root');
testutil.snapshotSandboxConf(__mysql_sandbox_port2);

// connect using IPv6 address
shell.connect({user: 'root', password: 'root', host: '::1', port: __mysql_sandbox_port1});

// dba methods which can be executed without a cluster

//@ dba.checkInstanceConfiguration() requires IPv4 connection data
dba.checkInstanceConfiguration("root:root@[::1]:" + __mysql_sandbox_port2);

//@ dba.checkInstanceConfiguration() requires a valid IPv4 hostname
dba.checkInstanceConfiguration("root:root@invalid_hostname:3456");

//@ dba.configureLocalInstance() requires IPv4 connection data
dba.configureLocalInstance("root:root@[::1]:" + __mysql_sandbox_port2);

// dba.configureLocalInstance() checks if host is local, cannot test hostname

//@ dba.checkInstanceConfiguration() using host that resolves to IPv4 (no error)
dba.checkInstanceConfiguration("root:root@localhost:" + __mysql_sandbox_port2);

//@ dba.configureLocalInstance() using host that resolves to IPv4 (no error)
dba.configureLocalInstance("root:root@localhost:" + __mysql_sandbox_port2, {clusterAdmin: "ca", clusterAdminPassword: "ca", mycnfPath: testutil.getSandboxConfPath(__mysql_sandbox_port2)});

//@ dba.createCluster() requires IPv4 connection
var cluster = dba.createCluster("dev");

// reconnect using IPv4 address, create cluster
session.close();
shell.connect({user: 'root', password: 'root', host: 'localhost', port: __mysql_sandbox_port1});
var cluster = dba.createCluster("dev");

// it's not possible to create or get an existing cluster using IPv6 connection,
// hence it's not possible to call any of the cluster's methods via IPv6 session

// cluster methods which take connection data as an argument

//@ cluster.addInstance() requires IPv4 connection data
cluster.addInstance("root:root@[::1]:" + __mysql_sandbox_port2);

//@ cluster.addInstance() requires a valid IPv4 hostname
cluster.addInstance("root:root@invalid_hostname:3456");

//@ cluster.rejoinInstance() requires IPv4 connection data
cluster.rejoinInstance("root:root@[::1]:" + __mysql_sandbox_port2);

//@ cluster.rejoinInstance() requires a valid IPv4 hostname
cluster.rejoinInstance("root:root@invalid_hostname:3456");

//@ cluster.removeInstance() requires IPv4 connection data
cluster.removeInstance("root:root@[::1]:" + __mysql_sandbox_port2);

//@ cluster.removeInstance() requires a valid IPv4 hostname
cluster.removeInstance("root:root@invalid_hostname:3456");

//@ cluster.checkInstanceState() requires IPv4 connection data
cluster.checkInstanceState("root:root@[::1]:" + __mysql_sandbox_port2);

//@ cluster.checkInstanceState() requires a valid IPv4 hostname
cluster.checkInstanceState("root:root@invalid_hostname:3456");

//@ cluster.forceQuorumUsingPartitionOf() requires IPv4 connection data
cluster.forceQuorumUsingPartitionOf("root:root@[::1]:" + __mysql_sandbox_port2);

//@ cluster.forceQuorumUsingPartitionOf() requires a valid IPv4 hostname
cluster.forceQuorumUsingPartitionOf("root:root@invalid_hostname:3456");

// reconnect using IPv6 address
session.close();
shell.connect({user: 'root', password: 'root', host: '::1', port: __mysql_sandbox_port1});

// dba methods which require an existing cluster

//@ dba.dropMetadataSchema() requires IPv4 connection
dba.dropMetadataSchema({force: true});

//@ dba.getCluster() requires IPv4 connection
dba.getCluster("dev");

//@ dba.rebootClusterFromCompleteOutage() requires IPv4 connection
dba.rebootClusterFromCompleteOutage();

// Smart deployment cleanup
cluster.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
