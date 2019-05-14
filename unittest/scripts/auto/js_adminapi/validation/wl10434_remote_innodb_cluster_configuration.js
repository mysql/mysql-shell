//@ FR1_1 SETUP {VER(>=8.0.11)}
||

//@ ConfigureInstance should fail if there's no session nor parameters provided
||An open session is required to perform this operation.

//@<OUT> FR_1 Configure instance not valid for InnoDB cluster {VER(>=8.0.11)}
true
Validating local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected

Checking instance configuration...

NOTE: Some configuration options need to be fixed:
+--------------------------+---------------+----------------+--------------------------------------------------+
| Variable                 | Current Value | Required Value | Note                                             |
+--------------------------+---------------+----------------+--------------------------------------------------+
| binlog_checksum          | CRC32         | NONE           | Update the server variable                       |
| enforce_gtid_consistency | OFF           | ON             | Update read-only variable and restart the server |
| gtid_mode                | OFF           | ON             | Update read-only variable and restart the server |
| server_id                | 1             | <unique ID>    | Update read-only variable and restart the server |
+--------------------------+---------------+----------------+--------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
NOTE: Please use the dba.configureInstance() command to repair these issues.

{
    "config_errors": [
        {
            "action": "server_update",
            "current": "CRC32",
            "option": "binlog_checksum",
            "required": "NONE"
        },
        {
            "action": "restart",
            "current": "OFF",
            "option": "enforce_gtid_consistency",
            "required": "ON"
        },
        {
            "action": "restart",
            "current": "OFF",
            "option": "gtid_mode",
            "required": "ON"
        },
        {
            "action": "restart",
            "current": "1",
            "option": "server_id",
            "required": "<unique ID>"
        }
    ],
    "status": "error"
}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

NOTE: Some configuration options need to be fixed:
+--------------------------+---------------+----------------+--------------------------------------------------+
| Variable                 | Current Value | Required Value | Note                                             |
+--------------------------+---------------+----------------+--------------------------------------------------+
| binlog_checksum          | CRC32         | NONE           | Update the server variable                       |
| enforce_gtid_consistency | OFF           | ON             | Update read-only variable and restart the server |
| gtid_mode                | OFF           | ON             | Update read-only variable and restart the server |
| server_id                | 1             | <unique ID>    | Update read-only variable and restart the server |
+--------------------------+---------------+----------------+--------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
Configuring instance...
The instance 'localhost:<<<__mysql_sandbox_port1>>>' was configured for InnoDB cluster usage.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.
Validating local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected

Checking instance configuration...
Instance configuration is compatible with InnoDB cluster

The instance 'localhost:<<<__mysql_sandbox_port1>>>' is valid for InnoDB cluster usage.

{
    "status": "ok"
}

//@ FR1_1 TEARDOWN {VER(>=8.0.11)}
||

//@ FR1.1_1 SETUP {VER(>=8.0.11)}
||

//@# FR1.1_1 Configure instance using dba.configureInstance() with variable that cannot remotely persisted {VER(>=8.0.11)}

|ERROR: The path to the MySQL configuration file is required to verify and fix InnoDB cluster related options.|
||Dba.configureInstance: Unable to update configuration

//@# FR1.1_2 Configure instance using dba.configureInstance() with variable that cannot remotely persisted {VER(>=8.0.11)}
|The instance 'localhost:<<<__mysql_sandbox_port1>>>' was configured for InnoDB cluster usage.|
|The instance 'localhost:<<<__mysql_sandbox_port1>>>' is valid for InnoDB cluster usage.|

//@ FR1.1 TEARDOWN {VER(>=8.0.11)}
||

//@ FR2_2 SETUP {VER(>=8.0.11)}
||

//@ FR2_2 - Configure local instance using dba.configureInstance() with 'persisted-globals-load' set to 'OFF' but no cnf file path {VER(>=8.0.11)}
|Remote configuration of the instance is not possible because options changed with SET PERSIST will not be loaded, unless 'persisted_globals_load' is set to ON.|
||Dba.configureInstance: Unable to update configuration

//@<OUT> FR2_2 - Configure local instance using dba.configureInstance() with 'persisted-globals-load' set to 'OFF' with cnf file path {VER(>=8.0.11)}
true
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

NOTE: Some configuration options need to be fixed:
+----------------------------------+---------------+----------------+------------------------------------------------+
| Variable                         | Current Value | Required Value | Note                                           |
+----------------------------------+---------------+----------------+------------------------------------------------+
| binlog_checksum                  | CRC32         | NONE           | Update the server variable and the config file |
| binlog_format                    | <not set>     | ROW            | Update the config file                         |
| enforce_gtid_consistency         | OFF           | ON             | Update the config file and restart the server  |
| gtid_mode                        | OFF           | ON             | Update the config file and restart the server  |
| log_slave_updates                | <not set>     | ON             | Update the config file                         |
| master_info_repository           | <not set>     | TABLE          | Update the config file                         |
| relay_log_info_repository        | <not set>     | TABLE          | Update the config file                         |
| report_port                      | <not set>     | <<<__mysql_sandbox_port1>>>           | Update the config file                         |
| server_id                        | 1             | <unique ID>    | Update the config file and restart the server  |
| transaction_write_set_extraction | <not set>     | XXHASH64       | Update the config file                         |
+----------------------------------+---------------+----------------+------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server: set persist support is disabled. Enable it or provide an option file.
NOTE: persisted_globals_load option is OFF
Remote configuration of the instance is not possible because options changed with SET PERSIST will not be loaded, unless 'persisted_globals_load' is set to ON.
Configuring instance...
The instance 'localhost:<<<__mysql_sandbox_port1>>>' was configured for InnoDB cluster usage.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.

//@ FR2_2 TEARDOWN {VER(>=8.0.11)}
||

//@ FR3.1_1 SETUP {VER(<8.0.11)}
||

//@<OUT>FR3.1_1 - Configure local instance with 'persisted-globals-load' set to 'OFF' {VER(<8.0.11)}
true
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

NOTE: Some configuration options need to be fixed:
+----------------------------------+---------------+----------------+--------------------------------------------------+
| Variable                         | Current Value | Required Value | Note                                             |
+----------------------------------+---------------+----------------+--------------------------------------------------+
| binlog_checksum                  | CRC32         | NONE           | Update the server variable and the config file   |
| binlog_format                    | <not set>     | ROW            | Update the config file                           |
| enforce_gtid_consistency         | OFF           | ON             | Update the config file and restart the server    |
| gtid_mode                        | OFF           | ON             | Update the config file and restart the server    |
| log_bin                          | <not set>     | <no value>     | Update the config file                           |
| log_bin                          | OFF           | ON             | Update read-only variable and restart the server |
| log_slave_updates                | OFF           | ON             | Update the config file and restart the server    |
| master_info_repository           | FILE          | TABLE          | Update the config file and restart the server    |
| relay_log_info_repository        | FILE          | TABLE          | Update the config file and restart the server    |
| report_port                      | <not set>     | <<<__mysql_sandbox_port1>>>           | Update the config file                           |
| server_id                        | 0             | <unique ID>    | Update the config file and restart the server    |
| transaction_write_set_extraction | OFF           | XXHASH64       | Update the config file and restart the server    |
+----------------------------------+---------------+----------------+--------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server: an option file is required.
Do you want to perform the required configuration changes? [y/n]: Configuring instance...
The instance 'localhost:<<<__mysql_sandbox_port1>>>' was configured for InnoDB cluster usage.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.
Validating local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected

Checking instance configuration...
Configuration file <<<sandbox_cnf1>>> will also be checked.

NOTE: Some configuration options need to be fixed:
+----------------------------------+---------------+----------------+--------------------------------------------------+
| Variable                         | Current Value | Required Value | Note                                             |
+----------------------------------+---------------+----------------+--------------------------------------------------+
| enforce_gtid_consistency         | OFF           | ON             | Update read-only variable and restart the server |
| gtid_mode                        | OFF           | ON             | Update read-only variable and restart the server |
| log_bin                          | OFF           | ON             | Update read-only variable and restart the server |
| log_slave_updates                | OFF           | ON             | Update read-only variable and restart the server |
| master_info_repository           | FILE          | TABLE          | Update read-only variable and restart the server |
| relay_log_info_repository        | FILE          | TABLE          | Update read-only variable and restart the server |
| server_id                        | 0             | <unique ID>    | Update read-only variable and restart the server |
| transaction_write_set_extraction | OFF           | XXHASH64       | Update read-only variable and restart the server |
+----------------------------------+---------------+----------------+--------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
NOTE: Please restart the MySQL server and try again.

//@ FR3.1_1 TEARDOWN {VER(<8.0.11)}
||

//@ FR3.2_1 SETUP {VER(<8.0.11)}
||

//@# FR3.2_1 - Configure local instance with 'persisted-globals-load' set to 'OFF' providing mycnfPath {VER(<8.0.11)}
|Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...|
|The instance 'localhost:<<<__mysql_sandbox_port1>>>' was configured for InnoDB cluster usage.|
|NOTE: MySQL server needs to be restarted for configuration changes to take effect.|

|Validating local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...|
|Please use the dba.configureInstance() command to repair these issues.|

//@ FR3.2_1 TEARDOWN {VER(<8.0.11)}
||

//@ FR3.3_1 SETUP {VER(<8.0.11)}
||

//@# FR3.3_1 - Configure local instance that does not require changes {VER(<8.0.11)}
|Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...|
|1) Create remotely usable account for 'root' with same grants and password|
|2) Create a new admin account for InnoDB cluster with minimal required grants|
|3) Ignore and continue|
|4) Cancel|
|The instance 'localhost:<<<__mysql_sandbox_port1>>>' is valid for InnoDB cluster usage.|

//@ FR3.3_1 TEARDOWN {VER(<8.0.11)}
||

//@ FR5 SETUP {VER(>=8.0.11)}
||

//@<OUT> FR5 Configure instance not valid for InnoDB cluster, with interaction enabled {VER(>=8.0.11)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

ERROR: User 'root' can only connect from 'localhost'. New account(s) with proper source address specification to allow remote connection from all instances must be created to manage the cluster.

1) Create remotely usable account for 'root' with same grants and password
2) Create a new admin account for InnoDB cluster with minimal required grants
3) Ignore and continue
4) Cancel

Please select an option [1]:
NOTE: Some configuration options need to be fixed:
+--------------------------+---------------+----------------+--------------------------------------------------+
| Variable                 | Current Value | Required Value | Note                                             |
+--------------------------+---------------+----------------+--------------------------------------------------+
| binlog_checksum          | CRC32         | NONE           | Update the server variable                       |
| enforce_gtid_consistency | OFF           | ON             | Update read-only variable and restart the server |
| gtid_mode                | OFF           | ON             | Update read-only variable and restart the server |
| server_id                | 1             | <unique ID>    | Update read-only variable and restart the server |
+--------------------------+---------------+----------------+--------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
Do you want to perform the required configuration changes? [y/n]: Do you want to restart the instance after configuring it? [y/n]: Configuring instance...
The instance 'localhost:<<<__mysql_sandbox_port1>>>' was configured for InnoDB cluster usage.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.

//@ FR5 TEARDOWN {VER(>=8.0.11)}
||

//@ FR7 SETUP {VER(>=8.0.11)}
||

//@ FR7 Configure instance already belonging to cluster {VER(>=8.0.11)}
||Dba.configureInstance: This function is not available through a session to an instance already in an InnoDB cluster (RuntimeError)

//@ FR7 TEARDOWN {VER(>=8.0.11)}
||

//@ ET SETUP
||

//@ ET_5 - Call dba.configureInstance() with interactive flag set to true and not specifying username {VER(>=8.0.11)}
||(using password: YES) (MySQL Error 1045)

//@ ET_7 - Call dba.configureInstance() with interactive flag set to true and not specifying a password {VER(>=8.0.11)}
||Access denied for user 'root'@'localhost' (using password: YES) (MySQL Error 1045)

//@ ET_8 - Call dba.configureInstance() with interactive flag set to false and not specifying a password {VER(>=8.0.11)}
||Access denied for user 'root'@'localhost' (using password: NO) (MySQL Error 1045)

//@<OUT> ET_10 - Call dba.configuereInstance() with interactive flag set to true and using clusterAdmin {VER(>=8.0.11)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>
Assuming full account name 'clusterAdminAccount'@'%' for clusterAdminAccount
Password for new account: Confirm password:
NOTE: Some configuration options need to be fixed:
+--------------------------+---------------+----------------+--------------------------------------------------+
| Variable                 | Current Value | Required Value | Note                                             |
+--------------------------+---------------+----------------+--------------------------------------------------+
| binlog_checksum          | CRC32         | NONE           | Update the server variable                       |
| enforce_gtid_consistency | OFF           | ON             | Update read-only variable and restart the server |
| gtid_mode                | OFF           | ON             | Update read-only variable and restart the server |
| server_id                | 1             | <unique ID>    | Update read-only variable and restart the server |
+--------------------------+---------------+----------------+--------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
Do you want to perform the required configuration changes? [y/n]: Do you want to restart the instance after configuring it? [y/n]:
Cluster admin user 'clusterAdminAccount'@'%' created.
Configuring instance...
The instance 'localhost:<<<__mysql_sandbox_port1>>>' was configured for InnoDB cluster usage.

//@<OUT> ET_12 - Call dba.configuereInstance() with interactive flag set to true, clusterAdmin option and super_read_only=1 {VER(>=8.0.11)}
true
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>
Assuming full account name 'newClusterAdminAccount'@'%' for newClusterAdminAccount
Password for new account: Confirm password:
NOTE: Some configuration options need to be fixed:
+--------------------------+---------------+----------------+--------------------------------------------------+
| Variable                 | Current Value | Required Value | Note                                             |
+--------------------------+---------------+----------------+--------------------------------------------------+
| enforce_gtid_consistency | OFF           | ON             | Update read-only variable and restart the server |
| gtid_mode                | OFF           | ON             | Update read-only variable and restart the server |
| server_id                | 1             | <unique ID>    | Update read-only variable and restart the server |
+--------------------------+---------------+----------------+--------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
Do you want to perform the required configuration changes? [y/n]: Do you want to restart the instance after configuring it? [y/n]:
The MySQL instance at 'localhost:<<<__mysql_sandbox_port1>>>' currently has the super_read_only system
variable set to protect it from inadvertent updates from applications. You must
first unset it to be able to perform any changes to this instance.
For more information see:
https://dev.mysql.com/doc/refman/en/server-system-variables.html#sysvar_super_read_only.

NOTE: There are open sessions to 'localhost:<<<__mysql_sandbox_port1>>>'.
You may want to kill these sessions to prevent them from performing unexpected updates:

1 open session(s) of 'root@localhost'.

Do you want to disable super_read_only and continue? [y/N]: Disabled super_read_only on the instance 'localhost:<<<__mysql_sandbox_port1>>>'
Cluster admin user 'newClusterAdminAccount'@'%' created.
Enabling super_read_only on the instance 'localhost:<<<__mysql_sandbox_port1>>>'
Configuring instance...
The instance 'localhost:<<<__mysql_sandbox_port1>>>' was configured for InnoDB cluster usage.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.

//@ ET_12_alt - Super read-only enabled and 'clearReadOnly' is set
||

//@ ET_12_alt - Super read-only enabled and 'clearReadOnly' is set 5.7
||

//@ ET_13 - Call dba.configuereInstance() with interactive flag set to false, clusterAdmin option and super_read_only=1 {VER(>=8.0.11)}
|ERROR: The MySQL instance at 'localhost:<<<__mysql_sandbox_port1>>>' currently has the super_read_only|
||Dba.configureInstance: Server in SUPER_READ_ONLY mode (RuntimeError)

//@ ET TEARDOWN
||
