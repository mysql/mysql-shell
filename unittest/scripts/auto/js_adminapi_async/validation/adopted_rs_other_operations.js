//@# INCLUDE async_utils.inc
||

//@ Status.
|    "primary": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",|
|    "status": "AVAILABLE",|
|"address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>", |
|"instanceRole": "PRIMARY", |
|"status": "ONLINE"|
|"address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>", |
|"instanceRole": "SECONDARY", |
|"status": "ONLINE"|

//@ Status, instance 2 OFFLINE.
|    "primary": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",|
|    "status": "AVAILABLE_PARTIAL",|
|"address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>", |
|"instanceRole": "PRIMARY", |
|"status": "ONLINE"|
|"address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>", |
|"instanceRole": "SECONDARY", |
|"status": "OFFLINE"|

//@<OUT> Rejoin instance 2 (succeed).
* Validating instance...
** Checking transaction state of the instance...
The safest and most convenient way to provision a new instance is through automatic clone provisioning, which will completely overwrite the state of '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' with a physical snapshot from an existing replicaset member. To use this method by default, set the 'recoveryMethod' option to 'clone'.

WARNING: It should be safe to rely on replication to incrementally recover the state of the new instance if you are sure all updates ever executed in the replicaset were done with GTIDs enabled, there are no purged transactions and the new instance contains the same GTID set as the replicaset or a subset of it. To use this method by default, set the 'recoveryMethod' option to 'incremental'.

Incremental state recovery was selected because it seems to be safely usable.

* Rejoining instance to replicaset...
** Configuring <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> to replicate from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>
** Checking replication channel status...
** Waiting for rejoined instance to synchronize with PRIMARY...

* Updating the Metadata...
The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' rejoined the replicaset and is replicating from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>.

//@ Status (after rejoin).
|    "primary": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",|
|    "status": "AVAILABLE",|
|"address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>", |
|"instanceRole": "PRIMARY", |
|"status": "ONLINE"|
|"address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>", |
|"instanceRole": "SECONDARY", |
|"status": "ONLINE"|