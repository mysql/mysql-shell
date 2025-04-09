//@<OUT> No-op - Still missing server_id attributes are added
Rescanning the cluster...

Result of the rescanning operation for the 'c' cluster:
{
    "metadataConsistent": true,
    "name": "c",
    "newTopologyMode": null,
    "newlyDiscoveredInstances": [],
    "unavailableInstances": [],
    "updatedInstances": []
}

//@<OUT> Rescan with removeObsolete: true.
Rescanning the cluster...

Result of the rescanning operation for the 'c' cluster:
{
    "metadataConsistent": true,
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

//@<OUT> Rescan with removeObsolete: true and interactive:true.
Rescanning the cluster...

Result of the rescanning operation for the 'c' cluster:
{
    "metadataConsistent": true,
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

//@<OUT> Rescan with removeObsolete: true and interactive:false.
Rescanning the cluster...

Result of the rescanning operation for the 'c' cluster:
{
    "metadataConsistent": true,
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

//@ WL10644 - TSF4_2: status() error because topology mode changed.
||The InnoDB Cluster topology type (Multi-Primary) does not match the current Group Replication configuration (Single-Primary). Please use <cluster>.rescan() or change the Group Replication configuration accordingly. (RuntimeError)

//@<OUT> WL10644 - TSF4_5: Check auto_increment settings after change to single-primary.
auto_increment_increment: 1
auto_increment_offset: 2
auto_increment_increment: 1
auto_increment_offset: 2
auto_increment_increment: 1
auto_increment_offset: 2

//@<OUT> WL10644 - TSF4_3: Rescan with interactive:true and change needed.
Rescanning the cluster...

Result of the rescanning operation for the 'c' cluster:
{
    "metadataConsistent": true,
    "name": "c",
    "newTopologyMode": "Multi-Primary",
    "newlyDiscoveredInstances": [],
    "unavailableInstances": [],
    "updatedInstances": []
}

NOTE: The topology mode of the cluster changed to 'Multi-Primary'.
Updating topology mode in the cluster metadata...
<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port1+"' cannot persist configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureInstance() command locally, using the 'mycnfPath' option, to persist the changes.\n":""\>>>
<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port2+"' cannot persist configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureInstance() command locally, using the 'mycnfPath' option, to persist the changes.\n":""\>>>
<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port3+"' cannot persist configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureInstance() command locally, using the 'mycnfPath' option, to persist the changes.\n":""\>>>
Topology mode was successfully updated to 'Multi-Primary' in the cluster metadata.

//@<OUT> WL10644 - TSF4_6: Check auto_increment settings after change to multi-primary.
auto_increment_increment: 7
auto_increment_offset: <<<offset1>>>
auto_increment_increment: 7
auto_increment_offset: <<<offset2>>>
auto_increment_increment: 7
auto_increment_offset: <<<offset3>>>

//@<OUT> WL10644 - TSF4_4: Rescan with interactive:false and change needed.
Rescanning the cluster...

Result of the rescanning operation for the 'c' cluster:
{
    "metadataConsistent": true,
    "name": "c",
    "newTopologyMode": "Multi-Primary",
    "newlyDiscoveredInstances": [],
    "unavailableInstances": [],
    "updatedInstances": []
}

NOTE: The topology mode of the cluster changed to 'Multi-Primary'.
