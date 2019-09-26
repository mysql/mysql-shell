//@# INCLUDE async_utils.inc
||

//@# Setup
||

//@# Bad options (should fail)
||Dba.createReplicaSet: Invalid number of arguments, expected 1 to 2 but got 0 (ArgumentError)
||Dba.createReplicaSet: An open session is required to perform this operation. (RuntimeError)
||Dba.createReplicaSet: Invalid number of arguments, expected 1 to 2 but got 0 (ArgumentError)
||Dba.createReplicaSet: Argument #1 is expected to be a string (ArgumentError)
||Dba.createReplicaSet: Argument #1 is expected to be a string (ArgumentError)
||Dba.createReplicaSet: ReplicaSet name may only contain alphanumeric characters or '_', and may not start with a number (my cluster) (ArgumentError)
||Dba.createReplicaSet: ReplicaSet name may only contain alphanumeric characters or '_', and may not start with a number (my:::cluster) (ArgumentError)
||Dba.createReplicaSet: ReplicaSet name may only contain alphanumeric characters or '_', and may not start with a number (my:::clus/ter) (ArgumentError)
||Dba.createReplicaSet: _1234567890::_1234567890123456789012345678901: The ReplicaSet name can not be greater than 40 characters. (ArgumentError)
||Dba.createReplicaSet: ReplicaSet name may only contain alphanumeric characters or '_', and may not start with a number (::) (ArgumentError)
||Dba.createReplicaSet: An open session is required to perform this operation. (RuntimeError)

//@#create with unmanaged GR (should fail)
||Dba.createReplicaSet: This function is not available through a session to an instance belonging to an unmanaged replication group (RuntimeError)

//@# create with unmanaged AR (should fail)
|ERROR: Extraneous replication channels found at <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>:|
|- channel '' from localhost:<<<__mysql_sandbox_port2>>>|
|Unmanaged replication channels are not supported in a replicaset. If you'd like|
|to manage an existing MySQL replication topology with the Shell, use the|
|createReplicaSet() operation with the adoptFromAR option.|
||Dba.createReplicaSet: Unexpected replication channel (MYSQLSH 51150)

//@ create with unmanaged AR from the master (should fail)
|ERROR: Extraneous replication channels found at <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>:|
|- <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> replicates from this instance|
|Unmanaged replication channels are not supported in a replicaset. If|
||Dba.createReplicaSet: Unexpected replication channel (MYSQLSH 51150)

//@# create with existing replicaset (should fail)
||Dba.createReplicaSet: Unable to create replicaset. The instance '<<<__address1>>>' already belongs to a replicaset. Use dba.getReplicaSet() to access it. (MYSQLSH 51306)

//@# create with insufficient privs (should fail)
||Dba.createReplicaSet: Unable to detect Metadata version. Please check account privileges. (RuntimeError)

//@# create with bad configs (should fail)
|NOTE: Some configuration options need to be fixed:|
@+--------------------------+---------------+----------------+--------------------------------------------------+@
@| Variable                 | Current Value | Required Value | Note                                             |@
@+--------------------------+---------------+----------------+--------------------------------------------------+@
@| enforce_gtid_consistency | OFF           | ON             | Update read-only variable and restart the server |@
@| gtid_mode                | OFF           | ON             | Update read-only variable and restart the server |@
@| server_id                | 1             | <unique ID>    | Update read-only variable and restart the server |@
@+--------------------------+---------------+----------------+--------------------------------------------------+@
@Some variables need to be changed, but cannot be done dynamically on the server.@
@ERROR: <<<__address3>>>: Instance must be configured and validated with dba.configureReplicaSetInstance() before it can be used in a replicaset.@
@@Dba.createReplicaSet: Instance check failed (MYSQLSH 51150)

//@# replication filters (should fail)
||Dba.createReplicaSet: <<<__address1>>>: instance has global replication filters configured, but they are not supported in InnoDB ReplicaSets. (MYSQLSH 51150)

//@# binlog filters (should fail)
||Dba.createReplicaSet: <<<__address1>>>: instance has binlog filters configured, but they are not supported in InnoDB ReplicaSets. (MYSQLSH 51150)

//@# from a X protocol session
|ReplicaSet object successfully created for <<<__address1>>>.|
|<ReplicaSet:myrs>|

//@# from a socket session
|ReplicaSet object successfully created|
|<ReplicaSet:myrs>|

//@<OUT> dryRun - should work, without changing the metadata
A new replicaset with instance '<<<__address1>>>' will be created.

NOTE: dryRun option was specified. Validations will be executed, but no changes will be applied.
* Checking MySQL instance at <<<__address1>>>

This instance reports its own address as 127.0.0.1:<<<__mysql_sandbox_port1>>>
<<<__address1>>>: Instance configuration is suitable.

* Updating metadata...

dryRun finished.

//@# instanceLabel
|<ReplicaSet:myrs>|
@+---------------+@
@| instance_name |@
@+---------------+@
@| seed          |@
@+---------------+@

//@# gtidSetIsCompatible
@| {"adopted": 0, "opt_gtidSetIsComplete": false} |@
@| {"adopted": 0, "opt_gtidSetIsComplete": true} |@
