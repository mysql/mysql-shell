//@ deploy the sandbox
||

//@ ConfigureLocalInstance should fail if there's no session nor parameters provided
||An open session is required to perform this operation.

//@<OUT> Interactive_dba_configure_local_instance read_only_no_prompts {VER(>=8.0)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...

This instance reports its own address as <<<hostname>>>
Assuming full account name 'root'@'%' for root

Some configuration options need to be fixed:
+----------------------------------+---------------+----------------+--------------------------------------------------+
| Variable                         | Current Value | Required Value | Note                                             |
+----------------------------------+---------------+----------------+--------------------------------------------------+
| binlog_checksum                  | CRC32         | NONE           | Update the server variable and the config file   |
| binlog_format                    | <not set>     | ROW            | Update the config file                           |
| enforce_gtid_consistency         | OFF           | ON             | Update the config file and restart the server    |
| gtid_mode                        | OFF           | ON             | Update the config file and restart the server    |
| log_bin                          | <not set>     | <no value>     | Update the config file                           |
| log_slave_updates                | <not set>     | ON             | Update the config file                           |
| master_info_repository           | <not set>     | TABLE          | Update the config file                           |
| relay_log_info_repository        | <not set>     | TABLE          | Update the config file                           |
| report_port                      | <not set>     | <<<__mysql_sandbox_port1>>>           | Update the config file                           |
| server_id                        | 1             | <unique ID>    | Update read-only variable and restart the server |
| transaction_write_set_extraction | <not set>     | XXHASH64       | Update the config file                           |
+----------------------------------+---------------+----------------+--------------------------------------------------+

The following variable needs to be changed, but cannot be done dynamically: 'log_bin'
Cluster admin user 'root'@'%' created.
Configuring instance...
The instance 'localhost:<<<__mysql_sandbox_port1>>>' was configured for cluster usage.
MySQL server needs to be restarted for configuration changes to take effect.

//@<OUT> Interactive_dba_configure_local_instance read_only_no_prompts {VER(<8.0)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...

This instance reports its own address as <<<hostname>>>
Assuming full account name 'root'@'%' for root

Some configuration options need to be fixed:
+----------------------------------+---------------+----------------+--------------------------------------------------+
| Variable                         | Current Value | Required Value | Note                                             |
+----------------------------------+---------------+----------------+--------------------------------------------------+
| binlog_checksum                  | CRC32         | NONE           | Update the server variable and the config file   |
| binlog_format                    | <not set>     | ROW            | Update the config file                           |
| enforce_gtid_consistency         | OFF           | ON             | Update the config file and restart the server    |
| gtid_mode                        | OFF           | ON             | Update the config file and restart the server    |
| log_bin                          | 0             | 1              | Update the config file and restart the server    |
| log_slave_updates                | 0             | ON             | Update the config file and restart the server    |
| master_info_repository           | FILE          | TABLE          | Update the config file and restart the server    |
| relay_log_info_repository        | FILE          | TABLE          | Update the config file and restart the server    |
| report_port                      | <not set>     | <<<__mysql_sandbox_port1>>>           | Update the config file                           |
| server_id                        | 0             | <unique ID>    | Update read-only variable and restart the server |
| transaction_write_set_extraction | OFF           | XXHASH64       | Update the config file and restart the server    |
+----------------------------------+---------------+----------------+--------------------------------------------------+

The following variable needs to be changed, but cannot be done dynamically: 'log_bin'
Cluster admin user 'root'@'%' created.
Configuring instance...
The instance 'localhost:<<<__mysql_sandbox_port1>>>' was configured for cluster usage.
MySQL server needs to be restarted for configuration changes to take effect.

//@<OUT> Interactive_dba_configure_local_instance read_only_no_flag_prompt_yes 8.0 {VER(>=8.0.11)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...

This instance reports its own address as <<<hostname>>>
Assuming full account name 'root2'@'%' for root2

Some configuration options need to be fixed:
+--------------------------+---------------+----------------+--------------------------------------------------+
| Variable                 | Current Value | Required Value | Note                                             |
+--------------------------+---------------+----------------+--------------------------------------------------+
| enforce_gtid_consistency | OFF           | ON             | Update read-only variable and restart the server |
| gtid_mode                | OFF           | ON             | Update read-only variable and restart the server |
| server_id                | 1             | <unique ID>    | Update read-only variable and restart the server |
+--------------------------+---------------+----------------+--------------------------------------------------+

Do you want to perform the required configuration changes? [y/n]: Do you want to restart the instance after configuring it? [y/n]:
The MySQL instance at 'localhost:<<<__mysql_sandbox_port1>>>' currently has the super_read_only
system variable set to protect it from inadvertent updates from applications.
You must first unset it to be able to perform any changes to this instance.
For more information see: https://dev.mysql.com/doc/refman/en/server-system-variables.html#sysvar_super_read_only.

Note: there are open sessions to 'localhost:<<<__mysql_sandbox_port1>>>'.
You may want to kill these sessions to prevent them from performing unexpected updates:

1 open session(s) of 'root@localhost'.

Do you want to disable super_read_only and continue? [y/N]: Disabled super_read_only on the instance 'localhost:<<<__mysql_sandbox_port1>>>'
Cluster admin user 'root2'@'%' created.
Enabling super_read_only on the instance 'localhost:<<<__mysql_sandbox_port1>>>'

//@<OUT> Interactive_dba_configure_local_instance read_only_no_flag_prompt_yes 5.7 {VER(<8.0.11)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...

This instance reports its own address as <<<hostname>>>
Assuming full account name 'root2'@'%' for root2

Some configuration options need to be fixed:
+----------------------------------+---------------+----------------+--------------------------------------------------+
| Variable                         | Current Value | Required Value | Note                                             |
+----------------------------------+---------------+----------------+--------------------------------------------------+
| enforce_gtid_consistency         | OFF           | ON             | Update read-only variable and restart the server |
| gtid_mode                        | OFF           | ON             | Update read-only variable and restart the server |
| log_bin                          | 0             | 1              | Update read-only variable and restart the server |
| log_slave_updates                | 0             | ON             | Update read-only variable and restart the server |
| master_info_repository           | FILE          | TABLE          | Update read-only variable and restart the server |
| relay_log_info_repository        | FILE          | TABLE          | Update read-only variable and restart the server |
| server_id                        | 0             | <unique ID>    | Update read-only variable and restart the server |
| transaction_write_set_extraction | OFF           | XXHASH64       | Update read-only variable and restart the server |
+----------------------------------+---------------+----------------+--------------------------------------------------+

The following variable needs to be changed, but cannot be done dynamically: 'log_bin'
Do you want to perform the required configuration changes? [y/n]:
The MySQL instance at 'localhost:<<<__mysql_sandbox_port1>>>' currently has the super_read_only
system variable set to protect it from inadvertent updates from applications.
You must first unset it to be able to perform any changes to this instance.
For more information see: https://dev.mysql.com/doc/refman/en/server-system-variables.html#sysvar_super_read_only.

Note: there are open sessions to 'localhost:<<<__mysql_sandbox_port1>>>'.
You may want to kill these sessions to prevent them from performing unexpected updates:

1 open session(s) of 'root@localhost'.

Do you want to disable super_read_only and continue? [y/N]: Disabled super_read_only on the instance 'localhost:<<<__mysql_sandbox_port1>>>'
Cluster admin user 'root2'@'%' created.
Enabling super_read_only on the instance 'localhost:<<<__mysql_sandbox_port1>>>'

//@ Interactive_dba_configure_local_instance read_only_no_flag_prompt_no 8.0 {VER(>=8.0.11)}
||Dba.configureLocalInstance: Cancelled (RuntimeError)

//@ Interactive_dba_configure_local_instance read_only_no_flag_prompt_no 5.7 {VER(<8.0.11)}
||Dba.configureLocalInstance: Cancelled (RuntimeError)

//@ Interactive_dba_configure_local_instance read_only_invalid_flag_value
||Dba.configureLocalInstance: Option 'clearReadOnly' is expected to be of type Bool, but is String

//@<OUT> Interactive_dba_configure_local_instance read_only_flag_true 8.0 {VER(>=8.0.11)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...

This instance reports its own address as <<<hostname>>>
Assuming full account name 'root5'@'%' for root5

Some configuration options need to be fixed:
+--------------------------+---------------+----------------+--------------------------------------------------+
| Variable                 | Current Value | Required Value | Note                                             |
+--------------------------+---------------+----------------+--------------------------------------------------+
| enforce_gtid_consistency | OFF           | ON             | Update read-only variable and restart the server |
| gtid_mode                | OFF           | ON             | Update read-only variable and restart the server |
| server_id                | 1             | <unique ID>    | Update read-only variable and restart the server |
+--------------------------+---------------+----------------+--------------------------------------------------+

Do you want to perform the required configuration changes? [y/n]: Do you want to restart the instance after configuring it? [y/n]: Disabled super_read_only on the instance 'localhost:<<<__mysql_sandbox_port1>>>'
Cluster admin user 'root5'@'%' created.
Enabling super_read_only on the instance 'localhost:<<<__mysql_sandbox_port1>>>'

//@<OUT> Interactive_dba_configure_local_instance read_only_flag_true 5.7 {VER(<8.0.11)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...

This instance reports its own address as <<<hostname>>>
Assuming full account name 'root5'@'%' for root5

Some configuration options need to be fixed:
+----------------------------------+---------------+----------------+--------------------------------------------------+
| Variable                         | Current Value | Required Value | Note                                             |
+----------------------------------+---------------+----------------+--------------------------------------------------+
| enforce_gtid_consistency         | OFF           | ON             | Update read-only variable and restart the server |
| gtid_mode                        | OFF           | ON             | Update read-only variable and restart the server |
| log_bin                          | 0             | 1              | Update read-only variable and restart the server |
| log_slave_updates                | 0             | ON             | Update read-only variable and restart the server |
| master_info_repository           | FILE          | TABLE          | Update read-only variable and restart the server |
| relay_log_info_repository        | FILE          | TABLE          | Update read-only variable and restart the server |
| server_id                        | 0             | <unique ID>    | Update read-only variable and restart the server |
| transaction_write_set_extraction | OFF           | XXHASH64       | Update read-only variable and restart the server |
+----------------------------------+---------------+----------------+--------------------------------------------------+

The following variable needs to be changed, but cannot be done dynamically: 'log_bin'
Do you want to perform the required configuration changes? [y/n]: Disabled super_read_only on the instance 'localhost:<<<__mysql_sandbox_port1>>>'
Cluster admin user 'root5'@'%' created.
Enabling super_read_only on the instance 'localhost:<<<__mysql_sandbox_port1>>>'

//@ Interactive_dba_configure_local_instance read_only_flag_false 8.0 {VER(>=8.0.11)}
||

//@ Interactive_dba_configure_local_instance read_only_flag_false 5.7 {VER(<8.0.11)}
||

//@ Cleanup
||
