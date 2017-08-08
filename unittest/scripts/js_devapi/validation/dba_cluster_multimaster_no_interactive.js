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
        "ssl": "<<<__ssl_mode>>>",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "localhost:<<<__mysql_sandbox_port1>>>": {
                "address": "localhost:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port2>>>": {
                "address": "localhost:<<<__mysql_sandbox_port2>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port3>>>": {
                "address": "localhost:<<<__mysql_sandbox_port3>>>",
                "mode": "R/W",
                "readReplicas": {},
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
        "ssl": "<<<__ssl_mode>>>",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "localhost:<<<__mysql_sandbox_port1>>>": {
                "address": "localhost:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port3>>>": {
                "address": "localhost:<<<__mysql_sandbox_port3>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        }
    }
}

//@ Cluster: removeInstance 3
||

//@ Cluster: Error cannot remove last instance
||Cluster.removeInstance: The instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>' cannot be removed because it is the only member of the Cluster. Please use <Cluster>.dissolve() instead to remove the last instance and dissolve the Cluster. (LogicError)

//@ Dissolve cluster with success
||

//@ Dba: createCluster multiMaster 2, ok
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
        "ssl": "<<<__ssl_mode>>>",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "localhost:<<<__mysql_sandbox_port1>>>": {
                "address": "localhost:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port2>>>": {
                "address": "localhost:<<<__mysql_sandbox_port2>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port3>>>": {
                "address": "localhost:<<<__mysql_sandbox_port3>>>",
                "mode": "R/W",
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

//@ Cluster: rejoinInstance errors
||Invalid number of arguments in Cluster.rejoinInstance, expected 1 to 2 but got 0
||Invalid number of arguments in Cluster.rejoinInstance, expected 1 to 2 but got 3
||Cluster.rejoinInstance: Invalid connection options, expected either a URI or a Dictionary
||Cluster.rejoinInstance: Invalid values in connection options: authMethod
||Cluster.rejoinInstance: The instance 'somehost:3306' does not belong to the ReplicaSet: 'default'

//@#: Dba: rejoin instance 3 ok
||

//@<OUT> Cluster: status for rejoin: success
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "ssl": "<<<__ssl_mode>>>",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "localhost:<<<__mysql_sandbox_port1>>>": {
                "address": "localhost:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port2>>>": {
                "address": "localhost:<<<__mysql_sandbox_port2>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port3>>>": {
                "address": "localhost:<<<__mysql_sandbox_port3>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        }
    }
}
