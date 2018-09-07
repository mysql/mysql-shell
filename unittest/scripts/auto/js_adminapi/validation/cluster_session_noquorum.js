//@ Initialization
||

//@<OUT> Check status
{
    "clusterName": "dev",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "localhost:<<<__mysql_sandbox_port1>>>",
        "ssl": "DISABLED",
        "status": "NO_QUORUM",
        "statusText": "Cluster has no quorum as visible from 'localhost:<<<__mysql_sandbox_port1>>>' and cannot process write transactions. 1 member is not active",
        "topology": {
            "localhost:<<<__mysql_sandbox_port1>>>": {
                "address": "localhost:<<<__mysql_sandbox_port1>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port2>>>": {
                "address": "localhost:<<<__mysql_sandbox_port2>>>",
                "mode": "n/a",
                "readReplicas": {},
                "role": "HA",
                "status": "UNREACHABLE"
            }
        }
    },
    "groupInformationSourceMember": "<<<real_hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@ Reconnect
|Creating a Classic session|

//@<OUT> getCluster() again (ok)
{
    "clusterName": "dev",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "localhost:<<<__mysql_sandbox_port1>>>",
        "ssl": "DISABLED",
        "status": "NO_QUORUM",
        "statusText": "Cluster has no quorum as visible from 'localhost:<<<__mysql_sandbox_port1>>>' and cannot process write transactions. 1 member is not active",
        "topology": {
            "localhost:<<<__mysql_sandbox_port1>>>": {
                "address": "localhost:<<<__mysql_sandbox_port1>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port2>>>": {
                "address": "localhost:<<<__mysql_sandbox_port2>>>",
                "mode": "n/a",
                "readReplicas": {},
                "role": "HA",
                "status": "UNREACHABLE"
            }
        }
    },
    "groupInformationSourceMember": "<<<real_hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@# getCluster() and connectToPrimary:true (fail)
||Dba.getCluster: Unable to find a cluster PRIMARY member from the active shell session because the cluster has too many UNREACHABLE members and no quorum is possible.
||Use Dba.getCluster(null, {connectToPrimary:false}) to get a read-only cluster handle. (RuntimeError)

//@<OUT> getCluster() and connectToPrimary:false (succeed)
{
    "clusterName": "dev",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "localhost:<<<__mysql_sandbox_port1>>>",
        "ssl": "DISABLED",
        "status": "NO_QUORUM",
        "statusText": "Cluster has no quorum as visible from 'localhost:<<<__mysql_sandbox_port1>>>' and cannot process write transactions. 1 member is not active",
        "topology": {
            "localhost:<<<__mysql_sandbox_port1>>>": {
                "address": "localhost:<<<__mysql_sandbox_port1>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port2>>>": {
                "address": "localhost:<<<__mysql_sandbox_port2>>>",
                "mode": "n/a",
                "readReplicas": {},
                "role": "HA",
                "status": "UNREACHABLE"
            }
        }
    },
    "groupInformationSourceMember": "<<<real_hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@<OUT> Connect shell to surviving member with --redirect-primary (fail)
ERROR: The cluster appears to be under a partial or total outage and the PRIMARY cannot be selected.
1 out of 2 members of the InnoDB cluster are unreachable from the member we’re connected to, which is not sufficient for a quorum to be reached.

//@<OUT> Connect shell to surviving member with --redirect-secondary (fail)
ERROR: The cluster appears to be under a partial or total outage and an ONLINE SECONDARY cannot be selected.
1 out of 2 members of the InnoDB cluster are unreachable from the member we’re connected to, which is not sufficient for a quorum to be reached.

//@<OUT> Connect shell to surviving member with --cluster (ok)
{
    "clusterName": "dev",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "localhost:<<<__mysql_sandbox_port1>>>",
        "ssl": "DISABLED",
        "status": "NO_QUORUM",
        "statusText": "Cluster has no quorum as visible from 'localhost:<<<__mysql_sandbox_port1>>>' and cannot process write transactions. 1 member is not active",
        "topology": {
            "localhost:<<<__mysql_sandbox_port1>>>": {
                "address": "localhost:<<<__mysql_sandbox_port1>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port2>>>": {
                "address": "localhost:<<<__mysql_sandbox_port2>>>",
                "mode": "n/a",
                "readReplicas": {},
                "role": "HA",
                "status": "UNREACHABLE"
            }
        }
    },
    "groupInformationSourceMember": "<<<real_hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@ Create new cluster and now kill the primary
||

//@<OUT> 2 getCluster() again (ok)
{
    "clusterName": "dev",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "localhost:<<<__mysql_sandbox_port1>>>",
        "ssl": "DISABLED",
        "status": "NO_QUORUM",
        "statusText": "Cluster has no quorum as visible from 'localhost:<<<__mysql_sandbox_port2>>>' and cannot process write transactions. 1 member is not active",
        "topology": {
            "localhost:<<<__mysql_sandbox_port1>>>": {
                "address": "localhost:<<<__mysql_sandbox_port1>>>",
                "mode": "n/a",
                "readReplicas": {},
                "role": "HA",
                "status": "UNREACHABLE"
            },
            "localhost:<<<__mysql_sandbox_port2>>>": {
                "address": "localhost:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        }
    },
    "groupInformationSourceMember": "<<<real_hostname>>>:<<<__mysql_sandbox_port2>>>"
}

//@# 2 getCluster() and connectToPrimary:true (fail)
||Dba.getCluster: Unable to find a cluster PRIMARY member from the active shell session because the cluster has too many UNREACHABLE members and no quorum is possible.
||Use Dba.getCluster(null, {connectToPrimary:false}) to get a read-only cluster handle. (RuntimeError)

//@<OUT> 2 getCluster() and connectToPrimary:false (succeed)
{
    "clusterName": "dev",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "localhost:<<<__mysql_sandbox_port1>>>",
        "ssl": "DISABLED",
        "status": "NO_QUORUM",
        "statusText": "Cluster has no quorum as visible from 'localhost:<<<__mysql_sandbox_port2>>>' and cannot process write transactions. 1 member is not active",
        "topology": {
            "localhost:<<<__mysql_sandbox_port1>>>": {
                "address": "localhost:<<<__mysql_sandbox_port1>>>",
                "mode": "n/a",
                "readReplicas": {},
                "role": "HA",
                "status": "UNREACHABLE"
            },
            "localhost:<<<__mysql_sandbox_port2>>>": {
                "address": "localhost:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        }
    },
    "groupInformationSourceMember": "<<<real_hostname>>>:<<<__mysql_sandbox_port2>>>"
}

//@<OUT> 2 Connect shell to surviving member with --redirect-primary (fail)
ERROR: The cluster appears to be under a partial or total outage and the PRIMARY cannot be selected.
1 out of 2 members of the InnoDB cluster are unreachable from the member we’re connected to, which is not sufficient for a quorum to be reached.

//@<OUT> 2 Connect shell to surviving member with --redirect-secondary (fail)
ERROR: The cluster appears to be under a partial or total outage and an ONLINE SECONDARY cannot be selected.
1 out of 2 members of the InnoDB cluster are unreachable from the member we’re connected to, which is not sufficient for a quorum to be reached.

//@<OUT> 2 Connect shell to surviving member with --cluster (ok)
{
    "clusterName": "dev",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "localhost:<<<__mysql_sandbox_port1>>>",
        "ssl": "DISABLED",
        "status": "NO_QUORUM",
        "statusText": "Cluster has no quorum as visible from 'localhost:<<<__mysql_sandbox_port2>>>' and cannot process write transactions. 1 member is not active",
        "topology": {
            "localhost:<<<__mysql_sandbox_port1>>>": {
                "address": "localhost:<<<__mysql_sandbox_port1>>>",
                "mode": "n/a",
                "readReplicas": {},
                "role": "HA",
                "status": "UNREACHABLE"
            },
            "localhost:<<<__mysql_sandbox_port2>>>": {
                "address": "localhost:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        }
    },
    "groupInformationSourceMember": "<<<real_hostname>>>:<<<__mysql_sandbox_port2>>>"
}

//@ Finalization
||
