//@ Cluster: validating members
|Cluster Members: 13|
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

//@# Cluster: addInstance errors
||Invalid number of arguments in Cluster.addInstance, expected 1 to 2 but got 0
||Invalid number of arguments in Cluster.addInstance, expected 1 to 2 but got 4
||Cluster.addInstance: Invalid connection options, expected either a URI or a Dictionary
||Cluster.addInstance: Invalid URI: empty.
||Cluster.addInstance: Invalid and missing values in connection options (invalid: weird), (missing: host)
||Cluster.addInstance: Missing values in connection options: host
||Cluster.addInstance: Invalid values in connection options: ipWhitelist, memberSslMode
||Cluster.addInstance: Missing values in connection options: host
||Cluster.addInstance: Argument #2 is expected to be a map
||Invalid value for memberSslMode option. Supported values: AUTO,DISABLED,REQUIRED.
||Invalid value for memberSslMode option. Supported values: AUTO,DISABLED,REQUIRED.
||Invalid value for ipWhitelist, string value cannot be empty.
||Cluster.addInstance: The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is already part of this InnoDB cluster
||Cluster.addInstance: The label can not be empty.
||Cluster.addInstance: The label can only start with an alphanumeric or the '_' character.
||Cluster.addInstance: The label can only contain alphanumerics or the '_', '.', '-', ':' characters. Invalid character '#' found.
||Cluster.addInstance: The label can not be greater than 256 characters.

//@ Cluster: addInstance 2
||

//@ Cluster: addInstance 3
||

//@<OUT> Cluster: describe cluster with instance
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
                "label": "second",
                "role": "HA"
            },
            {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                "label": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                "role": "HA"
            }
        ]
    }
}

//@<OUT> Cluster: status cluster with instance
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
            "<<<localhost>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "second": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        }
    },
    "groupInformationSourceMember": "mysql://root@<<<localhost>>>:<<<__mysql_sandbox_port1>>>"
}

//@ Cluster: removeInstance errors
||Invalid number of arguments in Cluster.removeInstance, expected 1 to 2 but got 0
||Invalid number of arguments in Cluster.removeInstance, expected 1 to 2 but got 3
||Cluster.removeInstance: Invalid connection options, expected either a URI or a Dictionary
||Cluster.removeInstance: Invalid values in connection options: fakeOption
||Cluster.removeInstance: The instance 'localhost:3306' does not belong to the ReplicaSet: 'default'
||Cluster.removeInstance: The instance 'localhost:3306' does not belong to the ReplicaSet: 'default'
||Cluster.removeInstance: The instance 'localhost:3306' does not belong to the ReplicaSet: 'default'

//@ Cluster: removeInstance read only
||

//@<OUT> Cluster: describe removed read only member
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
        ]
    }
}

//@<OUT> Cluster: status removed read only member
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
    },
    "groupInformationSourceMember": "mysql://root@<<<localhost>>>:<<<__mysql_sandbox_port1>>>"
}

//@ Cluster: addInstance read only back
||

//@<OUT> Cluster: describe after adding read only instance back
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
            },
            {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "label": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "role": "HA"
            }
        ]
    }
}


//@<OUT> Cluster: status after adding read only instance back
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
    },
    "groupInformationSourceMember": "mysql://root@<<<localhost>>>:<<<__mysql_sandbox_port1>>>"
}

//@ Cluster: remove_instance master
||

//@ Cluster: no operations can be done on a disconnected cluster
||The cluster object is disconnected. Please call Cluster.getCluster to obtain a fresh cluster handle. (RuntimeError)
||The cluster object is disconnected. Please call Cluster.getCluster to obtain a fresh cluster handle. (RuntimeError)
||The cluster object is disconnected. Please call Cluster.getCluster to obtain a fresh cluster handle. (RuntimeError)
||The cluster object is disconnected. Please call Cluster.getCluster to obtain a fresh cluster handle. (RuntimeError)
||The cluster object is disconnected. Please call Cluster.getCluster to obtain a fresh cluster handle. (RuntimeError)
||The cluster object is disconnected. Please call Cluster.getCluster to obtain a fresh cluster handle. (RuntimeError)
||The cluster object is disconnected. Please call Cluster.getCluster to obtain a fresh cluster handle. (RuntimeError)
||The cluster object is disconnected. Please call Cluster.getCluster to obtain a fresh cluster handle. (RuntimeError)
||The cluster object is disconnected. Please call Cluster.getCluster to obtain a fresh cluster handle. (RuntimeError)

//@ Connecting to new master
||

//@<OUT> Cluster: describe on new master
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "topology": [
            {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "label": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "role": "HA"
            },
            {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                "label": "third_sandbox",
                "role": "HA"
            }
        ]
    }
}

//@<OUT> Cluster: status on new master
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
        "ssl": "<<<__ssl_mode>>>",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "<<<localhost>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/W",
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
    },
    "groupInformationSourceMember": "mysql://root@<<<localhost>>>:<<<__mysql_sandbox_port2>>>"
}

//@ Cluster: addInstance adding old master as read only
||

//@<OUT> Cluster: describe on new master with slave
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "topology": [
            {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "label": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "role": "HA"
            },
            {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                "label": "third_sandbox",
                "role": "HA"
            },
            {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "label": "first_sandbox",
                "role": "HA"
            }
        ]
    }
}

//@<OUT> Cluster: status on new master with slave
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
        "ssl": "<<<__ssl_mode>>>",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "first_sandbox": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "<<<localhost>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/W",
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
    },
    "groupInformationSourceMember": "mysql://root@<<<localhost>>>:<<<__mysql_sandbox_port2>>>"
}

//@# Dba: kill instance 3
||

//@# Dba: start instance 3
||

//@ Cluster: rejoinInstance errors
||Invalid number of arguments in Cluster.rejoinInstance, expected 1 to 2 but got 0
||Invalid number of arguments in Cluster.rejoinInstance, expected 1 to 2 but got 3
||Cluster.rejoinInstance: Invalid connection options, expected either a URI or a Dictionary
||Cluster.rejoinInstance: Invalid values in connection options: ipWhitelist, memberSslMode
||Cluster.rejoinInstance: The instance 'localhost:3306' does not belong to the ReplicaSet: 'default'
||Cluster.rejoinInstance: The instance 'localhost:3306' does not belong to the ReplicaSet: 'default'
||Cluster.rejoinInstance: Argument #2 is expected to be a map
||Invalid value for memberSslMode option. Supported values: AUTO,DISABLED,REQUIRED.
||Invalid value for memberSslMode option. Supported values: AUTO,DISABLED,REQUIRED.
||Invalid value for ipWhitelist, string value cannot be empty.

//@#: Dba: rejoin instance 3 ok {VER(<8.0.11)}
||

//@#: Dba: Wait instance 3 ONLINE {VER(>=8.0.11)}
||

//@<OUT> Cluster: status for rejoin: success
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
        "ssl": "<<<__ssl_mode>>>",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "first_sandbox": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "<<<localhost>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/W",
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
    },
    "groupInformationSourceMember": "mysql://root@<<<localhost>>>:<<<__mysql_sandbox_port2>>>"
}

//@ Cluster: dissolve errors
||Cluster.dissolve: Argument #1 is expected to be a map
||Invalid number of arguments in Cluster.dissolve, expected 0 to 1 but got 2
||Cluster.dissolve: Argument #1 is expected to be a map
||Cluster.dissolve: Invalid values in dissolve options: foobar
||Cluster.dissolve: Argument 'force' is expected to be a bool

//@ Cluster: final dissolve
||

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

//@ Cluster: disconnect should work, tho
||
