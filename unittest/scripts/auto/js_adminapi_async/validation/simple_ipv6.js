
//@# configureReplicaSetInstance + create admin user
|This instance reports its own address as [::1]:<<<__mysql_sandbox_port1>>>|
|The instance '[::1]:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB ReplicaSet.|

//@# configureReplicaSetInstance
|This instance reports its own address as [::1]:<<<__mysql_sandbox_port2>>>|
|The instance '[::1]:<<<__mysql_sandbox_port2>>>' is valid to be used in an InnoDB ReplicaSet.|
|The instance '[::1]:<<<__mysql_sandbox_port3>>>' is valid to be used in an InnoDB ReplicaSet.|

//@# createReplicaSet
|This instance reports its own address as [::1]:<<<__mysql_sandbox_port1>>>|
|ReplicaSet object successfully created for [::1]:<<<__mysql_sandbox_port1>>>.|

//@# addInstance (incremental)
|This instance reports its own address as [::1]:<<<__mysql_sandbox_port2>>>|
|The instance '[::1]:<<<__mysql_sandbox_port2>>>' was added to the replicaset and is replicating from [::1]:<<<__mysql_sandbox_port1>>>.|

//@# addInstance (clone) (should fail)
||IPv6 addresses not supported for cloneDonor (ArgumentError)

|ERROR: None of the members in the replicaSet are compatible to be used as clone donors for [::1]:<<<__mysql_sandbox_port3>>>|
|SECONDARY '[::1]:<<<__mysql_sandbox_port2>>>' is not a suitable clone donor: Instance hostname/report_host is an IPv6 address, which is not supported for cloning|
|PRIMARY '[::1]:<<<__mysql_sandbox_port1>>>' is not a suitable clone donor: Instance hostname/report_host is an IPv6 address, which is not supported for cloning|
|ERROR: Error adding instance to replicaset: MYSQLSH 51400: The ReplicaSet has no compatible clone donors.|
||The ReplicaSet has no compatible clone donors. (MYSQLSH 51400)

//@# status
|{|
|    "replicaSet": {|
|        "name": "myrs",|
|        "primary": "[::1]:<<<__mysql_sandbox_port1>>>",|
|        "status": "AVAILABLE",|
|        "statusText": "All instances available.",|
|        "topology": {|
|            "[::1]:<<<__mysql_sandbox_port1>>>": {|
|                "address": "[::1]:<<<__mysql_sandbox_port1>>>",|
|                "instanceRole": "PRIMARY",|
|                "mode": "R/W",|
|                "status": "ONLINE"|
|            },|
|            "[::1]:<<<__mysql_sandbox_port2>>>": {|
|                "address": "[::1]:<<<__mysql_sandbox_port2>>>",|
|                "instanceRole": "SECONDARY",|
|                "mode": "R/O",|
|                "replication": {|
|                    "applierStatus": "APPLIED_ALL",|
|                    "applierThreadState": <<<(__version_num<80023)?'"Slave has read all relay log; waiting for more updates",':'"Waiting for an event from Coordinator",'>>>|
|                    <<<(__version_num<80023)?'"applierWorkerThreads": 4':''>>>|
|                    "receiverStatus": "ON",|
|                    "receiverThreadState": "Waiting for <<<__source_keyword>>> to send event",|
|                    "replicationLag": null|
|                },|
|                "status": "ONLINE"|
|            },|
|            "[::1]:<<<__mysql_sandbox_port3>>>": {|
|                "address": "[::1]:<<<__mysql_sandbox_port3>>>",|
|                "instanceRole": "SECONDARY",|
|                "mode": "R/O",|
|                "replication": {|
|                    "applierStatus": "APPLIED_ALL",|
|                    "applierThreadState": <<<(__version_num<80023)?'"Slave has read all relay log; waiting for more updates",':'"Waiting for an event from Coordinator",'>>>|
|                    <<<(__version_num<80023)?'"applierWorkerThreads": 4':''>>>|
|                    "receiverStatus": "ON",|
|                    "receiverThreadState": "Waiting for <<<__source_keyword>>> to send event",|
|                    "replicationLag": null|
|                },|
|                "status": "ONLINE"|
|            }|
|        },|
|        "type": "ASYNC"|
|    }|
|}|


//@# check IPv6 addresses in MD
|instance_id	cluster_id	address	mysql_server_uuid	instance_name	addresses	attributes	description|
|1	[[*]]	[::1]:<<<__mysql_sandbox_port1>>>	[[*]]	[::1]:<<<__mysql_sandbox_port1>>>	{"mysqlX": "[::1]:<<<__mysql_sandbox_port1>>>0", "mysqlClassic": "[::1]:<<<__mysql_sandbox_port1>>>"}	{"server_id": [[*]]}	NULL|
|2	[[*]]	[::1]:<<<__mysql_sandbox_port2>>>	[[*]]	[::1]:<<<__mysql_sandbox_port2>>>	{"mysqlX": "[::1]:<<<__mysql_sandbox_port2>>>0", "mysqlClassic": "[::1]:<<<__mysql_sandbox_port2>>>"}	{"server_id": [[*]]}	NULL|
|3	[[*]]	[::1]:<<<__mysql_sandbox_port3>>>	[[*]]	[::1]:<<<__mysql_sandbox_port3>>>	{"mysqlX": "[::1]:<<<__mysql_sandbox_port3>>>0", "mysqlClassic": "[::1]:<<<__mysql_sandbox_port3>>>"}	{"server_id": [[*]]}	NULL|


//@# rejoinInstance (clone) (should fail) {VER(>=8.0.17)}
|ERROR: None of the members in the replicaSet are compatible to be used as clone donors for [::1]:<<<__mysql_sandbox_port3>>>|
|SECONDARY '[::1]:<<<__mysql_sandbox_port2>>>' is not a suitable clone donor: Instance hostname/report_host is an IPv6 address, which is not supported for cloning|
|PRIMARY '[::1]:<<<__mysql_sandbox_port1>>>' is not a suitable clone donor: Instance hostname/report_host is an IPv6 address, which is not supported for cloning|
|ERROR: Error rejoining instance to replicaset: MYSQLSH 51400: The ReplicaSet has no compatible clone donors.|
||The ReplicaSet has no compatible clone donors. (MYSQLSH 51400)

//@ createReplicaSet(adopt)
|{|
|    "replicaSet": {|
|        "name": "adopted", |
|        "primary": "[::1]:<<<__mysql_sandbox_port1>>>", |
|        "status": "AVAILABLE", |
|        "statusText": "All instances available.", |
|        "topology": {|
|            "[::1]:<<<__mysql_sandbox_port1>>>": {|
|                "address": "[::1]:<<<__mysql_sandbox_port1>>>", |
|                "instanceRole": "PRIMARY", |
|                "mode": "R/W", |
|                "status": "ONLINE"|
|            }, |
|            "[::1]:<<<__mysql_sandbox_port2>>>": {|
|                "address": "[::1]:<<<__mysql_sandbox_port2>>>", |
|                "instanceRole": "SECONDARY", |
|                "mode": "R/O", |
|                "replication": {|
|                    "applierStatus": "APPLIED_ALL", |
|                    "applierThreadState": <<<(__version_num<80023)?'"Slave has read all relay log; waiting for more updates",':'"Waiting for an event from Coordinator",'>>>|
|                    <<<(__version_num<80023)?'"applierWorkerThreads": 4':''>>>|
|                    "receiverStatus": "ON", |
|                    "receiverThreadState": "Waiting for <<<__source_keyword>>> to send event", |
|                    "replicationLag": null|
|                }, |
|                "status": "ONLINE"|
|            }|
|        }, |
|        "type": "ASYNC"|
|    }|
|}|
