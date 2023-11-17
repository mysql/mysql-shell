//@ Initialization
||

//@ Setup 2 member cluster
||

//@<OUT> Multi-primary check
{
    "clusterName": "dev",
    "defaultReplicaSet": {
        "name": "default",
        "ssl": "DISABLED",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "memberRole": "PRIMARY",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "[[*]]"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "memberRole": "PRIMARY",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "[[*]]"
            }
        },
        "topologyMode": "Multi-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@ MP - Connect with no options and ensure it will connect to the specified member
|TCP port:                     <<<__mysql_sandbox_port1>>>|

//@ MP - Connect with no options and ensure it will connect to the specified member 2
|TCP port:                     <<<__mysql_sandbox_port2>>>|

//@ MP - Connect with --redirect-primary while connected to the primary
|NOTE: --redirect-primary ignored because target is already a PRIMARY|
|TCP port:                     <<<__mysql_sandbox_port1>>>|

//@ MP - Connect with --redirect-primary while connected to another primary
|NOTE: --redirect-primary ignored because target is already a PRIMARY|
|TCP port:                     <<<__mysql_sandbox_port2>>>|

//@ MP - Connect with --redirect-secondary (error)
|While handling --redirect-secondary:|
|Redirect to a SECONDARY member requested, but an InnoDB Cluster is multi-primary|

//@ MPX - Connect with no options and ensure it will connect to the specified member
|TCP port:                     <<<__mysql_sandbox_port1>>>0|

//@ MPX - Connect with no options and ensure it will connect to the specified member 2
|TCP port:                     <<<__mysql_sandbox_port2>>>0|

//@ MPX - Connect with --redirect-primary while connected to the primary
|NOTE: --redirect-primary ignored because target is already a PRIMARY|
|TCP port:                     <<<__mysql_sandbox_port1>>>0|

//@ MPX - Connect with --redirect-primary while connected to another primary
|NOTE: --redirect-primary ignored because target is already a PRIMARY|
|TCP port:                     <<<__mysql_sandbox_port2>>>0|

//@ MPX - Connect with --redirect-secondary (error)
|While handling --redirect-secondary:|
|Redirect to a SECONDARY member requested, but an InnoDB Cluster is multi-primary|

//@ MP - Connect with --cluster + --redirect-secondary (error)
|While handling --redirect-secondary:|
|Redirect to a SECONDARY member requested, but an InnoDB Cluster is multi-primary|

//@ MPX - Connect with --cluster + --redirect-secondary (error)
|While handling --redirect-secondary:|
|Redirect to a SECONDARY member requested, but an InnoDB Cluster is multi-primary|


//@ Dissolve the multi-primary cluster while connected to sandbox2
||

//@ Finalization
||
