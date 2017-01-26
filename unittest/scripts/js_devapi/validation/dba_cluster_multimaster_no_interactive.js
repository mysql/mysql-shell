//@ Dba: createCluster multiMaster, ok
||

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

//@<OUT> Cluster: status cluster with instance
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "status": "Cluster tolerant to up to ONE failure.",
        "topology": {
            "localhost:<<<__mysql_sandbox_port1>>>": {
                "address": "localhost:<<<__mysql_sandbox_port1>>>",
                "readReplicas": {},
                "mode": "R/W",
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port2>>>": {
                "address": "localhost:<<<__mysql_sandbox_port2>>>",
                "readReplicas": {},
                "mode": "R/W",
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port3>>>": {
                "address": "localhost:<<<__mysql_sandbox_port3>>>",
                "readReplicas": {},
                "mode": "R/W",
                "role": "HA",
                "status": "ONLINE"
            }
        }
    }
}


//@ Cluster: removeInstance 2
||

//@<OUT> Cluster: describe removed member
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

//@<OUT> Cluster: status removed member
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "status": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "localhost:<<<__mysql_sandbox_port1>>>": {
                "address": "localhost:<<<__mysql_sandbox_port1>>>",
                "readReplicas": {},
                "mode": "R/W",
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port3>>>": {
                "address": "localhost:<<<__mysql_sandbox_port3>>>",
                "readReplicas": {},
                "mode": "R/W",
                "role": "HA",
                "status": "ONLINE"
            }
        }
    }
}

//@ Cluster: removeInstance 3
||

//@ Cluster: removeInstance 1
||

//@<OUT> Cluster: describe
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "instances": [],
        "name": "default"
    }
}

//@<OUT> Cluster: status
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "status": "Cluster is NOT tolerant to any failures.",
        "topology": {}
    }
}

//@ Cluster: addInstance 1
||

//@ Cluster: addInstance 2
||

//@ Cluster: addInstance 3
||

//@<OUT> Cluster: status: success
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "status": "Cluster tolerant to up to ONE failure.",
        "topology": {
            "localhost:<<<__mysql_sandbox_port1>>>": {
                "address": "localhost:<<<__mysql_sandbox_port1>>>",
                "readReplicas": {},
                "mode": "R/W",
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port2>>>": {
                "address": "localhost:<<<__mysql_sandbox_port2>>>",
                "readReplicas": {},
                "mode": "R/W",
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port3>>>": {
                "address": "localhost:<<<__mysql_sandbox_port3>>>",
                "readReplicas": {},
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
        "status": "Cluster tolerant to up to ONE failure.",
        "topology": {
            "localhost:<<<__mysql_sandbox_port1>>>": {
                "address": "localhost:<<<__mysql_sandbox_port1>>>",
                "readReplicas": {},
                "mode": "R/W",
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port2>>>": {
                "address": "localhost:<<<__mysql_sandbox_port2>>>",
                "readReplicas": {},
                "mode": "R/W",
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port3>>>": {
                "address": "localhost:<<<__mysql_sandbox_port3>>>",
                "readReplicas": {},
                "mode": "R/W",
                "role": "HA",
                "status": "ONLINE"
            }
        }
    }
}
