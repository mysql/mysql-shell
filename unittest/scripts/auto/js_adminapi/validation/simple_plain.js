//@ configureLocalInstance {VER(<8.0.0)}
|The instance '127.0.0.1:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB cluster.|

//@ configureInstance {VER(>=8.0.0)}
|The instance '127.0.0.1:<<<__mysql_sandbox_port2>>>' is already ready to be used in an InnoDB cluster.|

//@ createCluster
||

//@ status
|{|
|    "clusterName": "mycluster", |
|    "defaultReplicaSet": {|
|        "name": "default",|
|        "primary": "127.0.0.1:<<<__mysql_sandbox_port1>>>",|
|        "ssl": "REQUIRED",|
|        "status": "OK_NO_TOLERANCE",|
|        "statusText": "Cluster is NOT tolerant to any failures.",|
|        "topology": {|
|            "127.0.0.1:<<<__mysql_sandbox_port1>>>": {|
|                "address": "127.0.0.1:<<<__mysql_sandbox_port1>>>",|
|                "mode": "R/W",|
|                "readReplicas": {},|
|                "role": "HA",|
|                "status": "ONLINE"|
|            }|
|        },|
|        "topologyMode": "Single-Primary"|
|    },|
|    "groupInformationSourceMember": "127.0.0.1:<<<__mysql_sandbox_port1>>>"|
|}|


//@ checkInstanceState
||

//@<OUT> describe
{
    "clusterName": "mycluster", 
    "defaultReplicaSet": {
        "name": "default", 
        "topology": [
            {
                "address": "127.0.0.1:<<<__mysql_sandbox_port1>>>", 
                "label": "127.0.0.1:<<<__mysql_sandbox_port1>>>", 
                "role": "HA"
            }
        ], 
        "topologyMode": "Single-Primary"
    }
}

//@ disconnect
||

//@ getCluster
||

//@ addInstance
||


//@ removeInstance
||

//@ setPrimaryInstance
||

//@ rejoinInstance
||

//@ forceQuorumUsingPartitionOf
||

//@ rebootClusterFromCompleteOutage
||


//@ setOption {VER(>=8.0.0)}
||

//@ setInstanceOption
||

//@# options
|            "127.0.0.1:<<<__mysql_sandbox_port1>>>": [|
|                {|
|                    "option": "memberWeight",|
|                    "value": "<<<custom_weigth>>>",|
|                    "variable": "group_replication_member_weight"|
|                }|
|            ],|
|            "127.0.0.1:<<<__mysql_sandbox_port2>>>": [|
|                {|
|                    "option": "memberWeight",|
|                    "value": "50",|
|                    "variable": "group_replication_member_weight"|
|                }|
|            ]|
|        }|
|    }|
|}|


//@ switchToMultiPrimaryMode {VER(>=8.0.0)}
||

//@ switchToSinglePrimaryMode {VER(>=8.0.0)}
||


//@ rescan
|A new instance '127.0.0.1:<<<__mysql_sandbox_port2>>>' was discovered in the cluster.|

//@<OUT> listRouters
{
    "clusterName": "clooster", 
    "routers": {
        "routerhost1::system": {
            "hostname": "routerhost1", 
            "lastCheckIn": "2019-01-01 11:22:33", 
            "roPort": null, 
            "roXPort": null, 
            "rwPort": null, 
            "rwXPort": null, 
            "upgradeRequired": true, 
            "version": "8.0.18"
        }, 
        "routerhost2::system": {
            "hostname": "routerhost2", 
            "lastCheckIn": "2019-01-01 11:22:33", 
            "roPort": null, 
            "roXPort": null, 
            "rwPort": null, 
            "rwXPort": null, 
            "upgradeRequired": true, 
            "version": "8.0.18"
        }
    }
}

//@<OUT> removeRouterMetadata
{
    "clusterName": "clooster", 
    "routers": {
        "routerhost2::system": {
            "hostname": "routerhost2", 
            "lastCheckIn": "2019-01-01 11:22:33", 
            "roPort": null, 
            "roXPort": null, 
            "rwPort": null, 
            "rwXPort": null, 
            "upgradeRequired": true, 
            "version": "8.0.18"
        }
    }
}

//@ createCluster(adopt)
||

//@# dissolve
||
