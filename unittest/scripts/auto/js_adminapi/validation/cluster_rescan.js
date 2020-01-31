//@ Initialize.
||

//@ Get needed Server_Ids and UUIDs.
||

//@ Configure sandboxes.
||

//@<OUT> Create cluster.
{
    "clusterName": "c", 
    "defaultReplicaSet": {
        "name": "default", 
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED", 
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W", 
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA", 
                "status": "ONLINE",
                "version": "<<<__version>>>"
            }, 
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O", 
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA", 
                "status": "ONLINE",
                "version": "<<<__version>>>"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            }
        },
        "topologyMode": "Single-Primary"
    }, 
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@<OUT> No-op.
Rescanning the cluster...

Result of the rescanning operation for the 'c' cluster:
{
    "name": "c",
    "newTopologyMode": null,
    "newlyDiscoveredInstances": [],
    "unavailableInstances": []
}

//@ WL10644 - TSF2_6: empty addInstances throw ArgumentError.
||Cluster.rescan: The list for 'addInstances' option cannot be empty. (ArgumentError)

//@ WL10644 - TSF2_8: invalid addInstances list throw ArgumentError.
||Cluster.rescan: Invalid value 'undefined' for 'addInstances' option: Invalid connection options, expected either a URI or a Dictionary. (ArgumentError)
||Cluster.rescan: Invalid value 'localhost' for 'addInstances' option: port is missing. (ArgumentError)
||Cluster.rescan: Invalid value ':3301' for 'addInstances' option: host cannot be empty. (ArgumentError)
||Cluster.rescan: Invalid value '@' for 'addInstances' option: Invalid URI: Missing user information (ArgumentError)
||Cluster.rescan: Invalid value '{}' for 'addInstances' option: Invalid connection options, no options provided. (ArgumentError)
||Cluster.rescan: Invalid value '{"host": "myhost"}' for 'addInstances' option: port is missing. (ArgumentError)
||Cluster.rescan: Invalid value '{"host": ""}' for 'addInstances' option: Host value cannot be an empty string. (ArgumentError)

//@ WL10644: Duplicated values for addInstances.
||Cluster.rescan: Duplicated value found for instance 'localhost:3301' in 'addInstances' option. (ArgumentError)
||Cluster.rescan: Duplicated value found for instance 'localhost:3301' in 'addInstances' option. (ArgumentError)
||Cluster.rescan: Duplicated value found for instance 'localhost:3301' in 'addInstances' option. (ArgumentError)

//@ WL10644 - TSF2_9: invalid value with addInstances throw ArgumentError.
||Cluster.rescan: Option 'addInstances' only accepts 'auto' as a valid string value, otherwise a list of instances is expected. (ArgumentError)

//@ WL10644 - TSF2_7: "auto" is case insensitive, no error.
||

//@ WL10644: Invalid type used for addInstances.
||Cluster.rescan: Option 'addInstances' is expected to be of type Array, but is Map (TypeError)
||Cluster.rescan: Option 'addInstances' is expected to be of type Array, but is Bool (TypeError)
||Cluster.rescan: Option 'addInstances' is expected to be of type Array, but is Integer (TypeError)

//@<OUT> WL10644 - TSF2_10: not active member in addInstances throw RuntimeError.
ERROR: The following instances cannot be added because they are not active members of the cluster: 'localhost:1111'. Please verify if the specified addresses are correct, or if the instances are currently inactive.

//@<ERR> WL10644 - TSF2_10: not active member in addInstances throw RuntimeError.
Cluster.rescan: The following instances are not active members of the cluster: 'localhost:1111'. (RuntimeError)

//@<OUT> WL10644 - TSF2_11: warning for already members in addInstances.
Rescanning the cluster...

Result of the rescanning operation for the 'c' cluster:
{
    "name": "c",
    "newTopologyMode": null,
    "newlyDiscoveredInstances": [],
    "unavailableInstances": []
}

WARNING: The following instances were not added to the metadata because they are already part of the cluster: '<<<member_address>>>'. Please verify if the specified value for 'addInstances' option is correct.

//@ WL10644 - TSF3_6: empty removeInstances throw ArgumentError.
||Cluster.rescan: The list for 'removeInstances' option cannot be empty. (ArgumentError)

//@ WL10644 - TSF3_8: invalid removeInstances list throw ArgumentError.
||Cluster.rescan: Invalid value 'undefined' for 'removeInstances' option: Invalid connection options, expected either a URI or a Dictionary. (ArgumentError)
||Cluster.rescan: Invalid value 'localhost' for 'removeInstances' option: port is missing. (ArgumentError)
||Cluster.rescan: Invalid value ':3301' for 'removeInstances' option: host cannot be empty. (ArgumentError)
||Cluster.rescan: Invalid value '@' for 'removeInstances' option: Invalid URI: Missing user information (ArgumentError)
||Cluster.rescan: Invalid value '{}' for 'removeInstances' option: Invalid connection options, no options provided. (ArgumentError)
||Cluster.rescan: Invalid value '{"host": "myhost"}' for 'removeInstances' option: port is missing. (ArgumentError)
||Cluster.rescan: Invalid value '{"host": ""}' for 'removeInstances' option: Host value cannot be an empty string. (ArgumentError)

//@ WL10644: Duplicated values for removeInstances.
||Cluster.rescan: Duplicated value found for instance 'localhost:3301' in 'removeInstances' option. (ArgumentError)
||Cluster.rescan: Duplicated value found for instance 'localhost:3301' in 'removeInstances' option. (ArgumentError)
||Cluster.rescan: Duplicated value found for instance 'localhost:3301' in 'removeInstances' option. (ArgumentError)

//@ WL10644 - TSF3_9: invalid value with removeInstances throw ArgumentError.
||Cluster.rescan: Option 'removeInstances' only accepts 'auto' as a valid string value, otherwise a list of instances is expected. (ArgumentError)

//@ WL10644 - TSF3_7: "auto" is case insensitive, no error.
||

//@ WL10644: Invalid type used for removeInstances.
||Cluster.rescan: Option 'removeInstances' is expected to be of type Array, but is Map (TypeError)
||Cluster.rescan: Option 'removeInstances' is expected to be of type Array, but is Bool (TypeError)
||Cluster.rescan: Option 'removeInstances' is expected to be of type Array, but is Integer (TypeError)

//@<OUT> WL10644 - TSF3_10: active member in removeInstances throw RuntimeError.
ERROR: The following instances cannot be removed because they are active members of the cluster: '<<<member_address>>>'. Please verify if the specified addresses are correct.

//@<ERR> WL10644 - TSF3_10: active member in removeInstances throw RuntimeError.
Cluster.rescan: The following instances are active members of the cluster: '<<<member_address>>>'. (RuntimeError)

//@<OUT> WL10644 - TSF3_11: warning for not members in removeInstances.
Rescanning the cluster...

Result of the rescanning operation for the 'c' cluster:
{
    "name": "c",
    "newTopologyMode": null,
    "newlyDiscoveredInstances": [],
    "unavailableInstances": []
}

WARNING: The following instances were not removed from the metadata because they are already not part of the cluster or are running auto-rejoin: 'localhost:1111'. Please verify if the specified value for 'removeInstances' option is correct.

//@ WL10644: Duplicated values between addInstances and removeInstances.
||Cluster.rescan: The same instances cannot be used in both 'addInstances' and 'removeInstances' options: 'localhost:3300, localhost:3301'. (ArgumentError)
||Cluster.rescan: The same instance cannot be used in both 'addInstances' and 'removeInstances' options: 'localhost:3301'. (ArgumentError)
||Cluster.rescan: The same instance cannot be used in both 'addInstances' and 'removeInstances' options: 'localhost:3301'. (ArgumentError)

//@<OUT> Remove instance on port 2 and 3 from MD but keep it in the group.
{
    "clusterName": "c", 
    "defaultReplicaSet": {
        "name": "default", 
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED", 
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W", 
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA", 
                "status": "ONLINE",
                "version": "<<<__version>>>"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>", 
                "instanceErrors": [
                    "WARNING: Instance is not managed by InnoDB cluster. Use cluster.rescan() to repair."
                ], 
                "mode": "R/O", 
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA", 
                "status": "ONLINE", 
                "version": "[[*]]"
            }, 
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>", 
                "instanceErrors": [
                    "WARNING: Instance is not managed by InnoDB cluster. Use cluster.rescan() to repair."
                ], 
                "mode": "R/O", 
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA", 
                "status": "ONLINE", 
                "version": "[[*]]"
            }
        },
        "topologyMode": "Single-Primary"
    }, 
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@<OUT> WL10644 - TSF2_1: Rescan with addInstances:[complete_valid_list].
Rescanning the cluster...

Result of the rescanning operation for the 'c' cluster:
{
    "name": "c",
    "newTopologyMode": null,
    "newlyDiscoveredInstances": [
        {
            "host": "<<<member_address2>>>",
            "member_id": "<<<instance2_uuid>>>",
            "name": null<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
        },
        {
            "host": "<<<member_address3>>>",
            "member_id": "<<<instance3_uuid>>>",
            "name": null<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
        }
    ],
    "unavailableInstances": []
}

A new instance '<<<member_address2>>>' was discovered in the cluster.
Adding instance to the cluster metadata...
The instance '<<<member_address2>>>' was successfully added to the cluster metadata.

A new instance '<<<member_address3>>>' was discovered in the cluster.
Adding instance to the cluster metadata...
The instance '<<<member_address3>>>' was successfully added to the cluster metadata.


//@<OUT> WL10644 - TSF2_1: Validate that the instances were added.
{
    "clusterName": "c",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            },
            "<<<member_address2>>>": {
                "address": "<<<member_address2>>>",
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            },
            "<<<member_address3>>>": {
                "address": "<<<member_address3>>>",
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@<OUT> WL10644 - TSF2_2: Remove instances on port 2 and 3 from MD again.
{
    "clusterName": "c",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>", 
                "instanceErrors": [
                    "WARNING: Instance is not managed by InnoDB cluster. Use cluster.rescan() to repair."
                ], 
                "mode": "R/O", 
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA", 
                "status": "ONLINE", 
                "version": "[[*]]"
            }, 
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>", 
                "instanceErrors": [
                    "WARNING: Instance is not managed by InnoDB cluster. Use cluster.rescan() to repair."
                ], 
                "mode": "R/O", 
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA", 
                "status": "ONLINE", 
                "version": "[[*]]"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@<OUT> WL10644 - TSF2_2: Rescan with addInstances:[incomplete_valid_list] and interactive:true.
Rescanning the cluster...

Result of the rescanning operation for the 'c' cluster:
{
    "name": "c",
    "newTopologyMode": null,
    "newlyDiscoveredInstances": [
        {
            "host": "<<<member_address2>>>",
            "member_id": "<<<instance2_uuid>>>",
            "name": null<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
        },
        {
            "host": "<<<member_address3>>>",
            "member_id": "<<<instance3_uuid>>>",
            "name": null<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
        }
    ],
    "unavailableInstances": []
}

A new instance '<<<member_address2>>>' was discovered in the cluster.
Adding instance to the cluster metadata...
The instance '<<<member_address2>>>' was successfully added to the cluster metadata.

A new instance '<<<member_address3>>>' was discovered in the cluster.
Would you like to add it to the cluster metadata? [Y/n]: Adding instance to the cluster metadata...
The instance '<<<member_address3>>>' was successfully added to the cluster metadata.

//@<OUT> WL10644 - TSF2_2: Validate that the instances were added.
{
    "clusterName": "c",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            },
            "<<<member_address2>>>": {
                "address": "<<<member_address2>>>",
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            },
            "<<<member_address3>>>": {
                "address": "<<<member_address3>>>",
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@<OUT> WL10644 - TSF2_3: Remove instances on port 2 and 3 from MD again.
{
    "clusterName": "c",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>", 
                "instanceErrors": [
                    "WARNING: Instance is not managed by InnoDB cluster. Use cluster.rescan() to repair."
                ], 
                "mode": "R/O", 
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA", 
                "status": "ONLINE", 
                "version": "[[*]]"
            }, 
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>", 
                "instanceErrors": [
                    "WARNING: Instance is not managed by InnoDB cluster. Use cluster.rescan() to repair."
                ], 
                "mode": "R/O", 
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA", 
                "status": "ONLINE", 
                "version": "[[*]]"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@<OUT> WL10644 - TSF2_3: Rescan with addInstances:[incomplete_valid_list] and interactive:false.
Rescanning the cluster...

Result of the rescanning operation for the 'c' cluster:
{
    "name": "c",
    "newTopologyMode": null,
    "newlyDiscoveredInstances": [
        {
            "host": "<<<member_address2>>>",
            "member_id": "<<<instance2_uuid>>>",
            "name": null<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
        },
        {
            "host": "<<<member_address3>>>",
            "member_id": "<<<instance3_uuid>>>",
            "name": null<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
        }
    ],
    "unavailableInstances": []
}

A new instance '<<<member_address2>>>' was discovered in the cluster.
Adding instance to the cluster metadata...
The instance '<<<member_address2>>>' was successfully added to the cluster metadata.

A new instance '<<<member_address3>>>' was discovered in the cluster.

//@<OUT> WL10644 - TSF2_3: Validate that the instances were added.
{
    "clusterName": "c",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            },
            "<<<member_address2>>>": {
                "address": "<<<member_address2>>>",
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            },
            "<<<member_address3>>>": {
                "address": "<<<member_address3>>>", 
                "instanceErrors": [
                    "WARNING: Instance is not managed by InnoDB cluster. Use cluster.rescan() to repair."
                ], 
                "mode": "R/O", 
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA", 
                "status": "ONLINE", 
                "version": "[[*]]"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@<OUT> WL10644 - TSF2_4: Remove instances on port 2 from MD.
{
    "clusterName": "c",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>", 
                "instanceErrors": [
                    "WARNING: Instance is not managed by InnoDB cluster. Use cluster.rescan() to repair."
                ], 
                "mode": "R/O", 
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA", 
                "status": "ONLINE", 
                "version": "[[*]]"
            }, 
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>", 
                "instanceErrors": [
                    "WARNING: Instance is not managed by InnoDB cluster. Use cluster.rescan() to repair."
                ], 
                "mode": "R/O", 
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA", 
                "status": "ONLINE", 
                "version": "[[*]]"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@<OUT> WL10644 - TSF2_4: Rescan with addInstances:"auto" and interactive:true.
Rescanning the cluster...

Result of the rescanning operation for the 'c' cluster:
{
    "name": "c",
    "newTopologyMode": null,
    "newlyDiscoveredInstances": [
        {
            "host": "<<<member_address2>>>",
            "member_id": "<<<instance2_uuid>>>",
            "name": null<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
        },
        {
            "host": "<<<member_address3>>>",
            "member_id": "<<<instance3_uuid>>>",
            "name": null<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
        }
    ],
    "unavailableInstances": []
}

A new instance '<<<member_address2>>>' was discovered in the cluster.
Adding instance to the cluster metadata...
The instance '<<<member_address2>>>' was successfully added to the cluster metadata.

A new instance '<<<member_address3>>>' was discovered in the cluster.
Adding instance to the cluster metadata...
The instance '<<<member_address3>>>' was successfully added to the cluster metadata.

//@<OUT> WL10644 - TSF2_4: Validate that the instances were added.
{
    "clusterName": "c",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            },
            "<<<member_address2>>>": {
                "address": "<<<member_address2>>>",
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            },
            "<<<member_address3>>>": {
                "address": "<<<member_address3>>>",
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@<OUT> WL10644 - TSF2_5: Remove instances on port 2 and 3 from MD again.
{
    "clusterName": "c",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>", 
                "instanceErrors": [
                    "WARNING: Instance is not managed by InnoDB cluster. Use cluster.rescan() to repair."
                ], 
                "mode": "R/O", 
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA", 
                "status": "ONLINE", 
                "version": "[[*]]"
            }, 
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>", 
                "instanceErrors": [
                    "WARNING: Instance is not managed by InnoDB cluster. Use cluster.rescan() to repair."
                ], 
                "mode": "R/O", 
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA", 
                "status": "ONLINE", 
                "version": "[[*]]"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@<OUT> WL10644 - TSF2_5: Rescan with addInstances:"auto" and interactive:false.
Rescanning the cluster...

Result of the rescanning operation for the 'c' cluster:
{
    "name": "c",
    "newTopologyMode": null,
    "newlyDiscoveredInstances": [
        {
            "host": "<<<member_address2>>>",
            "member_id": "<<<instance2_uuid>>>",
            "name": null<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
        },
        {
            "host": "<<<member_address3>>>",
            "member_id": "<<<instance3_uuid>>>",
            "name": null<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
        }
    ],
    "unavailableInstances": []
}

A new instance '<<<member_address2>>>' was discovered in the cluster.
Adding instance to the cluster metadata...
The instance '<<<member_address2>>>' was successfully added to the cluster metadata.

A new instance '<<<member_address3>>>' was discovered in the cluster.
Adding instance to the cluster metadata...
The instance '<<<member_address3>>>' was successfully added to the cluster metadata.

//@<OUT> WL10644 - TSF2_5: Validate that the instances were added.
{
    "clusterName": "c",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            },
            "<<<member_address2>>>": {
                "address": "<<<member_address2>>>",
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            },
            "<<<member_address3>>>": {
                "address": "<<<member_address3>>>",
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@ WL10644 - TSF3_1: Disable GR in persisted settings {VER(>=8.0.11)}.
||

//@<OUT> WL10644 - TSF3_1: Stop instances on port 2 and 3 (no metadata MD changes).
{
    "clusterName": "c",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures. 2 members are not active.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            },
            "<<<member_address2>>>": {
                "address": "<<<member_address2>>>",
                "mode": "n/a",
                "readReplicas": {},
                "role": "HA",
                "shellConnectError": "[[*]]",
                "status": "(MISSING)"
            },
            "<<<member_address3>>>": {
                "address": "<<<member_address3>>>",
                "mode": "n/a",
                "readReplicas": {},
                "role": "HA",
                "shellConnectError": "[[*]]",
                "status": "(MISSING)"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@<OUT> WL10644 - TSF3_1: Number of instances in the MD before rescan().
3

//@<OUT> WL10644 - TSF3_1: Rescan with removeInstances:[complete_valid_list].
Rescanning the cluster...

Result of the rescanning operation for the 'c' cluster:
{
    "name": "c",
    "newTopologyMode": null,
    "newlyDiscoveredInstances": [],
    "unavailableInstances": [
        {
            "host": "<<<member_address2>>>",
            "label": "<<<member_address2>>>",
            "member_id": "<<<instance2_uuid>>>"
        },
        {
            "host": "<<<member_address3>>>",
            "label": "<<<member_address3>>>",
            "member_id": "<<<instance3_uuid>>>"
        }
    ]
}

The instance '<<<member_address2>>>' is no longer part of the cluster.
Removing instance from the cluster metadata...
The instance '<<<member_address2>>>' was successfully removed from the cluster metadata.

The instance '<<<member_address3>>>' is no longer part of the cluster.
Removing instance from the cluster metadata...
The instance '<<<member_address3>>>' was successfully removed from the cluster metadata.

//@<OUT> WL10644 - TSF3_1: Number of instances in the MD after rescan().
1

//@<OUT> WL10644 - TSF3_1: Validate that the instances were removed.
{
    "clusterName": "c",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@ WL10644 - TSF3_2: Start instances and add them back to the cluster.
||

//@ WL10644 - TSF3_2: Disable GR in persisted settings {VER(>=8.0.11)}.
||

//@<OUT> WL10644 - TSF3_2: Stop instances on port 2 and 3 (no metadata MD changes).
{
    "clusterName": "c",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures. 2 members are not active.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "n/a",
                "readReplicas": {},
                "role": "HA",
                "shellConnectError": "[[*]]",
                "status": "(MISSING)"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "n/a",
                "readReplicas": {},
                "role": "HA",
                "shellConnectError": "[[*]]",
                "status": "(MISSING)"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@<OUT> WL10644 - TSF3_2: Number of instances in the MD before rescan().
3

//@<OUT> WL10644 - TSF3_2: Rescan with removeInstances:[incomplete_valid_list] and interactive:true.
Rescanning the cluster...

Result of the rescanning operation for the 'c' cluster:
{
    "name": "c",
    "newTopologyMode": null,
    "newlyDiscoveredInstances": [],
    "unavailableInstances": [
        {
            "host": "<<<member_fqdn_address2>>>",
            "label": "<<<member_fqdn_address2>>>",
            "member_id": "<<<instance2_uuid>>>"
        },
        {
            "host": "<<<member_fqdn_address3>>>",
            "label": "<<<member_fqdn_address3>>>",
            "member_id": "<<<instance3_uuid>>>"
        }
    ]
}

The instance '<<<member_fqdn_address2>>>' is no longer part of the cluster.
Removing instance from the cluster metadata...
The instance '<<<member_fqdn_address2>>>' was successfully removed from the cluster metadata.

The instance '<<<member_fqdn_address3>>>' is no longer part of the cluster.
The instance is either offline or left the HA group. You can try to add it to the cluster again with the cluster.rejoinInstance('<<<member_fqdn_address3>>>') command or you can remove it from the cluster configuration.
Would you like to remove it from the cluster metadata? [Y/n]: Removing instance from the cluster metadata...
The instance '<<<member_fqdn_address3>>>' was successfully removed from the cluster metadata.

//@<OUT> WL10644 - TSF3_2: Number of instances in the MD after rescan().
1

//@<OUT> WL10644 - TSF3_2: Validate that the instances were removed.
{
    "clusterName": "c",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@ WL10644 - TSF3_3: Start instances and add them back to the cluster.
||

//@ WL10644 - TSF3_3: Disable GR in persisted settings {VER(>=8.0.11)}.
||

//@<OUT> WL10644 - TSF3_3: Stop instances on port 2 and 3 (no metadata MD changes).
{
    "clusterName": "c",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures. 2 members are not active.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "n/a",
                "readReplicas": {},
                "role": "HA",
                "shellConnectError": "[[*]]",
                "status": "(MISSING)"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "n/a",
                "readReplicas": {},
                "role": "HA",
                "shellConnectError": "[[*]]",
                "status": "(MISSING)"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@<OUT> WL10644 - TSF3_3: Number of instances in the MD before rescan().
3

//@<OUT> WL10644 - TSF3_3: Rescan with removeInstances:[incomplete_valid_list] and interactive:false.
Rescanning the cluster...

Result of the rescanning operation for the 'c' cluster:
{
    "name": "c",
    "newTopologyMode": null,
    "newlyDiscoveredInstances": [],
    "unavailableInstances": [
        {
            "host": "<<<member_fqdn_address2>>>",
            "label": "<<<member_fqdn_address2>>>",
            "member_id": "<<<instance2_uuid>>>"
        },
        {
            "host": "<<<member_fqdn_address3>>>",
            "label": "<<<member_fqdn_address3>>>",
            "member_id": "<<<instance3_uuid>>>"
        }
    ]
}

The instance '<<<member_fqdn_address2>>>' is no longer part of the cluster.
Removing instance from the cluster metadata...
The instance '<<<member_fqdn_address2>>>' was successfully removed from the cluster metadata.

The instance '<<<member_fqdn_address3>>>' is no longer part of the cluster.


//@<OUT> WL10644 - TSF3_3: Number of instances in the MD after rescan().
2

//@<OUT> WL10644 - TSF3_3: Validate that the instances were removed.
{
    "clusterName": "c",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures. 1 member is not active.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "n/a",
                "readReplicas": {},
                "role": "HA",
                "shellConnectError": "[[*]]",
                "status": "(MISSING)"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@ WL10644 - TSF3_4: Start instance on port 2 and add it back to the cluster.
||

//@ WL10644 - TSF3_4: Disable GR in persisted settings {VER(>=8.0.11)}.
||

//@<OUT> WL10644 - TSF3_4: Stop instances on port 2 (no metadata MD changes).
{
    "clusterName": "c",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures. 2 members are not active.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "n/a",
                "readReplicas": {},
                "role": "HA",
                "shellConnectError": "[[*]]",
                "status": "(MISSING)"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "n/a",
                "readReplicas": {},
                "role": "HA",
                "shellConnectError": "[[*]]",
                "status": "(MISSING)"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@<OUT> WL10644 - TSF3_4: Number of instances in the MD before rescan().
3

//@<OUT> WL10644 - TSF3_4: Rescan with removeInstances:"auto" and interactive:true.
Rescanning the cluster...

Result of the rescanning operation for the 'c' cluster:
{
    "name": "c",
    "newTopologyMode": null,
    "newlyDiscoveredInstances": [],
    "unavailableInstances": [
        {
            "host": "<<<member_fqdn_address2>>>",
            "label": "<<<member_fqdn_address2>>>",
            "member_id": "<<<instance2_uuid>>>"
        },
        {
            "host": "<<<member_fqdn_address3>>>",
            "label": "<<<member_fqdn_address3>>>",
            "member_id": "<<<instance3_uuid>>>"
        }
    ]
}

The instance '<<<member_fqdn_address2>>>' is no longer part of the cluster.
Removing instance from the cluster metadata...
The instance '<<<member_fqdn_address2>>>' was successfully removed from the cluster metadata.

The instance '<<<member_fqdn_address3>>>' is no longer part of the cluster.
Removing instance from the cluster metadata...
The instance '<<<member_fqdn_address3>>>' was successfully removed from the cluster metadata.

//@<OUT> WL10644 - TSF3_4: Number of instances in the MD after rescan().
1

//@<OUT> WL10644 - TSF3_4: Validate that the instances were removed.
{
    "clusterName": "c",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@ WL10644 - TSF3_5: Start instances and add them back to the cluster.
||

//@ WL10644 - TSF3_5: Disable GR in persisted settings {VER(>=8.0.11)}.
||

//@<OUT> WL10644 - TSF3_5: Stop instances on port 2 and 3 (no metadata MD changes).
{
    "clusterName": "c",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures. 2 members are not active.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "n/a",
                "readReplicas": {},
                "role": "HA",
                "shellConnectError": "[[*]]",
                "status": "(MISSING)"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "n/a",
                "readReplicas": {},
                "role": "HA",
                "shellConnectError": "[[*]]",
                "status": "(MISSING)"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@<OUT> WL10644 - TSF3_5: Number of instances in the MD before rescan().
3

//@<OUT> WL10644 - TSF3_5: Rescan with removeInstances:"auto" and interactive:false.
Rescanning the cluster...

Result of the rescanning operation for the 'c' cluster:
{
    "name": "c",
    "newTopologyMode": null,
    "newlyDiscoveredInstances": [],
    "unavailableInstances": [
        {
            "host": "<<<member_fqdn_address2>>>",
            "label": "<<<member_fqdn_address2>>>",
            "member_id": "<<<instance2_uuid>>>"
        },
        {
            "host": "<<<member_fqdn_address3>>>",
            "label": "<<<member_fqdn_address3>>>",
            "member_id": "<<<instance3_uuid>>>"
        }
    ]
}

The instance '<<<member_fqdn_address2>>>' is no longer part of the cluster.
Removing instance from the cluster metadata...
The instance '<<<member_fqdn_address2>>>' was successfully removed from the cluster metadata.

The instance '<<<member_fqdn_address3>>>' is no longer part of the cluster.
Removing instance from the cluster metadata...
The instance '<<<member_fqdn_address3>>>' was successfully removed from the cluster metadata.

//@<OUT> WL10644 - TSF3_5: Number of instances in the MD after rescan().
1

//@<OUT> WL10644 - TSF3_5: Validate that the instances were removed.
{
    "clusterName": "c",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@ WL10644: Start instances and add them back to the cluster again.
||

//@ WL10644 - TSF4_1: Change the topology mode in the MD to the wrong value.
||

//@<OUT> WL10644 - TSF4_1: Topology mode in MD before rescan().
mm

//@ WL10644 - TSF4_5: Set auto_increment settings to unused values.
||

//@<> WL10644 - TSF4_2: status() error because topology mode changed.
||Cluster.status: The InnoDB Cluster topology type (Multi-Primary) does not match the current Group Replication configuration (Single-Primary). Please use <cluster>.rescan() or change the Group Replication configuration accordingly. (RuntimeError)

//@<> BUG#29330769: Verify deprecation message added about updateTopologyMode
|The updateTopologyMode option is deprecated. The topology-mode is now automatically updated.|

//@<OUT> WL10644 - TSF4_2: status() succeeds after rescan() updates topology mode.
{
    "clusterName": "c",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@<OUT> WL10644 - TSF4_5: Check auto_increment settings after change to single-primary.
auto_increment_increment: 1
auto_increment_offset: 2
auto_increment_increment: 1
auto_increment_offset: 2
auto_increment_increment: 1
auto_increment_offset: 2

//@<OUT> Create multi-primary cluster.
{
    "clusterName": "c",
    "defaultReplicaSet": {
        "name": "default",
        "ssl": "REQUIRED",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            }
        },
        "topologyMode": "Multi-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@ WL10644 - TSF4_3: Change the topology mode in the MD to the wrong value.
||

//@<OUT> WL10644 - TSF4_3: Topology mode in MD before rescan().
pm

//@ WL10644 - TSF4_6: Set auto_increment settings to unused values.
||

//@<OUT> WL10644 - TSF4_3: Rescan with interactive:true and change needed.
Rescanning the cluster...

Result of the rescanning operation for the 'c' cluster:
{
    "name": "c",
    "newTopologyMode": "Multi-Primary",
    "newlyDiscoveredInstances": [],
    "unavailableInstances": []
}

NOTE: The topology mode of the cluster changed to 'Multi-Primary'.
Updating topology mode in the cluster metadata...
<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port1+"' cannot persist configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.\n":""\>>>
<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port2+"' cannot persist configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.\n":""\>>>
<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port3+"' cannot persist configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.\n":""\>>>
Topology mode was successfully updated to 'Multi-Primary' in the cluster metadata.

//@<OUT> WL10644 - TSF4_3: Check topology mode in MD after rescan().
mm

//@<OUT> WL10644 - TSF4_6: Check auto_increment settings after change to multi-primary.
auto_increment_increment: 7
auto_increment_offset: <<<offset1>>>
auto_increment_increment: 7
auto_increment_offset: <<<offset2>>>
auto_increment_increment: 7
auto_increment_offset: <<<offset3>>>

//@ WL10644 - TSF4_4: Change the topology mode in the MD to the wrong value.
||

//@<OUT> WL10644 - TSF4_4: Topology mode in MD before rescan().
pm

//@<OUT> WL10644 - TSF4_4: Rescan with interactive:false and change needed.
Rescanning the cluster...

Result of the rescanning operation for the 'c' cluster:
{
    "name": "c",
    "newTopologyMode": "Multi-Primary",
    "newlyDiscoveredInstances": [],
    "unavailableInstances": []
}

NOTE: The topology mode of the cluster changed to 'Multi-Primary'.

//@<OUT> WL10644 - TSF4_4: Check topology mode in MD after rescan().
mm

//@ Finalize.
||
