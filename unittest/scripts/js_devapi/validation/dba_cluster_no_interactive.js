
//@# Cluster: addInstance errors
||Invalid number of arguments, expected 1 to 2 but got 0
||Invalid number of arguments, expected 1 to 2 but got 4
||Argument #2 is expected to be a map
||Argument #2 is expected to be a map
||Invalid values in connection options: weird
||Invalid values in connection options: ipWhitelist, memberSslMode
||Argument #2 is expected to be a map
||Invalid value for memberSslMode option. Supported values: AUTO,DISABLED,REQUIRED,VERIFY_CA,VERIFY_IDENTITY.
||Invalid value for memberSslMode option. Supported values: AUTO,DISABLED,REQUIRED,VERIFY_CA,VERIFY_IDENTITY.
||Invalid value for ipWhitelist: string value cannot be empty.
||The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is already part of this InnoDB cluster
||The label can not be empty.
||The label can only start with an alphanumeric or the '_' character.
||The label can only contain alphanumerics or the '_', '.', '-', ':' characters. Invalid character '#' found.
||The label can not be greater than 256 characters.

//@ Cluster: addInstance 2
||

//@ Cluster: addInstance 3
||

//@<OUT> Cluster: describe cluster with instance
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "topology": [
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            },
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "label": "2nd",
                "role": "HA"
            },
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "role": "HA"
            }
        ],
        "topologyMode": "Single-Primary"
    }
}

//@<ERR> Cluster: removeInstance errors
Cluster.removeInstance: Invalid number of arguments, expected 1 to 2 but got 0 (ArgumentError)
Cluster.removeInstance: Invalid number of arguments, expected 1 to 2 but got 3 (ArgumentError)
Cluster.removeInstance: Argument #1: Invalid connection options, expected either a URI or a Connection Options Dictionary (TypeError)
Cluster.removeInstance: Argument #1: Invalid values in connection options: fakeOption (ArgumentError)
Cluster.removeInstance: Metadata for instance <<<__host>>>:<<<__mysql_port>>> not found [[*]]

//@ Cluster: removeInstance read only
||

//@<OUT> Cluster: describe removed read only member
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "topology": [
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            },
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "role": "HA"
            }
        ],
        "topologyMode": "Single-Primary"
    }
}

//@ Cluster: addInstance read only back
||

//@<OUT> Cluster: describe after adding read only instance back
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "topology": [
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            },
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "role": "HA"
            },
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "role": "HA"
            }
        ],
        "topologyMode": "Single-Primary"
    }
}


//@ Cluster: remove_instance master
||

//@ Cluster: no operations can be done on a disconnected cluster
||The cluster object is disconnected. Please use dba.getCluster to obtain a fresh cluster handle. (RuntimeError)
||The cluster object is disconnected. Please use dba.getCluster to obtain a fresh cluster handle. (RuntimeError)
||The cluster object is disconnected. Please use dba.getCluster to obtain a fresh cluster handle. (RuntimeError)
||The cluster object is disconnected. Please use dba.getCluster to obtain a fresh cluster handle. (RuntimeError)
||The cluster object is disconnected. Please use dba.getCluster to obtain a fresh cluster handle. (RuntimeError)
||The cluster object is disconnected. Please use dba.getCluster to obtain a fresh cluster handle. (RuntimeError)
||The cluster object is disconnected. Please use dba.getCluster to obtain a fresh cluster handle. (RuntimeError)
||The cluster object is disconnected. Please use dba.getCluster to obtain a fresh cluster handle. (RuntimeError)
||The cluster object is disconnected. Please use dba.getCluster to obtain a fresh cluster handle. (RuntimeError)

//@ Connecting to new master
||

//@<OUT> Cluster: describe on new master
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "topology": [
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "role": "HA"
            },
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "label": "3rd_sandbox",
                "role": "HA"
            }
        ],
        "topologyMode": "Single-Primary"
    }
}

//@ Cluster: addInstance adding old master as read only
||

//@<OUT> Cluster: describe on new master with slave
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "topology": [
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "role": "HA"
            },
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "label": "3rd_sandbox",
                "role": "HA"
            },
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "label": "1st_sandbox",
                "role": "HA"
            }
        ],
        "topologyMode": "Single-Primary"
    }
}

//@# Dba: kill instance 3
||

//@# Dba: start instance 3
||

//@ Cluster: rejoinInstance errors
||Invalid number of arguments, expected 1 to 2 but got 0
||Invalid number of arguments, expected 1 to 2 but got 3
||Invalid connection options, expected either a URI or a Connection Options Dictionary
||Invalid values in connection options: ipWhitelist, memberSslMode
||Could not open connection to 'localhost'
||Could not open connection to 'localhost:3306'
||Argument #2 is expected to be a map
||Invalid value for memberSslMode option. Supported values: AUTO,DISABLED,REQUIRED,VERIFY_CA,VERIFY_IDENTITY.
||Invalid value for memberSslMode option. Supported values: AUTO,DISABLED,REQUIRED,VERIFY_CA,VERIFY_IDENTITY.
||Invalid value for ipWhitelist: string value cannot be empty.

//@#: Dba: rejoin instance 3 ok {VER(<8.0.11)}
||

//@#: Dba: Wait instance 3 ONLINE {VER(>=8.0.11)}
||

//@ Cluster: dissolve errors
||Argument #1 is expected to be a map
||Invalid number of arguments, expected 0 to 1 but got 2
||Argument #1 is expected to be a map
||Invalid options: foobar
||Option 'force' Bool expected, but value is String

//@ Cluster: final dissolve
||

//@ Cluster: no operations can be done on a dissolved cluster
||Can't access object member 'name' on an offline cluster
||Can't call function 'addInstance' on an offline cluster
||Can't call function 'checkInstanceState' on an offline cluster
||Can't call function 'describe' on an offline cluster
||Can't call function 'dissolve' on an offline cluster
||Can't call function 'forceQuorumUsingPartitionOf' on an offline cluster
||Can't call function 'getName' on an offline cluster
||Can't call function 'rejoinInstance' on an offline cluster
||Can't call function 'removeInstance' on an offline cluster
||Can't call function 'rescan' on an offline cluster
||Can't call function 'status' on an offline cluster

//@ Cluster: disconnect should work, tho
||
