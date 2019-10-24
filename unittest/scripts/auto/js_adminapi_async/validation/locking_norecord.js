//@# INCLUDE async_utils.inc
||

//@<OUT> Create ReplicaSet another operation in progress (fail)
ERROR: The operation cannot be executed because it failed to acquire the lock on instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>'. Another operation requiring exclusive access to the instance is still in progress, please wait for it to finish and try again.

//@<ERR> Create ReplicaSet another operation in progress (fail)
Dba.createReplicaSet: Failed to acquire lock on instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>' (MYSQLSH 51400)

//@<OUT> Configure instance when another operation in progress on the target (fail)
ERROR: The operation cannot be executed because it failed to acquire the lock on instance '<<<localhost>>>:<<<__mysql_sandbox_port2>>>'. Another operation requiring exclusive access to the instance is still in progress, please wait for it to finish and try again.

//@<ERR> Configure instance when another operation in progress on the target (fail)
Dba.configureReplicaSetInstance: Failed to acquire lock on instance '<<<localhost>>>:<<<__mysql_sandbox_port2>>>' (MYSQLSH 51400)

//@<OUT> Add instance when another operation holds an exclusive lock on the primary (fail).
ERROR: The operation cannot be executed because it failed to acquire the lock on instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>'. Another operation requiring exclusive access to the instance is still in progress, please wait for it to finish and try again.

//@<ERR> Add instance when another operation holds an exclusive lock on the primary (fail).
ReplicaSet.addInstance: Failed to acquire lock on instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>' (MYSQLSH 51400)

//@<OUT> Add instance when another operation in progress on the target (fail)
ERROR: The operation cannot be executed because it failed to acquire the lock on instance '<<<localhost>>>:<<<__mysql_sandbox_port2>>>'. Another operation requiring exclusive access to the instance is still in progress, please wait for it to finish and try again.

//@<ERR> Add instance when another operation in progress on the target (fail)
ReplicaSet.addInstance: Failed to acquire lock on instance '<<<localhost>>>:<<<__mysql_sandbox_port2>>>' (MYSQLSH 51400)

//@<OUT> Remove instance when another operation in progress on the target (fail)
ERROR: The operation cannot be executed because it failed to acquire the lock on instance '<<<localhost>>>:<<<__mysql_sandbox_port2>>>'. Another operation requiring exclusive access to the instance is still in progress, please wait for it to finish and try again.

//@<ERR> Remove instance when another operation in progress on the target (fail)
ReplicaSet.removeInstance: Failed to acquire lock on instance '<<<localhost>>>:<<<__mysql_sandbox_port2>>>' (MYSQLSH 51400)

//@<OUT> Remove instance when another operation holds an exclusive lock on the primary (fail).
ERROR: The operation cannot be executed because it failed to acquire the lock on instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>'. Another operation requiring exclusive access to the instance is still in progress, please wait for it to finish and try again.

//@<ERR> Remove instance when another operation holds an exclusive lock on the primary (fail).
ReplicaSet.removeInstance: Failed to acquire lock on instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>' (MYSQLSH 51400)

//@<OUT> Rejoin instance when another operation in progress on the target (fail)
ERROR: The operation cannot be executed because it failed to acquire the lock on instance '<<<localhost>>>:<<<__mysql_sandbox_port2>>>'. Another operation requiring exclusive access to the instance is still in progress, please wait for it to finish and try again.

//@<ERR> Rejoin instance when another operation in progress on the target (fail)
ReplicaSet.rejoinInstance: Failed to acquire lock on instance '<<<localhost>>>:<<<__mysql_sandbox_port2>>>' (MYSQLSH 51400)

//@<OUT> Rejoin instance when another operation holds an exclusive lock on the primary (fail).
ERROR: The operation cannot be executed because it failed to acquire the lock on instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>'. Another operation requiring exclusive access to the instance is still in progress, please wait for it to finish and try again.

//@<ERR> Rejoin instance when another operation holds an exclusive lock on the primary (fail).
ReplicaSet.rejoinInstance: Failed to acquire lock on instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>' (MYSQLSH 51400)

//@<ERR> (BUG#30673719) rejoinInstance on primary should not fail due to double self lock (using localhost).
ReplicaSet.rejoinInstance: Invalid status to execute operation, <<<localhost>>>:<<<__mysql_sandbox_port1>>> is ONLINE (MYSQLSH 51125)

//@<ERR> (BUG#30673719) rejoinInstance on primary should not fail due to double self lock (using hostname).
ReplicaSet.rejoinInstance: Invalid status to execute operation, <<<localhost>>>:<<<__mysql_sandbox_port1>>> is ONLINE (MYSQLSH 51125)

//@<ERR> (BUG#30673719) rejoinInstance on primary should not fail due to double self lock (using hostname_ip).
ReplicaSet.rejoinInstance: Invalid status to execute operation, <<<localhost>>>:<<<__mysql_sandbox_port1>>> is ONLINE (MYSQLSH 51125)

//@<ERR> (BUG#30673719) addInstance on primary should not fail due to double self lock (using localhost).
ReplicaSet.addInstance: <<<localhost>>>:<<<__mysql_sandbox_port1>>> is already a member of this replicaset. (MYSQLSH 51301)

//@<ERR> (BUG#30673719) addInstance on primary should not fail due to double self lock (using hostname).
ReplicaSet.addInstance: <<<localhost>>>:<<<__mysql_sandbox_port1>>> is already a member of this replicaset. (MYSQLSH 51301)

//@<ERR> (BUG#30673719) addInstance on primary should not fail due to double self lock (using hostname_ip).
ReplicaSet.addInstance: <<<localhost>>>:<<<__mysql_sandbox_port1>>> is already a member of this replicaset. (MYSQLSH 51301)

//@<OUT> Set primary instance when another operation in progress on the replica set (fail)
ERROR: The operation cannot be executed because it failed to acquire the lock on instance 'localhost:<<<__locked_port>>>'. Another operation requiring exclusive access to the instance is still in progress, please wait for it to finish and try again.

//@<ERR> Set primary instance when another operation in progress on the replica set (fail)
ReplicaSet.setPrimaryInstance: Failed to acquire lock on instance 'localhost:<<<__locked_port>>>' (MYSQLSH 51400)

//@<OUT> Force primary instance when another operation in progress on the replica set (fail)
ERROR: The operation cannot be executed because it failed to acquire the lock on instance '<<<localhost>>>:<<<__locked_port>>>'. Another operation requiring exclusive access to the instance is still in progress, please wait for it to finish and try again.

//@<ERR> Force primary instance when another operation in progress on the replica set (fail)
ReplicaSet.forcePrimaryInstance: Failed to acquire lock on instance '<<<localhost>>>:<<<__locked_port>>>' (MYSQLSH 51400)

//@<OUT> Remove router metadata when another operation holds an exclusive lock on the primary (fail).
ERROR: The operation cannot be executed because it failed to acquire the lock on instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>'. Another operation requiring exclusive access to the instance is still in progress, please wait for it to finish and try again.

//@<ERR> Remove router metadata when another operation holds an exclusive lock on the primary (fail).
ReplicaSet.removeRouterMetadata: Failed to acquire lock on instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>' (MYSQLSH 51400)

//@<OUT> Upgrade Metadata another operation in progress (fail)
ERROR: The operation cannot be executed because it failed to acquire the lock on instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>'. Another operation requiring exclusive access to the instance is still in progress, please wait for it to finish and try again.

//@<ERR> Upgrade Metadata another operation in progress (fail)
Dba.upgradeMetadata: Failed to acquire lock on instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>' (MYSQLSH 51400)

//@<OUT> Metadata lock error for remove instance (fail) {__dbug_off == 0}
ERROR: Cannot update the metadata because the maximum wait time to acquire a write lock has been reached.
Other operations requiring exclusive access to the metadata are running concurrently, please wait for those operations to finish and try again.

//@<ERR> Metadata lock error for remove instance (fail) {__dbug_off == 0}
ReplicaSet.removeInstance: Failed to acquire lock to update the metadata on instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>', wait timeout exceeded (MYSQLSH 51401)

//@<OUT> Metadata lock error for add instance (fail) {__dbug_off == 0}
ERROR: Cannot update the metadata because the maximum wait time to acquire a write lock has been reached.
Other operations requiring exclusive access to the metadata are running concurrently, please wait for those operations to finish and try again.

//@<ERR> Metadata lock error for add instance (fail) {__dbug_off == 0}
ReplicaSet.addInstance: Failed to acquire lock to update the metadata on instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>', wait timeout exceeded (MYSQLSH 51401)

//@<OUT> Metadata lock error for rejoin instance (fail) {__dbug_off == 0}
ERROR: Cannot update the metadata because the maximum wait time to acquire a write lock has been reached.
Other operations requiring exclusive access to the metadata are running concurrently, please wait for those operations to finish and try again.

//@<ERR> Metadata lock error for rejoin instance (fail) {__dbug_off == 0}
ReplicaSet.rejoinInstance: Failed to acquire lock to update the metadata on instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>', wait timeout exceeded (MYSQLSH 51401)

//@<OUT> Metadata lock error for remove router metadata (fail) {__dbug_off == 0}
ERROR: Cannot update the metadata because the maximum wait time to acquire a write lock has been reached.
Other operations requiring exclusive access to the metadata are running concurrently, please wait for those operations to finish and try again.

//@<ERR> Metadata lock error for remove router metadata (fail) {__dbug_off == 0}
ReplicaSet.removeRouterMetadata: Failed to acquire lock to update the metadata on instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>', wait timeout exceeded (MYSQLSH 51401)
