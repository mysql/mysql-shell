//@ BUG#27084767: Initialize new instances
||

//@ BUG#27084767: Create a cluster in single-primary mode
||

//@ BUG#27084767: Add instance to cluster in single-primary mode
||

//@ BUG#27084767: Dissolve the cluster
||

//@ BUG#27084767: Create a cluster in multi-primary mode
||

//@ BUG#27084767: Add instance to cluster in multi-primary mode
||

//@ BUG#27084767: Finalization
||

//@ BUG#27084767: Initialize new non-sandbox instance
||

//@ BUG#27084767: Create a cluster in single-primary mode non-sandbox
||

//@ BUG#27084767: Add instance to cluster in single-primary mode non-sandbox
||

//@ BUG#27084767: Dissolve the cluster non-sandbox
||

//@ BUG#27084767: Create a cluster in multi-primary mode non-sandbox
||

//@ BUG#27084767: Add instance to cluster in multi-primary mode non-sandbox
||

//@ BUG#27084767: Finalization non-sandbox
||

//@ BUG#27677227 cluster with x protocol disabled setup
||

//@<OUT> BUG#27677227 cluster with x protocol disabled, mysqlx should be NULL
<<<hostname>>>:<<<__mysql_sandbox_port1>>> = {"grLocal": "<<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>", "mysqlClassic": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"}
<<<hostname>>>:<<<__mysql_sandbox_port2>>> = {"grLocal": "<<<hostname>>>:<<<__mysql_sandbox_gr_port2>>>", "mysqlClassic": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>"}
<<<hostname>>>:<<<__mysql_sandbox_port3>>> = {"mysqlX": "<<<hostname>>>:<<<__mysql_sandbox_x_port3>>>", "grLocal": "<<<hostname>>>:<<<__mysql_sandbox_gr_port3>>>", "mysqlClassic": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>"}

//@ BUG#27677227 cluster with x protocol disabled cleanup
||

//@ BUG28056944 deploy sandboxes.
||

//@ BUG28056944 create cluster.
||

//@<OUT> BUG28056944 remove instance with wrong password and force = true.
NOTE: MySQL Error 1045 (28000): Access denied for user 'root'@'[[*]]' (using password: YES)
NOTE: The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is not reachable and it will only be removed from the metadata. Please take any necessary actions to ensure the instance will not rejoin the cluster if brought back online.

The instance will be removed from the InnoDB cluster. Depending on the instance
being the Seed or not, the Metadata session might become invalid. If so, please
start a new session to the Metadata Storage R/W instance.

<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port1+"' cannot persist configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.\n":""\>>>

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
ERROR: Cannot join instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' to the cluster because it has the same server ID of a member of the cluster. Please change the server ID of the instance to add: all members must have a unique server ID.

//@<ERR> BUG#29809560: add instance fails because server_id is not unique.
Cluster.addInstance: The server_id '666' is already used by instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'. (RuntimeError)

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

//@ WL#12773: FR4 - The ipWhitelist shall not change the behavior defined by FR1
|mysql_innodb_cluster_11111, %|
|mysql_innodb_cluster_22222, %|
|mysql_innodb_cluster_33333, %|

//@<OUT> BUG#25503159: number of recovery users before executing addInstance().
Number of recovery user before addInstance(): 1

//@<ERR> BUG#25503159: add instance fail (using an invalid localaddress).
Cluster.addInstance: Group Replication failed to start: [[*]]

//@ BUG#30281908: add instance using clone and simulating a restart timeout {VER(>= 8.0.17)}
|WARNING: Clone process appears to have finished and tried to restart the MySQL server, but it has not yet started back up.|
|Please make sure the MySQL server at '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is restarted and call <Cluster>.rescan() to complete the process. To increase the timeout, change shell.options["dba.restartWaitTimeout"].|
||Cluster.addInstance: Timeout waiting for server to restart (MYSQLSH 51156)

//@ BUG#30281908: complete the process with .rescan() {VER(>= 8.0.17)}
|Adding instance to the cluster metadata...|
|The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was successfully added to the cluster metadata.|

//@ canonical IPv6 addresses are supported on addInstance WL#12758 {VER(>= 8.0.14)}
|[::1]:<<<__mysql_sandbox_port1>>> = {"mysqlX": "[::1]:<<<__mysql_sandbox_x_port1>>>", "grLocal": "[::1]:<<<__mysql_sandbox_gr_port1>>>", "mysqlClassic": "[::1]:<<<__mysql_sandbox_port1>>>"}|
|[::1]:<<<__mysql_sandbox_port2>>> = {"mysqlX": "[::1]:<<<__mysql_sandbox_x_port2>>>", "grLocal": "[::1]:<<<__mysql_sandbox_gr_port2>>>", "mysqlClassic": "[::1]:<<<__mysql_sandbox_port2>>>"}|

// If the target instance is >= 8.0.22, when ipWhitelist is used a deprecation warning must be printed
//@ IPv6 addresses are supported on localAddress, groupSeeds and ipWhitelist WL#12758 {VER(>=8.0.22)}
|WARNING: The ipWhitelist option is deprecated in favor of ipAllowlist. ipAllowlist will be set instead.|

//@ canonical IPv6 addresses are not supported below 8.0.14 WL#12758 {VER(< 8.0.14)}
|ERROR: Cannot use host '::1' for instance '[::1]:<<<__mysql_sandbox_port2>>>' because it is an IPv6 address which is only supported by Group Replication from MySQL version >= 8.0.14. Set the MySQL server 'report_host' variable to an IPv4 address or hostname that resolves an IPv4 address.|
||Cluster.addInstance: Unsupported IP address '::1'. IPv6 is only supported by Group Replication on MySQL version >= 8.0.14. (RuntimeError)

//@ IPv6 local_address is not supported below 8.0.14 WL#12758 {VER(< 8.0.14)}
|ERROR: Cannot join instance '127.0.0.1:<<<__mysql_sandbox_port2>>>' to cluster: unsupported localAddress value.|
||Cluster.addInstance: Cannot use value '[::1]:<<<__mysql_sandbox_gr_port1>>>' for option localAddress because it has an IPv6 address which is only supported by Group Replication from MySQL version >= 8.0.14 and the target instance version is <<<__version>>>. (ArgumentError)

//@ IPv6 on ipWhitelist is not supported below 8.0.14 WL#12758 {VER(< 8.0.14)}
||Cluster.addInstance: Invalid value for ipWhitelist '::1': IPv6 not supported (version >= 8.0.14 required for IPv6 support). (ArgumentError)

//@ IPv6 on groupSeeds is not supported below 8.0.14 - 1 WL#12758 {VER(< 8.0.14)}
||Cluster.addInstance: Instance does not support the following groupSeed value :'[::1]:<<<__mysql_sandbox_gr_port2>>>'. IPv6 addresses/hostnames are only supported by Group Replication from MySQL version >= 8.0.14 and the target instance version is <<<__version>>>. (ArgumentError)

//@ IPv6 on groupSeeds is not supported below 8.0.14 - 2 WL#12758 {VER(< 8.0.14)}
||Cluster.addInstance: Instance does not support the following groupSeed values :'[::1]:<<<__mysql_sandbox_gr_port2>>>, [fe80::7e36:f49a:63c8:8ad6]:<<<__mysql_sandbox_gr_port1>>>'. IPv6 addresses/hostnames are only supported by Group Replication from MySQL version >= 8.0.14 and the target instance version is <<<__version>>>. (ArgumentError)

