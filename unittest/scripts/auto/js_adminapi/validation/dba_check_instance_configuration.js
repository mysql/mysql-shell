//@<OUT> Check instance configuration async replication warning {VER(<8.0.22)}
Validating local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port2>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected
WARNING: The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' cannot be added to an InnoDB cluster because it has the asynchronous (source-replica) replication configured and running. To add to it a cluster please stop the replica threads by executing the query: 'STOP SLAVE;'.

Checking instance configuration...
Instance configuration is compatible with InnoDB cluster

The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is valid to be used in an InnoDB cluster.

{
    "status": "ok"
}

//@<OUT> Check instance configuration async replication warning {VER(>=8.0.22)}
Validating local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port2>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected
WARNING: The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' cannot be added to an InnoDB cluster because it has the asynchronous (source-replica) replication configured and running. To add to it a cluster please stop the replica threads by executing the query: 'STOP REPLICA;'.

Checking instance configuration...
Instance configuration is compatible with InnoDB cluster

The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is valid to be used in an InnoDB cluster.

{
    "status": "ok"
}

//@<OUT> canonical IPv6 addresses are supported WL#12758 {VER(>= 8.0.14)}
Validating local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as [::1]:<<<__mysql_sandbox_port1>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected

Checking instance configuration...
Instance configuration is compatible with InnoDB cluster

The instance '[::1]:<<<__mysql_sandbox_port1>>>' is valid to be used in an InnoDB cluster.

{
    "status": "ok"
}

//@<OUT> canonical IPv4 addresses are supported WL#12758
Validating local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as 127.0.0.1:<<<__mysql_sandbox_port1>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected

Checking instance configuration...
Instance configuration is compatible with InnoDB cluster

The instance '127.0.0.1:<<<__mysql_sandbox_port1>>>' is valid to be used in an InnoDB cluster.

{
    "status": "ok"
}

//@<OUT> canonical IPv4 addresses are supported (using IPv6 connection data) WL#12758
Validating local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as 127.0.0.1:<<<__mysql_sandbox_port1>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected

Checking instance configuration...
Instance configuration is compatible with InnoDB cluster

The instance '127.0.0.1:<<<__mysql_sandbox_port1>>>' is valid to be used in an InnoDB cluster.

{
    "status": "ok"
}

//@ IPv6 not supported on versions below 8.0.14 WL#12758 {VER(< 8.0.14)}
|ERROR: Cannot use host '::1' for instance '[::1]:<<<__mysql_sandbox_port1>>>' because it is an IPv6 address which is only supported by Group Replication from MySQL version >= 8.0.14. Set the MySQL server 'report_host' variable to an IPv4 address or hostname that resolves an IPv4 address.|
||Dba.checkInstanceConfiguration: Unsupported IP address '::1'. IPv6 is only supported by Group Replication on MySQL version >= 8.0.14. (RuntimeError)
