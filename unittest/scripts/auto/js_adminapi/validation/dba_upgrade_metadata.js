//@<OUT> Creates the sample cluster
A new InnoDB Cluster will be created on instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'.

Validating instance configuration at localhost:<<<__mysql_sandbox_port1>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

Instance configuration is suitable.
NOTE: Group Replication will communicate with other members using '<<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>'. Use the localAddress option to override.

?{VER(<8.0.27)}
NOTE: The 'localAddress' "<<<hostname>>>" is [[*]], which is compatible with the Group Replication automatically generated list of IPs.
See https://dev.mysql.com/doc/refman/en/group-replication-ip-address-permissions.html for more details.
NOTE: When adding more instances to the Cluster, be aware that the subnet masks dictate whether the instance's address is automatically added to the allowlist or not. Please specify the 'ipAllowlist' accordingly if needed.
?{}
* Checking connectivity and SSL configuration...

Creating InnoDB Cluster 'sample' on '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'...

Adding Seed Instance...
Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

//@<OUT> upgradeMetadata, dryRrun upgrade required
Metadata Schema Upgrade

The topology you are connected to is using an outdated metadata schema version 1.0.1 and needs to be upgraded to 2.3.0.

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
[[*]]
There is 1 Router to be upgraded in order to perform the Metadata schema upgrade.

//@<OUT> Upgrades the metadata, no registered routers
Metadata Schema Upgrade

The topology you are connected to is using an outdated metadata schema version 1.0.1 and needs to be upgraded to 2.3.0.

Without doing this upgrade, no AdminAPI calls except read only operations will be allowed.

NOTE: After the upgrade, this InnoDB Cluster/ReplicaSet can no longer be managed using older versions of MySQL Shell.

The grants for the MySQL Router accounts that were created automatically when bootstrapping need to be updated to match the new metadata version's requirements.
NOTE: No automatically created Router accounts were found.
WARNING: If MySQL Routers have been bootstrapped using custom accounts, their grants can not be updated during the metadata upgrade, they have to be updated using the setupRouterAccount function.
For additional information use: \? setupRouterAccount

Upgrading metadata at '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' from version 1.0.1 to version 2.3.0.
Upgrade will require 4 steps
Creating backup of the metadata schema...
Step 1 of 4: upgrading from 1.0.1 to 2.0.0...
Step 2 of 4: upgrading from 2.0.0 to 2.1.0...
Step 3 of 4: upgrading from 2.1.0 to 2.2.0...
Step 4 of 4: upgrading from 2.2.0 to 2.3.0...
Removing metadata backup...
Upgrade process successfully finished, metadata schema is now on version 2.3.0

//@<OUT> Upgrades the metadata, up to date
NOTE: Installed metadata at '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is up to date (version 2.3.0).

//@<OUT> Upgrades the metadata from a slave instance
Metadata Schema Upgrade

The topology you are connected to is using an outdated metadata schema version 1.0.1 and needs to be upgraded to 2.3.0.

Without doing this upgrade, no AdminAPI calls except read only operations will be allowed.

NOTE: After the upgrade, this InnoDB Cluster/ReplicaSet can no longer be managed using older versions of MySQL Shell.

The grants for the MySQL Router accounts that were created automatically when bootstrapping need to be updated to match the new metadata version's requirements.
NOTE: No automatically created Router accounts were found.
WARNING: If MySQL Routers have been bootstrapped using custom accounts, their grants can not be updated during the metadata upgrade, they have to be updated using the setupRouterAccount function.
For additional information use: \? setupRouterAccount

Upgrading metadata at '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' from version 1.0.1 to version 2.3.0.
Upgrade will require 4 steps
Creating backup of the metadata schema...
Step 1 of 4: upgrading from 1.0.1 to 2.0.0...
Step 2 of 4: upgrading from 2.0.0 to 2.1.0...
Step 3 of 4: upgrading from 2.1.0 to 2.2.0...
Step 4 of 4: upgrading from 2.2.0 to 2.3.0...
Removing metadata backup...
Upgrade process successfully finished, metadata schema is now on version 2.3.0


//@<OUT> WL13386-TSFR1_5 Upgrades the metadata, upgrade done by unregistering 10 routers and no router accounts
Metadata Schema Upgrade

The topology you are connected to is using an outdated metadata schema version 1.0.1 and needs to be upgraded to 2.3.0.

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
| <<<real_hostname>>>::eighth[[*]]| <= 8.0.18 | NULL          | NULL     | NULL     |
| <<<real_hostname>>>::fifth[[*]]| <= 8.0.18 | NULL          | NULL     | NULL     |
| <<<real_hostname>>>::fourth[[*]]| <= 8.0.18 | NULL          | NULL     | NULL     |
| <<<real_hostname>>>::nineth[[*]]| <= 8.0.18 | NULL          | NULL     | NULL     |
| <<<real_hostname>>>::second[[*]]| <= 8.0.18 | NULL          | NULL     | NULL     |
| <<<real_hostname>>>::seventh[[*]]| <= 8.0.18 | NULL          | NULL     | NULL     |
| <<<real_hostname>>>::sixth[[*]]| <= 8.0.18 | NULL          | NULL     | NULL     |
| <<<real_hostname>>>::tenth[[*]]| <= 8.0.18 | NULL          | NULL     | NULL     |
| <<<real_hostname>>>::third[[*]]| <= 8.0.18 | NULL          | NULL     | NULL     |
[[*]]
There are 10 Routers to upgrade. Please upgrade them and select Continue once they are restarted.

NOTE: If Router's Metadata Cache TTL is high Router's own update in the Metadata will take longer. Re-check for outdated Routers if that's the case or alternatively restart the Router.

  1) Re-check for outdated Routers and continue with the metadata upgrade.
  2) Unregister the remaining Routers.
  3) Abort the operation.
  4) Help

Please select an option: Unregistering a Router implies it will not be used in the Cluster, do you want to continue? [y/N]: Upgrading metadata at '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' from version 1.0.1 to version 2.3.0.
Upgrade will require 4 steps
Creating backup of the metadata schema...
Step 1 of 4: upgrading from 1.0.1 to 2.0.0...
Step 2 of 4: upgrading from 2.0.0 to 2.1.0...
Step 3 of 4: upgrading from 2.1.0 to 2.2.0...
Step 4 of 4: upgrading from 2.2.0 to 2.3.0...
Removing metadata backup...
Upgrade process successfully finished, metadata schema is now on version 2.3.0

//@<OUT> WL13386-TSFR1_5 Upgrades the metadata, upgrade done by unregistering more than 10 routers with router accounts
Metadata Schema Upgrade

The topology you are connected to is using an outdated metadata schema version 1.0.1 and needs to be upgraded to 2.3.0.

Without doing this upgrade, no AdminAPI calls except read only operations will be allowed.

NOTE: After the upgrade, this InnoDB Cluster/ReplicaSet can no longer be managed using older versions of MySQL Shell.

The grants for the MySQL Router accounts that were created automatically when bootstrapping need to be updated to match the new metadata version's requirements.
Updating Router accounts...
NOTE: 2 Router accounts have been updated.
WARNING: If MySQL Routers have been bootstrapped using custom accounts, their grants can not be updated during the metadata upgrade, they have to be updated using the setupRouterAccount function.
For additional information use: \? setupRouterAccount

An upgrade of all cluster router instances is required. All router installations should be updated first before doing the actual metadata upgrade.

[[*]]
| Instance[[*]]| Version   | Last Check-in | R/O Port | R/W Port |
[[*]]
| <<<real_hostname>>>::[[*]]| <= 8.0.18 | NULL          | NULL     | NULL     |
| <<<real_hostname>>>::eighth[[*]]| <= 8.0.18 | NULL          | NULL     | NULL     |
| <<<real_hostname>>>::eleventh[[*]]| <= 8.0.18 | NULL          | NULL     | NULL     |
| <<<real_hostname>>>::fifth[[*]]| <= 8.0.18 | NULL          | NULL     | NULL     |
| <<<real_hostname>>>::fourth[[*]]| <= 8.0.18 | NULL          | NULL     | NULL     |
| <<<real_hostname>>>::nineth[[*]]| <= 8.0.18 | NULL          | NULL     | NULL     |
| <<<real_hostname>>>::second[[*]]| <= 8.0.18 | NULL          | NULL     | NULL     |
| <<<real_hostname>>>::seventh[[*]]| <= 8.0.18 | NULL          | NULL     | NULL     |
| <<<real_hostname>>>::sixth[[*]]| <= 8.0.18 | NULL          | NULL     | NULL     |
| ...[[*]]| ...       | ...           | ...      | ...      |
[[*]]
There are 11 Routers to upgrade. Please upgrade them and select Continue once they are restarted.

NOTE: If Router's Metadata Cache TTL is high Router's own update in the Metadata will take longer. Re-check for outdated Routers if that's the case or alternatively restart the Router.

  1) Re-check for outdated Routers and continue with the metadata upgrade.
  2) Unregister the remaining Routers.
  3) Abort the operation.
  4) Help

Please select an option: The metadata upgrade has been aborted.

//@<OUT> Second upgrade attempt should succeed
Metadata Schema Upgrade

The topology you are connected to is using an outdated metadata schema version 1.0.1 and needs to be upgraded to 2.3.0.

Without doing this upgrade, no AdminAPI calls except read only operations will be allowed.

NOTE: After the upgrade, this InnoDB Cluster/ReplicaSet can no longer be managed using older versions of MySQL Shell.

The grants for the MySQL Router accounts that were created automatically when bootstrapping need to be updated to match the new metadata version's requirements.
Updating Router accounts...
NOTE: 2 Router accounts have been updated.
WARNING: If MySQL Routers have been bootstrapped using custom accounts, their grants can not be updated during the metadata upgrade, they have to be updated using the setupRouterAccount function.
For additional information use: \? setupRouterAccount

An upgrade of all cluster router instances is required. All router installations should be updated first before doing the actual metadata upgrade.

[[*]]
| Instance[[*]]| Version   | Last Check-in | R/O Port | R/W Port |
[[*]]
| <<<real_hostname>>>::[[*]]| <= 8.0.18 | NULL          | NULL     | NULL     |
| <<<real_hostname>>>::eighth[[*]]| <= 8.0.18 | NULL          | NULL     | NULL     |
| <<<real_hostname>>>::eleventh[[*]]| <= 8.0.18 | NULL          | NULL     | NULL     |
| <<<real_hostname>>>::fifth[[*]]| <= 8.0.18 | NULL          | NULL     | NULL     |
| <<<real_hostname>>>::fourth[[*]]| <= 8.0.18 | NULL          | NULL     | NULL     |
| <<<real_hostname>>>::nineth[[*]]| <= 8.0.18 | NULL          | NULL     | NULL     |
| <<<real_hostname>>>::second[[*]]| <= 8.0.18 | NULL          | NULL     | NULL     |
| <<<real_hostname>>>::seventh[[*]]| <= 8.0.18 | NULL          | NULL     | NULL     |
| <<<real_hostname>>>::sixth[[*]]| <= 8.0.18 | NULL          | NULL     | NULL     |
| ...[[*]]| ...       | ...           | ...      | ...      |
[[*]]
There are 11 Routers to upgrade. Please upgrade them and select Continue once they are restarted.

NOTE: If Router's Metadata Cache TTL is high Router's own update in the Metadata will take longer. Re-check for outdated Routers if that's the case or alternatively restart the Router.

  1) Re-check for outdated Routers and continue with the metadata upgrade.
  2) Unregister the remaining Routers.
  3) Abort the operation.
  4) Help

Please select an option: Unregistering a Router implies it will not be used in the Cluster, do you want to continue? [y/N]: Upgrading metadata at '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' from version 1.0.1 to version 2.3.0.
Upgrade will require 4 steps
Creating backup of the metadata schema...
Step 1 of 4: upgrading from 1.0.1 to 2.0.0...
Step 2 of 4: upgrading from 2.0.0 to 2.1.0...
Step 3 of 4: upgrading from 2.1.0 to 2.2.0...
Step 4 of 4: upgrading from 2.2.0 to 2.3.0...
Removing metadata backup...
Upgrade process successfully finished, metadata schema is now on version 2.3.0

//@<OUT> Test Migration from 1.0.1 to 2.3.0
Metadata Schema Upgrade

The topology you are connected to is using an outdated metadata schema version 1.0.1 and needs to be upgraded to 2.3.0.

Without doing this upgrade, no AdminAPI calls except read only operations will be allowed.

NOTE: After the upgrade, this InnoDB Cluster/ReplicaSet can no longer be managed using older versions of MySQL Shell.

The grants for the MySQL Router accounts that were created automatically when bootstrapping need to be updated to match the new metadata version's requirements.
Updating Router accounts...
NOTE: 3 Router accounts have been updated.
WARNING: If MySQL Routers have been bootstrapped using custom accounts, their grants can not be updated during the metadata upgrade, they have to be updated using the setupRouterAccount function.
For additional information use: \? setupRouterAccount

An upgrade of all cluster router instances is required. All router installations should be updated first before doing the actual metadata upgrade.

[[*]]
| Instance[[*]]| Version   | Last Check-in | R/O Port | R/W Port |
[[*]]
| <<<real_hostname>>>::[[*]]| <= 8.0.18 | NULL          | NULL     | NULL     |
[[*]]
There is 1 Router to upgrade. Please upgrade it and select Continue once it is restarted.

NOTE: If Router's Metadata Cache TTL is high Router's own update in the Metadata will take longer. Re-check for outdated Routers if that's the case or alternatively restart the Router.

  1) Re-check for outdated Routers and continue with the metadata upgrade.
  2) Unregister the remaining Routers.
  3) Abort the operation.
  4) Help

Please select an option: Unregistering a Router implies it will not be used in the Cluster, do you want to continue? [y/N]: Upgrading metadata at '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' from version 1.0.1 to version 2.3.0.
Upgrade will require 4 steps
Creating backup of the metadata schema...
Step 1 of 4: upgrading from 1.0.1 to 2.0.0...
Step 2 of 4: upgrading from 2.0.0 to 2.1.0...
Step 3 of 4: upgrading from 2.1.0 to 2.2.0...
Step 4 of 4: upgrading from 2.2.0 to 2.3.0...
Removing metadata backup...
Upgrade process successfully finished, metadata schema is now on version 2.3.0
