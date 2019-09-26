//@# INCLUDE async_utils.inc
||

//@# create replicaset (should fail)
||Dba.createReplicaSet: Operation not allowed. The installed metadata version 1.0.1 is lower than the supported by the Shell which is version 2.0.0. Upgrade the metadata to execute this operation. See \? dba.upgradeMetadata for additional details. (RuntimeError)

//@ Merge schema from different sources (cluster then rs)
||

//@<OUT> clusters
*************************** 1. row ***************************
  cluster_type: gr
  primary_mode: pm
    cluster_id: [[*]]
  cluster_name: mycluster
router_options: NULL
*************************** 2. row ***************************
  cluster_type: ar
  primary_mode: pm
    cluster_id: [[*]]
  cluster_name: myrs
router_options: NULL
2

//@<OUT> instances
*************************** 1. row ***************************
      instance_id: 1
       cluster_id: [[*]]
            label: 127.0.0.1:<<<__mysql_sandbox_port1>>>
mysql_server_uuid: <<<uuid1>>>
          address: 127.0.0.1:<<<__mysql_sandbox_port1>>>
         endpoint: 127.0.0.1:<<<__mysql_sandbox_port1>>>
        xendpoint: 127.0.0.1:<<<__mysql_sandbox_port1>>>0
       attributes: {"recoveryAccountHost": "%", "recoveryAccountUser": "mysql_innodb_cluster_11"}
*************************** 2. row ***************************
      instance_id: 2
       cluster_id: [[*]]
            label: 127.0.0.1:<<<__mysql_sandbox_port2>>>
mysql_server_uuid: <<<uuid2>>>
          address: 127.0.0.1:<<<__mysql_sandbox_port2>>>
         endpoint: 127.0.0.1:<<<__mysql_sandbox_port2>>>
        xendpoint: 127.0.0.1:<<<__mysql_sandbox_port2>>>0
       attributes: {}
2

//@# this_instance
|cluster_id	instance_id	cluster_name	cluster_type|
|<<<cluster_id1>>>	1	mycluster	gr|
|cluster_id	instance_id	cluster_name	cluster_type|
|<<<cluster_id2>>>	2	myrs	ar|

//@# Check sb1.getCluster() with mixed metadata
|<Cluster:mycluster>|
|    "clusterName": "mycluster",|
|       "primary": "127.0.0.1:<<<__mysql_sandbox_port1>>>",|

//@# Check sb1.getReplicaSet() with mixed metadata (should fail)
||Dba.getReplicaSet: This function is not available through a session to an instance already in an InnoDB cluster

//@# Check sb2.getCluster() with mixed metadata (should fail)
||Dba.getCluster: This function is not available through a session to an instance that is member of an InnoDB ReplicaSet

//@# Check sb2.getReplicaSet() with mixed metadata
|<ReplicaSet:myrs>|
|"name": "myrs",|
|"primary": "127.0.0.1:<<<__mysql_sandbox_port2>>>",|

//@# Merge schema from different sources (rs then cluster)
|               Slave_IO_State: Waiting for master to send event|
|      Slave_SQL_Running_State: Slave has read all relay log; waiting for more updates|
|cluster_type	primary_mode	cluster_id	cluster_name	router_options|
|ar	pm	<<<cluster_id1>>>	myrs	NULL|
|gr	pm	<<<cluster_id2>>>	mycluster	NULL|
|instance_id	cluster_id	label	mysql_server_uuid	address	endpoint	xendpoint	attributes|
|1	<<<cluster_id1>>>	127.0.0.1:<<<__mysql_sandbox_port1>>>	<<<uuid1>>>	127.0.0.1:<<<__mysql_sandbox_port1>>>	127.0.0.1:<<<__mysql_sandbox_port1>>>	127.0.0.1:<<<__mysql_sandbox_port1>>>0	{}|
|2	<<<cluster_id2>>>	127.0.0.1:<<<__mysql_sandbox_port2>>>	<<<uuid2>>>	127.0.0.1:<<<__mysql_sandbox_port2>>>	127.0.0.1:<<<__mysql_sandbox_port2>>>	127.0.0.1:<<<__mysql_sandbox_port2>>>0	{"recoveryAccountHost": "%", "recoveryAccountUser": "mysql_innodb_cluster_22"}|

//@# this_instance again
|cluster_id	instance_id	cluster_name	cluster_type|
|<<<cluster_id1>>>	1	myrs	ar|
|cluster_id	instance_id	cluster_name	cluster_type|
|<<<cluster_id2>>>	2	mycluster	gr|

//@# Check sb1.getCluster() with mixed metadata again (should fail)
||Dba.getCluster: This function is not available through a session to an instance that is member of an InnoDB ReplicaSet (MYSQLSH 51306)

//@# Check sb1.getReplicaSet() with mixed metadata again
|<ReplicaSet:myrs>|
|"name": "myrs",|
|"primary": "127.0.0.1:<<<__mysql_sandbox_port1>>>",|

//@# Check sb2.getCluster() with mixed metadata again
|<Cluster:mycluster>|
|    "clusterName": "mycluster",|
|       "primary": "127.0.0.1:<<<__mysql_sandbox_port2>>>",|

//@# Check sb2.getReplicaSet() with mixed metadata again (should fail)
||Dba.getReplicaSet: This function is not available through a session to an instance already in an InnoDB cluster
