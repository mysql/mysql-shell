//@#configureReplicaSetInstance + create admin user
|Cluster admin user 'admin'@'%' created.|
|The instance '<<<__address1>>>' was configured to be used in an InnoDB ReplicaSet.|
|Cluster admin user 'admin'@'%' created.|
|The instance '<<<__address2>>>' is already ready to be used in an InnoDB ReplicaSet.|
|Cluster admin user 'admin'@'%' created.|
|The instance '<<<__address3>>>' is already ready to be used in an InnoDB ReplicaSet.|

//@<OUT> createReplicaSet
A new replicaset with instance '<<<__address1>>>' will be created.

* Checking MySQL instance at <<<__address1>>>

This instance reports its own address as 127.0.0.1:<<<__mysql_sandbox_port1>>>
<<<__address1>>>: Instance configuration is suitable.

* Updating metadata...

ReplicaSet object successfully created for <<<__address1>>>.
Use rs.addInstance() to add more asynchronously replicated instances to this replicaset and rs.status() to check its status.

<ReplicaSet:myrs>

//@<OUT> status
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

//@# disconnect
||

//@<OUT> getReplicaSet
You are connected to a member of replicaset 'myrs'.
<ReplicaSet:myrs>

//@<OUT> addInstance
Adding instance to the replicaset...

* Performing validation checks

This instance reports its own address as 127.0.0.1:<<<__mysql_sandbox_port2>>>
<<<__address2>>>: Instance configuration is suitable.
** Checking transaction state of the instance...

NOTE: The target instance '<<<__address2>>>' has not been pre-provisioned (GTID set is empty). The Shell is unable to decide whether replication can completely recover its state.

Incremental state recovery selected through the recoveryMethod option
* Checking async replication topology...


* Updating topology
** Configuring <<<__address2>>> to replicate from 127.0.0.1:<<<__mysql_sandbox_port1>>>
** Waiting for new instance to synchronize with PRIMARY...

The instance '<<<__address2>>>' was added to the replicaset and is replicating from 127.0.0.1:<<<__mysql_sandbox_port1>>>.

Adding instance to the replicaset...

* Performing validation checks

This instance reports its own address as 127.0.0.1:<<<__mysql_sandbox_port3>>>
<<<__address3>>>: Instance configuration is suitable.
** Checking transaction state of the instance...

NOTE: The target instance '<<<__address3>>>' has not been pre-provisioned (GTID set is empty). The Shell is unable to decide whether replication can completely recover its state.

Incremental state recovery selected through the recoveryMethod option
* Checking async replication topology...


* Updating topology
** Configuring <<<__address3>>> to replicate from 127.0.0.1:<<<__mysql_sandbox_port1>>>
** Waiting for new instance to synchronize with PRIMARY...

The instance '<<<__address3>>>' was added to the replicaset and is replicating from 127.0.0.1:<<<__mysql_sandbox_port1>>>.

//@<OUT> removeInstance
The instance '127.0.0.1:<<<__mysql_sandbox_port2>>>' was removed from the replicaset.

Adding instance to the replicaset...

* Performing validation checks

This instance reports its own address as 127.0.0.1:<<<__mysql_sandbox_port2>>>
<<<__address2>>>: Instance configuration is suitable.
** Checking transaction state of the instance...
Incremental state recovery selected through the recoveryMethod option
* Checking async replication topology...


* Updating topology
** Configuring <<<__address2>>> to replicate from 127.0.0.1:<<<__mysql_sandbox_port1>>>
** Waiting for new instance to synchronize with PRIMARY...

The instance '<<<__address2>>>' was added to the replicaset and is replicating from 127.0.0.1:<<<__mysql_sandbox_port1>>>.

//@<OUT> setPrimaryInstance
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

//@# forcePrimaryInstance (prepare)
|ERROR: Unable to connect to the PRIMARY of the replicaset myrs: 127.0.0.1:<<<__mysql_sandbox_port3>>>: Can't connect to MySQL server on '127.0.0.1'|
|WARNING: MYSQLSH 51118: PRIMARY instance is unavailable|
|        "status": "UNAVAILABLE", |

//@<OUT> forcePrimaryInstance
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
NOTE: 127.0.0.1:<<<__mysql_sandbox_port1>>> has status [[*]]
* Checking transaction set status
* Promoting 127.0.0.1:<<<__mysql_sandbox_port1>>> to a PRIMARY...

* Updating metadata...

127.0.0.1:<<<__mysql_sandbox_port1>>> was force-promoted to PRIMARY.
NOTE: Former PRIMARY 127.0.0.1:<<<__mysql_sandbox_port3>>> is now invalidated and must be removed from the replicaset.
* Updating source of remaining SECONDARY instances
** Changing replication source of 127.0.0.1:<<<__mysql_sandbox_port2>>> to 127.0.0.1:<<<__mysql_sandbox_port1>>>

Failover finished successfully.



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

//@<OUT> createReplicaSet(adopt)
A new replicaset with the topology visible from '<<<__address1>>>' will be created.

* Scanning replication topology...
** Scanning state of instance 127.0.0.1:[[*]]
** Scanning state of instance 127.0.0.1:[[*]]

* Discovering async replication topology starting with <<<__address1>>>
Discovered topology:
- 127.0.0.1:<<<__mysql_sandbox_port1>>>: uuid=[[*]] read_only=no
- 127.0.0.1:<<<__mysql_sandbox_port2>>>: uuid=[[*]] read_only=yes
    - replicates from 127.0.0.1:<<<__mysql_sandbox_port1>>>
	source="127.0.0.1:<<<__mysql_sandbox_port1>>>" channel= status=ON receiver=ON applier=ON

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

ReplicaSet object successfully created for <<<__address1>>>.
Use rs.addInstance() to add more asynchronously replicated instances to this replicaset and rs.status() to check its status.

<ReplicaSet:adopted>
{
    "replicaSet": {
        "name": "adopted", 
        "primary": "127.0.0.1:<<<__mysql_sandbox_port1>>>", 
        "status": "AVAILABLE", 
        "statusText": "All instances available.", 
        "topology": {
            "127.0.0.1:<<<__mysql_sandbox_port1>>>": {
                "address": "127.0.0.1:<<<__mysql_sandbox_port1>>>", 
                "instanceRole": "PRIMARY", 
                "mode": "R/W", 
                "status": "ONLINE"
            }, 
            "127.0.0.1:<<<__mysql_sandbox_port2>>>": {
                "address": "127.0.0.1:<<<__mysql_sandbox_port2>>>", 
                "instanceRole": "SECONDARY", 
                "mode": "R/O", 
                "replication": {
                    "applierStatus": "APPLIED_ALL", 
                    "applierThreadState": "Slave has read all relay log; waiting for more updates", 
                    "receiverStatus": "ON", 
                    "receiverThreadState": "Waiting for master to send event", 
                    "replicationLag": null
                }, 
                "status": "ONLINE"
            }
        }, 
        "type": "ASYNC"
    }
}
