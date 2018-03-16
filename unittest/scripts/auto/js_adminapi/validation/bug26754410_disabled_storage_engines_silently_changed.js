//@<OUT> checkInstanceConfiguration with disabled_storage_engines error.
{
    "config_errors": [
        {
            "action": "config_update+restart",
            "current": "MyISAM,BLACKHOLE,ARCHIVE",
            "option": "disabled_storage_engines",
            "required": "MyISAM,BLACKHOLE,FEDERATED,CSV,ARCHIVE"
        }
    ],
    "errors": [],
    "status": "error"
}

//@<OUT> configureLocalInstance disabled_storage_engines updated but needs restart.
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

WARNING: User 'root' can only connect from localhost.
If you need to manage this instance while connected from other hosts, new account(s) with the proper source address specification must be created.

Some configuration options need to be fixed:
+--------------------------+--------------------------+----------------------------------------+-----------------------------------------------+
| Variable                 | Current Value            | Required Value                         | Note                                          |
+--------------------------+--------------------------+----------------------------------------+-----------------------------------------------+
| disabled_storage_engines | MyISAM,BLACKHOLE,ARCHIVE | MyISAM,BLACKHOLE,FEDERATED,CSV,ARCHIVE | Update the config file and restart the server |
+--------------------------+--------------------------+----------------------------------------+-----------------------------------------------+

Configuring instance...
The instance 'localhost:<<<__mysql_sandbox_port1>>>' was configured for cluster usage.

//@<OUT> configureLocalInstance still indicate that a restart is needed.
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

WARNING: User 'root' can only connect from localhost.
If you need to manage this instance while connected from other hosts, new account(s) with the proper source address specification must be created.

Some configuration options need to be fixed:
+--------------------------+--------------------------+----------------------------------------+--------------------------------------------------+
| Variable                 | Current Value            | Required Value                         | Note                                             |
+--------------------------+--------------------------+----------------------------------------+--------------------------------------------------+
| disabled_storage_engines | MyISAM,BLACKHOLE,ARCHIVE | MyISAM,BLACKHOLE,FEDERATED,CSV,ARCHIVE | Update read-only variable and restart the server |
+--------------------------+--------------------------+----------------------------------------+--------------------------------------------------+

Configuring instance...
The instance 'localhost:<<<__mysql_sandbox_port1>>>' was configured for cluster usage.
MySQL server needs to be restarted for configuration changes to take effect.

//@ Restart sandbox.
||

//@<OUT> configureLocalInstance no issues after restart.
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

WARNING: User 'root' can only connect from localhost.
If you need to manage this instance while connected from other hosts, new account(s) with the proper source address specification must be created.

The instance 'localhost:<<<__mysql_sandbox_port1>>>' is valid for InnoDB cluster usage.

//@ Remove disabled_storage_engines option from configuration and restart.
||

//@<OUT> checkInstanceConfiguration no disabled_storage_engines in my.cnf (error).
{
    "config_errors": [
        {
            "action": "<<<if(testutil.versionCheck(__version, '>=', '8.0.5')){'config_update'}else{'config_update+restart'}>>>",
            "current": "<<<if(testutil.versionCheck(__version, '>=', '8.0.5')){'<not set>'}else{'<no value>'}>>>",
            "option": "disabled_storage_engines",
            "required": "MyISAM,BLACKHOLE,FEDERATED,CSV,ARCHIVE"
        }
    ],
    "errors": [],
    "status": "error"
}

//@<OUT> configureLocalInstance no disabled_storage_engines in my.cnf (needs restart). {VER(>=8.0.5)}
+--------------------------+---------------+----------------------------------------+------------------------+
| Variable                 | Current Value | Required Value                         | Note                   |
+--------------------------+---------------+----------------------------------------+------------------------+
| disabled_storage_engines | <not set>     | MyISAM,BLACKHOLE,FEDERATED,CSV,ARCHIVE | Update the config file |
+--------------------------+---------------+----------------------------------------+------------------------+

Configuring instance...
The instance 'localhost:<<<__mysql_sandbox_port1>>>' was configured for use in an InnoDB cluster.

//@<OUT> configureLocalInstance no disabled_storage_engines in my.cnf (needs restart). {VER(<8.0.5)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

WARNING: User 'root' can only connect from localhost.
If you need to manage this instance while connected from other hosts, new account(s) with the proper source address specification must be created.

Some configuration options need to be fixed:
+--------------------------+---------------+----------------------------------------+-----------------------------------------------+
| Variable                 | Current Value | Required Value                         | Note                                          |
+--------------------------+---------------+----------------------------------------+-----------------------------------------------+
| disabled_storage_engines | <no value>    | MyISAM,BLACKHOLE,FEDERATED,CSV,ARCHIVE | Update the config file and restart the server |
+--------------------------+---------------+----------------------------------------+-----------------------------------------------+

Configuring instance...
The instance 'localhost:<<<__mysql_sandbox_port1>>>' was configured for cluster usage.
MySQL server needs to be restarted for configuration changes to take effect.


//@<OUT> configureLocalInstance no disabled_storage_engines in my.cnf (still needs restart).
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

WARNING: User 'root' can only connect from localhost.
If you need to manage this instance while connected from other hosts, new account(s) with the proper source address specification must be created.

//@<OUT> configureLocalInstance no disabled_storage_engines in my.cnf (still needs restart). {VER(>=8.0.5)}
The instance 'localhost:<<<__mysql_sandbox_port1>>>' is valid for InnoDB cluster usage.

//@<OUT> configureLocalInstance no disabled_storage_engines in my.cnf (still needs restart). {VER(<8.0.5)}
Some configuration options need to be fixed:
+--------------------------+---------------+----------------------------------------+--------------------------------------------------+
| Variable                 | Current Value | Required Value                         | Note                                             |
+--------------------------+---------------+----------------------------------------+--------------------------------------------------+
| disabled_storage_engines | <no value>    | MyISAM,BLACKHOLE,FEDERATED,CSV,ARCHIVE | Update read-only variable and restart the server |
+--------------------------+---------------+----------------------------------------+--------------------------------------------------+

Configuring instance...
The instance 'localhost:<<<__mysql_sandbox_port1>>>' was configured for cluster usage.
MySQL server needs to be restarted for configuration changes to take effect.

//@ Restart sandbox again.
||

//@<OUT> configureLocalInstance no issues again after restart.
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

WARNING: User 'root' can only connect from localhost.
If you need to manage this instance while connected from other hosts, new account(s) with the proper source address specification must be created.

The instance 'localhost:<<<__mysql_sandbox_port1>>>' is valid for InnoDB cluster usage.
