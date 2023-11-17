//@<OUT> check instance with invalid parallel type. {VER(>=8.3.0)}
Validating local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB Cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected

Checking instance configuration...
Instance configuration is compatible with InnoDB cluster

The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is valid for InnoDB Cluster usage.

{
    "status": "ok"
}

//@<OUT> check instance with invalid parallel type. {VER(>=8.0.11) && VER(<8.3.0)}
Validating local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB Cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected

Checking instance configuration...

NOTE: Some configuration options need to be fixed:
?{VER(<8.0.26)}
+---------------------+---------------+----------------+----------------------------+
| Variable            | Current Value | Required Value | Note                       |
+---------------------+---------------+----------------+----------------------------+
| slave_parallel_type | DATABASE      | LOGICAL_CLOCK  | Update the server variable |
+---------------------+---------------+----------------+----------------------------+
?{}
?{VER(>=8.0.26)}
+-----------------------+---------------+----------------+----------------------------+
| Variable              | Current Value | Required Value | Note                       |
+-----------------------+---------------+----------------+----------------------------+
| replica_parallel_type | DATABASE      | LOGICAL_CLOCK  | Update the server variable |
+-----------------------+---------------+----------------+----------------------------+
?{}

NOTE: Please use the dba.configureInstance() command to repair these issues.

//@<OUT> check instance with invalid parallel type. {VER(<8.0.11)}
Validating local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB Cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected

Checking instance configuration...

NOTE: Some configuration options need to be fixed:
+---------------------+---------------+----------------+------------------------------------------------+
| Variable            | Current Value | Required Value | Note                                           |
+---------------------+---------------+----------------+------------------------------------------------+
| slave_parallel_type | DATABASE      | LOGICAL_CLOCK  | Update the server variable and the config file |
+---------------------+---------------+----------------+------------------------------------------------+

NOTE: Please use the dba.configureInstance() command to repair these issues.

//@<OUT> check instance with invalid commit order. {VER(>=8.0.11)}
Validating local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB Cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port2>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected

Checking instance configuration...

NOTE: Some configuration options need to be fixed:
?{VER(<8.0.26)}
+-----------------------------+---------------+----------------+----------------------------+
| Variable                    | Current Value | Required Value | Note                       |
+-----------------------------+---------------+----------------+----------------------------+
| slave_preserve_commit_order | OFF           | ON             | Update the server variable |
+-----------------------------+---------------+----------------+----------------------------+
?{}
?{VER(>=8.0.26)}
+-------------------------------+---------------+----------------+----------------------------+
| Variable                      | Current Value | Required Value | Note                       |
+-------------------------------+---------------+----------------+----------------------------+
| replica_preserve_commit_order | OFF           | ON             | Update the server variable |
+-------------------------------+---------------+----------------+----------------------------+
?{}

NOTE: Please use the dba.configureInstance() command to repair these issues.

//@<OUT> check instance with invalid commit order. {VER(<8.0.11)}
Validating local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB Cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port2>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected

Checking instance configuration...

NOTE: Some configuration options need to be fixed:
?{VER(<8.0.26)}
+-----------------------------+---------------+----------------+------------------------------------------------+
| Variable                    | Current Value | Required Value | Note                                           |
+-----------------------------+---------------+----------------+------------------------------------------------+
| slave_preserve_commit_order | OFF           | ON             | Update the server variable and the config file |
+-----------------------------+---------------+----------------+------------------------------------------------+
?{}
?{VER(>=8.0.26)}
+-------------------------------+---------------+----------------+------------------------------------------------+
| Variable                      | Current Value | Required Value | Note                                           |
+-------------------------------+---------------+----------------+------------------------------------------------+
| replica_preserve_commit_order | OFF           | ON             | Update the server variable and the config file |
+-------------------------------+---------------+----------------+------------------------------------------------+
?{}

NOTE: Please use the dba.configureInstance() command to repair these issues.

//@ Adding instance to cluster (fail: commit order wrong).
|Please use the dba.configureInstance() command to repair these issues.|
||Instance check failed (RuntimeError)

//@<OUT> check instance with invalid type and commit order. {VER(>=8.3.0)}
Validating local MySQL instance listening at port <<<__mysql_sandbox_port3>>> for use in an InnoDB Cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port3>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected

Checking instance configuration...

NOTE: Some configuration options need to be fixed:
+-------------------------------+---------------+----------------+----------------------------+
| Variable                      | Current Value | Required Value | Note                       |
+-------------------------------+---------------+----------------+----------------------------+
| replica_preserve_commit_order | OFF           | ON             | Update the server variable |
+-------------------------------+---------------+----------------+----------------------------+

NOTE: Please use the dba.configureInstance() command to repair these issues.

//@<OUT> check instance with invalid type and commit order. {VER(>=8.0.11) && VER(<8.3.0)}
Validating local MySQL instance listening at port <<<__mysql_sandbox_port3>>> for use in an InnoDB Cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port3>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected

Checking instance configuration...

NOTE: Some configuration options need to be fixed:
?{VER(<8.0.26)}
+-----------------------------+---------------+----------------+----------------------------+
| Variable                    | Current Value | Required Value | Note                       |
+-----------------------------+---------------+----------------+----------------------------+
| slave_parallel_type         | DATABASE      | LOGICAL_CLOCK  | Update the server variable |
| slave_preserve_commit_order | OFF           | ON             | Update the server variable |
+-----------------------------+---------------+----------------+----------------------------+
?{}
?{VER(>=8.0.26)}
+-------------------------------+---------------+----------------+----------------------------+
| Variable                      | Current Value | Required Value | Note                       |
+-------------------------------+---------------+----------------+----------------------------+
| replica_parallel_type         | DATABASE      | LOGICAL_CLOCK  | Update the server variable |
| replica_preserve_commit_order | OFF           | ON             | Update the server variable |
+-------------------------------+---------------+----------------+----------------------------+
?{}

NOTE: Please use the dba.configureInstance() command to repair these issues.

//@<OUT> check instance with invalid type and commit order. {VER(<8.0.11)}
Validating local MySQL instance listening at port <<<__mysql_sandbox_port3>>> for use in an InnoDB Cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port3>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected

Checking instance configuration...

NOTE: Some configuration options need to be fixed:
+-----------------------------+---------------+----------------+------------------------------------------------+
| Variable                    | Current Value | Required Value | Note                                           |
+-----------------------------+---------------+----------------+------------------------------------------------+
| slave_parallel_type         | DATABASE      | LOGICAL_CLOCK  | Update the server variable and the config file |
| slave_preserve_commit_order | OFF           | ON             | Update the server variable and the config file |
+-----------------------------+---------------+----------------+------------------------------------------------+

NOTE: Please use the dba.configureInstance() command to repair these issues.

//@ configure instance and update type and commit order with valid values.
|The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' was configured to be used in an InnoDB Cluster.|

//@<OUT> check instance, no invalid values after configure.
Validating local MySQL instance listening at port <<<__mysql_sandbox_port3>>> for use in an InnoDB Cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port3>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected

Checking instance configuration...
Instance configuration is compatible with InnoDB cluster

The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' is valid for InnoDB Cluster usage.
