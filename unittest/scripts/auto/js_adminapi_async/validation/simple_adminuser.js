//@#configureReplicaSetInstance + create admin user
|Cluster admin user 'admin'@'%' created.|
|The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB ReplicaSet.|
|Cluster admin user 'admin'@'%' created.|
|The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' is already ready to be used in an InnoDB ReplicaSet.|
|Cluster admin user 'admin'@'%' created.|
|The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>' is already ready to be used in an InnoDB ReplicaSet.|

//@<OUT> createReplicaSet
A new replicaset with instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>' will be created.

* Checking MySQL instance at <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>

This instance reports its own address as <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>
<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>: Instance configuration is suitable.

* Updating metadata...

ReplicaSet object successfully created for <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>.
Use rs.addInstance() to add more asynchronously replicated instances to this replicaset and rs.status() to check its status.

<ReplicaSet:myrs>

//@<OUT> status
{
    "replicaSet": {
        "name": "myrs", 
        "primary": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>", 
        "status": "AVAILABLE", 
        "statusText": "All instances available.", 
        "topology": {
            "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>", 
                "instanceRole": "PRIMARY", 
                "mode": "R/W", 
                "status": "ONLINE"
            }
        }, 
        "type": "ASYNC"
    }
}

//@# disconnect
||

//@<OUT> getReplicaSet
You are connected to a member of replicaset 'myrs'.
<ReplicaSet:myrs>

//@<OUT> addInstance (incremental)
Adding instance to the replicaset...

* Performing validation checks

This instance reports its own address as <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>
<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>: Instance configuration is suitable.

* Checking async replication topology...

* Checking transaction state of the instance...

NOTE: The target instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' has not been pre-provisioned (GTID set is empty). The Shell is unable to decide whether replication can completely recover its state.

Incremental state recovery selected through the recoveryMethod option

* Updating topology
** Changing replication source of <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> to <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>
** Waiting for new instance to synchronize with PRIMARY...


The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' was added to the replicaset and is replicating from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>.

//@# addInstance (clone) {VER(>=8.0.17)}
|Clone based recovery selected through the recoveryMethod option|
|The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>' was added to the replicaset and is replicating from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>.|

//@<OUT> removeInstance
The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' was removed from the replicaset.

Adding instance to the replicaset...

* Performing validation checks

This instance reports its own address as <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>
<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>: Instance configuration is suitable.

* Checking async replication topology...

* Checking transaction state of the instance...

Incremental state recovery selected through the recoveryMethod option

* Updating topology
** Changing replication source of <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> to <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>
** Waiting for new instance to synchronize with PRIMARY...


The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' was added to the replicaset and is replicating from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>.

//@<OUT> setPrimaryInstance
<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> will be promoted to PRIMARY of 'myrs'.
The current PRIMARY is <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>.

* Connecting to replicaset instances
** Connecting to <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>
** Connecting to <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>
** Connecting to <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>
** Connecting to <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>
** Connecting to <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>
** Connecting to <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>

* Performing validation checks
** Checking async replication topology...
** Checking transaction state of the instance...

* Synchronizing transaction backlog at <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>


* Updating metadata

* Acquiring locks in replicaset instances
** Pre-synchronizing SECONDARIES
** Acquiring global lock at PRIMARY
** Acquiring global lock at SECONDARIES

* Updating replication topology
** Changing replication source of <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> to <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>
** Changing replication source of <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> to <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>

<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> was promoted to PRIMARY.

//@# forcePrimaryInstance (prepare)
|WARNING: Unable to connect to the PRIMARY of the replicaset myrs: MYSQLSH 51118: Could not open connection to '<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>': Can't connect to MySQL server on '<<<libmysql_host_description(hostname_ip, __mysql_sandbox_port3)>>>'|
|        "status": "UNAVAILABLE", |

//@<OUT> forcePrimaryInstance
* Connecting to replicaset instances
** Connecting to <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>
** Connecting to <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>

* Waiting for all received transactions to be applied
** Waiting for received transactions to be applied at <<<hostname_ip>>>:[[*]]
** Waiting for received transactions to be applied at <<<hostname_ip>>>:[[*]]
<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> will be promoted to PRIMARY of the replicaset and the former PRIMARY will be invalidated.

* Checking status of last known PRIMARY
NOTE: <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> is UNREACHABLE
* Checking status of promoted instance
NOTE: <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> has status [[*]]
* Checking transaction set status
* Promoting <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> to a PRIMARY...

* Updating metadata...

<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> was force-promoted to PRIMARY.
NOTE: Former PRIMARY <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> is now invalidated and must be removed from the replicaset.
* Updating source of remaining SECONDARY instances
** Changing replication source of <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> to <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>

Failover finished successfully.

//@# rejoinInstance
|The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>' rejoined the replicaset and is replicating from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>.|

//@# rejoinInstance (clone) {VER(>=8.0.17)}
|NOTE: <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> is being cloned from <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>|
|The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>' rejoined the replicaset and is replicating from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>.|


//@<OUT> listRouters
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

//@<OUT> removeRouterMetadata
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

//@# createReplicaSet(adopt)
|A new replicaset with the topology visible from '<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>' will be created.|
|ReplicaSet object successfully created for <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>.|
|{|
|    "replicaSet": {|
|        "name": "adopted", |
|        "primary": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>", |
|        "status": "AVAILABLE", |
|        "statusText": "All instances available.", |
|        "topology": {|
|            "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>": {|
|                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>", |
|                "instanceRole": "PRIMARY", |
|                "mode": "R/W", |
|                "status": "ONLINE"|
|            }, |
|            "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>": {|
|                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>", |
|                "instanceRole": "SECONDARY", |
|                "mode": "R/O", |
|                "status": "ONLINE"|
|            },|
|            "<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>": {|
|                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>", |
|                "instanceRole": "SECONDARY", |
|                "mode": "R/O", |
|                "status": "ONLINE"|
|            }|
|        }, |
|        "type": "ASYNC"|
|    }|
|}|
