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

//@# Cluster: addInstance errors
||Invalid number of arguments in Cluster.addInstance, expected 1 to 2 but got 0
||Invalid number of arguments in Cluster.addInstance, expected 1 to 2 but got 4
||Cluster.addInstance: Invalid connection options, expected either a URI or a Dictionary
||Cluster.addInstance: Connection definition is empty
||Cluster.addInstance: Invalid and missing values in instance definition (invalid: weird), (missing: host)
||Cluster.addInstance: Invalid connection options, expected either a URI or a Dictionary
||Cluster.addInstance: Invalid values in instance definition: authMethod, schema
||Cluster.addInstance: Missing values in instance definition: host
||Cluster.addInstance: The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' already belongs to the ReplicaSet: 'default'

//@ Cluster: addInstance 2
||

//@ Cluster: addInstance 3
||

//@<OUT> Cluster: describe cluster with instance
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

//@<OUT> Cluster: status cluster with instance
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



//@ Cluster: removeInstance errors
||Invalid number of arguments in Cluster.removeInstance, expected 1 but got 0
||Invalid number of arguments in Cluster.removeInstance, expected 1 but got 2
||Cluster.removeInstance: Invalid connection options, expected either a URI or a Dictionary
||Cluster.removeInstance: Invalid values in instance definition: authMethod, schema, user
||Cluster.removeInstance: The instance 'somehost:3306' does not belong to the ReplicaSet: 'default'

//@ Cluster: removeInstance read only
||

//@<OUT> Cluster: describe removed read only member
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

//@<OUT> Cluster: status removed read only member
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

//@ Cluster: addInstance read only back
||

//@<OUT> Cluster: describe after adding read only instance back
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


//@<OUT> Cluster: status after adding read only instance back
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


//@ Cluster: remove_instance master
||

//@ Connecting to new master
||

//@<OUT> Cluster: describe on new master
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "instances": [
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

//@<OUT> Cluster: status on new master
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "status": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "<<<localhost>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
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

//@ Cluster: addInstance adding old master as read only
||

//@<OUT> Cluster: describe on new master with slave
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "instances": [
            {
                "host": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                "name": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                "role": "HA"
            },
            {
                "host": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "name": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
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

//@<OUT> Cluster: status on new master with slave
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "status": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "<<<localhost>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "leaves": {
                    "<<<localhost>>>:<<<__mysql_sandbox_port1>>>": {
                        "address": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
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

//@# Dba: kill instance 3
||

//@# Dba: start instance 3
||

//@ Cluster: rejoinInstance errors
||Invalid number of arguments in Cluster.rejoinInstance, expected 1 to 2 but got 0
||Invalid number of arguments in Cluster.rejoinInstance, expected 1 to 2 but got 3
||Cluster.rejoinInstance: Invalid connection options, expected either a URI or a Dictionary
||Cluster.rejoinInstance: Invalid values in instance definition: authMethod, schema
||Cluster.rejoinInstance: The instance 'somehost:3306' does not belong to the ReplicaSet: 'default'

//@#: Dba: rejoin instance 3 ok
||

//@<OUT> Cluster: status for rejoin: success
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "status": "{{Cluster tolerant to up to ONE failure.|Cluster is NOT tolerant to any failures.}}",
        "topology": {
            "<<<localhost>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "leaves": {
                    "<<<localhost>>>:<<<__mysql_sandbox_port1>>>": {
                        "address": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
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

//@# Dba: kill instance 1
||

//@# Dba: kill instance 3
||

//@# Dba: start instance 1
||

//@# Dba: start instance 3
||


//@<OUT> Cluster: status for rejoin quorum lost
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "status": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "<<<localhost>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "leaves": {
                    "<<<localhost>>>:<<<__mysql_sandbox_port1>>>": {
                        "address": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                        "leaves": {},
                        "mode": "R/O",
                        "role": "HA",
                        "status": "{{OFFLINE|UNREACHABLE}}"
                    },
                    "<<<localhost>>>:<<<__mysql_sandbox_port3>>>": {
                        "address": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                        "leaves": {},
                        "mode": "R/O",
                        "role": "HA",
                        "status": "{{OFFLINE|UNREACHABLE}}"
                    }
                },
                "mode": "R/W",
                "role": "HA",
                "status": "ONLINE"
            }
        }
    }
}

//@#: Dba: rejoin instance 3
||Cluster.rejoinInstance: Cannot rejoin instance: the group doesn't have quorum. Please remove and re-add the OFFLINE/UNREACHABLE instances from the cluster.

//@ Cluster: dissolve: error quorum
||Cluster.dissolve: Cannot dissolve the cluster: the group doesn't have quorum.

//@ Cluster: rejoinInstance 1
||

//@ Cluster: rejoinInstance 3
||

//@<OUT> Cluster: status for rejoin successful
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "status": "Cluster tolerant to up to ONE failure.",
        "topology": {
            "<<<localhost>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "leaves": {
                    "<<<localhost>>>:<<<__mysql_sandbox_port1>>>": {
                        "address": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
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

//@ Cluster: dissolve errors
||Cluster.dissolve: Argument #1 is expected to be a map
||Invalid number of arguments in Cluster.dissolve, expected 0 to 1 but got 2
||Cluster.dissolve: Argument #1 is expected to be a map
||Cluster.dissolve: Invalid values in dissolve options: foobar
||Cluster.dissolve: Argument 'force' is expected to be a bool

//The cluster 'devCluster' no longer exists.

//The cluster 'devCluster' no longer exists.
