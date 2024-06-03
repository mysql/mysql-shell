//@<ERR> Testing upgrade metadata on total lost
Dba.upgradeMetadata: This function is not available through a session to a standalone instance (metadata exists, instance belongs to that metadata) (MYSQLSH 51314)

//@<OUT> Testing rebootClusterFromCompleteOutage
WARNING: The installed metadata version '1.0.1' is lower than the version supported by Shell, version '2.2.0'. It is recommended to upgrade the Metadata. See \? dba.upgradeMetadata for additional details.
Restoring the Cluster 'sample' from complete outage...
${*}
<<<hostname>>>:<<<__mysql_sandbox_port1>>> was restored.
${*}
Rejoining instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' to cluster 'sample'...
${*}
{
    "clusterName": "sample",
    "defaultReplicaSet": {
        "clusterErrors": [
?{VER(>=8.0.27) && VER(<8.3.0)}
            "WARNING: The Cluster's group_replication_view_change_uuid is not stored in the Metadata. Please use <Cluster>.rescan() to update the metadata.",
?{}
            "WARNING: Cluster's transaction size limit is not registered in the metadata. Use cluster.rescan() to update the metadata."
        ],
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
${*}
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
${*}
                "memberRole": "PRIMARY",
                "mode": "R/W",
${*}
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
${*}
                "memberRole": "SECONDARY",
                "mode": "R/O",
${*}
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@<OUT> Testing upgrade metadata on rebooted cluster
Metadata Schema Upgrade

The topology you are connected to is using an outdated metadata schema version 1.0.1 and needs to be upgraded to 2.2.0.

Without doing this upgrade, no AdminAPI calls except read only operations will be allowed.

NOTE: After the upgrade, this InnoDB Cluster/ReplicaSet can no longer be managed using older versions of MySQL Shell.

The grants for the MySQL Router accounts that were created automatically when bootstrapping need to be updated to match the new metadata version's requirements.
NOTE: No automatically created Router accounts were found.
WARNING: If MySQL Routers have been bootstrapped using custom accounts, their grants can not be updated during the metadata upgrade, they have to be updated using the setupRouterAccount function.
For additional information use: \? setupRouterAccount

Upgrading metadata at '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' from version 1.0.1 to version 2.2.0.
Upgrade will require 3 steps
Creating backup of the metadata schema...
Step 1 of 3: upgrading from 1.0.1 to 2.0.0...
Step 2 of 3: upgrading from 2.0.0 to 2.1.0...
Step 3 of 3: upgrading from 2.1.0 to 2.2.0...
Removing metadata backup...
Upgrade process successfully finished, metadata schema is now on version 2.2.0

//@<ERR> Testing upgrade metadata with no quorum
Dba.upgradeMetadata: There is no quorum to perform the operation (MYSQLSH 51011)

//@<OUT> Getting cluster with quorum
WARNING: The installed metadata version '1.0.1' is lower than the version supported by Shell, version '2.2.0'. It is recommended to upgrade the Metadata. See \? dba.upgradeMetadata for additional details.

//@<ERR> Metadata continues failing...
Dba.upgradeMetadata: There is no quorum to perform the operation (MYSQLSH 51011)

