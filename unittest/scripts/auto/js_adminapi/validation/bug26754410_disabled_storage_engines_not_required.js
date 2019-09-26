//@<OUT> checkInstanceConfiguration with disabled_storage_engines no error.
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is valid to be used in an InnoDB cluster.

{
    "status": "ok"
}

//@<OUT> configureInstance disabled_storage_engines no error.
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is valid to be used in an InnoDB cluster.

//@ Remove disabled_storage_engines option from configuration and restart.
||

//@ Restart sandbox.
||

//@<OUT> configureInstance still no issues after restart.
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is valid to be used in an InnoDB cluster.
