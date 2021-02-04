//@ Initialization
||

//@ it's not possible to adopt from GR without existing group replication
||The adoptFromGR option is set to true, but there is no replication group to adopt (ArgumentError)

//@ Create group by hand
||

//@ Create cluster adopting from GR
||

//@<OUT> Confirm new replication users were created and replaced existing ones, but didn't drop the old ones that don't belong to shell (WL#12773 FR1.5 and FR3)
user	host
mysql_innodb_cluster_1111	%
mysql_innodb_cluster_2222	%
some_user	%
3
instance_name	attributes
<<<hostname>>>:<<<__mysql_sandbox_port1>>>	{"server_id": 1111, "recoveryAccountHost": "%", "recoveryAccountUser": "mysql_innodb_cluster_1111"}
<<<hostname>>>:<<<__mysql_sandbox_port2>>>	{"server_id": 2222, "recoveryAccountHost": "%", "recoveryAccountUser": "mysql_innodb_cluster_2222"}
2
recovery_user_name
mysql_innodb_cluster_1111
1
user	host
mysql_innodb_cluster_1111	%
mysql_innodb_cluster_2222	%
some_user	%
3
instance_name	attributes
<<<hostname>>>:<<<__mysql_sandbox_port1>>>	{"server_id": 1111, "recoveryAccountHost": "%", "recoveryAccountUser": "mysql_innodb_cluster_1111"}
<<<hostname>>>:<<<__mysql_sandbox_port2>>>	{"server_id": 2222, "recoveryAccountHost": "%", "recoveryAccountUser": "mysql_innodb_cluster_2222"}
2
recovery_user_name
mysql_innodb_cluster_2222
1

//@ dissolve the cluster
||

//@ it's not possible to adopt from GR when cluster was dissolved
||The adoptFromGR option is set to true, but there is no replication group to adopt (ArgumentError)

//@ Finalization
||
