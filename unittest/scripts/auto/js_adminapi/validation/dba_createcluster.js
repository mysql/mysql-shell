//@ WL#12049: Initialization
||

//@ WL#12049: Unsupported server version {VER(<5.7.24)}
||Option 'exitStateAction' not available for target server version.

//@ WL#12049: Create cluster errors using exitStateAction option {VER(>=5.7.24)}
||Invalid value for exitStateAction, string value cannot be empty.
||Invalid value for exitStateAction, string value cannot be empty.
||Error starting cluster: Invalid value for exitStateAction, can't be set to the value of ':'
||Error starting cluster: Invalid value for exitStateAction, can't be set to the value of 'AB'
||Error starting cluster: Invalid value for exitStateAction, can't be set to the value of '10'

//@ WL#12049: Create cluster specifying a valid value for exitStateAction (ABORT_SERVER) {VER(>=5.7.24)}
||

//@<OUT> WL#12049: Confirm group_replication_exit_state_action is set correctly (ABORT_SERVER) {VER(>=5.7.24)}
ABORT_SERVER

//@ WL#12049: Dissolve cluster 1 {VER(>=5.7.24)}
||

//@ WL#12049: Create cluster specifying a valid value for exitStateAction (READ_ONLY) {VER(>=5.7.24)}
||

//@<OUT> WL#12049: Confirm group_replication_exit_state_action is set correctly (READ_ONLY) {VER(>=5.7.24)}
READ_ONLY

//@ WL#12049: Dissolve cluster 2 {VER(>=5.7.24)}
||

//@ WL#12049: Create cluster specifying a valid value for exitStateAction (1) {VER(>=5.7.24)}
||

//@<OUT> WL#12049: Confirm group_replication_exit_state_action is set correctly (1) {VER(>=5.7.24)}
ABORT_SERVER

//@ WL#12049: Dissolve cluster 3 {VER(>=5.7.24)}
||

//@ WL#12049: Create cluster specifying a valid value for exitStateAction (0) {VER(>=5.7.24)}
||

//@<OUT> WL#12049: Confirm group_replication_exit_state_action is set correctly (0) {VER(>=5.7.24)}
READ_ONLY

//@ WL#12049: Dissolve cluster 4 {VER(>=5.7.24)}
||

//@ WL#12049: Create cluster {VER(>=8.0.12)}
||

//@<OUT> WL#12049: exitStateAction must be persisted on mysql >= 8.0.12 {VER(>=8.0.12)}
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

//@ WL#12049: Dissolve cluster 6 {VER(>=8.0.12)}
||

//@ WL#12049: Initialize new instance
||

//@ WL#12049: Create cluster 2 {VER(>=8.0.12)}
||

//@<OUT> BUG#28701263: DEFAULT VALUE OF EXITSTATEACTION TOO DRASTIC {VER(>=8.0.12)}
group_replication_bootstrap_group = OFF
group_replication_exit_state_action = READ_ONLY
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
group_replication_group_seeds =
group_replication_local_address = <<<localhost>>>:<<<__mysql_sandbox_gr_port1>>>
group_replication_recovery_use_ssl = ON
group_replication_single_primary_mode = ON
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = ON


//@ WL#12049: Finalization
||

//@ WL#11032: Initialization
||

//@ WL#11032: Unsupported server version {VER(<5.7.20)}
||Option 'memberWeight' not available for target server version.

//@ WL#11032: Create cluster errors using memberWeight option {VER(>=5.7.20)}
||Option 'memberWeight' Integer expected, but value is String (TypeError)
||Option 'memberWeight' is expected to be of type Integer, but is Bool (TypeError)
||Option 'memberWeight' Integer expected, but value is String (TypeError)
||Option 'memberWeight' is expected to be of type Integer, but is Float (TypeError)

//@ WL#11032: Create cluster specifying a valid value for memberWeight (25) {VER(>=5.7.20)}
||

//@<OUT> WL#11032: Confirm group_replication_member_weight is set correctly (25) {VER(>=5.7.20)}
25

//@ WL#11032: Dissolve cluster 1 {VER(>=5.7.20)}
||

//@ WL#11032: Create cluster specifying a valid value for memberWeight (100) {VER(>=5.7.20)}
||

//@<OUT> WL#11032: Confirm group_replication_member_weight is set correctly (100) {VER(>=5.7.20)}
100

//@ WL#11032: Dissolve cluster 2 {VER(>=5.7.20)}
||

//@ WL#11032: Create cluster specifying a valid value for memberWeight (-50) {VER(>=5.7.20)}
||

//@<OUT> WL#11032: Confirm group_replication_member_weight is set correctly (0) {VER(>=5.7.20)}
0

//@ WL#11032: Dissolve cluster 3 {VER(>=5.7.20)}
||

//@ WL#11032: Create cluster {VER(>=8.0.11)}
||

//@<OUT> WL#11032: memberWeight must be persisted on mysql >= 8.0.11 {VER(>=8.0.12)}
group_replication_bootstrap_group = OFF
group_replication_exit_state_action = READ_ONLY
group_replication_force_members =
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
group_replication_group_seeds =
group_replication_local_address = <<<localhost>>>:<<<__mysql_sandbox_gr_port1>>>
group_replication_member_weight = 75
group_replication_recovery_use_ssl = ON
group_replication_single_primary_mode = ON
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = ON

//@ WL#11032: Dissolve cluster 6 {VER(>=8.0.11)}
||

//@ WL#11032: Initialize new instance
||

//@ WL#11032: Create cluster 2 {VER(>=8.0.11)}
||

//@<OUT> WL#11032: memberWeight must not be persisted on mysql >= 8.0.11 if not set {VER(>=8.0.12)}
group_replication_bootstrap_group = OFF
group_replication_exit_state_action = READ_ONLY
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
group_replication_group_seeds =
group_replication_local_address = <<<localhost>>>:<<<__mysql_sandbox_gr_port1>>>
group_replication_recovery_use_ssl = ON
group_replication_single_primary_mode = ON
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = ON

//@ WL#11032: Finalization
||
