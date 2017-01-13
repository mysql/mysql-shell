#@ Cluster: validating members
|Cluster Members: 14|
|name: OK|
|get_name: OK|
|admin_type: OK|
|get_admin_type: OK|
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

#@# Cluster: add_instance errors
||Invalid number of arguments in Cluster.add_instance, expected 1 to 2 but got 0
||Invalid number of arguments in Cluster.add_instance, expected 1 to 2 but got 4
||Cluster.add_instance: Invalid connection options, expected either a URI or a Dictionary
||Cluster.add_instance: Connection definition is empty
||Cluster.add_instance: Invalid and missing values in instance definition (invalid: weird), (missing: host)
||Cluster.add_instance: Invalid connection options, expected either a URI or a Dictionary
||Cluster.add_instance: Invalid values in instance definition: authMethod, schema
||Cluster.add_instance: Missing values in instance definition: host
||Invalid value for memberSslMode option. Supported values: AUTO,DISABLED,REQUIRED.
||Invalid value for memberSslMode option. Supported values: AUTO,DISABLED,REQUIRED.
||Cluster.add_instance: The instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>' already belongs to the ReplicaSet: 'default'
||Invalid value for ipWhitelist, string value cannot be empty.

#@ Cluster: add_instance 2
||

#@ Cluster: add_instance 3
||

#@<OUT> Cluster: describe cluster with instance
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "instances": [
            {
                "host": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "name": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            },
            {
                "host": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "name": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "role": "HA"
            },
            {
                "host": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                "name": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                "role": "HA"
            }
        ],
        "name": "default"
    }
}

#@<OUT> Cluster: status cluster with instance
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
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
||Invalid number of arguments in Cluster.remove_instance, expected 1 but got 0
||Invalid number of arguments in Cluster.remove_instance, expected 1 but got 2
||Cluster.remove_instance: Invalid connection options, expected either a URI or a Dictionary
||Cluster.remove_instance: Invalid values in instance definition: authMethod, schema, user
||Cluster.remove_instance: The instance 'somehost:3306' does not belong to the ReplicaSet: 'default'

#@ Cluster: remove_instance read only
||

#@<OUT> Cluster: describe removed read only member
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "instances": [
            {
                "host": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "name": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            },
            {
                "host": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                "name": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                "role": "HA"
            }
        ],
        "name": "default"
    }
}

#@<OUT> Cluster: status removed read only member
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
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

#@ Cluster: addInstance read only back
||

#@<OUT> Cluster: describe after adding read only instance back
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "instances": [
            {
                "host": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "name": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            },
            {
                "host": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                "name": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                "role": "HA"
            },
            {
                "host": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "name": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "role": "HA"
            }
        ],
        "name": "default"
    }
}


#@<OUT> Cluster: status after adding read only instance back
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
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

#@ Cluster: remove_instance master
||

#@ Connecting to new master
||

#@<OUT> Cluster: describe on new master
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "instances": [
            {
                "host": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "name": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "role": "HA"
            },
            {
                "host": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                "name": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                "role": "HA"
            }
        ],
        "name": "default"
    }
}

#@<OUT> Cluster: status on new master
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
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

#@ Cluster: addInstance adding old master as read only
||

#@<OUT> Cluster: describe on new master with slave
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "instances": [
            {
                "host": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "name": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "role": "HA"
            },
            {
                "host": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                "name": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                "role": "HA"
            },
            {
                "host": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "name": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            }
        ],
        "name": "default"
    }
}

#@<OUT> Cluster: status on new master with slave
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "<<<localhost>>>:<<<__mysql_sandbox_port1>>>": {
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

#@# Dba: kill instance 3
||

#@# Dba: start instance 3
||

#@# Cluster: rejoin_instance errors
||Invalid number of arguments in Cluster.rejoin_instance, expected 1 to 2 but got 0
||Invalid number of arguments in Cluster.rejoin_instance, expected 1 to 2 but got 3
||Cluster.rejoin_instance: Invalid connection options, expected either a URI or a Dictionary
||Cluster.rejoin_instance: Invalid values in instance definition: authMethod, schema
||Cluster.rejoin_instance: The instance 'somehost:3306' does not belong to the ReplicaSet: 'default'
||Invalid value for memberSslMode option. Supported values: AUTO,DISABLED,REQUIRED.
||Invalid value for memberSslMode option. Supported values: AUTO,DISABLED,REQUIRED.
||Invalid value for ipWhitelist, string value cannot be empty.

#@#: Dba: rejoin instance 3 ok
||

#@<OUT> Cluster: status for rejoin: success
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "<<<localhost>>>:<<<__mysql_sandbox_port1>>>": {
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

#@ Cluster: dissolve errors
||Cluster.dissolve: Argument #1 is expected to be a map
||Invalid number of arguments in Cluster.dissolve, expected 0 to 1 but got 2
||Cluster.dissolve: Argument #1 is expected to be a map
||Cluster.dissolve: Invalid values in dissolve options: foobar
||Cluster.dissolve: Argument 'force' is expected to be a bool

#The cluster 'devCluster' no longer exists.

#The cluster 'devCluster' no longer exists.
