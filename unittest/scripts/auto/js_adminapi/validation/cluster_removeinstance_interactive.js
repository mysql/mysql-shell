//@ Initialization
||

//@ Connect
||

//@ create cluster
||

//@ remove instance not in MD but reachable when there's just 1 (should fail)
||Metadata for instance <<<hostname>>>:<<<__mysql_sandbox_port3>>> not found (MYSQLSH 51104)
||Metadata for instance <<<hostname>>>:<<<__mysql_sandbox_port3>>> not found (MYSQLSH 51104)

//@ Adding instance
||

//@ remove instance not in MD but reachable when there are 2 (should fail)
||Metadata for instance <<<hostname>>>:<<<__mysql_sandbox_port3>>> not found (MYSQLSH 51104)
||Metadata for instance <<<hostname>>>:<<<__mysql_sandbox_port3>>> not found (MYSQLSH 51104)

//@ Configure instance on port1 to persist auto-rejoin settings {VER(<8.0.11)}
||

//@ Configure instance on port2 to persist auto-rejoin settings {VER(<8.0.11)}
||

//@<OUT> Number of instance according to GR.
2

//@ Failure: remove_instance - invalid uri
||Invalid URI: Missing user information

//@<OUT> Cluster status
{
    "clusterName": "dev",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "memberRole": "PRIMARY",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "[[*]]"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "memberRole": "SECONDARY",
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

//@ Remove instance failure due to wrong credentials
// NOTE: Do not use @<ERR> because it matches the whole line but result after
// 'foo' is not deterministic: @'hostanme' or @'hostname_ip' can appear.
||Could not open connection to '[[*]]:<<<__mysql_sandbox_port2>>>': Access denied for user 'foo'@'[[*]]' (using password: YES) (MySQL Error 1045)

//@<OUT> Cluster status after remove failed
{
    "clusterName": "dev",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "memberRole": "PRIMARY",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "[[*]]"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "memberRole": "SECONDARY",
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

//@<OUT> Removing instance
The instance will be removed from the InnoDB cluster. Depending on the instance
being the Seed or not, the Metadata session might become invalid. If so, please
start a new session to the Metadata Storage R/W instance.

* Waiting for instance to synchronize with the primary...
Instance 'localhost:<<<__mysql_sandbox_port2>>>' is attempting to leave the cluster...
<<<(__version_num<80011)?"WARNING: On instance '"+localhost+":"+__mysql_sandbox_port2+"' configuration cannot be persisted since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please set the 'group_replication_start_on_boot' variable to 'OFF' in the server configuration file, otherwise it might rejoin the cluster upon restart.\n":""\>>>
<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port1+"' cannot persist configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.\n":""\>>>

The instance 'localhost:<<<__mysql_sandbox_port2>>>' was successfully removed from the cluster.


//@<OUT> Cluster status after removal
{
    "clusterName": "dev",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "memberRole": "PRIMARY",
                "mode": "R/W",
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

//@ Adding instance on port2 back (interactive use)
||

//@ Adding instance on port3 (interactive use)
||

//@<OUT> Removing instance (interactive: true)
The instance will be removed from the InnoDB cluster. Depending on the instance
being the Seed or not, the Metadata session might become invalid. If so, please
start a new session to the Metadata Storage R/W instance.

* Waiting for instance to synchronize with the primary...
Instance 'localhost:<<<__mysql_sandbox_port2>>>' is attempting to leave the cluster...
<<<(__version_num<80011)?"WARNING: On instance '"+localhost+":"+__mysql_sandbox_port2+"' configuration cannot be persisted since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please set the 'group_replication_start_on_boot' variable to 'OFF' in the server configuration file, otherwise it might rejoin the cluster upon restart.\n":""\>>>
<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port1+"' cannot persist configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.\n":""\>>>
<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port3+"' cannot persist configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.\n":""\>>>

The instance 'localhost:<<<__mysql_sandbox_port2>>>' was successfully removed from the cluster.

//@<OUT> Removing instance (interactive: false)
The instance will be removed from the InnoDB cluster. Depending on the instance
being the Seed or not, the Metadata session might become invalid. If so, please
start a new session to the Metadata Storage R/W instance.

* Waiting for instance to synchronize with the primary...
Instance 'localhost:<<<__mysql_sandbox_port3>>>' is attempting to leave the cluster...
<<<(__version_num<80011)?"WARNING: On instance '"+localhost+":"+__mysql_sandbox_port3+"' configuration cannot be persisted since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please set the 'group_replication_start_on_boot' variable to 'OFF' in the server configuration file, otherwise it might rejoin the cluster upon restart.\n":""\>>>
<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port1+"' cannot persist configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.\n":""\>>>

The instance 'localhost:<<<__mysql_sandbox_port3>>>' was successfully removed from the cluster.

//@ Adding instance on port2 back (interactive: true, force: false)
||

//@ Adding instance on port3 back (interactive: true, force: true)
||

//@<OUT> Removing instance (interactive: true, force: false)
The instance will be removed from the InnoDB cluster. Depending on the instance
being the Seed or not, the Metadata session might become invalid. If so, please
start a new session to the Metadata Storage R/W instance.

* Waiting for instance to synchronize with the primary...
Instance 'localhost:<<<__mysql_sandbox_port2>>>' is attempting to leave the cluster...
<<<(__version_num<80011)?"WARNING: On instance '"+localhost+":"+__mysql_sandbox_port2+"' configuration cannot be persisted since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please set the 'group_replication_start_on_boot' variable to 'OFF' in the server configuration file, otherwise it might rejoin the cluster upon restart.\n":""\>>>
<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port1+"' cannot persist configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.\n":""\>>>
<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port3+"' cannot persist configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.\n":""\>>>

The instance 'localhost:<<<__mysql_sandbox_port2>>>' was successfully removed from the cluster.

//@<OUT> Removing instance (interactive: true, force: true)
The instance will be removed from the InnoDB cluster. Depending on the instance
being the Seed or not, the Metadata session might become invalid. If so, please
start a new session to the Metadata Storage R/W instance.

* Waiting for instance to synchronize with the primary...
Instance 'localhost:<<<__mysql_sandbox_port3>>>' is attempting to leave the cluster...
<<<(__version_num<80011)?"WARNING: On instance '"+localhost+":"+__mysql_sandbox_port3+"' configuration cannot be persisted since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please set the 'group_replication_start_on_boot' variable to 'OFF' in the server configuration file, otherwise it might rejoin the cluster upon restart.\n":""\>>>
<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port1+"' cannot persist configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.\n":""\>>>

The instance 'localhost:<<<__mysql_sandbox_port3>>>' was successfully removed from the cluster.

//@ Adding instance on port2 back (interactive: false, force: false)
||

//@ Adding instance on port3 back (interactive: false, force: true)
||

//@<OUT> Removing instance (interactive: false, force: false)
The instance will be removed from the InnoDB cluster. Depending on the instance
being the Seed or not, the Metadata session might become invalid. If so, please
start a new session to the Metadata Storage R/W instance.

* Waiting for instance to synchronize with the primary...
Instance 'localhost:<<<__mysql_sandbox_port2>>>' is attempting to leave the cluster...
<<<(__version_num<80011)?"WARNING: On instance '"+localhost+":"+__mysql_sandbox_port2+"' configuration cannot be persisted since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please set the 'group_replication_start_on_boot' variable to 'OFF' in the server configuration file, otherwise it might rejoin the cluster upon restart.\n":""\>>>
<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port1+"' cannot persist configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.\n":""\>>>
<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port3+"' cannot persist configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.\n":""\>>>

The instance 'localhost:<<<__mysql_sandbox_port2>>>' was successfully removed from the cluster.

//@<OUT> Removing instance (interactive: false, force: true)
The instance will be removed from the InnoDB cluster. Depending on the instance
being the Seed or not, the Metadata session might become invalid. If so, please
start a new session to the Metadata Storage R/W instance.

* Waiting for instance to synchronize with the primary...
Instance 'localhost:<<<__mysql_sandbox_port3>>>' is attempting to leave the cluster...
<<<(__version_num<80011)?"WARNING: On instance '"+localhost+":"+__mysql_sandbox_port3+"' configuration cannot be persisted since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please set the 'group_replication_start_on_boot' variable to 'OFF' in the server configuration file, otherwise it might rejoin the cluster upon restart.\n":""\>>>
<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port1+"' cannot persist configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.\n":""\>>>

The instance 'localhost:<<<__mysql_sandbox_port3>>>' was successfully removed from the cluster.

//@ Adding instance on port2 back (force: false)
||

//@ Adding instance on port3 back (force: true)
||

//@<OUT> Removing instance (force: false)
The instance will be removed from the InnoDB cluster. Depending on the instance
being the Seed or not, the Metadata session might become invalid. If so, please
start a new session to the Metadata Storage R/W instance.

* Waiting for instance to synchronize with the primary...
Instance 'localhost:<<<__mysql_sandbox_port2>>>' is attempting to leave the cluster...
<<<(__version_num<80011)?"WARNING: On instance '"+localhost+":"+__mysql_sandbox_port2+"' configuration cannot be persisted since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please set the 'group_replication_start_on_boot' variable to 'OFF' in the server configuration file, otherwise it might rejoin the cluster upon restart.\n":""\>>>
<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port1+"' cannot persist configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.\n":""\>>>
<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port3+"' cannot persist configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.\n":""\>>>

The instance 'localhost:<<<__mysql_sandbox_port2>>>' was successfully removed from the cluster.

//@<OUT> Instance (force: false) does not exist in cluster metadata
false

//@<OUT> Removing instance (force: true)
The instance will be removed from the InnoDB cluster. Depending on the instance
being the Seed or not, the Metadata session might become invalid. If so, please
start a new session to the Metadata Storage R/W instance.

* Waiting for instance to synchronize with the primary...
Instance 'localhost:<<<__mysql_sandbox_port3>>>' is attempting to leave the cluster...
<<<(__version_num<80011)?"WARNING: On instance '"+localhost+":"+__mysql_sandbox_port3+"' configuration cannot be persisted since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please set the 'group_replication_start_on_boot' variable to 'OFF' in the server configuration file, otherwise it might rejoin the cluster upon restart.\n":""\>>>
<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port1+"' cannot persist configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.\n":""\>>>

The instance 'localhost:<<<__mysql_sandbox_port3>>>' was successfully removed from the cluster.

//@<OUT> Instance (force: true) does not exist in cluster metadata
false

//@<OUT> Number of instance according to GR after removal.
1

//@ Stop instance on port2.
||

//@ Restart instance on port2.
||

//@ Connect to restarted instance3.
||

//@<OUT> Removed instance3 does not exist on its Metadata.
false

//@ Connect to restarted instance2.
||

//@<OUT> Removed instance2 does not exist on its Metadata.
false

//@<OUT> Confirm that GR start on boot is disabled {VER(>=8.0.11)}.
0

//@ Connect back to seed instance and get cluster.
||

//@ Adding instance on port2 back
||

//@ Adding instance on port3 back
||

//@ Reset persisted gr_start_on_boot on instances {VER(>=8.0.11)}
||

//@ Stop instance on port2
||

//@ Stop instance on port3
||

//@<OUT> Cluster status after instance on port2 is stopped
{
    "clusterName": "dev",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures. 2 members are not active.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "memberRole": "PRIMARY",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "[[*]]"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "memberRole": "SECONDARY",
                "mode": "n/a",
                "readReplicas": {},
                "role": "HA",
                "shellConnectError": "[[*]]",
                "status": "(MISSING)"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "memberRole": "SECONDARY",
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

//@<OUT> Error removing stopped instance on port2 using alternative host not in Metadata (no prompt)
WARNING: MySQL Error 2003 (HY000): Can't connect to MySQL server on '<<<libmysql_host_description(hostname_ip, __mysql_sandbox_port2)>>>' ([[*]])
ERROR: The instance <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> is not reachable and does not belong to the cluster either. Please ensure the member is either connectable or remove it through the exact address as shown in the cluster status output.

//@<ERR> Error removing stopped instance on port2 using alternative host not in Metadata (no prompt)
Cluster.removeInstance: Metadata for instance <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> not found (MYSQLSH 51104)

//@<OUT> Error removing stopped instance on port2 using alternative host not in Metadata and wrong pwd (no prompt)
WARNING: MySQL Error 2003 (HY000): Can't connect to MySQL server on '<<<libmysql_host_description(hostname_ip, __mysql_sandbox_port2)>>>' ([[*]])
ERROR: The instance <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> is not reachable and does not belong to the cluster either. Please ensure the member is either connectable or remove it through the exact address as shown in the cluster status output.

//@<ERR> Error removing stopped instance on port2 using alternative host not in Metadata and wrong pwd (no prompt)
Cluster.removeInstance: Metadata for instance <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> not found (MYSQLSH 51104)

//@ Error removing stopped instance on port2 (no prompt if interactive is false)
|WARNING: MySQL Error 2003 (HY000): Can't connect to MySQL server on '<<<libmysql_host_description(hostname, __mysql_sandbox_port2)>>>' ([[*]])|
|ERROR: The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is not reachable and cannot be safely removed from the cluster.|
||Can't connect to MySQL server on '<<<libmysql_host_description(hostname, __mysql_sandbox_port2)>>>' ([[*]]) (MySQL Error 2003)

//@ Error removing stopped instance on port2 (no prompt if force is used)
|WARNING: MySQL Error 2003 (HY000): Can't connect to MySQL server on '<<<libmysql_host_description(hostname, __mysql_sandbox_port2)>>>' ([[*]])|
|ERROR: The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is not reachable and cannot be safely removed from the cluster.|
||Can't connect to MySQL server on '<<<libmysql_host_description(hostname, __mysql_sandbox_port2)>>>' ([[*]]) (MySQL Error 2003)

//@ Error removing stopped instance on port2
|WARNING: MySQL Error 2003 (HY000): Can't connect to MySQL server on '<<<libmysql_host_description(hostname, __mysql_sandbox_port2)>>>' ([[*]])|
|ERROR: The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is not reachable and cannot be safely removed from the cluster.|
||Can't connect to MySQL server on '<<<libmysql_host_description(hostname, __mysql_sandbox_port2)>>>' ([[*]]) (MySQL Error 2003)

//@ Remove stopped instance on port2 with force option
||

//@<OUT> Confirm instance2 is not in cluster metadata
false

//@<OUT> Remove unreachable instance (interactive: false, force: false) - error
WARNING: MySQL Error 20[[*]]
ERROR: The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' is not reachable and cannot be safely removed from the cluster.
To safely remove the instance from the cluster, make sure the instance is back ONLINE and try again. If you are sure the instance is permanently unable to rejoin the group and no longer connectable, use the 'force' option to remove it from the metadata.

//@<ERR> Remove unreachable instance (interactive: false, force: false) - error
Cluster.removeInstance: [[*]] (MySQL Error 20[[*]])

//@<OUT> Remove unreachable instance (interactive: false, force: true) - success
NOTE: MySQL Error 20[[*]]
NOTE: The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' is not reachable and it will only be removed from the metadata. Please take any necessary actions to ensure the instance will not rejoin the cluster if brought back online.

The instance will be removed from the InnoDB cluster. Depending on the instance
being the Seed or not, the Metadata session might become invalid. If so, please
start a new session to the Metadata Storage R/W instance.

<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port1+"' cannot persist configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.\n":""\>>>

The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' was successfully removed from the cluster.

//@<OUT> remove instance not in MD and unreachable, interactive true (should fail)
WARNING: MySQL Error 20[[*]]
ERROR: The instance <<<hostname>>>:<<<__mysql_sandbox_port3>>> is not reachable and does not belong to the cluster either. Please ensure the member is either connectable or remove it through the exact address as shown in the cluster status output.

//@<ERR> remove instance not in MD and unreachable, interactive true (should fail)
Cluster.removeInstance: Metadata for instance <<<hostname>>>:<<<__mysql_sandbox_port3>>> not found (MYSQLSH 51104)

//@<OUT> remove instance not in MD and unreachable, interactive false (should fail)
WARNING: MySQL Error 20[[*]]
ERROR: The instance <<<hostname>>>:<<<__mysql_sandbox_port3>>> is not reachable and does not belong to the cluster either. Please ensure the member is either connectable or remove it through the exact address as shown in the cluster status output.

//@<ERR> remove instance not in MD and unreachable, interactive false (should fail)
Cluster.removeInstance: Metadata for instance <<<hostname>>>:<<<__mysql_sandbox_port3>>> not found (MYSQLSH 51104)

//@<OUT> Cluster status after removal of instance on port2 and port3
{
    "clusterName": "dev",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "memberRole": "PRIMARY",
                "mode": "R/W",
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

//@ Remove instances post actions since remove when unreachable (ensure they do not rejoin cluster)
||

//@ Restart instance on port2 again.
||

//@ Restart instance on port3 again.
||

//@<OUT> remove reachable instance but not MD, interactive false (should fail)
ERROR: The instance <<<hostname>>>:<<<__mysql_sandbox_port3>>> does not belong to the cluster.

//@<ERR> remove reachable instance but not MD, interactive false (should fail)
Cluster.removeInstance: Metadata for instance <<<hostname>>>:<<<__mysql_sandbox_port3>>> not found (MYSQLSH 51104)

//@<OUT> remove reachable instance but not MD, interactive true (should fail)
ERROR: The instance <<<hostname>>>:<<<__mysql_sandbox_port3>>> does not belong to the cluster.

//@<ERR> remove reachable instance but not MD, interactive true (should fail)
Cluster.removeInstance: Metadata for instance <<<hostname>>>:<<<__mysql_sandbox_port3>>> not found (MYSQLSH 51104)

//@ Connect to instance2 (removed unreachable)
||

//@<OUT> Confirm instance2 is still in its metadata
true

//@ Connect to seed instance and get cluster again
||

//@ Adding instance on port2 again
||

//@ Reset persisted gr_start_on_boot on instance2 {VER(>=8.0.11)}
||

//@ Stop instance on port2 again
||

//@ Adding instance on port3 again
||

//@ Stop instance on port3 again
||

//@<OUT> Remove unreachable instance (interactive: true, force: false) - error
WARNING: MySQL Error 2003 (HY000): Can't connect to MySQL server on '<<<libmysql_host_description(hostname, __mysql_sandbox_port2)>>>' ([[*]])
ERROR: The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is not reachable and cannot be safely removed from the cluster.
To safely remove the instance from the cluster, make sure the instance is back ONLINE and try again. If you are sure the instance is permanently unable to rejoin the group and no longer connectable, use the 'force' option to remove it from the metadata.

//@<ERR> Remove unreachable instance (interactive: true, force: false) - error
Cluster.removeInstance: Can't connect to MySQL server on '<<<libmysql_host_description(hostname, __mysql_sandbox_port2)>>>' ([[*]]) (MySQL Error 2003)

//@<OUT> Remove unreachable instance (interactive: true, answer NO) - error
WARNING: MySQL Error 2003 (HY000): Can't connect to MySQL server on '<<<libmysql_host_description(hostname, __mysql_sandbox_port2)>>>' ([[*]])
ERROR: The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is not reachable and cannot be safely removed from the cluster.
To safely remove the instance from the cluster, make sure the instance is back ONLINE and try again. If you are sure the instance is permanently unable to rejoin the group and no longer connectable, use the 'force' option to remove it from the metadata.

Do you want to continue anyway (only the instance metadata will be removed)? [y/N]:

//@<ERR> Remove unreachable instance (interactive: true, answer NO) - error
Cluster.removeInstance: Can't connect to MySQL server on '<<<libmysql_host_description(hostname, __mysql_sandbox_port2)>>>' ([[*]]) (MySQL Error 2003)

//@<OUT> Remove unreachable instance (interactive: true, answer YES) - success
WARNING: MySQL Error 2003 (HY000): Can't connect to MySQL server on '<<<libmysql_host_description(hostname, __mysql_sandbox_port2)>>>' ([[*]])
ERROR: The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is not reachable and cannot be safely removed from the cluster.
To safely remove the instance from the cluster, make sure the instance is back ONLINE and try again. If you are sure the instance is permanently unable to rejoin the group and no longer connectable, use the 'force' option to remove it from the metadata.

Do you want to continue anyway (only the instance metadata will be removed)? [y/N]:
The instance will be removed from the InnoDB cluster. Depending on the instance
being the Seed or not, the Metadata session might become invalid. If so, please
start a new session to the Metadata Storage R/W instance.

<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port1+"' cannot persist configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.\n":""\>>>

The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was successfully removed from the cluster.

//@<OUT> Remove unreachable instance (interactive: true, force: true) - success
NOTE: MySQL Error 20[[*]]
NOTE: The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' is not reachable and it will only be removed from the metadata. Please take any necessary actions to ensure the instance will not rejoin the cluster if brought back online.

The instance will be removed from the InnoDB cluster. Depending on the instance
being the Seed or not, the Metadata session might become invalid. If so, please
start a new session to the Metadata Storage R/W instance.

<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port1+"' cannot persist configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.\n":""\>>>

The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' was successfully removed from the cluster.

//@ Remove instance post actions (ensure they do not rejoin cluster)
||

//@ Restart instance on port2 one last time
||

//@ Adding instance on port2 one last time
||

//@ Connect to instance2 to introduce a replication error
||

//@ Introduce a replication error at instance 2
||

//@ Connect to seed instance and get cluster on last time
||

//@ Change shell option dba.gtidWaitTimeout to 5 second
||

//@<OUT> Remove instance with replication error - error
The instance will be removed from the InnoDB cluster. Depending on the instance
being the Seed or not, the Metadata session might become invalid. If so, please
start a new session to the Metadata Storage R/W instance.

* Waiting for instance to synchronize with the primary...

//@<OUT> Remove instance with replication error - error {VER(>=8.0.23)}
ERROR: Coordinator error in replication channel 'group_replication_applier': [[*]]
ERROR: The instance 'localhost:<<<__mysql_sandbox_port2>>>' was unable to catch up with cluster transactions. There might be too many transactions to apply or some replication error. In the former case, you can retry the operation (using a higher timeout value by setting the global shell option 'dba.gtidWaitTimeout'). In the later case, analyze and fix any replication error. You can also choose to skip this error using the 'force: true' option, but it might leave the instance in an inconsistent state and lead to errors if you want to reuse it.

//@<OUT> Remove instance with replication error - error {VER(<8.0.23)}
ERROR: Applier error in replication channel 'group_replication_applier': [[*]]

//@<ERR> Remove instance with replication error - error {VER(>=8.0.23)}
Cluster.removeInstance: <<<hostname>>>:<<<__mysql_sandbox_port2>>>: Error found in replication coordinator thread (MYSQLSH 51144)

//@<ERR> Remove instance with replication error - error {VER(<8.0.23)}
Cluster.removeInstance: <<<hostname>>>:<<<__mysql_sandbox_port2>>>: Error found in replication applier thread (MYSQLSH 51145)

//@ Change shell option dba.gtidWaitTimeout to 10 second
||

//@<OUT> Remove instance with replication error (force: true) - success
NOTE: <<<hostname>>>:<<<__mysql_sandbox_port2>>> is reachable but has state ERROR
The instance will be removed from the InnoDB cluster. Depending on the instance
being the Seed or not, the Metadata session might become invalid. If so, please
start a new session to the Metadata Storage R/W instance.

NOTE: The recovery user name for instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' does not match the expected format for users created automatically by InnoDB Cluster. Skipping its removal.
NOTE: Transaction sync was skipped
Instance 'localhost:<<<__mysql_sandbox_port2>>>' is attempting to leave the cluster...
<<<(__version_num<80011)?"WARNING: On instance '"+localhost+":"+__mysql_sandbox_port2+"' configuration cannot be persisted since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please set the 'group_replication_start_on_boot' variable to 'OFF' in the server configuration file, otherwise it might rejoin the cluster upon restart.\n":""\>>>
<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port1+"' cannot persist configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.\n":""\>>>

The instance 'localhost:<<<__mysql_sandbox_port2>>>' was successfully removed from the cluster.

//@ Error removing last instance
|ERROR: The instance 'localhost:<<<__mysql_sandbox_port1>>>' cannot be removed because it is the only member of the Cluster. Please use <Cluster>.dissolve() instead to remove the last instance and dissolve the Cluster.|Cluster.removeInstance: The instance 'localhost:<<<__mysql_sandbox_port1>>>' is the last member of the cluster (RuntimeError)

//@ Dissolve cluster with success
||

//@ Cluster re-created with success
||

//@ Finalization
||
