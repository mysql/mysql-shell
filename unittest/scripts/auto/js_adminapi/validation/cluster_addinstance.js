//@ Initialization
||

//@ Create cluster 1 {VER(>=5.7.24)}
||

//@ addInstance() errors using exitStateAction option {VER(>=5.7.24)}
||Invalid value for exitStateAction, string value cannot be empty.
||Invalid value for exitStateAction, string value cannot be empty.
||Error joining instance to cluster: Invalid value for exitStateAction, can't be set to the value of ':'
||Error joining instance to cluster: Invalid value for exitStateAction, can't be set to the value of 'AB'
||Error joining instance to cluster: Invalid value for exitStateAction, can't be set to the value of '10'

//@ Add instance using a valid exitStateAction 1 {VER(>=5.7.24)}
||

//@ Dissolve cluster 1 {VER(>=5.7.24)}
||

//@ Create cluster 2 {VER(>=8.0.11)}
||

//@ Add instance using a valid exitStateAction 2 {VER(>=8.0.11)}
||

//@<OUT> exitStateAction must be persisted on mysql >= 8.0.11 {VER(>=8.0.11)}
group_replication_exit_state_action = READ_ONLY
group_replication_force_members =
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
group_replication_group_seeds = <<<localhost>>>:<<<__mysql_sandbox_gr_port1>>>
group_replication_local_address = <<<localhost>>>:<<<__mysql_sandbox_gr_port2>>>
group_replication_recovery_use_ssl = ON
group_replication_single_primary_mode = ON
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = ON

//@ Dissolve cluster 2 {VER(>=8.0.11)}
||

//@ Initialize new instance
||

//@ Create cluster 3 {VER(>=8.0.11)}
||

//@ Add instance without using exitStateAction {VER(>=8.0.11)}
||

//@<OUT> exitStateAction must not be persisted on mysql >= 8.0.11 if not set {VER(>=8.0.11)}
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
group_replication_group_seeds = <<<localhost>>>:<<<__mysql_sandbox_gr_port1>>>
group_replication_local_address = <<<localhost>>>:<<<__mysql_sandbox_gr_port2>>>
group_replication_recovery_use_ssl = ON
group_replication_single_primary_mode = ON
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = ON

//@ Finalization
||
