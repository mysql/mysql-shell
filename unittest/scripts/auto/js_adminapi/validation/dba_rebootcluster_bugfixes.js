//@ BUG29265869 - Deploy sandboxes.
||

//@ BUG29265869 - Create cluster with custom GR settings.  {VER(>=8.0.16)}
||

//@ BUG29265869 - Create cluster with custom GR settings for 5.7. {VER(<8.0.0)}
||

//@ BUG29265869 - Add instance with custom GR settings. {VER(>=8.0.16)}
||

//@ BUG29265869 - Add instance with custom GR settings for 5.7. {VER(<8.0.0)}
||

//@ BUG29265869 - Persist GR settings for 5.7. {VER(<8.0.0)}
||

//@<OUT> BUG29265869 - Show initial cluster options.  {VER(>=8.0.17)}
{
    "clusterName": "test",
    "defaultReplicaSet": {
        "globalOptions": [
            {
                "option": "groupName",
                "value": "<<<grp_name>>>",
                "variable": "group_replication_group_name"
            },
            {
                "option": "memberSslMode",
                "value": "REQUIRED",
                "variable": "group_replication_ssl_mode"
            },
            {
                "option": "disableClone",
                "value": false
            }
        ],
        "topology": {
            "<<<uri1>>>": [
                {
                    "option": "consistency",
                    "value": "<<<consistency>>>",
                    "variable": "group_replication_consistency"
                },
                {
                    "option": "exitStateAction",
                    "value": "<<<exit_state>>>",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": "<<<expel_timeout>>>",
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "<<<local_address2>>>",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipWhitelist",
                    "value": "<<<ip_white_list80>>>",
                    "variable": "group_replication_ip_whitelist"
                },
                {
                    "option": "localAddress",
                    "value": "<<<local_address1>>>",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "<<<member_weight1>>>",
                    "variable": "group_replication_member_weight"
                }
            ],
            "<<<uri2>>>": [
                {
                    "option": "consistency",
                    "value": "<<<consistency>>>",
                    "variable": "group_replication_consistency"
                },
                {
                    "option": "exitStateAction",
                    "value": "<<<exit_state>>>",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": "<<<expel_timeout>>>",
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "<<<local_address1>>>",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipWhitelist",
                    "value": "<<<ip_white_list80>>>",
                    "variable": "group_replication_ip_whitelist"
                },
                {
                    "option": "localAddress",
                    "value": "<<<local_address2>>>",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "<<<member_weight2>>>",
                    "variable": "group_replication_member_weight"
                }
            ]
        }
    }
}

//@<OUT> BUG29265869 - Show initial cluster options.  {VER(<8.0.0)}
{
    "clusterName": "test",
    "defaultReplicaSet": {
        "globalOptions": [
            {
                "option": "groupName",
                "value": "<<<grp_name>>>",
                "variable": "group_replication_group_name"
            },
            {
                "option": "memberSslMode",
                "value": "REQUIRED",
                "variable": "group_replication_ssl_mode"
            },
            {
                "option": "disableClone",
                "value": true
            }
        ],
        "topology": {
            "<<<uri1>>>": [
                {
                    "option": "consistency",
                    "value": null,
                    "variable": "group_replication_consistency"
                },
                {
                    "option": "exitStateAction",
                    "value": "READ_ONLY",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": null,
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "<<<local_address2>>>",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipWhitelist",
                    "value": "<<<ip_white_list57>>>",
                    "variable": "group_replication_ip_whitelist"
                },
                {
                    "option": "localAddress",
                    "value": "<<<local_address1>>>",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "50",
                    "variable": "group_replication_member_weight"
                }
            ],
            "<<<uri2>>>": [
                {
                    "option": "consistency",
                    "value": null,
                    "variable": "group_replication_consistency"
                },
                {
                    "option": "exitStateAction",
                    "value": "READ_ONLY",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": null,
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "<<<local_address1>>>",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipWhitelist",
                    "value": "<<<ip_white_list57>>>",
                    "variable": "group_replication_ip_whitelist"
                },
                {
                    "option": "localAddress",
                    "value": "<<<local_address2>>>",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "75",
                    "variable": "group_replication_member_weight"
                }
            ]
        }
    }
}

//@ BUG29265869 - Kill all cluster members.
||

//@ BUG29265869 - Start the members again.
||

//@ BUG29265869 - connect to instance.
||

//@<OUT> BUG29265869 - Reboot cluster from complete outage and BUG30501978 no provision output shown. {VER(<8.0.11)}
WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' cannot persist Group Replication configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance() command locally to persist the changes.
WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' cannot persist Group Replication configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance() command locally to persist the changes.
ONLINE
ONLINE

//@<OUT> BUG29265869 - Reboot cluster from complete outage and BUG30501978 no provision output shown. {VER(>=8.0.11)}
ONLINE
ONLINE

//@<OUT> BUG29265869 - Show cluster options after reboot. {VER(>=8.0.17)}
{
    "clusterName": "test",
    "defaultReplicaSet": {
        "globalOptions": [
            {
                "option": "groupName",
                "value": "<<<grp_name>>>",
                "variable": "group_replication_group_name"
            },
            {
                "option": "memberSslMode",
                "value": "REQUIRED",
                "variable": "group_replication_ssl_mode"
            },
            {
                "option": "disableClone",
                "value": false
            }
        ],
        "topology": {
            "<<<uri1>>>": [
                {
                    "option": "consistency",
                    "value": "<<<consistency>>>",
                    "variable": "group_replication_consistency"
                },
                {
                    "option": "exitStateAction",
                    "value": "<<<exit_state>>>",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": "<<<expel_timeout>>>",
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "<<<local_address2>>>",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipWhitelist",
                    "value": "<<<ip_white_list80>>>",
                    "variable": "group_replication_ip_whitelist"
                },
                {
                    "option": "localAddress",
                    "value": "<<<local_address1>>>",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "<<<member_weight1>>>",
                    "variable": "group_replication_member_weight"
                }
            ],
            "<<<uri2>>>": [
                {
                    "option": "consistency",
                    "value": "<<<consistency>>>",
                    "variable": "group_replication_consistency"
                },
                {
                    "option": "exitStateAction",
                    "value": "<<<exit_state>>>",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": "<<<expel_timeout>>>",
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "<<<local_address1>>>",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipWhitelist",
                    "value": "<<<ip_white_list80>>>",
                    "variable": "group_replication_ip_whitelist"
                },
                {
                    "option": "localAddress",
                    "value": "<<<local_address2>>>",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "<<<member_weight2>>>",
                    "variable": "group_replication_member_weight"
                }
            ]
        }
    }
}

//@<OUT> BUG29265869 - Show cluster options after reboot.  {VER(<8.0.0)}
{
    "clusterName": "test",
    "defaultReplicaSet": {
        "globalOptions": [
            {
                "option": "groupName",
                "value": "<<<grp_name>>>",
                "variable": "group_replication_group_name"
            },
            {
                "option": "memberSslMode",
                "value": "REQUIRED",
                "variable": "group_replication_ssl_mode"
            },
            {
                "option": "disableClone",
                "value": true
            }
        ],
        "topology": {
            "<<<uri1>>>": [
                {
                    "option": "consistency",
                    "value": null,
                    "variable": "group_replication_consistency"
                },
                {
                    "option": "exitStateAction",
                    "value": "READ_ONLY",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": null,
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "<<<local_address2>>>",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipWhitelist",
                    "value": "<<<ip_white_list57>>>",
                    "variable": "group_replication_ip_whitelist"
                },
                {
                    "option": "localAddress",
                    "value": "<<<local_address1>>>",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "50",
                    "variable": "group_replication_member_weight"
                }
            ],
            "<<<uri2>>>": [
                {
                    "option": "consistency",
                    "value": null,
                    "variable": "group_replication_consistency"
                },
                {
                    "option": "exitStateAction",
                    "value": "READ_ONLY",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": null,
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "<<<local_address1>>>",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipWhitelist",
                    "value": "<<<ip_white_list57>>>",
                    "variable": "group_replication_ip_whitelist"
                },
                {
                    "option": "localAddress",
                    "value": "<<<local_address2>>>",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "75",
                    "variable": "group_replication_member_weight"
                }
            ]
        }
    }
}

//@<OUT> BUG#29305551 - Reboot cluster from complete outage, rejoin fails
ERROR: Cannot rejoin instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' to the cluster because it has asynchronous (master-slave) replication configured and running. Please stop the slave threads by executing the query: 'STOP SLAVE;'

//@ BUG#29305551: Finalization
||

//@ BUG30501978 - Reboot cluster from complete outage fails with informative message saying current session is not the most up to date
||Dba.rebootClusterFromCompleteOutage: The active session instance isn't the most updated in comparison with the ONLINE instances of the Cluster's metadata. Please use the most up to date instance: '<<<hostname>>>:<<<__mysql_sandbox_port2>>>'. (RuntimeError)

//@ BUG30501978 - Reboot cluster from complete outage fails with informative message saying there is a gtid mismatch
||Conflicting transaction sets between <<<hostname>>>:<<<__mysql_sandbox_port1>>> and <<<hostname>>>:<<<__mysql_sandbox_port2>>>

//@ BUG30501978 - Reboot cluster from complete outage fails with informative message saying to run rejoinInstance
|ERROR: A GTID set check of the MySQL instance at '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' determined that it is missing transactions that were purged from all cluster members.|
|Unable to rejoin instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' to the cluster but the <Dba>.rebootClusterFromCompleteOutage() operation will continue. The instance must be either cloned or fully re-provisioned before it can be re-added to the cluster.|
