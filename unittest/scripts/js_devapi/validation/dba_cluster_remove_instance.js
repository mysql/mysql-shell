//@ Initialization
||

//@ Connect
||

//@ create cluster
||

//@ Adding instance
||

//@ Failure: remove_instance - invalid uri
||Invalid URI: Missing user information

//@<OUT> Cluster status
{
    "clusterName": "dev",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "<<<__ssl_mode>>>",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "<<<localhost>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "<<<localhost>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        }
    },
    "groupInformationSourceMember": "mysql://root@<<<localhost>>>:<<<__mysql_sandbox_port1>>>"
}

//@ Remove instance failure due to wrong credentials
||Cluster.removeInstance: ERROR: Error leaving cluster: '<<<localhost>>>:<<<__mysql_sandbox_port2>>>' - mysql.get_classic_session: Access denied for user 'foo'@'localhost' (using password: YES) (RuntimeError)

//@<OUT> Cluster status after remove failed
{
    "clusterName": "dev",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "<<<__ssl_mode>>>",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "<<<localhost>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "<<<localhost>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        }
    },
    "groupInformationSourceMember": "mysql://root@<<<localhost>>>:<<<__mysql_sandbox_port1>>>"
}

//@ Removing instance
||

//@<OUT> Cluster status after removal
{
    "clusterName": "dev",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "<<<__ssl_mode>>>",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "<<<localhost>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        }
    },
    "groupInformationSourceMember": "mysql://root@<<<localhost>>>:<<<__mysql_sandbox_port1>>>"
}

//@ Adding instance on port2 back
||

//@ Stop instance on port2
||

//@<OUT> Cluster status after instance on port2 is stopped
{
    "clusterName": "dev",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "<<<__ssl_mode>>>",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures. 1 member is not active",
        "topology": {
            "<<<localhost>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "<<<localhost>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "(MISSING)"
            }
        }
    },
    "groupInformationSourceMember": "mysql://root@<<<localhost>>>:<<<__mysql_sandbox_port1>>>"
}

//@ Error removing stopped instance on port2
||Cluster.removeInstance: The instance '<<<localhost>>>:<<<__mysql_sandbox_port2>>>' cannot be removed because it is on a '(Missing)' state. Please bring the instance back ONLINE and try to remove it again. If the instance is permanently not reachable, then please use <Cluster>.removeInstance() with the force option set to true to proceed with the operation and only remove the instance from the Cluster Metadata. (RuntimeError)

//@ Remove stopped instance on port2 with force option
||

//@<OUT> Cluster status after removal of instance on port2
{
    "clusterName": "dev",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "<<<__ssl_mode>>>",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "<<<localhost>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        }
    },
    "groupInformationSourceMember": "mysql://root@<<<localhost>>>:<<<__mysql_sandbox_port1>>>"
}

//@ Error removing last instance
||Cluster.removeInstance: The instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>' cannot be removed because it is the only member of the Cluster. Please use <Cluster>.dissolve() instead to remove the last instance and dissolve the Cluster. (LogicError)

//@ Dissolve cluster with success
||

//@ Cluster re-created with success
||

//@ Finalization
||
