//@ Initialize.
||

//@ Get needed Server_Ids and UUIDs.
||

//@ Configure sandboxes.
||

//@<OUT> No-op - Still missing server_id attributes are added
Rescanning the cluster...

Result of the rescanning operation for the 'c' cluster:
{
    "name": "c",
    "newTopologyMode": null,
    "newlyDiscoveredInstances": [],
    "unavailableInstances": [],
    "updatedInstances": []
}

//@ WL10644 - TSF2_6: empty addInstances throw ArgumentError.
||The list for 'addInstances' option cannot be empty. (ArgumentError)

//@ WL10644 - TSF2_8: invalid addInstances list throw ArgumentError.
||Invalid value 'undefined' for 'addInstances' option: Invalid connection options, expected either a URI or a Connection Options Dictionary (ArgumentError)
||Invalid value 'localhost' for 'addInstances' option: port is missing. (ArgumentError)
||Invalid value ':3301' for 'addInstances' option: host cannot be empty. (ArgumentError)
||Invalid value '@' for 'addInstances' option: Invalid URI: Missing user information (ArgumentError)
||Invalid value '{}' for 'addInstances' option: Invalid connection options, no options provided. (ArgumentError)
||Invalid value '{"host": "myhost"}' for 'addInstances' option: port is missing. (ArgumentError)
||Invalid value '{"host": ""}' for 'addInstances' option: Host value cannot be an empty string. (ArgumentError)

//@ WL10644: Duplicated values for addInstances.
||Duplicated value found for instance 'localhost:3301' in 'addInstances' option. (ArgumentError)
||Duplicated value found for instance 'localhost:3301' in 'addInstances' option. (ArgumentError)
||Duplicated value found for instance 'localhost:3301' in 'addInstances' option. (ArgumentError)

//@ WL10644 - TSF2_9: invalid value with addInstances throw ArgumentError.
||Option 'addInstances' only accepts 'auto' as a valid string value, otherwise a list of instances is expected. (ArgumentError)

//@ WL10644 - TSF2_7: "auto" is case insensitive, no error.
||

//@ WL10644: Invalid type used for addInstances.
||The 'addInstances' option must be a string or a list of strings. (ArgumentError)
||The 'addInstances' option must be a string or a list of strings. (ArgumentError)
||The 'addInstances' option must be a string or a list of strings. (ArgumentError)

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
    "unavailableInstances": [],
    "updatedInstances": []
}

WARNING: The following instances were not added to the metadata because they are already part of the cluster: '<<<member_address>>>'. Please verify if the specified value for 'addInstances' option is correct.

//@ WL10644 - TSF3_6: empty removeInstances throw ArgumentError.
||The list for 'removeInstances' option cannot be empty. (ArgumentError)

//@ WL10644 - TSF3_8: invalid removeInstances list throw ArgumentError.
||Invalid value 'undefined' for 'removeInstances' option: Invalid connection options, expected either a URI or a Connection Options Dictionary (ArgumentError)
||Invalid value 'localhost' for 'removeInstances' option: port is missing. (ArgumentError)
||Invalid value ':3301' for 'removeInstances' option: host cannot be empty. (ArgumentError)
||Invalid value '@' for 'removeInstances' option: Invalid URI: Missing user information (ArgumentError)
||Invalid value '{}' for 'removeInstances' option: Invalid connection options, no options provided. (ArgumentError)
||Invalid value '{"host": "myhost"}' for 'removeInstances' option: port is missing. (ArgumentError)
||Invalid value '{"host": ""}' for 'removeInstances' option: Host value cannot be an empty string. (ArgumentError)

//@ WL10644: Duplicated values for removeInstances.
||Duplicated value found for instance 'localhost:3301' in 'removeInstances' option. (ArgumentError)
||Duplicated value found for instance 'localhost:3301' in 'removeInstances' option. (ArgumentError)
||Duplicated value found for instance 'localhost:3301' in 'removeInstances' option. (ArgumentError)

//@ WL10644 - TSF3_9: invalid value with removeInstances throw ArgumentError.
||Option 'removeInstances' only accepts 'auto' as a valid string value, otherwise a list of instances is expected. (ArgumentError)

//@ WL10644 - TSF3_7: "auto" is case insensitive, no error.
||

//@ WL10644: Invalid type used for removeInstances.
||The 'removeInstances' option must be a string or a list of strings. (ArgumentError)
||The 'removeInstances' option must be a string or a list of strings. (ArgumentError)
||The 'removeInstances' option must be a string or a list of strings. (ArgumentError)

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
    "unavailableInstances": [],
    "updatedInstances": []
}

WARNING: The following instances were not removed from the metadata because they are already not part of the cluster or are running auto-rejoin: 'localhost:1111'. Please verify if the specified value for 'removeInstances' option is correct.

//@ WL10644: Duplicated values between addInstances and removeInstances.
||The same instances cannot be used in both 'addInstances' and 'removeInstances' options: 'localhost:3300, localhost:3301'. (ArgumentError)
||The same instance cannot be used in both 'addInstances' and 'removeInstances' options: 'localhost:3301'. (ArgumentError)
||The same instance cannot be used in both 'addInstances' and 'removeInstances' options: 'localhost:3301'. (ArgumentError)

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
    "unavailableInstances": [],
    "updatedInstances": []
}

A new instance '<<<member_address2>>>' was discovered in the cluster.
Adding instance to the cluster metadata...
The instance '<<<member_address2>>>' was successfully added to the cluster metadata.

A new instance '<<<member_address3>>>' was discovered in the cluster.
Adding instance to the cluster metadata...
The instance '<<<member_address3>>>' was successfully added to the cluster metadata.

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
    "unavailableInstances": [],
    "updatedInstances": []
}

A new instance '<<<member_address2>>>' was discovered in the cluster.
Adding instance to the cluster metadata...
The instance '<<<member_address2>>>' was successfully added to the cluster metadata.

A new instance '<<<member_address3>>>' was discovered in the cluster.
Would you like to add it to the cluster metadata? [Y/n]: Adding instance to the cluster metadata...
The instance '<<<member_address3>>>' was successfully added to the cluster metadata.

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
    "unavailableInstances": [],
    "updatedInstances": []
}

A new instance '<<<member_address2>>>' was discovered in the cluster.
Adding instance to the cluster metadata...
The instance '<<<member_address2>>>' was successfully added to the cluster metadata.

A new instance '<<<member_address3>>>' was discovered in the cluster.



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
    "unavailableInstances": [],
    "updatedInstances": []
}

A new instance '<<<member_address2>>>' was discovered in the cluster.
Adding instance to the cluster metadata...
The instance '<<<member_address2>>>' was successfully added to the cluster metadata.

A new instance '<<<member_address3>>>' was discovered in the cluster.
Adding instance to the cluster metadata...
The instance '<<<member_address3>>>' was successfully added to the cluster metadata.




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
    "unavailableInstances": [],
    "updatedInstances": []
}

A new instance '<<<member_address2>>>' was discovered in the cluster.
Adding instance to the cluster metadata...
The instance '<<<member_address2>>>' was successfully added to the cluster metadata.

A new instance '<<<member_address3>>>' was discovered in the cluster.
Adding instance to the cluster metadata...
The instance '<<<member_address3>>>' was successfully added to the cluster metadata.


//@ WL10644 - TSF3_1: Disable GR in persisted settings {VER(>=8.0.11)}.
||


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
    ],
    "updatedInstances": []
}

The instance '<<<member_address2>>>' is no longer part of the cluster.
Removing instance from the cluster metadata...
The instance '<<<member_address2>>>' was successfully removed from the cluster metadata.

The instance '<<<member_address3>>>' is no longer part of the cluster.
Removing instance from the cluster metadata...
The instance '<<<member_address3>>>' was successfully removed from the cluster metadata.

//@<OUT> WL10644 - TSF3_1: Number of instances in the MD after rescan().
1


//@ WL10644 - TSF3_2: Start instances and add them back to the cluster.
||

//@ WL10644 - TSF3_2: Disable GR in persisted settings {VER(>=8.0.11)}.
||


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
    ],
    "updatedInstances": []
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


//@ WL10644 - TSF3_3: Start instances and add them back to the cluster.
||

//@ WL10644 - TSF3_3: Disable GR in persisted settings {VER(>=8.0.11)}.
||


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
    ],
    "updatedInstances": []
}

The instance '<<<member_fqdn_address2>>>' is no longer part of the cluster.
Removing instance from the cluster metadata...
The instance '<<<member_fqdn_address2>>>' was successfully removed from the cluster metadata.

The instance '<<<member_fqdn_address3>>>' is no longer part of the cluster.


//@<OUT> WL10644 - TSF3_3: Number of instances in the MD after rescan().
2


//@ WL10644 - TSF3_4: Start instance on port 2 and add it back to the cluster.
||

//@ WL10644 - TSF3_4: Disable GR in persisted settings {VER(>=8.0.11)}.
||


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
    ],
    "updatedInstances": []
}

The instance '<<<member_fqdn_address2>>>' is no longer part of the cluster.
Removing instance from the cluster metadata...
The instance '<<<member_fqdn_address2>>>' was successfully removed from the cluster metadata.

The instance '<<<member_fqdn_address3>>>' is no longer part of the cluster.
Removing instance from the cluster metadata...
The instance '<<<member_fqdn_address3>>>' was successfully removed from the cluster metadata.

//@<OUT> WL10644 - TSF3_4: Number of instances in the MD after rescan().
1


//@ WL10644 - TSF3_5: Start instances and add them back to the cluster.
||

//@ WL10644 - TSF3_5: Disable GR in persisted settings {VER(>=8.0.11)}.
||


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
    ],
    "updatedInstances": []
}

The instance '<<<member_fqdn_address2>>>' is no longer part of the cluster.
Removing instance from the cluster metadata...
The instance '<<<member_fqdn_address2>>>' was successfully removed from the cluster metadata.

The instance '<<<member_fqdn_address3>>>' is no longer part of the cluster.
Removing instance from the cluster metadata...
The instance '<<<member_fqdn_address3>>>' was successfully removed from the cluster metadata.

//@<OUT> WL10644 - TSF3_5: Number of instances in the MD after rescan().
1


//@ WL10644: Start instances and add them back to the cluster again.
||

//@ WL10644 - TSF4_1: Change the topology mode in the MD to the wrong value.
||

//@<OUT> WL10644 - TSF4_1: Topology mode in MD before rescan().
mm

//@ WL10644 - TSF4_5: Set auto_increment settings to unused values.
||

//@ WL10644 - TSF4_2: status() error because topology mode changed.
||The InnoDB Cluster topology type (Multi-Primary) does not match the current Group Replication configuration (Single-Primary). Please use <cluster>.rescan() or change the Group Replication configuration accordingly. (RuntimeError)

//@<> BUG#29330769: Verify deprecation message added about updateTopologyMode
|The updateTopologyMode option is deprecated. The topology-mode is now automatically updated.|


//@<OUT> WL10644 - TSF4_5: Check auto_increment settings after change to single-primary.
auto_increment_increment: 1
auto_increment_offset: 2
auto_increment_increment: 1
auto_increment_offset: 2
auto_increment_increment: 1
auto_increment_offset: 2


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
    "unavailableInstances": [],
    "updatedInstances": []
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
    "unavailableInstances": [],
    "updatedInstances": []
}

NOTE: The topology mode of the cluster changed to 'Multi-Primary'.

//@<OUT> WL10644 - TSF4_4: Check topology mode in MD after rescan().
mm

//@ Finalize.
||
