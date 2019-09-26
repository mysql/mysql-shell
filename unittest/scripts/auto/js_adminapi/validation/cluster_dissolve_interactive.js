//@ Initialization
||

//@ Create cluster, disable interactive mode.
||

//@<OUT> Dissolve cluster, disable interactive mode.
Instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is attempting to leave the cluster...
<<<(__version_num<80011)?"WARNING: On instance '"+hostname+":"+__mysql_sandbox_port1+"' configuration cannot be persisted since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please set the 'group_replication_start_on_boot' variable to 'OFF' in the server configuration file, otherwise it might rejoin the cluster upon restart.\n":""\>>>

The cluster was successfully dissolved.
Replication was disabled but user data was left intact.

//@ Create cluster, with unreachable instances.
||

//@ Add instance on port2.
||

//@ Add instance on port3.
||

//@ Reset persisted gr_start_on_boot on instance3 {VER(>=8.0.11)}
||

//@ Stop instance on port3 and wait for it to be unreachable (missing).
||

//@<OUT> Dissolve stopped because instance is unreachable and user answer 'no' to continue.
The cluster still has the following registered instances:
{
    "clusterName": "c",
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
WARNING: You are about to dissolve the whole cluster and lose the high availability features provided by it. This operation cannot be reverted. All members will be removed from the cluster and replication will be stopped, internal recovery user accounts and the cluster metadata will be dropped. User data will be maintained intact in all instances.

Are you sure you want to dissolve the cluster? [y/N]:
ERROR: The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' cannot be removed because it is on a '(MISSING)' state. Please bring the instance back ONLINE and try to dissolve the cluster again. If the instance is permanently not reachable, then you can choose to proceed with the operation and only remove the instance from the Cluster Metadata.

Do you want to continue anyway (only the instance metadata will be removed)? [y/N]:


//@<ERR> Dissolve stopped because instance is unreachable and user answer 'no' to continue.
Cluster.dissolve: The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' is '(MISSING)' (RuntimeError)

//@<OUT> Dissolve stopped because one instance is unreachable (force: false).
The cluster still has the following registered instances:
{
    "clusterName": "c",
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
WARNING: You are about to dissolve the whole cluster and lose the high availability features provided by it. This operation cannot be reverted. All members will be removed from the cluster and replication will be stopped, internal recovery user accounts and the cluster metadata will be dropped. User data will be maintained intact in all instances.

Are you sure you want to dissolve the cluster? [y/N]:
ERROR: The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' cannot be removed because it is on a '(MISSING)' state. Please bring the instance back ONLINE and try to dissolve the cluster again. If the instance is permanently not reachable, then please use <Cluster>.dissolve() with the force option set to true to proceed with the operation and only remove the instance from the Cluster Metadata.


//@<ERR> Dissolve stopped because one instance is unreachable (force: false).
Cluster.dissolve: The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' is '(MISSING)' (RuntimeError)

//@<OUT> Dissolve continues because instance is unreachable and user answer 'yes' to continue.
The cluster still has the following registered instances:
{
    "clusterName": "c",
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
WARNING: You are about to dissolve the whole cluster and lose the high availability features provided by it. This operation cannot be reverted. All members will be removed from the cluster and replication will be stopped, internal recovery user accounts and the cluster metadata will be dropped. User data will be maintained intact in all instances.

Are you sure you want to dissolve the cluster? [y/N]:
ERROR: The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' cannot be removed because it is on a '(MISSING)' state. Please bring the instance back ONLINE and try to dissolve the cluster again. If the instance is permanently not reachable, then you can choose to proceed with the operation and only remove the instance from the Cluster Metadata.

Do you want to continue anyway (only the instance metadata will be removed)? [y/N]:
Instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is attempting to leave the cluster...
<<<(__version_num<80011)?"WARNING: On instance '"+hostname+":"+__mysql_sandbox_port2+"' configuration cannot be persisted since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please set the 'group_replication_start_on_boot' variable to 'OFF' in the server configuration file, otherwise it might rejoin the cluster upon restart.\n":""\>>>
Instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is attempting to leave the cluster...
<<<(__version_num<80011)?"WARNING: On instance '"+hostname+":"+__mysql_sandbox_port1+"' configuration cannot be persisted since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please set the 'group_replication_start_on_boot' variable to 'OFF' in the server configuration file, otherwise it might rejoin the cluster upon restart.\n":""\>>>

WARNING: The cluster was successfully dissolved, but the following instance was skipped: '<<<hostname>>>:<<<__mysql_sandbox_port3>>>'. Please make sure this instance is permanently unavailable or take any necessary manual action to ensure the cluster is fully dissolved.

//@ Dissolve post action on unreachable instance (ensure GR is not started)
||

//@ Restart instance on port3
||

//@ Create cluster, instance with replication errors.
||

//@ Add instance on port2, again.
||

//@ Add instance on port3, again.
||

//@ Connect to instance on port2 to introduce a replication error
||

//@ Avoid server from aborting (killing itself) {VER(>=8.0.12)}
||

//@ Connect to seed and get cluster
||

//@ Change shell option dba.gtidWaitTimeout to 1 second (to issue error faster)
||

//@ Execute trx that will lead to error on instance2
||

//@<OUT> Dissolve stopped because instance cannot catch up with cluster and user answer 'n' to continue.
The cluster still has the following registered instances:
{
    "clusterName": "c",
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
WARNING: You are about to dissolve the whole cluster and lose the high availability features provided by it. This operation cannot be reverted. All members will be removed from the cluster and replication will be stopped, internal recovery user accounts and the cluster metadata will be dropped. User data will be maintained intact in all instances.

Are you sure you want to dissolve the cluster? [y/N]:
ERROR: The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was unable to catch up with cluster transactions. There might be too many transactions to apply or some replication error. In the former case, you can retry the operation (using a higher timeout value by setting the global shell option 'dba.gtidWaitTimeout'). In the later case, analyze and fix any replication error. You can also choose to skip this error using the 'force: true' option, but it might leave the instance in an inconsistent state and lead to errors if you want to reuse it.

Do you want to continue anyway (only the instance metadata will be removed)? [y/N]:


//@<ERR> Dissolve stopped because instance cannot catch up with cluster and user answer 'n' to continue.
Cluster.dissolve: Timeout reached waiting for cluster transactions to be applied on instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' (RuntimeError)


//@ Connect to instance on port2 to fix error and add new one
||

//@ Connect to seed and get cluster again
||

//@ Execute trx that will lead to error on instance2, again
||

//@<OUT> Dissolve stopped because instance cannot catch up with cluster (force: false).
The cluster still has the following registered instances:
{
    "clusterName": "c",
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
WARNING: You are about to dissolve the whole cluster and lose the high availability features provided by it. This operation cannot be reverted. All members will be removed from the cluster and replication will be stopped, internal recovery user accounts and the cluster metadata will be dropped. User data will be maintained intact in all instances.

Are you sure you want to dissolve the cluster? [y/N]:
ERROR: The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was unable to catch up with cluster transactions. There might be too many transactions to apply or some replication error. In the former case, you can retry the operation (using a higher timeout value by setting the global shell option 'dba.gtidWaitTimeout'). In the later case, analyze and fix any replication error. You can also choose to skip this error using the 'force: true' option, but it might leave the instance in an inconsistent state and lead to errors if you want to reuse it.


//@<ERR> Dissolve stopped because instance cannot catch up with cluster (force: false).
Cluster.dissolve: Timeout reached waiting for cluster transactions to be applied on instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' (RuntimeError)

//@ Connect to instance on port2 to fix error and add new one, one last time
||

//@ Connect to seed and get cluster one last time
||

//@ Execute trx that will lead to error on instance2, one last time
||

//@<OUT> Dissolve continues because instance cannot catch up with cluster and user answer 'y' to continue.
The cluster still has the following registered instances:
{
    "clusterName": "c",
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
WARNING: You are about to dissolve the whole cluster and lose the high availability features provided by it. This operation cannot be reverted. All members will be removed from the cluster and replication will be stopped, internal recovery user accounts and the cluster metadata will be dropped. User data will be maintained intact in all instances.

Are you sure you want to dissolve the cluster? [y/N]:
ERROR: The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was unable to catch up with cluster transactions. There might be too many transactions to apply or some replication error. In the former case, you can retry the operation (using a higher timeout value by setting the global shell option 'dba.gtidWaitTimeout'). In the later case, analyze and fix any replication error. You can also choose to skip this error using the 'force: true' option, but it might leave the instance in an inconsistent state and lead to errors if you want to reuse it.

Do you want to continue anyway (only the instance metadata will be removed)? [y/N]:
WARNING: An error occurred when trying to catch up with cluster transactions and instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' might have been left in an inconsistent state that will lead to errors if it is reused.

Instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is attempting to leave the cluster...
<<<(__version_num<80011)?"WARNING: On instance '"+hostname+":"+__mysql_sandbox_port2+"' configuration cannot be persisted since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please set the 'group_replication_start_on_boot' variable to 'OFF' in the server configuration file, otherwise it might rejoin the cluster upon restart.\n":""\>>>
Instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' is attempting to leave the cluster...
<<<(__version_num<80011)?"WARNING: On instance '"+hostname+":"+__mysql_sandbox_port3+"' configuration cannot be persisted since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please set the 'group_replication_start_on_boot' variable to 'OFF' in the server configuration file, otherwise it might rejoin the cluster upon restart.\n":""\>>>
Instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is attempting to leave the cluster...
<<<(__version_num<80011)?"WARNING: On instance '"+hostname+":"+__mysql_sandbox_port1+"' configuration cannot be persisted since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please set the 'group_replication_start_on_boot' variable to 'OFF' in the server configuration file, otherwise it might rejoin the cluster upon restart.\n":""\>>>

WARNING: The cluster was successfully dissolved, but the following instance was unable to catch up with the cluster transactions: '<<<hostname>>>:<<<__mysql_sandbox_port2>>>'. Please make sure the cluster metadata was removed on this instance in order to be able to be reused.

//@ Finalization
||
