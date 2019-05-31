#@ Initialization
||

#@<OUT> check_instance_configuration() - instance not valid for cluster usage {VER(<8.0.11)}
Validating local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected

Checking instance configuration...
Configuration file <<<__sandbox1_cnf_path>>> will also be checked.

NOTE: Some configuration options need to be fixed:
+-----------------+---------------+----------------+------------------------------------------------+
| Variable        | Current Value | Required Value | Note                                           |
+-----------------+---------------+----------------+------------------------------------------------+
| binlog_checksum | CRC32         | NONE           | Update the server variable and the config file |
| gtid_mode       | OFF           | ON             | Update the config file and restart the server  |
| server_id       | 0             | <unique ID>    | Update the config file and restart the server  |
+-----------------+---------------+----------------+------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server: an option file is required.
NOTE: Please use the dba.configure_instance() command to repair these issues.

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
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected

Checking instance configuration...
Configuration file <<<__sandbox1_cnf_path>>> will also be checked.

NOTE: Some configuration options need to be fixed:
+-----------------+---------------+----------------+------------------------------------------------+
| Variable        | Current Value | Required Value | Note                                           |
+-----------------+---------------+----------------+------------------------------------------------+
| binlog_checksum | CRC32         | NONE           | Update the server variable and the config file |
| gtid_mode       | OFF           | ON             | Update the config file and restart the server  |
| server_id       | 0             | <unique ID>    | Update the config file and restart the server  |
+-----------------+---------------+----------------+------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
NOTE: Please use the dba.configure_instance() command to repair these issues.

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
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port2>>>

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
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port2>>>

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
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port3>>>

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
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port3>>>

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
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>
Assuming full account name 'myAdmin'@'%' for myAdmin

NOTE: Some configuration options need to be fixed:
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
NOTE: MySQL server needs to be restarted for configuration changes to take effect.

#@<OUT> configure_instance() - instance not valid for cluster usage {VER(<8.0.11)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>
Assuming full account name 'myAdmin'@'%' for myAdmin

NOTE: Some configuration options need to be fixed:
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
NOTE: MySQL server needs to be restarted for configuration changes to take effect.

#@<OUT> configure_instance() - create admin account 2
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port2>>>
Assuming full account name 'myAdmin'@'%' for myAdmin

The instance 'localhost:<<<__mysql_sandbox_port2>>>' is valid for InnoDB cluster usage.

Cluster admin user 'myAdmin'@'%' created.

#@<OUT> configure_instance() - create admin account 3
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port3>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port3>>>
Assuming full account name 'myAdmin'@'%' for myAdmin

The instance 'localhost:<<<__mysql_sandbox_port3>>>' is valid for InnoDB cluster usage.

Cluster admin user 'myAdmin'@'%' created.

#@<OUT> configure_instance() - check if configure_instance() was actually successful by double-checking with check_instance_configuration() {VER(>=8.0.11)}
Validating local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

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
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

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
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port2>>>

The instance 'localhost:<<<__mysql_sandbox_port2>>>' is valid for InnoDB cluster usage.
The instance 'localhost:<<<__mysql_sandbox_port2>>>' is already ready for InnoDB cluster usage.

#@ configure_instance() 2 - instance already valid for cluster usage
||

#@<OUT> create_cluster() {VER(>=8.0.11)}
A new InnoDB cluster will be created on instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'.

Validating instance at <<<hostname>>>:<<<__mysql_sandbox_port1>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

Instance configuration is suitable.
Creating InnoDB cluster 'testCluster' on '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'...

Adding Seed Instance...
Cluster successfully created. Use Cluster.add_instance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

#@<OUT> create_cluster() {VER(<8.0.11)}
A new InnoDB cluster will be created on instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'.

Validating instance at <<<hostname>>>:<<<__mysql_sandbox_port1>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

Instance configuration is suitable.
WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' cannot persist Group Replication configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configure_local_instance() command locally to persist the changes.
Creating InnoDB cluster 'testCluster' on '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'...

Adding Seed Instance...
Cluster successfully created. Use Cluster.add_instance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

#@<OUT> add_instance() 1 clone {VER(>=8.0.17)}
NOTE: The target instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' has not been pre-provisioned (GTID set is
empty), but the cluster was configured to assume that incremental distributed
state recovery can correctly provision it in this case.
The safest and most convenient way to provision a new instance is through
automatic clone provisioning, which will completely overwrite the state of
'<<<hostname>>>:<<<__mysql_sandbox_port2>>>' with a physical snapshot from an existing cluster member. To use
this method by default, set the 'recoveryMethod' option to 'clone'.

The incremental distributed state recovery may be safely used if you are sure
all updates ever executed in the cluster were done with GTIDs enabled, there
are no purged transactions and the new instance contains the same GTID set as
the cluster or a subset of it. To use this method by default, set the
'recoveryMethod' option to 'incremental'.

Incremental distributed state recovery was selected because it seems to be safely usable.
Validating instance at <<<hostname>>>:<<<__mysql_sandbox_port2>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port2>>>

Instance configuration is suitable.
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Adding instance to the cluster...

#@<OUT> add_instance() 1 clone {VER(>=8.0.17)}
{{State recovery already finished for 'localhost:<<<__mysql_sandbox_port2>>>'|Incremental distributed state recovery is now in progress.}}

#@<OUT> add_instance() 1 clone {VER(>=8.0.17)}
The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was successfully added to the cluster.

#@<OUT> add_instance() 1 {VER(<8.0.11)}
Validating instance at <<<hostname>>>:<<<__mysql_sandbox_port2>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port2>>>

Instance configuration is suitable.
WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' cannot persist Group Replication configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configure_local_instance() command locally to persist the changes.
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Adding instance to the cluster...

#@<OUT> add_instance() 1 {VER(<8.0.11)}
{{State recovery already finished for 'localhost:<<<__mysql_sandbox_port2>>>'|Incremental distributed state recovery is now in progress.}}

#@<OUT> add_instance() 1 {VER(<8.0.11)}
WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' cannot persist configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configure_local_instance() command locally to persist the changes.
The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was successfully added to the cluster.

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
                "role": "HA"<<<",\n[[*]]\"version\": \"" + __version + "\"" if (__version_num>=80011) else "">>>
            },
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "role": "HA"<<<",\n[[*]]\"version\": \"" + __version + "\"" if (__version_num>=80011) else "">>>
            },
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "role": "HA"<<<",\n[[*]]\"version\": \"" + __version + "\"" if (__version_num>=80011) else "">>>
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
                "status": "ONLINE"<<<",\n[[*]]\"version\": \"" + __version + "\"" if (__version_num>=80011) else "">>>
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"<<<",\n[[*]]\"version\": \"" + __version + "\"" if (__version_num>=80011) else "">>>
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"<<<",\n[[*]]\"version\": \"" + __version + "\"" if (__version_num>=80011) else "">>>
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
                "status": "ONLINE"<<<",\n[[*]]\"version\": \"" + __version + "\"" if (__version_num>=80011) else "">>>
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"<<<",\n[[*]]\"version\": \"" + __version + "\"" if (__version_num>=80011) else "">>>
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
                "status": "ONLINE"<<<",\n[[*]]\"version\": \"" + __version + "\"" if (__version_num>=80011) else "">>>
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"<<<",\n[[*]]\"version\": \"" + __version + "\"" if (__version_num>=80011) else "">>>
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"<<<",\n[[*]]\"version\": \"" + __version + "\"" if (__version_num>=80011) else "">>>
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
                "status": "ONLINE"<<<",\n[[*]]\"version\": \"" + __version + "\"" if (__version_num>=80011) else "">>>
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"<<<",\n[[*]]\"version\": \"" + __version + "\"" if (__version_num>=80011) else "">>>
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"<<<",\n[[*]]\"version\": \"" + __version + "\"" if (__version_num>=80011) else "">>>
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
                "status": "ONLINE"<<<",\n[[*]]\"version\": \"" + __version + "\"" if (__version_num>=80011) else "">>>
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "n/a",
                "readReplicas": {},
                "role": "HA",
                "shellConnectError": "[[*]]",
                "status": "(MISSING)"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "n/a",
                "readReplicas": {},
                "role": "HA",
                "shellConnectError": "[[*]]",
                "status": "UNREACHABLE"<<<",\n[[*]]\"version\": \"" + __version + "\"" if (__version_num>=80011) else "">>>
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
                "status": "ONLINE"<<<",\n[[*]]\"version\": \"" + __version + "\"" if (__version_num>=80011) else "">>>
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "n/a",
                "readReplicas": {},
                "role": "HA",
                "shellConnectError": "[[*]]",
                "status": "(MISSING)"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "n/a",
                "readReplicas": {},
                "role": "HA",
                "shellConnectError": "[[*]]",
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
                "status": "ONLINE"<<<",\n[[*]]\"version\": \"" + __version + "\"" if (__version_num>=80011) else "">>>
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"<<<",\n[[*]]\"version\": \"" + __version + "\"" if (__version_num>=80011) else "">>>
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"<<<",\n[[*]]\"version\": \"" + __version + "\"" if (__version_num>=80011) else "">>>
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
|| This function is not available through a session to a standalone instance (metadata exists, instance belongs to that metadata, but GR is not active)

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
                "status": "ONLINE"<<<",\n[[*]]\"version\": \"" + __version + "\"" if (__version_num>=80011) else "">>>
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"<<<",\n[[*]]\"version\": \"" + __version + "\"" if (__version_num>=80011) else "">>>
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"<<<",\n[[*]]\"version\": \"" + __version + "\"" if (__version_num>=80011) else "">>>
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
                "status": "ONLINE"<<<",\n[[*]]\"version\": \"" + __version + "\"" if (__version_num>=80011) else "">>>
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"<<<",\n[[*]]\"version\": \"" + __version + "\"" if (__version_num>=80011) else "">>>
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
