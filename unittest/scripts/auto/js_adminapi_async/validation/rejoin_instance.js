//@# INCLUDE async_utils.inc
||

//@ Invalid parameters (fail).
||ReplicaSet.rejoinInstance: Invalid number of arguments, expected 1 to 2 but got 0 (ArgumentError)
||ReplicaSet.rejoinInstance: Argument #1 is expected to be a string (ArgumentError)
||ReplicaSet.rejoinInstance: Argument #1 is expected to be a string (ArgumentError)
||ReplicaSet.rejoinInstance: Argument #1 is expected to be a string (ArgumentError)
||ReplicaSet.rejoinInstance: Argument #1 is expected to be a string (ArgumentError)
||ReplicaSet.rejoinInstance: Invalid URI: empty. (ArgumentError)
||ReplicaSet.rejoinInstance: Invalid URI: empty. (ArgumentError)
||ReplicaSet.rejoinInstance: Invalid number of arguments, expected 1 to 2 but got 3 (ArgumentError)
||ReplicaSet.rejoinInstance: Invalid options: badOption (ArgumentError)
||ReplicaSet.rejoinInstance: Could not open connection to 'localhost:<<<__mysql_sandbox_port3>>>': Can't connect to MySQL server on 'localhost' ([[*]]) (MySQL Error 2003)
||ReplicaSet.rejoinInstance: Invalid value for option recoveryMethod: bogus (ArgumentError)
||ReplicaSet.rejoinInstance: Invalid value '42' for option 'waitRecovery'. It must be an integer in the range [1, 3]. (ArgumentError)
||ReplicaSet.rejoinInstance: Invalid value '42' for option 'waitRecovery'. It must be an integer in the range [1, 3]. (ArgumentError)
||ReplicaSet.rejoinInstance: Option cloneDonor only allowed if option recoveryMethod is set to 'clone'. (ArgumentError)
||ReplicaSet.rejoinInstance: Invalid value for cloneDonor, string value cannot be empty. (ArgumentError)
||ReplicaSet.rejoinInstance: Invalid value for cloneDonor: Invalid address format in 'foobar'. Must be <host>:<port> or [<ip>]:<port> for IPv6 addresses (ArgumentError)
||ReplicaSet.rejoinInstance: Invalid value for cloneDonor: Invalid address format in 'root@foobar:3232'. Must be <host>:<port> or [<ip>]:<port> for IPv6 addresses (ArgumentError)
||ReplicaSet.rejoinInstance: IPv6 addresses not supported for cloneDonor (ArgumentError)
||ReplicaSet.rejoinInstance: Invalid value for cloneDonor: Invalid address format in '::1:3232'. Must be <host>:<port> or [<ip>]:<port> for IPv6 addresses (ArgumentError)
||ReplicaSet.rejoinInstance: Invalid value for cloneDonor: Invalid address format in '::1'. Must be <host>:<port> or [<ip>]:<port> for IPv6 addresses (ArgumentError)

//@ Try rejoin ONLINE instance (fail).
|ERROR: Unable to rejoin an ONLINE instance. This operation can only be used to rejoin instances with an INVALIDATED, OFFLINE, INCONSISTENT or ERROR status.|ReplicaSet.rejoinInstance: Invalid status to execute operation, <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> is ONLINE (MYSQLSH 51125)

//@ Try rejoin instance not belonging to any replicaset (fail).
|ERROR: Cannot rejoin an instance that does not belong to the replicaset. Please confirm the specified address or execute the operation against the correct ReplicaSet object.|ReplicaSet.rejoinInstance: Instance <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> does not belong to the replicaset (MYSQLSH 51310)

//@ Try rejoin instance belonging to another replicaset (fail).
|ERROR: Cannot rejoin an instance that does not belong to the replicaset. Please confirm the specified address or execute the operation against the correct ReplicaSet object.|ReplicaSet.rejoinInstance: Instance <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> does not belong to the replicaset (MYSQLSH 51310)

//@ Try rejoin instance with disconnected rs object (fail).
||ReplicaSet.rejoinInstance: The replicaset object is disconnected. Please use dba.getReplicaSet() to obtain a new object. (RuntimeError)

//@ Try rejoin instance with a user different from the cluster admin user (fail).
|ERROR: Target instance must be given as host:port. Credentials will be taken from the main session and, if given, must match them|ReplicaSet.rejoinInstance: Invalid target instance specification (ArgumentError)

//@<OUT> Rejoin instance with wrong settings (fail) {VER(<8.0.23)}
* Validating instance...

This instance reports its own address as <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>

NOTE: Some configuration options need to be fixed:
+---------------+---------------+----------------+----------------------------+
| Variable      | Current Value | Required Value | Note                       |
+---------------+---------------+----------------+----------------------------+
| binlog_format | STATEMENT     | ROW            | Update the server variable |
+---------------+---------------+----------------+----------------------------+

ERROR: <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>: Instance must be configured and validated with dba.configureReplicaSetInstance() before it can be used in a replicaset.

//@ Rejoin instance with wrong settings (fail) {VER(<8.0.23)}
||ReplicaSet.rejoinInstance: Instance check failed (MYSQLSH 51150)

//@<OUT> Rejoin instance with wrong parallel-applier settings (fail) {VER(>=8.0.23)}
* Validating instance...

This instance reports its own address as <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>

NOTE: Some configuration options need to be fixed:
+-----------------------------+---------------+----------------+----------------------------+
| Variable                    | Current Value | Required Value | Note                       |
+-----------------------------+---------------+----------------+----------------------------+
| slave_preserve_commit_order | OFF           | ON             | Update the server variable |
+-----------------------------+---------------+----------------+----------------------------+

ERROR: <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>: Instance must be configured and validated with dba.configureReplicaSetInstance() before it can be used in a replicaset.

//@ Rejoin instance with wrong parallel-applier settings (fail) {VER(>=8.0.23)}
||ReplicaSet.rejoinInstance: Instance check failed (MYSQLSH 51150)

//@<OUT> Rejoin instance with replication stopped (succeed).
* Validating instance...

This instance reports its own address as <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>
<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>: Instance configuration is suitable.
** Checking transaction state of the instance...
The safest and most convenient way to provision a new instance is through automatic clone provisioning, which will completely overwrite the state of '<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>' with a physical snapshot from an existing replicaset member. To use this method by default, set the 'recoveryMethod' option to 'clone'.

WARNING: It should be safe to rely on replication to incrementally recover the state of the new instance if you are sure all updates ever executed in the replicaset were done with GTIDs enabled, there are no purged transactions and the new instance contains the same GTID set as the replicaset or a subset of it. To use this method by default, set the 'recoveryMethod' option to 'incremental'.

Incremental state recovery was selected because it seems to be safely usable.

* Rejoining instance to replicaset...
** Configuring <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> to replicate from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>
** Checking replication channel status...
** Waiting for rejoined instance to synchronize with PRIMARY...

* Updating the Metadata...
The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>' rejoined the replicaset and is replicating from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>.

//@ Rejoin instance with replication SQL thread stopped (succeed).
||

//@ Rejoin instance with replication IO thread stopped (succeed).
||

//@ Rejoin instance with replication reset and stopped (succeed).
||

//@<OUT> Rejoin old primary to replicaset (success) and confirm status.
* Validating instance...

This instance reports its own address as <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>
<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>: Instance configuration is suitable.
** Checking transaction state of the instance...
The safest and most convenient way to provision a new instance is through automatic clone provisioning, which will completely overwrite the state of '<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>' with a physical snapshot from an existing replicaset member. To use this method by default, set the 'recoveryMethod' option to 'clone'.

WARNING: It should be safe to rely on replication to incrementally recover the state of the new instance if you are sure all updates ever executed in the replicaset were done with GTIDs enabled, there are no purged transactions and the new instance contains the same GTID set as the replicaset or a subset of it. To use this method by default, set the 'recoveryMethod' option to 'incremental'.

Incremental state recovery was selected because it seems to be safely usable.

* Rejoining instance to replicaset...
** Configuring <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> to replicate from <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>
** Checking replication channel status...
** Waiting for rejoined instance to synchronize with PRIMARY...

* Updating the Metadata...
The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>' rejoined the replicaset and is replicating from <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>.

//@ Try to rejoin instance with errant transaction (fail).
||ReplicaSet.rejoinInstance: Cannot use recoveryMethod=incremental option because the GTID state is not compatible or cannot be recovered. (RuntimeError)

//@ Try to rejoin instance with unsolved replication error (fail). {VER(<8.0.23)}
||ReplicaSet.rejoinInstance: <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>: Error found in replication applier thread (MYSQLSH 51145)

//@ Try to rejoin instance with unsolved replication error (fail). {VER(>=8.0.23)}
||ReplicaSet.rejoinInstance: <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>: Error found in replication coordinator thread (MYSQLSH 51144)

//@ Rejoin instance after solving replication error (succeed).
||

//@ Rejoin instance with connection failure, rpl user password reset (succeed).
||

// BUG#30884590: ADDING AN INSTANCE WITH COMPATIBLE GTID SET SHOULDN'T PROMPT FOR CLONE
//@ Try to rejoin instance with purged transactions on PRIMARY (should work, clone automatically selected)
|* Validating instance...|
|** Checking transaction state of the instance...|
|NOTE: A GTID set check of the MySQL instance at '<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>' determined that it|
|is missing transactions that were purged from all replicaset members.|
|Clone based recovery was selected because it seems to be safely usable.|
|* Rejoining instance to replicaset...|
|* Waiting for clone to finish...|
|NOTE: <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> is being cloned from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>|
|The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>' rejoined the replicaset and is replicating from <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>.|

//@ Try to rejoin instance with purged transactions on PRIMARY and gtid-set empty (should fail)
|* Validating instance...|
|** Checking transaction state of the instance...|
|NOTE: A GTID set check of the MySQL instance at '<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>' determined that it|
|is missing transactions that were purged from all replicaset members.|
|NOTE: The target instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>' has not been pre-provisioned (GTID set is|
|empty). The Shell is unable to decide whether clone based recovery is safe to use.|
||ReplicaSet.rejoinInstance: 'recoveryMethod' option must be set to 'clone' or 'incremental' (RuntimeError)

//@ Try to rejoin instance with purged transactions on PRIMARY (should work with clone) {VER(>=8.0.17)}
|Clone based recovery selected through the recoveryMethod option|
|* Rejoining instance to replicaset...|
|* Waiting for clone to finish...|
|NOTE: <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> is being cloned from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>|
|The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>' rejoined the replicaset and is replicating from <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>.|

//@ cloneDonor valid {VER(>=8.0.17)}
|NOTE: <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> is being cloned from <<<__sandbox1>>>|
|The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>' rejoined the replicaset and is replicating from <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>.|

//@ cloneDonor valid 2 {VER(>=8.0.17)}
|NOTE: <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> is being cloned from <<<__sandbox2>>>|
|The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>' rejoined the replicaset and is replicating from <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>.|

//@ BUG#30628746: wait for timeout {VER(>=8.0.17)}
|* Waiting for the donor to synchronize with PRIMARY...|
|ERROR: The donor instance failed to synchronize its transaction set with the PRIMARY.|
||ReplicaSet.rejoinInstance: Timeout reached waiting for transactions from <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> to be applied on instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>' (MYSQLSH 51157)

//@ BUG#30628746: donor primary should not error with timeout {VER(>=8.0.17)}
|The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>' rejoined the replicaset and is replicating from <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>.|
