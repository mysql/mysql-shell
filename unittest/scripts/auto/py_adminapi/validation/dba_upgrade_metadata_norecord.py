#@<OUT> Upgrades the metadata, no registered routers
Metadata Schema Upgrade

The topology you are connected to is using an outdated metadata schema version 1.0.1 and needs to be upgraded to 2.3.0.

Without doing this upgrade, no AdminAPI calls except read only operations will be allowed.

NOTE: After the upgrade, this InnoDB Cluster/ReplicaSet can no longer be managed using older versions of MySQL Shell.

The grants for the MySQL Router accounts that were created automatically when bootstrapping need to be updated to match the new metadata version's requirements.
NOTE: No automatically created Router accounts were found.
WARNING: If MySQL Routers have been bootstrapped using custom accounts, their grants can not be updated during the metadata upgrade, they have to be updated using the setup_router_account function.
For additional information use: \? setup_router_account

Upgrading metadata at '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' from version 1.0.1 to version 2.3.0.
Upgrade will require 4 steps
Creating backup of the metadata schema...
Step 1 of 4: upgrading from 1.0.1 to 2.0.0...
Step 2 of 4: upgrading from 2.0.0 to 2.1.0...
Step 3 of 4: upgrading from 2.1.0 to 2.2.0...
Step 4 of 4: upgrading from 2.2.0 to 2.3.0...
Removing metadata backup...
Upgrade process successfully finished, metadata schema is now on version 2.3.0

#@<OUT> Upgrades the metadata, up to date
NOTE: Installed metadata at '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is up to date (version 2.3.0).

#@<OUT> Upgrades the metadata, interactive off, error
Metadata Schema Upgrade

The topology you are connected to is using an outdated metadata schema version 1.0.1 and needs to be upgraded to 2.3.0.

Without doing this upgrade, no AdminAPI calls except read only operations will be allowed.

NOTE: After the upgrade, this InnoDB Cluster/ReplicaSet can no longer be managed using older versions of MySQL Shell.

The grants for the MySQL Router accounts that were created automatically when bootstrapping need to be updated to match the new metadata version's requirements.
NOTE: No automatically created Router accounts were found.
WARNING: If MySQL Routers have been bootstrapped using custom accounts, their grants can not be updated during the metadata upgrade, they have to be updated using the setup_router_account function.
For additional information use: \? setup_router_account

An upgrade of all cluster router instances is required. All router installations should be updated first before doing the actual metadata upgrade.

+-------------+-----------+---------------+----------+----------+
| Instance    | Version   | Last Check-in | R/O Port | R/W Port |
+-------------+-----------+---------------+----------+----------+
| rennox-tc:: | <= 8.0.18 | NULL          | NULL     | NULL     |
+-------------+-----------+---------------+----------+----------+

#@<OUT> Upgrades the metadata, upgrade done by unregistering 10 routers and no router accounts
Metadata Schema Upgrade

The topology you are connected to is using an outdated metadata schema version 1.0.1 and needs to be upgraded to 2.3.0.

Without doing this upgrade, no AdminAPI calls except read only operations will be allowed.

NOTE: After the upgrade, this InnoDB Cluster/ReplicaSet can no longer be managed using older versions of MySQL Shell.

The grants for the MySQL Router accounts that were created automatically when bootstrapping need to be updated to match the new metadata version's requirements.
NOTE: No automatically created Router accounts were found.
WARNING: If MySQL Routers have been bootstrapped using custom accounts, their grants can not be updated during the metadata upgrade, they have to be updated using the setup_router_account function.
For additional information use: \? setup_router_account

An upgrade of all cluster router instances is required. All router installations should be updated first before doing the actual metadata upgrade.

+--------------------+-----------+---------------+----------+----------+
| Instance           | Version   | Last Check-in | R/O Port | R/W Port |
+--------------------+-----------+---------------+----------+----------+
| rennox-tc::        | <= 8.0.18 | NULL          | NULL     | NULL     |
| rennox-tc::eighth  | <= 8.0.18 | NULL          | NULL     | NULL     |
| rennox-tc::fifth   | <= 8.0.18 | NULL          | NULL     | NULL     |
| rennox-tc::fourth  | <= 8.0.18 | NULL          | NULL     | NULL     |
| rennox-tc::nineth  | <= 8.0.18 | NULL          | NULL     | NULL     |
| rennox-tc::second  | <= 8.0.18 | NULL          | NULL     | NULL     |
| rennox-tc::seventh | <= 8.0.18 | NULL          | NULL     | NULL     |
| rennox-tc::sixth   | <= 8.0.18 | NULL          | NULL     | NULL     |
| rennox-tc::tenth   | <= 8.0.18 | NULL          | NULL     | NULL     |
| rennox-tc::third   | <= 8.0.18 | NULL          | NULL     | NULL     |
+--------------------+-----------+---------------+----------+----------+
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

#@<OUT> Upgrades the metadata, upgrade done by unregistering more than 10 routers with router accounts
Metadata Schema Upgrade

The topology you are connected to is using an outdated metadata schema version 1.0.1 and needs to be upgraded to 2.3.0.

Without doing this upgrade, no AdminAPI calls except read only operations will be allowed.

NOTE: After the upgrade, this InnoDB Cluster/ReplicaSet can no longer be managed using older versions of MySQL Shell.

The grants for the MySQL Router accounts that were created automatically when bootstrapping need to be updated to match the new metadata version's requirements.
Updating Router accounts...
NOTE: 2 Router accounts have been updated.
WARNING: If MySQL Routers have been bootstrapped using custom accounts, their grants can not be updated during the metadata upgrade, they have to be updated using the setup_router_account function.
For additional information use: \? setup_router_account

An upgrade of all cluster router instances is required. All router installations should be updated first before doing the actual metadata upgrade.

+---------------------+-----------+---------------+----------+----------+
| Instance            | Version   | Last Check-in | R/O Port | R/W Port |
+---------------------+-----------+---------------+----------+----------+
| rennox-tc::         | <= 8.0.18 | NULL          | NULL     | NULL     |
| rennox-tc::eighth   | <= 8.0.18 | NULL          | NULL     | NULL     |
| rennox-tc::eleventh | <= 8.0.18 | NULL          | NULL     | NULL     |
| rennox-tc::fifth    | <= 8.0.18 | NULL          | NULL     | NULL     |
| rennox-tc::fourth   | <= 8.0.18 | NULL          | NULL     | NULL     |
| rennox-tc::nineth   | <= 8.0.18 | NULL          | NULL     | NULL     |
| rennox-tc::second   | <= 8.0.18 | NULL          | NULL     | NULL     |
| rennox-tc::seventh  | <= 8.0.18 | NULL          | NULL     | NULL     |
| rennox-tc::sixth    | <= 8.0.18 | NULL          | NULL     | NULL     |
| ...                 | ...       | ...           | ...      | ...      |
+---------------------+-----------+---------------+----------+----------+
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

#@<OUT> Test Migration from 1.0.1 to 2.3.0
Metadata Schema Upgrade

The topology you are connected to is using an outdated metadata schema version 1.0.1 and needs to be upgraded to 2.3.0.

Without doing this upgrade, no AdminAPI calls except read only operations will be allowed.

NOTE: After the upgrade, this InnoDB Cluster/ReplicaSet can no longer be managed using older versions of MySQL Shell.

The grants for the MySQL Router accounts that were created automatically when bootstrapping need to be updated to match the new metadata version's requirements.
Updating Router accounts...
NOTE: 2 Router accounts have been updated.
WARNING: If MySQL Routers have been bootstrapped using custom accounts, their grants can not be updated during the metadata upgrade, they have to be updated using the setup_router_account function.
For additional information use: \? setup_router_account

An upgrade of all cluster router instances is required. All router installations should be updated first before doing the actual metadata upgrade.

+-------------+-----------+---------------+----------+----------+
| Instance    | Version   | Last Check-in | R/O Port | R/W Port |
+-------------+-----------+---------------+----------+----------+
| rennox-tc:: | <= 8.0.18 | NULL          | NULL     | NULL     |
+-------------+-----------+---------------+----------+----------+
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
