// Tests for configure instance (and check instance) bugs

//@ BUG#28727505: Initialization.
||

//@<OUT> BUG#28727505: configure instance 5.7. {VER(<8.0.11)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

NOTE: Some configuration options need to be fixed:
+-----------+---------------+----------------+--------------------------------------------------+
| Variable  | Current Value | Required Value | Note                                             |
+-----------+---------------+----------------+--------------------------------------------------+
| gtid_mode | OFF           | ON             | Update read-only variable and restart the server |
+-----------+---------------+----------------+--------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB cluster.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.

//@<OUT> BUG#28727505: configure instance again 5.7. {VER(<8.0.11)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

NOTE: Some configuration options need to be fixed:
+-----------+---------------+----------------+--------------------------------------------------+
| Variable  | Current Value | Required Value | Note                                             |
+-----------+---------------+----------------+--------------------------------------------------+
| gtid_mode | OFF           | ON             | Update read-only variable and restart the server |
+-----------+---------------+----------------+--------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB cluster.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.

//@<OUT> BUG#28727505: configure instance. {VER(>=8.0.11)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
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
| gtid_mode | OFF           | ON             | Update read-only variable and restart the server |
+-----------+---------------+----------------+--------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB cluster.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.

//@<OUT> BUG#28727505: configure instance again. {VER(>=8.0.11)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

?{VER(>=8.0.23)}
applierWorkerThreads will be set to the default value of 4.

?{}
NOTE: Some configuration options need to be fixed:
+-----------+---------------+----------------+--------------------+
| Variable  | Current Value | Required Value | Note               |
+-----------+---------------+----------------+--------------------+
| gtid_mode | OFF           | ON             | Restart the server |
+-----------+---------------+----------------+--------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB cluster.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.

//@ BUG#28727505: clean-up.
||

//@ BUG#29765093: Initialization. {VER(>=8.0.11)}
||

//@ BUG#29765093: Change some persisted (only) values. {VER(>=8.0.11)}
||

//@<OUT> BUG#29765093: check instance configuration instance. {VER(>=8.0.11)}
Validating local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected

Checking instance configuration...

NOTE: Some configuration options need to be fixed:
+---------------+---------------+----------------+----------------------------+
| Variable      | Current Value | Required Value | Note                       |
+---------------+---------------+----------------+----------------------------+
| binlog_format | MIXED         | ROW            | Update the server variable |
| gtid_mode     | ON            | ON             | Update the server variable |
+---------------+---------------+----------------+----------------------------+

NOTE: Please use the dba.configureInstance() command to repair these issues.

{
    "config_errors": [
        {
            "action": "server_update",
            "current": "MIXED",
            "option": "binlog_format",
            "persisted": "STATEMENT",
            "required": "ROW"
        },
        {
            "action": "server_update",
            "current": "ON",
            "option": "gtid_mode",
            "persisted": "OFF",
            "required": "ON"
        }
    ],
    "status": "error"
}


//@<OUT> BUG#29765093: configure instance. {VER(>=8.0.11)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

?{VER(>=8.0.23)}
applierWorkerThreads will be set to the default value of 4.

?{}
NOTE: Some configuration options need to be fixed:
+---------------+---------------+----------------+----------------------------+
| Variable      | Current Value | Required Value | Note                       |
+---------------+---------------+----------------+----------------------------+
| binlog_format | MIXED         | ROW            | Update the server variable |
| gtid_mode     | ON            | ON             | Update the server variable |
+---------------+---------------+----------------+----------------------------+

Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB cluster.

//@<OUT> BUG#29765093: configure instance again. {VER(>=8.0.11)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

?{VER(>=8.0.23)}
applierWorkerThreads will be set to the default value of 4.

?{}
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is valid to be used in an InnoDB cluster.
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is already ready to be used in an InnoDB cluster.

//@ BUG#29765093: clean-up. {VER(>=8.0.11)}
||

//@ BUG#30339460: Use configureInstance to create the Admin user.
||

//@ BUG#30339460: Use configureInstance with the Admin user (no error).
||
