//@ WL#12049: Initialization
||

//@ WL#12049: Create cluster 1 {VER(>=5.7.24)}
||

//@ WL#12049: addInstance() errors using exitStateAction option {VER(>=5.7.24)}
||Invalid value for exitStateAction, string value cannot be empty.
||Invalid value for exitStateAction, string value cannot be empty.
||Error joining instance to cluster: Invalid value for exitStateAction, can't be set to the value of ':'
||Error joining instance to cluster: Invalid value for exitStateAction, can't be set to the value of 'AB'
||Error joining instance to cluster: Invalid value for exitStateAction, can't be set to the value of '10'

//@ WL#12049: Add instance using a valid exitStateAction 1 {VER(>=5.7.24)}
||

//@ WL#12049: Dissolve cluster 1 {VER(>=5.7.24)}
||

//@ WL#12049: Create cluster 2 {VER(>=8.0.12)}
||

//@ WL#12049: Add instance using a valid exitStateAction 2 {VER(>=8.0.12)}
||

//@<OUT> WL#12049: exitStateAction must be persisted on mysql >= 8.0.12 {VER(>=8.0.12)}
group_replication_exit_state_action = READ_ONLY
group_replication_force_members =
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
group_replication_group_seeds = <<<localhost>>>:<<<__mysql_sandbox_gr_port1>>>
group_replication_local_address = <<<localhost>>>:<<<__mysql_sandbox_gr_port2>>>
group_replication_recovery_use_ssl = ON
group_replication_single_primary_mode = ON
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = ON

//@ WL#12049: Dissolve cluster 2 {VER(>=8.0.12)}
||

//@ WL#12049: Initialize new instance
||

//@ WL#12049: Create cluster 3 {VER(>=8.0.12)}
||

//@ WL#12049: Add instance without using exitStateAction {VER(>=8.0.12)}
||

//@<OUT> BUG#28701263: DEFAULT VALUE OF EXITSTATEACTION TOO DRASTIC {VER(>=8.0.12)}
group_replication_exit_state_action = READ_ONLY
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
group_replication_group_seeds = <<<localhost>>>:<<<__mysql_sandbox_gr_port1>>>
group_replication_local_address = <<<localhost>>>:<<<__mysql_sandbox_gr_port2>>>
group_replication_recovery_use_ssl = ON
group_replication_single_primary_mode = ON
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = ON

//@ WL#12049: Finalization
||

//@ WL#11032: Initialization
||

//@ WL#11032: Create cluster 1 {VER(>=5.7.20)}
||

//@ WL#11032: addInstance() errors using memberWeight option {VER(>=5.7.20)}
||Option 'memberWeight' Integer expected, but value is String (TypeError)
||Option 'memberWeight' is expected to be of type Integer, but is Bool (TypeError)
||Option 'memberWeight' Integer expected, but value is String (TypeError)
||Option 'memberWeight' is expected to be of type Integer, but is Float (TypeError)

//@ WL#11032: Add instance using a valid value for memberWeight (25) {VER(>=5.7.20)}
||

//@ WL#11032: Dissolve cluster 1 {VER(>=5.7.20)}
||

//@ WL#11032: Create cluster 2 {VER(>=8.0.11)}
||

//@ WL#11032: Add instance using a valid value for memberWeight (-50) {VER(>=8.0.11)}
||

//@<OUT> WL#11032: memberWeight must be persisted on mysql >= 8.0.11 {VER(>=8.0.12)}
group_replication_exit_state_action = READ_ONLY
group_replication_force_members =
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
group_replication_group_seeds = <<<localhost>>>:<<<__mysql_sandbox_gr_port1>>>
group_replication_local_address = <<<localhost>>>:<<<__mysql_sandbox_gr_port2>>>
group_replication_member_weight = 0
group_replication_recovery_use_ssl = ON
group_replication_single_primary_mode = ON
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = ON

//@ WL#11032: Dissolve cluster 2 {VER(>=8.0.11)}
||

//@ WL#11032: Initialize new instance
||

//@ WL#11032: Create cluster 3 {VER(>=8.0.11)}
||

//@ WL#11032: Add instance without using memberWeight {VER(>=8.0.11)}
||

//@<OUT> WL#11032: memberWeight must not be persisted on mysql >= 8.0.11 if not set {VER(>=8.0.12)}
group_replication_exit_state_action = READ_ONLY
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
group_replication_group_seeds = <<<localhost>>>:<<<__mysql_sandbox_gr_port1>>>
group_replication_local_address = <<<localhost>>>:<<<__mysql_sandbox_gr_port2>>>
group_replication_recovery_use_ssl = ON
group_replication_single_primary_mode = ON
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = ON

//@ WL#11032: Finalization
||

//@ BUG#27084767: Initialize new instances
||

//@ BUG#27084767: Create a cluster in single-primary mode
||

//@<OUT> BUG#27084767: Verify the values of auto_increment_% in the seed instance
auto_increment_increment = 1
auto_increment_offset = 2

//@ BUG#27084767: Add instance to cluster in single-primary mode
||

//@<OUT> BUG#27084767: Verify the values of auto_increment_%
auto_increment_increment = 1
auto_increment_offset = 2

//@ BUG#27084767: Dissolve the cluster
||

//@ BUG#27084767: Create a cluster in multi-primary mode
||

//@<OUT> BUG#27084767: Verify the values of auto_increment_% in the seed instance in multi-primary mode
auto_increment_increment = 7
auto_increment_offset = <<<__expected_auto_inc_offset>>>

//@ BUG#27084767: Add instance to cluster in multi-primary mode
||

//@<OUT> BUG#27084767: Verify the values of auto_increment_% multi-primary
auto_increment_increment = 7
auto_increment_offset = <<<__expected_auto_inc_offset>>>

//@ BUG#27084767: Finalization
||

//@ BUG#27084767: Initialize new non-sandbox instance
||

//@ BUG#27084767: Create a cluster in single-primary mode non-sandbox
||

//@<OUT> BUG#27084767: Verify the values of auto_increment_% in the seed instance non-sandbox
auto_increment_increment = 1
auto_increment_offset = 2

//@ BUG#27084767: Add instance to cluster in single-primary mode non-sandbox
||

//@<OUT> BUG#27084767: Verify the values of auto_increment_% non-sandbox
auto_increment_increment = 1
auto_increment_offset = 2

//@ BUG#27084767: Dissolve the cluster non-sandbox
||

//@ BUG#27084767: Create a cluster in multi-primary mode non-sandbox
||

//@<OUT> BUG#27084767: Verify the values of auto_increment_% in multi-primary non-sandbox
auto_increment_increment = 7
auto_increment_offset = <<<__expected_auto_inc_offset>>>

//@ BUG#27084767: Add instance to cluster in multi-primary mode non-sandbox
||

//@<OUT> BUG#27084767: Verify the values of auto_increment_% multi-primary non-sandbox
auto_increment_increment = 7
auto_increment_offset = <<<__expected_auto_inc_offset>>>

//@ BUG#27084767: Finalization non-sandbox
||

//@ BUG#27677227 cluster with x protocol disabled setup
||

//@<OUT> BUG#27677227 cluster with x protocol disabled, mysqlx should be NULL
<<<localhost>>>:<<<__mysql_sandbox_port1>>> = {"grLocal": "<<<localhost>>>:<<<__mysql_sandbox_gr_port1>>>", "mysqlClassic": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>"}
<<<localhost>>>:<<<__mysql_sandbox_port2>>> = {"grLocal": "<<<localhost>>>:<<<__mysql_sandbox_gr_port2>>>", "mysqlClassic": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>"}
<<<localhost>>>:<<<__mysql_sandbox_port3>>> = {"mysqlX": "<<<localhost>>>:<<<__mysql_sandbox_x_port3>>>", "grLocal": "<<<localhost>>>:<<<__mysql_sandbox_gr_port3>>>", "mysqlClassic": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>"}

//@ BUG#27677227 cluster with x protocol disabled cleanup
||

//@ WL#12067: Initialization {VER(>=8.0.14)}
||

//@ WL#12067: TSF2_1 The value for group_replication_consistency must be the same on all cluster members (single-primary) {VER(>=8.0.14)}
||

//@ WL#12067: TSF2_1 Confirm group_replication_consistency value is the same on all cluster members (single-primary) {VER(>=8.0.14)}
|BEFORE_ON_PRIMARY_FAILOVER|
|BEFORE_ON_PRIMARY_FAILOVER|

//@ WL#12067: TSF2_1 Confirm group_replication_consistency value was persisted (single-primary) {VER(>=8.0.14)}
|group_replication_consistency = BEFORE_ON_PRIMARY_FAILOVER|
|group_replication_consistency = BEFORE_ON_PRIMARY_FAILOVER|

//@ WL#12067: Dissolve cluster 1 {VER(>=8.0.14)}
||

//@ WL#12067: TSF2_2 The value for group_replication_consistency must be the same on all cluster members (multi-primary) {VER(>=8.0.14)}
||

//@ WL#12067: TSF2_2 Confirm group_replication_consistency value is the same on all cluster members (multi-primary) {VER(>=8.0.14)}
|BEFORE_ON_PRIMARY_FAILOVER|
|BEFORE_ON_PRIMARY_FAILOVER|

//@ WL#12067: TSF2_2 Confirm group_replication_consistency value was persisted (multi-primary) {VER(>=8.0.14)}
|group_replication_consistency = BEFORE_ON_PRIMARY_FAILOVER|
|group_replication_consistency = BEFORE_ON_PRIMARY_FAILOVER|

//@ WL#12067: Finalization {VER(>=8.0.14)}
||
