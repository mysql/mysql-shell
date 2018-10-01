testutil.deploy_sandbox(__mysql_sandbox_port1, 'root');
testutil.snapshot_sandbox_conf(__mysql_sandbox_port1);

shell.connect({'scheme': 'mysql', 'host': localhost, 'port': __mysql_sandbox_port1, 'user': 'root', 'password': 'root'})

c = dba.create_cluster('c')
session.close()

#@ cluster
c.help()

#@ global ? for cluster[USE:cluster]
\? cluster

#@ global help for cluster[USE:cluster]
\help cluster

#@ cluster.add_instance
c.help('add_instance')

#@ global ? for add_instance[USE:cluster.add_instance]
\? cluster.add_instance

#@ global help for add_instance[USE:cluster.add_instance]
\help cluster.add_instance

#@ cluster.check_instance_state
c.help('check_instance_state')

#@ global ? for check_instance_state[USE:cluster.check_instance_state]
\? cluster.check_instance_state

#@ global help for check_instance_state[USE:cluster.check_instance_state]
\help cluster.check_instance_state

#@ cluster.describe
c.help('describe')

#@ global ? for describe[USE:cluster.describe]
\? cluster.describe

#@ global help for describe[USE:cluster.describe]
\help cluster.describe

#@ cluster.disconnect
c.help('disconnect')

#@ global ? for disconnect[USE:cluster.disconnect]
\? cluster.disconnect

#@ global help for disconnect[USE:cluster.disconnect]
\help cluster.disconnect

#@ cluster.dissolve
c.help('dissolve')

#@ global ? for dissolve[USE:cluster.dissolve]
\? cluster.dissolve

#@ global help for dissolve[USE:cluster.dissolve]
\help cluster.dissolve

#@ cluster.force_quorum_using_partition_of
c.help('force_quorum_using_partition_of')

#@ global ? for force_quorum_using_partition_of[USE:cluster.force_quorum_using_partition_of]
\? cluster.force_quorum_using_partition_of

#@ global help for force_quorum_using_partition_of[USE:cluster.force_quorum_using_partition_of]
\help cluster.force_quorum_using_partition_of

#@ cluster.get_name
c.help('get_name')

#@ global ? for get_name[USE:cluster.get_name]
\? cluster.get_name

#@ global help for get_name[USE:cluster.get_name]
\help cluster.get_name

#@ cluster.help
c.help('help')

#@ global ? for help[USE:cluster.help]
\? cluster.help

#@ global help for help[USE:cluster.help]
\help cluster.help

#@ cluster.name
c.help('name')

#@ global ? for name[USE:cluster.name]
\? cluster.name

#@ global help for name[USE:cluster.name]
\help cluster.name

#@ cluster.rejoin_instance
c.help('rejoin_instance')

#@ global ? for rejoin_instance[USE:cluster.rejoin_instance]
\? cluster.rejoin_instance

#@ global help for rejoin_instance[USE:cluster.rejoin_instance]
\help cluster.rejoin_instance

#@ cluster.remove_instance
c.help('remove_instance')

#@ global ? for remove_instance[USE:cluster.remove_instance]
\? cluster.remove_instance

#@ global help for remove_instance[USE:cluster.remove_instance]
\help cluster.remove_instance

#@ cluster.rescan
c.help('rescan')

#@ global ? for rescan[USE:cluster.rescan]
\? cluster.rescan

#@ global help for rescan[USE:cluster.rescan]
\help cluster.rescan

#@ cluster.status
c.help('status')

#@ global ? for status[USE:cluster.status]
\? cluster.status

#@ global help for status[USE:cluster.status]
\help cluster.status

#@ cluster.set_primary_instance
c.help('set_primary_instance')

#@ global ? for set_primary_instance[USE:cluster.set_primary_instance]
\? cluster.set_primary_instance

#@ global help for set_primary_instance[USE:cluster.set_primary_instance]
\help cluster.set_primary_instance

#@ cluster.switch_to_multi_primary_mode
c.help('switch_to_multi_primary_mode')

#@ global ? for switch_to_multi_primary_mode[USE:cluster.switch_to_multi_primary_mode]
\? cluster.switch_to_multi_primary_mode

#@ global help for switch_to_multi_primary_mode[USE:cluster.switch_to_multi_primary_mode]
\help cluster.switch_to_multi_primary_mode

#@ cluster.switch_to_single_primary_mode
c.help('switch_to_single_primary_mode')

#@ global ? for switch_to_single_primary_mode[USE:cluster.switch_to_single_primary_mode]
\? cluster.switch_to_single_primary_mode

#@ global help for switch_to_single_primary_mode[USE:cluster.switch_to_single_primary_mode]
\help cluster.switch_to_single_primary_mode

c.disconnect()

testutil.destroy_sandbox(__mysql_sandbox_port1);
