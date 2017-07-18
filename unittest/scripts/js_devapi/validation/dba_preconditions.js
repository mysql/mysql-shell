//@ Initialization
||

//@<OUT> Standalone Instance : check instance config
{
    "status": "ok"
}

//@<OUT> Standalone Instance : config local instance
{
    "status": "ok"
}

//@<OUT> Standalone Instance: create cluster
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
    }
}

//@ Standalone Instance: Failed preconditions
||Dba.getCluster: This function is not available through a session to a standalone instance (RuntimeError)
||Dba.dropMetadataSchema: This function is not available through a session to a standalone instance (RuntimeError)
||Cluster.addInstance: The session was closed. An open session is required to perform this operation (RuntimeError)
||Cluster.rejoinInstance: The session was closed. An open session is required to perform this operation (RuntimeError)
||Cluster.removeInstance: The session was closed. An open session is required to perform this operation (RuntimeError)
||Cluster.describe: The session was closed. An open session is required to perform this operation (RuntimeError)
||Cluster.status: The session was closed. An open session is required to perform this operation (RuntimeError)
||Cluster.dissolve: The session was closed. An open session is required to perform this operation (RuntimeError)
||Cluster.checkInstanceState: The session was closed. An open session is required to perform this operation (RuntimeError)
||Cluster.rescan: The session was closed. An open session is required to perform this operation (RuntimeError)

//@ Preparation for read only tests
||

//@ Read Only Instance : get cluster
||

//@<OUT> Read Only Instance : check instance config
{
    "status": "ok"
}

//@<OUT> Read Only Instance : config local instance
{
    "status": "ok"
}

//@<OUT> Read Only Instance : check instance state
{
    "reason": "{{recoverable|new}}",
    "state": "ok"
}

//@ Read Only Instance : rejoin instance
||Cluster.rejoinInstance: The instance '<<<localhost>>>:<<<__mysql_sandbox_port3>>>' does not belong to the ReplicaSet: 'default'. (RuntimeError)

//@<OUT> Read Only Instance : describe
{
    "clusterName": "dev",
    "defaultReplicaSet": {
        "instances": [
            {
                "host": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "label": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            },
            {
                "host": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "label": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "role": "HA"
            }
        ],
        "name": "default"
    }
}

//@<OUT> Read Only Instance : status
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
    }
}

//@ Read Only: Failed preconditions
||Dba.createCluster: Unable to create cluster. The instance '<<<localhost>>>:<<<__mysql_sandbox_port2>>>' already belongs to an InnoDB cluster. Use <Dba>.getCluster() to access it. (RuntimeError)
||Dba.dropMetadataSchema: This function is not available through a session to a read only instance (RuntimeError)
||Cluster.addInstance: This function is not available through a session to a read only instance (RuntimeError)
||Cluster.removeInstance: This function is not available through a session to a read only instance (RuntimeError)
||Cluster.dissolve: This function is not available through a session to a read only instance (RuntimeError)
||Cluster.rescan: This function is not available through a session to a read only instance (RuntimeError)

//@ Preparation for quorumless cluster tests
||

//@ Quorumless Cluster: Failed preconditions
||Dba.createCluster: Unable to create cluster. The instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>' already belongs to an InnoDB cluster. Use <Dba>.getCluster() to access it. (RuntimeError)
||Dba.dropMetadataSchema: There is no quorum to perform the operation
||Cluster.addInstance: There is no quorum to perform the operation
||Cluster.rejoinInstance: There is no quorum to perform the operation
||Cluster.removeInstance: There is no quorum to perform the operation
||Cluster.dissolve: There is no quorum to perform the operation
||Cluster.rescan: There is no quorum to perform the operation

//@ Quorumless Cluster: get cluster
||

//@<OUT> Quorumless Cluster : check instance config
{
    "status": "ok"
}

//@<OUT> Quorumless Cluster : config local instance
{
    "status": "ok"
}

//@<OUT> Quorumless Cluster : describe
{
    "clusterName": "dev",
    "defaultReplicaSet": {
        "instances": [
            {
                "host": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "label": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            },
            {
                "host": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "label": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "role": "HA"
            }
        ],
        "name": "default"
    }
}
//@<OUT> Quorumless Cluster : status
{
    "clusterName": "dev",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "<<<__ssl_mode>>>",
        "status": "NO_QUORUM",
        "statusText": "Cluster has no quorum as visible from '<<<localhost>>>:<<<__mysql_sandbox_port1>>>' and cannot process write transactions. 1 member is not active",
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
                "status": "{{OFFLINE|UNREACHABLE}}"
            }
        }
    }
}

//@ Preparation for unmanaged instance tests
||

//@ Unmanaged Instance: Failed preconditions
||Dba.createCluster: Creating a cluster on an unmanaged replication group requires adoptFromGR option to be true (ArgumentError)
||Dba.getCluster: This function is not available through a session to an instance belonging to an unmanaged replication group (RuntimeError)
||Dba.dropMetadataSchema: This function is not available through a session to an instance belonging to an unmanaged replication group (RuntimeError)
||Cluster.addInstance: This function is not available through a session to an instance belonging to an unmanaged replication group (RuntimeError)
||Cluster.rejoinInstance: This function is not available through a session to an instance belonging to an unmanaged replication group (RuntimeError)
||Cluster.removeInstance: This function is not available through a session to an instance belonging to an unmanaged replication group (RuntimeError)
||Cluster.describe: This function is not available through a session to an instance belonging to an unmanaged replication group (RuntimeError)
||Cluster.status: This function is not available through a session to an instance belonging to an unmanaged replication group (RuntimeError)
||Cluster.dissolve: This function is not available through a session to an instance belonging to an unmanaged replication group (RuntimeError)
||Cluster.checkInstanceState: This function is not available through a session to an instance belonging to an unmanaged replication group (RuntimeError)
||Cluster.rescan: This function is not available through a session to an instance belonging to an unmanaged replication group (RuntimeError)

//@<OUT> Unmanaged Instance: create cluster
{
    "clusterName": "dev",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "<<<__ssl_mode>>>",
        "status": "{{OK|OK_NO_TOLERANCE}}",
        "status": "{{Cluster tolerant to up to ONE failure.|Cluster is NOT tolerant to any failures.}}",
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
                "status": "{{OFFLINE|UNREACHABLE}}"
            }
        }
    }
}

//@ XSession: Failed preconditions
||Dba.createCluster: a Classic Session is required to perform this operation (RuntimeError)
||Dba.getCluster: a Classic Session is required to perform this operation (RuntimeError)
||Dba.dropMetadataSchema: a Classic Session is required to perform this operation (RuntimeError)
||Cluster.addInstance: The session was closed. An open session is required to perform this operation (RuntimeError)
||Cluster.rejoinInstance: The session was closed. An open session is required to perform this operation (RuntimeError)
||Cluster.removeInstance: The session was closed. An open session is required to perform this operation (RuntimeError)
||Cluster.describe: The session was closed. An open session is required to perform this operation (RuntimeError)
||Cluster.status: The session was closed. An open session is required to perform this operation (RuntimeError)
||Cluster.dissolve: The session was closed. An open session is required to perform this operation (RuntimeError)
||Cluster.checkInstanceState: The session was closed. An open session is required to perform this operation (RuntimeError)
||Cluster.rescan: The session was closed. An open session is required to perform this operation (RuntimeError)

//@ Finalization
||