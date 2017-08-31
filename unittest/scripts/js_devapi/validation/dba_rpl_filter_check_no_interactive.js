//@ Initialization
||

//@## Dba: create_cluster fails with binlog-do-db
||Invalid 'binlog-do-db' settings, metadata cannot be excluded. Remove binlog filters or include the 'mysql_innodb_cluster_metadata' database in the 'binlog-do-db' option.

//@# Dba: create_cluster fails with binlog-ignore-db
||Invalid 'binlog-ignore-db' settings, metadata cannot be excluded. Remove binlog filters or the 'mysql_innodb_cluster_metadata' database from the 'binlog-ignore-db' option.

//@# Dba: create_cluster succeed with binlog-do-db
|<Cluster:testCluster>|

//@# Dba: add_instance fails with binlog-do-db
||Invalid 'binlog-do-db' settings, metadata cannot be excluded. Remove binlog filters or include the 'mysql_innodb_cluster_metadata' database in the 'binlog-do-db' option.

//@# Dba: add_instance fails with binlog-ignore-db
||Invalid 'binlog-ignore-db' settings, metadata cannot be excluded. Remove binlog filters or the 'mysql_innodb_cluster_metadata' database from the 'binlog-ignore-db' option.

//@ Finalization
||
