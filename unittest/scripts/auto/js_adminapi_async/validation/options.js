//@<OUT> WL#13788 Check the output of rs.options is as expected and that the function gets its information through the primary
{
    "replicaSet": {
        "globalOptions": [
            {
                "option": "replicationAllowedHost", 
                "value": "%"
            }
        ],
        "name": "myrs",
        "tags": {
            ".global": [
                {
                    "option": "global_custom",
                    "value": "global_tag"
                }
            ],
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": [
                {
                    "option": "bool_tag",
                    "value": true
                },
                {
                    "option": "int_tag",
                    "value": 111
                },
                {
                    "option": "string_tag",
                    "value": "instance1"
                }
            ],
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": [
                {
                    "option": "_disconnect_existing_sessions_when_hidden",
                    "value": true
                },
                {
                    "option": "_hidden",
                    "value": false
                },
                {
                    "option": "bool_tag",
                    "value": false
                },
                {
                    "option": "int_tag",
                    "value": 222
                },
                {
                    "option": "string_tag",
                    "value": "instance2"
                }
            ]
        },
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": [
?{VER(<8.0.22)}
                {
                    "value": "COMMIT_ORDER",
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
                    "value": "DATABASE",
                    "variable": "<<<__replica_keyword>>>_parallel_type"
                },
                {
                    "value": "0",
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
                    "value": "OFF",
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
                },
?{}
?{VER(>=8.0.23)}
                {
                    "value": "WRITESET",
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
                    "value": "LOGICAL_CLOCK",
                    "variable": "<<<__replica_keyword>>>_parallel_type"
                },
                {
                    "value": "4",
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
                    "value": "ON",
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
                },
?{}
                {
                    "value": "XXHASH64",
                    "variable": "transaction_write_set_extraction"
                }
            ],
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": [
?{VER(<8.0.22)}
                {
                    "value": "COMMIT_ORDER",
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
                    "value": "DATABASE",
                    "variable": "<<<__replica_keyword>>>_parallel_type"
                },
                {
                    "value": "0",
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
                    "value": "OFF",
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
                },
?{}
?{VER(>=8.0.23)}
                {
                    "value": "WRITESET",
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
                    "value": "LOGICAL_CLOCK",
                    "variable": "<<<__replica_keyword>>>_parallel_type"
                },
                {
                    "value": "4",
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
                    "value": "ON",
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
                },
?{}
                {
                    "value": "XXHASH64",
                    "variable": "transaction_write_set_extraction"
                }
            ]
        }
    }
}

//@ Change the value of applierWorkerThreads of a member of the ReplicaSet {VER(>=8.0.23)}
|The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' belongs to an InnoDB ReplicaSet.|
|Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB ReplicaSet...|
|This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port2>>>|
|The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is valid to be used in an InnoDB ReplicaSet.|
|The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is already ready to be used in an InnoDB ReplicaSet.|
|WARNING: The changes on the value of <<<__replica_keyword>>>_parallel_workers will only take place after the instance leaves and rejoins the ReplicaSet.|
|Successfully set the value of <<<__replica_keyword>>>_parallel_workers.|

//@<OUT> Check the output of options after changing applierWorkerThreads {VER(>=8.0.23)}
{
    "replicaSet": {
        "globalOptions": [
            {
                "option": "replicationAllowedHost", 
                "value": "%"
            }
        ], 
        "name": "myrs",
        "tags": {
            ".global": [
                {
                    "option": "global_custom",
                    "value": "global_tag"
                }
            ],
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": [
                {
                    "option": "bool_tag",
                    "value": true
                },
                {
                    "option": "int_tag",
                    "value": 111
                },
                {
                    "option": "string_tag",
                    "value": "instance1"
                }
            ],
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": [
                {
                    "option": "_disconnect_existing_sessions_when_hidden",
                    "value": true
                },
                {
                    "option": "_hidden",
                    "value": false
                },
                {
                    "option": "bool_tag",
                    "value": false
                },
                {
                    "option": "int_tag",
                    "value": 222
                },
                {
                    "option": "string_tag",
                    "value": "instance2"
                }
            ]
        },
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": [
                {
                    "value": "WRITESET",
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
                    "value": "LOGICAL_CLOCK",
                    "variable": "<<<__replica_keyword>>>_parallel_type"
                },
                {
                    "value": "4",
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
                    "value": "ON",
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
                },
                {
                    "value": "XXHASH64",
                    "variable": "transaction_write_set_extraction"
                }
            ],
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": [
                {
                    "value": "WRITESET",
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
                    "value": "LOGICAL_CLOCK",
                    "variable": "<<<__replica_keyword>>>_parallel_type"
                },
                {
                    "value": "10",
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
                    "value": "ON",
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
                },
                {
                    "value": "XXHASH64",
                    "variable": "transaction_write_set_extraction"
                }
            ]
        }
    }
}
