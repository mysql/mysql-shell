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
Some configuration options need to be fixed:
+-----------+---------------+----------------+-----------------------------------------------+
| Variable  | Current Value | Required Value | Note                                          |
+-----------+---------------+----------------+-----------------------------------------------+
| server_id | 0             | <unique ID>    | Update the config file and restart the server |
+-----------+---------------+----------------+-----------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server: an option file is required.
Please use the dba.configureInstance() command to repair these issues.

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

//@<OUT> configureLocalInstance server_id updated but needs restart. {VER(>=8.0.11)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Some configuration options need to be fixed:
+-----------+---------------+----------------+-----------------------------------------------+
| Variable  | Current Value | Required Value | Note                                          |
+-----------+---------------+----------------+-----------------------------------------------+
| server_id | 0             | <unique ID>    | Update the config file and restart the server |
+-----------+---------------+----------------+-----------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
Configuring instance...
The instance 'localhost:<<<__mysql_sandbox_port1>>>' was configured for InnoDB cluster usage.
MySQL server needs to be restarted for configuration changes to take effect.

//@<OUT> configureLocalInstance server_id updated but needs restart. {VER(<8.0.11)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Some configuration options need to be fixed:
+-----------+---------------+----------------+-----------------------------------------------+
| Variable  | Current Value | Required Value | Note                                          |
+-----------+---------------+----------------+-----------------------------------------------+
| server_id | 0             | <unique ID>    | Update the config file and restart the server |
+-----------+---------------+----------------+-----------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server: an option file is required.
Configuring instance...
The instance 'localhost:<<<__mysql_sandbox_port1>>>' was configured for InnoDB cluster usage.
MySQL server needs to be restarted for configuration changes to take effect.

//@<OUT> configureLocalInstance still indicate that a restart is needed. {VER(<8.0.11)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Some configuration options need to be fixed:
+-----------+---------------+----------------+--------------------------------------------------+
| Variable  | Current Value | Required Value | Note                                             |
+-----------+---------------+----------------+--------------------------------------------------+
| server_id | 0             | <unique ID>    | Update read-only variable and restart the server |
+-----------+---------------+----------------+--------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
The instance 'localhost:<<<__mysql_sandbox_port1>>>' is already ready for InnoDB cluster usage.
MySQL server needs to be restarted for configuration changes to take effect.

//@<OUT> configureLocalInstance still indicate that a restart is needed. {VER(>=8.0.11)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Some configuration options need to be fixed:
+-----------+---------------+----------------+--------------------------------------------------+
| Variable  | Current Value | Required Value | Note                                             |
+-----------+---------------+----------------+--------------------------------------------------+
| server_id | 0             | <unique ID>    | Update read-only variable and restart the server |
+-----------+---------------+----------------+--------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
Configuring instance...
The instance 'localhost:<<<__mysql_sandbox_port1>>>' was configured for InnoDB cluster usage.
MySQL server needs to be restarted for configuration changes to take effect.

//@ Restart sandbox 1.
||

//@<OUT> configureLocalInstance no issues after restart for sandobox 1.
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

The instance 'localhost:<<<__mysql_sandbox_port1>>>' is valid for InnoDB cluster usage.

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

//@<OUT> configureLocalInstance no server_id in my.cnf (needs restart). {VER(>=8.0.3)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Some configuration options need to be fixed:
+-----------+---------------+----------------+-----------------------------------------------+
| Variable  | Current Value | Required Value | Note                                          |
+-----------+---------------+----------------+-----------------------------------------------+
| server_id | 1             | <unique ID>    | Update the config file and restart the server |
+-----------+---------------+----------------+-----------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
Configuring instance...
The instance 'localhost:<<<__mysql_sandbox_port2>>>' was configured for InnoDB cluster usage.
MySQL server needs to be restarted for configuration changes to take effect.

//@<OUT> configureLocalInstance no server_id in my.cnf (still needs restart). {VER(>=8.0.3)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Some configuration options need to be fixed:
+-----------+---------------+----------------+--------------------------------------------------+
| Variable  | Current Value | Required Value | Note                                             |
+-----------+---------------+----------------+--------------------------------------------------+
| server_id | 1             | <unique ID>    | Update read-only variable and restart the server |
+-----------+---------------+----------------+--------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
Configuring instance...
The instance 'localhost:<<<__mysql_sandbox_port2>>>' was configured for InnoDB cluster usage.
MySQL server needs to be restarted for configuration changes to take effect.

//@ Restart sandbox 2. {VER(>=8.0.3)}
||

//@<OUT> configureLocalInstance no issues after restart for sandbox 2. {VER(>=8.0.3)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

The instance 'localhost:<<<__mysql_sandbox_port2>>>' is valid for InnoDB cluster usage.

//@ Clean-up deployed instances.
||