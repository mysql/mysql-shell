//@ Initialization
||

//@ Setup 2 member cluster
||

//@<OUT> cluster status
{
    "clusterName": "dev",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "DISABLED",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE"<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE"<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@<OUT> cluster session closed: no longer error
{
    "clusterName": "dev",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "DISABLED",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE"<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE"<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@# disconnect the cluster object
||The cluster object is disconnected. Please use <Dba>.getCluster to obtain a fresh cluster handle.

//@ SP - getCluster() on primary
|TCP port:                     <<<__mysql_sandbox_port1>>>|
|    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SP - getCluster() on secondary
|TCP port:                     <<<__mysql_sandbox_port2>>>|
|    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SP - getCluster() on primary with connectToPrimary: true
|TCP port:                     <<<__mysql_sandbox_port1>>>|
|    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SP - getCluster() on secondary with connectToPrimary: true
|TCP port:                     <<<__mysql_sandbox_port2>>>|
|    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SP - getCluster() on primary with connectToPrimary: false
|TCP port:                     <<<__mysql_sandbox_port1>>>|
|    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SP - getCluster() on secondary with connectToPrimary: false
|TCP port:                     <<<__mysql_sandbox_port2>>>|
|    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>"|

//@ SPX - getCluster() on session to primary
|TCP port:                     <<<__mysql_sandbox_port1>>>0|
|    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SPX - getCluster() on session to secondary
|TCP port:                     <<<__mysql_sandbox_port2>>>0|
|    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SPX - getCluster() on session to primary (no redirect)
|TCP port:                     <<<__mysql_sandbox_port1>>>0|
|    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SPX - getCluster() on session to secondary (no redirect)
|TCP port:                     <<<__mysql_sandbox_port2>>>0|
|    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>"|

//@ SPX implicit - getCluster() on session to primary
|TCP port:                     <<<__mysql_sandbox_port1>>>0|
|    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SPX implicit - getCluster() on session to secondary
|TCP port:                     <<<__mysql_sandbox_port2>>>0|
|    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SPX implicit - getCluster() on session to primary (no redirect)
|TCP port:                     <<<__mysql_sandbox_port1>>>0|
|    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SPX implicit - getCluster() on session to secondary (no redirect)
|TCP port:                     <<<__mysql_sandbox_port2>>>0|
|    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>"|

//@ SP - Connect with no options and ensure it will connect to the specified member
|TCP port:                     <<<__mysql_sandbox_port1>>>|

//@ SP - Connect with no options and ensure it will connect to the specified member 2
|TCP port:                     <<<__mysql_sandbox_port2>>>|

//@ SP - Connect with --redirect-primary while connected to the primary
|NOTE: --redirect-primary ignored because target is already a PRIMARY|
|Session type:                 Classic|
|TCP port:                     <<<__mysql_sandbox_port1>>>|

//@ SP - Connect with --redirect-primary while connected to a secondary
|Reconnecting to PRIMARY instance of the InnoDB cluster (mysql://<<<hostname>>>:<<<__mysql_sandbox_port1>>>)...|
|Session type:                 Classic|
|TCP port:                     <<<__mysql_sandbox_port1>>>|

//@ SP - Connect with --redirect-primary while connected to a non-cluster member (error)
|While handling --redirect-primary:|
|InnoDB cluster error 10001: Cluster metadata not found: Cluster metadata schema not found|

//@ SP - Connect with --redirect-secondary while connected to the primary
|Reconnecting to SECONDARY instance of the InnoDB cluster (mysql://<<<hostname>>>:<<<__mysql_sandbox_port2>>>)...|
|Session type:                 Classic|
|TCP port:                     <<<__mysql_sandbox_port2>>>|

//@ SP - Connect with --redirect-secondary while connected to a secondary
|NOTE: --redirect-secondary ignored because target is already a SECONDARY|
|Session type:                 Classic|
|TCP port:                     <<<__mysql_sandbox_port2>>>|

//@ SP - Connect with --redirect-secondary while connected to a non-cluster member (error)
|While handling --redirect-secondary:|
|InnoDB cluster error 10001: Cluster metadata not found: Cluster metadata schema not found|

//@ SPX - Connect with no options and ensure it will connect to the specified member
|Session type:                 X|
|TCP port:                     <<<__mysql_sandbox_port1>>>0|

//@ SPX - Connect with no options and ensure it will connect to the specified member 2
|Session type:                 X|
|TCP port:                     <<<__mysql_sandbox_port2>>>0|

//@ SPX - Connect with --redirect-primary while connected to the primary
|NOTE: --redirect-primary ignored because target is already a PRIMARY|
|Session type:                 X|
|TCP port:                     <<<__mysql_sandbox_port1>>>0|

//@ SPX - Connect with --redirect-primary while connected to a secondary
|Reconnecting to PRIMARY instance of the InnoDB cluster (mysqlx://<<<hostname>>>:<<<__mysql_sandbox_port1>>>0)...|
|Session type:                 X|
|TCP port:                     <<<__mysql_sandbox_port1>>>0|

//@ SPX - Connect with --redirect-primary while connected to a non-cluster member (error)
|While handling --redirect-primary:|
|InnoDB cluster error 10001: Cluster metadata not found: Cluster metadata schema not found|

//@ SPX - Connect with --redirect-secondary while connected to the primary
|Reconnecting to SECONDARY instance of the InnoDB cluster (mysqlx://<<<hostname>>>:<<<__mysql_sandbox_port2>>>0)...|
|Session type:                 X|
|TCP port:                     <<<__mysql_sandbox_port2>>>0|

//@ SPX - Connect with --redirect-secondary while connected to a secondary
|NOTE: --redirect-secondary ignored because target is already a SECONDARY|
|Session type:                 X|
|TCP port:                     <<<__mysql_sandbox_port2>>>0|

//@ SPX - Connect with --redirect-secondary while connected to a non-cluster member (error)
|While handling --redirect-secondary:|
|InnoDB cluster error 10001: Cluster metadata not found: Cluster metadata schema not found|

//@ SPX implicit - Connect with no options and ensure it will connect to the specified member
|Session type:                 X|
|TCP port:                     <<<__mysql_sandbox_port1>>>0|

//@ SPX implicit - Connect with no options and ensure it will connect to the specified member 2
|Session type:                 X|
|TCP port:                     <<<__mysql_sandbox_port2>>>0|

//@ SPX implicit - Connect with --redirect-primary while connected to the primary
|NOTE: --redirect-primary ignored because target is already a PRIMARY|
|Session type:                 X|
|TCP port:                     <<<__mysql_sandbox_port1>>>0|

//@ SPX implicit - Connect with --redirect-primary while connected to a secondary
|Reconnecting to PRIMARY instance of the InnoDB cluster (mysqlx://<<<hostname>>>:<<<__mysql_sandbox_port1>>>0)...|
|Session type:                 X|
|TCP port:                     <<<__mysql_sandbox_port1>>>0|

//@ SPX implicit - Connect with --redirect-primary while connected to a non-cluster member (error)
|While handling --redirect-primary:|
|InnoDB cluster error 10001: Cluster metadata not found: Cluster metadata schema not found|

//@ SPX implicit - Connect with --redirect-secondary while connected to the primary
|Reconnecting to SECONDARY instance of the InnoDB cluster (mysqlx://<<<hostname>>>:<<<__mysql_sandbox_port2>>>0)...|
|Session type:                 X|
|TCP port:                     <<<__mysql_sandbox_port2>>>0|

//@ SPX implicit - Connect with --redirect-secondary while connected to a secondary
|NOTE: --redirect-secondary ignored because target is already a SECONDARY|
|Session type:                 X|
|TCP port:                     <<<__mysql_sandbox_port2>>>0|

//@ SPX implicit - Connect with --redirect-secondary while connected to a non-cluster member (error)
|While handling --redirect-secondary:|
|InnoDB cluster error 10001: Cluster metadata not found: Cluster metadata schema not found|

//@ SP - Connect with --cluster 1
|"groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SP - Connect with --cluster 2
|"groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SP - Connect with --cluster py
|"groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SP - Connect with --cluster on a non-cluster member + cmd (error)
|Option --cluster requires a session to a member of a InnoDB cluster.|
|ERROR: RuntimeError: Dba.getCluster: This function is not available through a session to a standalone instance|

//@ SP - Connect with --cluster on a non-cluster member interactive (error)
|Option --cluster requires a session to a member of a InnoDB cluster.|
|ERROR: RuntimeError: Dba.getCluster: This function is not available through a session to a standalone instance|

//@ SP - Connect with --cluster on a non-cluster member (error)
|Option --cluster requires a session to a member of a InnoDB cluster.|
|ERROR: RuntimeError: Dba.getCluster: This function is not available through a session to a standalone instance|

//@ SP - Connect with --cluster + --redirect-primary 1
|NOTE: --redirect-primary ignored because target is already a PRIMARY|
|    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SP - Connect with --cluster + --redirect-primary 2
|Reconnecting to PRIMARY instance of the InnoDB cluster (mysql://<<<hostname>>>:<<<__mysql_sandbox_port1>>>)...|
|    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SP - Connect with --cluster + --redirect-secondary 1
|Reconnecting to SECONDARY instance of the InnoDB cluster (mysql://<<<hostname>>>:<<<__mysql_sandbox_port2>>>)...|
|"groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SP - Connect with --cluster + --redirect-secondary 2
|NOTE: --redirect-secondary ignored because target is already a SECONDARY|
|"groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SPX - Connect with --cluster 1
|"groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SPX - Connect with --cluster 2
|"groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SPX - Connect with --cluster py
|"groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SPX - Connect with --cluster on a non-cluster member (error)
|Option --cluster requires a session to a member of a InnoDB cluster.|
|ERROR: RuntimeError: Dba.getCluster: This function is not available through a session to a standalone instance|

//@ SPX - Connect with --cluster + --redirect-primary 1
|NOTE: --redirect-primary ignored because target is already a PRIMARY|
|    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SPX - Connect with --cluster + --redirect-primary 2
|Reconnecting to PRIMARY instance of the InnoDB cluster (mysqlx://<<<hostname>>>:<<<__mysql_sandbox_port1>>>0)...|
|    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SPX - Connect with --cluster + --redirect-secondary 1
|Reconnecting to SECONDARY instance of the InnoDB cluster (mysqlx://<<<hostname>>>:<<<__mysql_sandbox_port2>>>0)...|
|"groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SPX - Connect with --cluster + --redirect-secondary 2
|NOTE: --redirect-secondary ignored because target is already a SECONDARY|
|"groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SPX implicit - Connect with --cluster 1
|"groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SPX implicit - Connect with --cluster 2
|"groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SPX implicit - Connect with --cluster py
|"groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SPX implicit - Connect with --cluster on a non-cluster member (error)
|Option --cluster requires a session to a member of a InnoDB cluster.|
|ERROR: RuntimeError: Dba.getCluster: This function is not available through a session to a standalone instance|

//@ SPX implicit - Connect with --cluster + --redirect-primary 1
|NOTE: --redirect-primary ignored because target is already a PRIMARY|
|    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SPX implicit - Connect with --cluster + --redirect-primary 2
|Reconnecting to PRIMARY instance of the InnoDB cluster (mysqlx://<<<hostname>>>:<<<__mysql_sandbox_port1>>>0)...|
|    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SPX implicit - Connect with --cluster + --redirect-secondary 1
|Reconnecting to SECONDARY instance of the InnoDB cluster (mysqlx://<<<hostname>>>:<<<__mysql_sandbox_port2>>>0)...|
|"groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SPX implicit - Connect with --cluster + --redirect-secondary 2
|NOTE: --redirect-secondary ignored because target is already a SECONDARY|
|"groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|


//@ SP - Dissolve the single-primary cluster while still connected to a secondary
||

//@ Finalization
||
