//@ FR1-TS-01 SETUP{VER(>=8.0.12)}
||

//@<OUT> FR1-TS-01 Check persisted variables after create cluster {VER(>=8.0.12)}
group_replication_exit_state_action = READ_ONLY
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
group_replication_local_address = <<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>
group_replication_recovery_use_ssl = ON
group_replication_single_primary_mode = ON
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = ON

//@ FR1-TS-01 reboot instance {VER(>=8.0.12)}
||

//@<OUT> FR1-TS-01 reboot cluster {VER(>=8.0.12)}
true
Reconfiguring the cluster 'C' from complete outage...


The cluster was successfully rebooted.

<Cluster:C>

//@<OUT> FR1-TS-01 check persisted variables {VER(>=8.0.12)}
group_replication_consistency = EVENTUAL
group_replication_autorejoin_tries = 0
group_replication_exit_state_action = READ_ONLY
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
group_replication_ip_whitelist = AUTOMATIC
group_replication_local_address = <<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>
group_replication_member_expel_timeout = 0
group_replication_member_weight = 50
group_replication_recovery_use_ssl = ON
group_replication_single_primary_mode = ON
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = ON
{
    "clusterName": "C",
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
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@ FR1-TS-01 TEARDOWN {VER(>=8.0.12)}
||

//@ FR1-TS-03 SETUP {VER(>=8.0.12)}
||

//@<OUT> FR1-TS-03 {VER(>=8.0.12)}
true
A new InnoDB cluster will be created on instance 'localhost:<<<__mysql_sandbox_port1>>>'.

Validating instance at localhost:<<<__mysql_sandbox_port1>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Instance configuration is suitable.
WARNING: Instance 'localhost:<<<__mysql_sandbox_port1>>>' will not load the persisted cluster configuration upon reboot since 'persisted-globals-load' is set to 'OFF'. Please use the <Dba>.configureLocalInstance() command locally to persist the changes or set 'persisted-globals-load' to 'ON' on the configuration file.
Creating InnoDB cluster 'C' on 'localhost:<<<__mysql_sandbox_port1>>>'...

Adding Seed Instance...
Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

<Cluster:C>

//@ FR1-TS-03 TEARDOWN {VER(>=8.0.12)}
||

//@ FR1-TS-04/05 SETUP {VER(>=8.0.12)}
|The instance 'localhost:<<<__mysql_sandbox_port1>>>' was configured for InnoDB cluster usage.|
|Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.|

//@<OUT> FR1-TS-04/05 {VER(>=8.0.12)}
true

group_replication_exit_state_action = READ_ONLY
group_replication_group_name = 62d73bbd-b830-11e7-a7b7-34e6d72fbd80
group_replication_ip_whitelist = 255.255.255.255/32,127.0.0.1,<<<hostname_ip>>>,<<<hostname>>>
group_replication_local_address = localhost:<<<__local_address_1>>>
group_replication_recovery_use_ssl = ON
group_replication_single_primary_mode = ON
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = ON

The instance 'localhost:<<<__mysql_sandbox_port1>>>' belongs to an InnoDB cluster.
Calling this function on a cluster member is only required for MySQL versions 8.0.4 or earlier.

//@ FR1-TS-04/05 TEARDOWN {VER(>=8.0.12)}
||

//@ FR1-TS-06 SETUP {VER(<8.0.12)}
||

//@<OUT> FR1-TS-06 {VER(<8.0.12)}
true
A new InnoDB cluster will be created on instance 'localhost:<<<__mysql_sandbox_port1>>>'.

Validating instance at localhost:<<<__mysql_sandbox_port1>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Instance configuration is suitable.
WARNING: Instance 'localhost:<<<__mysql_sandbox_port1>>>' cannot persist Group Replication configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance() command locally to persist the changes.
Creating InnoDB cluster 'ClusterName' on 'localhost:<<<__mysql_sandbox_port1>>>'...

Adding Seed Instance...
Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

<Cluster:ClusterName>

//@ FR1-TS-06 TEARDOWN {VER(<8.0.12)}
||

//@ FR1-TS-7 SETUP {VER(>=8.0.12)}
||

//@<OUT> FR1-TS-7 show persisted cluster variables {VER(>=8.0.12)}
true

group_replication_exit_state_action = READ_ONLY
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
group_replication_local_address = <<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>
group_replication_recovery_use_ssl = ON
group_replication_single_primary_mode = OFF
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = ON


//@ FR1-TS-7 reboot instance 1 {VER(>=8.0.12)}
||

//@<OUT> FR1-TS-7 reboot cluster {VER(>=8.0.12)}
true
Reconfiguring the cluster 'ClusterName' from complete outage...


The cluster was successfully rebooted.

<Cluster:ClusterName>

//@<OUT> FR1-TS-7 check persisted variables {VER(>=8.0.12)}
group_replication_consistency = EVENTUAL
group_replication_autorejoin_tries = 0
group_replication_exit_state_action = READ_ONLY
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
group_replication_ip_whitelist = AUTOMATIC
group_replication_local_address = <<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>
group_replication_member_expel_timeout = 0
group_replication_member_weight = 50
group_replication_recovery_use_ssl = ON
group_replication_single_primary_mode = OFF
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = ON
{
    "clusterName": "ClusterName",
    "defaultReplicaSet": {
        "name": "default",
        "ssl": "REQUIRED",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            }
        },
        "topologyMode": "Multi-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@ FR1-TS-7 TEARDOWN {VER(>=8.0.12)}
||

//@ FR2-TS-1 SETUP {VER(>=8.0.12)}
||

//@<OUT> FR2-TS-1 check persisted variables on instance 1 {VER(>=8.0.12)}
true

group_replication_consistency = EVENTUAL
group_replication_exit_state_action = READ_ONLY
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
group_replication_group_seeds = <<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>
group_replication_local_address = <<<hostname>>>:<<<__mysql_sandbox_gr_port2>>>
group_replication_member_expel_timeout = 0
group_replication_recovery_use_ssl = ON
group_replication_single_primary_mode = ON
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = ON

//@ FR2-TS-1 stop instance 2 {VER(>=8.0.12)}
||

//@<OUT> FR2-TS-1 cluster status showing instance 2 is missing {VER(>=8.0.12)}
true
{
    "clusterName": "ClusterName",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures. 1 member is not active",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "n/a",
                "readReplicas": {},
                "role": "HA",
                "status": "(MISSING)"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@ FR2-TS-1 start instance 1 {VER(>=8.0.12)}
||

//@<OUT> FR2-TS-1 cluster status showing instance 2 is back online {VER(>=8.0.12)}
true
{
    "clusterName": "ClusterName",
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
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@ FR2-TS-1 TEARDOWN {VER(>=8.0.12)}
||

//@ FR2-TS-3 SETUP {VER(>=8.0.12)}
||

//@<OUT> FR2-TS-3 check that warning is displayed when adding instance with persisted-globals-load=OFF {VER(>=8.0.12)}
true
Validating instance at localhost:<<<__mysql_sandbox_port2>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Instance configuration is suitable.
WARNING: Instance 'localhost:<<<__mysql_sandbox_port2>>>' will not load the persisted cluster configuration upon reboot since 'persisted-globals-load' is set to 'OFF'. Please use the <Dba>.configureLocalInstance() command locally to persist the changes or set 'persisted-globals-load' to 'ON' on the configuration file.
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Adding instance to the cluster ...

The instance 'localhost:<<<__mysql_sandbox_port2>>>' was successfully added to the cluster.

ONLINE
{
    "clusterName": "ClusterName",
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
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@ FR2-TS-3 TEARDOWN {VER(>=8.0.12)}
||

//@ FR2-TS-4 SETUP {VER(>=8.0.12)}
||

//@<OUT> FR2-TS-4 Check that persisted variables match the ones passed on the arguments to create cluster and addInstance {VER(>=8.0.12)}
true

group_replication_exit_state_action = READ_ONLY
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
group_replication_group_seeds = localhost:<<<__local_address_2>>>
group_replication_ip_whitelist = 255.255.255.255/32,127.0.0.1,<<<hostname_ip>>>,<<<hostname>>>
group_replication_local_address = <<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>
group_replication_recovery_use_ssl = ON
group_replication_single_primary_mode = ON
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = ON

group_replication_consistency = EVENTUAL
group_replication_exit_state_action = READ_ONLY
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
group_replication_group_seeds = <<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>
group_replication_ip_whitelist = 255.255.255.255/32,127.0.0.1,<<<hostname_ip>>>,<<<hostname>>>
group_replication_local_address = localhost:<<<__local_address_2>>>
group_replication_member_expel_timeout = 0
group_replication_recovery_use_ssl = ON
group_replication_single_primary_mode = ON
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = ON


//@ FR2-TS-4 TEARDOWN {VER(>=8.0.12)}
||

//@ FR2-TS-5 SETUP {VER(>=8.0.12)}
||

//@<OUT> FR2-TS-5 {VER(>=8.0.12)}
true

group_replication_exit_state_action = READ_ONLY
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
group_replication_group_seeds = localhost:<<<__local_address_3>>>
group_replication_local_address = <<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>
group_replication_recovery_use_ssl = ON
group_replication_single_primary_mode = ON
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = ON

group_replication_consistency = EVENTUAL
group_replication_exit_state_action = READ_ONLY
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
group_replication_group_seeds = <<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>
group_replication_ip_whitelist = 255.255.255.255/32,127.0.0.1,<<<hostname_ip>>>,<<<hostname>>>
group_replication_local_address = localhost:<<<__local_address_3>>>
group_replication_member_expel_timeout = 0
group_replication_recovery_use_ssl = ON
group_replication_single_primary_mode = ON
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = ON
The instance 'localhost:<<<__mysql_sandbox_port2>>>' belongs to an InnoDB cluster.
Calling this function on a cluster member is only required for MySQL versions 8.0.4 or earlier.

group_replication_consistency = EVENTUAL
group_replication_exit_state_action = READ_ONLY
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
group_replication_group_seeds = <<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>
group_replication_ip_whitelist = 255.255.255.255/32,127.0.0.1,<<<hostname_ip>>>,<<<hostname>>>
group_replication_local_address = localhost:<<<__local_address_3>>>
group_replication_member_expel_timeout = 0
group_replication_recovery_use_ssl = ON
group_replication_single_primary_mode = ON
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = ON

//@ FR2-TS-5 TEARDOWN {VER(>=8.0.12)}
||

//@ FR2-TS-6 SETUP {VER(<8.0.12)}
||

//@<OUT> FR2-TS-6 Warning is displayed on addInstance {VER(<8.0.12)}
Validating instance at localhost:<<<__mysql_sandbox_port2>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Instance configuration is suitable.
WARNING: Instance 'localhost:<<<__mysql_sandbox_port2>>>' cannot persist Group Replication configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance() command locally to persist the changes.
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Adding instance to the cluster ...

WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' cannot persist configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance() command locally to persist the changes.
The instance 'localhost:<<<__mysql_sandbox_port2>>>' was successfully added to the cluster.

//@ FR2-TS-6 TEARDOWN {VER(<8.0.12)}
||

//@ FR2-TS-8 SETUP {VER(>=8.0.12)}
||

//@<OUT> FR2-TS-8 Check that correct values were persisted and that instance rejoins automatically {VER(>=8.0.12)}
true

group_replication_exit_state_action = READ_ONLY
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
group_replication_group_seeds = <<<hostname>>>:<<<__mysql_sandbox_gr_port2>>>
group_replication_local_address = <<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>
group_replication_recovery_use_ssl = ON
group_replication_single_primary_mode = OFF
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = ON

group_replication_consistency = EVENTUAL
group_replication_exit_state_action = READ_ONLY
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
group_replication_group_seeds = <<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>
group_replication_local_address = <<<hostname>>>:<<<__mysql_sandbox_gr_port2>>>
group_replication_member_expel_timeout = 0
group_replication_recovery_use_ssl = ON
group_replication_single_primary_mode = OFF
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = ON
(MISSING)
{
    "clusterName": "ClusterName",
    "defaultReplicaSet": {
        "name": "default",
        "ssl": "REQUIRED",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures. 1 member is not active",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "n/a",
                "readReplicas": {},
                "role": "HA",
                "status": "(MISSING)"
            }
        },
        "topologyMode": "Multi-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}
ONLINE
{
    "clusterName": "ClusterName",
    "defaultReplicaSet": {
        "name": "default",
        "ssl": "REQUIRED",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            }
        },
        "topologyMode": "Multi-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@ FR2-TS-8 TEARDOWN {VER(>=8.0.12)}
||

//@ FR2-TS-9 SETUP {VER(>=8.0.12)}
||

//@<OUT> FR2-TS-9 Check that correct values were persisted on instance 2 {VER(>=8.0.12)}
true

group_replication_consistency = EVENTUAL
group_replication_exit_state_action = READ_ONLY
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
group_replication_group_seeds = <<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>
group_replication_local_address = <<<hostname>>>:<<<__mysql_sandbox_gr_port2>>>
group_replication_member_expel_timeout = 0
group_replication_recovery_use_ssl = ON
group_replication_single_primary_mode = ON
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = ON

//@FR2-TS-9 Add instance 3 and wait for it to be online {VER(>=8.0.12)}
||

//@<OUT> FR2-TS-9 Check that correct values are persisted and updated when instances are added and that instances rejoin automatically {VER(>=8.0.12)}
group_replication_consistency = EVENTUAL
group_replication_exit_state_action = READ_ONLY
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
group_replication_group_seeds = <<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>,<<<hostname>>>:<<<__mysql_sandbox_gr_port2>>>
group_replication_local_address = <<<hostname>>>:<<<__mysql_sandbox_gr_port3>>>
group_replication_member_expel_timeout = 0
group_replication_recovery_use_ssl = ON
group_replication_single_primary_mode = ON
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = ON

group_replication_consistency = EVENTUAL
group_replication_exit_state_action = READ_ONLY
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
group_replication_group_seeds = <<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>,<<<hostname>>>:<<<__mysql_sandbox_gr_port3>>>
group_replication_local_address = <<<hostname>>>:<<<__mysql_sandbox_gr_port2>>>
group_replication_member_expel_timeout = 0
group_replication_recovery_use_ssl = ON
group_replication_single_primary_mode = ON
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = ON

group_replication_exit_state_action = READ_ONLY
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
group_replication_group_seeds = <<<hostname>>>:<<<__mysql_sandbox_gr_port2>>>,<<<hostname>>>:<<<__mysql_sandbox_gr_port3>>>
group_replication_local_address = <<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>
group_replication_recovery_use_ssl = ON
group_replication_single_primary_mode = ON
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = ON
(MISSING)
(MISSING)
{
    "clusterName": "ClusterName",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures. 2 members are not active",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "n/a",
                "readReplicas": {},
                "role": "HA",
                "status": "(MISSING)"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "n/a",
                "readReplicas": {},
                "role": "HA",
                "status": "(MISSING)"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}
ONLINE
ONLINE
{
    "clusterName": "ClusterName",
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
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@ FR2-TS-9 TEARDOWN {VER(>=8.0.12)}
||

//@ FR5-TS-1 SETUP {VER(>=8.0.12)}
||

//@<OUT> FR5-TS-1 Check that persisted variables are updated/reset after removeCluster operation {VER(>=8.0.12)}
true

group_replication_consistency = EVENTUAL
group_replication_bootstrap_group = OFF
group_replication_exit_state_action = READ_ONLY
group_replication_force_members =
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
group_replication_group_seeds =
group_replication_local_address =
group_replication_member_expel_timeout = 0
group_replication_recovery_use_ssl = ON
group_replication_single_primary_mode = ON
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = OFF

group_replication_exit_state_action = READ_ONLY
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
group_replication_group_seeds =
group_replication_local_address = <<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>
group_replication_recovery_use_ssl = ON
group_replication_single_primary_mode = ON
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = ON

//@ FR5-TS-1 TEARDOWN {VER(>=8.0.12)}
||

//@ FR5-TS-4 SETUP {VER(>=8.0.12)}
||

//@<OUT> FR5-TS-4 Check that persisted variables are updated/reset after removeCluster operation - before {VER(>=8.0.12)}
true

group_replication_consistency = EVENTUAL
group_replication_exit_state_action = READ_ONLY
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
group_replication_group_seeds = <<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>,<<<hostname>>>:<<<__mysql_sandbox_gr_port2>>>
group_replication_local_address = <<<hostname>>>:<<<__mysql_sandbox_gr_port3>>>
group_replication_member_expel_timeout = 0
group_replication_recovery_use_ssl = ON
group_replication_single_primary_mode = ON
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = ON
The instance will be removed from the InnoDB cluster. Depending on the instance
being the Seed or not, the Metadata session might become invalid. If so, please
start a new session to the Metadata Storage R/W instance.

Instance 'localhost:<<<__mysql_sandbox_port2>>>' is attempting to leave the cluster...

The instance 'localhost:<<<__mysql_sandbox_port2>>>' was successfully removed from the cluster.

//@<OUT> FR5-TS-4 Check that persisted variables are updated/reset after removeCluster operation - after {VER(>=8.0.12)}
true

group_replication_consistency = EVENTUAL
group_replication_exit_state_action = READ_ONLY
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
group_replication_group_seeds = <<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>
group_replication_local_address = <<<hostname>>>:<<<__mysql_sandbox_gr_port3>>>
group_replication_member_expel_timeout = 0
group_replication_recovery_use_ssl = ON
group_replication_single_primary_mode = ON
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = ON

group_replication_consistency = EVENTUAL
group_replication_bootstrap_group = OFF
group_replication_exit_state_action = READ_ONLY
group_replication_force_members =
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
group_replication_group_seeds =
group_replication_local_address =
group_replication_member_expel_timeout = 0
group_replication_recovery_use_ssl = ON
group_replication_single_primary_mode = ON
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = OFF

group_replication_exit_state_action = READ_ONLY
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
group_replication_group_seeds = <<<hostname>>>:<<<__mysql_sandbox_gr_port3>>>
group_replication_local_address = <<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>
group_replication_recovery_use_ssl = ON
group_replication_single_primary_mode = ON
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = ON

//@ FR5-TS-4 TEARDOWN {VER(>=8.0.12)}
||

//@ FR5-Extra SETUP {VER(<8.0.12)}
||

//@<OUT> FR5-Extra Check that warning is shown when removeInstance is called {VER(<8.0.12)}
The instance will be removed from the InnoDB cluster. Depending on the instance
being the Seed or not, the Metadata session might become invalid. If so, please
start a new session to the Metadata Storage R/W instance.

Instance 'localhost:<<<__mysql_sandbox_port2>>>' is attempting to leave the cluster...
WARNING: On instance 'localhost:<<<__mysql_sandbox_port2>>>' configuration cannot be persisted since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please set the 'group_replication_start_on_boot' variable to 'OFF' in the server configuration file, otherwise it might rejoin the cluster upon restart.
WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' cannot persist configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance() command locally to persist the changes.
WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' cannot persist configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance() command locally to persist the changes.

The instance 'localhost:<<<__mysql_sandbox_port2>>>' was successfully removed from the cluster.

{
    "clusterName": "ClusterName",
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
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}


//@ FR5-Extra TEARDOWN {VER(<8.0.12)}
||

//@ Check if Cluster dissolve will reset persisted variables SETUP {VER(>=8.0.12)}
||


//@<OUT> Check if Cluster dissolve will reset persisted variables {VER(>=8.0.12)}
group_replication_bootstrap_group = OFF
group_replication_exit_state_action = READ_ONLY
group_replication_force_members =
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
group_replication_group_seeds =
group_replication_local_address =
group_replication_recovery_use_ssl = ON
group_replication_single_primary_mode = ON
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = OFF

//@ Check if Cluster dissolve will reset persisted variables TEARDOWN {VER(>=8.0.12)}
||
