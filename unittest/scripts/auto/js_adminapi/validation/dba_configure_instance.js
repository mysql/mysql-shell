//@<OUT> configureInstance custom cluster admin and password
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB Cluster...

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

ERROR: User 'root' can only connect from 'localhost'. New account(s) with proper source address specification to allow remote connection from all instances must be created to manage the cluster.

1) Create remotely usable account for 'root' with same grants and password
2) Create a new admin account for InnoDB Cluster with minimal required grants
3) Ignore and continue
4) Cancel

Please select an option [1]: Please provide an account name (e.g: icroot@%) to have it created with the necessary
privileges or leave empty and press Enter to cancel.
Account Name: Password for new account: Confirm password:
?{VER(>=8.0.23)}
applierWorkerThreads will be set to the default value of 4.

?{}
NOTE: Some configuration options need to be fixed:
?{VER(<8.0.11)}
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
Do you want to perform the required configuration changes? [y/n]:
Creating user repl_admin@%.
Account repl_admin@% was successfully created.

Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB Cluster.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.
?{}
?{VER(>=8.0.11) && VER(<8.0.21)}
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

Some variables need to be changed, but cannot be done dynamically on the server.
Do you want to perform the required configuration changes? [y/n]: Do you want to restart the instance after configuring it? [y/n]:
Creating user repl_admin@%.
Account repl_admin@% was successfully created.

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

Some variables need to be changed, but cannot be done dynamically on the server.
Do you want to perform the required configuration changes? [y/n]: Do you want to restart the instance after configuring it? [y/n]:
Creating user repl_admin@%.
Account repl_admin@% was successfully created.

Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB Cluster.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.
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

Some variables need to be changed, but cannot be done dynamically on the server.
Do you want to perform the required configuration changes? [y/n]: Do you want to restart the instance after configuring it? [y/n]:
Creating user repl_admin@%.
Account repl_admin@% was successfully created.

Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB Cluster.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.
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

Some variables need to be changed, but cannot be done dynamically on the server.
Do you want to perform the required configuration changes? [y/n]: Do you want to restart the instance after configuring it? [y/n]:
Creating user repl_admin@%.
Account repl_admin@% was successfully created.

Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB Cluster.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.
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

Some variables need to be changed, but cannot be done dynamically on the server.
Do you want to perform the required configuration changes? [y/n]: Do you want to restart the instance after configuring it? [y/n]:
Creating user repl_admin@%.
Account repl_admin@% was successfully created.

Configuring instance...
?{}
?{VER(>=8.3.0) && VER(<8.4.0)}
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

Some variables need to be changed, but cannot be done dynamically on the server.
Do you want to perform the required configuration changes? [y/n]: Do you want to restart the instance after configuring it? [y/n]:
Creating user repl_admin@%.
Account repl_admin@% was successfully created.

Configuring instance...
?{}
?{VER(>=8.4.0)}
+-------------------------------+---------------+----------------+-----------------------------------------------+
| Variable                      | Current Value | Required Value | Note                                          |
+-------------------------------+---------------+----------------+-----------------------------------------------+
| binlog_format                 | <not set>     | ROW            | Update the config file                        |
| enforce_gtid_consistency      | OFF           | ON             | Update the config file and restart the server |
| gtid_mode                     | OFF           | ON             | Update the config file and restart the server |
| replica_preserve_commit_order | <not set>     | ON             | Update the config file                        |
| report_port                   | <not set>     | <<<__mysql_sandbox_port1>>>           | Update the config file                        |
| server_id                     | 1             | <unique ID>    | Update the config file and restart the server |
+-------------------------------+---------------+----------------+-----------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
Do you want to perform the required configuration changes? [y/n]: Do you want to restart the instance after configuring it? [y/n]:
Creating user repl_admin@%.
Account repl_admin@% was successfully created.

Configuring instance...
?{}
?{((VER(>=8.0.35) && VER(<8.1.0)) || (VER(>=8.2.0) && VER(<8.3.0))) && !__replaying}

WARNING: '@@binlog_transaction_dependency_tracking' is deprecated and will be removed in a future release. (Code 1287).
?{}
?{VER(>=8.0.27)}
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB Cluster.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.
?{}

//@<OUT> configureInstance custom cluster admin and no password
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB Cluster...

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

ERROR: User 'root' can only connect from 'localhost'. New account(s) with proper source address specification to allow remote connection from all instances must be created to manage the cluster.

1) Create remotely usable account for 'root' with same grants and password
2) Create a new admin account for InnoDB Cluster with minimal required grants
3) Ignore and continue
4) Cancel

Please select an option [1]: Please provide an account name (e.g: icroot@%) to have it created with the necessary
privileges or leave empty and press Enter to cancel.
Account Name: Password for new account: Confirm password:
?{VER(>=8.0.23)}
applierWorkerThreads will be set to the default value of 4.

?{}
NOTE: Some configuration options need to be fixed:
?{VER(<8.0.11)}
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
Do you want to perform the required configuration changes? [y/n]:
Creating user repl_admin2@%.
Account repl_admin2@% was successfully created.

Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB Cluster.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.
?{}
?{VER(>=8.0.11)}
+--------------------------+---------------+----------------+--------------------------------------------------+
| Variable                 | Current Value | Required Value | Note                                             |
+--------------------------+---------------+----------------+--------------------------------------------------+
| enforce_gtid_consistency | OFF           | ON             | Restart the server                               |
| gtid_mode                | OFF           | ON             | Restart the server                               |
| server_id                | 1             | <unique ID>    | Update read-only variable and restart the server |
+--------------------------+---------------+----------------+--------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
Do you want to perform the required configuration changes? [y/n]: Do you want to restart the instance after configuring it? [y/n]:
Creating user repl_admin2@%.
Account repl_admin2@% was successfully created.

Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB Cluster.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.
?{}

//@<OUT> test connection with custom cluster admin and no password
No default schema selected; type \use <schema> to set one.
<ClassicSession:repl_admin2@localhost:<<<__mysql_sandbox_port1>>>>

//@<OUT> Verify that configureInstance() detects and fixes the wrong settings {VER(>=8.0.23)}
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' belongs to an InnoDB Cluster.
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB Cluster...

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

applierWorkerThreads will be set to the default value of 4.

NOTE: Some configuration options need to be fixed:
?{VER(>8.0.25)}
+-------------------------------+---------------+----------------+----------------------------+
| Variable                      | Current Value | Required Value | Note                       |
+-------------------------------+---------------+----------------+----------------------------+
| <<<__replica_keyword>>>_preserve_commit_order | OFF           | ON             | Update the server variable |
+-------------------------------+---------------+----------------+----------------------------+
?{}
?{VER(<=8.0.25)}
+-----------------------------+---------------+----------------+----------------------------+
| Variable                    | Current Value | Required Value | Note                       |
+-----------------------------+---------------+----------------+----------------------------+
| <<<__replica_keyword>>>_preserve_commit_order | OFF           | ON             | Update the server variable |
+-----------------------------+---------------+----------------+----------------------------+
?{}

Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB Cluster.

//@<OUT> canonical IPv6 addresses are supported WL#12758 {VER(>= 8.0.14)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB Cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as [::1]:<<<__mysql_sandbox_port1>>>

?{VER(>=8.0.23)}
applierWorkerThreads will be set to the default value of 4.

?{}
The instance '[::1]:<<<__mysql_sandbox_port1>>>' is valid for InnoDB Cluster usage.

//@<OUT> canonical IPv4 addresses are supported WL#12758
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB Cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as 127.0.0.1:<<<__mysql_sandbox_port1>>>

?{VER(>=8.0.23)}
applierWorkerThreads will be set to the default value of 4.

?{}
The instance '127.0.0.1:<<<__mysql_sandbox_port1>>>' is valid for InnoDB Cluster usage.

//@ IPv6 not supported on versions below 8.0.14 WL#12758 {VER(< 8.0.14)}
|ERROR: Cannot use host '::1' for instance '[::1]:<<<__mysql_sandbox_port1>>>' because it is an IPv6 address which is only supported by Group Replication from MySQL version >= 8.0.14. Set the MySQL server 'report_host' variable to an IPv4 address or hostname that resolves an IPv4 address.|
||Unsupported IP address '::1'. IPv6 is only supported by Group Replication on MySQL version >= 8.0.14. (RuntimeError)

//@ create cluster admin
|Account ca@% was successfully created.|

//@<OUT> check global privileges of cluster admin {VER(<8.0.11)}
+--------------------+--------------+
| PRIVILEGE_TYPE     | IS_GRANTABLE |
+--------------------+--------------+
| CREATE USER        | YES          |
| FILE               | YES          |
| PROCESS            | YES          |
| RELOAD             | YES          |
| REPLICATION CLIENT | YES          |
| REPLICATION SLAVE  | YES          |
| SELECT             | YES          |
| SHUTDOWN           | YES          |
| SUPER              | YES          |
+--------------------+--------------+

//@<OUT> check global privileges of cluster admin {VER(>=8.0.11) && VER(<8.0.17)}
+----------------------------+--------------+
| PRIVILEGE_TYPE             | IS_GRANTABLE |
+----------------------------+--------------+
| CONNECTION_ADMIN           | YES          |
| CREATE USER                | YES          |
| FILE                       | YES          |
| GROUP_REPLICATION_ADMIN    | YES          |
| PERSIST_RO_VARIABLES_ADMIN | YES          |
| PROCESS                    | YES          |
| RELOAD                     | YES          |
| REPLICATION CLIENT         | YES          |
| REPLICATION SLAVE          | YES          |
| REPLICATION_SLAVE_ADMIN    | YES          |
| ROLE_ADMIN                 | YES          |
| SELECT                     | YES          |
| SHUTDOWN                   | YES          |
| SYSTEM_VARIABLES_ADMIN     | YES          |
+----------------------------+--------------+

//@<OUT> check global privileges of cluster admin {VER(>=8.0.17)}
+----------------------------+--------------+
| PRIVILEGE_TYPE             | IS_GRANTABLE |
+----------------------------+--------------+
| CLONE_ADMIN                | YES          |
| CONNECTION_ADMIN           | YES          |
| CREATE USER                | YES          |
| EXECUTE                    | YES          |
| FILE                       | YES          |
| GROUP_REPLICATION_ADMIN    | YES          |
| PERSIST_RO_VARIABLES_ADMIN | YES          |
| PROCESS                    | YES          |
| RELOAD                     | YES          |
| REPLICATION CLIENT         | YES          |
| REPLICATION SLAVE          | YES          |<<<(__version_num>=80018) ?  "\n| REPLICATION_APPLIER        | YES          |":"">>>
| REPLICATION_SLAVE_ADMIN    | YES          |
| ROLE_ADMIN                 | YES          |
| SELECT                     | YES          |
| SHUTDOWN                   | YES          |
| SYSTEM_VARIABLES_ADMIN     | YES          |<<<(__version_num>=80300) ?  "\n| TRANSACTION_GTID_TAG       | YES          |":"">>>
+----------------------------+--------------+

//@<OUT> check schema privileges of cluster admin
+-------------------------+--------------+----------------------------------------+
| PRIVILEGE_TYPE          | IS_GRANTABLE | TABLE_SCHEMA                           |
+-------------------------+--------------+----------------------------------------+
| DELETE                  | YES          | mysql                                  |
| INSERT                  | YES          | mysql                                  |
| UPDATE                  | YES          | mysql                                  |
| ALTER                   | YES          | mysql_innodb_cluster_metadata          |
| ALTER ROUTINE           | YES          | mysql_innodb_cluster_metadata          |
| CREATE                  | YES          | mysql_innodb_cluster_metadata          |
| CREATE ROUTINE          | YES          | mysql_innodb_cluster_metadata          |
| CREATE TEMPORARY TABLES | YES          | mysql_innodb_cluster_metadata          |
| CREATE VIEW             | YES          | mysql_innodb_cluster_metadata          |
| DELETE                  | YES          | mysql_innodb_cluster_metadata          |
| DROP                    | YES          | mysql_innodb_cluster_metadata          |
| EVENT                   | YES          | mysql_innodb_cluster_metadata          |
| EXECUTE                 | YES          | mysql_innodb_cluster_metadata          |
| INDEX                   | YES          | mysql_innodb_cluster_metadata          |
| INSERT                  | YES          | mysql_innodb_cluster_metadata          |
| LOCK TABLES             | YES          | mysql_innodb_cluster_metadata          |
| REFERENCES              | YES          | mysql_innodb_cluster_metadata          |
| SHOW VIEW               | YES          | mysql_innodb_cluster_metadata          |
| TRIGGER                 | YES          | mysql_innodb_cluster_metadata          |
| UPDATE                  | YES          | mysql_innodb_cluster_metadata          |
| ALTER                   | YES          | mysql_innodb_cluster_metadata_bkp      |
| ALTER ROUTINE           | YES          | mysql_innodb_cluster_metadata_bkp      |
| CREATE                  | YES          | mysql_innodb_cluster_metadata_bkp      |
| CREATE ROUTINE          | YES          | mysql_innodb_cluster_metadata_bkp      |
| CREATE TEMPORARY TABLES | YES          | mysql_innodb_cluster_metadata_bkp      |
| CREATE VIEW             | YES          | mysql_innodb_cluster_metadata_bkp      |
| DELETE                  | YES          | mysql_innodb_cluster_metadata_bkp      |
| DROP                    | YES          | mysql_innodb_cluster_metadata_bkp      |
| EVENT                   | YES          | mysql_innodb_cluster_metadata_bkp      |
| EXECUTE                 | YES          | mysql_innodb_cluster_metadata_bkp      |
| INDEX                   | YES          | mysql_innodb_cluster_metadata_bkp      |
| INSERT                  | YES          | mysql_innodb_cluster_metadata_bkp      |
| LOCK TABLES             | YES          | mysql_innodb_cluster_metadata_bkp      |
| REFERENCES              | YES          | mysql_innodb_cluster_metadata_bkp      |
| SHOW VIEW               | YES          | mysql_innodb_cluster_metadata_bkp      |
| TRIGGER                 | YES          | mysql_innodb_cluster_metadata_bkp      |
| UPDATE                  | YES          | mysql_innodb_cluster_metadata_bkp      |
| ALTER                   | YES          | mysql_innodb_cluster_metadata_previous |
| ALTER ROUTINE           | YES          | mysql_innodb_cluster_metadata_previous |
| CREATE                  | YES          | mysql_innodb_cluster_metadata_previous |
| CREATE ROUTINE          | YES          | mysql_innodb_cluster_metadata_previous |
| CREATE TEMPORARY TABLES | YES          | mysql_innodb_cluster_metadata_previous |
| CREATE VIEW             | YES          | mysql_innodb_cluster_metadata_previous |
| DELETE                  | YES          | mysql_innodb_cluster_metadata_previous |
| DROP                    | YES          | mysql_innodb_cluster_metadata_previous |
| EVENT                   | YES          | mysql_innodb_cluster_metadata_previous |
| EXECUTE                 | YES          | mysql_innodb_cluster_metadata_previous |
| INDEX                   | YES          | mysql_innodb_cluster_metadata_previous |
| INSERT                  | YES          | mysql_innodb_cluster_metadata_previous |
| LOCK TABLES             | YES          | mysql_innodb_cluster_metadata_previous |
| REFERENCES              | YES          | mysql_innodb_cluster_metadata_previous |
| SHOW VIEW               | YES          | mysql_innodb_cluster_metadata_previous |
| TRIGGER                 | YES          | mysql_innodb_cluster_metadata_previous |
| UPDATE                  | YES          | mysql_innodb_cluster_metadata_previous |
+-------------------------+--------------+----------------------------------------+


//@<OUT> check table privileges of cluster admin
Empty set ([[*]])

//@ cluster admin should be able to create another cluster admin
|Account ca2@% was successfully created.|

//@ Deploy instances (with invalid server_id).
||

//@ Deploy instances (with invalid server_id in 8.0). {VER(>=8.0.3)}
||

//@<OUT> checkInstanceConfiguration with server_id error. {VER(>=8.0.11)}
{
    "config_errors": [
        {
            "action": "config_update+restart",
            "current": "0",
            "option": "server_id",
            "required": "<unique ID>"
        }
    ],
    "status": "error"
}

//@<OUT> checkInstanceConfiguration with server_id error. {VER(<8.0.11)}
NOTE: Some configuration options need to be fixed:
+-----------+---------------+----------------+-----------------------------------------------+
| Variable  | Current Value | Required Value | Note                                          |
+-----------+---------------+----------------+-----------------------------------------------+
| server_id | 0             | <unique ID>    | Update the config file and restart the server |
+-----------+---------------+----------------+-----------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server: an option file is required.
NOTE: Please use the dba.configureInstance() command to repair these issues.

{
    "config_errors": [
        {
            "action": "config_update+restart",
            "current": "0",
            "option": "server_id",
            "required": "<unique ID>"
        }
    ],
    "status": "error"
}

//@<OUT> configureInstance server_id updated but needs restart. {VER(>=8.0.11)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB Cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

?{VER(>=8.0.23)}
applierWorkerThreads will be set to the default value of 4.

?{}
NOTE: Some configuration options need to be fixed:
+-----------+---------------+----------------+-----------------------------------------------+
| Variable  | Current Value | Required Value | Note                                          |
+-----------+---------------+----------------+-----------------------------------------------+
| server_id | 0             | <unique ID>    | Update the config file and restart the server |
+-----------+---------------+----------------+-----------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB Cluster.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.

//@<OUT> configureInstance server_id updated but needs restart. {VER(<8.0.11)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB Cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

NOTE: Some configuration options need to be fixed:
+-----------+---------------+----------------+-----------------------------------------------+
| Variable  | Current Value | Required Value | Note                                          |
+-----------+---------------+----------------+-----------------------------------------------+
| server_id | 0             | <unique ID>    | Update the config file and restart the server |
+-----------+---------------+----------------+-----------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server: an option file is required.
Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB Cluster.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.

//@<OUT> configureInstance still indicate that a restart is needed. {VER(<8.0.11)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB Cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

NOTE: Some configuration options need to be fixed:
+-----------+---------------+----------------+--------------------------------------------------+
| Variable  | Current Value | Required Value | Note                                             |
+-----------+---------------+----------------+--------------------------------------------------+
| server_id | 0             | <unique ID>    | Update read-only variable and restart the server |
+-----------+---------------+----------------+--------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB Cluster.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.

//@<OUT> configureInstance still indicate that a restart is needed. {VER(>=8.0.11)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB Cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

?{VER(>=8.0.23)}
applierWorkerThreads will be set to the default value of 4.

?{}
NOTE: Some configuration options need to be fixed:
+-----------+---------------+----------------+--------------------------------------------------+
| Variable  | Current Value | Required Value | Note                                             |
+-----------+---------------+----------------+--------------------------------------------------+
| server_id | 0             | <unique ID>    | Update read-only variable and restart the server |
+-----------+---------------+----------------+--------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB Cluster.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.

//@ Restart sandbox 1.
||

//@<OUT> configureInstance no issues after restart for sandobox 1.
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB Cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

?{VER(>=8.0.23)}
applierWorkerThreads will be set to the default value of 4.

?{}
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is valid for InnoDB Cluster usage.

?{VER(>=8.0.23)}
Successfully enabled parallel appliers.
?{}

//@<OUT> checkInstanceConfiguration no server_id in my.cnf (error). {VER(>=8.0.3)}
{
    "config_errors": [
        {
            "action": "config_update+restart",
            "current": "1",
            "option": "server_id",
            "required": "<unique ID>"
        }
    ],
    "status": "error"
}

//@<OUT> configureInstance no server_id in my.cnf (needs restart). {VER(>=8.0.3)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB Cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port2>>>

?{VER(>=8.0.23)}
applierWorkerThreads will be set to the default value of 4.

?{}
NOTE: Some configuration options need to be fixed:
+-----------+---------------+----------------+-----------------------------------------------+
| Variable  | Current Value | Required Value | Note                                          |
+-----------+---------------+----------------+-----------------------------------------------+
| server_id | 1             | <unique ID>    | Update the config file and restart the server |
+-----------+---------------+----------------+-----------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was configured to be used in an InnoDB Cluster.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.

//@<OUT> configureInstance no server_id in my.cnf (still needs restart). {VER(>=8.0.3)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB Cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port2>>>

?{VER(>=8.0.23)}
applierWorkerThreads will be set to the default value of 4.

?{}
NOTE: Some configuration options need to be fixed:
+-----------+---------------+----------------+--------------------------------------------------+
| Variable  | Current Value | Required Value | Note                                             |
+-----------+---------------+----------------+--------------------------------------------------+
| server_id | 1             | <unique ID>    | Update read-only variable and restart the server |
+-----------+---------------+----------------+--------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was configured to be used in an InnoDB Cluster.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.

//@ Restart sandbox 2. {VER(>=8.0.3)}
||

//@<OUT> configureInstance no issues after restart for sandbox 2. {VER(>=8.0.3)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB Cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port2>>>

?{VER(>=8.0.23)}
applierWorkerThreads will be set to the default value of 4.

?{}
The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is valid for InnoDB Cluster usage.

?{VER(>=8.0.23)}
Successfully enabled parallel appliers.
?{}

//@ Clean-up deployed instances.
||

//@ Clean-up deployed instances (with invalid server_id in 8.0). {VER(>=8.0.3)}
||







//@ ConfigureInstance should fail if there's no session nor parameters provided
||An open session is required to perform this operation.

//@# Error no write privileges
|The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' belongs to an InnoDB cluster.|
|ERROR: mycnfPath is not writable: <<<cnfPath>>>: Permission denied|
||Unable to update MySQL configuration file

//@ Close session
||

//@ IPv6 not supported on versions below 8.0.14 WL#12758 {VER(< 8.0.14)}
|ERROR: Cannot use host '::1' for instance '[::1]:<<<__mysql_sandbox_port1>>>' because it is an IPv6 address which is only supported by Group Replication from MySQL version >= 8.0.14. Set the MySQL server 'report_host' variable to an IPv4 address or hostname that resolves an IPv4 address.|
||Unsupported IP address '::1'. IPv6 is only supported by Group Replication on MySQL version >= 8.0.14. (RuntimeError)
