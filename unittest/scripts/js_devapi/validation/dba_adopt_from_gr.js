//@ Initialization
||

//@ it's not possible to adopt from GR without existing group replication
||Dba.createCluster: The adoptFromGR option is set to true, but there is no replication group to adopt (ArgumentError)

//@ Create group by hand
||

//@ Create cluster adopting from GR
||

//@<OUT> Confirm new replication users were created and replaced existing ones, but didn't drop the old ones that don't belong to shell (WL#12773 FR1.5 and FR3)
user	host
mysql_innodb_cluster_1111	%
mysql_innodb_cluster_2222	%
some_user	%
3
instance_name	attributes
<<<hostname>>>:<<<__mysql_sandbox_port1>>>	{"recoveryAccountHost": "%", "recoveryAccountUser": "mysql_innodb_cluster_1111"}
<<<hostname>>>:<<<__mysql_sandbox_port2>>>	{"recoveryAccountHost": "%", "recoveryAccountUser": "mysql_innodb_cluster_2222"}
2
recovery_user_name
mysql_innodb_cluster_1111
1
user	host
mysql_innodb_cluster_1111	%
mysql_innodb_cluster_2222	%
some_user	%
3
instance_name	attributes
<<<hostname>>>:<<<__mysql_sandbox_port1>>>	{"recoveryAccountHost": "%", "recoveryAccountUser": "mysql_innodb_cluster_1111"}
<<<hostname>>>:<<<__mysql_sandbox_port2>>>	{"recoveryAccountHost": "%", "recoveryAccountUser": "mysql_innodb_cluster_2222"}
2
recovery_user_name
mysql_innodb_cluster_2222
1

//@<OUT> Check cluster status
{
    "clusterName": "testCluster",
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
                "status": "ONLINE",
                "version": "[[*]]"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "[[*]]"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@ dissolve the cluster
||

//@ it's not possible to adopt from GR when cluster was dissolved
||Dba.createCluster: The adoptFromGR option is set to true, but there is no replication group to adopt (ArgumentError)

//@ Finalization
||
