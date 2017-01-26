#@ Dba: create_cluster multiMaster, ok
||

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
                "host": "localhost:<<<__mysql_sandbox_port1>>>",
                "label": "localhost:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            },
            {
                "host": "localhost:<<<__mysql_sandbox_port2>>>",
                "label": "localhost:<<<__mysql_sandbox_port2>>>",
                "role": "HA"
            },
            {
                "host": "localhost:<<<__mysql_sandbox_port3>>>",
                "label": "localhost:<<<__mysql_sandbox_port3>>>",
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
        "status": "Cluster tolerant to up to ONE failure.",
        "topology": {
            "localhost:<<<__mysql_sandbox_port1>>>": {
                "address": "localhost:<<<__mysql_sandbox_port1>>>",
                "leaves": {},
                "mode": "R/W",
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port2>>>": {
                "address": "localhost:<<<__mysql_sandbox_port2>>>",
                "leaves": {},
                "mode": "R/W",
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port3>>>": {
                "address": "localhost:<<<__mysql_sandbox_port3>>>",
                "leaves": {},
                "mode": "R/W",
                "role": "HA",
                "status": "ONLINE"
            }
        }
    }
}


#@ Cluster: remove_instance 2
||

#@<OUT> Cluster: describe removed member
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "instances": [
            {
                "host": "localhost:<<<__mysql_sandbox_port1>>>",
                "label": "localhost:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            },
            {
                "host": "localhost:<<<__mysql_sandbox_port3>>>",
                "label": "localhost:<<<__mysql_sandbox_port3>>>",
                "role": "HA"
            }
        ],
        "name": "default"
    }
}

#@<OUT> Cluster: status removed member
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "status": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "localhost:<<<__mysql_sandbox_port1>>>": {
                "address": "localhost:<<<__mysql_sandbox_port1>>>",
                "leaves": {},
                "mode": "R/W",
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port3>>>": {
                "address": "localhost:<<<__mysql_sandbox_port3>>>",
                "leaves": {},
                "mode": "R/W",
                "role": "HA",
                "status": "ONLINE"
            }
        }
    }
}

#@ Cluster: remove_instance 3
||

#@ Cluster: remove_instance 1
||

#@<OUT> Cluster: describe
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "instances": [],
        "name": "default"
    }
}

#@<OUT> Cluster: status
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "status": "Cluster is NOT tolerant to any failures.",
        "topology": {}
    }
}

#@ Cluster: add_instance 1
||

#@ Cluster: add_instance 2
||

#@ Cluster: add_instance 3
||

#@<OUT> Cluster: status: success
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "status": "Cluster tolerant to up to ONE failure.",
        "topology": {
            "localhost:<<<__mysql_sandbox_port1>>>": {
                "address": "localhost:<<<__mysql_sandbox_port1>>>",
                "leaves": {},
                "mode": "R/W",
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port2>>>": {
                "address": "localhost:<<<__mysql_sandbox_port2>>>",
                "leaves": {},
                "mode": "R/W",
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port3>>>": {
                "address": "localhost:<<<__mysql_sandbox_port3>>>",
                "leaves": {},
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

#@ Cluster: rejoin_instance errors
||Invalid number of arguments in Cluster.rejoin_instance, expected 1 to 2 but got 0
||Invalid number of arguments in Cluster.rejoin_instance, expected 1 to 2 but got 3
||Cluster.rejoin_instance: Invalid connection options, expected either a URI or a Dictionary
||Cluster.rejoin_instance: Invalid values in instance definition: authMethod, schema
||Cluster.rejoin_instance: The instance 'somehost:3306' does not belong to the ReplicaSet: 'default'

#@#: Dba: rejoin instance 3 ok
||

#@<OUT> Cluster: status for rejoin: success
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "status": "Cluster tolerant to up to ONE failure.",
        "topology": {
            "localhost:<<<__mysql_sandbox_port1>>>": {
                "address": "localhost:<<<__mysql_sandbox_port1>>>",
                "leaves": {},
                "mode": "R/W",
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port2>>>": {
                "address": "localhost:<<<__mysql_sandbox_port2>>>",
                "leaves": {},
                "mode": "R/W",
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port3>>>": {
                "address": "localhost:<<<__mysql_sandbox_port3>>>",
                "leaves": {},
                "mode": "R/W",
                "role": "HA",
                "status": "ONLINE"
            }
        }
    }
}
