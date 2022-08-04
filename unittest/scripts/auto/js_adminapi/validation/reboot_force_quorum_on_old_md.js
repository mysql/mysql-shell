//@<ERR> Testing upgrade metadata on total lost
Dba.upgradeMetadata: This function is not available through a session to a standalone instance (metadata exists, instance belongs to that metadata) (MYSQLSH 51314)

//@<OUT> Testing rebootClusterFromCompleteOutage
WARNING: The cluster will be rebooted as configured on the metadata, however, no change operations can be executed because the installed metadata version 1.0.1 is lower than the version required by Shell which is version 2.1.0. Upgrade the metadata to remove this restriction. See \? dba.upgradeMetadata for additional details.
Restoring the Cluster 'sample' from complete outage...
${*}
<<<hostname>>>:<<<__mysql_sandbox_port1>>> was restored.
${*}
Rejoining instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' to cluster 'sample'...
${*}
{
    "clusterName": "sample",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
${*}
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "memberRole": "PRIMARY",
                "mode": "R/W",
${*}
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
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

The topology you are connected to is using an outdated metadata schema version 1.0.1 and needs to be upgraded to 2.1.0.

Without doing this upgrade, no AdminAPI calls except read only operations will be allowed.

NOTE: After the upgrade, this InnoDB Cluster/ReplicaSet can no longer be managed using older versions of MySQL Shell.

The grants for the MySQL Router accounts that were created automatically when bootstrapping need to be updated to match the new metadata version's requirements.
NOTE: No automatically created Router accounts were found.
WARNING: If MySQL Routers have been bootstrapped using custom accounts, their grants can not be updated during the metadata upgrade, they have to be updated using the setupRouterAccount function.
For additional information use: \? setupRouterAccount

Upgrading metadata at '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' from version 1.0.1 to version 2.1.0.
Upgrade will require 2 steps
Creating backup of the metadata schema...
Step 1 of 2: upgrading from 1.0.1 to 2.0.0...
Step 2 of 2: upgrading from 2.0.0 to 2.1.0...
Removing metadata backup...
Upgrade process successfully finished, metadata schema is now on version 2.1.0

//@<ERR> Testing upgrade metadata with no quorum
Dba.upgradeMetadata: There is no quorum to perform the operation (MYSQLSH 51011)

//@<OUT> Getting cluster with no quorum
WARNING: No cluster change operations can be executed because the installed metadata version 1.0.1 is lower than the version required by Shell which is version 2.1.0. Upgrade the metadata to remove this restriction. See \? dba.upgradeMetadata for additional details.
WARNING: Cluster has no quorum and cannot process write transactions: Group has no quorum

//@<OUT> Getting cluster with quorum
WARNING: No cluster change operations can be executed because the installed metadata version 1.0.1 is lower than the version required by Shell which is version 2.1.0. Upgrade the metadata to remove this restriction. See \? dba.upgradeMetadata for additional details.

//@<ERR> Metadata continues failing...
Dba.upgradeMetadata: This operation requires all the cluster members to be ONLINE (RuntimeError)

