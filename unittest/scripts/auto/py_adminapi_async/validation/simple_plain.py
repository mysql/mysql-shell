#@<OUT> configure_replica_set_instance + create admin user
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB ReplicaSet...

This instance reports its own address as 127.0.0.1:<<<__mysql_sandbox_port1>>>
Assuming full account name 'admin'@'%' for admin

?{VER(>=8.0.23)}
applierWorkerThreads will be set to the default value of 4.

?{}
NOTE: Some configuration options need to be fixed:
?{VER(<8.0.23)}
+--------------------------+---------------+----------------+--------------------------------------------------+
| Variable                 | Current Value | Required Value | Note                                             |
+--------------------------+---------------+----------------+--------------------------------------------------+
| enforce_gtid_consistency | OFF           | ON             | Update read-only variable and restart the server |
| gtid_mode                | OFF           | ON             | Update read-only variable and restart the server |
| server_id                | 1             | <unique ID>    | Update read-only variable and restart the server |
+--------------------------+---------------+----------------+--------------------------------------------------+
?{}
?{VER(>=8.0.23) and VER(<8.0.26)}
+----------------------------------------+---------------+----------------+--------------------------------------------------+
| Variable                               | Current Value | Required Value | Note                                             |
+----------------------------------------+---------------+----------------+--------------------------------------------------+
| binlog_transaction_dependency_tracking | COMMIT_ORDER  | WRITESET       | Update the server variable                       |
| enforce_gtid_consistency               | OFF           | ON             | Update read-only variable and restart the server |
| gtid_mode                              | OFF           | ON             | Update read-only variable and restart the server |
| server_id                              | 1             | <unique ID>    | Update read-only variable and restart the server |
| slave_parallel_type                    | DATABASE      | LOGICAL_CLOCK  | Update the server variable                       |
| slave_preserve_commit_order            | OFF           | ON             | Update the server variable                       |
+----------------------------------------+---------------+----------------+--------------------------------------------------+
?{}
?{VER(>=8.0.26)}
+----------------------------------------+---------------+----------------+--------------------------------------------------+
| Variable                               | Current Value | Required Value | Note                                             |
+----------------------------------------+---------------+----------------+--------------------------------------------------+
| binlog_transaction_dependency_tracking | COMMIT_ORDER  | WRITESET       | Update the server variable                       |
| enforce_gtid_consistency               | OFF           | ON             | Update read-only variable and restart the server |
| gtid_mode                              | OFF           | ON             | Update read-only variable and restart the server |
| replica_parallel_type                  | DATABASE      | LOGICAL_CLOCK  | Update the server variable                       |
| replica_preserve_commit_order          | OFF           | ON             | Update the server variable                       |
| server_id                              | 1             | <unique ID>    | Update read-only variable and restart the server |
+----------------------------------------+---------------+----------------+--------------------------------------------------+
?{}

Some variables need to be changed, but cannot be done dynamically on the server.
Cluster admin user 'admin'@'%' created.
Configuring instance...
The instance '127.0.0.1:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB ReplicaSet.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.

#@<OUT> configure_replica_set_instance
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB ReplicaSet...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as 127.0.0.1:<<<__mysql_sandbox_port2>>>
Assuming full account name 'admin'@'%' for admin

?{VER(>=8.0.23)}
applierWorkerThreads will be set to the default value of 4.

?{}
The instance '127.0.0.1:<<<__mysql_sandbox_port2>>>' is valid to be used in an InnoDB ReplicaSet.
Cluster admin user 'admin'@'%' created.
The instance '127.0.0.1:<<<__mysql_sandbox_port2>>>' is already ready to be used in an InnoDB ReplicaSet.

?{VER(>=8.0.23)}
Successfully enabled parallel appliers.
?{}

#@<OUT> create_replica_set
A new replicaset with instance '127.0.0.1:<<<__mysql_sandbox_port1>>>' will be created.

* Checking MySQL instance at 127.0.0.1:<<<__mysql_sandbox_port1>>>

This instance reports its own address as 127.0.0.1:<<<__mysql_sandbox_port1>>>
127.0.0.1:<<<__mysql_sandbox_port1>>>: Instance configuration is suitable.

* Updating metadata...

ReplicaSet object successfully created for 127.0.0.1:<<<__mysql_sandbox_port1>>>.
Use rs.add_instance() to add more asynchronously replicated instances to this replicaset and rs.status() to check its status.


#@<OUT> status
{
    "replicaSet": {
        "name": "myrs",
        "primary": "127.0.0.1:<<<__mysql_sandbox_port1>>>",
        "status": "AVAILABLE",
        "statusText": "All instances available.",
        "topology": {
            "127.0.0.1:<<<__mysql_sandbox_port1>>>": {
                "address": "127.0.0.1:<<<__mysql_sandbox_port1>>>",
                "instanceRole": "PRIMARY",
                "mode": "R/W",
                "status": "ONLINE"
            }
        },
        "type": "ASYNC"
    }
}

#@# disconnect
||

#@<OUT> get_replica_set
You are connected to a member of replicaset 'myrs'.

#@<> add_instance using clone recovery {VER(>=8.0.17)}
|Clone based recovery selected through the recoveryMethod option|
|The instance '127.0.0.1:<<<__mysql_sandbox_port2>>>' was added to the replicaset and is replicating from 127.0.0.1:<<<__mysql_sandbox_port1>>>.|

#@<OUT> add_instance using incremental recovery {VER(<8.0.17)}
Adding instance to the replicaset...

* Performing validation checks

This instance reports its own address as 127.0.0.1:<<<__mysql_sandbox_port2>>>
127.0.0.1:<<<__mysql_sandbox_port2>>>: Instance configuration is suitable.

* Checking async replication topology...

* Checking transaction state of the instance...

NOTE: The target instance '127.0.0.1:<<<__mysql_sandbox_port2>>>' has not been pre-provisioned (GTID set is empty). The Shell is unable to decide whether replication can completely recover its state.

Incremental state recovery selected through the recoveryMethod option

* Updating topology
** Configuring 127.0.0.1:<<<__mysql_sandbox_port2>>> to replicate from 127.0.0.1:<<<__mysql_sandbox_port1>>>
** Waiting for new instance to synchronize with PRIMARY...

The instance '127.0.0.1:<<<__mysql_sandbox_port2>>>' was added to the replicaset and is replicating from 127.0.0.1:<<<__mysql_sandbox_port1>>>.

#@<> add_instance 3
||

#@<OUT> remove_instance
The instance '127.0.0.1:<<<__mysql_sandbox_port2>>>' was removed from the replicaset.

Adding instance to the replicaset...

* Performing validation checks

This instance reports its own address as 127.0.0.1:<<<__mysql_sandbox_port2>>>
127.0.0.1:<<<__mysql_sandbox_port2>>>: Instance configuration is suitable.

* Checking async replication topology...

* Checking transaction state of the instance...

Incremental state recovery selected through the recoveryMethod option

* Updating topology
** Configuring 127.0.0.1:<<<__mysql_sandbox_port2>>> to replicate from 127.0.0.1:<<<__mysql_sandbox_port1>>>
** Waiting for new instance to synchronize with PRIMARY...

The instance '127.0.0.1:<<<__mysql_sandbox_port2>>>' was added to the replicaset and is replicating from 127.0.0.1:<<<__mysql_sandbox_port1>>>.

#@<OUT> set_primary_instance
127.0.0.1:<<<__mysql_sandbox_port3>>> will be promoted to PRIMARY of 'myrs'.
The current PRIMARY is 127.0.0.1:<<<__mysql_sandbox_port1>>>.

* Connecting to replicaset instances
** Connecting to 127.0.0.1:<<<__mysql_sandbox_port1>>>
** Connecting to 127.0.0.1:<<<__mysql_sandbox_port3>>>
** Connecting to 127.0.0.1:<<<__mysql_sandbox_port2>>>
** Connecting to 127.0.0.1:<<<__mysql_sandbox_port1>>>
** Connecting to 127.0.0.1:<<<__mysql_sandbox_port3>>>
** Connecting to 127.0.0.1:<<<__mysql_sandbox_port2>>>

* Performing validation checks
** Checking async replication topology...
** Checking transaction state of the instance...

* Synchronizing transaction backlog at 127.0.0.1:<<<__mysql_sandbox_port3>>>

* Updating metadata

* Acquiring locks in replicaset instances
** Pre-synchronizing SECONDARIES
** Acquiring global lock at PRIMARY
** Acquiring global lock at SECONDARIES

* Updating replication topology
** Configuring 127.0.0.1:<<<__mysql_sandbox_port1>>> to replicate from 127.0.0.1:<<<__mysql_sandbox_port3>>>
** Changing replication source of 127.0.0.1:<<<__mysql_sandbox_port2>>> to 127.0.0.1:<<<__mysql_sandbox_port3>>>

127.0.0.1:<<<__mysql_sandbox_port3>>> was promoted to PRIMARY.

#@# force_primary_instance (prepare) {VER(<8.0.23)}
|ERROR: Unable to connect to the PRIMARY of the replicaset myrs: MySQL Error 2003: Could not open connection to '127.0.0.1:<<<__mysql_sandbox_port3>>>': Can't connect to MySQL server on '<<<libmysql_host_description('127.0.0.1', __mysql_sandbox_port3)>>>' ([[*]])|
|{|
|    "replicaSet": {|
|        "name": "myrs", |
|        "primary": "127.0.0.1:<<<__mysql_sandbox_port3>>>", |
|        "status": "UNAVAILABLE", |
|        "statusText": "PRIMARY instance is not available, but there is at least one SECONDARY that could be force-promoted.", |
|        "topology": {|
|            "127.0.0.1:<<<__mysql_sandbox_port1>>>": {|
|                "address": "127.0.0.1:<<<__mysql_sandbox_port1>>>", |
|                "instanceRole": "SECONDARY", |
|                "mode": "R/O", |
|                "replication": {|
|                    "applierStatus": "APPLIED_ALL", |
|                    "applierThreadState": "<<<__replica_keyword_capital>>> has read all relay log; waiting for more updates", |
|                    "receiverStatus": "ERROR", |
|                    "receiverThreadState": "", |
|                    "replicationLag": null|
|                }, |
|                "status": "ERROR"|
|            }, |
|            "127.0.0.1:<<<__mysql_sandbox_port2>>>": {|
|                "address": "127.0.0.1:<<<__mysql_sandbox_port2>>>", |
|                "instanceRole": "SECONDARY", |
|                "mode": "R/O", |
|                "replication": {|
|                    "applierStatus": "APPLIED_ALL", |
|                    "applierThreadState": "<<<__replica_keyword_capital>>> has read all relay log; waiting for more updates", |
|                    "receiverStatus": "ERROR", |
|                    "receiverThreadState": "", |
|                    "replicationLag": null|
|                }, |
|                "status": "ERROR"|
|            }, |
|            "127.0.0.1:<<<__mysql_sandbox_port3>>>": {|
|                "address": "127.0.0.1:<<<__mysql_sandbox_port3>>>", |
|                "connectError": "Could not open connection to '127.0.0.1:<<<__mysql_sandbox_port3>>>': Can't connect to MySQL server on '<<<libmysql_host_description('127.0.0.1', __mysql_sandbox_port3)>>>' ([[*]])", |
|                "fenced": null, |
|                "instanceRole": "PRIMARY", |
|                "mode": null, |
|                "status": "UNREACHABLE"|
|            }|
|        }, |
|        "type": "ASYNC"|
|    }|
|}|

#@# force_primary_instance (prepare) {VER(>=8.0.23)}
|ERROR: Unable to connect to the PRIMARY of the replicaset myrs: MySQL Error 2003: Could not open connection to '127.0.0.1:<<<__mysql_sandbox_port3>>>': Can't connect to MySQL server on '<<<libmysql_host_description('127.0.0.1', __mysql_sandbox_port3)>>>' ([[*]])|
|{|
|    "replicaSet": {|
|        "name": "myrs", |
|        "primary": "127.0.0.1:<<<__mysql_sandbox_port3>>>", |
|        "status": "UNAVAILABLE", |
|        "statusText": "PRIMARY instance is not available, but there is at least one SECONDARY that could be force-promoted.", |
|        "topology": {|
|            "127.0.0.1:<<<__mysql_sandbox_port1>>>": {|
|                "address": "127.0.0.1:<<<__mysql_sandbox_port1>>>", |
|                "instanceRole": "SECONDARY", |
|                "mode": "R/O", |
|                "replication": {|
|                    "applierStatus": "APPLIED_ALL", |
|                    "applierThreadState": "Waiting for an event from Coordinator", |
|                    "applierWorkerThreads": 4, |
|                    "receiverStatus": "ERROR", |
|                    "receiverThreadState": "", |
|                    "replicationLag": null|
|                }, |
|                "status": "ERROR"|
|            }, |
|            "127.0.0.1:<<<__mysql_sandbox_port2>>>": {|
|                "address": "127.0.0.1:<<<__mysql_sandbox_port2>>>", |
|                "instanceRole": "SECONDARY", |
|                "mode": "R/O", |
|                "replication": {|
|                    "applierStatus": "APPLIED_ALL", |
|                    "applierThreadState": "Waiting for an event from Coordinator", |
|                    "applierWorkerThreads": 4, |
|                    "receiverStatus": "ERROR", |
|                    "receiverThreadState": "", |
|                    "replicationLag": null|
|                }, |
|                "status": "ERROR"|
|            }, |
|            "127.0.0.1:<<<__mysql_sandbox_port3>>>": {|
|                "address": "127.0.0.1:<<<__mysql_sandbox_port3>>>", |
|                "connectError": "Could not open connection to '127.0.0.1:<<<__mysql_sandbox_port3>>>': Can't connect to MySQL server on '<<<libmysql_host_description('127.0.0.1', __mysql_sandbox_port3)>>>' ([[*]])", |
|                "fenced": null, |
|                "instanceRole": "PRIMARY", |
|                "mode": null, |
|                "status": "UNREACHABLE"|
|            }|
|        }, |
|        "type": "ASYNC"|
|    }|
|}|

#@<OUT> force_primary_instance
* Connecting to replicaset instances
** Connecting to 127.0.0.1:<<<__mysql_sandbox_port1>>>
** Connecting to 127.0.0.1:<<<__mysql_sandbox_port2>>>

* Waiting for all received transactions to be applied
** Waiting for received transactions to be applied at 127.0.0.1:[[*]]
** Waiting for received transactions to be applied at 127.0.0.1:[[*]]
127.0.0.1:<<<__mysql_sandbox_port1>>> will be promoted to PRIMARY of the replicaset and the former PRIMARY will be invalidated.

* Checking status of last known PRIMARY
NOTE: 127.0.0.1:<<<__mysql_sandbox_port3>>> is UNREACHABLE
* Checking status of promoted instance
NOTE: 127.0.0.1:<<<__mysql_sandbox_port1>>> has status ERROR
* Checking transaction set status
* Promoting 127.0.0.1:<<<__mysql_sandbox_port1>>> to a PRIMARY...

* Updating metadata...

127.0.0.1:<<<__mysql_sandbox_port1>>> was force-promoted to PRIMARY.
NOTE: Former PRIMARY 127.0.0.1:<<<__mysql_sandbox_port3>>> is now invalidated and must be removed from the replicaset.
* Updating source of remaining SECONDARY instances
** Changing replication source of 127.0.0.1:<<<__mysql_sandbox_port2>>> to 127.0.0.1:<<<__mysql_sandbox_port1>>>

Failover finished successfully.



#@<OUT> list_routers
{
    "replicaSetName": "myrs",
    "routers": {
        "routerhost1::system": {
            "hostname": "routerhost1",
            "lastCheckIn": "2019-01-01 11:22:33",
            "roPort": null,
            "roXPort": null,
            "rwPort": null,
            "rwXPort": null,
            "upgradeRequired": true,
            "version": "8.0.18"
        },
        "routerhost2::system": {
            "hostname": "routerhost2",
            "lastCheckIn": "2019-01-01 11:22:33",
            "roPort": null,
            "roXPort": null,
            "rwPort": null,
            "rwXPort": null,
            "upgradeRequired": true,
            "version": "8.0.18"
        }
    }
}

#@<OUT> remove_router_metadata
{
    "replicaSetName": "myrs",
    "routers": {
        "routerhost2::system": {
            "hostname": "routerhost2",
            "lastCheckIn": "2019-01-01 11:22:33",
            "roPort": null,
            "roXPort": null,
            "rwPort": null,
            "rwXPort": null,
            "upgradeRequired": true,
            "version": "8.0.18"
        }
    }
}

#@<OUT> create_replica_set(adopt)
A new replicaset with the topology visible from '127.0.0.1:<<<__mysql_sandbox_port1>>>' will be created.

* Scanning replication topology...
** Scanning state of instance 127.0.0.1:[[*]]
** Scanning state of instance 127.0.0.1:[[*]]

* Discovering async replication topology starting with 127.0.0.1:<<<__mysql_sandbox_port1>>>
Discovered topology:
- 127.0.0.1:<<<__mysql_sandbox_port1>>>: uuid=[[*]] read_only=no
- 127.0.0.1:<<<__mysql_sandbox_port2>>>: uuid=[[*]] read_only=yes
    - replicates from 127.0.0.1:<<<__mysql_sandbox_port1>>>
?{VER(<8.0.23)}
	source="127.0.0.1:<<<__mysql_sandbox_port1>>>" channel= status=ON receiver=ON applier=ON
?{}
?{VER(>=8.0.23)}
	source="127.0.0.1:<<<__mysql_sandbox_port1>>>" channel= status=ON receiver=ON coordinator=ON applier0=ON applier1=ON applier2=ON applier3=ON
?{}

* Checking configuration of discovered instances...

This instance reports its own address as 127.0.0.1:<<<__mysql_sandbox_port1>>>
127.0.0.1:<<<__mysql_sandbox_port1>>>: Instance configuration is suitable.

This instance reports its own address as 127.0.0.1:<<<__mysql_sandbox_port2>>>
127.0.0.1:<<<__mysql_sandbox_port2>>>: Instance configuration is suitable.

* Checking discovered replication topology...
127.0.0.1:<<<__mysql_sandbox_port1>>> detected as the PRIMARY.
Replication state of 127.0.0.1:<<<__mysql_sandbox_port2>>> is OK.

Validations completed successfully.

* Updating metadata...

ReplicaSet object successfully created for 127.0.0.1:<<<__mysql_sandbox_port1>>>.
Use rs.add_instance() to add more asynchronously replicated instances to this replicaset and rs.status() to check its status.
