//@<OUT> createClusterSet
A new ClusterSet will be created based on the Cluster 'cluster'.

* Validating Cluster 'cluster' for ClusterSet compliance.

* Creating InnoDB ClusterSet 'clusterset' on 'cluster'...

* Updating metadata...

ClusterSet successfully created. Use ClusterSet.createReplicaCluster() to add Replica Clusters to it.

//@ createReplicaCluster() - incremental recovery
|Setting up replica 'replicacluster' of cluster 'cluster' at instance '<<<hostname>>>:<<<__mysql_sandbox_port4>>>'.|
||
|A new InnoDB cluster will be created on instance 'localhost:<<<__mysql_sandbox_port4>>>'.|
||
|Creating InnoDB cluster 'replicacluster' on '<<<hostname>>>:<<<__mysql_sandbox_port4>>>'...|
||
|Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.|
|At least 3 instances are needed for the cluster to be able to withstand up to|
|one server failure.|
||
|* Updating topology|
||
|Replica Cluster 'replicacluster' successfully created on ClusterSet 'clusterset'.|
||

//@<OUT> removeCluster()
The Cluster 'replicacluster' will be removed from the InnoDB ClusterSet.

* Waiting for the Cluster to synchronize with the PRIMARY Cluster...

* Updating topology

* Stopping and deleting ClusterSet managed replication channel...

The Cluster 'replicacluster' was removed from the ClusterSet.
