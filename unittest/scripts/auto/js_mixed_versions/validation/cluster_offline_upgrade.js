//@ configureLocalInstance
|The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB cluster.|

//@ status
|{|
|    "clusterName": "mycluster", |
|    "defaultReplicaSet": {|
|        "name": "default",|
|        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",|
|        "ssl": "REQUIRED",|
|        "status": "OK_NO_TOLERANCE",|
|        "statusText": "Cluster is NOT tolerant to any failures.",|
|        "topology": {|
|            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {|
|                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",|
|                "mode": "R/W",|
|                "readReplicas": {},|
|                "role": "HA",|
|                "status": "ONLINE"|
|            }|
|        },|
|        "topologyMode": "Single-Primary"|
|    },|
|    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|
|}|

//@<OUT> describe
{
    "clusterName": "mycluster",
    "defaultReplicaSet": {
        "name": "default",
        "topology": [
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            }
        ],
        "topologyMode": "Single-Primary"
    }
}

//@# options
|            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": [|
|                {|
|                    "option": "memberWeight",|
|                    "value": "<<<custom_weigth>>>",|
|                    "variable": "group_replication_member_weight"|
|                },|
|            ],|
|            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": [|
|                {|
|                    "option": "memberWeight",|
|                    "value": "50",|
|                    "variable": "group_replication_member_weight"|
|                },|
|            ],|
|            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": [|
|                {|
|                    "option": "memberWeight",|
|                    "value": "50",|
|                    "variable": "group_replication_member_weight"|
|                },|
|        }|
|    }|
|}|


//@ rescan
|A new instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was discovered in the cluster.|

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
