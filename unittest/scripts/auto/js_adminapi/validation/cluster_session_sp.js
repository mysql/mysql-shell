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
||The cluster object is disconnected. Please use dba.getCluster to obtain a fresh cluster handle.

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
|Protocol version:             Classic 10|
|TCP port:                     <<<__mysql_sandbox_port1>>>|

//@ SP - Connect with --redirect-primary while connected to a secondary
|Reconnecting to the PRIMARY instance of an InnoDB cluster (<<<hostname>>>:<<<__mysql_sandbox_port1>>>)...|
|Protocol version:             Classic 10|
|TCP port:                     <<<__mysql_sandbox_port1>>>|

//@ SP - Connect with --redirect-primary while connected to a non-cluster member (error)
|While handling --redirect-primary:|
|Metadata schema of an InnoDB cluster or ReplicaSet not found|

//@ SP - Connect with --redirect-secondary while connected to the primary
|Reconnecting to the SECONDARY instance of an InnoDB cluster (<<<hostname>>>:<<<__mysql_sandbox_port2>>>)...|
|Protocol version:             Classic 10|
|TCP port:                     <<<__mysql_sandbox_port2>>>|

//@ SP - Connect with --redirect-secondary while connected to a secondary
|NOTE: --redirect-secondary ignored because target is already a SECONDARY|
|Protocol version:             Classic 10|
|TCP port:                     <<<__mysql_sandbox_port2>>>|

//@ SP - Connect with --redirect-secondary while connected to a non-cluster member (error)
|While handling --redirect-secondary:|
|Metadata schema of an InnoDB cluster or ReplicaSet not found|

//@ SPX - Connect with no options and ensure it will connect to the specified member
|Protocol version:             X protocol|
|TCP port:                     <<<__mysql_sandbox_port1>>>0|

//@ SPX - Connect with no options and ensure it will connect to the specified member 2
|Protocol version:             X protocol|
|TCP port:                     <<<__mysql_sandbox_port2>>>0|

//@ SPX - Connect with --redirect-primary while connected to the primary
|NOTE: --redirect-primary ignored because target is already a PRIMARY|
|Protocol version:             X protocol|
|TCP port:                     <<<__mysql_sandbox_port1>>>0|

//@ SPX - Connect with --redirect-primary while connected to a secondary
|Reconnecting to the PRIMARY instance of an InnoDB cluster (<<<hostname>>>:<<<__mysql_sandbox_port1>>>0)...|
|Protocol version:             X protocol|
|TCP port:                     <<<__mysql_sandbox_port1>>>0|

//@ SPX - Connect with --redirect-primary while connected to a non-cluster member (error)
|While handling --redirect-primary:|
|Metadata schema of an InnoDB cluster or ReplicaSet not found|

//@ SPX - Connect with --redirect-secondary while connected to the primary
|Reconnecting to the SECONDARY instance of an InnoDB cluster (<<<hostname>>>:<<<__mysql_sandbox_port2>>>0)...|
|Protocol version:             X protocol|
|TCP port:                     <<<__mysql_sandbox_port2>>>0|

//@ SPX - Connect with --redirect-secondary while connected to a secondary
|NOTE: --redirect-secondary ignored because target is already a SECONDARY|
|Protocol version:             X protocol|
|TCP port:                     <<<__mysql_sandbox_port2>>>0|

//@ SPX - Connect with --redirect-secondary while connected to a non-cluster member (error)
|While handling --redirect-secondary:|
|Metadata schema of an InnoDB cluster or ReplicaSet not found|

//@ SPX implicit - Connect with no options and ensure it will connect to the specified member
|Protocol version:             X protocol|
|TCP port:                     <<<__mysql_sandbox_port1>>>0|

//@ SPX implicit - Connect with no options and ensure it will connect to the specified member 2
|Protocol version:             X protocol|
|TCP port:                     <<<__mysql_sandbox_port2>>>0|

//@ SPX implicit - Connect with --redirect-primary while connected to the primary
|NOTE: --redirect-primary ignored because target is already a PRIMARY|
|Protocol version:             X protocol|
|TCP port:                     <<<__mysql_sandbox_port1>>>0|

//@ SPX implicit - Connect with --redirect-primary while connected to a secondary
|Reconnecting to the PRIMARY instance of an InnoDB cluster (<<<hostname>>>:<<<__mysql_sandbox_port1>>>0)...|
|Protocol version:             X protocol|
|TCP port:                     <<<__mysql_sandbox_port1>>>0|

//@ SPX implicit - Connect with --redirect-primary while connected to a non-cluster member (error)
|While handling --redirect-primary:|
|Metadata schema of an InnoDB cluster or ReplicaSet not found|

//@ SPX implicit - Connect with --redirect-secondary while connected to the primary
|Reconnecting to the SECONDARY instance of an InnoDB cluster (<<<hostname>>>:<<<__mysql_sandbox_port2>>>0)...|
|Protocol version:             X protocol|
|TCP port:                     <<<__mysql_sandbox_port2>>>0|

//@ SPX implicit - Connect with --redirect-secondary while connected to a secondary
|NOTE: --redirect-secondary ignored because target is already a SECONDARY|
|Protocol version:             X protocol|
|TCP port:                     <<<__mysql_sandbox_port2>>>0|

//@ SPX implicit - Connect with --redirect-secondary while connected to a non-cluster member (error)
|While handling --redirect-secondary:|
|Metadata schema of an InnoDB cluster or ReplicaSet not found|

//@ SP - Connect with --cluster 1
|"groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SP - Connect with --cluster 2
|"groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SP - Connect with --cluster py
|"groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SP - Connect with --cluster on a non-cluster member + cmd (error)
|Option --cluster requires a session to a member of an InnoDB cluster.|
|ERROR: RuntimeError: This function is not available through a session to a standalone instance|

//@ SP - Connect with --cluster on a non-cluster member interactive (error)
|Option --cluster requires a session to a member of an InnoDB cluster.|
|ERROR: RuntimeError: This function is not available through a session to a standalone instance|

//@ SP - Connect with --cluster on a non-cluster member (error)
|Option --cluster requires a session to a member of an InnoDB cluster.|
|ERROR: RuntimeError: This function is not available through a session to a standalone instance|

//@ SP - Connect with --replicaset, expect error {VER(>8.0.0)}
|Option --replicaset requires a session to a member of an InnoDB ReplicaSet.|
|This function is not available through a session to an instance already in an InnoDB cluster|

//@ SP - Connect with --replicaset, expect error {VER(<8.0.0)}
|Unsupported server version: This AdminAPI operation requires MySQL version 8.0 or newer, but target is |

//@ SP - Connect with --cluster + --redirect-primary 1
|NOTE: --redirect-primary ignored because target is already a PRIMARY|
|    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SP - Connect with --cluster + --redirect-primary 2
|Reconnecting to the PRIMARY instance of an InnoDB cluster (<<<hostname>>>:<<<__mysql_sandbox_port1>>>)...|
|    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SP - Connect with --cluster + --redirect-secondary 1
|Reconnecting to the SECONDARY instance of an InnoDB cluster (<<<hostname>>>:<<<__mysql_sandbox_port2>>>)...|
|"groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SP - Connect with --cluster + --redirect-secondary 2
|NOTE: --redirect-secondary ignored because target is already a SECONDARY|
|"groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SP - Connect with --replicaset + --redirect-primary 1, expect error {VER(>8.0.0)}
|NOTE: --redirect-primary ignored because target is already a PRIMARY|
|Option --replicaset requires a session to a member of an InnoDB ReplicaSet.|
|This function is not available through a session to an instance already in an InnoDB cluster|

//@ SP - Connect with --replicaset + --redirect-primary 1, expect error {VER(<8.0.0)}
|Unsupported server version: This AdminAPI operation requires MySQL version 8.0 or newer, but target is |

//@ SP - Connect with --replicaset + --redirect-primary 2, expect error {VER(>8.0.0)}
|Reconnecting to the PRIMARY instance of an InnoDB cluster (<<<hostname>>>:<<<__mysql_sandbox_port1>>>)...|
|Option --replicaset requires a session to a member of an InnoDB ReplicaSet.|
|This function is not available through a session to an instance already in an InnoDB cluster|

//@ SP - Connect with --replicaset + --redirect-primary 2, expect error {VER(<8.0.0)}
|Unsupported server version: This AdminAPI operation requires MySQL version 8.0 or newer, but target is |

//@ SP - Connect with --replicaset + --redirect-secondary 1, expect error {VER(>8.0.0)}
|Reconnecting to the SECONDARY instance of an InnoDB cluster (<<<hostname>>>:<<<__mysql_sandbox_port2>>>)...|
|Option --replicaset requires a session to a member of an InnoDB ReplicaSet.|
|This function is not available through a session to an instance already in an InnoDB cluster|

//@ SP - Connect with --replicaset + --redirect-secondary 1, expect error {VER(<8.0.0)}
|Unsupported server version: This AdminAPI operation requires MySQL version 8.0 or newer, but target is |

//@ SP - Connect with --replicaset + --redirect-secondary 2, expect error {VER(>8.0.0)}
|NOTE: --redirect-secondary ignored because target is already a SECONDARY|
|Option --replicaset requires a session to a member of an InnoDB ReplicaSet.|
|This function is not available through a session to an instance already in an InnoDB cluster|

//@ SP - Connect with --replicaset + --redirect-secondary 2, expect error {VER(<8.0.0)}
|Unsupported server version: This AdminAPI operation requires MySQL version 8.0 or newer, but target is |

//@ SPX - Connect with --cluster 1
|"groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SPX - Connect with --cluster 2
|"groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SPX - Connect with --cluster py
|"groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SPX - Connect with --cluster on a non-cluster member (error)
|Option --cluster requires a session to a member of an InnoDB cluster.|
|ERROR: RuntimeError: This function is not available through a session to a standalone instance|

//@ SPX - Connect with --cluster + --redirect-primary 1
|NOTE: --redirect-primary ignored because target is already a PRIMARY|
|    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SPX - Connect with --cluster + --redirect-primary 2
|Reconnecting to the PRIMARY instance of an InnoDB cluster (<<<hostname>>>:<<<__mysql_sandbox_port1>>>0)...|
|    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SPX - Connect with --cluster + --redirect-secondary 1
|Reconnecting to the SECONDARY instance of an InnoDB cluster (<<<hostname>>>:<<<__mysql_sandbox_port2>>>0)...|
|"groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SPX - Connect with --cluster + --redirect-secondary 2
|NOTE: --redirect-secondary ignored because target is already a SECONDARY|
|"groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SPX - Connect with --replicaset + --redirect-primary 2, expect error {VER(>8.0.0)}
|Reconnecting to the PRIMARY instance of an InnoDB cluster (<<<hostname>>>:<<<__mysql_sandbox_x_port1>>>)...|
|Option --replicaset requires a session to a member of an InnoDB ReplicaSet.|
|This function is not available through a session to an instance already in an InnoDB cluster|

//@ SPX - Connect with --replicaset + --redirect-primary 2, expect error {VER(<8.0.0)}
|Unsupported server version: This AdminAPI operation requires MySQL version 8.0 or newer, but target is |

//@ SPX - Connect with --replicaset + --redirect-secondary 1, expect error {VER(>8.0.0)}
|Reconnecting to the SECONDARY instance of an InnoDB cluster (<<<hostname>>>:<<<__mysql_sandbox_x_port2>>>)...|
|Option --replicaset requires a session to a member of an InnoDB ReplicaSet.|
|This function is not available through a session to an instance already in an InnoDB cluster|

//@ SPX - Connect with --replicaset + --redirect-secondary 1, expect error {VER(<8.0.0)}
|Unsupported server version: This AdminAPI operation requires MySQL version 8.0 or newer, but target is |

//@ SPX implicit - Connect with --cluster 1
|"groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SPX implicit - Connect with --cluster 2
|"groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SPX implicit - Connect with --cluster py
|"groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SPX implicit - Connect with --cluster on a non-cluster member (error)
|Option --cluster requires a session to a member of an InnoDB cluster.|
|ERROR: RuntimeError: This function is not available through a session to a standalone instance|

//@ SPX implicit - Connect with --cluster + --redirect-primary 1
|NOTE: --redirect-primary ignored because target is already a PRIMARY|
|    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SPX implicit - Connect with --cluster + --redirect-primary 2
|Reconnecting to the PRIMARY instance of an InnoDB cluster (<<<hostname>>>:<<<__mysql_sandbox_port1>>>0)...|
|    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SPX implicit - Connect with --cluster + --redirect-secondary 1
|Reconnecting to the SECONDARY instance of an InnoDB cluster (<<<hostname>>>:<<<__mysql_sandbox_port2>>>0)...|
|"groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|

//@ SPX implicit - Connect with --cluster + --redirect-secondary 2
|NOTE: --redirect-secondary ignored because target is already a SECONDARY|
|"groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"|


//@ SP - Dissolve the single-primary cluster while still connected to a secondary
||

//@ Finalization
||
