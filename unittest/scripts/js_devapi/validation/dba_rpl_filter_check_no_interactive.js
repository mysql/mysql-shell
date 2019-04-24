//@ Initialization
||

//@## Dba: create_cluster fails with binlog-do-db
||instance has binlog filters configured, but they are not supported in InnoDB clusters.

//@# Dba: create_cluster fails with binlog-ignore-db
||instance has binlog filters configured, but they are not supported in InnoDB clusters.

//@# Dba: create_cluster fails with global repl filter {VER(>=8.0.11)}
||instance has global replication filters configured, but they are not supported in InnoDB clusters.
||instance has global replication filters configured, but they are not supported in InnoDB clusters.

//@# Dba: create_cluster fails with repl filter {VER(>=8.0.11)}
||instance has replication filters configured, but they are not supported in InnoDB clusters.
||instance has replication filters configured, but they are not supported in InnoDB clusters.

//@# Dba: create_cluster succeed without binlog-do-db nor repl filter
|<Cluster:testCluster>|

//@# Dba: add_instance fails with binlog-do-db
||instance has binlog filters configured, but they are not supported in InnoDB clusters.

//@# Dba: add_instance fails with binlog-ignore-db
||instance has binlog filters configured, but they are not supported in InnoDB clusters.

//@# Dba: add_instance fails with global repl filter {VER(>=8.0.11)}
||instance has global replication filters configured, but they are not supported in InnoDB clusters.
||instance has global replication filters configured, but they are not supported in InnoDB clusters.

//@# Dba: add_instance fails with repl filter {VER(>=8.0.11)}
||instance has replication filters configured, but they are not supported in InnoDB clusters.
||instance has replication filters configured, but they are not supported in InnoDB clusters.

//@# Dba: add_instance succeeds without repl filter
||

//@ Finalization
||
