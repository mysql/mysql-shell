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

//@ WL#12049: Dissolve cluster 2 {VER(>=8.0.12)}
||

//@ WL#12049: Initialize new instance
||

//@ WL#12049: Create cluster 3 {VER(>=8.0.12)}
||

//@ WL#12049: Add instance without using exitStateAction {VER(>=8.0.12)}
||

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

//@ WL#11032: Dissolve cluster 2 {VER(>=8.0.11)}
||

//@ WL#11032: Initialize new instance
||

//@ WL#11032: Create cluster 3 {VER(>=8.0.11)}
||

//@ WL#11032: Add instance without using memberWeight {VER(>=8.0.11)}
||

//@ WL#11032: Finalization
||

//@ BUG#27084767: Initialize new instances
||

//@ BUG#27084767: Create a cluster in single-primary mode
||

//@ BUG#27084767: Add instance to cluster in single-primary mode
||

//@ BUG#27084767: Dissolve the cluster
||

//@ BUG#27084767: Create a cluster in multi-primary mode
||

//@ BUG#27084767: Add instance to cluster in multi-primary mode
||

//@ BUG#27084767: Finalization
||

//@ BUG#27084767: Initialize new non-sandbox instance
||

//@ BUG#27084767: Create a cluster in single-primary mode non-sandbox
||

//@ BUG#27084767: Add instance to cluster in single-primary mode non-sandbox
||

//@ BUG#27084767: Dissolve the cluster non-sandbox
||

//@ BUG#27084767: Create a cluster in multi-primary mode non-sandbox
||

//@ BUG#27084767: Add instance to cluster in multi-primary mode non-sandbox
||

//@ BUG#27084767: Finalization non-sandbox
||

//@ BUG#27677227 cluster with x protocol disabled setup
||

//@<OUT> BUG#27677227 cluster with x protocol disabled, mysqlx should be NULL
<<<hostname>>>:<<<__mysql_sandbox_port1>>> = {"grLocal": "<<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>", "mysqlClassic": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"}
<<<hostname>>>:<<<__mysql_sandbox_port2>>> = {"grLocal": "<<<hostname>>>:<<<__mysql_sandbox_gr_port2>>>", "mysqlClassic": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>"}
<<<hostname>>>:<<<__mysql_sandbox_port3>>> = {"mysqlX": "<<<hostname>>>:<<<__mysql_sandbox_x_port3>>>", "grLocal": "<<<hostname>>>:<<<__mysql_sandbox_gr_port3>>>", "mysqlClassic": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>"}

//@ BUG#27677227 cluster with x protocol disabled cleanup
||

//@ BUG28056944 deploy sandboxes.
||

//@ BUG28056944 create cluster.
||

//@<OUT> BUG28056944 remove instance with wrong password and force = true.
ERROR: Unable to connect to instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>'. Please, verify connection credentials and make sure the instance is available.
NOTE: The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is not reachable and it will only be removed from the metadata. Please take any necessary actions to make sure that the instance will not rejoin the cluster if brought back online.

The instance will be removed from the InnoDB cluster. Depending on the instance
being the Seed or not, the Metadata session might become invalid. If so, please
start a new session to the Metadata Storage R/W instance.


The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was successfully removed from the cluster.

//@<OUT> BUG28056944 Error adding instance already in group but not in Metadata.
ERROR: Instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is part of the Group Replication group but is not in the metadata. Please use <Cluster>.rescan() to update the metadata.

//@<ERR> BUG28056944 Error adding instance already in group but not in Metadata.
Cluster.addInstance: Metadata inconsistent (RuntimeError)

//@ BUG28056944 clean-up.
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

//@<OUT> AddInstance async replication error
ERROR: Cannot add instance '<<<localhost>>>:<<<__mysql_sandbox_port2>>>' to the cluster because it has asynchronous (master-slave) replication configured and running. Please stop the slave threads by executing the query: 'STOP SLAVE;'

//@<ERR> AddInstance async replication error
Cluster.addInstance: The instance '<<<localhost>>>:<<<__mysql_sandbox_port2>>>' is running asynchronous (master-slave) replication. (RuntimeError)

//@ BUG#29305551: Finalization
||

//@ BUG#29809560: deploy sandboxes.
||

//@ BUG#29809560: set the same server id an all instances.
||

//@ BUG#29809560: create cluster.
||

//@<OUT> BUG#29809560: add instance fails because server_id is not unique.
ERROR: Cannot add instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' to the cluster because it has the same server ID of a member of the cluster. Please change the server ID of the instance to add: all members must have a unique server ID.

//@<ERR> BUG#29809560: add instance fails because server_id is not unique.
Cluster.addInstance: The server_id '666' is already used by instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'. (RuntimeError)

//@ BUG#29809560: clean-up.
||

// BUG#28855764: user created by shell expires with default_password_lifetime
//@ BUG#28855764: deploy sandboxes.
||

//@ BUG#28855764: create cluster.
||

//@ BUG#28855764: add instance an instance to the cluster
||

//@ BUG#28855764: get recovery user for instance 2.
||

//@ BUG#28855764: get recovery user for instance 1.
||

//@<OUT> BUG#28855764: Passwords for recovery users never expire (password_lifetime=0).
Number of accounts for '<<<recovery_user_1>>>': 1
Number of accounts for '<<<recovery_user_2>>>': 1

//@ BUG#28855764: clean-up.
||

//@ WL#12773: FR4 - The ipWhitelist shall not change the behavior defined by FR1
|mysql_innodb_cluster_11111, %|
|mysql_innodb_cluster_22222, %|
|mysql_innodb_cluster_33333, %|

//@<OUT> BUG#25503159: number of recovery users before executing addInstance().
Number of recovery user before addInstance(): 1

//@<ERR> BUG#25503159: add instance fail (using an invalid localaddress).
Cluster.addInstance: Group Replication failed to start: [[*]]
