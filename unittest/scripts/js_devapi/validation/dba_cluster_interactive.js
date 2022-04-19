//@ Initialization
||


//@ Cluster: addInstance errors
||Invalid number of arguments, expected 1 to 2 but got 0
||Invalid number of arguments, expected 1 to 2 but got 4
||Argument #2 is expected to be a map
||Argument #2 is expected to be a map
||Invalid connection options, expected either a URI or a Connection Options Dictionary
||Invalid URI: empty.
||Invalid values in connection options: ipWhitelist, memberSslMode
||Argument #2 is expected to be a map
||Invalid value for memberSslMode option. Supported values: AUTO,DISABLED,REQUIRED,VERIFY_CA,VERIFY_IDENTITY.
||Invalid value for memberSslMode option. Supported values: AUTO,DISABLED,REQUIRED,VERIFY_CA,VERIFY_IDENTITY.
||Invalid value for ipWhitelist: string value cannot be empty.
||The label can not be empty.
||The label can only start with an alphanumeric or the '_' character.
||The label can only contain alphanumerics or the '_', '.', '-', ':' characters. Invalid character '#' found.
||The label can not be greater than 256 characters.

//@ Cluster: addInstance with interaction, error
||The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is already part of this InnoDB cluster

//@<OUT> Cluster: addInstance with interaction, ok {VER(>=8.0.11)}
NOTE: The target instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' has not been pre-provisioned (GTID set is empty), but the cluster was configured to assume that incremental state recovery can correctly provision it in this case.
The safest and most convenient way to provision a new instance is through automatic clone provisioning, which will completely overwrite the state of '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' with a physical snapshot from an existing cluster member. To use this method by default, set the 'recoveryMethod' option to 'clone'.

The incremental state recovery may be safely used if you are sure all updates ever executed in the cluster were done with GTIDs enabled, there are no purged transactions and the new instance contains the same GTID set as the cluster or a subset of it. To use this method by default, set the 'recoveryMethod' option to 'incremental'.

Incremental state recovery was selected because it seems to be safely usable.

Validating instance configuration at localhost:<<<__mysql_sandbox_port2>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port2>>>

Instance configuration is suitable.
NOTE: Group Replication will communicate with other members using '<<<hostname>>>:<<<__mysql_sandbox_gr_port2>>>'. Use the localAddress option to override.

A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Adding instance to the cluster...

Monitoring recovery process of the new cluster member. Press ^C to stop monitoring and let it continue in background.

//@<OUT> Cluster: addInstance with interaction, ok {VER(>=8.0.11)}
{{State recovery already finished for '<<<hostname>>>:<<<__mysql_sandbox_port2>>>'|Incremental state recovery is now in progress.}}

//@<OUT> Cluster: addInstance with interaction, ok {VER(>=8.0.11)}
The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was successfully added to the cluster.

//@<OUT> Cluster: addInstance with interaction, ok {VER(<8.0.11)}
Validating instance configuration at localhost:<<<__mysql_sandbox_port2>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port2>>>

Instance configuration is suitable.
NOTE: Group Replication will communicate with other members using '<<<hostname>>>:<<<__mysql_sandbox_gr_port2>>>'. Use the localAddress option to override.

A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' cannot persist Group Replication configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.
Adding instance to the cluster...

Monitoring recovery process of the new cluster member. Press ^C to stop monitoring and let it continue in background.

//@<OUT> Cluster: addInstance with interaction, ok {VER(<8.0.11)}
{{State recovery already finished for '<<<hostname>>>:<<<__mysql_sandbox_port2>>>'|Incremental state recovery is now in progress.}}

//@<OUT> Cluster: addInstance with interaction, ok {VER(<8.0.11)}
WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' cannot persist configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.
The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was successfully added to the cluster.

//@<OUT> Cluster: addInstance 3 with interaction, ok {VER(>=8.0.11)}
Validating instance configuration at localhost:<<<__mysql_sandbox_port3>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port3>>>

Instance configuration is suitable.
NOTE: Group Replication will communicate with other members using '<<<hostname>>>:<<<__mysql_sandbox_gr_port3>>>'. Use the localAddress option to override.

A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Adding instance to the cluster...

Monitoring recovery process of the new cluster member. Press ^C to stop monitoring and let it continue in background.

//@<OUT> Cluster: addInstance 3 with interaction, ok {VER(>=8.0.11)}
{{State recovery already finished for '<<<hostname>>>:<<<__mysql_sandbox_port3>>>'|Incremental state recovery is now in progress.}}

//@<OUT> Cluster: addInstance 3 with interaction, ok {VER(>=8.0.11)}
The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' was successfully added to the cluster.

//@<OUT> Cluster: addInstance 3 with interaction, ok {VER(<8.0.11)}
Validating instance configuration at localhost:<<<__mysql_sandbox_port3>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port3>>>

Instance configuration is suitable.
NOTE: Group Replication will communicate with other members using '<<<hostname>>>:<<<__mysql_sandbox_gr_port3>>>'. Use the localAddress option to override.

A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' cannot persist Group Replication configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.
Adding instance to the cluster...

Monitoring recovery process of the new cluster member. Press ^C to stop monitoring and let it continue in background.

//@<OUT> Cluster: addInstance 3 with interaction, ok {VER(<8.0.11)}
{{State recovery already finished for '<<<hostname>>>:<<<__mysql_sandbox_port3>>>'|Incremental state recovery is now in progress.}}

//@<OUT> Cluster: addInstance 3 with interaction, ok {VER(<8.0.11)}
WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' cannot persist configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.
WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' cannot persist configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.
The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' was successfully added to the cluster.

//@<OUT> Cluster: describe1
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "topology": [
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            },
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "role": "HA"
            },
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "role": "HA"
            }
        ],
        "topologyMode": "Single-Primary"
    }
}

//@<ERR> Cluster: removeInstance errors
Cluster.removeInstance: Invalid number of arguments, expected 1 to 2 but got 0 (ArgumentError)
Cluster.removeInstance: Invalid number of arguments, expected 1 to 2 but got 3 (ArgumentError)
Cluster.removeInstance: Argument #1: Invalid connection options, expected either a URI or a Connection Options Dictionary (TypeError)
Cluster.removeInstance: Argument #1: Argument auth-method is expected to be a string (TypeError)
Cluster.removeInstance: Metadata for instance <<<__host>>>:<<<__mysql_port>>> not found [[*]]

//@ Cluster: removeInstance
||

//@<OUT> Cluster: describe2
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "topology": [
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            },
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "role": "HA"
            }
        ],
        "topologyMode": "Single-Primary"
    }
}

//@<OUT> Cluster: dissolve error: not empty
The cluster still has the following registered instances:
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "topology": [
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            },
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "role": "HA"
            }
        ],
        "topologyMode": "Single-Primary"
    }
}
WARNING: You are about to dissolve the whole cluster and lose the high availability features provided by it. This operation cannot be reverted. All members will be removed from the cluster and replication will be stopped, internal recovery user accounts and the cluster metadata will be dropped. User data will be maintained intact in all instances.

Are you sure you want to dissolve the cluster? [y/N]:

//@<ERR> Cluster: dissolve error: not empty
Cluster.dissolve: Operation canceled by user. (RuntimeError)

//@ Cluster: dissolve errors
||Argument #1 is expected to be a map
||Invalid number of arguments, expected 0 to 1 but got 2
||Argument #1 is expected to be a map
||Invalid options: enforce
||Option 'force' Bool expected, but value is String

//@ Cluster: remove_instance 3
||

//@<OUT> Cluster: addInstance with interaction, ok 3 {VER(>=8.0.11)}
Validating instance configuration at localhost:<<<__mysql_sandbox_port2>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port2>>>

Instance configuration is suitable.
NOTE: Group Replication will communicate with other members using '<<<hostname>>>:<<<__mysql_sandbox_gr_port2>>>'. Use the localAddress option to override.

A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Adding instance to the cluster...

Monitoring recovery process of the new cluster member. Press ^C to stop monitoring and let it continue in background.

//@<OUT> Cluster: addInstance with interaction, ok 3 {VER(>=8.0.11)}
{{State recovery already finished for '<<<hostname>>>:<<<__mysql_sandbox_port2>>>'|Incremental state recovery is now in progress.}}

//@<OUT> Cluster: addInstance with interaction, ok 3 {VER(>=8.0.11)}
The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was successfully added to the cluster.

//@<OUT> Cluster: addInstance with interaction, ok 3 {VER(<8.0.11)}
The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was successfully added to the cluster.

//@<OUT> Cluster: addInstance with interaction, ok 3 {VER(<8.0.11)}
{{State recovery already finished for '<<<hostname>>>:<<<__mysql_sandbox_port2>>>'|Incremental state recovery is now in progress.}}

//@<OUT> Cluster: addInstance with interaction, ok 3 {VER(<8.0.11)}
WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' cannot persist configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.
The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was successfully added to the cluster.

//@<OUT> Cluster: addInstance with interaction, ok 4 {VER(>=8.0.11)}
Validating instance configuration at localhost:<<<__mysql_sandbox_port3>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port3>>>

Instance configuration is suitable.
NOTE: Group Replication will communicate with other members using '<<<hostname>>>:<<<__mysql_sandbox_gr_port3>>>'. Use the localAddress option to override.

A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Adding instance to the cluster...

Monitoring recovery process of the new cluster member. Press ^C to stop monitoring and let it continue in background.

//@<OUT> Cluster: addInstance with interaction, ok 4 {VER(>=8.0.11)}
{{State recovery already finished for '<<<hostname>>>:<<<__mysql_sandbox_port3>>>'|Incremental state recovery is now in progress.}}

//@<OUT> Cluster: addInstance with interaction, ok 4 {VER(>=8.0.11)}
The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' was successfully added to the cluster.

//@<OUT> Cluster: addInstance with interaction, ok 4 {VER(<8.0.11)}
Validating instance configuration at localhost:<<<__mysql_sandbox_port3>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port3>>>

Instance configuration is suitable.
NOTE: Group Replication will communicate with other members using '<<<hostname>>>:<<<__mysql_sandbox_gr_port3>>>'. Use the localAddress option to override.

A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' cannot persist Group Replication configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.
Adding instance to the cluster...

Monitoring recovery process of the new cluster member. Press ^C to stop monitoring and let it continue in background.

//@<OUT> Cluster: addInstance with interaction, ok 4 {VER(<8.0.11)}
{{State recovery already finished for '<<<hostname>>>:<<<__mysql_sandbox_port3>>>'|Incremental state recovery is now in progress.}}

//@<OUT> Cluster: addInstance with interaction, ok 4 {VER(<8.0.11)}
WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' cannot persist configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.
WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' cannot persist configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.
The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' was successfully added to the cluster.

//@# Dba: kill instance 3
||

//@# Dba: start instance 3
||

//@: Cluster: rejoinInstance errors
||Invalid number of arguments, expected 1 to 2 but got 0
||Invalid number of arguments, expected 1 to 2 but got 3
||Invalid connection options, expected either a URI or a Connection Options Dictionary
||Invalid values in connection options: ipWhitelist, memberSslMode
||Argument #2 is expected to be a map
||Could not open connection to 'localhost'
||Could not open connection to 'localhost:3306'
||Invalid value for memberSslMode option. Supported values: AUTO,DISABLED,REQUIRED,VERIFY_CA,VERIFY_IDENTITY.
||Invalid value for memberSslMode option. Supported values: AUTO,DISABLED,REQUIRED,VERIFY_CA,VERIFY_IDENTITY.
||Invalid value for ipWhitelist: string value cannot be empty.

//@<OUT> Cluster: rejoinInstance with interaction, ok
<<<(__version_num>=80011) ? "NOTE: The instance 'localhost:"+__mysql_sandbox_port3+"' is running auto-rejoin process, which will be cancelled.\n\n":""\>>>
Validating instance configuration at localhost:<<<__mysql_sandbox_port3>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port3>>>

Instance configuration is suitable.
<<<(__version_num<80000) ? "WARNING: Instance '"+hostname+":"+__mysql_sandbox_port3+"' cannot persist Group Replication configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.\n":""\>>>
Rejoining instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' to cluster 'devCluster'...

<<<(__version_num>=80011) ? "NOTE: Cancelling active GR auto-initialization at "+hostname+":"+__mysql_sandbox_port3+"\n":""\>>>
?{VER(>=8.0.27)}
Re-creating recovery account...
NOTE: User '<<<repl_user>>>'@'%' already existed at instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'. It will be deleted and created again with a new password.
?{}
<<<(__version_num<80000) ? "WARNING: Instance '"+hostname+":"+__mysql_sandbox_port1+"' cannot persist configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.\n":""\>>>
<<<(__version_num<80000) ? "WARNING: Instance '"+hostname+":"+__mysql_sandbox_port2+"' cannot persist configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.\n":""\>>>
The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' was successfully rejoined to the cluster.

//@<OUT> Cluster: final dissolve
The cluster still has the following registered instances:
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "topology": [
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            },
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "label": "z2nd_sandbox",
                "role": "HA"
            },
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "label": "z3rd_sandbox",
                "role": "HA"
            }
        ],
        "topologyMode": "Single-Primary"
    }
}
WARNING: You are about to dissolve the whole cluster and lose the high availability features provided by it. This operation cannot be reverted. All members will be removed from the cluster and replication will be stopped, internal recovery user accounts and the cluster metadata will be dropped. User data will be maintained intact in all instances.

Are you sure you want to dissolve the cluster? [y/N]:
* Waiting for instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' to synchronize with the primary...


* Waiting for instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' to synchronize with the primary...


* Waiting for instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' to synchronize with the primary...


* Dissolving the Cluster...
* Waiting for instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' to synchronize with the primary...


* Instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is attempting to leave the cluster...
<<<(__version_num<80011)?"WARNING: On instance '"+hostname+":"+__mysql_sandbox_port2+"' configuration cannot be persisted since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please set the 'group_replication_start_on_boot' variable to 'OFF' in the server configuration file, otherwise it might rejoin the cluster upon restart.\n":""\>>>
* Waiting for instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' to synchronize with the primary...


* Instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' is attempting to leave the cluster...
<<<(__version_num<80011)?"WARNING: On instance '"+hostname+":"+__mysql_sandbox_port3+"' configuration cannot be persisted since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please set the 'group_replication_start_on_boot' variable to 'OFF' in the server configuration file, otherwise it might rejoin the cluster upon restart.\n":""\>>>
* Instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is attempting to leave the cluster...
<<<(__version_num<80011)?"WARNING: On instance '"+hostname+":"+__mysql_sandbox_port1+"' configuration cannot be persisted since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please set the 'group_replication_start_on_boot' variable to 'OFF' in the server configuration file, otherwise it might rejoin the cluster upon restart.\n":""\>>>

The cluster was successfully dissolved.
Replication was disabled but user data was left intact.

//@ Cluster: no operations can be done on an offline cluster
||Can't access object member 'name' on an offline cluster
||Can't call function 'addInstance' on an offline cluster
||Can't call function 'checkInstanceState' on an offline cluster
||Can't call function 'describe' on an offline cluster
||Can't call function 'dissolve' on an offline cluster
||Can't call function 'forceQuorumUsingPartitionOf' on an offline cluster
||Can't call function 'getName' on an offline cluster
||Can't call function 'rejoinInstance' on an offline cluster
||Can't call function 'removeInstance' on an offline cluster
||Can't call function 'rescan' on an offline cluster
||Can't call function 'status' on an offline cluster
||Can't call function 'listRouters' on an offline cluster
||The cluster object is disconnected. Please use dba.getCluster to obtain a fresh cluster handle. (RuntimeError)

//@ Cluster: disconnect() is ok on an offline cluster
||
