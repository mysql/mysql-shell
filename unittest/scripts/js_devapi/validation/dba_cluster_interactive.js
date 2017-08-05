//@<OUT> Cluster: getCluster with interaction

//@ Cluster: validating members
|Cluster Members: 12|
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
|rescan: OK|
|forceQuorumUsingPartitionOf: OK|

//@ Cluster: addInstance errors
||Invalid number of arguments in Cluster.addInstance, expected 1 to 2 but got 0
||Invalid number of arguments in Cluster.addInstance, expected 1 to 2 but got 4
||Invalid connection options, expected either a URI or a Dictionary
||Cluster.addInstance: Invalid URI: empty.
||Cluster.addInstance: Invalid values in connection options: ipWhitelist, memberSslMode
||Cluster.addInstance: Missing values in connection options: host
||Cluster.addInstance: Argument #2 is expected to be a map
||Invalid value for memberSslMode option. Supported values: AUTO,DISABLED,REQUIRED.
||Invalid value for memberSslMode option. Supported values: AUTO,DISABLED,REQUIRED.
||Invalid value for ipWhitelist, string value cannot be empty.

//@ Cluster: addInstance with interaction, error
||Cluster.addInstance: The instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>' is already part of this InnoDB cluster

//@<OUT> Cluster: addInstance with interaction, ok
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Adding instance to the cluster ...

The instance 'root@localhost:<<<__mysql_sandbox_port2>>>' was successfully added to the cluster.

//@<OUT> Cluster: addInstance 3 with interaction, ok
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Adding instance to the cluster ...

The instance 'root@localhost:<<<__mysql_sandbox_port3>>>' was successfully added to the cluster.

//@<OUT> Cluster: describe1
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "instances": [
            {
                "host": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "label": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            },
            {
                "host": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "label": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "role": "HA"
            },
            {
                "host": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                "label": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                "role": "HA"
            }
        ],
        "name": "default"
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
        }
    }
}

//@ Cluster: removeInstance errors
||Invalid number of arguments in Cluster.removeInstance, expected 1 to 2 but got 0
||Invalid number of arguments in Cluster.removeInstance, expected 1 to 2 but got 3
||Invalid connection options, expected either a URI or a Dictionary
||Cluster.removeInstance: The instance 'localhost:33060' does not belong to the ReplicaSet: 'default'
||Cluster.removeInstance: The instance 'somehost:3306' does not belong to the ReplicaSet: 'default'

//@ Cluster: removeInstance
||

//@<OUT> Cluster: describe2
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "instances": [
            {
                "host": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "label": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            },
            {
                "host": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                "label": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                "role": "HA"
            }
        ],
        "name": "default"
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
        }
    }
}

//@<OUT> Cluster: dissolve error: not empty
The cluster still has active ReplicaSets.
Please use cluster.dissolve({force: true}) to deactivate replication
and unregister the ReplicaSets from the cluster.

The following replicasets are currently registered:
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "instances": [
            {
                "host": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "label": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            },
            {
                "host": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                "label": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                "role": "HA"
            }
        ],
        "name": "default"
    }
}

//@ Cluster: dissolve errors
||Cluster.dissolve: Argument #1 is expected to be a map
||Invalid number of arguments in Cluster.dissolve, expected 0 to 1 but got 2
||Cluster.dissolve: Argument #1 is expected to be a map
||Cluster.dissolve: Invalid values in dissolve options: enforce
||Cluster.dissolve: Argument 'force' is expected to be a bool

//@ Cluster: remove_instance 3
||

//@<OUT> Cluster: addInstance with interaction, ok 3
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Adding instance to the cluster ...

The instance 'root@localhost:<<<__mysql_sandbox_port2>>>' was successfully added to the cluster.

//@<OUT> Cluster: addInstance with interaction, ok 4
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Adding instance to the cluster ...

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
        }
    }
}

//@# Dba: kill instance 3
||

//@# Dba: start instance 3
||

//@: Cluster: rejoinInstance errors
||Invalid number of arguments in Cluster.rejoinInstance, expected 1 to 2 but got 0
||Invalid number of arguments in Cluster.rejoinInstance, expected 1 to 2 but got 3
||Invalid connection options, expected either a URI or a Dictionary
||Cluster.rejoinInstance: The instance 'localhost:3306' does not belong to the ReplicaSet: 'default'
||Cluster.rejoinInstance: Invalid values in connection options: ipWhitelist, memberSslMode
||Cluster.rejoinInstance: The instance 'somehost:3306' does not belong to the ReplicaSet: 'default'
||Cluster.rejoinInstance: Argument #2 is expected to be a map
||Invalid value for memberSslMode option. Supported values: AUTO,DISABLED,REQUIRED.
||Invalid value for memberSslMode option. Supported values: AUTO,DISABLED,REQUIRED.
||Invalid value for ipWhitelist, string value cannot be empty.

//@<OUT> Cluster: rejoinInstance with interaction, ok
Rejoining the instance to the InnoDB cluster. Depending on the original
problem that made the instance unavailable, the rejoin operation might not be
successful and further manual steps will be needed to fix the underlying
problem.

Please monitor the output of the rejoin operation and take necessary action if
the instance cannot rejoin.

Rejoining instance to the cluster ...

The instance 'root@localhost:<<<__mysql_sandbox_port3>>>' was successfully rejoined on the cluster.

The instance 'localhost:<<<__mysql_sandbox_port3>>>' was successfully added to the MySQL Cluster.

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
        }
    }
}

//@ Cluster: final dissolve
|The cluster was successfully dissolved.|
|Replication was disabled but user data was left intact.|

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
