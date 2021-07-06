//@<OUT> configureInstance custom cluster admin and password
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

ERROR: User 'root' can only connect from 'localhost'. New account(s) with proper source address specification to allow remote connection from all instances must be created to manage the cluster.

1) Create remotely usable account for 'root' with same grants and password
2) Create a new admin account for InnoDB cluster with minimal required grants
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
Do you want to perform the required configuration changes? [y/n]:
Cluster admin user 'repl_admin'@'%' created.
Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB cluster.
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
Cluster admin user 'repl_admin'@'%' created.
Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB cluster.
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
Cluster admin user 'repl_admin'@'%' created.
Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB cluster.
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
Cluster admin user 'repl_admin'@'%' created.
Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB cluster.
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
Cluster admin user 'repl_admin'@'%' created.
Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB cluster.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.
?{}
?{VER(>=8.0.27)}
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
Cluster admin user 'repl_admin'@'%' created.
Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB cluster.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.
?{}

//@<OUT> test connection with custom cluster admin and password
No default schema selected; type \use <schema> to set one.
<ClassicSession:repl_admin@localhost:<<<__mysql_sandbox_port1>>>>

//@ test configureInstance providing clusterAdminPassword without clusterAdmin
||The clusterAdminPassword is allowed only if clusterAdmin is specified.

//@ test configureInstance providing clusterAdminPassword and an existing clusterAdmin
||The 'repl_admin'@'%' account already exists, clusterAdminPassword is not allowed for an existing account.

//@<OUT> configureInstance custom cluster admin and no password
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

ERROR: User 'root' can only connect from 'localhost'. New account(s) with proper source address specification to allow remote connection from all instances must be created to manage the cluster.

1) Create remotely usable account for 'root' with same grants and password
2) Create a new admin account for InnoDB cluster with minimal required grants
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
| log_bin                          | OFF           | ON             | Update read-only variable and restart the server |
| log_slave_updates                | OFF           | ON             | Update read-only variable and restart the server |
| master_info_repository           | FILE          | TABLE          | Update read-only variable and restart the server |
| relay_log_info_repository        | FILE          | TABLE          | Update read-only variable and restart the server |
| server_id                        | 0             | <unique ID>    | Update read-only variable and restart the server |
| transaction_write_set_extraction | OFF           | XXHASH64       | Update read-only variable and restart the server |
+----------------------------------+---------------+----------------+--------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
Do you want to perform the required configuration changes? [y/n]:
Cluster admin user 'repl_admin2'@'%' created.
Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB cluster.
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
Cluster admin user 'repl_admin2'@'%' created.
Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB cluster.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.
?{}

//@<OUT> test connection with custom cluster admin and no password
No default schema selected; type \use <schema> to set one.
<ClassicSession:repl_admin2@localhost:<<<__mysql_sandbox_port1>>>>

//@<OUT> Verify that configureInstance() detects and fixes the wrong settings {VER(>=8.0.23)}
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' belongs to an InnoDB Cluster.
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...

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
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB cluster.

//@<OUT> canonical IPv6 addresses are supported WL#12758 {VER(>= 8.0.14)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as [::1]:<<<__mysql_sandbox_port1>>>

?{VER(>=8.0.23)}
applierWorkerThreads will be set to the default value of 4.

?{}
The instance '[::1]:<<<__mysql_sandbox_port1>>>' is valid to be used in an InnoDB cluster.
The instance '[::1]:<<<__mysql_sandbox_port1>>>' is already ready to be used in an InnoDB cluster.

//@<OUT> canonical IPv4 addresses are supported WL#12758
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as 127.0.0.1:<<<__mysql_sandbox_port1>>>

?{VER(>=8.0.23)}
applierWorkerThreads will be set to the default value of 4.

?{}
The instance '127.0.0.1:<<<__mysql_sandbox_port1>>>' is valid to be used in an InnoDB cluster.
The instance '127.0.0.1:<<<__mysql_sandbox_port1>>>' is already ready to be used in an InnoDB cluster.

//@ IPv6 not supported on versions below 8.0.14 WL#12758 {VER(< 8.0.14)}
|ERROR: Cannot use host '::1' for instance '[::1]:<<<__mysql_sandbox_port1>>>' because it is an IPv6 address which is only supported by Group Replication from MySQL version >= 8.0.14. Set the MySQL server 'report_host' variable to an IPv4 address or hostname that resolves an IPv4 address.|
||Unsupported IP address '::1'. IPv6 is only supported by Group Replication on MySQL version >= 8.0.14. (RuntimeError)
