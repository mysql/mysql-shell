//@# INCLUDE async_utils.inc
||

//@<OUT> Create ReplicaSet another operation in progress (fail)
ERROR: The operation cannot be executed because it failed to acquire the lock on instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.

//@<ERR> Create ReplicaSet another operation in progress (fail)
Dba.createReplicaSet: Failed to acquire lock on instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>' (MYSQLSH 51500)

//@<OUT> Configure instance when another operation in progress on the target (fail)
ERROR: The operation cannot be executed because it failed to acquire the lock on instance '<<<localhost>>>:<<<__mysql_sandbox_port2>>>'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.

//@<ERR> Configure instance when another operation in progress on the target (fail)
Dba.configureReplicaSetInstance: Failed to acquire lock on instance '<<<localhost>>>:<<<__mysql_sandbox_port2>>>' (MYSQLSH 51500)

//@<OUT> Add instance when another operation holds an exclusive lock on the primary (fail).
ERROR: The operation cannot be executed because it failed to acquire the lock on instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.

//@<ERR> Add instance when another operation holds an exclusive lock on the primary (fail).
ReplicaSet.addInstance: Failed to acquire lock on instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>' (MYSQLSH 51500)

//@<OUT> Add instance when another operation in progress on the target (fail)
ERROR: The operation cannot be executed because it failed to acquire the lock on instance '<<<localhost>>>:<<<__mysql_sandbox_port2>>>'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.

//@<ERR> Add instance when another operation in progress on the target (fail)
ReplicaSet.addInstance: Failed to acquire lock on instance '<<<localhost>>>:<<<__mysql_sandbox_port2>>>' (MYSQLSH 51500)

//@<OUT> Remove instance when another operation in progress on the target (fail)
ERROR: The operation cannot be executed because it failed to acquire the lock on instance '<<<localhost>>>:<<<__mysql_sandbox_port2>>>'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.

//@<ERR> Remove instance when another operation in progress on the target (fail)
ReplicaSet.removeInstance: Failed to acquire lock on instance '<<<localhost>>>:<<<__mysql_sandbox_port2>>>' (MYSQLSH 51500)

//@<OUT> Remove instance when another operation holds an exclusive lock on the primary (fail).
ERROR: The operation cannot be executed because it failed to acquire the lock on instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.

//@<ERR> Remove instance when another operation holds an exclusive lock on the primary (fail).
ReplicaSet.removeInstance: Failed to acquire lock on instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>' (MYSQLSH 51500)

//@<OUT> Rejoin instance when another operation in progress on the target (fail)
ERROR: The operation cannot be executed because it failed to acquire the lock on instance '<<<localhost>>>:<<<__mysql_sandbox_port2>>>'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.

//@<ERR> Rejoin instance when another operation in progress on the target (fail)
ReplicaSet.rejoinInstance: Failed to acquire lock on instance '<<<localhost>>>:<<<__mysql_sandbox_port2>>>' (MYSQLSH 51500)

//@<OUT> Rejoin instance when another operation holds an exclusive lock on the primary (fail).
ERROR: The operation cannot be executed because it failed to acquire the lock on instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.

//@<ERR> Rejoin instance when another operation holds an exclusive lock on the primary (fail).
ReplicaSet.rejoinInstance: Failed to acquire lock on instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>' (MYSQLSH 51500)

//@<OUT> (BUG#30673719) rejoinInstance on primary should not fail due to double self lock (using localhost).
The instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>' is ONLINE and the primary of the ReplicaSet.

//@<OUT> (BUG#30673719) rejoinInstance on primary should not fail due to double self lock (using hostname).
The instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>' is ONLINE and the primary of the ReplicaSet.

//@<OUT> (BUG#30673719) rejoinInstance on primary should not fail due to double self lock (using hostname_ip).
The instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>' is ONLINE and the primary of the ReplicaSet.

//@<ERR> (BUG#30673719) addInstance on primary should not fail due to double self lock (using localhost).
ReplicaSet.addInstance: <<<localhost>>>:<<<__mysql_sandbox_port1>>> is already a member of this replicaset. (MYSQLSH 51301)

//@<ERR> (BUG#30673719) addInstance on primary should not fail due to double self lock (using hostname).
ReplicaSet.addInstance: <<<localhost>>>:<<<__mysql_sandbox_port1>>> is already a member of this replicaset. (MYSQLSH 51301)

//@<ERR> (BUG#30673719) addInstance on primary should not fail due to double self lock (using hostname_ip).
ReplicaSet.addInstance: <<<localhost>>>:<<<__mysql_sandbox_port1>>> is already a member of this replicaset. (MYSQLSH 51301)

//@<OUT> Set primary instance when another operation in progress on the replica set (fail)
ERROR: The operation cannot be executed because it failed to acquire the lock on instance 'localhost:<<<__locked_port>>>'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.

//@<ERR> Set primary instance when another operation in progress on the replica set (fail)
ReplicaSet.setPrimaryInstance: Failed to acquire lock on instance 'localhost:<<<__locked_port>>>' (MYSQLSH 51500)

//@<OUT> Force primary instance when another operation in progress on the replica set (fail)
ERROR: The operation cannot be executed because it failed to acquire the lock on instance '<<<localhost>>>:<<<__locked_port>>>'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.

//@<ERR> Force primary instance when another operation in progress on the replica set (fail)
ReplicaSet.forcePrimaryInstance: Failed to acquire lock on instance '<<<localhost>>>:<<<__locked_port>>>' (MYSQLSH 51500)

//@<OUT> Remove router metadata when another operation holds an exclusive lock on the primary (fail).
ERROR: The operation cannot be executed because it failed to acquire the lock on instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.

//@<ERR> Remove router metadata when another operation holds an exclusive lock on the primary (fail).
ReplicaSet.removeRouterMetadata: Failed to acquire lock on instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>' (MYSQLSH 51500)

//@<OUT> Upgrade Metadata another operation in progress (fail)
ERROR: The operation cannot be executed because it failed to acquire the lock on instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.

//@<ERR> Upgrade Metadata another operation in progress (fail)
Dba.upgradeMetadata: Failed to acquire lock on instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>' (MYSQLSH 51500)
