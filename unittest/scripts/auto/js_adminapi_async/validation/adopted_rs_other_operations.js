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
* Rejoining instance to replicaset...
** Configuring <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> to replicate from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>
** Checking replication channel status...
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