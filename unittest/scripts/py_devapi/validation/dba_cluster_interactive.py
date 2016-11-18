#@<OUT> Cluster: get_cluster with interaction

#@ Cluster: validating members
|Cluster Members: 13|
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

#@ Cluster: add_instance errors
||Invalid number of arguments in Cluster.add_instance, expected 1 to 2 but got 0
||Invalid number of arguments in Cluster.add_instance, expected 1 to 2 but got 4
||Invalid connection options, expected either a URI or a Dictionary
||Cluster.add_instance: Connection definition is empty
||Cluster.add_instance: Invalid values in instance definition: authMethod, schema
||Cluster.add_instance: Missing values in instance definition: host

#@ Cluster: add_instance with interaction, error
||Cluster.add_instance: The instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>' already belongs to the ReplicaSet: 'default'.

#@<OUT> Cluster: add_instance with interaction, ok
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Please provide the password for 'root@localhost:<<<__mysql_sandbox_port2>>>': Adding instance to the cluster ...

The instance 'root@localhost:<<<__mysql_sandbox_port2>>>' was successfully added to the cluster.

#@<OUT> Cluster: add_instance 3 with interaction, ok
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Please provide the password for 'root@localhost:<<<__mysql_sandbox_port3>>>': Adding instance to the cluster ...

The instance 'root@localhost:<<<__mysql_sandbox_port3>>>' was successfully added to the cluster.

#@<OUT> Cluster: describe1
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

#@<OUT> Cluster: status1
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "status": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "<<<localhost>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "leaves": {
                    "<<<localhost>>>:<<<__mysql_sandbox_port2>>>": {
                        "address": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                        "leaves": {},
                        "mode": "R/O",
                        "role": "HA",
                        "status": "{{ONLINE|RECOVERING}}"
                    },
                    "<<<localhost>>>:<<<__mysql_sandbox_port3>>>": {
                        "address": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                        "leaves": {},
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


#@ Cluster: remove_instance errors
||Invalid number of arguments in Cluster.remove_instance, expected 1 but got 0
||Invalid number of arguments in Cluster.remove_instance, expected 1 but got 2
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

#@<OUT> Cluster: status2
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "status": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "<<<localhost>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "leaves": {
                    "<<<localhost>>>:<<<__mysql_sandbox_port3>>>": {
                        "address": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                        "leaves": {},
                        "mode": "R/O",
                        "role": "HA",
                        "status": "ONLINE"
                    }
                },
                "mode": "R/W",
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

#@ Cluster: dissolve errors
||Cluster.dissolve: Argument #1 is expected to be a map
||Invalid number of arguments in Cluster.dissolve, expected 0 to 1 but got 2
||Cluster.dissolve: Argument #1 is expected to be a map
||Cluster.dissolve: Invalid values in dissolve options: enforce
||Cluster.dissolve: Argument 'force' is expected to be a bool

#@ Cluster: remove_instance 3
||

#@ Cluster: remove_instance last
||

#@<OUT> Cluster: describe3
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "instances": [],
        "name": "default"
    }
}

#@<OUT> Cluster: status3
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
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

#@<OUT> Cluster: add_instance with interaction, ok 4
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Please provide the password for 'root@localhost:<<<__mysql_sandbox_port3>>>': Adding instance to the cluster ...

The instance 'root@localhost:<<<__mysql_sandbox_port3>>>' was successfully added to the cluster.

#@<OUT> Cluster: status: success
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "status": "Cluster tolerant to up to ONE failure.",
        "topology": {
            "<<<localhost>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "leaves": {
                    "<<<localhost>>>:<<<__mysql_sandbox_port2>>>": {
                        "address": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                        "leaves": {},
                        "mode": "R/O",
                        "role": "HA",
                        "status": "ONLINE"
                    },
                    "<<<localhost>>>:<<<__mysql_sandbox_port3>>>": {
                        "address": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                        "leaves": {},
                        "mode": "R/O",
                        "role": "HA",
                        "status": "ONLINE"
                    }
                },
                "mode": "R/W",
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
||Cluster.rejoin_instance: Invalid values in instance definition: authMethod, schema
||Cluster.rejoin_instance: The instance 'somehost:3306' does not belong to the ReplicaSet: 'default'

#@<OUT> Cluster: rejoin_instance with interaction, ok
The instance will try rejoining the InnoDB cluster. Depending on the original
problem that made the instance unavailable, the rejoin operation might not be
successful and further manual steps will be needed to fix the underlying
problem.

Please monitor the output of the rejoin operation and take necessary action if
the instance cannot rejoin.

Please provide the password for 'root@localhost:<<<__mysql_sandbox_port3>>>': Rejoining instance to the cluster ...

The instance 'root@localhost:<<<__mysql_sandbox_port3>>>' was successfully rejoined on the cluster.

#@<OUT> Cluster: status for rejoin: success
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "status": "Cluster tolerant to up to ONE failure.",
        "topology": {
            "<<<localhost>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "leaves": {
                    "<<<localhost>>>:<<<__mysql_sandbox_port2>>>": {
                        "address": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                        "leaves": {},
                        "mode": "R/O",
                        "role": "HA",
                        "status": "ONLINE"
                    },
                    "<<<localhost>>>:<<<__mysql_sandbox_port3>>>": {
                        "address": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                        "leaves": {},
                        "mode": "R/O",
                        "role": "HA",
                        "status": "ONLINE"
                    }
                },
                "mode": "R/W",
                "role": "HA",
                "status": "ONLINE"
            }
        }
    }
}
