//@# INCLUDE async_utils.inc
||

//@# create replicaset (should fail)
||Operation not allowed. The installed metadata version 1.0.1 is lower than the version required by Shell which is version 2.1.0. Upgrade the metadata to execute this operation. See \? dba.upgradeMetadata for additional details. (RuntimeError)

//@ Merge schema from different sources (cluster then rs)
||

//@<OUT> clusters
*************************** 1. row ***************************
  cluster_type: gr
  primary_mode: pm
    cluster_id: [[*]]
  cluster_name: mycluster
router_options: NULL
1

//@<OUT> instances
*************************** 1. row ***************************
      instance_id: 1
       cluster_id: [[*]]
            label: 127.0.0.1:<<<__mysql_sandbox_port1>>>
mysql_server_uuid: <<<uuid1>>>
          address: 127.0.0.1:<<<__mysql_sandbox_port1>>>
         endpoint: 127.0.0.1:<<<__mysql_sandbox_port1>>>
        xendpoint: 127.0.0.1:<<<__mysql_sandbox_port1>>>0
       attributes: {"server_id": 11, "recoveryAccountHost": "%", "recoveryAccountUser": "mysql_innodb_cluster_11"}
1

//@# this_instance
|cluster_id	instance_id	cluster_name	cluster_type|
|<<<cluster_id2>>>	1	myrs	ar|

//@# Check sb1.getCluster()
||

//@# Check sb1.getReplicaSet() (should fail)
||This function is not available through a session to an instance already in an InnoDB Cluster

//@# Check sb2.getCluster() (should fail)
|No InnoDB Cluster found, did you meant to call dba.getReplicaSet()?|
||This function is not available through a session to an instance that is a member of an InnoDB ReplicaSet

//@# Check sb2.getReplicaSet()
|<ReplicaSet:myrs>|
|"metadataVersion": "<<<testutil.getInstalledMetadataVersion()>>>",|
|"name": "myrs",|
|"primary": "127.0.0.1:<<<__mysql_sandbox_port2>>>",|

//@# Merge schema from different sources (rs then cluster)
|cluster_type	primary_mode	cluster_id	cluster_name	router_options|
|ar	pm	<<<cluster_id1>>>	myrs	NULL|
|instance_id	cluster_id	label	mysql_server_uuid	address	endpoint	xendpoint	attributes|
|1	<<<cluster_id1>>>	127.0.0.1:<<<__mysql_sandbox_port1>>>	<<<uuid1>>>	127.0.0.1:<<<__mysql_sandbox_port1>>>	127.0.0.1:<<<__mysql_sandbox_port1>>>	127.0.0.1:<<<__mysql_sandbox_port1>>>0	{"server_id": 11, "replicationAccountHost": "%", "replicationAccountUser": "mysql_innodb_rs_11"}|

//@# this_instance again
|cluster_id	instance_id	cluster_name	cluster_type|
|<<<cluster_id1>>>	1	myrs	ar|

//@# Check sb1.getCluster() (should fail)
|No InnoDB Cluster found, did you meant to call dba.getReplicaSet()?|
||This function is not available through a session to an instance that is a member of an InnoDB ReplicaSet

//@# Check sb1.getReplicaSet()
|<ReplicaSet:myrs>|
|"name": "myrs",|
|"primary": "127.0.0.1:<<<__mysql_sandbox_port1>>>",|

//@# Check sb2.getCluster()
|<Cluster:mycluster>|
|    "clusterName": "mycluster",|
|       "primary": "127.0.0.1:<<<__mysql_sandbox_port2>>>",|

//@# Check sb2.getReplicaSet() (should fail)
||This function is not available through a session to an instance already in an InnoDB Cluster
