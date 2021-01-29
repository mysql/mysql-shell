testutil.deploy_sandbox(__mysql_sandbox_port1, 'root', {'report_host': hostname})
testutil.snapshot_sandbox_conf(__mysql_sandbox_port1)

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

#@ cluster.options
c.help('options')

#@ global ? for options[USE:cluster.options]
\? cluster.options

#@ global help for options[USE:cluster.options]
\help cluster.options

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

#@ cluster.set_instance_option
c.help('set_instance_option')

#@ global ? for set_instance_option[USE:cluster.set_instance_option]
\? cluster.set_instance_option

#@ global help for set_instance_option[USE:cluster.set_instance_option]
\help cluster.set_instance_option

#@ cluster.set_option
c.help('set_option')

#@ global ? for set_option[USE:cluster.set_option]
\? cluster.set_option

#@ global help for set_option[USE:cluster.set_option]
\help cluster.set_option

#@ cluster.setup_admin_account
c.help('setup_admin_account')

#@ global ? for setup_admin_account[USE:cluster.setup_admin_account]
\? cluster.setup_admin_account

#@ global help for setup_admin_account[USE:cluster.setup_admin_account]
\help cluster.setup_admin_account

#@ cluster.setup_router_account
c.help('setup_router_account')

#@ global ? for setup_router_account[USE:cluster.setup_router_account]
\? cluster.setup_router_account

#@ global help for setup_router_account[USE:cluster.setup_router_account]
\help cluster.setup_router_account

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

#@ cluster.list_routers
c.help('list_routers')

#@ global ? for cluster.list_routers[USE:cluster.list_routers]
\? cluster.list_routers

#@ cluster.remove_router_metadata
c.help('remove_router_metadata')

#@ global ? for cluster.remove_router_metadata[USE:cluster.remove_router_metadata]
\? cluster.remove_router_metadata

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

#@ cluster.reset_recovery_accounts_password
c.help('reset_recovery_accounts_password')

#@ global ? for reset_recovery_accounts_password[USE:cluster.reset_recovery_accounts_password]
\? cluster.reset_recovery_accounts_password

#@ global help for reset_recovery_accounts_password[USE:cluster.reset_recovery_accounts_password]
\help cluster.reset_recovery_accounts_password

#@ cluster.create_cluster_set
c.help('create_cluster_set')

#@ global ? for create_cluster_set[USE:cluster.create_cluster_set]
\? cluster.create_cluster_set

#@ global help for create_cluster_set[USE:cluster.create_cluster_set]
\help cluster.create_cluster_set

#@ cluster.get_cluster_set
c.help('get_cluster_set')

#@ global ? for get_cluster_set[USE:cluster.get_cluster_set]
\? cluster.get_cluster_set

#@ global help for get_cluster_set[USE:cluster.get_cluster_set]
\help cluster.get_cluster_set

c.disconnect()

testutil.destroy_sandbox(__mysql_sandbox_port1);
