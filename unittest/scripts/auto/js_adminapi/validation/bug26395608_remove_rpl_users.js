//@ Create a cluster with 3 members.
||

//@ Get initial number of replication users.
||

//@ Remove last added instance.
||

//@ Connect to removed instance.
||

//@<OUT> Confirm that all replication users where removed.
0

//@ Connect back to primary and get cluster.
||

//@<OUT> Confirm that some replication user was removed.
true

//@ Dissolve cluster.
||

//@<OUT> Confirm that replication users where removed on primary.
0

//@ Connect to remaining instance.
||

//@<OUT> Confirm that replication users where removed on remaining instance.
0
