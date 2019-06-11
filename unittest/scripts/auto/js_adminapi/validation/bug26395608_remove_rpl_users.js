//@ Create a cluster with 3 members.
||

//@ Get initial number of replication users.
|3|

//@ Remove last added instance.
||

//@ Connect to removed instance.
||

//@<OUT> Confirm that replication user was removed and other were kept (BUG#29559303).
2

//@ Connect back to primary and get cluster.
||

//@<OUT> Confirm that a single replication user was removed (BUG#29559303).
true

//@ Dissolve cluster.
||

//@<OUT> Confirm that replication users where removed on primary.
0

//@ Connect to remaining instance.
||

//@<OUT> Confirm that replication users where removed on remaining instance.
0
