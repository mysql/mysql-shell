//@ Initialization
||

//@ it's not possible to adopt from GR without existing group replication
||Dba.createCluster: The adoptFromGR option is set to true, but there is no replication group to adopt (ArgumentError)

//@ Create cluster
||

//@ Adding instance to cluster
||

//@ Drop Metadata
||

//@ Check cluster status after drop metadata schema
||Cluster.status: This function is not available through a session to an instance belonging to an unmanaged replication group (RuntimeError)

//@ Get data about existing replication users before createCluster with adoptFromGR.
||

//@ Create cluster adopting from GR
||

//@<OUT> Confirm no new replication user was created.
false

//@<OUT> Check cluster status
{
    "clusterName": "testCluster",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<real_hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "<<<__ssl_mode>>>",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "<<<real_hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<real_hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "<<<real_hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<real_hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        }
    },
    "groupInformationSourceMember": "<<<real_hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@ dissolve the cluster
||

//@ it's not possible to adopt from GR when cluster was dissolved
||Dba.createCluster: The adoptFromGR option is set to true, but there is no replication group to adopt (ArgumentError)

//@ Finalization
||
