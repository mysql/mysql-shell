//@# INCLUDE async_utils.inc
||

//@ bad parameters (should fail)
||ReplicaSet.removeInstance: Invalid number of arguments, expected 1 to 2 but got 0 (ArgumentError)
||ReplicaSet.removeInstance: Argument #1 is expected to be a string (ArgumentError)
||ReplicaSet.removeInstance: Argument #1 is expected to be a string (ArgumentError)
||ReplicaSet.removeInstance: Argument #1 is expected to be a string (ArgumentError)
||ReplicaSet.removeInstance: Argument #2 is expected to be a map (ArgumentError)
||ReplicaSet.removeInstance: Invalid number of arguments, expected 1 to 2 but got 3 (ArgumentError)
||ReplicaSet.removeInstance: Argument #1 is expected to be a string (ArgumentError)
||ReplicaSet.removeInstance: Argument #1 is expected to be a string (ArgumentError)
||ReplicaSet.removeInstance: Invalid options: badOption (ArgumentError)
||ReplicaSet.removeInstance: Argument #1 is expected to be a string (ArgumentError)

//@ disconnected rs object (should fail)
||ReplicaSet.removeInstance: The replicaset object is disconnected. Please use dba.getReplicaSet() to obtain a new object. (RuntimeError)

//@ remove invalid host (should fail)
||ReplicaSet.removeInstance: <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> does not belong to the replicaset (MYSQLSH 51310)
||ReplicaSet.removeInstance: bogushost: Unknown MySQL server host 'bogushost'

//@ remove PRIMARY (should fail)
||ReplicaSet.removeInstance: PRIMARY instance cannot be removed from the replicaset. (MYSQLSH 51302)
||ReplicaSet.removeInstance: PRIMARY instance cannot be removed from the replicaset. (MYSQLSH 51302)

//@ remove when not in metadata (should fail)
||ReplicaSet.removeInstance: <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> does not belong to the replicaset (MYSQLSH 51310)

//@ remove while the instance we got the rs from is down (should fail)
||ReplicaSet.removeInstance: Failed to execute query on Metadata server <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>: Lost connection to MySQL server during query

//@<OUT> bad URI with a different user (should fail)
ERROR: Target instance must be given as host:port. Credentials will be taken from the main session and, if given, must match them.

//@<ERR> bad URI with a different user (should fail)
ReplicaSet.removeInstance: Invalid target instance specification (ArgumentError)

//@<OUT> bad URI with a different password (should fail)
ERROR: Target instance must be given as host:port. Credentials will be taken from the main session and, if given, must match them.

//@<ERR> bad URI with a different password (should fail)
ReplicaSet.removeInstance: Invalid target instance specification (ArgumentError)

//@<OUT> removeInstance on a URI
The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' was removed from the replicaset.

//@<OUT> check state after removal
view_id	cluster_id	instance_id	label	member_id	member_role	master_instance_id	master_member_id
9	[[*]]	1	<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>	5ef81566-9395-11e9-87e9-111111111111	PRIMARY	NULL	NULL
9	[[*]]	3	<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>	5ef81566-9395-11e9-87e9-333333333333	SECONDARY	1	5ef81566-9395-11e9-87e9-111111111111
2
0
0

//@<OUT> from same target as main session
The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' was removed from the replicaset.

//@ rs is connected to an invalidated member now, but the rs should handle that
||

//@<OUT> while some other secondary (sb3) is down
The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' was removed from the replicaset.

//@<OUT> target is not super-read-only
The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' was removed from the replicaset.

//@<OUT> target is not read-only
The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' was removed from the replicaset.

//@ timeout (should fail)
||ReplicaSet.removeInstance: Timeout reached waiting for transactions from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> to be applied on instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' (MYSQLSH 51157)

//@<OUT> remove while target down - no force (should fail)
ERROR: Unable to connect to the target instance localhost:<<<__mysql_sandbox_port2>>>. Please make sure the instance is available and try again. If the instance is permanently not reachable, use the 'force' option to remove it from the replicaset metadata and skip reconfiguration of that instance.

//@<ERR> remove while target down - no force (should fail)
ReplicaSet.removeInstance: localhost:<<<__mysql_sandbox_port2>>>: Can't connect to MySQL server on 'localhost' [[*]]

//@<OUT> remove while target down (localhost) - force (should fail)
ERROR: Instance localhost:<<<__mysql_sandbox_port2>>> is unreachable and was not found in the replicaset metadata. The exact address of the instance as recorded in the metadata must be used in cases where the target is unreachable.

//@<ERR> remove while target down (localhost) - force (should fail)
ReplicaSet.removeInstance: localhost:<<<__mysql_sandbox_port2>>> does not belong to the replicaset (MYSQLSH 51310)

//@<OUT> remove while target down (hostname) - force
NOTE: Unable to connect to the target instance <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>. The instance will only be removed from the metadata, but its replication configuration cannot be updated. Please, take any necessary actions to make sure that the instance will not replicate from the replicaset if brought back online.
Metadata for instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' was deleted, but instance configuration could not be updated.

//@<OUT> remove instance not in replicaset (metadata), no force (fail).
ERROR: Instance <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> cannot be removed because it does not belong to the replicaset (not found in the metadata). If you really want to remove this instance because it is still using replication then it must be stopped manually.

//@<ERR> remove instance not in replicaset (metadata), no force (fail).
ReplicaSet.removeInstance: <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> does not belong to the replicaset (MYSQLSH 51310)

//@<OUT> remove instance not in replicaset (metadata), with force true (fail).
ERROR: Instance <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> cannot be removed because it does not belong to the replicaset (not found in the metadata). If you really want to remove this instance because it is still using replication then it must be stopped manually.

//@<ERR> remove instance not in replicaset (metadata), with force true (fail).
ReplicaSet.removeInstance: <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> does not belong to the replicaset (MYSQLSH 51310)

//@<OUT> remove while target down (using extra elements in URL) - force
NOTE: Unable to connect to the target instance <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>. The instance will only be removed from the metadata, but its replication configuration cannot be updated. Please, take any necessary actions to make sure that the instance will not replicate from the replicaset if brought back online.
Metadata for instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' was deleted, but instance configuration could not be updated.

//@<OUT> error in replication channel - no force (should fail)
ERROR: Replication applier error in <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>: [[*]]

//@<ERR> error in replication channel - no force (should fail)
ReplicaSet.removeInstance: Replication applier error in <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> (MYSQLSH 51145)

//@ error in replication channel - force
||ReplicaSet.removeInstance: Replication applier error in <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> (MYSQLSH 51145)
||ReplicaSet.addInstance: <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> is already a member of this replicaset. (MYSQLSH 51301)

//@<OUT> remove with repl already stopped - no force (should fail)
WARNING: Replication is not active in instance <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>.
ERROR: Instance cannot be safely removed while in this state.
Use the 'force' option to remove this instance anyway. The instance may be left in an inconsistent state after removed.

//@<ERR> remove with repl already stopped - no force (should fail)
ReplicaSet.removeInstance: Replication is not active in target instance (MYSQLSH 51132)

//@<OUT> remove with repl already stopped - force
WARNING: Replication is not active in instance <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>.
The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' was removed from the replicaset.

//@ impossible sync - no force (should fail)
||ReplicaSet.removeInstance: Missing purged transactions at <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> (MYSQLSH 51153)

//@<OUT> impossible sync - force
WARNING: <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> is missing [[*]] transactions that have been purged from the current PRIMARY (<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>)
The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' was removed from the replicaset.

//@ remove while PRIMARY down (should fail)
||ReplicaSet.removeInstance: PRIMARY instance is unavailable (MYSQLSH 51118)


