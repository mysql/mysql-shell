//@ Dba: createCluster multiPrimary, ok
||

//@ Dissolve cluster
||

//@ Dba: createCluster multiMaster with interaction, regression for BUG#25926603
|WARNING: The multiMaster option is deprecated. Please use the multiPrimary option instead.|

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
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            },
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "role": "HA"
            },
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "role": "HA"
            }
        ],
        "topologyMode": "Multi-Primary"
    }
}

//@ Cluster: removeInstance 2
||

//@<OUT> Cluster: describe removed member
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "topology": [
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            },
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "role": "HA"
            }
        ],
        "topologyMode": "Multi-Primary"
    }
}

//@ Cluster: removeInstance 3
||

//@ Cluster: Error cannot remove last instance
||The instance 'localhost:<<<__mysql_sandbox_port1>>>' is the last member of the cluster (RuntimeError)

//@ Dissolve cluster with success
||

//@ Dba: createCluster multiPrimary 2, ok
||

//@ Cluster: addInstance 2 again
||

//@ Cluster: addInstance 3 again
||

//@# Dba: stop instance 3
||

//@ Cluster: rejoinInstance errors
||Invalid number of arguments, expected 1 to 2 but got 0
||Invalid number of arguments, expected 1 to 2 but got 3
||Invalid connection options, expected either a URI or a Connection Options Dictionary
||Invalid values in connection options: authMethod
||Could not open connection to 'localhost'
||Could not open connection to 'localhost:3306'

//@#: Dba: rejoin instance 3 ok
||

//@ Cluster: disconnect should work, tho
||
