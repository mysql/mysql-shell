//@# on a standalone server (should fail)
||Target server is not part of an async replication topology (MYSQLSH 51151)

//@# Bad options (should fail)
||Argument #1 is expected to be a string (TypeError)
||Argument #2 instanceLabel option not allowed when adoptFromAR:true (ArgumentError)
||An open session is required to perform this operation. (RuntimeError)

//@# adopt with InnoDB cluster (should fail)
||Unable to create replicaset. The instance '<<<__address1>>>' already belongs to an InnoDB cluster. Use dba.getCluster() to access it. (MYSQLSH 51305)

//@# indirect adopt with InnoDB cluster (should fail)
||group_replication active (MYSQLSH 51150)

//@# adopt with unmanaged GR (should fail)
||This function is not available through a session to an instance belonging to an unmanaged replication group (RuntimeError)

//@# indirect adopt with unmanaged GR (should fail)
||group_replication active (MYSQLSH 51150)

//@# adopt with existing replicaset (should fail)
||Unable to create replicaset. The instance '<<<__address1>>>' already belongs to a replicaset. Use dba.getReplicaSet() to access it or dba.dropMetadataSchema() to drop the metadata if the replicaset was dissolved. (MYSQLSH 51306)

//@# indirect adopt with existing replicaset (should fail)
||Unable to create replicaset. The instance '<<<__address2>>>' already belongs to a replicaset. Use dba.getReplicaSet() to access it or dba.dropMetadataSchema() to drop the metadata if the replicaset was dissolved. (MYSQLSH 51306)

//@# adopt with insufficient privs (should fail)
||Unable to detect state for instance '127.0.0.1:<<<__mysql_sandbox_port1>>>'. Please check account privileges. (RuntimeError)
||Unable to detect state for instance '127.0.0.1:<<<__mysql_sandbox_port1>>>'. Please check account privileges. (RuntimeError)

//@# adopt with existing old metadata, belongs to other (should fail)
|Incompatible Metadata version. This operation is disallowed because the installed Metadata version '1.0.1' is lower than the required version, '2.3.0'. Upgrade the Metadata to remove this restriction. See \? dba.upgradeMetadata for additional details.|
||Metadata version is not compatible (MYSQLSH 51107)

//@# indirect adopt with existing old metadata, belongs to other (should fail)
|Incompatible Metadata version. This operation is disallowed because the installed Metadata version '1.0.1' is lower than the required version, '2.3.0'. Upgrade the Metadata to remove this restriction. See \? dba.upgradeMetadata for additional details.|
||Metadata version is not compatible (MYSQLSH 51107)

//@# replication filters (should fail)
||127.0.0.1:<<<__mysql_sandbox_port1>>>: instance has global replication filters configured, but they are not supported in InnoDB ReplicaSets. (MYSQLSH 51150)
||127.0.0.1:<<<__mysql_sandbox_port1>>>: instance has global replication filters configured, but they are not supported in InnoDB ReplicaSets. (MYSQLSH 51150)

//@# binlog filters (should fail)
||127.0.0.1:<<<__mysql_sandbox_port1>>>: instance has binlog filters configured, but they are not supported in InnoDB ReplicaSets. (MYSQLSH 51150)

//@# broken repl (should fail)
|ERROR: 127.0.0.1:<<<__mysql_sandbox_port2>>> has a replication channel '' in an unexpected state.|
|Any existing replication settings must be working correctly before it can be adopted.|
||Invalid replication state (MYSQLSH 51150)

//@# bad repl channels - bad name (should fail)
|ERROR: 127.0.0.1:<<<__mysql_sandbox_port1>>> has one or more replicas with unsupported channels|
|Adopted topologies must not have any replication channels other than the default one.|
||Unsupported replication topology (MYSQLSH 51151)

//@# bad repl channels - 2 channels (should fail)
|    - replicates from 'localhost:<<<__mysql_sandbox_port1>>>' through unsupported channel 'bla'|
|ERROR: 127.0.0.1:<<<__mysql_sandbox_port2>>> has one or more unsupported replication channels: bla|
||Unsupported replication topology (MYSQLSH 51151)

//@# bad repl channels - master has a bogus channel
|    - replicates from 'localhost:<<<__mysql_sandbox_port3>>>' through unsupported channel 'foob'|
|ERROR: 127.0.0.1:<<<__mysql_sandbox_port1>>> has one or more unsupported replication channels: foob|
||Unsupported replication topology (MYSQLSH 51151)

//@# admin account has mismatched passwords (should fail)
|ERROR: Could not connect to 127.0.0.1:<<<__mysql_sandbox_port2>>> (slave of <<<__address1>>>): MySQL Error 1045: Could not open connection to '127.0.0.1:<<<__mysql_sandbox_port2>>>': Access denied for user 'rooty'@'localhost' (using password: YES)|
||Could not open connection to '127.0.0.1:<<<__mysql_sandbox_port2>>>': Access denied for user 'rooty'@'localhost' (using password: YES) (MySQL Error 1045)

//@# admin account passwords match, but they don't allow connection from the shell (should fail)
||Unable to detect state for instance '127.0.0.1:<<<__mysql_sandbox_port1>>>'. Please check account privileges. (RuntimeError)

//@# invalid topology: master-master (should fail)
|ERROR: Unable to determine the PRIMARY instance in the topology. Multi-master topologies are not supported.|
||Unsupported replication topology (MYSQLSH 51151)


//@# invalid topology: master-master-master (should fail)
|ERROR: Unable to determine the PRIMARY instance in the topology. Multi-master topologies are not supported.|
||Unsupported replication topology (MYSQLSH 51151)

//@# invalid topology: master-master-slave (should fail)
|ERROR: Unable to determine the PRIMARY instance in the topology. Multi-master topologies are not supported.|
||Unsupported replication topology (MYSQLSH 51151)

//@# bad configs: SBR (should fail)
|ERROR: 127.0.0.1:<<<__mysql_sandbox_port1>>>: Instance must be configured and validated with dba.configureReplicaSetInstance() before it can be used in a replicaset.|
||Instance check failed (MYSQLSH 51150)
|ERROR: 127.0.0.1:<<<__mysql_sandbox_port2>>>: Instance must be configured and validated with dba.configureReplicaSetInstance() before it can be used in a replicaset.|
||Instance check failed (MYSQLSH 51150)

//@# bad configs: filepos replication (should fail) {VER(<8.0.23)}
@| enforce_gtid_consistency | OFF           | ON             | Update read-only variable and restart the server |@
@| gtid_mode                | OFF           | ON             | Update read-only variable and restart the server |@
|Some variables need to be changed, but cannot be done dynamically on the server.|
|ERROR: 127.0.0.1:<<<__mysql_sandbox_port1>>>: Instance must be configured and validated with dba.configureReplicaSetInstance() before it can be used in a replicaset.|
||Instance check failed (MYSQLSH 51150)

//@# bad configs: filepos replication (should fail) {VER(>=8.0.23) && VER(<8.0.27)}
@| binlog_transaction_dependency_tracking | COMMIT_ORDER  | WRITESET       | Update the server variable                       |@
@| enforce_gtid_consistency               | OFF           | ON             | Update read-only variable and restart the server |@
@| gtid_mode                              | OFF           | ON             | Update read-only variable and restart the server |@
@| <<<__replica_keyword>>>_parallel_type[[*]]                  | DATABASE      | LOGICAL_CLOCK  | Update the server variable                       |@
@| <<<__replica_keyword>>>_preserve_commit_order[[*]]          | OFF           | ON             | Update the server variable                       |@
|Some variables need to be changed, but cannot be done dynamically on the server.|
|ERROR: 127.0.0.1:<<<__mysql_sandbox_port1>>>: Instance must be configured and validated with dba.configureReplicaSetInstance() before it can be used in a replicaset.|
||Instance check failed (MYSQLSH 51150)

//@# bad configs: filepos replication (should fail) {VER(>=8.0.27) && VER(<8.3.0)}
@| binlog_transaction_dependency_tracking | COMMIT_ORDER  | WRITESET       | Update the server variable                       |@
@| enforce_gtid_consistency               | OFF           | ON             | Update read-only variable and restart the server |@
@| gtid_mode                              | OFF           | ON             | Update read-only variable and restart the server |@
|Some variables need to be changed, but cannot be done dynamically on the server.|
|ERROR: 127.0.0.1:<<<__mysql_sandbox_port1>>>: Instance must be configured and validated with dba.configureReplicaSetInstance() before it can be used in a replicaset.|
||Instance check failed (MYSQLSH 51150)

//@# bad configs: filepos replication (should fail) {VER(>=8.3.0)}
@| enforce_gtid_consistency | OFF           | ON             | Update read-only variable and restart the server |@
@| gtid_mode                | OFF           | ON             | Update read-only variable and restart the server |@
|Some variables need to be changed, but cannot be done dynamically on the server.|
|ERROR: 127.0.0.1:<<<__mysql_sandbox_port1>>>: Instance must be configured and validated with dba.configureReplicaSetInstance() before it can be used in a replicaset.|
||Instance check failed (MYSQLSH 51150)

//@# unsupported option: delay (should fail) {VER(<8.0.23)}
||Replication option 'MASTER_DELAY' at '127.0.0.1:<<<__mysql_sandbox_port2>>>' has a non-default value '5' but it is currently not supported by the AdminAPI. (MYSQLSH 51164)

//@# unsupported option: delay (should fail) {VER(>=8.0.23)}
||Replication option 'SOURCE_DELAY' at '127.0.0.1:<<<__mysql_sandbox_port2>>>' has a non-default value '5' but it is currently not supported by the AdminAPI. (MYSQLSH 51164)

//@# unsupported option: connect_retry (should fail) {VER(<8.0.23)}
||Replication option 'MASTER_CONNECT_RETRY' at '127.0.0.1:<<<__mysql_sandbox_port2>>>' has a non-default value '4' but it is currently not supported by the AdminAPI. (MYSQLSH 51164)

//@# unsupported option: connect_retry (should fail) {VER(>=8.0.23)}
||Replication option 'SOURCE_CONNECT_RETRY' at '127.0.0.1:<<<__mysql_sandbox_port2>>>' has a non-default value '4' but it is currently not supported by the AdminAPI. (MYSQLSH 51164)

//@# unsupported option: retry_count (should fail) {VER(<8.0.23)}
||Replication option 'MASTER_RETRY_COUNT' at '127.0.0.1:<<<__mysql_sandbox_port2>>>' has a non-default value '3' but it is currently not supported by the AdminAPI. (MYSQLSH 51164)

//@# unsupported option: retry_count (should fail) {VER(>=8.0.23)}
||Replication option 'SOURCE_RETRY_COUNT' at '127.0.0.1:<<<__mysql_sandbox_port2>>>' has a non-default value '3' but it is currently not supported by the AdminAPI. (MYSQLSH 51164)

//@# unsupported option: heartbeat (should fail) {VER(<8.0.23)}
||Replication option 'MASTER_HEARTBEAT_PERIOD' at '127.0.0.1:<<<__mysql_sandbox_port2>>>' has a non-default value '32.00' but it is currently not supported by the AdminAPI. (MYSQLSH 51164)

//@# unsupported option: heartbeat (should fail) {VER(>=8.0.23)}
||Replication option 'SOURCE_HEARTBEAT_PERIOD' at '127.0.0.1:<<<__mysql_sandbox_port2>>>' has a non-default value '32.00' but it is currently not supported by the AdminAPI. (MYSQLSH 51164)

//@# unsupported option: no auto_position (should fail)
||127.0.0.1:<<<__mysql_sandbox_port2>>> uses replication without auto-positioning, which is not supported by the AdminAPI. (MYSQLSH 51164)

//@# unsupported option: MASTER_COMPRESSION_ALGORITHMS (should fail) {VER(<8.0.23)}
||Replication option 'MASTER_COMPRESSION_ALGORITHMS' at '127.0.0.1:<<<__mysql_sandbox_port2>>>' has a non-default value 'zstd' but it is currently not supported by the AdminAPI. (MYSQLSH 51164)

//@# unsupported option: MASTER_COMPRESSION_ALGORITHMS (should fail) {VER(>=8.0.23)}
||Replication option 'SOURCE_COMPRESSION_ALGORITHMS' at '127.0.0.1:<<<__mysql_sandbox_port2>>>' has a non-default value 'zstd' but it is currently not supported by the AdminAPI. (MYSQLSH 51164)

//@# supported option: GET_MASTER_PUBLIC_KEY
|ReplicaSet object successfully created for 127.0.0.1:<<<__mysql_sandbox_port1>>>.|

//@ adopt master-slave requires connection to the master (should fail)
|ERROR: Active connection must be to the PRIMARY when adopting an existing replication topology.|
||Target instance is not the PRIMARY (MYSQLSH 51313)

//@# adopt master-slave
|ReplicaSet object successfully created for <<<__address1>>>.|

//@# adopt master-slave,slave
|ReplicaSet object successfully created for <<<__address1>>>.|
|{|
|    "replicaSet": {|
|        "name": "myrs",|
|        "primary": "127.0.0.1:<<<__mysql_sandbox_port1>>>",|
|        "status": "AVAILABLE",|
|        "statusText": "All instances available.",|
|        "topology": {|
|            "127.0.0.1:<<<__mysql_sandbox_port1>>>": {|
|                "address": "127.0.0.1:<<<__mysql_sandbox_port1>>>",|
|                "instanceRole": "PRIMARY",|
|                "mode": "R/W",|
|                "status": "ONLINE"|
|            },|
|            "127.0.0.1:<<<__mysql_sandbox_port2>>>": {|
|                "address": "127.0.0.1:<<<__mysql_sandbox_port2>>>",|
|                "instanceRole": "SECONDARY",|
|                "mode": "R/O",|
|                "replication": {|
|                    "applierStatus": "APPLIED_ALL",|
|                    "applierThreadState": <<<(__version_num<80023)?'"Slave has read all relay log; waiting for more updates",':'"Waiting for an event from Coordinator",'>>>|
|                    <<<(__version_num<80023)?'"applierWorkerThreads": 4':''>>>|
|                    "receiverStatus": "ON",|
|                    "receiverThreadState": "Waiting for <<<__source_keyword>>> to send event",|
|                    "replicationLag": "applier_queue_applied"|
|                },|
|                "status": "ONLINE"|
|            },|
|            "127.0.0.1:<<<__mysql_sandbox_port3>>>": {|
|                "address": "127.0.0.1:<<<__mysql_sandbox_port3>>>",|
|                "instanceRole": "SECONDARY",|
|                "mode": "R/O",|
|                "replication": {|
|                    "applierStatus": "APPLIED_ALL",|
|                    "applierThreadState": <<<(__version_num<80023)?'"Slave has read all relay log; waiting for more updates",':'"Waiting for an event from Coordinator",'>>>|
|                    <<<(__version_num<80023)?'"applierWorkerThreads": 4':''>>>|
|                    "receiverStatus": "ON",|
|                    "receiverThreadState": "Waiting for <<<__source_keyword>>> to send event",|
|                    "replicationLag": "applier_queue_applied"|
|                },|
|                "status": "ONLINE"|
|            }|
|        },|
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
    - replicates from '127.0.0.1:<<<__mysql_sandbox_port1>>>'
<<<(__version_num<80023)?'	source="localhost:"' + __mysql_sandbox_port1 + '" channel= status=ON receiver=ON applier=ON':'	source="localhost:' + __mysql_sandbox_port1 + '" channel= status=ON receiver=ON coordinator=ON applier0=ON applier1=ON applier2=ON applier3=ON'>>>

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
