//@# createCluster
||

//@# addInstance
||

//@# status
|{|
|    "clusterName": "clus",|
|    "defaultReplicaSet": {|
|        "name": "default",|
|        "primary": "127.0.0.1:<<<__mysql_sandbox_port1>>>",|
|        "ssl": "REQUIRED",|
|        "status": "OK",|
|        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",|
|        "topology": {|
|            "127.0.0.1:<<<__mysql_sandbox_port1>>>": {|
|                "address": "127.0.0.1:<<<__mysql_sandbox_port1>>>",|
|                "mode": "R/W",|
|                "readReplicas": {},|
|                "role": "HA",|
|                "status": "ONLINE"|
|            },|
|            "127.0.0.1:<<<__mysql_sandbox_port2>>>": {|
|                "address": "127.0.0.1:<<<__mysql_sandbox_port2>>>",|
|                "mode": "R/O",|
|                "readReplicas": {},|
|                "role": "HA",|
|                "status": "ONLINE"|
|            },|
|            "127.0.0.1:<<<__mysql_sandbox_port3>>>": {|
|                "address": "127.0.0.1:<<<__mysql_sandbox_port3>>>",|
|                "mode": "R/O",|
|                "readReplicas": {},|
|                "role": "HA",|
|                "status": "ONLINE"|
|            }|
|        },|
|        "topologyMode": "Single-Primary"|
|    },|
|    "groupInformationSourceMember": "127.0.0.1:<<<__mysql_sandbox_port1>>>"|
|}|

//@# rejoinInstance
||

//@<OUT> describe
{
    "clusterName": "clus", 
    "defaultReplicaSet": {
        "name": "default", 
        "topology": [
            {
                "address": "127.0.0.1:<<<__mysql_sandbox_port1>>>", 
                "label": "127.0.0.1:<<<__mysql_sandbox_port1>>>", 
                "role": "HA"
            }, 
            {
                "address": "127.0.0.1:<<<__mysql_sandbox_port2>>>", 
                "label": "127.0.0.1:<<<__mysql_sandbox_port2>>>", 
                "role": "HA"
            }, 
            {
                "address": "127.0.0.1:<<<__mysql_sandbox_port3>>>", 
                "label": "127.0.0.1:<<<__mysql_sandbox_port3>>>", 
                "role": "HA"
            }
        ], 
        "topologyMode": "Single-Primary"
    }
}

//@<OUT> listRouters
{
    "clusterName": "clus", 
    "routers": {}
}

//@# removeInstance
||

//@# setPrimaryInstance {VER(>=8.0.0)}
||

//@# options
||

//@ setOption {VER(>=8.0.0)}
||

//@# setInstanceOption
|            "127.0.0.1:<<<__mysql_sandbox_port1>>>": [|
|                {|
|                    "option": "memberWeight", |
|                    "value": "42", |
|                    "variable": "group_replication_member_weight"|
|                }|
|            ], |
|            "127.0.0.1:<<<__mysql_sandbox_port2>>>": [|
|                {|
|                    "option": "memberWeight", |
|                    "value": "50", |
|                    "variable": "group_replication_member_weight"|
|                }|
|            ]|
|        }|
|    }|
|}|

//@# forceQuorum
||

//@# rebootCluster
||

//@# rescan
|A new instance '127.0.0.1:<<<__mysql_sandbox_port2>>>' was discovered in the cluster.|

//@# dissolve
||
