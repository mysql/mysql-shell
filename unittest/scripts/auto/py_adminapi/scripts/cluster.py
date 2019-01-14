#@<> Cluster: validating members
testutil.deploy_sandbox(__mysql_sandbox_port1, 'root', {'report_host': hostname})
testutil.snapshot_sandbox_conf(__mysql_sandbox_port1)


shell.connect({'scheme': 'mysql', 'host': localhost, 'port': __mysql_sandbox_port1, 'user': 'root', 'password': 'root'})

cluster = dba.create_cluster('devCluster')

validate_members(cluster, [
    'add_instance',
    'check_instance_state',
    'describe',
    'dissolve',
    'disconnect',
    'force_quorum_using_partition_of',
    'get_name',
    'help',
    'name',
    'rejoin_instance',
    'remove_instance',
    'rescan',
    'options',
    'status',
    'set_primary_instance',
    'set_option',
    'set_instance_option',
    'switch_to_multi_primary_mode',
    'switch_to_single_primary_mode'
    ])

cluster.disconnect()
session.close()

testutil.destroy_sandbox(__mysql_sandbox_port1);
