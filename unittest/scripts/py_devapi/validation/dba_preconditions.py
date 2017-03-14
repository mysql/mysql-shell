#@ Initialization
||

#@<OUT> Standalone Instance : check instance config
{
    "status": "ok"
}

#@<OUT> Standalone Instance : config local instance
{
    "status": "ok"
}

#@<OUT> Standalone Instance: create cluster
{
    "clusterName": "dev",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
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

#@ Standalone Instance: Failed preconditions
||Dba.get_cluster: This function is not available through a session to a standalone instance
||Dba.drop_metadata_schema: This function is not available through a session to a standalone instance
||Cluster.add_instance: This function is not available through a session to a standalone instance
||Cluster.rejoin_instance: This function is not available through a session to a standalone instance
||Cluster.remove_instance: This function is not available through a session to a standalone instance
||Cluster.describe: This function is not available through a session to a standalone instance
||Cluster.status: This function is not available through a session to a standalone instance
||Cluster.dissolve: This function is not available through a session to a standalone instance
||Cluster.check_instance_state: This function is not available through a session to a standalone instance
||Cluster.rescan: This function is not available through a session to a standalone instance

#@ Preparation for read only tests
||

#@ Read Only Instance : get cluster
||

#@<OUT> Read Only Instance : check instance config
{
    "status": "ok"
}

#@<OUT> Read Only Instance : config local instance
{
    "status": "ok"
}

#@<OUT> Read Only Instance : check instance state
{
    "reason": "{{recoverable|new}}",
    "state": "ok"
}

#@ Read Only Instance : rejoin instance
||Cluster.rejoin_instance: The instance '<<<localhost>>>:<<<__mysql_sandbox_port3>>>' does not belong to the ReplicaSet: 'default'.

#@<OUT> Read Only Instance : describe
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

#@<OUT> Read Only Instance : status
{
    "clusterName": "dev",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
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

#@ Read Only: Failed preconditions
||Dba.create_cluster: Unable to create cluster. The instance '<<<localhost>>>:<<<__mysql_sandbox_port2>>>' already belongs to an InnoDB cluster. Use <Dba>.get_cluster() to access it.
||Dba.drop_metadata_schema: This function is not available through a session to a read only instance
||Cluster.add_instance: This function is not available through a session to a read only instance
||Cluster.remove_instance: This function is not available through a session to a read only instance
||Cluster.dissolve: This function is not available through a session to a read only instance
||Cluster.rescan: This function is not available through a session to a read only instance

#@ Preparation for quorumless cluster tests
||

#@ Quorumless Cluster: Failed preconditions
||Dba.create_cluster: Unable to create cluster. The instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>' already belongs to an InnoDB cluster. Use <Dba>.get_cluster() to access it.
||Dba.drop_metadata_schema: There is no quorum to perform the operation
||Cluster.add_instance: There is no quorum to perform the operation
||Cluster.rejoin_instance: There is no quorum to perform the operation
||Cluster.remove_instance: There is no quorum to perform the operation
||Cluster.dissolve: There is no quorum to perform the operation
||Cluster.rescan: There is no quorum to perform the operation

#@ Quorumless Cluster: get cluster
||

#@<OUT> Quorumless Cluster : check instance config
{
    "status": "ok"
}

#@<OUT> Quorumless Cluster : config local instance
{
    "status": "ok"
}

#@<OUT> Quorumless Cluster : describe
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
#@<OUT> Quorumless Cluster : status
{
    "clusterName": "dev",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
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

#@ Preparation for unmanaged instance tests
||

#@ Unmanaged Instance: Failed preconditions
||Dba.create_cluster: Creating a cluster on an unmanaged replication group requires adoptFromGR option to be true
||Dba.get_cluster: This function is not available through a session to an instance belonging to an unmanaged replication group
||Dba.drop_metadata_schema: This function is not available through a session to an instance belonging to an unmanaged replication group
||Cluster.add_instance: This function is not available through a session to an instance belonging to an unmanaged replication group
||Cluster.rejoin_instance: This function is not available through a session to an instance belonging to an unmanaged replication group
||Cluster.remove_instance: This function is not available through a session to an instance belonging to an unmanaged replication group
||Cluster.describe: This function is not available through a session to an instance belonging to an unmanaged replication group
||Cluster.status: This function is not available through a session to an instance belonging to an unmanaged replication group
||Cluster.dissolve: This function is not available through a session to an instance belonging to an unmanaged replication group
||Cluster.check_instance_state: This function is not available through a session to an instance belonging to an unmanaged replication group
||Cluster.rescan: This function is not available through a session to an instance belonging to an unmanaged replication group

#@<OUT> Unmanaged Instance: create cluster
{
    "clusterName": "dev",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
        "status": "{{OK|OK_NO_TOLERANCE}}",
        "statusText": "{{Cluster tolerant to up to ONE failure.|Cluster is NOT tolerant to any failures.}}",
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

#@ XSession: Failed preconditions
||Dba.create_cluster: a Classic Session is required to perform this operation
||Dba.get_cluster: a Classic Session is required to perform this operation
||Dba.drop_metadata_schema: a Classic Session is required to perform this operation
||Cluster.add_instance: a Classic Session is required to perform this operation
||Cluster.rejoin_instance: a Classic Session is required to perform this operation
||Cluster.remove_instance: a Classic Session is required to perform this operation
||Cluster.describe: a Classic Session is required to perform this operation
||Cluster.status: a Classic Session is required to perform this operation
||Cluster.dissolve: a Classic Session is required to perform this operation
||Cluster.check_instance_state: a Classic Session is required to perform this operation
||Cluster.rescan: a Classic Session is required to perform this operation

#@ Finalization
||