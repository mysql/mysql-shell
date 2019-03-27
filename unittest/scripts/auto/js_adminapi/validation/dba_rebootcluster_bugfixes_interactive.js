//@ Deploy sandboxes
||

//@ Deploy cluster
||

//@<OUT> Check cluster status before reboot
{
    "clusterName": "myCluster",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "DISABLED",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
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
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@ Kill all cluster members
||

//@ Start the members again
||

//@ Reboot cluster from complete outage, not specifying the name
||

//@<OUT> Check cluster status after reboot
{
    "clusterName": "myCluster",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "DISABLED",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
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
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@<OUT> BUG#29305551 - Reboot cluster from complete outage, rejoin fails
ERROR: Cannot rejoin instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' to the cluster because it has asynchronous (master-slave) replication configured and running. Please stop the slave threads by executing the query: 'STOP SLAVE;'

//@<OUT> Check cluster status after reboot - uri2 should not be present
{
    "clusterName": "test",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "DISABLED",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures. 1 member is not active",
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
                "mode": "n/a",
                "readReplicas": {},
                "role": "HA",
                "status": "(MISSING)"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@ BUG#29305551: Finalization
||
