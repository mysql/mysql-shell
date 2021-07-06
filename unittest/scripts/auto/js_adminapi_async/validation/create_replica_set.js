//@# INCLUDE async_utils.inc
||

//@# Setup
||

//@# Bad options (should fail)
||Invalid number of arguments, expected 1 to 2 but got 0 (ArgumentError)
||An open session is required to perform this operation. (RuntimeError)
||Invalid number of arguments, expected 1 to 2 but got 0 (ArgumentError)
||Argument #1 is expected to be a string (TypeError)
||Argument #1 is expected to be a string (TypeError)
||ReplicaSet name may only contain alphanumeric characters or '_', and may not start with a number (my cluster) (ArgumentError)
||ReplicaSet name may only contain alphanumeric characters or '_', and may not start with a number (my:::cluster) (ArgumentError)
||ReplicaSet name may only contain alphanumeric characters or '_', and may not start with a number (my:::clus/ter) (ArgumentError)
||_1234567890::_1234567890123456789012345678901: The ReplicaSet name can not be greater than 40 characters. (ArgumentError)
||ReplicaSet name may only contain alphanumeric characters or '_', and may not start with a number (::) (ArgumentError)
||An open session is required to perform this operation. (RuntimeError)

//@#create with unmanaged GR (should fail)
||This function is not available through a session to an instance belonging to an unmanaged replication group (RuntimeError)

//@# create with unmanaged AR (should fail)
|ERROR: Extraneous replication channels found at <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>:|
|- channel '' from <<<__endpoint_uri2>>>|
|Unmanaged replication channels are not supported in a replicaset. If you'd like|
|to manage an existing MySQL replication topology with the Shell, use the|
|createReplicaSet() operation with the adoptFromAR option.|
||Unexpected replication channel (MYSQLSH 51150)

//@ create with unmanaged AR from the master (should fail)
|ERROR: Extraneous replication channels found at <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>:|
|- <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> replicates from this instance|
|Unmanaged replication channels are not supported in a replicaset. If|
||Unexpected replication channel (MYSQLSH 51150)

//@# create with existing replicaset (should fail)
||Unable to create replicaset. The instance '<<<__address1>>>' already belongs to a replicaset. Use dba.getReplicaSet() to access it. (MYSQLSH 51306)

//@# create with insufficient privs (should fail)
||Unable to detect target instance state. Please check account privileges. (RuntimeError)

//@# create with bad configs (should fail) {VER(<8.0.23)}
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

//@# create with bad configs (should fail) {VER(>=8.0.23) && VER(<8.0.26)}
|NOTE: Some configuration options need to be fixed:|
@+----------------------------------------+---------------+----------------+--------------------------------------------------+@
@| Variable                               | Current Value | Required Value | Note                                             |@
@+----------------------------------------+---------------+----------------+--------------------------------------------------+@
@| binlog_transaction_dependency_tracking | COMMIT_ORDER  | WRITESET       | Update the server variable                       |@
@| enforce_gtid_consistency               | OFF           | ON             | Update read-only variable and restart the server |@
@| gtid_mode                              | OFF           | ON             | Update read-only variable and restart the server |@
@| server_id                              | 1             | <unique ID>    | Update read-only variable and restart the server |@
@| slave_parallel_type                    | DATABASE      | LOGICAL_CLOCK  | Update the server variable                       |@
@| slave_preserve_commit_order            | OFF           | ON             | Update the server variable                       |@
@+----------------------------------------+---------------+----------------+--------------------------------------------------+@
@Some variables need to be changed, but cannot be done dynamically on the server.@
@ERROR: <<<__address3>>>: Instance must be configured and validated with dba.configureReplicaSetInstance() before it can be used in a replicaset.@
@@Dba.createReplicaSet: Instance check failed (MYSQLSH 51150)

//@# create with bad configs (should fail) {VER(==8.0.26)}
|NOTE: Some configuration options need to be fixed:|
@+----------------------------------------+---------------+----------------+--------------------------------------------------+@
@| Variable                               | Current Value | Required Value | Note                                             |@
@+----------------------------------------+---------------+----------------+--------------------------------------------------+@
@| binlog_transaction_dependency_tracking | COMMIT_ORDER  | WRITESET       | Update the server variable                       |@
@| enforce_gtid_consistency               | OFF           | ON             | Update read-only variable and restart the server |@
@| gtid_mode                              | OFF           | ON             | Update read-only variable and restart the server |@
@| replica_parallel_type                  | DATABASE      | LOGICAL_CLOCK  | Update the server variable                       |@
@| replica_preserve_commit_order          | OFF           | ON             | Update the server variable                       |@
@| server_id                              | 1             | <unique ID>    | Update read-only variable and restart the server |@
@+----------------------------------------+---------------+----------------+--------------------------------------------------+@
@Some variables need to be changed, but cannot be done dynamically on the server.@
@ERROR: <<<__address3>>>: Instance must be configured and validated with dba.configureReplicaSetInstance() before it can be used in a replicaset.@
@@Dba.createReplicaSet: Instance check failed (MYSQLSH 51150)

//@# create with bad configs (should fail) {VER(>=8.0.27)}
|NOTE: Some configuration options need to be fixed:|
@+----------------------------------------+---------------+----------------+--------------------------------------------------+@
@| Variable                               | Current Value | Required Value | Note                                             |@
@+----------------------------------------+---------------+----------------+--------------------------------------------------+@
@| binlog_transaction_dependency_tracking | COMMIT_ORDER  | WRITESET       | Update the server variable                       |@
@| enforce_gtid_consistency               | OFF           | ON             | Update read-only variable and restart the server |@
@| gtid_mode                              | OFF           | ON             | Update read-only variable and restart the server |@
@| server_id                              | 1             | <unique ID>    | Update read-only variable and restart the server |@
@+----------------------------------------+---------------+----------------+--------------------------------------------------+@
@Some variables need to be changed, but cannot be done dynamically on the server.@
@ERROR: <<<__address3>>>: Instance must be configured and validated with dba.configureReplicaSetInstance() before it can be used in a replicaset.@
@@Dba.createReplicaSet: Instance check failed (MYSQLSH 51150)

//@# replication filters (should fail)
||<<<__address1>>>: instance has global replication filters configured, but they are not supported in InnoDB ReplicaSets. (MYSQLSH 51150)

//@# binlog filters (should fail)
||<<<__address1>>>: instance has binlog filters configured, but they are not supported in InnoDB ReplicaSets. (MYSQLSH 51150)

//@# Invalid loopback ip (should fail)
|ERROR: Cannot use host '127.0.1.1' for instance '127.0.1.1:<<<__mysql_sandbox_port1>>>' because it resolves to an IP address (127.0.1.1) that does not match a real network interface, thus it is not supported. Change your system settings and/or set the MySQL server 'report_host' variable to a hostname that resolves to a supported IP address.|
||Invalid host/IP '127.0.1.1' resolves to '127.0.1.1' which is not supported. (RuntimeError)

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
