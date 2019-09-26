//@# INCLUDE async_utils.inc
||

//@ Invalid parameters (fail).
||ReplicaSet.rejoinInstance: Invalid number of arguments, expected 1 but got 0 (ArgumentError)
||ReplicaSet.rejoinInstance: Argument #1 is expected to be a string (ArgumentError)
||ReplicaSet.rejoinInstance: Argument #1 is expected to be a string (ArgumentError)
||ReplicaSet.rejoinInstance: Argument #1 is expected to be a string (ArgumentError)
||ReplicaSet.rejoinInstance: Argument #1 is expected to be a string (ArgumentError)
||ReplicaSet.rejoinInstance: Invalid URI: empty. (ArgumentError)
||ReplicaSet.rejoinInstance: Connection 'invalid_uri' is not valid: unable to resolve the address as either an IPv4 or IPv6 host. (ArgumentError)
||ReplicaSet.rejoinInstance: Invalid number of arguments, expected 1 but got 2 (ArgumentError)
||ReplicaSet.rejoinInstance: Invalid number of arguments, expected 1 but got 3 (ArgumentError)
||ReplicaSet.rejoinInstance: Invalid number of arguments, expected 1 but got 2 (ArgumentError)
|ERROR: Unable to connect to the target instance localhost:<<<__mysql_sandbox_port3>>>. Please verify the connection settings, make sure the instance is available and try again.|ReplicaSet.rejoinInstance: localhost:<<<__mysql_sandbox_port3>>>: Can't connect to MySQL server on 'localhost'

//@ Try rejoin ONLINE instance (fail).
|ERROR: Unable to rejoin an ONLINE instance. This operation can only be used to rejoin instances with an INVALIDATED, OFFLINE, or ERROR status.|ReplicaSet.rejoinInstance: Invalid status to execute operation, <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> is ONLINE (MYSQLSH 51125)

//@ Try rejoin instance not belonging to any replicaset (fail).
|ERROR: Cannot rejoin an instance that does not belong to the replicaset. Please confirm the specified address or execute the operation against the correct ReplicaSet object.|ReplicaSet.rejoinInstance: Instance <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> does not belong to the replicaset (MYSQLSH 51310)

//@ Try rejoin instance belonging to another replicaset (fail).
|ERROR: Cannot rejoin an instance that does not belong to the replicaset. Please confirm the specified address or execute the operation against the correct ReplicaSet object.|ReplicaSet.rejoinInstance: Instance <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> does not belong to the replicaset (MYSQLSH 51310)

//@ Try rejoin instance with disconnected rs object (fail).
||ReplicaSet.rejoinInstance: The replicaset object is disconnected. Please use dba.getReplicaSet() to obtain a new object. (RuntimeError)

//@ Try rejoin instance with a user different from the cluster admin user (fail).
|ERROR: Target instance must be given as host:port. Credentials will be taken from the main session and, if given, must match them.|ReplicaSet.rejoinInstance: Invalid target instance specification (ArgumentError)

//@<OUT> Rejoin instance with replication stopped (succeed).
* Validating instance...
* Rejoining instance to replicaset...
** Configuring <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> to replicate from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>
** Checking replication channel status...
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
* Rejoining instance to replicaset...
** Configuring <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> to replicate from <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>
** Checking replication channel status...
* Updating the Metadata...
The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>' rejoined the replicaset and is replicating from <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>.

//@ Try to rejoin instance with errant transaction (fail).
|ERROR: <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> has 1 errant transactions that have not originated from the current PRIMARY (<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>)|ReplicaSet.rejoinInstance: Errant transactions at <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> (MYSQLSH 51152)

//@ Try to rejoin instance with unsolved replication error (fail).
|ERROR: Issue found in the replication channel. Please fix the error and retry to join the instance.|ReplicaSet.rejoinInstance: Replication applier thread error:

//@ Rejoin instance after solving replication error (succeed).
||

//@ Rejoin instance with connection failure, rpl user password reset (succeed).
||

//@ Try to rejoin instance with purged transactions on PRIMARY (fail).
|ERROR: <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> is missing 1 transactions that have been purged from the current PRIMARY (<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>)|ReplicaSet.rejoinInstance: Missing purged transactions at <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> (MYSQLSH 51153)
