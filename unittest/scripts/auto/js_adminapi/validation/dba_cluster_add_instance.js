//@<OUT> Check that recovery account looks OK and that it was stored in the metadata and that it was configured
user	host
mysql_innodb_cluster_111	%
mysql_innodb_cluster_222	%
2
instance_name	attributes
<<<hostname>>>:<<<__mysql_sandbox_port1>>>	{"server_id": 111, "opt_certSubject": "", "recoveryAccountHost": "%", "recoveryAccountUser": "mysql_innodb_cluster_111"}
<<<hostname>>>:<<<__mysql_sandbox_port2>>>	{"joinTime": [[*]], "server_id": 222, "opt_certSubject": "", "recoveryAccountHost": "%", "recoveryAccountUser": "mysql_innodb_cluster_222"}
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
||The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' is already part of another InnoDB Cluster

//@ Failure adding instance from an unmanaged replication group into single
||The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' is already part of another Replication Group

//@ Failure adding instance already in the InnoDB cluster
||The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is already part of this InnoDB Cluster
