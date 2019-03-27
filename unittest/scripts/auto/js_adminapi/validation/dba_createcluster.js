//@ WL#12049: Initialization
||

//@ WL#12049: Unsupported server version {VER(<5.7.24)}
||Option 'exitStateAction' not supported on target server version: '<<<__version>>>'

//@ WL#12049: Create cluster errors using exitStateAction option {VER(>=5.7.24)}
||Invalid value for exitStateAction, string value cannot be empty.
||Invalid value for exitStateAction, string value cannot be empty.
||Error starting cluster: Invalid value for exitStateAction, can't be set to the value of ':'
||Error starting cluster: Invalid value for exitStateAction, can't be set to the value of 'AB'
||Error starting cluster: Invalid value for exitStateAction, can't be set to the value of '10'

//@ WL#12049: Create cluster specifying a valid value for exitStateAction (ABORT_SERVER) {VER(>=5.7.24)}
||

//@ WL#12049: Dissolve cluster 1 {VER(>=5.7.24)}
||

//@ WL#12049: Create cluster specifying a valid value for exitStateAction (READ_ONLY) {VER(>=5.7.24)}
||

//@ WL#12049: Dissolve cluster 2 {VER(>=5.7.24)}
||

//@ WL#12049: Create cluster specifying a valid value for exitStateAction (1) {VER(>=5.7.24)}
||

//@ WL#12049: Dissolve cluster 3 {VER(>=5.7.24)}
||

//@ WL#12049: Create cluster specifying a valid value for exitStateAction (0) {VER(>=5.7.24)}
||

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
group_replication_local_address = <<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>
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
group_replication_local_address = <<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>
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

//@ WL#11032: Dissolve cluster 1 {VER(>=5.7.20)}
||

//@ WL#11032: Create cluster specifying a valid value for memberWeight (100) {VER(>=5.7.20)}
||

//@ WL#11032: Dissolve cluster 2 {VER(>=5.7.20)}
||

//@ WL#11032: Create cluster specifying a valid value for memberWeight (-50) {VER(>=5.7.20)}
||

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
group_replication_local_address = <<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>
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
group_replication_local_address = <<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>
group_replication_recovery_use_ssl = ON
group_replication_single_primary_mode = ON
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = ON

//@ WL#11032: Finalization
||

//@ WL#12067: Initialization
||

//@ WL#12067: TSF1_6 Unsupported server version {VER(<8.0.14)}
||Option 'consistency' not supported on target server version: '<<<__version>>>'

//@ WL#12067: Create cluster errors using consistency option {VER(>=8.0.14)}
||Invalid value for consistency, string value cannot be empty.
||Invalid value for consistency, string value cannot be empty.
||Error starting cluster: Invalid value for consistency, can't be set to the value of ':'
||Error starting cluster: Invalid value for consistency, can't be set to the value of 'AB'
||Error starting cluster: Invalid value for consistency, can't be set to the value of '10'
||Option 'consistency' is expected to be of type String, but is Integer (TypeError)
||Cannot use the failoverConsistency and consistency options simultaneously. The failoverConsistency option is deprecated, please use the consistency option instead. (ArgumentError)


//@ WL#12067: TSF1_1 Create cluster using BEFORE_ON_PRIMARY_FAILOVER as value for consistency {VER(>=8.0.14)}
||

//@ WL#12067: Dissolve cluster 1 {VER(>=8.0.14)}
||

//@ WL#12067: TSF1_2 Create cluster using EVENTUAL as value for consistency {VER(>=8.0.14)}
||

//@ WL#12067: Dissolve cluster 2 {VER(>=8.0.14)}
||

//@ WL#12067: TSF1_1 Create cluster using 1 as value for consistency {VER(>=8.0.14)}
||

//@ WL#12067: Dissolve cluster 3 {VER(>=8.0.14)}
||

//@ WL#12067: TSF1_2 Create cluster using 0 as value for consistency {VER(>=8.0.14)}
||

//@ WL#12067: Dissolve cluster 4 {VER(>=8.0.14)}
||

//@ WL#12067: TSF1_3 Create cluster using no value for consistency {VER(>=8.0.14)}
||

//@ WL#12067: Dissolve cluster 5 {VER(>=8.0.14)}
||

//@ WL#12067: TSF1_7 Create cluster using evenTual as value for consistency throws no exception (case insensitive) {VER(>=8.0.14)}
||

//@ WL#12067: Dissolve cluster 6 {VER(>=8.0.14)}
||

//@ WL#12067: TSF1_8 Create cluster using Before_ON_PriMary_FailoveR as value for consistency throws no exception (case insensitive) {VER(>=8.0.14)}
||

//@ WL#12067: Dissolve cluster 7 {VER(>=8.0.14)}
||

//@ WL#12067: Initialize new instance {VER(>=8.0.14)}
||

//@ WL#12067: Create cluster 2 {VER(>=8.0.14)}
||

//@ WL#12067: Finalization
||

//@ WL#12050: Initialization
||

//@ WL#12050: TSF1_5 Unsupported server version {VER(<8.0.13)}
||Option 'expelTimeout' not supported on target server version: '<<<__version>>>'

//@ WL#12050: Create cluster errors using expelTimeout option {VER(>=8.0.13)}
// TSF1_3, TSF1_4, TSF1_6
||Option 'expelTimeout' Integer expected, but value is String (TypeError)
||Option 'expelTimeout' Integer expected, but value is String (TypeError)
||Option 'expelTimeout' is expected to be of type Integer, but is Float (TypeError)
||Option 'expelTimeout' is expected to be of type Integer, but is Bool (TypeError)
||Invalid value for expelTimeout, integer value must be in the range: [0, 3600] (ArgumentError)
||Invalid value for expelTimeout, integer value must be in the range: [0, 3600] (ArgumentError)

//@ WL#12050: TSF1_1 Create cluster using 12 as value for expelTimeout {VER(>=8.0.13)}
||

//@ WL#12050: Dissolve cluster 1 {VER(>=8.0.13)}
||

//@ WL#12050: TSF1_2 Initialize new instance {VER(>=8.0.13)}
||

//@ WL#12050: TSF1_2 Create cluster using no value for expelTimeout, confirm it has the default value {VER(>=8.0.13)}
||

//@ WL#12050: Finalization
||

//@ BUG#25867733: Deploy instances (setting performance_schema value).
||

//@ BUG#25867733: createCluster error with performance_schema=off
|ERROR: Instance 'localhost:<<<__mysql_sandbox_port1>>>' has the performance_schema disabled (performance_schema=OFF). Instances must have the performance_schema enabled to for InnoDB Cluster usage.|performance_schema disabled on target instance. (RuntimeError)

//@ BUG#25867733: createCluster no error with performance_schema=on
||

//@ BUG#25867733: cluster.status() successful
||

//@ BUG#25867733: finalization
||

//@ BUG#29246110: Deploy instances, setting non supported host: 127.0.1.1.
||

//@<OUT> BUG#29246110: check instance error with non supported host.
This instance reports its own address as 127.0.1.1
ERROR: Cannot use host '127.0.1.1' for instance 'localhost:<<<__mysql_sandbox_port1>>>' because it resolves to an IP address (127.0.1.1) that does not match a real network interface, thus it is not supported by the Group Replication communication layer. Change your system settings and/or set the MySQL server 'report_host' variable to a hostname that resolves to a supported IP address.

//@<ERR> BUG#29246110: check instance error with non supported host.
Dba.checkInstanceConfiguration: Invalid host/IP '127.0.1.1' resolves to '127.0.1.1' which is not supported by Group Replication. (RuntimeError)

//@<OUT> BUG#29246110: createCluster error with non supported host.
This instance reports its own address as 127.0.1.1
ERROR: Cannot use host '127.0.1.1' for instance 'localhost:<<<__mysql_sandbox_port1>>>' because it resolves to an IP address (127.0.1.1) that does not match a real network interface, thus it is not supported by the Group Replication communication layer. Change your system settings and/or set the MySQL server 'report_host' variable to a hostname that resolves to a supported IP address.

//@<ERR> BUG#29246110: createCluster error with non supported host.
Dba.createCluster: Invalid host/IP '127.0.1.1' resolves to '127.0.1.1' which is not supported by Group Replication. (RuntimeError)

//@ BUG#29246110: create cluster succeed with supported host.
||

//@<OUT> BUG#29246110: add instance error with non supported host.
This instance reports its own address as 127.0.1.1
ERROR: Cannot use host '127.0.1.1' for instance 'localhost:<<<__mysql_sandbox_port1>>>' because it resolves to an IP address (127.0.1.1) that does not match a real network interface, thus it is not supported by the Group Replication communication layer. Change your system settings and/or set the MySQL server 'report_host' variable to a hostname that resolves to a supported IP address.

//@<ERR> BUG#29246110: add instance error with non supported host.
Cluster.addInstance: Invalid host/IP '127.0.1.1' resolves to '127.0.1.1' which is not supported by Group Replication. (RuntimeError)

//@ BUG#29246110: finalization
||

//@ WL#12066: Initialization {VER(>=8.0.16)}
||

//@ WL#12066: TSF1_4 Validate that an exception is thrown if the value specified is not an unsigned integer. {VER(>=8.0.16)}
||Option 'autoRejoinTries' UInteger expected, but Integer value is out of range (TypeError)

//@ WL#12066: TSF1_5 Validate that an exception is thrown if the value  is not in the range 0 to 2016. {VER(>=8.0.16)}
||ERROR: Error starting cluster: Invalid value for autoRejoinTries, can't be set to the value of '2017'

//@ WL#12066: TSF1_1 Validate that the functions [dba.]createCluster() and [cluster.]addInstance() support a new option named autoRejoinTries. {VER(>=8.0.16)}
|WARNING: The member will only proceed according to its exitStateAction if auto-rejoin fails (i.e. all retry attempts are exhausted).|

//@ WL#12066: Dissolve cluster {VER(>=8.0.16)}
||

//@ WL#12066: Finalization {VER(>=8.0.16)}
||

//@<OUT> BUG#29308037: Confirm that all replication users where removed
0

//@ BUG#29308037: Finalization
||

//@ BUG#29305551: Initialization
||

//@ Create cluster async replication OK
||

//@ BUG#29305551: Finalization
||
