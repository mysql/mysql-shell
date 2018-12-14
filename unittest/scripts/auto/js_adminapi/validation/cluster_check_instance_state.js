//@ Create single-primary cluster
||

//@ checkInstanceState: ArgumentErrors
||Invalid number of arguments, expected 1 but got 0 (ArgumentError)
||Invalid URI: empty. (ArgumentError)
||Argument #1 is expected to be a string (ArgumentError)
||Invalid connection options, no options provided. (ArgumentError)

//@ checkInstanceState: unreachable instance
|ERROR: Failed to connect to instance: Can't connect to MySQL server on 'localhost' (111)|Cluster.checkInstanceState: The instance 'localhost:11111' is not reachable. (RuntimeError)

//@<ERR> checkInstanceState: cluster member
Cluster.checkInstanceState: The instance 'localhost:<<<__mysql_sandbox_port1>>>' already belongs to the ReplicaSet: 'default'. (RuntimeError)

//@ Create a single-primary cluster on instance 2
||

//@ Drop metadatata schema from instance 2
||

//@<ERR> checkInstanceState: instance belongs to unmanaged GR group
Cluster.checkInstanceState: The instance 'localhost:<<<__mysql_sandbox_port2>>>' belongs to a Group Replication group that is not managed as an InnoDB cluster. (RuntimeError)

//@ Re-create cluster
||

//@ Stop GR on instance 2
||

//@<ERR> checkInstanceState: is a standalone instance but is part of a different InnoDB Cluster
Cluster.checkInstanceState: The instance 'localhost:<<<__mysql_sandbox_port2>>>' is a standalone instance but is part of a different InnoDB Cluster (metadata exists, but Group Replication is not active). (RuntimeError)

//@ Drop metadatata schema from instance 2 to get diverged GTID
||

//@<OUT> checkInstanceState: state: error, reason: diverged
Analyzing the instance 'localhost:<<<__mysql_sandbox_port2>>>' replication state...

The instance 'localhost:<<<__mysql_sandbox_port2>>>' is invalid for the cluster.
The instance contains additional transactions in relation to the cluster.

{
    "reason": "diverged",
    "state": "error"
}

//@ Destroy cluster2
||

//@ Deploy instance 2
||

//@<OUT> checkInstanceState: state: ok, reason: new
Analyzing the instance 'localhost:<<<__mysql_sandbox_port2>>>' replication state...

The instance 'localhost:<<<__mysql_sandbox_port2>>>' is valid for the cluster.
The instance is new to Group Replication.

{
    "reason": "new",
    "state": "ok"
}

//@ Add instance 2 to the cluster
||

//@ Remove instance 2 from the cluster manually
||

//@ Drop metadatata schema from instance 2 with binlog disabled
||

//@ Remove instance 2 from cluster metadata
||

//@<OUT> checkInstanceState: state: ok, reason: recoverable
Analyzing the instance 'localhost:<<<__mysql_sandbox_port2>>>' replication state...

The instance 'localhost:<<<__mysql_sandbox_port2>>>' is valid for the cluster.
The instance is fully recoverable.

{
    "reason": "recoverable",
    "state": "ok"
}

//@<OUT> checkInstanceState: state: error, reason: lost_transactions
Analyzing the instance 'localhost:<<<__mysql_sandbox_port2>>>' replication state...

The instance 'localhost:<<<__mysql_sandbox_port2>>>' is invalid for the cluster.
There are transactions in the cluster that can't be recovered on the instance.

{
    "reason": "lost_transactions",
    "state": "error"
}

//@ Finalization
||
