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
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
            }
        },
        "topologyMode": "Multi-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}


//@ MP - getCluster() on primary
|TCP port:                     <<<__mysql_sandbox_port1>>>|
|    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ MP - getCluster() on another primary
|TCP port:                     <<<__mysql_sandbox_port2>>>|
|    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>"|

//@ MP - getCluster() on primary with connectToPrimary: true
|TCP port:                     <<<__mysql_sandbox_port1>>>|
|    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ MP - getCluster() on another primary with connectToPrimary: true
|TCP port:                     <<<__mysql_sandbox_port2>>>|
|    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>"|

//@ MP - getCluster() on primary with connectToPrimary: false
|TCP port:                     <<<__mysql_sandbox_port1>>>|
|    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ MP - getCluster() on another primary with connectToPrimary: false
|TCP port:                     <<<__mysql_sandbox_port2>>>|
|    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>"|

//@ MPX - getCluster() on primary
|TCP port:                     <<<__mysql_sandbox_port1>>>0|
|    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ MPX - getCluster() on primary (no redirect)
|TCP port:                     <<<__mysql_sandbox_port1>>>0|
|    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

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
|Secondary member requested, but cluster is multi-primary|

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
|Secondary member requested, but cluster is multi-primary|


//@ MP - Connect with --cluster 1
|"groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ MP - Connect with --cluster 2
|"groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>"|

//@ MP - Connect with --cluster + --redirect-primary 1
|NOTE: --redirect-primary ignored because target is already a PRIMARY|
|    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ MP - Connect with --cluster + --redirect-primary 2
|NOTE: --redirect-primary ignored because target is already a PRIMARY|
|    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>"|

//@ MP - Connect with --cluster + --redirect-secondary (error)
|While handling --redirect-secondary:|
|Secondary member requested, but cluster is multi-primary|

//@ MPX - Connect with --cluster 1
|"groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ MPX - Connect with --cluster 2
|"groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>"|

//@ MPX - Connect with --cluster + --redirect-primary 1
|NOTE: --redirect-primary ignored because target is already a PRIMARY|
|    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ MPX - Connect with --cluster + --redirect-primary 2
|NOTE: --redirect-primary ignored because target is already a PRIMARY|
|    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>"|

//@ MPX - Connect with --cluster + --redirect-secondary (error)
|While handling --redirect-secondary:|
|Secondary member requested, but cluster is multi-primary|


//@ Dissolve the multi-primary cluster while connected to sandbox2
||

//@ Finalization
||
