#@<OUT> Dba: create_cluster multiMaster with interaction, cancel
A new InnoDB cluster will be created on instance 'root@localhost:<<<__mysql_sandbox_port1>>>'.

The MySQL InnoDB cluster is going to be setup in advanced Multi-Master Mode.
Before continuing you have to confirm that you understand the requirements and
limitations of Multi-Master Mode. Please read the manual before proceeding.


I have read the MySQL InnoDB cluster manual and I understand the requirements
and limitations of advanced Multi-Master Mode.
Confirm (Yes/No): Cancelled

#@<OUT> Dba: create_cluster multiMaster with interaction, ok
A new InnoDB cluster will be created on instance 'root@localhost:<<<__mysql_sandbox_port1>>>'.

The MySQL InnoDB cluster is going to be setup in advanced Multi-Master Mode.
Before continuing you have to confirm that you understand the requirements and
limitations of Multi-Master Mode. Please read the manual before proceeding.


I have read the MySQL InnoDB cluster manual and I understand the requirements
and limitations of advanced Multi-Master Mode.
Confirm (Yes/No): Creating InnoDB cluster 'devCluster' on 'root@localhost:<<<__mysql_sandbox_port1>>>'...
Adding Seed Instance...

Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

<Cluster:devCluster>

#@ Cluster: add_instance with interaction, error
||Cluster.add_instance: The instance 'localhost:<<<__mysql_sandbox_port1>>>' is already part of this InnoDB cluster

#@<OUT> Cluster: add_instance with interaction, ok
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Adding instance to the cluster ...

The instance 'root@localhost:<<<__mysql_sandbox_port2>>>' was successfully added to the cluster.

#@<OUT> Cluster: add_instance 3 with interaction, ok
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Adding instance to the cluster ...

The instance 'root@localhost:<<<__mysql_sandbox_port3>>>' was successfully added to the cluster.

#@<OUT> Cluster: describe1
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "instances": [
            {
                "host": "localhost:<<<__mysql_sandbox_port1>>>",
                "label": "localhost:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            },
            {
                "host": "localhost:<<<__mysql_sandbox_port2>>>",
                "label": "localhost:<<<__mysql_sandbox_port2>>>",
                "role": "HA"
            },
            {
                "host": "localhost:<<<__mysql_sandbox_port3>>>",
                "label": "localhost:<<<__mysql_sandbox_port3>>>",
                "role": "HA"
            }
        ],
        "name": "default"
    }
}

#@<OUT> Cluster: status1
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "status": "Cluster tolerant to up to ONE failure.",
        "topology": {
            "localhost:<<<__mysql_sandbox_port1>>>": {
                "address": "localhost:<<<__mysql_sandbox_port1>>>",
                "leaves": {},
                "mode": "R/W",
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port2>>>": {
                "address": "localhost:<<<__mysql_sandbox_port2>>>",
                "leaves": {},
                "mode": "R/W",
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port3>>>": {
                "address": "localhost:<<<__mysql_sandbox_port3>>>",
                "leaves": {},
                "mode": "R/W",
                "role": "HA",
                "status": "ONLINE"
            }
        }
    }
}

#@ Cluster: remove_instance
||

#@<OUT> Cluster: describe2
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "instances": [
            {
                "host": "localhost:<<<__mysql_sandbox_port1>>>",
                "label": "localhost:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            },
            {
                "host": "localhost:<<<__mysql_sandbox_port3>>>",
                "label": "localhost:<<<__mysql_sandbox_port3>>>",
                "role": "HA"
            }
        ],
        "name": "default"
    }
}

#@<OUT> Cluster: status2
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "status": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "localhost:<<<__mysql_sandbox_port1>>>": {
                "address": "localhost:<<<__mysql_sandbox_port1>>>",
                "leaves": {},
                "mode": "R/W",
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port3>>>": {
                "address": "localhost:<<<__mysql_sandbox_port3>>>",
                "leaves": {},
                "mode": "R/W",
                "role": "HA",
                "status": "ONLINE"
            }
        }
    }
}

#@ Cluster: remove_instance 3
||

#@ Cluster: remove_instance last
||

#@<OUT> Cluster: describe3
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "instances": [],
        "name": "default"
    }
}

#@<OUT> Cluster: status3
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "status": "Cluster is NOT tolerant to any failures.",
        "topology": {}
    }
}

#@<OUT> Cluster: add_instance with interaction, ok 2
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Adding instance to the cluster ...

The instance 'root@localhost:<<<__mysql_sandbox_port1>>>' was successfully added to the cluster.

#@<OUT> Cluster: add_instance with interaction, ok 3
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Adding instance to the cluster ...

The instance 'root@localhost:<<<__mysql_sandbox_port2>>>' was successfully added to the cluster.

#@<OUT> Cluster: add_instance with interaction, ok 4
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Adding instance to the cluster ...

The instance 'root@localhost:<<<__mysql_sandbox_port3>>>' was successfully added to the cluster.

#@<OUT> Cluster: status: success
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "status": "Cluster tolerant to up to ONE failure.",
        "topology": {
            "localhost:<<<__mysql_sandbox_port1>>>": {
                "address": "localhost:<<<__mysql_sandbox_port1>>>",
                "leaves": {},
                "mode": "R/W",
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port2>>>": {
                "address": "localhost:<<<__mysql_sandbox_port2>>>",
                "leaves": {},
                "mode": "R/W",
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port3>>>": {
                "address": "localhost:<<<__mysql_sandbox_port3>>>",
                "leaves": {},
                "mode": "R/W",
                "role": "HA",
                "status": "ONLINE"
            }
        }
    }
}

#@# Dba: kill instance 3
||

#@# Dba: start instance 3
||

#@: Cluster: rejoin_instance errors
||Invalid number of arguments in Cluster.rejoin_instance, expected 1 to 2 but got 0
||Invalid number of arguments in Cluster.rejoin_instance, expected 1 to 2 but got 3
||Invalid connection options, expected either a URI or a Dictionary
||Cluster.rejoin_instance: The instance 'localhost:3306' does not belong to the ReplicaSet: 'default'
||Cluster.rejoin_instance: Invalid values in instance definition: authMethod, schema
||Cluster.rejoin_instance: The instance 'somehost:3306' does not belong to the ReplicaSet: 'default'

#@<OUT> Cluster: rejoin_instance with interaction, ok
The instance will try rejoining the InnoDB cluster. Depending on the original
problem that made the instance unavailable, the rejoin operation might not be
successful and further manual steps will be needed to fix the underlying
problem.

Please monitor the output of the rejoin operation and take necessary action if
the instance cannot rejoin.

Please provide the password for 'root@localhost:<<<__mysql_sandbox_port3>>>': Rejoining instance to the cluster ...

The instance 'root@localhost:<<<__mysql_sandbox_port3>>>' was successfully rejoined on the cluster.

#@<OUT> Cluster: status for rejoin: success
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "status": "Cluster tolerant to up to ONE failure.",
        "topology": {
            "localhost:<<<__mysql_sandbox_port1>>>": {
                "address": "localhost:<<<__mysql_sandbox_port1>>>",
                "leaves": {},
                "mode": "R/W",
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port2>>>": {
                "address": "localhost:<<<__mysql_sandbox_port2>>>",
                "leaves": {},
                "mode": "R/W",
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port3>>>": {
                "address": "localhost:<<<__mysql_sandbox_port3>>>",
                "leaves": {},
                "mode": "R/W",
                "role": "HA",
                "status": "ONLINE"
            }
        }
    }
}
