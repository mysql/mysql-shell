//@<OUT> rejoinInstance async replication error
ERROR: Cannot rejoin instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' to the cluster because it has asynchronous (source-replica) replication channel(s) configured. MySQL InnoDB Cluster does not support manually configured channels as they are not managed using the AdminAPI (e.g. when PRIMARY moves to another member) which may cause cause replication to break or even create split-brain scenarios (data loss).

//@<ERR> rejoinInstance async replication error
Cluster.rejoinInstance: The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' has asynchronous replication configured. (RuntimeError)

//@<OUT> rejoinInstance async replication error with channels stopped
ERROR: Cannot rejoin instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' to the cluster because it has asynchronous (source-replica) replication channel(s) configured. MySQL InnoDB Cluster does not support manually configured channels as they are not managed using the AdminAPI (e.g. when PRIMARY moves to another member) which may cause cause replication to break or even create split-brain scenarios (data loss).

//@<ERR> rejoinInstance async replication error with channels stopped
Cluster.rejoinInstance: The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' has asynchronous replication configured. (RuntimeError)

//@<OUT> BUG#29754915: rejoin instance 3 successfully.
<<<(__version_num==80016)?"NOTE: Unable to determine the Group Replication protocol version, while verifying if a protocol upgrade would be possible: Can't initialize function 'group_replication_get_communication_protocol'; A member is joining the group, wait for it to be ONLINE.":"">>>
<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port3+"' cannot persist Group Replication configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureInstance() command locally to persist the changes.":"">>>

//@ canonical IPv6 addresses are not supported below 8.0.14 WL#12758 {VER(< 8.0.14)}
|ERROR: Cannot use host '::1' for instance '[::1]:<<<__mysql_sandbox_port2>>>' because it is an IPv6 address which is only supported by Group Replication from MySQL version >= 8.0.14. Set the MySQL server 'report_host' variable to an IPv4 address or hostname that resolves an IPv4 address.|
||Unsupported IP address '::1'. IPv6 is only supported by Group Replication on MySQL version >= 8.0.14. (RuntimeError)

//@ IPv6 on ipAllowlist is not supported below 8.0.14 WL#12758 {VER(< 8.0.14)}
||Invalid value for ipAllowlist '::1': IPv6 not supported (version >= 8.0.14 required for IPv6 support). (ArgumentError)

//@ Rejoin instance fails if the target instance contains errant transactions (BUG#29953812) {VER(>=8.0.17)}
|WARNING: A GTID set check of the MySQL instance at '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' determined that it contains transactions that do not originate from the cluster, which must be discarded before it can join the cluster.|
||
|Instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' has the following errant GTIDs that do not exist in the cluster:|
|00025721-1111-1111-1111-111111111111:1|
||
|WARNING: Discarding these extra GTID events can either be done manually or by completely overwriting the state of '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' with a physical snapshot from an existing cluster member. To use this method by default, set the 'recoveryMethod' option to 'clone'.|
||
|Having extra GTID events is not expected, and it is recommended to investigate this further and ensure that the data can be removed prior to choosing the clone recovery method.|
||
|ERROR: The target instance must be either cloned or fully re-provisioned before it can be re-added to the target cluster.|
||Instance provisioning required (MYSQLSH 51153)

//@ Rejoin instance fails if the target instance contains errant transactions 5.7 (BUG#29953812) {VER(<8.0.17)}
|WARNING: A GTID set check of the MySQL instance at '<<<hostname>>>:<<<__mysql_sandbox_port2>>>'|
||
|Instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' has the following errant GTIDs that do not exist in the cluster:|
|00025721-1111-1111-1111-111111111111:1|
||
|WARNING: Discarding these extra GTID events can either be done manually or by completely overwriting the state of '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' with a physical snapshot from an existing cluster member. To use this method by default, set the 'recoveryMethod' option to 'clone'.|
||
|Having extra GTID events is not expected, and it is recommended to investigate this further and ensure that the data can be removed prior to choosing the clone recovery method.|
|ERROR: The target instance must be either cloned or fully re-provisioned before it can be re-added to the target cluster.|
|Built-in clone support is available starting with MySQL 8.0.17 and is the recommended method for provisioning instances. Instance is running MySQL <<<__version>>>.|
||Instance provisioning required (MYSQLSH 51153)

//@ Rejoin instance fails if the transactions were purged from the cluster (BUG#29953812) {VER(>=8.0.17)}
|NOTE: A GTID set check of the MySQL instance at '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' determined that it is missing transactions that were purged from all cluster members.|
|NOTE: The target instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' has not been pre-provisioned (GTID set is empty). The Shell is unable to determine whether the instance has pre-existing data that would be overwritten with clone based recovery.|
|The safest and most convenient way to provision a new instance is through automatic clone provisioning, which will completely overwrite the state of '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' with a physical snapshot from an existing cluster member. To use this method by default, set the 'recoveryMethod' option to 'clone'.|
||
||'recoveryMethod' option must be set to 'clone' or 'incremental' (MYSQLSH 51167)

//@ Rejoin instance fails if the transactions were purged from the cluster (BUG#29953812) {VER(<8.0.17)}
|NOTE: A GTID set check of the MySQL instance at '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' determined that it is missing transactions that were purged from all cluster members.|
|NOTE: The target instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' has not been pre-provisioned (GTID set is empty). The Shell is unable to determine whether the instance has pre-existing data that would be overwritten with clone based recovery.|
|The safest and most convenient way to provision a new instance is through automatic clone provisioning, which will completely overwrite the state of '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' with a physical snapshot from an existing cluster member. To use this method by default, set the 'recoveryMethod' option to 'clone'.|
||
|ERROR: The target instance must be either cloned or fully re-provisioned before it can be re-added to the target cluster.|
|Built-in clone support is available starting with MySQL 8.0.17 and is the recommended method for provisioning instances. Instance is running MySQL <<<__version>>>.|
||Cluster.rejoinInstance: Instance provisioning required (MYSQLSH 51153)
