//@<OUT> Cluster: getCluster with interaction

//@ Cluster: validating members
|Cluster Members: 13|
|name: OK|
|getName: OK|
|adminType: OK|
|getAdminType: OK|
|addInstance: OK|
|removeInstance: OK|
|rejoinInstance: OK|
|checkInstanceState: OK|
|describe: OK|
|status: OK|
|help: OK|
|dissolve: OK|
|rescan: OK|

//@ Cluster: addInstance errors
||Invalid number of arguments in Cluster.addInstance, expected 1 to 2 but got 0
||Invalid number of arguments in Cluster.addInstance, expected 1 to 2 but got 4
||Invalid connection options, expected either a URI or a Dictionary
||Cluster.addInstance: Connection definition is empty
||Cluster.addInstance: Invalid values in instance definition: authMethod, schema
||Cluster.addInstance: Missing values in instance definition: host

//@ Cluster: addInstance with interaction, error
||Cluster.addInstance: The instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>' already belongs to the ReplicaSet: 'default'.

//@<OUT> Cluster: addInstance with interaction, ok
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Please provide a password for 'root@localhost:<<<__mysql_sandbox_port2>>>': Adding instance to the cluster ...

The instance 'root@localhost:<<<__mysql_sandbox_port2>>>' was successfully added to the cluster.

//@<OUT> Cluster: describe1
{
    "adminType": "local",
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
            }
        ],
        "name": "default"
    }
}

//@<OUT> Cluster: status1
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "status": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "<<<localhost>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "leaves": {
                    "<<<localhost>>>:<<<__mysql_sandbox_port2>>>": {
                        "address": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                        "leaves": {

                        },
                        "mode": "R/O",
                        "role": "HA",
                        "status": "{{ONLINE|RECOVERING}}"
                    }
                },
                "mode": "R/W",
                "role": "HA",
                "status": "ONLINE"
            }
        }
    }
}


//@ Cluster: removeInstance errors
||Invalid number of arguments in Cluster.removeInstance, expected 1 but got 0
||Invalid number of arguments in Cluster.removeInstance, expected 1 but got 2
||Invalid connection options, expected either a URI or a Dictionary
||Cluster.removeInstance: The instance 'localhost:33060' does not belong to the ReplicaSet: 'default'
||Cluster.removeInstance: The instance 'somehost:3306' does not belong to the ReplicaSet: 'default'

//@ Cluster: removeInstance
||

//@<OUT> Cluster: describe2
{
    "adminType": "local",
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "instances": [
            {
                "host": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "name": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
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
        "status": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "<<<localhost>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "leaves": {

                },
                "mode": "R/W",
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
    "adminType": "local",
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "instances": [
            {
                "host": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "name": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
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
||Cluster.dissolve: Invalid values in dissolve options: foobar
||Cluster.dissolve: Argument 'force' is expected to be a bool

//@ Cluster: remove_instance last
||

//@<OUT> Cluster: describe3
{
    "adminType": "local",
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "instances": [
        ],
        "name": "default"
    }
}

//@<OUT> Cluster: status3
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "status": "Cluster is NOT tolerant to any failures.",
        "topology": {

        }
    }
}

//@<OUT> Cluster: addInstance with interaction, ok 2
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Please provide a password for 'root@localhost:<<<__mysql_sandbox_port1>>>': Adding instance to the cluster ...

The instance 'root@localhost:<<<__mysql_sandbox_port1>>>' was successfully added to the cluster.

//@<OUT> Cluster: addInstance with interaction, ok 3
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Please provide a password for 'root@localhost:<<<__mysql_sandbox_port2>>>': Adding instance to the cluster ...

The instance 'root@localhost:<<<__mysql_sandbox_port2>>>' was successfully added to the cluster.

//@<OUT> Cluster: dissolve
The cluster was successfully dissolved.
Replication was disabled but user data was left intact.

//@ Cluster: describe: dissolved cluster
||The cluster 'devCluster' no longer exists.

//@ Cluster: status: dissolved cluster
||The cluster 'devCluster' no longer exists.
