//@<OUT> Verify Cluster Status
{
    "clusterName": "sample", 
    "defaultReplicaSet": {
        "name": "default", 
        "ssl": "REQUIRED", 
        "status": "OK_NO_TOLERANCE", 
        "statusText": "Cluster is NOT tolerant to any failures.", 
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>", 
                "mode": "R/W", 
                "readReplicas": {}, 
?{VER(>=8.0.0)}
                "replicationLag": null, 
?{}
                "role": "HA", 
?{VER(<8.0.16)}
                "status": "ONLINE"
?{}
?{VER(>=8.0.16)}
                "status": "ONLINE", 
                "version": "[[*]]"
?{}
            }
        }, 
        "topologyMode": "Multi-Primary"
    }, 
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}
