#@ Initialization
||

#@<OUT> check_instance_configuration() - instance not valid for cluster usage {VER(<8.0.11)}
Validating local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected

Checking instance configuration...
Configuration file <<<__sandbox1_cnf_path>>> will also be checked.

Some configuration options need to be fixed:
+-----------------+---------------+----------------+------------------------------------------------+
| Variable        | Current Value | Required Value | Note                                           |
+-----------------+---------------+----------------+------------------------------------------------+
| binlog_checksum | CRC32         | NONE           | Update the server variable and the config file |
| gtid_mode       | OFF           | ON             | Update the config file and restart the server  |
| server_id       | 0             | <unique ID>    | Update the config file and restart the server  |
+-----------------+---------------+----------------+------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server: an option file is required.
Please use the dba.configureInstance() command to repair these issues.

{
    "config_errors": [
        {
            "action": "server_update+config_update",
            "current": "CRC32",
            "option": "binlog_checksum",
            "required": "NONE"
        },
        {
            "action": "config_update+restart",
            "current": "OFF",
            "option": "gtid_mode",
            "required": "ON"
        },
        {
            "action": "config_update+restart",
            "current": "0",
            "option": "server_id",
            "required": "<unique ID>"
        }
    ],
    "status": "error"
}

#@<OUT> check_instance_configuration() - instance not valid for cluster usage {VER(>=8.0.11)}
Validating local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected

Checking instance configuration...
Configuration file <<<__sandbox1_cnf_path>>> will also be checked.

Some configuration options need to be fixed:
+-----------------+---------------+----------------+------------------------------------------------+
| Variable        | Current Value | Required Value | Note                                           |
+-----------------+---------------+----------------+------------------------------------------------+
| binlog_checksum | CRC32         | NONE           | Update the server variable and the config file |
| gtid_mode       | OFF           | ON             | Update the config file and restart the server  |
| server_id       | 0             | <unique ID>    | Update the config file and restart the server  |
+-----------------+---------------+----------------+------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
Please use the dba.configureInstance() command to repair these issues.

{
    "config_errors": [
        {
            "action": "server_update+config_update",
            "current": "CRC32",
            "option": "binlog_checksum",
            "required": "NONE"
        },
        {
            "action": "config_update+restart",
            "current": "OFF",
            "option": "gtid_mode",
            "required": "ON"
        },
        {
            "action": "config_update+restart",
            "current": "0",
            "option": "server_id",
            "required": "<unique ID>"
        }
    ],
    "status": "error"
}

#@<OUT> check_instance_configuration() - instance valid for cluster usage 2 {VER(<8.0.11)}
Validating local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected

Checking instance configuration...
Configuration file <<<__sandbox2_cnf_path>>> will also be checked.
Instance configuration is compatible with InnoDB cluster

The instance 'localhost:<<<__mysql_sandbox_port2>>>' is valid for InnoDB cluster usage.

{
    "status": "ok"
}

#@<OUT> check_instance_configuration() - instance valid for cluster usage 2 {VER(>=8.0.11)}
Validating local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected

Checking instance configuration...
Configuration file <<<__sandbox2_cnf_path>>> will also be checked.
Instance configuration is compatible with InnoDB cluster

The instance 'localhost:<<<__mysql_sandbox_port2>>>' is valid for InnoDB cluster usage.

{
    "status": "ok"
}

#@<OUT> check_instance_configuration() - instance valid for cluster usage 3 {VER(<8.0.11)}
Validating local MySQL instance listening at port <<<__mysql_sandbox_port3>>> for use in an InnoDB cluster...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected

Checking instance configuration...
Configuration file <<<__sandbox3_cnf_path>>> will also be checked.
Instance configuration is compatible with InnoDB cluster

The instance 'localhost:<<<__mysql_sandbox_port3>>>' is valid for InnoDB cluster usage.

{
    "status": "ok"
}

#@<OUT> check_instance_configuration() - instance valid for cluster usage 3 {VER(>=8.0.11)}
Validating local MySQL instance listening at port <<<__mysql_sandbox_port3>>> for use in an InnoDB cluster...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected

Checking instance configuration...
Configuration file <<<__sandbox3_cnf_path>>> will also be checked.
Instance configuration is compatible with InnoDB cluster

The instance 'localhost:<<<__mysql_sandbox_port3>>>' is valid for InnoDB cluster usage.

{
    "status": "ok"
}

#@<OUT> configure_instance() - instance not valid for cluster usage {VER(>=8.0.11)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>
Assuming full account name 'myAdmin'@'%' for myAdmin

Some configuration options need to be fixed:
+-----------------+---------------+----------------+------------------------------------------------+
| Variable        | Current Value | Required Value | Note                                           |
+-----------------+---------------+----------------+------------------------------------------------+
| binlog_checksum | CRC32         | NONE           | Update the server variable and the config file |
| gtid_mode       | OFF           | ON             | Update the config file and restart the server  |
| server_id       | 0             | <unique ID>    | Update the config file and restart the server  |
+-----------------+---------------+----------------+------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
Do you want to perform the required configuration changes? [y/n]:
Cluster admin user 'myAdmin'@'%' created.
Configuring instance...
The instance 'localhost:<<<__mysql_sandbox_port1>>>' was configured for InnoDB cluster usage.
MySQL server needs to be restarted for configuration changes to take effect.

#@<OUT> configure_instance() - instance not valid for cluster usage {VER(<8.0.11)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>
Assuming full account name 'myAdmin'@'%' for myAdmin

Some configuration options need to be fixed:
+-----------------+---------------+----------------+------------------------------------------------+
| Variable        | Current Value | Required Value | Note                                           |
+-----------------+---------------+----------------+------------------------------------------------+
| binlog_checksum | CRC32         | NONE           | Update the server variable and the config file |
| gtid_mode       | OFF           | ON             | Update the config file and restart the server  |
| server_id       | 0             | <unique ID>    | Update the config file and restart the server  |
+-----------------+---------------+----------------+------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server: an option file is required.
Do you want to perform the required configuration changes? [y/n]:
Cluster admin user 'myAdmin'@'%' created.
Configuring instance...
The instance 'localhost:<<<__mysql_sandbox_port1>>>' was configured for InnoDB cluster usage.
MySQL server needs to be restarted for configuration changes to take effect.

#@<OUT> configure_instance() - create admin account 2
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>
Assuming full account name 'myAdmin'@'%' for myAdmin

The instance 'localhost:<<<__mysql_sandbox_port2>>>' is valid for InnoDB cluster usage.

Cluster admin user 'myAdmin'@'%' created.

#@<OUT> configure_instance() - create admin account 3
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port3>>> for use in an InnoDB cluster...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>
Assuming full account name 'myAdmin'@'%' for myAdmin

The instance 'localhost:<<<__mysql_sandbox_port3>>>' is valid for InnoDB cluster usage.

Cluster admin user 'myAdmin'@'%' created.

#@<OUT> configure_instance() - check if configure_instance() was actually successful by double-checking with check_instance_configuration() {VER(>=8.0.11)}
Validating local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected

Checking instance configuration...
Instance configuration is compatible with InnoDB cluster

The instance 'localhost:<<<__mysql_sandbox_port1>>>' is valid for InnoDB cluster usage.

{
    "status": "ok"
}

#@<OUT> configure_instance() - check if configure_instance() was actually successful by double-checking with check_instance_configuration() {VER(<8.0.11)}
Validating local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected

Checking instance configuration...
Instance configuration is compatible with InnoDB cluster

The instance 'localhost:<<<__mysql_sandbox_port1>>>' is valid for InnoDB cluster usage.

{
    "status": "ok"
}


#@<OUT> configure_instance() - instance already valid for cluster usage
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

WARNING: User 'root' can only connect from localhost.
If you need to manage this instance while connected from other hosts, new account(s) with the proper source address specification must be created.

1) Create remotely usable account for 'root' with same grants and password
2) Create a new admin account for InnoDB cluster with minimal required grants
3) Ignore and continue
4) Cancel

Please select an option [1]: 
The instance 'localhost:<<<__mysql_sandbox_port2>>>' is valid for InnoDB cluster usage.

#@ configure_instance() 2 - instance already valid for cluster usage
||

#@<OUT> create_cluster() {VER(>=8.0.11)}
A new InnoDB cluster will be created on instance 'myAdmin@<<<hostname>>>:<<<__mysql_sandbox_port1>>>'.

Validating instance at <<<hostname>>>:<<<__mysql_sandbox_port1>>>...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Instance configuration is suitable.
Creating InnoDB cluster 'testCluster' on 'myAdmin@<<<hostname>>>:<<<__mysql_sandbox_port1>>>'...
Adding Seed Instance...

Cluster successfully created. Use Cluster.add_instance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

#@<OUT> create_cluster() {VER(<8.0.11)}
A new InnoDB cluster will be created on instance 'myAdmin@<<<hostname>>>:<<<__mysql_sandbox_port1>>>'.

Validating instance at <<<hostname>>>:<<<__mysql_sandbox_port1>>>...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Instance configuration is suitable.
Creating InnoDB cluster 'testCluster' on 'myAdmin@<<<hostname>>>:<<<__mysql_sandbox_port1>>>'...
WARNING: On instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' membership change cannot be persisted since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance() command locally to persist the changes.
Adding Seed Instance...

Cluster successfully created. Use Cluster.add_instance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

#@<OUT> add_instance() 1 {VER(>=8.0.11)}
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Adding instance to the cluster ...

Validating instance at <<<hostname>>>:<<<__mysql_sandbox_port2>>>...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Instance configuration is suitable.
The instance 'myAdmin@<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was successfully added to the cluster.

#@<OUT> add_instance() 1 {VER(<8.0.11)}
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Adding instance to the cluster ...

Validating instance at <<<hostname>>>:<<<__mysql_sandbox_port2>>>...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Instance configuration is suitable.
WARNING: On instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' membership change cannot be persisted since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance() command locally to persist the changes.
WARNING: On instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' membership change cannot be persisted since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance() command locally to persist the changes.
The instance 'myAdmin@<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was successfully added to the cluster.

#@ add_instance() 2
||

#@<OUT> Verify the output of cluster.describe()
{
    "clusterName": "testCluster",
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
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
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

#@<OUT> Verify the output of cluster.status()
{
    "clusterName": "testCluster",
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
                "status": "ONLINE"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

#@ Remove instance
||

#@ get_cluster():
||

#@<OUT> Verify the output of cluster.status() after the instance removal
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
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

#@ Add the instance back to the cluster
||

#@ Kill instance 3 {VER(<8.0.11)}
||

#@ Restart instance 3 {VER(<8.0.11)}
||

#@ Rejoin instance 3 {VER(<8.0.11)}
||

#@<OUT> Verify the output of cluster.status() after the instance rejoined {VER(<8.0.11)}
{
    "clusterName": "testCluster",
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
                "status": "ONLINE"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

#@ dba_configure_local_instance() 1: {VER(<8.0.11)}
||

#@ dba_configure_local_instance() 2: {VER(<8.0.11)}
||

#@ dba_configure_local_instance() 3: {VER(<8.0.11)}
||

#@ Kill instance 2
||

#@ Restart instance 2
||

#@<OUT> Verify the output of cluster.status() after the instance automatically rejoined
{
    "clusterName": "testCluster",
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
                "status": "ONLINE"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

#@ Kill instance 2 - quorum-loss
||

#@ Kill instance 3 - quorum-loss
||

#@<OUT> Verify the cluster lost the quorum
{
    "clusterName": "testCluster",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "DISABLED",
        "status": "NO_QUORUM",
        "statusText": "Cluster has no quorum as visible from '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' and cannot process write transactions. 2 members are not active",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
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
                "mode": "n/a",
                "readReplicas": {},
                "role": "HA",
                "status": "UNREACHABLE"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

#@<OUT> Restore the cluster quorum
Restoring replicaset 'default' from loss of quorum, by using the partition composed of [<<<hostname>>>:<<<__mysql_sandbox_port1>>>]

Restoring the InnoDB cluster ...

The InnoDB cluster was successfully restored using the partition from the instance 'myAdmin@<<<hostname>>>:<<<__mysql_sandbox_port1>>>'.

WARNING: To avoid a split-brain scenario, ensure that all other members of the replicaset are removed or joined back to the group that was restored.

#@<OUT> Check the cluster status after restoring the quorum
{
    "clusterName": "testCluster",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "DISABLED",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures. 2 members are not active",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
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
                "mode": "n/a",
                "readReplicas": {},
                "role": "HA",
                "status": "(MISSING)"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

#@ Restart instance 2 - quorum-loss
||

#@ Restart instance 3 - quorum-loss
||

#@<OUT> Verify the final cluster status
{
    "clusterName": "testCluster",
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
                "status": "ONLINE"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

#@ Kill instance 2 - complete-outage
||

#@ Kill instance 3 - complete-outage
||

#@ Kill instance 1 - complete-outage
||

#@ Restart instance 2 - complete-outage
||

#@ Restart instance 3 - complete-outage
||

#@ Restart instance 1 - complete-outage
||

#@ Verify that it's not possible to get the cluster handle due to complete outage
|| This function is not available through a session to a standalone instance (metadata exists, but GR is not active)

#@ Reboot cluster from complete outage
||

#@<OUT> Verify the final cluster status after reboot
{
    "clusterName": "testCluster",
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
                "status": "ONLINE"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

#@ Remove instance 3 from the MD schema
||

#@ Stop GR on instance 2
||

#@ Get the cluster handle back
||

#@ Rescan the cluster
||

#@<OUT> Verify the final cluster status after rescan
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
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

#@<OUT> Dissolve cluster {VER(<8.0.11)}
Instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is attempting to leave the cluster...
WARNING: On instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' configuration cannot be persisted since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please set the 'group_replication_start_on_boot' variable to 'OFF' in the server configuration file, otherwise it might rejoin the cluster upon restart.
Instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is attempting to leave the cluster...
WARNING: On instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' configuration cannot be persisted since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please set the 'group_replication_start_on_boot' variable to 'OFF' in the server configuration file, otherwise it might rejoin the cluster upon restart.

The cluster was successfully dissolved.
Replication was disabled but user data was left intact.

#@<OUT> Dissolve cluster {VER(>=8.0.11)}
Instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is attempting to leave the cluster...
Instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is attempting to leave the cluster...

The cluster was successfully dissolved.
Replication was disabled but user data was left intact.

#@ Finalization
||
