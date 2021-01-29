//@# getReplicaSet() in a standalone instance (should fail) {VER(>8.0.4)}
||This function is not available through a session to a standalone instance (MYSQLSH 51300)

//@# getReplicaSet() in a standalone instance (should fail) {VER(<8.0.0)}
||Unsupported server version: This AdminAPI operation requires MySQL version 8.0 or newer, but target is 5.7.[[*]] (RuntimeError)

//@# getReplicaSet() in a standalone instance with GR MD 1.0.1 (should fail)
|WARNING: No replicaset change operations can be executed because the installed metadata version 1.0.1 is lower than the version required by Shell which is version 2.1.0. Upgrade the metadata to remove this restriction. See \? dba.upgradeMetadata for additional details.|
||This function is not available through a session to a standalone instance (metadata exists, instance belongs to that metadata) (MYSQLSH 51314)

//@# getReplicaSet() in a standalone instance with GR MD 2.0.0 (should fail)
||This function is not available through a session to a standalone instance (metadata exists, instance belongs to that metadata) (MYSQLSH 51314)

//@# getReplicaSet() in a standalone instance with AR MD 2.0.0 in 5.7 (should fail) {VER(<8.0.4)}
||Unsupported server version: This AdminAPI operation requires MySQL version 8.0 or newer, but target is 5.7.[[*]] (RuntimeError)

//@ getReplicaSet() in a GR instance (should fail) {VER(<8.0.4)}
||Unsupported server version: This AdminAPI operation requires MySQL version 8.0 or newer, but target is 5.7.[[*]] (RuntimeError)

//@ getReplicaSet() in a GR instance (should fail) {VER(>8.0.4)}
||This function is not available through a session to an instance already in an InnoDB Cluster (MYSQLSH 51305)

//@# Positive case {VER(>=8.0.11)}
||

//@# From a secondary {VER(>=8.0.11)}
||

//@# Delete metadata for the instance (should fail) {VER(>=8.0.11)}
||This function is not available through a session to a standalone instance (metadata exists, instance does not belong to that metadata) (RuntimeError)

