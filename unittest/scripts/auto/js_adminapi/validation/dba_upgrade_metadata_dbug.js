//@<OUT> Upgrades the metadata, interactive options simulating unregister (no upgrade performed)
InnoDB Cluster Metadata Upgrade

The cluster you are connected to is using an outdated metadata schema version 1.0.1 and needs to be upgraded to 2.1.0.

Without doing this upgrade, no AdminAPI calls except read only operations will be allowed.

NOTE: After the upgrade, this InnoDB Cluster/ReplicaSet can no longer be managed using older versions of MySQL Shell.

The grants for the MySQL Router accounts that were created automatically when bootstrapping need to be updated to match the new metadata version's requirements.
NOTE: No automatically created Router accounts were found.
WARNING: If MySQL Routers have been bootstrapped using custom accounts, their grants can not be updated during the metadata upgrade, they have to be updated using the setupRouterAccount function.
For additional information use: \? setupRouterAccount

An upgrade of all cluster router instances is required. All router installations should be updated first before doing the actual metadata upgrade.

[[*]]
| Instance[[*]]| Version   | Last Check-in | R/O Port | R/W Port |
[[*]]
| <<<real_hostname>>>::[[*]]| <= 8.0.18 | NULL          | NULL     | NULL     |
| <<<real_hostname>>>::test-router[[*]]| <= 8.0.18 | NULL          | NULL     | NULL     |
[[*]]
There are 2 Routers to upgrade. Please upgrade them and select Continue once they are restarted.

  1) Re-check for outdated Routers and continue with the metadata upgrade.
  2) Unregister the remaining Routers.
  3) Abort the operation.
  4) Help

Please select an option: [[*]]
| Instance[[*]]| Version   | Last Check-in | R/O Port | R/W Port |
[[*]]
| <<<real_hostname>>>::[[*]]| <= 8.0.18 | NULL          | NULL     | NULL     |
[[*]]
There is 1 Router to upgrade. Please upgrade it and select Continue once it is restarted.

  1) Re-check for outdated Routers and continue with the metadata upgrade.
  2) Unregister the remaining Routers.
  3) Abort the operation.
  4) Help

Please select an option: Unregistering a Router implies it will not be used in the Cluster, do you want to continue? [y/N]: [[*]]
| Instance[[*]]| Version   | Last Check-in | R/O Port | R/W Port |
[[*]]
| <<<real_hostname>>>::[[*]]| <= 8.0.18 | NULL          | NULL     | NULL     |
[[*]]
There is 1 Router to upgrade. Please upgrade it and select Continue once it is restarted.

  1) Re-check for outdated Routers and continue with the metadata upgrade.
  2) Unregister the remaining Routers.
  3) Abort the operation.
  4) Help

Please select an option: To perform a rolling upgrade of the InnoDB Cluster/ReplicaSet metadata, execute the following steps:

1 - Upgrade the Shell to the latest version
2 - Execute dba.upgradeMetadata() (the current step)
3 - Upgrade MySQL Router instances to the latest version
4 - Continue with the metadata upgrade once all Router instances are upgraded or accounted for

If the following Router instances no longer exist, select Unregister to delete their metadata.
[[*]]
| Instance[[*]]| Version   | Last Check-in | R/O Port | R/W Port |
[[*]]
| <<<real_hostname>>>::[[*]]| <= 8.0.18 | NULL          | NULL     | NULL     |
[[*]]
There is 1 Router to upgrade. Please upgrade it and select Continue once it is restarted.

  1) Re-check for outdated Routers and continue with the metadata upgrade.
  2) Unregister the remaining Routers.
  3) Abort the operation.
  4) Help

Please select an option: The metadata upgrade has been aborted.

//@<OUT> Upgrades the metadata, interactive options simulating upgrade (no upgrade performed)
InnoDB Cluster Metadata Upgrade

The cluster you are connected to is using an outdated metadata schema version 1.0.1 and needs to be upgraded to 2.1.0.

Without doing this upgrade, no AdminAPI calls except read only operations will be allowed.

NOTE: After the upgrade, this InnoDB Cluster/ReplicaSet can no longer be managed using older versions of MySQL Shell.

The grants for the MySQL Router accounts that were created automatically when bootstrapping need to be updated to match the new metadata version's requirements.
NOTE: No automatically created Router accounts were found.
WARNING: If MySQL Routers have been bootstrapped using custom accounts, their grants can not be updated during the metadata upgrade, they have to be updated using the setupRouterAccount function.
For additional information use: \? setupRouterAccount

An upgrade of all cluster router instances is required. All router installations should be updated first before doing the actual metadata upgrade.

[[*]]
| Instance[[*]]| Version   | Last Check-in | R/O Port | R/W Port |
[[*]]
| <<<real_hostname>>>::[[*]]| <= 8.0.18 | NULL          | NULL     | NULL     |
| <<<real_hostname>>>::second[[*]]| <= 8.0.18 | NULL          | NULL     | NULL     |
[[*]]
There are 2 Routers to upgrade. Please upgrade them and select Continue once they are restarted.

  1) Re-check for outdated Routers and continue with the metadata upgrade.
  2) Unregister the remaining Routers.
  3) Abort the operation.
  4) Help

Please select an option: [[*]]
| Instance[[*]]| Version   | Last Check-in | R/O Port | R/W Port |
[[*]]
| <<<real_hostname>>>::[[*]]| <= 8.0.18 | NULL          | NULL     | NULL     |
[[*]]
There is 1 Router to upgrade. Please upgrade it and select Continue once it is restarted.

  1) Re-check for outdated Routers and continue with the metadata upgrade.
  2) Unregister the remaining Routers.
  3) Abort the operation.
  4) Help

Please select an option: Unregistering a Router implies it will not be used in the Cluster, do you want to continue? [y/N]: [[*]]
| Instance[[*]]| Version   | Last Check-in | R/O Port | R/W Port |
[[*]]
| <<<real_hostname>>>::[[*]]| <= 8.0.18 | NULL          | NULL     | NULL     |
[[*]]
There is 1 Router to upgrade. Please upgrade it and select Continue once it is restarted.

  1) Re-check for outdated Routers and continue with the metadata upgrade.
  2) Unregister the remaining Routers.
  3) Abort the operation.
  4) Help

Please select an option: To perform a rolling upgrade of the InnoDB Cluster/ReplicaSet metadata, execute the following steps:

1 - Upgrade the Shell to the latest version
2 - Execute dba.upgradeMetadata() (the current step)
3 - Upgrade MySQL Router instances to the latest version
4 - Continue with the metadata upgrade once all Router instances are upgraded or accounted for

If the following Router instances no longer exist, select Unregister to delete their metadata.
[[*]]
| Instance[[*]]| Version   | Last Check-in | R/O Port | R/W Port |
[[*]]
| <<<real_hostname>>>::[[*]]| <= 8.0.18 | NULL          | NULL     | NULL     |
[[*]]
There is 1 Router to upgrade. Please upgrade it and select Continue once it is restarted.

  1) Re-check for outdated Routers and continue with the metadata upgrade.
  2) Unregister the remaining Routers.
  3) Abort the operation.
  4) Help

Please select an option: The metadata upgrade has been aborted.
