#@<OUT> get cluster status
{
    "clusterName": "testCluster", 
    "defaultReplicaSet": {
        "name": "default", 
        "topology": [
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>", 
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>", 
                "role": "HA"
            }, 
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>", 
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>", 
                "role": "HA"
            }
        ], 
        "topologyMode": "Single-Primary"
    }
}
