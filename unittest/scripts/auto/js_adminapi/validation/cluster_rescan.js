//@<OUT> Initialize

{
    "clusterName": "c", 
    "defaultReplicaSet": {
        "name": "default", 
        "primary": "localhost:<<<__mysql_sandbox_port1>>>", 
        "ssl": "REQUIRED", 
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
            "localhost:<<<__mysql_sandbox_port2>>>": {
                "address": "localhost:<<<__mysql_sandbox_port2>>>", 
                "mode": "R/O", 
                "readReplicas": {}, 
                "role": "HA", 
                "status": "ONLINE"
            }
        },
        "topologyMode": "Single-Primary"
    }, 
    "groupInformationSourceMember": "<<<real_hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@<OUT> No-op
{
    "defaultReplicaSet": {
        "name": "default", 
        "newlyDiscoveredInstances": [], 
        "unavailableInstances": []
    }
}

//@<OUT> Instance in group but not MD
{
    "clusterName": "c", 
    "defaultReplicaSet": {
        "name": "default", 
        "primary": "localhost:<<<__mysql_sandbox_port1>>>", 
        "ssl": "REQUIRED", 
        "status": "OK_NO_TOLERANCE", 
        "statusText": "Cluster is NOT tolerant to any failures.", 
        "topology": {
            "localhost:<<<__mysql_sandbox_port1>>>": {
                "address": "localhost:<<<__mysql_sandbox_port1>>>", 
                "mode": "R/W", 
                "readReplicas": {}, 
                "role": "HA", 
                "status": "ONLINE"
            }
        },
        "topologyMode": "Single-Primary"
    }, 
    "groupInformationSourceMember": "<<<real_hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@<OUT> addInstance should fail and suggest a rescan
Instance configuration is suitable.
ERROR: Instance localhost:<<<__mysql_sandbox_port2>>> is part of the GR group but is not in the metadata. Please use <Cluster>.rescan() to update the metadata.

//@<ERR> addInstance should fail and suggest a rescan
Cluster.addInstance: Metadata inconsistent (RuntimeError)

//@<OUT> Finalize
