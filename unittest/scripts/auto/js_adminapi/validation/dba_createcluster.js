//@ Initialization
||

//@ Unsupported server version {VER(<5.7.24)}
||Option 'exitStateAction' not available for target server version.

//@ Create cluster errors using exitStateAction option {VER(>=5.7.24)}
||Invalid value for exitStateAction, string value cannot be empty.
||Invalid value for exitStateAction, string value cannot be empty.
||Error starting cluster: Invalid value for exitStateAction, can't be set to the value of ':'
||Error starting cluster: Invalid value for exitStateAction, can't be set to the value of 'AB'
||Error starting cluster: Invalid value for exitStateAction, can't be set to the value of '10'

//@ Create cluster specifying a valid value for exitStateAction (ABORT_SERVER) {VER(>=5.7.24)}
||

//@<OUT> Confirm group_replication_exit_state_action is set correctly (ABORT_SERVER) {VER(>=5.7.24)}
ABORT_SERVER

//@ Dissolve cluster 1 {VER(>=5.7.24)}
||

//@ Create cluster specifying a valid value for exitStateAction (READ_ONLY) {VER(>=5.7.24)}
||

//@<OUT> Confirm group_replication_exit_state_action is set correctly (READ_ONLY) {VER(>=5.7.24)}
READ_ONLY

//@ Dissolve cluster 2 {VER(>=5.7.24)}
||

//@ Create cluster specifying a valid value for exitStateAction (1) {VER(>=5.7.24)}
||

//@<OUT> Confirm group_replication_exit_state_action is set correctly (1) {VER(>=5.7.24)}
ABORT_SERVER

//@ Dissolve cluster 3 {VER(>=5.7.24)}
||

//@ Create cluster specifying a valid value for exitStateAction (0) {VER(>=5.7.24)}
||

//@<OUT> Confirm group_replication_exit_state_action is set correctly (0) {VER(>=5.7.24)}
READ_ONLY

//@ Dissolve cluster 4 {VER(>=5.7.24)}
||

//@ Create cluster {VER(>=8.0.11)}
||

//@<OUT> exitStateAction must be persisted on mysql >= 8.0.11 {VER(>=8.0.11)}
group_replication_bootstrap_group = OFF
group_replication_exit_state_action = READ_ONLY
group_replication_force_members =
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
group_replication_group_seeds =
group_replication_local_address = <<<localhost>>>:<<<__mysql_sandbox_gr_port1>>>
group_replication_recovery_use_ssl = ON
group_replication_single_primary_mode = ON
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = ON

//@ Dissolve cluster 6 {VER(>=8.0.11)}
||

//@ Initialize new instance
||

//@ Create cluster 2 {VER(>=8.0.11)}
||

//@<OUT> exitStateAction must not be persisted on mysql >= 8.0.11 if not set {VER(>=8.0.11)}
group_replication_bootstrap_group = OFF
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
group_replication_group_seeds =
group_replication_local_address = <<<localhost>>>:<<<__mysql_sandbox_gr_port1>>>
group_replication_recovery_use_ssl = ON
group_replication_single_primary_mode = ON
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = ON


//@ Finalization
||
