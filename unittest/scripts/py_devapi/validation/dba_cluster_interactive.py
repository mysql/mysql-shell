#@<OUT> Cluster: get_cluster with interaction

#@ Cluster: validating members
|Cluster Members: 12|
|name: OK|
|get_name: OK|
|add_instance: OK|
|remove_instance: OK|
|rejoin_instance: OK|
|check_instance_state: OK|
|describe: OK|
|status: OK|
|help: OK|
|dissolve: OK|
|rescan: OK|
|force_quorum_using_partition_of: OK|

#@ Cluster: add_instance errors
||Invalid number of arguments in Cluster.add_instance, expected 1 to 2 but got 0
||Invalid number of arguments in Cluster.add_instance, expected 1 to 2 but got 4
||Invalid connection options, expected either a URI or a Dictionary
||Cluster.add_instance: Connection definition is empty
||Cluster.add_instance: Invalid values in instance definition: ipWhitelist, memberSslMode
||Cluster.add_instance: Missing values in instance definition: host
||Cluster.add_instance: Argument #2 is expected to be a map
||Invalid value for memberSslMode option. Supported values: AUTO,DISABLED,REQUIRED.
||Invalid value for memberSslMode option. Supported values: AUTO,DISABLED,REQUIRED.
||Invalid value for ipWhitelist, string value cannot be empty.
||Cluster.add_instance: The label can not be empty.
||Cluster.add_instance: The label can only start with an alphanumeric or the '_' character.
||Cluster.add_instance: The label can only contain alphanumerics or the '_', '.', '-', ':' characters. Invalid character '#' found.
||Cluster.add_instance: The label can not be greater than 256 characters.

#@ Cluster: add_instance with interaction, error
||Cluster.add_instance: The instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>' is already part of this InnoDB cluster

#@<OUT> Cluster: add_instance with interaction, ok
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Adding instance to the cluster ...

The instance 'root@localhost:<<<__mysql_sandbox_port2>>>' was successfully added to the cluster.

#@<OUT> Cluster: add_instance 3 with interaction, ok
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Adding instance to the cluster ...

The instance 'root@localhost:<<<__mysql_sandbox_port3>>>' was successfully added to the cluster.

#@<OUT> Cluster: describe1
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

#@<OUT> Cluster: status1
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


#@ Cluster: remove_instance errors
||Invalid number of arguments in Cluster.remove_instance, expected 1 to 2 but got 0
||Invalid number of arguments in Cluster.remove_instance, expected 1 to 2 but got 3
||Invalid connection options, expected either a URI or a Dictionary
||Cluster.remove_instance: The instance 'localhost:33060' does not belong to the ReplicaSet: 'default'
||Cluster.remove_instance: The instance 'somehost:3306' does not belong to the ReplicaSet: 'default'

#@ Cluster: remove_instance
||

#@<OUT> Cluster: describe2
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

#@<OUT> Cluster: status2
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

#@<OUT> Cluster: dissolve error: not empty
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

#@ Cluster: dissolve errors
||Cluster.dissolve: Argument #1 is expected to be a map
||Invalid number of arguments in Cluster.dissolve, expected 0 to 1 but got 2
||Cluster.dissolve: Argument #1 is expected to be a map
||Cluster.dissolve: Invalid values in dissolve options: enforce
||Cluster.dissolve: Argument 'force' is expected to be a bool

#@ Cluster: remove_instance 3
||

#@<OUT> Cluster: add_instance with interaction, ok 3
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Adding instance to the cluster ...

The instance 'root@localhost:<<<__mysql_sandbox_port2>>>' was successfully added to the cluster.

#@<OUT> Cluster: add_instance with interaction, ok 4
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Adding instance to the cluster ...

The instance 'root@localhost:<<<__mysql_sandbox_port3>>>' was successfully added to the cluster.

#@<OUT> Cluster: status: success
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

#@# Dba: kill instance 3
||

#@# Dba: start instance 3
||

#@: Cluster: rejoin_instance errors
||Invalid number of arguments in Cluster.rejoin_instance, expected 1 to 2 but got 0
||Invalid number of arguments in Cluster.rejoin_instance, expected 1 to 2 but got 3
||Invalid connection options, expected either a URI or a Dictionary
||Cluster.rejoin_instance: The instance 'localhost:3306' does not belong to the ReplicaSet: 'default'
||Cluster.rejoin_instance: Invalid values in instance definition: ipWhitelist, memberSslMode
||Cluster.rejoin_instance: The instance 'somehost:3306' does not belong to the ReplicaSet: 'default'
||Cluster.rejoin_instance: Argument #2 is expected to be a map
||Invalid value for memberSslMode option. Supported values: AUTO,DISABLED,REQUIRED.
||Invalid value for memberSslMode option. Supported values: AUTO,DISABLED,REQUIRED.
||Invalid value for ipWhitelist, string value cannot be empty.

#@<OUT> Cluster: rejoin_instance with interaction, ok
Rejoining the instance to the InnoDB cluster. Depending on the original
problem that made the instance unavailable, the rejoin operation might not be
successful and further manual steps will be needed to fix the underlying
problem.

Please monitor the output of the rejoin operation and take necessary action if
the instance cannot rejoin.

Rejoining instance to the cluster ...

The instance 'root@localhost:<<<__mysql_sandbox_port3>>>' was successfully rejoined on the cluster.

The instance 'localhost:<<<__mysql_sandbox_port3>>>' was successfully added to the MySQL Cluster.

#@<OUT> Cluster: status for rejoin: success
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

#@ Cluster: final dissolve
|The cluster was successfully dissolved.|
|Replication was disabled but user data was left intact.|

#@ Cluster: no operations can be done on a dissolved cluster errors
||Cluster.name: Can't access object member 'name' on a dissolved cluster
||Cluster.add_instance: Can't call function 'add_instance' on a dissolved cluster
||Cluster.check_instance_state: Can't call function 'check_instance_state' on a dissolved cluster
||Cluster.describe: Can't call function 'describe' on a dissolved cluster
||Cluster.dissolve: Can't call function 'dissolve' on a dissolved cluster
||Cluster.force_quorum_using_partition_of: Can't call function 'force_quorum_using_partition_of' on a dissolved cluster
||Cluster.name: Can't access object member 'name' on a dissolved cluster
||Cluster.rejoin_instance: Can't call function 'rejoin_instance' on a dissolved cluster
||Cluster.remove_instance: Can't call function 'remove_instance' on a dissolved cluster
||Cluster.rescan: Can't call function 'rescan' on a dissolved cluster
||Cluster.status: Can't call function 'status' on a dissolved cluster
