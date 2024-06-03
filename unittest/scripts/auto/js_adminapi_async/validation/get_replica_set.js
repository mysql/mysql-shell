//@# getReplicaSet() in a standalone instance with GR MD 2.0.0 (should fail)
||This function is not available through a session to a standalone instance (metadata exists, instance belongs to that metadata) (MYSQLSH 51314)

//@# getReplicaSet() in a standalone instance with AR MD 2.0.0 in 5.7 (should fail) {VER(<8.0.4)}
||Unsupported server version: This AdminAPI operation requires MySQL version 8.0 or newer, but target is 5.7.[[*]] (RuntimeError)

//@ getReplicaSet() in a GR instance (should fail) {VER(<8.0.4)}
||Unsupported server version: This AdminAPI operation requires MySQL version 8.0 or newer, but target is 5.7.[[*]] (RuntimeError)

//@ getReplicaSet() in a GR instance (should fail) {VER(>8.0.4)}
||This function is not available through a session to an instance already in an InnoDB Cluster (MYSQLSH 51305)
