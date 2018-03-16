//@ dba.checkInstanceConfiguration() requires IPv4 connection data
||Dba.checkInstanceConfiguration: Connection 'root@[::1]:<<<__mysql_sandbox_port2>>>' is not valid: ::1 is an IPv6 address, which is not supported by the Group Replication. Please ensure an IPv4 address is used when setting up an InnoDB cluster. (ArgumentError)

//@ dba.checkInstanceConfiguration() requires a valid IPv4 hostname
||Dba.checkInstanceConfiguration: Connection 'root@invalid_hostname:3456' is not valid: unable to resolve the IPv4 address. (ArgumentError)

//@ dba.configureLocalInstance() requires IPv4 connection data
||Dba.configureLocalInstance: Connection 'root@[::1]:<<<__mysql_sandbox_port2>>>' is not valid: ::1 is an IPv6 address, which is not supported by the Group Replication. Please ensure an IPv4 address is used when setting up an InnoDB cluster. (ArgumentError)

//@ dba.checkInstanceConfiguration() using host that resolves to IPv4 (no error)
|"status": "ok"|

//@ dba.configureLocalInstance() using host that resolves to IPv4 (no error)
||

//@ dba.createCluster() requires IPv4 connection
||Dba.createCluster: Connection 'root@[::1]:<<<__mysql_sandbox_port1>>>' is not valid: ::1 is an IPv6 address, which is not supported by the Group Replication. Please ensure an IPv4 address is used when setting up an InnoDB cluster. (RuntimeError)

//@ cluster.addInstance() requires IPv4 connection data
||Cluster.addInstance: Connection 'root@[::1]:<<<__mysql_sandbox_port2>>>' is not valid: ::1 is an IPv6 address, which is not supported by the Group Replication. Please ensure an IPv4 address is used when setting up an InnoDB cluster. (ArgumentError)

//@ cluster.addInstance() requires a valid IPv4 hostname
||Cluster.addInstance: Connection 'root@invalid_hostname:3456' is not valid: unable to resolve the IPv4 address. (ArgumentError)

//@ cluster.rejoinInstance() requires IPv4 connection data
||Cluster.rejoinInstance: Connection 'root@[::1]:<<<__mysql_sandbox_port2>>>' is not valid: ::1 is an IPv6 address, which is not supported by the Group Replication. Please ensure an IPv4 address is used when setting up an InnoDB cluster. (ArgumentError)

//@ cluster.rejoinInstance() requires a valid IPv4 hostname
||Cluster.rejoinInstance: Connection 'root@invalid_hostname:3456' is not valid: unable to resolve the IPv4 address. (ArgumentError)

//@ cluster.removeInstance() requires IPv4 connection data
||Cluster.removeInstance: Connection 'root@[::1]:<<<__mysql_sandbox_port2>>>' is not valid: ::1 is an IPv6 address, which is not supported by the Group Replication. Please ensure an IPv4 address is used when setting up an InnoDB cluster. (ArgumentError)

//@ cluster.removeInstance() requires a valid IPv4 hostname
||Cluster.removeInstance: Connection 'root@invalid_hostname:3456' is not valid: unable to resolve the IPv4 address. (ArgumentError)

//@ cluster.checkInstanceState() requires IPv4 connection data
||Cluster.checkInstanceState: Connection 'root@[::1]:<<<__mysql_sandbox_port2>>>' is not valid: ::1 is an IPv6 address, which is not supported by the Group Replication. Please ensure an IPv4 address is used when setting up an InnoDB cluster. (ArgumentError)

//@ cluster.checkInstanceState() requires a valid IPv4 hostname
||Cluster.checkInstanceState: Connection 'root@invalid_hostname:3456' is not valid: unable to resolve the IPv4 address. (ArgumentError)

//@ cluster.forceQuorumUsingPartitionOf() requires IPv4 connection data
||Cluster.forceQuorumUsingPartitionOf: Connection 'root@[::1]:<<<__mysql_sandbox_port2>>>' is not valid: ::1 is an IPv6 address, which is not supported by the Group Replication. Please ensure an IPv4 address is used when setting up an InnoDB cluster. (ArgumentError)

//@ cluster.forceQuorumUsingPartitionOf() requires a valid IPv4 hostname
||Cluster.forceQuorumUsingPartitionOf: Connection 'root@invalid_hostname:3456' is not valid: unable to resolve the IPv4 address. (ArgumentError)

//@ dba.dropMetadataSchema() requires IPv4 connection
||Dba.dropMetadataSchema: Connection 'root@[::1]:<<<__mysql_sandbox_port1>>>' is not valid: ::1 is an IPv6 address, which is not supported by the Group Replication. Please ensure an IPv4 address is used when setting up an InnoDB cluster. (RuntimeError)

//@ dba.getCluster() requires IPv4 connection
||Dba.getCluster: Connection 'root@[::1]:<<<__mysql_sandbox_port1>>>' is not valid: ::1 is an IPv6 address, which is not supported by the Group Replication. Please ensure an IPv4 address is used when setting up an InnoDB cluster. (RuntimeError)

//@ dba.rebootClusterFromCompleteOutage() requires IPv4 connection
||Dba.rebootClusterFromCompleteOutage: Connection 'root@[::1]:<<<__mysql_sandbox_port1>>>' is not valid: ::1 is an IPv6 address, which is not supported by the Group Replication. Please ensure an IPv4 address is used when setting up an InnoDB cluster. (RuntimeError)
