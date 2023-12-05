//@<OUT> BUG#27677227 cluster with x protocol disabled, mysqlx should be NULL
<<<hostname>>>:<<<__mysql_sandbox_port1>>> = {"grLocal": "<<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>", "mysqlClassic": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"}
<<<hostname>>>:<<<__mysql_sandbox_port2>>> = {"grLocal": "<<<hostname>>>:<<<__mysql_sandbox_gr_port2>>>", "mysqlClassic": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>"}

//@ BUG#27677227 cluster with x protocol disabled cleanup
||

//@ BUG28056944 deploy sandboxes.
||

//@ BUG28056944 create cluster.
||

//@<OUT> BUG28056944 remove instance with wrong password and force = true.
NOTE: MySQL Error 1045 (28000): Access denied for user 'root'@'[[*]]' (using password: YES)
NOTE: The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is not reachable and it will only be removed from the metadata. Please take any necessary actions to ensure the instance will not rejoin the cluster if brought back online.

The instance will be removed from the InnoDB Cluster.

<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port1+"' cannot persist configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureInstance() command locally, using the 'mycnfPath' option, to persist the changes.\n":""\>>>

The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was successfully removed from the cluster.

//@<OUT> BUG28056944 Error adding instance already in group but not in Metadata.
The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was successfully added to the cluster.

//@ BUG28056944 clean-up.
||

//@<OUT> AddInstance async replication error
ERROR: Cannot join instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' to the cluster because it has asynchronous (source-replica) replication channel(s) configured. MySQL InnoDB Cluster does not support manually configured channels as they are not managed using the AdminAPI (e.g. when PRIMARY moves to another member) which may cause cause replication to break or even create split-brain scenarios (data loss).

//@<ERR> AddInstance async replication error
Cluster.addInstance: The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' has asynchronous replication configured. (RuntimeError)

//@<OUT> AddInstance async replication error with channels stopped
ERROR: Cannot join instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' to the cluster because it has asynchronous (source-replica) replication channel(s) configured. MySQL InnoDB Cluster does not support manually configured channels as they are not managed using the AdminAPI (e.g. when PRIMARY moves to another member) which may cause cause replication to break or even create split-brain scenarios (data loss).

//@<ERR> AddInstance async replication error with channels stopped
Cluster.addInstance: The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' has asynchronous replication configured. (RuntimeError)

//@ BUG#29305551: Finalization
||

//@ BUG#29809560: deploy sandboxes.
||

//@ BUG#29809560: set the same server id an all instances.
||

//@ BUG#29809560: create cluster.
||

//@<OUT> BUG#29809560: add instance fails because server_id is not unique.
ERROR: The target instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' has a 'server_id' already being used by instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'.

//@<ERR> BUG#29809560: add instance fails because server_id is not unique.
Cluster.addInstance: Invalid server_id. (MYSQLSH 51606)

//@ BUG#29809560: clean-up.
||

// BUG#28855764: user created by shell expires with default_password_lifetime
//@ BUG#28855764: deploy sandboxes.
||

//@ BUG#28855764: create cluster.
||

//@ BUG#28855764: add instance an instance to the cluster
||

//@ BUG#28855764: get recovery user for instance 2.
||

//@ BUG#28855764: get recovery user for instance 1.
||

//@<OUT> BUG#28855764: Passwords for recovery users never expire (password_lifetime=0).
Number of accounts for '<<<recovery_user_1>>>': 1
Number of accounts for '<<<recovery_user_2>>>': 1

//@ BUG#28855764: clean-up.
||

//@ WL#12773: FR4 - The ipAllowlist shall not change the behavior defined by FR1
|mysql_innodb_cluster_11111, %|
|mysql_innodb_cluster_22222, %|
|mysql_innodb_cluster_33333, %|

//@<OUT> BUG#25503159: number of recovery users before executing addInstance().
Number of recovery user before addInstance(): 1

//@ BUG#30281908: add instance using clone and simulating a restart timeout {VER(>= 8.0.17)}
|WARNING: Clone process appears to have finished and tried to restart the MySQL server, but it has not yet started back up.|
|Please make sure the MySQL server at '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is restarted and call <Cluster>.rescan() to complete the process. To increase the timeout, change shell.options["dba.restartWaitTimeout"].|
||Timeout waiting for server to restart (MYSQLSH 51156)

//@ BUG#30281908: complete the process with .rescan() {VER(>= 8.0.17)}
|Adding instance to the cluster metadata...|
|The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was successfully added to the cluster metadata.|

//@ canonical IPv6 addresses are supported on addInstance WL#12758 {VER(>= 8.0.14)}
|[::1]:<<<__mysql_sandbox_port1>>> = {"mysqlX": "[::1]:<<<__mysql_sandbox_x_port1>>>", "grLocal": "[::1]:<<<__mysql_sandbox_gr_port1>>>", "mysqlClassic": "[::1]:<<<__mysql_sandbox_port1>>>"}|
|[::1]:<<<__mysql_sandbox_port2>>> = {"mysqlX": "[::1]:<<<__mysql_sandbox_x_port2>>>", "grLocal": "[::1]:<<<__mysql_sandbox_gr_port2>>>", "mysqlClassic": "[::1]:<<<__mysql_sandbox_port2>>>"}|

//@ canonical IPv6 addresses are not supported below 8.0.14 WL#12758 {VER(< 8.0.14)}
|ERROR: Cannot use host '::1' for instance '[::1]:<<<__mysql_sandbox_port2>>>' because it is an IPv6 address which is only supported by Group Replication from MySQL version >= 8.0.14. Set the MySQL server 'report_host' variable to an IPv4 address or hostname that resolves an IPv4 address.|
||Unsupported IP address '::1'. IPv6 is only supported by Group Replication on MySQL version >= 8.0.14. (RuntimeError)

//@ IPv6 local_address is not supported below 8.0.14 WL#12758 {VER(< 8.0.14)}
|ERROR: Cannot join instance '127.0.0.1:<<<__mysql_sandbox_port2>>>' to cluster: unsupported localAddress value.|
||Cannot use value '[::1]:<<<__mysql_sandbox_gr_port1>>>' for option localAddress because it has an IPv6 address which is only supported by Group Replication from MySQL version >= 8.0.14 and the target instance version is <<<__version>>>. (ArgumentError)

//@ IPv6 on ipAllowlist is not supported below 8.0.14 WL#12758 {VER(< 8.0.14)}
||Invalid value for ipAllowlist '::1': IPv6 not supported (version >= 8.0.14 required for IPv6 support). (ArgumentError)

