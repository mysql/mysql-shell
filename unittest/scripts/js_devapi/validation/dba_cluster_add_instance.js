//@ Initialization
||

//@ connect to instance
||

//@ create first cluster
|~WARNING: Option 'memberSslMode' is deprecated|

//@ ipWhitelist deprecation error {VER(>=8.0.22)}
||Cluster.addInstance: Cannot use the ipWhitelist and ipAllowlist options simultaneously. The ipWhitelist option is deprecated, please use the ipAllowlist option instead. (ArgumentError)

//@ Success adding instance
|WARNING: Option 'memberSslMode' is deprecated for this operation and it will be removed in a future release. This option is not needed because the SSL mode is automatically obtained from the cluster. Please do not use it here.|

//@<OUT> Check that recovery account looks OK and that it was stored in the metadata and that it was configured
user	host
mysql_innodb_cluster_111	%
mysql_innodb_cluster_222	%
2
instance_name	attributes
<<<hostname>>>:<<<__mysql_sandbox_port1>>>	{"recoveryAccountHost": "%", "recoveryAccountUser": "mysql_innodb_cluster_111"}
<<<hostname>>>:<<<__mysql_sandbox_port2>>>	{"joinTime": [[*]], "recoveryAccountHost": "%", "recoveryAccountUser": "mysql_innodb_cluster_222"}
2
user_name
mysql_innodb_cluster_111
1

//@<OUT> Check auto_increment values for single-primary

//@ Get the cluster back
||

//@ Restore the quorum
||

//@ Success adding instance to the single cluster
||

//@ Remove the instance from the cluster
||

//@ create second cluster
||

//@ Failure adding instance from multi cluster into single
||The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' is already part of another InnoDB cluster

//@ Failure adding instance from an unmanaged replication group into single
||The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' is already part of another Replication Group

//@ Failure adding instance already in the InnoDB cluster
||The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is already part of this InnoDB cluster

//@ Finalization
||
