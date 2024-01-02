//@ WL#12049: Initialization
||

//@ WL#12049: Create cluster 1
||

//@ WL#12049: addInstance() errors using exitStateAction option
||Invalid value for exitStateAction, string value cannot be empty.
||Invalid value for exitStateAction, string value cannot be empty.
||Variable 'group_replication_exit_state_action' can't be set to the value of ':'
||Variable 'group_replication_exit_state_action' can't be set to the value of 'AB'
||Variable 'group_replication_exit_state_action' can't be set to the value of '10'

//@ WL#12049: Add instance using a valid exitStateAction 1
||

//@ WL#12049: Finalization cluster
||

//@ WL#12049: Finalization
||

//@ WL#11032: Initialization
||

//@ WL#11032: Create cluster 1
||

//@ WL#11032: addInstance() errors using memberWeight option
||Option 'memberWeight' Integer expected, but value is String (TypeError)
||Option 'memberWeight' is expected to be of type Integer, but is Bool (TypeError)
||Option 'memberWeight' Integer expected, but value is String (TypeError)
||Option 'memberWeight' is expected to be of type Integer, but is Float (TypeError)

//@ WL#11032: Add instance using a valid memberWeight (integer)
||

//@ WL#11032: Finalization
||
