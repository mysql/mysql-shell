//@<OUT> WL#12052: Create cluster
Creating a Classic session to 'root@localhost:<<<__mysql_sandbox_port1>>>'

//@<ERR> WL#12052: Error when executing switchToMultiPrimaryMode on a cluster with members < 8.0.13 {VER(<8.0.13)}
Cluster.switchToMultiPrimaryMode: Operation not supported on target server version: '<<<__version>>>' (RuntimeError)

//@ WL#12052: Error when executing switchToMultiPrimaryMode on a cluster with 1 or more members not ONLINE < 8.0.13 {VER(>=8.0.13)}
|ERROR: The instance 'localhost:<<<__mysql_sandbox_port3>>>' has the status: '(MISSING)'. All members must be ONLINE.|One or more instances of the cluster are not ONLINE. (RuntimeError)

//@<ERR> WL#12052: Error when executing switchToMultiPrimaryMode on a cluster with no visible quorum < 8.0.13 {VER(>=8.0.13)}
Cluster.switchToMultiPrimaryMode: There is no quorum to perform the operation (RuntimeError)

//@ WL#12052: Re-create the cluster {VER(>=8.0.13)}
||

//@<OUT> WL#12052: Switch to multi-primary mode {VER(>=8.0.13)}
Switching cluster 'cluster' to Multi-Primary mode...

Instance 'localhost:<<<__mysql_sandbox_port1>>>' remains PRIMARY.
Instance 'localhost:<<<__mysql_sandbox_port2>>>' was switched from SECONDARY to PRIMARY.
Instance 'localhost:<<<__mysql_sandbox_port3>>>' was switched from SECONDARY to PRIMARY.

The cluster successfully switched to Multi-Primary mode.

//@<OUT> WL#12052: Check cluster status after changing to multi-primary {VER(>=8.0.13)}
{
    "clusterName": "cluster",
    "defaultReplicaSet": {
        "name": "default",
        "ssl": "REQUIRED",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "localhost:<<<__mysql_sandbox_port1>>>": {
                "address": "localhost:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port2>>>": {
                "address": "localhost:<<<__mysql_sandbox_port2>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port3>>>": {
                "address": "localhost:<<<__mysql_sandbox_port3>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        },
        "topologyMode": "Multi-Primary"
    },
    "groupInformationSourceMember": "<<<real_hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@<OUT> WL#12052: Verify the values of auto_increment_% in the seed instance {VER(>=8.0.13)}
auto_increment_increment = 7
auto_increment_offset = <<<__expected_auto_inc_offset>>>

//@<OUT> WL#12052: Verify the values of auto_increment_% in the member2 {VER(>=8.0.13)}
auto_increment_increment = 7
auto_increment_offset = <<<__expected_auto_inc_offset>>>

//@<OUT> WL#12052: Verify the values of auto_increment_% in the member3 {VER(>=8.0.13)}
auto_increment_increment = 7
auto_increment_offset = <<<__expected_auto_inc_offset>>>

//@<OUT> WL#12052: Verify the value of replicasets.topology_type {VER(>=8.0.13)}
mm

//@<OUT> WL#12052: Switch a multi-primary cluster to multi-primary is a no-op {VER(>=8.0.13)}
Switching cluster 'cluster' to Multi-Primary mode...

Instance 'localhost:<<<__mysql_sandbox_port1>>>' remains PRIMARY.
Instance 'localhost:<<<__mysql_sandbox_port2>>>' remains PRIMARY.
Instance 'localhost:<<<__mysql_sandbox_port3>>>' remains PRIMARY.

The cluster successfully switched to Multi-Primary mode.

//@<OUT> WL#12052: Check cluster status after changing to multi-primary no-op {VER(>=8.0.13)}
{
    "clusterName": "cluster",
    "defaultReplicaSet": {
        "name": "default",
        "ssl": "REQUIRED",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "localhost:<<<__mysql_sandbox_port1>>>": {
                "address": "localhost:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port2>>>": {
                "address": "localhost:<<<__mysql_sandbox_port2>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port3>>>": {
                "address": "localhost:<<<__mysql_sandbox_port3>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        },
        "topologyMode": "Multi-Primary"
    },
    "groupInformationSourceMember": "<<<real_hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@ WL#12052: Finalization
||
