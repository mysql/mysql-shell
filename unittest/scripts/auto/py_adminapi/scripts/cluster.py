#@<> Cluster: validating members
testutil.deploy_sandbox(__mysql_sandbox_port1, 'root');
testutil.snapshot_sandbox_conf(__mysql_sandbox_port1);


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
    'status',
    ])

cluster.disconnect()
session.close()

testutil.destroy_sandbox(__mysql_sandbox_port1);
