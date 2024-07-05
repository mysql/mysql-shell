//@ Cluster: addInstance with interaction, error
||The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is already part of this InnoDB Cluster

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

* Checking connectivity and SSL configuration...

A new instance will be added to the InnoDB Cluster. Depending on the amount of
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

?{VER(<8.0.27)}
NOTE: The 'localAddress' "<<<hostname>>>" is [[*]], which is compatible with the Group Replication automatically generated list of IPs.
See https://dev.mysql.com/doc/refman/en/group-replication-ip-address-permissions.html for more details.
?{}
* Checking connectivity and SSL configuration...

A new instance will be added to the InnoDB Cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' cannot persist Group Replication configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureInstance() command locally, using the 'mycnfPath' option, to persist the changes.
Adding instance to the cluster...

Monitoring recovery process of the new cluster member. Press ^C to stop monitoring and let it continue in background.

//@<OUT> Cluster: addInstance with interaction, ok {VER(<8.0.11)}
{{State recovery already finished for '<<<hostname>>>:<<<__mysql_sandbox_port2>>>'|Incremental state recovery is now in progress.}}

//@<OUT> Cluster: addInstance with interaction, ok {VER(<8.0.11)}
WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' cannot persist configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureInstance() command locally, using the 'mycnfPath' option, to persist the changes.
The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was successfully added to the cluster.

//@<OUT> Cluster: addInstance 3 with interaction, ok {VER(>=8.0.11)}
Validating instance configuration at localhost:<<<__mysql_sandbox_port3>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port3>>>

Instance configuration is suitable.
NOTE: Group Replication will communicate with other members using '<<<hostname>>>:<<<__mysql_sandbox_gr_port3>>>'. Use the localAddress option to override.

?{VER(<8.0.27)}
NOTE: The 'localAddress' "<<<hostname>>>" is [[*]], which is compatible with the Group Replication automatically generated list of IPs.
See https://dev.mysql.com/doc/refman/en/group-replication-ip-address-permissions.html for more details.
?{}
* Checking connectivity and SSL configuration...

A new instance will be added to the InnoDB Cluster. Depending on the amount of
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

?{VER(<8.0.27)}
NOTE: The 'localAddress' "<<<hostname>>>" is [[*]], which is compatible with the Group Replication automatically generated list of IPs.
See https://dev.mysql.com/doc/refman/en/group-replication-ip-address-permissions.html for more details.
?{}
* Checking connectivity and SSL configuration...

A new instance will be added to the InnoDB Cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' cannot persist Group Replication configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureInstance() command locally, using the 'mycnfPath' option, to persist the changes.
Adding instance to the cluster...

Monitoring recovery process of the new cluster member. Press ^C to stop monitoring and let it continue in background.

//@<OUT> Cluster: addInstance 3 with interaction, ok {VER(<8.0.11)}
{{State recovery already finished for '<<<hostname>>>:<<<__mysql_sandbox_port3>>>'|Incremental state recovery is now in progress.}}

//@<OUT> Cluster: addInstance 3 with interaction, ok {VER(<8.0.11)}
WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' cannot persist configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureInstance() command locally, using the 'mycnfPath' option, to persist the changes.
WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' cannot persist configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureInstance() command locally, using the 'mycnfPath' option, to persist the changes.
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
The cluster has the following registered instances:
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
Operation canceled by user. (RuntimeError)

//@<OUT> Cluster: addInstance with interaction, ok 3 {VER(>=8.0.11)}
Validating instance configuration at localhost:<<<__mysql_sandbox_port2>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port2>>>

Instance configuration is suitable.
NOTE: Group Replication will communicate with other members using '<<<hostname>>>:<<<__mysql_sandbox_gr_port2>>>'. Use the localAddress option to override.

?{VER(<8.0.27)}
NOTE: The 'localAddress' "<<<hostname>>>" is [[*]], which is compatible with the Group Replication automatically generated list of IPs.
See https://dev.mysql.com/doc/refman/en/group-replication-ip-address-permissions.html for more details.
?{}
* Checking connectivity and SSL configuration...

A new instance will be added to the InnoDB Cluster. Depending on the amount of
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
WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' cannot persist configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureInstance() command locally, using the 'mycnfPath' option, to persist the changes.
The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was successfully added to the cluster.

//@<OUT> Cluster: addInstance with interaction, ok 4 {VER(>=8.0.11)}
Validating instance configuration at localhost:<<<__mysql_sandbox_port3>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port3>>>

Instance configuration is suitable.
NOTE: Group Replication will communicate with other members using '<<<hostname>>>:<<<__mysql_sandbox_gr_port3>>>'. Use the localAddress option to override.

?{VER(<8.0.27)}
NOTE: The 'localAddress' "<<<hostname>>>" is [[*]], which is compatible with the Group Replication automatically generated list of IPs.
See https://dev.mysql.com/doc/refman/en/group-replication-ip-address-permissions.html for more details.
?{}
* Checking connectivity and SSL configuration...

A new instance will be added to the InnoDB Cluster. Depending on the amount of
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

?{VER(<8.0.27)}
NOTE: The 'localAddress' "<<<hostname>>>" is [[*]], which is compatible with the Group Replication automatically generated list of IPs.
See https://dev.mysql.com/doc/refman/en/group-replication-ip-address-permissions.html for more details.
?{}
* Checking connectivity and SSL configuration...

A new instance will be added to the InnoDB Cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' cannot persist Group Replication configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureInstance() command locally, using the 'mycnfPath' option, to persist the changes.
Adding instance to the cluster...

Monitoring recovery process of the new cluster member. Press ^C to stop monitoring and let it continue in background.

//@<OUT> Cluster: addInstance with interaction, ok 4 {VER(<8.0.11)}
{{State recovery already finished for '<<<hostname>>>:<<<__mysql_sandbox_port3>>>'|Incremental state recovery is now in progress.}}

//@<OUT> Cluster: addInstance with interaction, ok 4 {VER(<8.0.11)}
WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' cannot persist configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureInstance() command locally, using the 'mycnfPath' option, to persist the changes.
WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' cannot persist configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureInstance() command locally, using the 'mycnfPath' option, to persist the changes.
The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' was successfully added to the cluster.

//@# Dba: kill instance 3
||

//@# Dba: start instance 3
||

//@<OUT> Cluster: rejoinInstance with interaction, ok
<<<(__version_num>=80011) ? "NOTE: The instance 'localhost:"+__mysql_sandbox_port3+"' is running auto-rejoin process, which will be cancelled.\n\n":""\>>>
The safest and most convenient way to provision a new instance is through automatic clone provisioning, which will completely overwrite the state of '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' with a physical snapshot from an existing cluster member. To use this method by default, set the 'recoveryMethod' option to 'clone'.

The incremental state recovery may be safely used if you are sure all updates ever executed in the cluster were done with GTIDs enabled, there are no purged transactions and the new instance contains the same GTID set as the cluster or a subset of it. To use this method by default, set the 'recoveryMethod' option to 'incremental'.

Incremental state recovery was selected because it seems to be safely usable.

Validating instance configuration at localhost:<<<__mysql_sandbox_port3>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port3>>>

Instance configuration is suitable.
Rejoining instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' to cluster 'devCluster'...

<<<(__version_num>=80011) ? "NOTE: Cancelling active GR auto-initialization at "+hostname+":"+__mysql_sandbox_port3+"\n":""\>>>
?{VER(>=8.0.27)}
Re-creating recovery account...
NOTE: User '<<<repl_user>>>'@'%' already existed at instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'. It will be deleted and created again with a new password.
?{}
Monitoring recovery process of the new cluster member. Press ^C to stop monitoring and let it continue in background.
State recovery already finished for '<<<hostname>>>:<<<__mysql_sandbox_port3>>>'

The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' was successfully rejoined to the cluster.

//@<OUT> Cluster: final dissolve
The cluster has the following registered instances:
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
* Waiting for instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' to apply received transactions...


* Waiting for instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' to apply received transactions...


* Waiting for instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' to apply received transactions...


* Dissolving the Cluster...
* Waiting for instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' to apply received transactions...


* Instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is attempting to leave the cluster...
<<<(__version_num<80011)?"WARNING: On instance '"+hostname+":"+__mysql_sandbox_port1+"' configuration cannot be persisted since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please set the 'group_replication_start_on_boot' variable to 'OFF' in the server configuration file, otherwise it might rejoin the cluster upon restart.\n":""\>>>
* Waiting for instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' to apply received transactions...


* Instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is attempting to leave the cluster...
<<<(__version_num<80011)?"WARNING: On instance '"+hostname+":"+__mysql_sandbox_port2+"' configuration cannot be persisted since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please set the 'group_replication_start_on_boot' variable to 'OFF' in the server configuration file, otherwise it might rejoin the cluster upon restart.\n":""\>>>
* Waiting for instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' to apply received transactions...


* Instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' is attempting to leave the cluster...
<<<(__version_num<80011)?"WARNING: On instance '"+hostname+":"+__mysql_sandbox_port3+"' configuration cannot be persisted since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please set the 'group_replication_start_on_boot' variable to 'OFF' in the server configuration file, otherwise it might rejoin the cluster upon restart.\n":""\>>>

The cluster was successfully dissolved.
Replication was disabled but user data was left intact.

