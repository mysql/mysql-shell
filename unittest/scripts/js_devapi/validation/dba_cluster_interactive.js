//@ Initialization
||

//@ Cluster: validating members
|Cluster Members: 19|
|name: OK|
|getName: OK|
|addInstance: OK|
|removeInstance: OK|
|rejoinInstance: OK|
|checkInstanceState: OK|
|describe: OK|
|status: OK|
|help: OK|
|dissolve: OK|
|disconnect: OK|
|rescan: OK|
|forceQuorumUsingPartitionOf: OK|
|switchToSinglePrimaryMode: OK|
|switchToMultiPrimaryMode: OK|
|setPrimaryInstance: OK|
|options: OK|
|setOption: OK|
|setInstanceOption: OK|

//@ Cluster: addInstance errors
||Cluster.addInstance: Invalid number of arguments, expected 1 to 2 but got 0
||Cluster.addInstance: Invalid number of arguments, expected 1 to 2 but got 4
||Invalid connection options, expected either a URI or a Dictionary
||Cluster.addInstance: Invalid URI: empty.
||Cluster.addInstance: Invalid values in connection options: ipWhitelist, memberSslMode
||Cluster.addInstance: Missing values in connection options: host
||Cluster.addInstance: Argument #2 is expected to be a map
||Invalid value for memberSslMode option. Supported values: AUTO,DISABLED,REQUIRED.
||Invalid value for memberSslMode option. Supported values: AUTO,DISABLED,REQUIRED.
||Invalid value for ipWhitelist: string value cannot be empty.

//@ Cluster: addInstance with interaction, error
||Cluster.addInstance: The instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>' is already part of this InnoDB cluster

//@<OUT> Cluster: addInstance with interaction, ok {VER(>=8.0.11)}
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Adding instance to the cluster ...

Validating instance at localhost:<<<__mysql_sandbox_port2>>>...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Instance configuration is suitable.
The instance 'root@localhost:<<<__mysql_sandbox_port2>>>' was successfully added to the cluster.

//@<OUT> Cluster: addInstance with interaction, ok {VER(<8.0.11)}
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Adding instance to the cluster ...

Validating instance at localhost:<<<__mysql_sandbox_port2>>>...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Instance configuration is suitable.
WARNING: On instance 'localhost:<<<__mysql_sandbox_port2>>>' membership change cannot be persisted since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance command locally to persist the changes.
WARNING: On instance 'localhost:<<<__mysql_sandbox_port1>>>' membership change cannot be persisted since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance command locally to persist the changes.
The instance 'root@localhost:<<<__mysql_sandbox_port2>>>' was successfully added to the cluster.

//@<OUT> Cluster: addInstance 3 with interaction, ok {VER(>=8.0.11)}
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Adding instance to the cluster ...

Validating instance at localhost:<<<__mysql_sandbox_port3>>>...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Instance configuration is suitable.
The instance 'root@localhost:<<<__mysql_sandbox_port3>>>' was successfully added to the cluster.

//@<OUT> Cluster: addInstance 3 with interaction, ok {VER(<8.0.11)}
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Adding instance to the cluster ...

Validating instance at localhost:<<<__mysql_sandbox_port3>>>...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Instance configuration is suitable.
WARNING: On instance 'localhost:<<<__mysql_sandbox_port3>>>' membership change cannot be persisted since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance command locally to persist the changes.
WARNING: On instance 'localhost:<<<__mysql_sandbox_port1>>>' membership change cannot be persisted since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance command locally to persist the changes.
WARNING: On instance 'localhost:<<<__mysql_sandbox_port2>>>' membership change cannot be persisted since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance command locally to persist the changes.
The instance 'root@localhost:<<<__mysql_sandbox_port3>>>' was successfully added to the cluster.

//@<OUT> Cluster: describe1
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "topology": [
            {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "label": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            },
            {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "label": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "role": "HA"
            },
            {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                "label": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                "role": "HA"
            }
        ],
        "topologyMode": "Single-Primary"
    }
}

//@<OUT> Cluster: status1
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "<<<__ssl_mode>>>",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "<<<localhost>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "<<<localhost>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "<<<localhost>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@ Cluster: removeInstance errors
||Cluster.removeInstance: Invalid number of arguments, expected 1 to 2 but got 0
||Cluster.removeInstance: Invalid number of arguments, expected 1 to 2 but got 3
||Cluster.removeInstance: Invalid connection options, expected either a URI or a Dictionary
||Cluster.removeInstance: Argument auth-method is expected to be a string
||Cluster.removeInstance: The instance 'localhost:33060' does not belong to the ReplicaSet: 'default'
||Cluster.removeInstance: The instance 'localhost:3306' does not belong to the ReplicaSet: 'default'

//@ Cluster: removeInstance
||

//@<OUT> Cluster: describe2
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "topology": [
            {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "label": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            },
            {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                "label": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                "role": "HA"
            }
        ],
        "topologyMode": "Single-Primary"
    }
}

//@<OUT> Cluster: status2
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "<<<__ssl_mode>>>",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "<<<localhost>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "<<<localhost>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@<OUT> Cluster: dissolve error: not empty
The cluster still has the following registered ReplicaSets:
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "topology": [
            {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "label": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            },
            {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                "label": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                "role": "HA"
            }
        ],
        "topologyMode": "Single-Primary"
    }
}
WARNING: You are about to dissolve the whole cluster and lose the high availability features provided by it. This operation cannot be reverted. All members will be removed from their ReplicaSet and replication will be stopped, internal recovery user accounts and the cluster metadata will be dropped. User data will be maintained intact in all instances.

Are you sure you want to dissolve the cluster? [y/N]:

//@<ERR> Cluster: dissolve error: not empty
Cluster.dissolve: Operation canceled by user. (RuntimeError)

//@ Cluster: dissolve errors
||Cluster.dissolve: Argument #1 is expected to be a map
||Cluster.dissolve: Invalid number of arguments, expected 0 to 1 but got 2
||Cluster.dissolve: Argument #1 is expected to be a map
||Cluster.dissolve: Invalid options: enforce
||Cluster.dissolve: Option 'force' Bool expected, but value is String

//@ Cluster: remove_instance 3
||

//@<OUT> Cluster: addInstance with interaction, ok 3 {VER(>=8.0.11)}
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Adding instance to the cluster ...

Validating instance at localhost:<<<__mysql_sandbox_port2>>>...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Instance configuration is suitable.
The instance 'root@localhost:<<<__mysql_sandbox_port2>>>' was successfully added to the cluster.

//@<OUT> Cluster: addInstance with interaction, ok 3 {VER(<8.0.11)}
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Adding instance to the cluster ...

Validating instance at localhost:<<<__mysql_sandbox_port2>>>...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Instance configuration is suitable.
WARNING: On instance 'localhost:<<<__mysql_sandbox_port2>>>' membership change cannot be persisted since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance command locally to persist the changes.
WARNING: On instance 'localhost:<<<__mysql_sandbox_port1>>>' membership change cannot be persisted since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance command locally to persist the changes.
The instance 'root@localhost:<<<__mysql_sandbox_port2>>>' was successfully added to the cluster.

//@<OUT> Cluster: addInstance with interaction, ok 4 {VER(>=8.0.11)}
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Adding instance to the cluster ...

Validating instance at localhost:<<<__mysql_sandbox_port3>>>...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Instance configuration is suitable.
The instance 'root@localhost:<<<__mysql_sandbox_port3>>>' was successfully added to the cluster.

//@<OUT> Cluster: addInstance with interaction, ok 4 {VER(<8.0.11)}
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Adding instance to the cluster ...

Validating instance at localhost:<<<__mysql_sandbox_port3>>>...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Instance configuration is suitable.
WARNING: On instance 'localhost:<<<__mysql_sandbox_port3>>>' membership change cannot be persisted since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance command locally to persist the changes.
WARNING: On instance 'localhost:<<<__mysql_sandbox_port1>>>' membership change cannot be persisted since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance command locally to persist the changes.
WARNING: On instance 'localhost:<<<__mysql_sandbox_port2>>>' membership change cannot be persisted since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance command locally to persist the changes.
The instance 'root@localhost:<<<__mysql_sandbox_port3>>>' was successfully added to the cluster.

//@<OUT> Cluster: status: success
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "<<<__ssl_mode>>>",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "<<<localhost>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "second_sandbox": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "third_sandbox": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@# Dba: kill instance 3
||

//@# Dba: start instance 3
||

//@: Cluster: rejoinInstance errors
||Cluster.rejoinInstance: Invalid number of arguments, expected 1 to 2 but got 0
||Cluster.rejoinInstance: Invalid number of arguments, expected 1 to 2 but got 3
||Invalid connection options, expected either a URI or a Dictionary
||Cluster.rejoinInstance: Invalid values in connection options: ipWhitelist, memberSslMode
||Cluster.rejoinInstance: Argument #2 is expected to be a map
||Cluster.rejoinInstance: The instance 'localhost:3306' does not belong to the ReplicaSet: 'default'
||Cluster.rejoinInstance: The instance 'localhost:3306' does not belong to the ReplicaSet: 'default'
||Invalid value for memberSslMode option. Supported values: AUTO,DISABLED,REQUIRED.
||Invalid value for memberSslMode option. Supported values: AUTO,DISABLED,REQUIRED.
||Invalid value for ipWhitelist: string value cannot be empty.

//@<OUT> Cluster: rejoinInstance with interaction, ok
Rejoining the instance to the InnoDB cluster. Depending on the original
problem that made the instance unavailable, the rejoin operation might not be
successful and further manual steps will be needed to fix the underlying
problem.

Please monitor the output of the rejoin operation and take necessary action if
the instance cannot rejoin.

Rejoining instance to the cluster ...

WARNING: Option 'memberSslMode' is deprecated for this operation and it will be removed in a future release. This option is not needed because the SSL mode is automatically obtained from the cluster. Please do not use it here.

The instance 'localhost:<<<__mysql_sandbox_port3>>>' was successfully rejoined on the cluster.

//@<OUT> Cluster: status for rejoin: success
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "<<<__ssl_mode>>>",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "<<<localhost>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "second_sandbox": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "third_sandbox": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@<OUT> Cluster: final dissolve
The cluster still has the following registered ReplicaSets:
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "topology": [
            {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "label": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            },
            {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "label": "second_sandbox",
                "role": "HA"
            },
            {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                "label": "third_sandbox",
                "role": "HA"
            }
        ],
        "topologyMode": "Single-Primary"
    }
}
WARNING: You are about to dissolve the whole cluster and lose the high availability features provided by it. This operation cannot be reverted. All members will be removed from their ReplicaSet and replication will be stopped, internal recovery user accounts and the cluster metadata will be dropped. User data will be maintained intact in all instances.

Are you sure you want to dissolve the cluster? [y/N]:
Instance '<<<localhost>>>:<<<__mysql_sandbox_port2>>>' is attempting to leave the cluster...
<<<(__version_num<80011)?"WARNING: On instance 'localhost:"+__mysql_sandbox_port2+"' configuration cannot be persisted since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please set the 'group_replication_start_on_boot' variable to 'OFF' in the server configuration file, otherwise it might rejoin the cluster upon restart.\n":""\>>>
Instance '<<<localhost>>>:<<<__mysql_sandbox_port3>>>' is attempting to leave the cluster...
<<<(__version_num<80011)?"WARNING: On instance 'localhost:"+__mysql_sandbox_port3+"' configuration cannot be persisted since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please set the 'group_replication_start_on_boot' variable to 'OFF' in the server configuration file, otherwise it might rejoin the cluster upon restart.\n":""\>>>
Instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>' is attempting to leave the cluster...
<<<(__version_num<80011)?"WARNING: On instance 'localhost:"+__mysql_sandbox_port1+"' configuration cannot be persisted since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please set the 'group_replication_start_on_boot' variable to 'OFF' in the server configuration file, otherwise it might rejoin the cluster upon restart.\n":""\>>>

The cluster was successfully dissolved.
Replication was disabled but user data was left intact.

//@ Cluster: no operations can be done on a dissolved cluster
||Cluster.name: Can't access object member 'name' on a dissolved cluster
||Cluster.addInstance: Can't call function 'addInstance' on a dissolved cluster
||Cluster.checkInstanceState: Can't call function 'checkInstanceState' on a dissolved cluster
||Cluster.describe: Can't call function 'describe' on a dissolved cluster
||Cluster.dissolve: Can't call function 'dissolve' on a dissolved cluster
||Cluster.forceQuorumUsingPartitionOf: Can't call function 'forceQuorumUsingPartitionOf' on a dissolved cluster
||Cluster.getName: Can't call function 'getName' on a dissolved cluster
||Cluster.rejoinInstance: Can't call function 'rejoinInstance' on a dissolved cluster
||Cluster.removeInstance: Can't call function 'removeInstance' on a dissolved cluster
||Cluster.rescan: Can't call function 'rescan' on a dissolved cluster
||Cluster.status: Can't call function 'status' on a dissolved cluster

//@ Cluster: disconnect() is ok on a dissolved cluster
||
