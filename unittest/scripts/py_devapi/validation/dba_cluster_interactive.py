#@<OUT> Cluster: get_cluster with interaction
When the InnoDB cluster was setup, a MASTER key was defined in order to enable
performing administrative tasks on the cluster.

Please specify the administrative MASTER key for the cluster 'devCluster':

#@ Cluster: validating members
|Cluster Members: 11|
|name: OK|
|get_name: OK|
|admin_type: OK|
|get_admin_type: OK|
|add_instance: OK|
|remove_instance: OK|
|rejoin_instance: OK|
|describe: OK|
|status: OK|
|help: OK|
|dissolve: OK|

#@# Cluster: add_instance errors
||Invalid number of arguments in Cluster.add_instance, expected 1 to 2 but got 0
||Invalid number of arguments in Cluster.add_instance, expected 1 to 2 but got 4
||Invalid connection options, expected either a URI, a Dictionary or an Instance object

#@# Cluster: add_instance errors: missing host interactive, cancel
||

#@# Cluster: add_instance errors 2
||Invalid connection options, expected either a URI, a Dictionary or an Instance object

#@# Cluster: add_instance errors: invalid attributes, cancel
||

#@# Cluster: add_instance errors: missing host interactive, cancel 2
||

#@# Cluster: add_instance with interaction, error
||Cluster.addInstance: The instance 'localhost:<<<__mysql_sandbox_port1>>>' already belongs to the ReplicaSet: 'default'.

#@<OUT> Cluster: add_instance with interaction, ok
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Please provide the password for 'root@localhost:<<<__mysql_sandbox_port2>>>': Adding instance to the cluster ...

The instance 'root@localhost:<<<__mysql_sandbox_port2>>>' was successfully added to the cluster.

#@<OUT> Cluster: describe1
{
    "clusterName": "devCluster",
    "adminType": "local",
    "defaultReplicaSet": {
        "name": "default",
        "instances": [
            {
                "name": "localhost:<<<__mysql_sandbox_port1>>>",
                "host": "localhost:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            },
            {
                "name": "localhost:<<<__mysql_sandbox_port2>>>",
                "host": "localhost:<<<__mysql_sandbox_port2>>>",
                "role": "HA"
            }
        ]
    }
}

#@<OUT> Cluster: status1
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "status": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "localhost:<<<__mysql_sandbox_port1>>>": {
                "address": "localhost:<<<__mysql_sandbox_port1>>>",
                "status": "ONLINE",
                "role": "HA",
                "mode": "R/W",
                "leaves": {
                    "localhost:<<<__mysql_sandbox_port2>>>": {
                        "address": "localhost:<<<__mysql_sandbox_port2>>>",
                        "status": "RECOVERING",
                        "role": "HA",
                        "mode": "R/O",
                        "leaves": {}
                    }
                }
            }
        }
    }
}


#@# Cluster: remove_instance errors
||Invalid number of arguments in Cluster.remove_instance, expected 1 but got 0
||Invalid number of arguments in Cluster.remove_instance, expected 1 but got 2
||Cluster.removeInstance: Invalid connection options, expected either a URI, a Dictionary or an Instance object
||Cluster.removeInstance: The instance 'localhost:0' does not belong to the ReplicaSet: 'default'
||Cluster.removeInstance: The instance 'somehost:3306' does not belong to the ReplicaSet: 'default'

#@ Cluster: remove_instance
||

#@<OUT> Cluster: describe2
{
    "clusterName": "devCluster",
    "adminType": "local",
    "defaultReplicaSet": {
        "name": "default",
        "instances": [
            {
                "name": "localhost:<<<__mysql_sandbox_port1>>>",
                "host": "localhost:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            }
        ]
    }
}

#@<OUT> Cluster: status2
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "status": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "localhost:<<<__mysql_sandbox_port1>>>": {
                "address": "localhost:<<<__mysql_sandbox_port1>>>",
                "status": "ONLINE",
                "role": "HA",
                "mode": "R/W",
                "leaves": {}
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
    "adminType": "local",
    "defaultReplicaSet": {
        "name": "default",
        "instances": [
            {
                "name": "localhost:<<<__mysql_sandbox_port1>>>",
                "host": "localhost:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            }
        ]
    }
}

#@ Cluster: dissolve errors
||Cluster.dissolve: Argument #1 is expected to be a map
||Invalid number of arguments in Cluster.dissolve, expected 0 to 1 but got 2
||Cluster.dissolve: Argument #1 is expected to be a map
||Cluster.dissolve: The options contain the following invalid attributes: enforce
||Cluster.dissolve: Invalid data type for 'force' field, should be a boolean

#@ Cluster: remove_instance last
||

#@<OUT> Cluster: describe3
{
    "clusterName": "devCluster",
    "adminType": "local",
    "defaultReplicaSet": {
        "name": "default",
        "instances": []
    }
}

#@<OUT> Cluster: status3
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "status": "Cluster is NOT tolerant to any failures.",
        "topology": {}
    }
}

#@<OUT> Cluster: add_instance with interaction, ok 2
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Please provide the password for 'root@localhost:<<<__mysql_sandbox_port1>>>': Adding instance to the cluster ...

The instance 'root@localhost:<<<__mysql_sandbox_port1>>>' was successfully added to the cluster.

#@<OUT> Cluster: add_instance with interaction, ok 3
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Please provide the password for 'root@localhost:<<<__mysql_sandbox_port2>>>': Adding instance to the cluster ...

The instance 'root@localhost:<<<__mysql_sandbox_port2>>>' was successfully added to the cluster.

#@<OUT> Cluster: dissolve
The cluster was successfully dissolved.
Replication was disabled but user data was left intact.

#@ Cluster: describe: dissolved cluster
||The cluster 'devCluster' no longer exists.

#@ Cluster: status: dissolved cluster
||The cluster 'devCluster' no longer exists.
