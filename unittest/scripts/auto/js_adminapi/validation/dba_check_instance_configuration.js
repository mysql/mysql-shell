//@<OUT> Check instance configuration async replication warning
Validating local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected
WARNING: The instance 'localhost:<<<__mysql_sandbox_port2>>>' cannot be added to an InnoDB cluster because it has the asynchronous (master-slave) replication configured and running. To add to it a cluster please stop the slave threads by executing the query: 'STOP SLAVE;'

Checking instance configuration...
Instance configuration is compatible with InnoDB cluster

The instance 'localhost:<<<__mysql_sandbox_port2>>>' is valid for InnoDB cluster usage.

{
    "status": "ok"
}
