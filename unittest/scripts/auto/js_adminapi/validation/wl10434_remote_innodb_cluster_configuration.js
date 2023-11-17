//@ FR1_1 SETUP {VER(>=8.0.11)}
||

//@ ConfigureInstance should fail if there's no session nor parameters provided
||An open session is required to perform this operation.

//@<OUT> FR_1 Configure instance not valid for InnoDB cluster {VER(>=8.0.11)}
Validating local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB Cluster...

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected

Checking instance configuration...

NOTE: Some configuration options need to be fixed:
?{VER(<8.0.23)}
+--------------------------+---------------+----------------+--------------------------------------------------+
| Variable                 | Current Value | Required Value | Note                                             |
+--------------------------+---------------+----------------+--------------------------------------------------+<<<(__version_num<80021) ?  "\n| binlog_checksum          | CRC32         | NONE           | Update the server variable                       |\n":"">>>
| enforce_gtid_consistency | OFF           | ON             | Update read-only variable and restart the server |
| gtid_mode                | OFF           | ON             | Update read-only variable and restart the server |
| server_id                | 1             | <unique ID>    | Update read-only variable and restart the server |
+--------------------------+---------------+----------------+--------------------------------------------------+
?{}
?{VER(>=8.0.23) && VER(<8.0.26)}
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
?{VER(==8.0.26)}
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
?{VER(>=8.0.27) && VER(<8.3.0)}
+----------------------------------------+---------------+----------------+--------------------------------------------------+
| Variable                               | Current Value | Required Value | Note                                             |
+----------------------------------------+---------------+----------------+--------------------------------------------------+
| binlog_transaction_dependency_tracking | COMMIT_ORDER  | WRITESET       | Update the server variable                       |
| enforce_gtid_consistency               | OFF           | ON             | Update read-only variable and restart the server |
| gtid_mode                              | OFF           | ON             | Update read-only variable and restart the server |
| server_id                              | 1             | <unique ID>    | Update read-only variable and restart the server |
+----------------------------------------+---------------+----------------+--------------------------------------------------+
?{}
?{VER(>=8.3.0)}
+--------------------------+---------------+----------------+--------------------------------------------------+
| Variable                 | Current Value | Required Value | Note                                             |
+--------------------------+---------------+----------------+--------------------------------------------------+
| enforce_gtid_consistency | OFF           | ON             | Update read-only variable and restart the server |
| gtid_mode                | OFF           | ON             | Update read-only variable and restart the server |
| server_id                | 1             | <unique ID>    | Update read-only variable and restart the server |
+--------------------------+---------------+----------------+--------------------------------------------------+
?{}

Some variables need to be changed, but cannot be done dynamically on the server.
NOTE: Please use the dba.configureInstance() command to repair these issues.

?{VER(<8.0.23)}
{
    "config_errors": [
?{}
?{VER(<8.0.21)}
        {
            "action": "server_update",
            "current": "CRC32",
            "option": "binlog_checksum",
            "required": "NONE"
        },
?{}
?{VER(<8.0.23)}
        {
            "action": "server_update+restart",
            "current": "OFF",
            "option": "enforce_gtid_consistency",
            "required": "ON"
        },
        {
            "action": "server_update+restart",
            "current": "OFF",
            "option": "gtid_mode",
            "required": "ON"
        },
        {
            "action": "server_update+restart",
            "current": "1",
            "option": "server_id",
            "required": "<unique ID>"
        }
    ],
    "status": "error"
}
?{}
?{VER(>=8.0.23) && VER(<8.0.26)}
{
    "config_errors": [
        {
            "action": "server_update",
            "current": "COMMIT_ORDER",
            "option": "binlog_transaction_dependency_tracking",
            "required": "WRITESET"
        },
        {
            "action": "server_update+restart",
            "current": "OFF",
            "option": "enforce_gtid_consistency",
            "required": "ON"
        },
        {
            "action": "server_update+restart",
            "current": "OFF",
            "option": "gtid_mode",
            "required": "ON"
        },
        {
            "action": "server_update+restart",
            "current": "1",
            "option": "server_id",
            "required": "<unique ID>"
        },
        {
            "action": "server_update",
            "current": "DATABASE",
            "option": "slave_parallel_type",
            "required": "LOGICAL_CLOCK"
        },
        {
            "action": "server_update",
            "current": "OFF",
            "option": "slave_preserve_commit_order",
            "required": "ON"
        }
    ],
    "status": "error"
}
?{}
?{VER(==8.0.26)}
{
    "config_errors": [
        {
            "action": "server_update",
            "current": "COMMIT_ORDER",
            "option": "binlog_transaction_dependency_tracking",
            "required": "WRITESET"
        },
        {
            "action": "server_update+restart",
            "current": "OFF",
            "option": "enforce_gtid_consistency",
            "required": "ON"
        },
        {
            "action": "server_update+restart",
            "current": "OFF",
            "option": "gtid_mode",
            "required": "ON"
        },
        {
            "action": "server_update",
            "current": "DATABASE",
            "option": "replica_parallel_type",
            "required": "LOGICAL_CLOCK"
        },
        {
            "action": "server_update",
            "current": "OFF",
            "option": "replica_preserve_commit_order",
            "required": "ON"
        },
        {
            "action": "server_update+restart",
            "current": "1",
            "option": "server_id",
            "required": "<unique ID>"
        }
    ],
    "status": "error"
}
?{}
?{VER(>=8.0.27) && VER(<8.3.0)}
{
    "config_errors": [
        {
            "action": "server_update",
            "current": "COMMIT_ORDER",
            "option": "binlog_transaction_dependency_tracking",
            "required": "WRITESET"
        },
        {
            "action": "server_update+restart",
            "current": "OFF",
            "option": "enforce_gtid_consistency",
            "required": "ON"
        },
        {
            "action": "server_update+restart",
            "current": "OFF",
            "option": "gtid_mode",
            "required": "ON"
        },
        {
            "action": "server_update+restart",
            "current": "1",
            "option": "server_id",
            "required": "<unique ID>"
        }
    ],
    "status": "error"
}
?{}
?{VER(>=8.3.0)}
{
    "config_errors": [
        {
            "action": "server_update+restart",
            "current": "OFF",
            "option": "enforce_gtid_consistency",
            "required": "ON"
        },
        {
            "action": "server_update+restart",
            "current": "OFF",
            "option": "gtid_mode",
            "required": "ON"
        },
        {
            "action": "server_update+restart",
            "current": "1",
            "option": "server_id",
            "required": "<unique ID>"
        }
    ],
    "status": "error"
}
?{}
WARNING: The interactive option is deprecated and will be removed in a future release.

Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB Cluster...

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>
?{VER(>=8.0.23)}

applierWorkerThreads will be set to the default value of 4.
?{}

NOTE: Some configuration options need to be fixed:
?{VER(<8.0.23)}
+--------------------------+---------------+----------------+--------------------------------------------------+
| Variable                 | Current Value | Required Value | Note                                             |
+--------------------------+---------------+----------------+--------------------------------------------------+<<<(__version_num<80021) ?  "\n| binlog_checksum          | CRC32         | NONE           | Update the server variable                       |\n":"">>>
| enforce_gtid_consistency | OFF           | ON             | Update read-only variable and restart the server |
| gtid_mode                | OFF           | ON             | Update read-only variable and restart the server |
| server_id                | 1             | <unique ID>    | Update read-only variable and restart the server |
+--------------------------+---------------+----------------+--------------------------------------------------+
?{}
?{VER(>=8.0.23) && VER(<8.0.26)}
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
?{VER(==8.0.26)}
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
?{VER(>=8.0.27) && VER(<8.3.0)}
+----------------------------------------+---------------+----------------+--------------------------------------------------+
| Variable                               | Current Value | Required Value | Note                                             |
+----------------------------------------+---------------+----------------+--------------------------------------------------+
| binlog_transaction_dependency_tracking | COMMIT_ORDER  | WRITESET       | Update the server variable                       |
| enforce_gtid_consistency               | OFF           | ON             | Update read-only variable and restart the server |
| gtid_mode                              | OFF           | ON             | Update read-only variable and restart the server |
| server_id                              | 1             | <unique ID>    | Update read-only variable and restart the server |
+----------------------------------------+---------------+----------------+--------------------------------------------------+
?{}
?{VER(>=8.3.0)}
+--------------------------+---------------+----------------+--------------------------------------------------+
| Variable                 | Current Value | Required Value | Note                                             |
+--------------------------+---------------+----------------+--------------------------------------------------+
| enforce_gtid_consistency | OFF           | ON             | Update read-only variable and restart the server |
| gtid_mode                | OFF           | ON             | Update read-only variable and restart the server |
| server_id                | 1             | <unique ID>    | Update read-only variable and restart the server |
+--------------------------+---------------+----------------+--------------------------------------------------+
?{}

Some variables need to be changed, but cannot be done dynamically on the server.
Configuring instance...
?{((VER(>=8.0.35) && VER(<8.1.0)) || (VER(>=8.2.0) && VER(<8.3.0))) && !__replaying}

WARNING: '@@binlog_transaction_dependency_tracking' is deprecated and will be removed in a future release. (Code 1287).
?{}
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB Cluster.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.
Validating local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB Cluster...

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected

Checking instance configuration...
Instance configuration is compatible with InnoDB cluster

The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is valid for InnoDB Cluster usage.

{
    "status": "ok"
}

//@ FR1_1 TEARDOWN {VER(>=8.0.11)}
||

//@ FR1.1_1 SETUP {VER(>=8.0.11)}
||

//@# FR1.1_1 Configure instance using dba.configureInstance() with variable that cannot remotely persisted {VER(>=8.0.11)}

|ERROR: The path to the MySQL configuration file is required to verify and fix InnoDB cluster related options.|
||Unable to update configuration

//@# FR1.1_2 Configure instance using dba.configureInstance() with variable that cannot remotely persisted {VER(>=8.0.11)}
|The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB Cluster.|
|The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is valid for InnoDB Cluster usage.|

//@ FR1.1 TEARDOWN {VER(>=8.0.11)}
||

//@ FR2_2 SETUP {VER(>=8.0.11)}
||

//@ FR2_2 - Configure local instance using dba.configureInstance() with 'persisted-globals-load' set to 'OFF' but no cnf file path {VER(>=8.0.11)}
|Remote configuration of the instance is not possible because options changed with SET PERSIST will not be loaded, unless 'persisted_globals_load' is set to ON.|
||Unable to update configuration

//@<OUT> FR2_2 - Configure local instance using dba.configureInstance() with 'persisted-globals-load' set to 'OFF' with cnf file path {VER(>=8.0.11)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB Cluster...

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>
?{VER(>=8.0.23)}

applierWorkerThreads will be set to the default value of 4.
?{}

NOTE: Some configuration options need to be fixed:
?{VER(<8.0.21)}
+----------------------------------+---------------+----------------+------------------------------------------------+
| Variable                         | Current Value | Required Value | Note                                           |
+----------------------------------+---------------+----------------+------------------------------------------------+
| binlog_checksum                  | CRC32         | NONE           | Update the server variable and the config file |
| binlog_format                    | <not set>     | ROW            | Update the config file                         |
| enforce_gtid_consistency         | OFF           | ON             | Update the config file and restart the server  |
| gtid_mode                        | OFF           | ON             | Update the config file and restart the server  |
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
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB Cluster.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.
?{}
?{VER(>=8.0.21) && VER(<8.0.23)}
+----------------------------------+---------------+----------------+-----------------------------------------------+
| Variable                         | Current Value | Required Value | Note                                          |
+----------------------------------+---------------+----------------+-----------------------------------------------+
| binlog_format                    | <not set>     | ROW            | Update the config file                        |
| enforce_gtid_consistency         | OFF           | ON             | Update the config file and restart the server |
| gtid_mode                        | OFF           | ON             | Update the config file and restart the server |
| master_info_repository           | <not set>     | TABLE          | Update the config file                        |
| relay_log_info_repository        | <not set>     | TABLE          | Update the config file                        |
| report_port                      | <not set>     | <<<__mysql_sandbox_port1>>>           | Update the config file                        |
| server_id                        | 1             | <unique ID>    | Update the config file and restart the server |
| transaction_write_set_extraction | <not set>     | XXHASH64       | Update the config file                        |
+----------------------------------+---------------+----------------+-----------------------------------------------+
?{}
?{VER(>=8.0.23) && VER(<8.0.26)}
+----------------------------------------+---------------+----------------+------------------------------------------------+
| Variable                               | Current Value | Required Value | Note                                           |
+----------------------------------------+---------------+----------------+------------------------------------------------+
| binlog_format                          | <not set>     | ROW            | Update the config file                         |
| binlog_transaction_dependency_tracking | COMMIT_ORDER  | WRITESET       | Update the server variable and the config file |
| enforce_gtid_consistency               | OFF           | ON             | Update the config file and restart the server  |
| gtid_mode                              | OFF           | ON             | Update the config file and restart the server  |
| report_port                            | <not set>     | <<<__mysql_sandbox_port1>>>           | Update the config file                         |
| server_id                              | 1             | <unique ID>    | Update the config file and restart the server  |
| slave_parallel_type                    | DATABASE      | LOGICAL_CLOCK  | Update the server variable and the config file |
| slave_preserve_commit_order            | OFF           | ON             | Update the server variable and the config file |
| transaction_write_set_extraction       | <not set>     | XXHASH64       | Update the config file                         |
+----------------------------------------+---------------+----------------+------------------------------------------------+
?{}
?{VER(==8.0.26)}
+----------------------------------------+---------------+----------------+------------------------------------------------+
| Variable                               | Current Value | Required Value | Note                                           |
+----------------------------------------+---------------+----------------+------------------------------------------------+
| binlog_format                          | <not set>     | ROW            | Update the config file                         |
| binlog_transaction_dependency_tracking | COMMIT_ORDER  | WRITESET       | Update the server variable and the config file |
| enforce_gtid_consistency               | OFF           | ON             | Update the config file and restart the server  |
| gtid_mode                              | OFF           | ON             | Update the config file and restart the server  |
| replica_parallel_type                  | DATABASE      | LOGICAL_CLOCK  | Update the server variable and the config file |
| replica_preserve_commit_order          | OFF           | ON             | Update the server variable and the config file |
| report_port                            | <not set>     | <<<__mysql_sandbox_port1>>>           | Update the config file                         |
| server_id                              | 1             | <unique ID>    | Update the config file and restart the server  |
| transaction_write_set_extraction       | <not set>     | XXHASH64       | Update the config file                         |
+----------------------------------------+---------------+----------------+------------------------------------------------+
?{}
?{VER(>=8.0.27) && VER(<8.3.0)}
+----------------------------------------+---------------+----------------+------------------------------------------------+
| Variable                               | Current Value | Required Value | Note                                           |
+----------------------------------------+---------------+----------------+------------------------------------------------+
| binlog_format                          | <not set>     | ROW            | Update the config file                         |
| binlog_transaction_dependency_tracking | COMMIT_ORDER  | WRITESET       | Update the server variable and the config file |
| enforce_gtid_consistency               | OFF           | ON             | Update the config file and restart the server  |
| gtid_mode                              | OFF           | ON             | Update the config file and restart the server  |
| replica_parallel_type                  | <not set>     | LOGICAL_CLOCK  | Update the config file                         |
| replica_preserve_commit_order          | <not set>     | ON             | Update the config file                         |
| report_port                            | <not set>     | <<<__mysql_sandbox_port1>>>           | Update the config file                         |
| server_id                              | 1             | <unique ID>    | Update the config file and restart the server  |
| transaction_write_set_extraction       | <not set>     | XXHASH64       | Update the config file                         |
+----------------------------------------+---------------+----------------+------------------------------------------------+
?{}
?{VER(>=8.3.0)}
+----------------------------------------+---------------+----------------+-----------------------------------------------+
| Variable                               | Current Value | Required Value | Note                                          |
+----------------------------------------+---------------+----------------+-----------------------------------------------+
| binlog_format                          | <not set>     | ROW            | Update the config file                        |
| binlog_transaction_dependency_tracking | <not set>     | WRITESET       | Update the config file                        |
| enforce_gtid_consistency               | OFF           | ON             | Update the config file and restart the server |
| gtid_mode                              | OFF           | ON             | Update the config file and restart the server |
| replica_preserve_commit_order          | <not set>     | ON             | Update the config file                        |
| report_port                            | <not set>     | <<<__mysql_sandbox_port1>>>           | Update the config file                        |
| server_id                              | 1             | <unique ID>    | Update the config file and restart the server |
+----------------------------------------+---------------+----------------+-----------------------------------------------+
?{}

Some variables need to be changed, but cannot be done dynamically on the server: set persist support is disabled. Enable it or provide an option file.
NOTE: persisted_globals_load option is OFF
Remote configuration of the instance is not possible because options changed with SET PERSIST will not be loaded, unless 'persisted_globals_load' is set to ON.
Configuring instance...
?{((VER(>=8.0.35) && VER(<8.1.0)) || (VER(>=8.2.0) && VER(<8.3.0))) && !__replaying}

WARNING: '@@binlog_transaction_dependency_tracking' is deprecated and will be removed in a future release. (Code 1287).
?{}
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB Cluster.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.

//@ FR2_2 TEARDOWN {VER(>=8.0.11)}
||

//@ FR3.1_1 SETUP {VER(<8.0.11)}
||

//@<OUT>FR3.1_1 - Configure local instance with 'persisted-globals-load' set to 'OFF' {VER(<8.0.11)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB Cluster...

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

NOTE: Some configuration options need to be fixed:
+----------------------------------+---------------+----------------+------------------------------------------------+
| Variable                         | Current Value | Required Value | Note                                           |
+----------------------------------+---------------+----------------+------------------------------------------------+
| binlog_checksum                  | CRC32         | NONE           | Update the server variable and the config file |
| binlog_format                    | <not set>     | ROW            | Update the config file                         |
| enforce_gtid_consistency         | OFF           | ON             | Update the config file and restart the server  |
| gtid_mode                        | OFF           | ON             | Update the config file and restart the server  |
| log_bin                          | <not present> | ON             | Update the config file and restart the server  |
| log_slave_updates                | OFF           | ON             | Update the config file and restart the server  |
| master_info_repository           | FILE          | TABLE          | Update the config file and restart the server  |
| relay_log_info_repository        | FILE          | TABLE          | Update the config file and restart the server  |
| report_port                      | <not set>     | <<<__mysql_sandbox_port1>>>           | Update the server variable and the config file |
| server_id                        | 0             | <unique ID>    | Update the config file and restart the server  |
| transaction_write_set_extraction | OFF           | XXHASH64       | Update the config file and restart the server  |
+----------------------------------+---------------+----------------+------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server: an option file is required.
Do you want to perform the required configuration changes? [y/n]: Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB Cluster.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.
WARNING: Support for AdminAPI operations in MySQL version 5.7 is deprecated and will be removed in a future release of MySQL Shell
Validating local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB Cluster...

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
| log_bin                          | OFF           | ON             | Restart the server                               |
| log_slave_updates                | OFF           | ON             | Update read-only variable and restart the server |
| master_info_repository           | FILE          | TABLE          | Update read-only variable and restart the server |
| relay_log_info_repository        | FILE          | TABLE          | Update read-only variable and restart the server |
| server_id                        | 0             | <unique ID>    | Update read-only variable and restart the server |
| transaction_write_set_extraction | OFF           | XXHASH64       | Update read-only variable and restart the server |
+----------------------------------+---------------+----------------+--------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
NOTE: Please use the dba.configureInstance() command to repair these issues.

//@ FR3.1_1 TEARDOWN {VER(<8.0.11)}
||

//@ FR3.2_1 SETUP {VER(<8.0.11)}
||

//@# FR3.2_1 - Configure local instance with 'persisted-globals-load' set to 'OFF' providing mycnfPath {VER(<8.0.11)}
|Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB Cluster...|
|The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB Cluster.|
|NOTE: MySQL server needs to be restarted for configuration changes to take effect.|

|Validating local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB Cluster...|
|Please use the dba.configureInstance() command to repair these issues.|

//@ FR3.2_1 TEARDOWN {VER(<8.0.11)}
||

//@ FR3.3_1 SETUP {VER(<8.0.11)}
||

//@# FR3.3_1 - Configure local instance that does not require changes {VER(<8.0.11)}
|Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB Cluster...|
|1) Create remotely usable account for 'root' with same grants and password|
|2) Create a new admin account for InnoDB Cluster with minimal required grants|
|3) Ignore and continue|
|4) Cancel|
|The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is valid for InnoDB Cluster usage.|

//@ FR3.3_1 TEARDOWN {VER(<8.0.11)}
||

//@ FR5 SETUP {VER(>=8.0.11)}
||

//@<OUT> FR5 Configure instance not valid for InnoDB cluster, with interaction enabled {VER(>=8.0.11)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB Cluster...

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

ERROR: User 'root' can only connect from 'localhost'. New account(s) with proper source address specification to allow remote connection from all instances must be created to manage the cluster.

1) Create remotely usable account for 'root' with same grants and password
2) Create a new admin account for InnoDB Cluster with minimal required grants
3) Ignore and continue
4) Cancel

Please select an option [1]:
?{VER(>=8.0.23)}
applierWorkerThreads will be set to the default value of 4.

?{}
NOTE: Some configuration options need to be fixed:
?{VER(<8.0.23)}
+--------------------------+---------------+----------------+--------------------------------------------------+
| Variable                 | Current Value | Required Value | Note                                             |
+--------------------------+---------------+----------------+--------------------------------------------------+<<<(__version_num<80021) ?  "\n| binlog_checksum          | CRC32         | NONE           | Update the server variable                       |\n":"">>>
| enforce_gtid_consistency | OFF           | ON             | Update read-only variable and restart the server |
| gtid_mode                | OFF           | ON             | Update read-only variable and restart the server |
| server_id                | 1             | <unique ID>    | Update read-only variable and restart the server |
+--------------------------+---------------+----------------+--------------------------------------------------+
?{}
?{VER(>=8.0.23) && VER(<8.0.26)}
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
?{VER(==8.0.26)}
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
?{VER(>=8.0.27) && VER(<8.3.0)}
+----------------------------------------+---------------+----------------+--------------------------------------------------+
| Variable                               | Current Value | Required Value | Note                                             |
+----------------------------------------+---------------+----------------+--------------------------------------------------+
| binlog_transaction_dependency_tracking | COMMIT_ORDER  | WRITESET       | Update the server variable                       |
| enforce_gtid_consistency               | OFF           | ON             | Update read-only variable and restart the server |
| gtid_mode                              | OFF           | ON             | Update read-only variable and restart the server |
| server_id                              | 1             | <unique ID>    | Update read-only variable and restart the server |
+----------------------------------------+---------------+----------------+--------------------------------------------------+
?{}
?{VER(>=8.3.0)}
+--------------------------+---------------+----------------+--------------------------------------------------+
| Variable                 | Current Value | Required Value | Note                                             |
+--------------------------+---------------+----------------+--------------------------------------------------+
| enforce_gtid_consistency | OFF           | ON             | Update read-only variable and restart the server |
| gtid_mode                | OFF           | ON             | Update read-only variable and restart the server |
| server_id                | 1             | <unique ID>    | Update read-only variable and restart the server |
+--------------------------+---------------+----------------+--------------------------------------------------+
?{}

Some variables need to be changed, but cannot be done dynamically on the server.
Do you want to perform the required configuration changes? [y/n]: Do you want to restart the instance after configuring it? [y/n]: Configuring instance...
?{((VER(>=8.0.35) && VER(<8.1.0)) || (VER(>=8.2.0) && VER(<8.3.0))) && !__replaying}

WARNING: '@@binlog_transaction_dependency_tracking' is deprecated and will be removed in a future release. (Code 1287).
?{}
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB Cluster.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.

//@ FR5 TEARDOWN {VER(>=8.0.11)}
||

//@ FR7 SETUP {VER(>=8.0.11)}
||

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
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB Cluster...

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>
Assuming full account name 'clusterAdminAccount'@'%' for clusterAdminAccount
Password for new account: Confirm password:
?{VER(>=8.0.23)}
applierWorkerThreads will be set to the default value of 4.

?{}
NOTE: Some configuration options need to be fixed:
?{VER(<8.0.23)}
+--------------------------+---------------+----------------+--------------------------------------------------+
| Variable                 | Current Value | Required Value | Note                                             |
+--------------------------+---------------+----------------+--------------------------------------------------+<<<(__version_num<80021) ?  "\n| binlog_checksum          | CRC32         | NONE           | Update the server variable                       |\n":"">>>
| enforce_gtid_consistency | OFF           | ON             | Update read-only variable and restart the server |
| gtid_mode                | OFF           | ON             | Update read-only variable and restart the server |
| server_id                | 1             | <unique ID>    | Update read-only variable and restart the server |
+--------------------------+---------------+----------------+--------------------------------------------------+
?{}
?{VER(>=8.0.23) && VER(<8.0.26)}
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
?{VER(==8.0.26)}
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
?{VER(>=8.0.27) && VER(<8.3.0)}
+----------------------------------------+---------------+----------------+--------------------------------------------------+
| Variable                               | Current Value | Required Value | Note                                             |
+----------------------------------------+---------------+----------------+--------------------------------------------------+
| binlog_transaction_dependency_tracking | COMMIT_ORDER  | WRITESET       | Update the server variable                       |
| enforce_gtid_consistency               | OFF           | ON             | Update read-only variable and restart the server |
| gtid_mode                              | OFF           | ON             | Update read-only variable and restart the server |
| server_id                              | 1             | <unique ID>    | Update read-only variable and restart the server |
+----------------------------------------+---------------+----------------+--------------------------------------------------+
?{}
?{VER(>=8.3.0)}
+--------------------------+---------------+----------------+--------------------------------------------------+
| Variable                 | Current Value | Required Value | Note                                             |
+--------------------------+---------------+----------------+--------------------------------------------------+
| enforce_gtid_consistency | OFF           | ON             | Update read-only variable and restart the server |
| gtid_mode                | OFF           | ON             | Update read-only variable and restart the server |
| server_id                | 1             | <unique ID>    | Update read-only variable and restart the server |
+--------------------------+---------------+----------------+--------------------------------------------------+
?{}

Some variables need to be changed, but cannot be done dynamically on the server.
Do you want to perform the required configuration changes? [y/n]: Do you want to restart the instance after configuring it? [y/n]:
Creating user clusterAdminAccount@%.
Account clusterAdminAccount@% was successfully created.

Configuring instance...
?{((VER(>=8.0.35) && VER(<8.1.0)) || (VER(>=8.2.0) && VER(<8.3.0))) && !__replaying}

WARNING: '@@binlog_transaction_dependency_tracking' is deprecated and will be removed in a future release. (Code 1287).
?{}
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB Cluster.

//@<OUT> ET_12 - Call dba.configuereInstance() with interactive flag set to true, clusterAdmin option and super_read_only=1 {VER(>=8.0.11)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB Cluster...

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>
Assuming full account name 'newClusterAdminAccount'@'%' for newClusterAdminAccount
Password for new account: Confirm password:
?{VER(>=8.0.23)}
applierWorkerThreads will be set to the default value of 4.

?{}
NOTE: Some configuration options need to be fixed:
+--------------------------+---------------+----------------+--------------------------------------------------+
| Variable                 | Current Value | Required Value | Note                                             |
+--------------------------+---------------+----------------+--------------------------------------------------+
| enforce_gtid_consistency | OFF           | ON             | Update read-only variable and restart the server |
| gtid_mode                | OFF           | ON             | Update read-only variable and restart the server |
| server_id                | 1             | <unique ID>    | Update read-only variable and restart the server |
+--------------------------+---------------+----------------+--------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
Do you want to perform the required configuration changes? [y/n]: Do you want to restart the instance after configuring it? [y/n]: Disabled super_read_only on the instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'

Creating user newClusterAdminAccount@%.
Account newClusterAdminAccount@% was successfully created.

Enabling super_read_only on the instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'
Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB Cluster.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.

//@ ET_12_alt - Super read-only enabled and 'clearReadOnly' is set
||

//@ ET_12_alt - Super read-only enabled and 'clearReadOnly' is set 5.7
||

//@ ET_13 - Call dba.configuereInstance() with interactive flag set to false, clusterAdmin option and super_read_only=1 {VER(>=8.0.11)}
||

//@ ET TEARDOWN
||
