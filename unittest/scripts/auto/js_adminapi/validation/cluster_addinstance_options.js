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

//@ WL#11032: Initialize new instance
||

//@ WL#11032: Create cluster 3 {VER(>=8.0.11)}
||

//@ WL#11032: Add instance without using memberWeight {VER(>=8.0.11)}
||

//@ WL#11032: Finalization
||

//@ WL#12049: Initialization
||

//@ WL#12049: Create cluster 1 {VER(>=5.7.24)}
||

//@ WL#12049: addInstance() errors using exitStateAction option {VER(>=5.7.24)}
||Invalid value for exitStateAction, string value cannot be empty.
||Invalid value for exitStateAction, string value cannot be empty.
||Variable 'group_replication_exit_state_action' can't be set to the value of ':'
||Variable 'group_replication_exit_state_action' can't be set to the value of 'AB'
||Variable 'group_replication_exit_state_action' can't be set to the value of '10'

//@ WL#12049: Add instance using a valid exitStateAction 1 {VER(>=5.7.24)}
||

//@ WL#12049: Dissolve cluster 1 {VER(>=5.7.24)}
||

//@ WL#12049: Create cluster 2 {VER(>=8.0.12)}
||

//@ WL#12049: Add instance using a valid exitStateAction 2 {VER(>=8.0.12)}
||

//@ WL#12049: Initialize new instance
||

//@ WL#12049: Create cluster 3 {VER(>=8.0.12)}
||

//@ WL#12049: Add instance without using exitStateAction {VER(>=8.0.12)}
||

//@ WL#12049: Finalization
||

//@ WL#12067: Initialization {VER(>=8.0.14)}
||

//@ WL#12067: TSF2_1 The value for group_replication_consistency must be the same on all cluster members (single-primary) {VER(>=8.0.14)}
||

//@ WL#12067: Dissolve cluster 1 {VER(>=8.0.14)}
||

//@ WL#12067: TSF2_2 The value for group_replication_consistency must be the same on all cluster members (multi-primary) {VER(>=8.0.14)}
||

//@ WL#12067: Finalization {VER(>=8.0.14)}
||

//@ WL#12050: Initialization {VER(>=8.0.13)}
||

//@ WL#12050: TSF2_1 The value for group_replication_member_expel_timeout must be the same on all cluster members (single-primary) {VER(>=8.0.13)}
||

//@ WL#12050: Dissolve cluster 1 {VER(>=8.0.13)}
||

//@ WL#12050: TSF2_2 The value for group_replication_member_expel_timeout must be the same on all cluster members (multi-primary) {VER(>=8.0.13)}
||

//@ WL#12050: Finalization {VER(>=8.0.13)}
||

//@ WL#12066: Initialization {VER(>=8.0.16)}
||

//@ WL#12066: TSF1_4 Validate that an exception is thrown if the value specified is not an unsigned integer. {VER(>=8.0.16)}
||Variable 'group_replication_autorejoin_tries' can't be set to the value of '-5' (RuntimeError)

//@ WL#12066: TSF1_5 Validate that an exception is thrown if the value  is not in the range 0 to 2016. {VER(>=8.0.16)}
||Variable 'group_replication_autorejoin_tries' can't be set to the value of '2017' (RuntimeError)

//@ WL#12066: TSF1_1, TSF6_1 Validate that the functions [dba.]createCluster() and [cluster.]addInstance() support a new option named autoRejoinTries. {VER(>=8.0.16)}
|WARNING: The member will only proceed according to its exitStateAction if auto-rejoin fails (i.e. all retry attempts are exhausted).|

//@WL#12066: TSF1_3, TSF1_6 Confirm group_replication_autorejoin_tries value was persisted {VER(>=8.0.16)}
||

//@ WL#12066: Finalization {VER(>=8.0.16)}
||