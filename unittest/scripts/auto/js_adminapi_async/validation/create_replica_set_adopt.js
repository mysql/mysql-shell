//@# INCLUDE async_utils.inc
||

//@# on a standalone server (should fail)
||Dba.createReplicaSet: Target server is not part of an async replication topology (MYSQLSH 51151)

//@# Bad options (should fail)
||Dba.createReplicaSet: Argument #1 is expected to be a string (ArgumentError)
||Dba.createReplicaSet: instanceLabel option not allowed when adoptFromAR:true (ArgumentError)
||Dba.createReplicaSet: An open session is required to perform this operation. (RuntimeError)

//@# adopt with InnoDB cluster (should fail)
||Dba.createReplicaSet: Unable to create replicaset. The instance '<<<__address1>>>' already belongs to an InnoDB cluster. Use dba.getCluster() to access it. (MYSQLSH 51305)

//@# indirect adopt with InnoDB cluster (should fail)
||Dba.createReplicaSet: group_replication active (MYSQLSH 51150)

//@# adopt with unmanaged GR (should fail)
||Dba.createReplicaSet: This function is not available through a session to an instance belonging to an unmanaged replication group (RuntimeError)

//@# indirect adopt with unmanaged GR (should fail)
||Dba.createReplicaSet: group_replication active (MYSQLSH 51150)

//@# adopt with existing replicaset (should fail)
||Dba.createReplicaSet: Unable to create replicaset. The instance '<<<__address1>>>' already belongs to a replicaset. Use dba.getReplicaSet() to access it. (MYSQLSH 51306)

//@# indirect adopt with existing replicaset (should fail)
||Dba.createReplicaSet: Unable to create replicaset. The instance '<<<__address2>>>' already belongs to a replicaset. Use dba.getReplicaSet() to access it. (MYSQLSH 51306)

//@# adopt with insufficient privs (should fail)
||Dba.createReplicaSet: Unable to detect target instance state. Please check account privileges. (RuntimeError)
|ERROR: <<<__address1>>>: could not query instance: MySQL Error 1227 (42000): Access denied; you need (at least one of) the REPLICATION SLAVE privilege(s) for this operation|
|ERROR: <<<__address1>>>: could not query instance: MySQL Error 1227 (42000): Access denied; you need (at least one of) the REPLICATION SLAVE privilege(s) for this operation|
||Dba.createReplicaSet: Access denied; you need (at least one of) the SUPER, REPLICATION CLIENT privilege(s) for this operation (RuntimeError)

//@# adopt with existing old metadata, belongs to other (should fail)
||Dba.createReplicaSet: Operation not allowed. The installed metadata version 1.0.1 is lower than the version required by Shell which is version 2.0.0. Upgrade the metadata to execute this operation. See \? dba.upgradeMetadata for additional details. (RuntimeError)

//@# indirect adopt with existing old metadata, belongs to other (should fail)
||Dba.createReplicaSet: Operation not allowed. The installed metadata version 1.0.1 is lower than the version required by Shell which is version 2.0.0. Upgrade the metadata to execute this operation. See \? dba.upgradeMetadata for additional details. (RuntimeError)

//@# replication filters (should fail)
||Dba.createReplicaSet: 127.0.0.1:<<<__mysql_sandbox_port1>>>: instance has global replication filters configured, but they are not supported in InnoDB ReplicaSets. (MYSQLSH 51150)
||Dba.createReplicaSet: 127.0.0.1:<<<__mysql_sandbox_port1>>>: instance has global replication filters configured, but they are not supported in InnoDB ReplicaSets. (MYSQLSH 51150)

//@# binlog filters (should fail)
||Dba.createReplicaSet: 127.0.0.1:<<<__mysql_sandbox_port1>>>: instance has binlog filters configured, but they are not supported in InnoDB ReplicaSets. (MYSQLSH 51150)

//@# broken repl (should fail)
|ERROR: 127.0.0.1:<<<__mysql_sandbox_port2>>> has a replication channel '' in an unexpected state.|
|Any existing replication settings must be working correctly before it can be adopted.|
||Dba.createReplicaSet: Invalid replication state (MYSQLSH 51150)

//@# bad repl channels - bad name (should fail)
|ERROR: 127.0.0.1:<<<__mysql_sandbox_port1>>> has one or more replicas with unsupported channels|
|Adopted topologies must not have any replication channels other than the default one.|
||Dba.createReplicaSet: Unsupported replication topology (MYSQLSH 51151)

//@# bad repl channels - 2 channels (should fail)
|    - replicates from localhost:<<<__mysql_sandbox_port1>>> through unsupported channel 'bla'|
|ERROR: 127.0.0.1:<<<__mysql_sandbox_port2>>> has one or more unsupported replication channels: bla|
||Dba.createReplicaSet: Unsupported replication topology (MYSQLSH 51151)

//@# bad repl channels - master has a bogus channel
|    - replicates from localhost:<<<__mysql_sandbox_port3>>> through unsupported channel 'foob'|
|ERROR: 127.0.0.1:<<<__mysql_sandbox_port1>>> has one or more unsupported replication channels: foob|
||Dba.createReplicaSet: Unsupported replication topology (MYSQLSH 51151)

//@# admin account has mismatched passwords (should fail)
|ERROR: Could not connect to 127.0.0.1:<<<__mysql_sandbox_port2>>> (slave of <<<__address1>>>): MySQL Error 1045: 127.0.0.1:<<<__mysql_sandbox_port2>>>: Access denied for user 'rooty'@'localhost' (using password: YES)|
||Dba.createReplicaSet: 127.0.0.1:<<<__mysql_sandbox_port2>>>: Access denied for user 'rooty'@'localhost' (using password: YES) (MySQL Error 1045)

//@# admin account passwords match, but they don't allow connection from the shell (should fail)
||Dba.createReplicaSet: Unable to detect target instance state. Please check account privileges. (RuntimeError)

//@# invalid topology: master-master (should fail)
|ERROR: Unable to determine the PRIMARY instance in the topology. Multi-master topologies are not supported.|
||Dba.createReplicaSet: Unsupported replication topology (MYSQLSH 51151)


//@# invalid topology: master-master-master (should fail)
|ERROR: Unable to determine the PRIMARY instance in the topology. Multi-master topologies are not supported.|
||Dba.createReplicaSet: Unsupported replication topology (MYSQLSH 51151)

//@# invalid topology: master-master-slave (should fail)
|ERROR: Unable to determine the PRIMARY instance in the topology. Multi-master topologies are not supported.|
||Dba.createReplicaSet: Unsupported replication topology (MYSQLSH 51151)

//@# bad configs: SBR (should fail)
|ERROR: 127.0.0.1:<<<__mysql_sandbox_port1>>>: Instance must be configured and validated with dba.configureReplicaSetInstance() before it can be used in a replicaset.|
||Dba.createReplicaSet: Instance check failed (MYSQLSH 51150)
|ERROR: 127.0.0.1:<<<__mysql_sandbox_port2>>>: Instance must be configured and validated with dba.configureReplicaSetInstance() before it can be used in a replicaset.|
||Dba.createReplicaSet: Instance check failed (MYSQLSH 51150)

//@# bad configs: filepos replication (should fail)
@| enforce_gtid_consistency | OFF           | ON             | Update read-only variable and restart the server |@
@| gtid_mode                | OFF           | ON             | Update read-only variable and restart the server |@
|Some variables need to be changed, but cannot be done dynamically on the server.|
|ERROR: 127.0.0.1:<<<__mysql_sandbox_port1>>>: Instance must be configured and validated with dba.configureReplicaSetInstance() before it can be used in a replicaset.|
||Dba.createReplicaSet: Instance check failed (MYSQLSH 51150)


//@# unsupported option: SSL (should fail)
||Dba.createReplicaSet: Replication option 'MASTER_SSL' at '127.0.0.1:<<<__mysql_sandbox_port2>>>' has a non-default value '1' but it is currently not supported by the AdminAPI. (MYSQLSH 51164)

//@# unsupported option: SSL=0
|ReplicaSet object successfully created for 127.0.0.1:<<<__mysql_sandbox_port1>>>.|

//@# unsupported option: delay (should fail)
||Dba.createReplicaSet: Replication option 'MASTER_DELAY' at '127.0.0.1:<<<__mysql_sandbox_port2>>>' has a non-default value '5' but it is currently not supported by the AdminAPI. (MYSQLSH 51164)

//@# unsupported option: connect_retry (should fail)
||Dba.createReplicaSet: Replication option 'MASTER_CONNECT_RETRY' at '127.0.0.1:<<<__mysql_sandbox_port2>>>' has a non-default value '4' but it is currently not supported by the AdminAPI. (MYSQLSH 51164)

//@# unsupported option: retry_count (should fail)
||Dba.createReplicaSet: Replication option 'MASTER_RETRY_COUNT' at '127.0.0.1:<<<__mysql_sandbox_port2>>>' has a non-default value '3' but it is currently not supported by the AdminAPI. (MYSQLSH 51164)

//@# unsupported option: heartbeat (should fail)
||Dba.createReplicaSet: Replication option 'MASTER_HEARTBEAT_PERIOD' at '127.0.0.1:<<<__mysql_sandbox_port2>>>' has a non-default value '32' but it is currently not supported by the AdminAPI. (MYSQLSH 51164)

//@# unsupported option: no auto_position (should fail)
||Dba.createReplicaSet: 127.0.0.1:<<<__mysql_sandbox_port2>>> uses replication without auto-positioning, which is not supported by the AdminAPI. (MYSQLSH 51164)

//@# unsupported option: MASTER_COMPRESSION_ALGORITHMS (should fail)
||Dba.createReplicaSet: Replication option 'MASTER_COMPRESSION_ALGORITHMS' at '127.0.0.1:<<<__mysql_sandbox_port2>>>' has a non-default value 'zstd' but it is currently not supported by the AdminAPI. (MYSQLSH 51164)

//@# supported option: GET_MASTER_PUBLIC_KEY
|ReplicaSet object successfully created for 127.0.0.1:<<<__mysql_sandbox_port1>>>.|

//@ adopt master-slave requires connection to the master (should fail)
|ERROR: Active connection must be to the PRIMARY when adopting an existing replication topology.|
||Dba.createReplicaSet: Target instance is not the PRIMARY (MYSQLSH 51313)

//@# adopt master-slave
|ReplicaSet object successfully created for <<<__address1>>>.|

//@# adopt master-slave,slave
|ReplicaSet object successfully created for <<<__address1>>>.|
|{|
|    "replicaSet": {|
|        "name": "myrs", |
|        "primary": "127.0.0.1:<<<__mysql_sandbox_port1>>>", |
|        "status": "AVAILABLE", |
|        "statusText": "All instances available.", |
|        "topology": {|
|            "127.0.0.1:<<<__mysql_sandbox_port1>>>": {|
|                "address": "127.0.0.1:<<<__mysql_sandbox_port1>>>", |
|                "instanceRole": "PRIMARY", |
|                "mode": "R/W", |
|                "status": "ONLINE"|
|            }, |
|            "127.0.0.1:<<<__mysql_sandbox_port2>>>": {|
|                "address": "127.0.0.1:<<<__mysql_sandbox_port2>>>", |
|                "instanceRole": "SECONDARY", |
|                "mode": "R/O", |
|                "replication": {|
|                    "applierStatus": "APPLIED_ALL", |
|                    "applierThreadState": "Slave has read all relay log; waiting for more updates", |
|                    "receiverStatus": "ON", |
|                    "receiverThreadState": "Waiting for master to send event", |
|                    "replicationLag": null|
|                }, |
|                "status": "ONLINE"|
|            }, |
|            "127.0.0.1:<<<__mysql_sandbox_port3>>>": {|
|                "address": "127.0.0.1:<<<__mysql_sandbox_port3>>>", |
|                "instanceRole": "SECONDARY", |
|                "mode": "R/O", |
|                "replication": {|
|                    "applierStatus": "APPLIED_ALL", |
|                    "applierThreadState": "Slave has read all relay log; waiting for more updates", |
|                    "receiverStatus": "ON", |
|                    "receiverThreadState": "Waiting for master to send event", |
|                    "replicationLag": null|
|                }, |
|                "status": "ONLINE"|
|            }|
|        }, |
|        "type": "ASYNC"|
|    }|
|}|

//@<OUT> dryRun - should work, without changing the metadata
A new replicaset with the topology visible from '<<<__address1>>>' will be created.

NOTE: dryRun option was specified. Validations will be executed, but no changes will be applied.
* Scanning replication topology...
** Scanning state of instance 127.0.0.1:<<<__mysql_sandbox_port1>>>
** Scanning state of instance 127.0.0.1:<<<__mysql_sandbox_port2>>>

* Discovering async replication topology starting with <<<__address1>>>
Discovered topology:
- 127.0.0.1:<<<__mysql_sandbox_port1>>>: uuid=[[*]] read_only=no
- 127.0.0.1:<<<__mysql_sandbox_port2>>>: uuid=[[*]] read_only=no
    - replicates from 127.0.0.1:<<<__mysql_sandbox_port1>>>
	source="localhost:<<<__mysql_sandbox_port1>>>" channel= status=ON receiver=ON applier=ON

* Checking configuration of discovered instances...

This instance reports its own address as 127.0.0.1:<<<__mysql_sandbox_port1>>>
127.0.0.1:<<<__mysql_sandbox_port1>>>: Instance configuration is suitable.

This instance reports its own address as 127.0.0.1:<<<__mysql_sandbox_port2>>>
127.0.0.1:<<<__mysql_sandbox_port2>>>: Instance configuration is suitable.

* Checking discovered replication topology...
127.0.0.1:<<<__mysql_sandbox_port1>>> detected as the PRIMARY.
Replication state of 127.0.0.1:<<<__mysql_sandbox_port2>>> is OK.

Validations completed successfully.

* Updating metadata...

dryRun finished.

ReplicaSet object successfully created for <<<__address1>>>.
Use rs.addInstance() to add more asynchronously replicated instances to this replicaset and rs.status() to check its status.
