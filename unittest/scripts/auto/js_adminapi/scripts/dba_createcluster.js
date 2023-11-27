// Assumptions: smart deployment functions available

var number_of_rpl_users_query = "SELECT COUNT(*) FROM INFORMATION_SCHEMA.USER_PRIVILEGES WHERE GRANTEE REGEXP \"mysql_innodb_cluster_[0-9]+\"";

function gtid_contains_gr_group_name(session) {
    // Get GR group_name.
    var res = session.runSql("SELECT @@GLOBAL.group_replication_group_name");
    var row = res.fetchOne();
    var group_name = row[0];

    // Get GTIDs (executed) set.
    res = session.runSql("SELECT @@GLOBAL.gtid_executed");
    var row = res.fetchOne();
    var gtids = row[0];

    return gtids.search(group_name) >= 0;
}

function gtid_contains_server_uuid(session) {
    // Get server UUID;
    res = session.runSql("SELECT @@GLOBAL.server_uuid");
    row = res.fetchOne();
    var uuid = row[0];

    // Get GTIDs (executed) set.
    res = session.runSql("SELECT @@GLOBAL.gtid_executed");
    var row = res.fetchOne();
    var gtids = row[0];

    return gtids.search(uuid) >= 0;
}

function print_metadata_instance_addresses(session) {
    var res = session.runSql("select * from mysql_innodb_cluster_metadata.instances");
    var row = res.fetchOneObject()
    while(row) {
        print(row.instance_name + " = " + row.addresses + "\n");
        row = res.fetchOneObject();
    }
    print("\n");
}

//@<> init
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.snapshotSandboxConf(__mysql_sandbox_port2);

shell.connect(__sandbox_uri1);

// WL#12011: AdminAPI: Refactor dba.createCluster()
//@<> WL#12011: FR2-04 - invalid value for interactive option.
EXPECT_THROWS_TYPE(function() { dba.createCluster('test', {interactive: "not a valid type"}) }, "Argument #2: Option 'interactive' Bool expected, but value is String", "TypeError");

//@<> WL#12011: FR2-01 - interactive = true.
testutil.expectPrompt("Confirm [y/N]:", "n");

EXPECT_THROWS_TYPE(function() { dba.createCluster('test', {interactive: true, multiPrimary: true}) }, "Cancelled", "RuntimeError");

EXPECT_STDOUT_CONTAINS_MULTILINE(`A new InnoDB Cluster will be created on instance '${hostname}:${__mysql_sandbox_port1}'.

The MySQL InnoDB Cluster is going to be setup in advanced Multi-Primary Mode.
Before continuing you have to confirm that you understand the requirements and
limitations of Multi-Primary Mode. For more information see
https://dev.mysql.com/doc/refman/en/group-replication-limitations.html before
proceeding.

I have read the MySQL InnoDB Cluster manual and I understand the requirements
and limitations of advanced Multi-Primary Mode.
`);

//@<> WL#12011: FR2-03 - no interactive option (default: non-interactive).
WIPE_SHELL_LOG();

var c;
EXPECT_NO_THROWS(function() { c = dba.createCluster('test', {multiPrimary: true, force: true}); });

if (__version_num < 80011) {
 EXPECT_STDOUT_CONTAINS_MULTILINE(`A new InnoDB Cluster will be created on instance '${hostname}:${__mysql_sandbox_port1}'.

Validating instance configuration at localhost:${__mysql_sandbox_port1}...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as ${hostname}:${__mysql_sandbox_port1}

Instance configuration is suitable.
NOTE: Group Replication will communicate with other members using '${hostname}:${__mysql_sandbox_gr_port1}'. Use the localAddress option to override.

NOTE: The 'localAddress' "${hostname}" is [[*]], which is compatible with the Group Replication automatically generated list of IPs.
See https://dev.mysql.com/doc/refman/en/group-replication-ip-address-permissions.html for more details.
NOTE: When adding more instances to the Cluster, be aware that the subnet masks dictate whether the instance's address is automatically added to the allowlist or not. Please specify the 'ipAllowlist' accordingly if needed.
* Checking connectivity and SSL configuration...

WARNING: Instance '${hostname}:${__mysql_sandbox_port1}' cannot persist Group Replication configuration since MySQL version ${__version} does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.
Creating InnoDB Cluster 'test' on '${hostname}:${__mysql_sandbox_port1}'...

The MySQL InnoDB Cluster is going to be setup in advanced Multi-Primary Mode. Consult its requirements and limitations in https://dev.mysql.com/doc/refman/en/group-replication-limitations.html
Adding Seed Instance...
Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.`);
} else if (__version_num > 80011 && __version_num < 80027) {
EXPECT_STDOUT_CONTAINS_MULTILINE(`A new InnoDB Cluster will be created on instance '${hostname}:${__mysql_sandbox_port1}'.

Validating instance configuration at localhost:${__mysql_sandbox_port1}...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as ${hostname}:${__mysql_sandbox_port1}

Instance configuration is suitable.
NOTE: Group Replication will communicate with other members using '${hostname}:${__mysql_sandbox_gr_port1}'. Use the localAddress option to override.

NOTE: The 'localAddress' "${hostname}" is [[*]], which is compatible with the Group Replication automatically generated list of IPs.
See https://dev.mysql.com/doc/refman/en/group-replication-ip-address-permissions.html for more details.
NOTE: When adding more instances to the Cluster, be aware that the subnet masks dictate whether the instance's address is automatically added to the allowlist or not. Please specify the 'ipAllowlist' accordingly if needed.
* Checking connectivity and SSL configuration...

Creating InnoDB Cluster 'test' on '${hostname}:${__mysql_sandbox_port1}'...

The MySQL InnoDB Cluster is going to be setup in advanced Multi-Primary Mode. Consult its requirements and limitations in https://dev.mysql.com/doc/refman/en/group-replication-limitations.html
Adding Seed Instance...
Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.`);
} else {
EXPECT_STDOUT_CONTAINS_MULTILINE(
`A new InnoDB Cluster will be created on instance '${hostname}:${__mysql_sandbox_port1}'.

Validating instance configuration at localhost:${__mysql_sandbox_port1}...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as ${hostname}:${__mysql_sandbox_port1}

Instance configuration is suitable.
NOTE: Group Replication will communicate with other members using '${hostname}:${__mysql_sandbox_port1}'. Use the localAddress option to override.

* Checking connectivity and SSL configuration...

Creating InnoDB Cluster 'test' on '${hostname}:${__mysql_sandbox_port1}'...

The MySQL InnoDB Cluster is going to be setup in advanced Multi-Primary Mode. Consult its requirements and limitations in https://dev.mysql.com/doc/refman/en/group-replication-limitations.html
Adding Seed Instance...
Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.`);
}

// check that some vars don't have it's default values set
if (__version_num >= 80002) {
    EXPECT_SHELL_LOG_CONTAINS("Ignoring 'group_replication_recovery_ssl_verify_server_cert': default value ('false') is the expected.");
    EXPECT_SHELL_LOG_CONTAINS("Ignoring 'group_replication_start_on_boot': default value ('true') is the expected.");
}

//@<> Check if dba.configureLocalInstance() only sets vars that don't have it's default values {VER(<8.0.2)}
WIPE_SHELL_LOG();

EXPECT_NO_THROWS(function(){ dba.configureLocalInstance(); });

EXPECT_SHELL_LOG_CONTAINS("Ignoring 'group_replication_allow_local_disjoint_gtids_join': default value ('OFF') is the expected.");
EXPECT_SHELL_LOG_CONTAINS("Ignoring 'group_replication_allow_local_lower_version_join': default value ('OFF') is the expected.");
EXPECT_SHELL_LOG_CONTAINS("Ignoring 'group_replication_bootstrap_group': default value ('OFF') is the expected.");
EXPECT_SHELL_LOG_CONTAINS("Ignoring 'group_replication_components_stop_timeout': default value ('31536000') is the expected.");
EXPECT_SHELL_LOG_CONTAINS("Ignoring 'group_replication_exit_state_action': default value ('READ_ONLY') is the expected.");
EXPECT_SHELL_LOG_CONTAINS("Ignoring 'group_replication_flow_control_applier_threshold': default value ('25000') is the expected.");
EXPECT_SHELL_LOG_CONTAINS("Ignoring 'group_replication_flow_control_certifier_threshold': default value ('25000') is the expected.");
EXPECT_SHELL_LOG_CONTAINS("Ignoring 'group_replication_flow_control_mode': default value ('QUOTA') is the expected.");
EXPECT_SHELL_LOG_CONTAINS("Ignoring 'group_replication_force_members': default value ('') is the expected.");
EXPECT_SHELL_LOG_CONTAINS("Ignoring 'group_replication_group_seeds': default value ('') is the expected.");
EXPECT_SHELL_LOG_CONTAINS("Ignoring 'group_replication_member_weight': default value ('50') is the expected.");
EXPECT_SHELL_LOG_CONTAINS("Ignoring 'group_replication_poll_spin_loops': default value ('0') is the expected.");
EXPECT_SHELL_LOG_CONTAINS("Ignoring 'group_replication_recovery_complete_at': default value ('TRANSACTIONS_APPLIED') is the expected.");
EXPECT_SHELL_LOG_CONTAINS("Ignoring 'group_replication_recovery_reconnect_interval': default value ('60') is the expected.");
EXPECT_SHELL_LOG_CONTAINS("Ignoring 'group_replication_recovery_retry_count': default value ('10') is the expected.");
EXPECT_SHELL_LOG_CONTAINS("Ignoring 'group_replication_recovery_ssl_ca': default value ('') is the expected.");
EXPECT_SHELL_LOG_CONTAINS("Ignoring 'group_replication_recovery_ssl_capath': default value ('') is the expected.");
EXPECT_SHELL_LOG_CONTAINS("Ignoring 'group_replication_recovery_ssl_cert': default value ('') is the expected.");
EXPECT_SHELL_LOG_CONTAINS("Ignoring 'group_replication_recovery_ssl_cipher': default value ('') is the expected.");
EXPECT_SHELL_LOG_CONTAINS("Ignoring 'group_replication_recovery_ssl_crl': default value ('') is the expected.");
EXPECT_SHELL_LOG_CONTAINS("Ignoring 'group_replication_recovery_ssl_crlpath': default value ('') is the expected.");
EXPECT_SHELL_LOG_CONTAINS("Ignoring 'group_replication_recovery_ssl_key': default value ('') is the expected.");
EXPECT_SHELL_LOG_CONTAINS("Ignoring 'group_replication_recovery_ssl_verify_server_cert': default value ('OFF') is the expected.");
EXPECT_SHELL_LOG_CONTAINS("Ignoring 'group_replication_start_on_boot': default value ('ON') is the expected.");
EXPECT_SHELL_LOG_CONTAINS("Ignoring 'group_replication_transaction_size_limit': default value ('150000000') is the expected.");
EXPECT_SHELL_LOG_CONTAINS("Ignoring 'group_replication_unreachable_majority_timeout': default value ('0') is the expected.");

// some vars (group_replication_group_seeds) are persisted in other places, by different reasons, so we can't check them
EXPECT_THROWS(function(){ testutil.getSandboxConf(__mysql_sandbox_port1, "group_replication_allow_local_disjoint_gtids_join"); }, "Option 'group_replication_allow_local_disjoint_gtids_join' does not exist in group 'mysqld'");
EXPECT_THROWS(function(){ testutil.getSandboxConf(__mysql_sandbox_port1, "group_replication_allow_local_lower_version_join"); }, "Option 'group_replication_allow_local_lower_version_join' does not exist in group 'mysqld'");
EXPECT_THROWS(function(){ testutil.getSandboxConf(__mysql_sandbox_port1, "group_replication_bootstrap_group"); }, "Option 'group_replication_bootstrap_group' does not exist in group 'mysqld'");
EXPECT_THROWS(function(){ testutil.getSandboxConf(__mysql_sandbox_port1, "group_replication_components_stop_timeout"); }, "Option 'group_replication_components_stop_timeout' does not exist in group 'mysqld'");
EXPECT_THROWS(function(){ testutil.getSandboxConf(__mysql_sandbox_port1, "group_replication_exit_state_action"); }, "Option 'group_replication_exit_state_action' does not exist in group 'mysqld'");
EXPECT_THROWS(function(){ testutil.getSandboxConf(__mysql_sandbox_port1, "group_replication_flow_control_applier_threshold"); }, "Option 'group_replication_flow_control_applier_threshold' does not exist in group 'mysqld'");
EXPECT_THROWS(function(){ testutil.getSandboxConf(__mysql_sandbox_port1, "group_replication_flow_control_certifier_threshold"); }, "Option 'group_replication_flow_control_certifier_threshold' does not exist in group 'mysqld'");
EXPECT_THROWS(function(){ testutil.getSandboxConf(__mysql_sandbox_port1, "group_replication_flow_control_mode"); }, "Option 'group_replication_flow_control_mode' does not exist in group 'mysqld'");
EXPECT_THROWS(function(){ testutil.getSandboxConf(__mysql_sandbox_port1, "group_replication_force_members"); }, "Option 'group_replication_force_members' does not exist in group 'mysqld'");
EXPECT_THROWS(function(){ testutil.getSandboxConf(__mysql_sandbox_port1, "group_replication_member_weight"); }, "Option 'group_replication_member_weight' does not exist in group 'mysqld'");
EXPECT_THROWS(function(){ testutil.getSandboxConf(__mysql_sandbox_port1, "group_replication_poll_spin_loops"); }, "Option 'group_replication_poll_spin_loops' does not exist in group 'mysqld'");
EXPECT_THROWS(function(){ testutil.getSandboxConf(__mysql_sandbox_port1, "group_replication_recovery_complete_at"); }, "Option 'group_replication_recovery_complete_at' does not exist in group 'mysqld'");
EXPECT_THROWS(function(){ testutil.getSandboxConf(__mysql_sandbox_port1, "group_replication_recovery_reconnect_interval"); }, "Option 'group_replication_recovery_reconnect_interval' does not exist in group 'mysqld'");
EXPECT_THROWS(function(){ testutil.getSandboxConf(__mysql_sandbox_port1, "group_replication_recovery_retry_count"); }, "Option 'group_replication_recovery_retry_count' does not exist in group 'mysqld'");
EXPECT_THROWS(function(){ testutil.getSandboxConf(__mysql_sandbox_port1, "group_replication_recovery_ssl_ca"); }, "Option 'group_replication_recovery_ssl_ca' does not exist in group 'mysqld'");
EXPECT_THROWS(function(){ testutil.getSandboxConf(__mysql_sandbox_port1, "group_replication_recovery_ssl_capath"); }, "Option 'group_replication_recovery_ssl_capath' does not exist in group 'mysqld'");
EXPECT_THROWS(function(){ testutil.getSandboxConf(__mysql_sandbox_port1, "group_replication_recovery_ssl_cert"); }, "Option 'group_replication_recovery_ssl_cert' does not exist in group 'mysqld'");
EXPECT_THROWS(function(){ testutil.getSandboxConf(__mysql_sandbox_port1, "group_replication_recovery_ssl_cipher"); }, "Option 'group_replication_recovery_ssl_cipher' does not exist in group 'mysqld'");
EXPECT_THROWS(function(){ testutil.getSandboxConf(__mysql_sandbox_port1, "group_replication_recovery_ssl_crl"); }, "Option 'group_replication_recovery_ssl_crl' does not exist in group 'mysqld'");
EXPECT_THROWS(function(){ testutil.getSandboxConf(__mysql_sandbox_port1, "group_replication_recovery_ssl_crlpath"); }, "Option 'group_replication_recovery_ssl_crlpath' does not exist in group 'mysqld'");
EXPECT_THROWS(function(){ testutil.getSandboxConf(__mysql_sandbox_port1, "group_replication_recovery_ssl_key"); }, "Option 'group_replication_recovery_ssl_key' does not exist in group 'mysqld'");
EXPECT_THROWS(function(){ testutil.getSandboxConf(__mysql_sandbox_port1, "group_replication_recovery_ssl_verify_server_cert"); }, "Option 'group_replication_recovery_ssl_verify_server_cert' does not exist in group 'mysqld'");
EXPECT_THROWS(function(){ testutil.getSandboxConf(__mysql_sandbox_port1, "group_replication_start_on_boot"); }, "Option 'group_replication_start_on_boot' does not exist in group 'mysqld'");
EXPECT_THROWS(function(){ testutil.getSandboxConf(__mysql_sandbox_port1, "group_replication_transaction_size_limit"); }, "Option 'group_replication_transaction_size_limit' does not exist in group 'mysqld'");
EXPECT_THROWS(function(){ testutil.getSandboxConf(__mysql_sandbox_port1, "group_replication_unreachable_majority_timeout"); }, "Option 'group_replication_unreachable_majority_timeout' does not exist in group 'mysqld'");

//@<> Create cluster without group_replication_start_on_boot
reset_instance(session);

session.runSql("set global group_replication_start_on_boot=1");
var start_on_boot = session.runSql("select @@group_replication_start_on_boot").fetchOne()[0];
EXPECT_EQ(1, start_on_boot);

// this also ensures the ensure_updates_everywhere_checks option
// is updated correctly when we're switching from multi-primary to single-primary
session.runSql("set global group_replication_single_primary_mode=0");
session.runSql("set global group_replication_enforce_update_everywhere_checks=1");

c = dba.createCluster("test", {manualStartOnBoot:true});
start_on_boot = session.runSql("select @@group_replication_start_on_boot").fetchOne()[0];
EXPECT_EQ(0, start_on_boot);

c.addInstance(__sandbox_uri2, {recoveryMethod:"incremental"});
session2 = mysql.getSession(__sandbox_uri2);
start_on_boot = session2.runSql("select @@group_replication_start_on_boot").fetchOne()[0];
EXPECT_EQ(0, start_on_boot);

c.dissolve();

//@<> check error log dumping on create if supported {VER(>=8.0.22)}

// force GR start to fail by passing an invalid localAddress
EXPECT_THROWS(function(){
    dba.createCluster("test", {communicationStack: "XCOM", localAddress:"0.0.0.0:1"});
}, "The 'localAddress' isn't compatible with the Group Replication automatically generated list of allowed IPs.");

EXPECT_OUTPUT_CONTAINS(`The 'localAddress' "0.0.0.0" isn't compatible with the Group Replication automatically generated list of allowed IPs.`);
if (__os_type != 'windows') {
    EXPECT_OUTPUT_CONTAINS("In this scenario, it's necessary to explicitly use the 'ipAllowlist' option to manually specify the list of allowed IPs.");
}
EXPECT_OUTPUT_CONTAINS("See https://dev.mysql.com/doc/refman/en/group-replication-ip-address-permissions.html for more details.");

//@<> check error log dumping on add if supported {VER(>=8.0.22)}
var c = dba.createCluster("test", {communicationStack: "XCOM"});

EXPECT_THROWS(function(){
    c.addInstance(__sandbox_uri2, {localAddress:"0.0.0.0:1", recoveryMethod:"incremental"});
}, "The 'localAddress' isn't compatible with the Group Replication automatically generated list of allowed IPs.");

EXPECT_OUTPUT_CONTAINS(`The 'localAddress' "0.0.0.0" isn't compatible with the Group Replication automatically generated list of allowed IPs.`);
if (__os_type != 'windows') {
    EXPECT_OUTPUT_CONTAINS("In this scenario, it's necessary to explicitly use the 'ipAllowlist' option to manually specify the list of allowed IPs.");
}
EXPECT_OUTPUT_CONTAINS("See https://dev.mysql.com/doc/refman/en/group-replication-ip-address-permissions.html for more details.");

//@<> WL#12011: Finalization.
c.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

// WL#12049 AdminAPI: option to shutdown server when dropping out of the
// cluster
//
// In 8.0.12 and 5.7.24, Group Replication introduced an option to allow
// defining the behaviour of a cluster member whenever it drops the cluster
// unintentionally, i.e. not manually removed by the user (DBA). The
// behaviour defined can be either automatically shutting itself down or
// switching itself to super-read-only (current behaviour). The option is:
// group_replication_exit_state_action.
//
// In order to support defining such option, the AdminAPI was extended by
// introducing a new optional parameter, named 'exitStateAction', in the
// following functions:
//
// - dba.createCluster()
// - Cluster.addInstance()
//

//@<> WL#12049: Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});

shell.connect(__sandbox_uri1);

//@ WL#12049: Unsupported server version {VER(<5.7.24)}
var c = dba.createCluster('test', {exitStateAction: "READ_ONLY", gtidSetIsComplete: true});

//@ WL#12049: Create cluster errors using exitStateAction option {VER(>=5.7.24)}
// F1.2 - The exitStateAction option shall be a string value.
// NOTE: GR validates the value, which is an Enumerator, and accepts the values
// `ABORT_SERVER` or `READ_ONLY`, or 1 or 0.
var c = dba.createCluster('test', {exitStateAction: ""});

var c = dba.createCluster('test', {exitStateAction: " "});

var c = dba.createCluster('test', {exitStateAction: ":"});

var c = dba.createCluster('test', {exitStateAction: "AB"});

var c = dba.createCluster('test', {exitStateAction: "10"});

//@ WL#12049: Create cluster specifying a valid value for exitStateAction (ABORT_SERVER) {VER(>=5.7.24)}
var c = dba.createCluster('test', {exitStateAction: "ABORT_SERVER"});

//@<> WL#12049: Confirm group_replication_exit_state_action is set correctly (ABORT_SERVER) {VER(>=5.7.24)}
// F1.1 - If the exitStateAction option is given, the
// group_replication_exit_state_action variable is set to the value given by the
// user.
EXPECT_EQ("ABORT_SERVER", get_sysvar(session, "group_replication_exit_state_action"));

//@<> group_replication_view_change_uuid must not be set for Clusters running versions < 8.0.27 || >= 8.3.0 {VER(<8.0.27) || VER(>=8.3.0)}
var view_change_uuid_md = session.runSql("select (attributes->>'$.group_replication_view_change_uuid') from mysql_innodb_cluster_metadata.clusters where cluster_name='newcluster'").fetchOne();
EXPECT_EQ(null, view_change_uuid_md);

//@ WL#12049: Dissolve cluster 1 {VER(>=5.7.24)}
c.dissolve({force: true});

//@ WL#12049: Create cluster specifying a valid value for exitStateAction (READ_ONLY) {VER(>=5.7.24)}
// NOTE: the server is in super-read-only since it was dissolved so we must
// disable it
var c = dba.createCluster('test', {clearReadOnly: true, exitStateAction: "READ_ONLY"});

//@<> WL#12049: Confirm group_replication_exit_state_action is set correctly (READ_ONLY) {VER(>=5.7.24)}
// F1.1 - If the exitStateAction option is given, the
// group_replication_exit_state_action variable is set to the value given by the
// user.
EXPECT_EQ("READ_ONLY", get_sysvar(session, "group_replication_exit_state_action"));

//@ WL#12049: Dissolve cluster 2 {VER(>=5.7.24)}
c.dissolve({force: true});

//@ WL#12049: Create cluster specifying a valid value for exitStateAction (1) {VER(>=5.7.24)}
// NOTE: the server is in super-read-only since it was dissolved so we must
// disable it
var c = dba.createCluster('test', {clearReadOnly: true, exitStateAction: "1"});

//@<> WL#12049: Confirm group_replication_exit_state_action is set correctly (1) {VER(>=5.7.24)}
// F1.1 - If the exitStateAction option is given, the
// group_replication_exit_state_action variable is set to the value given by the
// user.
EXPECT_EQ("ABORT_SERVER", get_sysvar(session, "group_replication_exit_state_action"));

//@ WL#12049: Dissolve cluster 3 {VER(>=5.7.24)}
c.dissolve({force: true});

//@ WL#12049: Create cluster specifying a valid value for exitStateAction (0) {VER(>=5.7.24)}
// NOTE: the server is in super-read-only since it was dissolved so we must
// disable it
var c = dba.createCluster('test', {clearReadOnly: true, exitStateAction: "0"});

//@<> WL#12049: Confirm group_replication_exit_state_action is set correctly (0) {VER(>=5.7.24)}
// F1.1 - If the exitStateAction option is given, the
// group_replication_exit_state_action variable is set to the value given by the
// user.
EXPECT_EQ("READ_ONLY", get_sysvar(session, "group_replication_exit_state_action"));

//@ WL#12049: Dissolve cluster 4 {VER(>=5.7.24)}
c.dissolve({force: true});

// Verify if exitStateAction is persisted on >= 8.0.12

// F2 - On a successful [dba.]createCluster() or [Cluster.]addInstance() call
// using the option exitStateAction, the GR sysvar
// group_replication_exit_state_action must be persisted using SET PERSIST at
// the target instance, if the MySQL Server instance version is >= 8.0.12.

//@ WL#12049: Create cluster {VER(>=8.0.12)}
var c = dba.createCluster('test', {clearReadOnly: true, groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc", exitStateAction: "READ_ONLY"});

//@<OUT> WL#12049: exitStateAction must be persisted on mysql >= 8.0.12 {VER(>=8.0.12)}
print(get_persisted_gr_sysvars(__mysql_sandbox_port1));

//@ WL#12049: Dissolve cluster 6 {VER(>=8.0.12)}
c.dissolve({force: true});

// Verify that group_replication_exit_state_action is not persisted when not used
// We need a clean instance for that because dissolve does not unset the previously set variables

//@ WL#12049: Initialize new instance
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});

shell.connect(__sandbox_uri1);

//@ WL#12049: Create cluster 2 {VER(>=8.0.12)}
var c = dba.createCluster('test', {groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc"});

//@<OUT> BUG#28701263: DEFAULT VALUE OF EXITSTATEACTION TOO DRASTIC {VER(>=8.0.12) && VER(<8.0.16)}
print(get_persisted_gr_sysvars(__mysql_sandbox_port1));

//@<> BUG#29037274: ADMINAPI MUST NOT SET DEFAULT VALUE FOR G_R_EXIT_STATE_ACTION IN 8.0.16 {VER(>=8.0.16)}
EXPECT_EQ(null, session.runSql("SELECT * from performance_schema.persisted_variables WHERE Variable_name like 'group_replication_exit_state_action'").fetchOne());

//@<> WL#12049: Finalization
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);


// WL#11032 AdminAPI: Configure member weight for automatic primary election on
// failover
//
// In MySQL 8.0.2 and 5.7.20, Group Replication introduces an option to control
// the outcome of the primary election algorithm in single-primary mode. With
// this option, the user can influence the primary member election by providing
// a member weight value for each member node. That weight value is used for
// electing the primary member instead of the member uuid which is the default
// method used for the election.
//
// In order to support defining such option, the AdminAPI was extended by
// introducing a new optional parameter, named 'memberWeight', in the
// following functions:
//
// - dba.createCluster()
// - Cluster.addInstance()
//

//@ WL#11032: Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});

shell.connect(__sandbox_uri1);

//@ WL#11032: Unsupported server version {VER(<5.7.20)}
var c = dba.createCluster('test', {memberWeight: 25});

//@ WL#11032: Create cluster errors using memberWeight option {VER(>=5.7.20)}
// F1.2 - The memberWeight option shall be an integer value.
var c = dba.createCluster('test', {memberWeight: ""});

var c = dba.createCluster('test', {memberWeight: true});

var c = dba.createCluster('test', {memberWeight: "AB"});

var c = dba.createCluster('test', {memberWeight: 10.5});

//@ WL#11032: Create cluster specifying a valid value for memberWeight (25) {VER(>=5.7.20)}
var c = dba.createCluster('test', {memberWeight: 25});

//@<> WL#11032: Confirm group_replication_member_weight is set correctly (25) {VER(>=5.7.20)}
// F1.1 - If the memberWeight option is given, the
// group_replication_member_weight variable is set to the value given by the
// user.
EXPECT_EQ(25, get_sysvar(session, "group_replication_member_weight"));

//@ WL#11032: Dissolve cluster 1 {VER(>=5.7.20)}
c.dissolve({force: true});

//@ WL#11032: Create cluster specifying a valid value for memberWeight (100) {VER(>=5.7.20)}
// NOTE: the server is in super-read-only since it was dissolved so we must
// disable it
var c = dba.createCluster('test', {clearReadOnly: true, memberWeight: 100});

//@<> WL#11032: Confirm group_replication_member_weight is set correctly (100) {VER(>=5.7.20)}
// F1.1 - If the memberWeight option is given, the
// group_replication_member_weight variable is set to the value given by the
// user.
EXPECT_EQ(100, get_sysvar(session, "group_replication_member_weight"));

//@ WL#11032: Dissolve cluster 2 {VER(>=5.7.20)}
c.dissolve({force: true});

//@ WL#11032: Create cluster specifying a valid value for memberWeight (-50) {VER(>=5.7.20)}
// NOTE: the server is in super-read-only since it was dissolved so we must
// disable it
var c = dba.createCluster('test', {clearReadOnly: true, memberWeight: -50});

//@<> WL#11032: Confirm group_replication_member_weight is set correctly (0) {VER(>=5.7.20)}
// F1.1 - If the memberWeight option is given, the
// group_replication_member_weight variable is set to the value given by the
// user.
EXPECT_EQ(0, get_sysvar(session, "group_replication_member_weight"));

//@ WL#11032: Dissolve cluster 3 {VER(>=5.7.20)}
c.dissolve({force: true});

// Verify if memberWeight is persisted on >= 8.0.11

// F2 - On a successful [dba.]createCluster() or [Cluster.]addInstance() call
// using the option memberWeight, the GR sysvar
// group_replication_member_weight must be persisted using SET PERSIST at
// the target instance, if the MySQL Server instance version is >= 8.0.11.

//@ WL#11032: Create cluster {VER(>=8.0.11)}
var c = dba.createCluster('test', {clearReadOnly: true, groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc", memberWeight: 75});

//@<OUT> WL#11032: memberWeight must be persisted on mysql >= 8.0.11 {VER(>=8.0.11)}
print(get_persisted_gr_sysvars(__mysql_sandbox_port1));

//@ WL#11032: Dissolve cluster 6 {VER(>=8.0.11)}
c.dissolve({force: true});

// Verify that group_replication_member_weight is not persisted when not used
// We need a clean instance for that because dissolve does not unset the previously set variables

//@ WL#11032: Initialize new instance
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});

shell.connect(__sandbox_uri1);

//@ WL#11032: Create cluster 2 {VER(>=8.0.11)}
var c = dba.createCluster('test', {groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc"});

//@<OUT> WL#11032: memberWeight must not be persisted on mysql >= 8.0.11 if not set {VER(>=8.0.11)}
print(get_persisted_gr_sysvars(__mysql_sandbox_port1));

//@ WL#11032: Finalization
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);

// WL#12067 AdminAPI: Define failover consistency
//
// In 8.0.14, Group Replication introduces an option to specify the failover
// guarantees (eventual or read_your_writes) when a primary failover happens in
// single-primary mode). The new option defines the behavior of a new fencing
// mechanism when a new primary is being promoted in a group. The fencing will
// restrict connections from writing and reading from the new primary until it
// has applied all the pending backlog of changes that came from the old
// primary (read_your_writes). Applications will not see time going backward for
// a short period of time (during the new primary promotion).

// In order to support defining such option, the AdminAPI was extended by
// introducing a new optional parameter, named 'consistency', in the
// dba.createCluster function.
//
//@ WL#12067: Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});

shell.connect(__sandbox_uri1);

//@ WL#12067: TSF1_6 Unsupported server version {VER(<8.0.14)}
var c = dba.createCluster('test', {consistency: "EVENTUAL"});

//@ WL#12067: Create cluster errors using consistency option {VER(>=8.0.14)}
// TSF1_4, TSF1_5 - The consistency option shall be a string value.
// NOTE: GR validates the value, which is an Enumerator, and accepts the values
// `BEFORE_ON_PRIMARY_FAILOVER` or `EVENTUAL`, or 1 or 0.
var c = dba.createCluster('test', {consistency: ""});

var c = dba.createCluster('test', {consistency: " "});

var c = dba.createCluster('test', {consistency: ":"});

var c = dba.createCluster('test', {consistency: "AB"});

var c = dba.createCluster('test', {consistency: "10"});

var c = dba.createCluster('test', {consistency: 1});

var c = dba.createCluster('test', {consistency: "1", failoverConsistency: "1"});

//@ WL#12067: TSF1_1 Create cluster using BEFORE_ON_PRIMARY_FAILOVER as value for consistency {VER(>=8.0.14)}
var c = dba.createCluster('test', {consistency: "BEFORE_ON_PRIMARY_FAILOVER"});

//@<> WL#12067: TSF1_1 Confirm group_replication_consistency is set correctly (BEFORE_ON_PRIMARY_FAILOVER) {VER(>=8.0.14)}
EXPECT_EQ("BEFORE_ON_PRIMARY_FAILOVER", get_sysvar(session, "group_replication_consistency"));

//@<> WL#12067: TSF1_1 Confirm group_replication_consistency was correctly persisted. {VER(>=8.0.14)}
EXPECT_EQ("BEFORE_ON_PRIMARY_FAILOVER", get_sysvar(session, "group_replication_consistency", "PERSISTED"));

//@ WL#12067: Dissolve cluster 1 {VER(>=8.0.14)}
c.dissolve({force: true});

//@ WL#12067: TSF1_2 Create cluster using EVENTUAL as value for consistency {VER(>=8.0.14)}
// NOTE: the server is in super-read-only since it was dissolved so we must disable it
var c = dba.createCluster('test', {clearReadOnly:true, consistency: "EVENTUAL"});

//@<> WL#12067: TSF1_2 Confirm group_replication_consistency is set correctly (EVENTUAL) {VER(>=8.0.14)}
EXPECT_EQ("EVENTUAL", get_sysvar(session, "group_replication_consistency"));

//@<> WL#12067: TSF1_2 Confirm group_replication_consistency was correctly persisted. {VER(>=8.0.14)}
EXPECT_EQ("EVENTUAL", get_sysvar(session, "group_replication_consistency", "PERSISTED"));

//@ WL#12067: Dissolve cluster 2 {VER(>=8.0.14)}
c.dissolve({force: true});

//@ WL#12067: TSF1_1 Create cluster using 1 as value for consistency {VER(>=8.0.14)}
var c = dba.createCluster('test', {clearReadOnly:true, consistency: "1"});

//@<> WL#12067: TSF1_1 Confirm group_replication_consistency is set correctly (1) {VER(>=8.0.14)}
EXPECT_EQ("BEFORE_ON_PRIMARY_FAILOVER", get_sysvar(session, "group_replication_consistency"));

//@ WL#12067: Dissolve cluster 3 {VER(>=8.0.14)}
c.dissolve({force: true});

//@ WL#12067: TSF1_2 Create cluster using 0 as value for consistency {VER(>=8.0.14)}
// NOTE: the server is in super-read-only since it was dissolved so we must disable it
var c = dba.createCluster('test', {clearReadOnly:true, consistency: "0"});

//@<> WL#12067: TSF1_2 Confirm group_replication_consistency is set correctly (0) {VER(>=8.0.14)}
EXPECT_EQ("EVENTUAL", get_sysvar(session, "group_replication_consistency"));

//@ WL#12067: Dissolve cluster 4 {VER(>=8.0.14)}
c.dissolve({force: true});

//@ WL#12067: TSF1_3 Create cluster using no value for consistency {VER(>=8.0.14)}
// NOTE: the server is in super-read-only since it was dissolved so we must disable it
var c = dba.createCluster('test', {clearReadOnly:true});

//@<> WL#12067: TSF1_3 Confirm without consistency group_replication_consistency is set to default (EVENTUAL) {VER(>=8.0.14)}
EXPECT_EQ("EVENTUAL", get_sysvar(session, "group_replication_consistency"));

//@ WL#12067: Dissolve cluster 5 {VER(>=8.0.14)}
c.dissolve({force: true});

//@ WL#12067: TSF1_7 Create cluster using evenTual as value for consistency throws no exception (case insensitive) {VER(>=8.0.14)}
// NOTE: the server is in super-read-only since it was dissolved so we must disable it
var c = dba.createCluster('test', {clearReadOnly:true, consistency: "EvenTual"});

//@<> WL#12067: TSF1_7 Confirm group_replication_consistency is set correctly (EVENTUAL) {VER(>=8.0.14)}
EXPECT_EQ("EVENTUAL", get_sysvar(session, "group_replication_consistency"));

//@ WL#12067: Dissolve cluster 6 {VER(>=8.0.14)}
c.dissolve({force: true});

//@ WL#12067: TSF1_8 Create cluster using Before_ON_PriMary_FailoveR as value for consistency throws no exception (case insensitive) {VER(>=8.0.14)}
// NOTE: the server is in super-read-only since it was dissolved so we must disable it
var c = dba.createCluster('test', {clearReadOnly:true, consistency: "Before_ON_PriMary_FailoveR"});

//@<> WL#12067: TSF1_8 Confirm group_replication_consistency is set correctly (BEFORE_ON_PRIMARY_FAILOVER) {VER(>=8.0.14)}
EXPECT_EQ("BEFORE_ON_PRIMARY_FAILOVER", get_sysvar(session, "group_replication_consistency"));

//@ WL#12067: Dissolve cluster 7 {VER(>=8.0.14)}
c.dissolve({force: true});

// Verify that group_replication_consistency is not persisted when not used
// We need a clean instance for that because dissolve does not unset the previously set variables
//@ WL#12067: Initialize new instance {VER(>=8.0.14)}
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});

shell.connect(__sandbox_uri1);

//@ WL#12067: Create cluster 2 {VER(>=8.0.14)}
var c = dba.createCluster('test');

//@<> WL#12067: consistency must not be persisted on mysql >= 8.0.14 if not set {VER(>=8.0.14)}
EXPECT_EQ("", get_sysvar(session, "group_replication_consistency", "PERSISTED"));

//@ WL#12067: Finalization
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);

// WL#12050 AdminAPI: Define the timeout for evicting previously active cluster members
//
// In 8.0.13, Group Replication introduced an option to allow defining
// the timeout for evicting previously active nodes.  In order to support
// defining such option, the AdminAPI was extended by introducing a new
// optional parameter, named 'expelTimeout', in the dba.createCluster()
// function.
//
//@ WL#12050: Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
shell.connect(__sandbox_uri1);

//@ WL#12050: TSF1_5 Unsupported server version {VER(<8.0.13)}
var c = dba.createCluster('test', {expelTimeout: 100});

//@<> WL#12050: Create cluster errors using expelTimeout option {VER(>=8.0.13)}
// TSF1_3, TSF1_4, TSF1_6
EXPECT_THROWS(function(){dba.createCluster('test', {expelTimeout:""});}, "Option 'expelTimeout' Integer expected, but value is String");
EXPECT_THROWS(function(){dba.createCluster('test', {expelTimeout: "10a"});}, "Option 'expelTimeout' Integer expected, but value is String");
EXPECT_THROWS(function(){dba.createCluster('test', {expelTimeout: 10.5});}, "Option 'expelTimeout' is expected to be of type Integer, but is Float");
EXPECT_THROWS(function(){dba.createCluster('test', {expelTimeout: true});}, "Option 'expelTimeout' is expected to be of type Integer, but is Bool");
EXPECT_THROWS(function(){dba.createCluster('test', {expelTimeout: -1});}, "Invalid value for 'expelTimeout': integer value must be >= 0");

//@<> WL#12050: Create cluster errors using expelTimeout option (v8.0.13) {VER(==8.0.13)}
EXPECT_THROWS(function(){dba.createCluster('test', {expelTimeout: 3601});}, "Invalid value for 'expelTimeout': integer value must be in the range [0, 3600]");

//@ WL#12050: TSF1_1 Create cluster using 12 as value for expelTimeout {VER(>=8.0.13)}
var c = dba.createCluster('test', {expelTimeout: 12});

//@<> WL#12050: TSF1_1 Confirm group_replication_member_expel_timeout is set correctly (12) {VER(>=8.0.13)}
EXPECT_EQ(12, get_sysvar(session, "group_replication_member_expel_timeout"));

//@<> WL#12050: TSF1_1 Confirm group_replication_consistency was correctly persisted. {VER(>=8.0.13)}
EXPECT_EQ("12", get_sysvar(session, "group_replication_member_expel_timeout", "PERSISTED"));

//@ WL#12050: Dissolve cluster 1 {VER(>=8.0.13)}
c.dissolve({force: true});

// Verify that group_replication_member_expel_timeout is not persisted when not used
// We need a clean instance for that because dissolve does not unset the previously set variables
//@ WL#12050: TSF1_2 Initialize new instance {VER(>=8.0.13)}
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
shell.connect(__sandbox_uri1);

//@ WL#12050: TSF1_2 Create cluster using no value for expelTimeout, confirm it has the default value {VER(>=8.0.13)}
// NOTE: the server is in super-read-only since it was dissolved so we must disable it
var c = dba.createCluster('test', {clearReadOnly:true});
c.disconnect();

//@<> WL#12050: TSF1_2 Confirm group_replication_member_expel_timeout is set correctly (0) {VER(>=8.0.13)}
EXPECT_EQ(__default_gr_expel_timeout, get_sysvar(session, "group_replication_member_expel_timeout"));

//@<> WL#12050: TSF1_2 Confirm group_replication_member_expel_timeout was not persisted since no value was provided. {VER(>=8.0.13)}
EXPECT_EQ("", get_sysvar(session, "group_replication_member_expel_timeout", "PERSISTED"));

//@ WL#12050: Finalization
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);

// Regression test for BUG#25867733: CHECKINSTANCECONFIGURATION SUCCESS BUT CREATE CLUSTER FAILING IF PFS DISABLED
//@ BUG#25867733: Deploy instances (setting performance_schema value).
testutil.deploySandbox(__mysql_sandbox_port1, "root", {"performance_schema": "off", report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {"performance_schema": "on",report_host: hostname});
shell.connect(__sandbox_uri1);

//@ BUG#25867733: createCluster error with performance_schema=off
dba.createCluster("test_cluster");

session.close();
shell.connect(__sandbox_uri2);

//@ BUG#25867733: createCluster no error with performance_schema=on
var c = dba.createCluster("test_cluster");

//@ BUG#25867733: cluster.status() successful
c.status();

//@ BUG#25867733: finalization
c.disconnect();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);


// Regression test for BUG#29246110: CREATECLUSTER() OR ADDINSTANCE() FAILS IN DEBIAN-BASED PLATFORMS WITH LOCALHOST
//@ BUG#29246110: Deploy instances, setting non supported host: 127.0.1.1.
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: "127.0.1.1"});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});

//@ BUG#29246110: check instance error with non supported host.
dba.checkInstanceConfiguration(__sandbox_uri1);

//@ BUG#29246110: createCluster error with non supported host.
shell.connect(__sandbox_uri1);
var c = dba.createCluster("error");
session.close();

//@ BUG#29246110: create cluster succeed with supported host.
shell.connect(__sandbox_uri2);
var c = dba.createCluster("test_cluster", {gtidSetIsComplete: true});

//@ BUG#29246110: add instance error with non supported host.
c.addInstance(__sandbox_uri1);

// NOTE: Do not destroy sandbox 2 to be used on the next tests
//@ BUG#29246110: finalization
c.dissolve({force: true});
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);

// WL#12066 AdminAPI: option to define the number of auto-rejoins
//
// In 8.0.16, Group Replication introduced an option to allow setting the
// number of auto-rejoin attempts an instance will do after being
// expelled from the group.  In order to support defining such option,
// the AdminAPI was extended by introducing a new optional parameter,
// named 'autoRejoinTries', in the dba.createCluster(),
// cluster.addInstance, cluster.setOption and cluster.setInstanceOption
// functions.

//@ WL#12066: Initialization {VER(>=8.0.16)}
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
var s1 = mysql.getSession(__sandbox_uri1);
var s2 = mysql.getSession(__sandbox_uri2);
shell.connect(__sandbox_uri1);

// FR1 - A new option autoRejoinTries shall be added to the [dba.]createCluster() and [cluster.]addInstance() to allow users to set the Group Replication (GR) system variable group_replication_autorejoin_tries.

//@ WL#12066: TSF1_4 Validate that an exception is thrown if the value specified is not an unsigned integer. {VER(>=8.0.16)}
var c = dba.createCluster('test', {autoRejoinTries: -1});

//@ WL#12066: TSF1_5 Validate that an exception is thrown if the value  is not in the range 0 to 2016. {VER(>=8.0.16)}
var c = dba.createCluster('test', {autoRejoinTries: 2017});

//@ WL#12066: TSF1_1 Validate that the functions [dba.]createCluster() and [cluster.]addInstance() support a new option named autoRejoinTries. {VER(>=8.0.16)}
var c = dba.createCluster('test', {autoRejoinTries: 2016});
session.close();
shell.connect(__sandbox_uri2);
var c2 = dba.createCluster('test2', {clearReadOnly: true});
session.close();

//@<> WL#12066: TSF1_3, TSF1_6 Validate that when calling the functions [dba.]createCluster() and [cluster.]addInstance(), the GR variable group_replication_autorejoin_tries is persisted with the value given by the user on the target instance.{VER(>=8.0.16)}
EXPECT_EQ(2016, get_sysvar(__mysql_sandbox_port1, "group_replication_autorejoin_tries"));
EXPECT_EQ(__default_gr_auto_rejoin_tries, get_sysvar(__mysql_sandbox_port2, "group_replication_autorejoin_tries"));

//@<> WL#12066: TSF1_3, TSF1_6 Confirm group_replication_autorejoin_tries value was persisted {VER(>=8.0.16)}
EXPECT_EQ("2016", get_sysvar(__mysql_sandbox_port1, "group_replication_autorejoin_tries", "PERSISTED"));
EXPECT_EQ("", get_sysvar(__mysql_sandbox_port2, "group_replication_autorejoin_tries", "PERSISTED"));

//@ WL#12066: Dissolve cluster {VER(>=8.0.16)}
c.dissolve({force: true});
c2.dissolve({force: true});

//@ WL#12066: Finalization {VER(>=8.0.16)}
// NOTE: Do not destroy sandbox 2 to be used on the next tests
testutil.destroySandbox(__mysql_sandbox_port1);

// Regression test for BUG#29308037
//
// When dba.createCluster() fails and a rollback is performed to remove the
// created replication user, the account created at 'localhost' is not being
// removed

// Force a failure of the create_cluster function after the replication-user was created
//@<> BUG#29308037: Create cluster using an invalid localAddress
shell.connect(__sandbox_uri2);
if (__version_num < 80027) {
    EXPECT_THROWS(function() {
        dba.createCluster('test', {localAddress: '1a', clearReadOnly: true});
    }, "The 'localAddress' isn't compatible with the Group Replication automatically generated list of allowed IPs.");
}
else {
    EXPECT_THROWS(function() {
        dba.createCluster('test', {localAddress: '1a', clearReadOnly: true});
    }, "Server address configuration error");
}


//@<OUT> BUG#29308037: Confirm that all replication users where removed
print(get_query_single_result(session, number_of_rpl_users_query) + "\n");

//@ BUG#29308037: Finalization
//NOTE: Do not destroy sandbox 2 to be used on the next tests
session.close();

// BUG#32197222: ADMINAPI CREATECLUSTER() SHOULD NOT ALLOW EXISTING ASYNC REPLICATION CHANNELS
//
// With BUG#29305551, dba.checkInstanceConfiguration() was extended to
// include a check to verify if asynchronous replication is configured and
// running on the target instance and print a warning if that's the case.
// On top of that, the same check is used in <Cluster>.addInstance() and
// <Cluster>.rejoinInstance() to terminate the commands with an error if
// such scenario is verified.
// The same check is also used in dba.rebootClusterFromCompleteOutage()
// whenever there are instances to be rejoined to the cluster.
//
// However, dba.createCluster() was skipping that check.
//
// dba.createCluster() must fail if asynchronous replication is running, unless
// the 'force' option is

//@<> BUG#29305551: Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});

//@<> BUG#29305551: Setup asynchronous replication
shell.connect(__sandbox_uri1);

// Create Replication user
session.runSql("CREATE USER 'repl'@'%' IDENTIFIED BY 'password' REQUIRE SSL");
session.runSql("GRANT REPLICATION SLAVE ON *.* TO 'repl'@'%';");

// Set async channel on instance2
session.close();
shell.connect(__sandbox_uri2);

session.runSql("change " + get_replication_source_keyword() + " TO " + get_replication_option_keyword() + "_HOST='" + hostname + "', " + get_replication_option_keyword() + "_PORT=" + __mysql_sandbox_port1 + ", " + get_replication_option_keyword() + "_USER='repl', " + get_replication_option_keyword() + "_PASSWORD='password', " + get_replication_option_keyword() + "_AUTO_POSITION=1, " + get_replication_option_keyword() + "_SSL=1");

session.runSql("START " + get_replica_keyword());

//@ Create cluster async replication (should fail)
dba.createCluster('testAsync', {clearReadOnly: true});

//@<> Create cluster async replication using force option (should pass)
EXPECT_NO_THROWS(function(){dba.createCluster('testAsync', {clearReadOnly: true, force: true});});

//@<> BUG#29305551: Finalization
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

// BUG#29361352: multiprimary warning and prompt displayed with multiPrimary:false
//
// The warning an prompt for creating multi-primary cluster should only be
// displayed if the multiPrimary option is set to true.
// This is a regression issue.

//@<> BUG#29361352: Initialization.
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
shell.connect(__sandbox_uri1);

//@<> BUG#29361352: no warning or prompt for multi-primary (interactive: true, multiPrimary: false).

var c;
EXPECT_NO_THROWS(function() { c = dba.createCluster('test', {interactive: true, multiPrimary: false, force: false}); });

if (__version_num < 80011) {
 EXPECT_STDOUT_CONTAINS_MULTILINE(`A new InnoDB Cluster will be created on instance '${hostname}:${__mysql_sandbox_port1}'.

Validating instance configuration at localhost:${__mysql_sandbox_port1}...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as ${hostname}:${__mysql_sandbox_port1}

Instance configuration is suitable.
NOTE: Group Replication will communicate with other members using '${hostname}:${__mysql_sandbox_gr_port1}'. Use the localAddress option to override.

NOTE: The 'localAddress' "${hostname}" is [[*]], which is compatible with the Group Replication automatically generated list of IPs.
See https://dev.mysql.com/doc/refman/en/group-replication-ip-address-permissions.html for more details.
NOTE: When adding more instances to the Cluster, be aware that the subnet masks dictate whether the instance's address is automatically added to the allowlist or not. Please specify the 'ipAllowlist' accordingly if needed.
* Checking connectivity and SSL configuration...

WARNING: Instance '${hostname}:${__mysql_sandbox_port1}' cannot persist Group Replication configuration since MySQL version ${__version} does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.
Creating InnoDB Cluster 'test' on '${hostname}:${__mysql_sandbox_port1}'...

Adding Seed Instance...
Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.`);
} else if (__version_num > 80011 && __version_num < 80027) {
EXPECT_STDOUT_CONTAINS_MULTILINE(`A new InnoDB Cluster will be created on instance '${hostname}:${__mysql_sandbox_port1}'.

Validating instance configuration at localhost:${__mysql_sandbox_port1}...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as ${hostname}:${__mysql_sandbox_port1}

Instance configuration is suitable.
NOTE: Group Replication will communicate with other members using '${hostname}:${__mysql_sandbox_gr_port1}'. Use the localAddress option to override.

* Checking connectivity and SSL configuration...

Creating InnoDB Cluster 'test' on '${hostname}:${__mysql_sandbox_port1}'...

Adding Seed Instance...
Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.`);
} else {
EXPECT_STDOUT_CONTAINS_MULTILINE(
`A new InnoDB Cluster will be created on instance '${hostname}:${__mysql_sandbox_port1}'.

Validating instance configuration at localhost:${__mysql_sandbox_port1}...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as ${hostname}:${__mysql_sandbox_port1}

Instance configuration is suitable.
NOTE: Group Replication will communicate with other members using '${hostname}:${__mysql_sandbox_port1}'. Use the localAddress option to override.

* Checking connectivity and SSL configuration...

Creating InnoDB Cluster 'test' on '${hostname}:${__mysql_sandbox_port1}'...

Adding Seed Instance...
Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.`);
}

//@<> BUG#29361352: Finalization.
c.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);

// BUG#28064729: mysql-shell builds the metadata schema before first member bootstraps the group
//
// The create cluster operation creates the metadata and replication users
// before bootstrapping the GR group, creating GTIDs for those transaction
// associated to the server UUID instead of the Gr group name (UUID).

//@<> BUG#28064729: Initialization.
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
shell.connect(__sandbox_uri1);

//@<> BUG#28064729: create a cluster.
// NOTE: This test only makes sense when using the XCom communication protocol.
// With MySQL we have to create the replication account before starting GR so
// there's no way to guarantee that there won't be any transactions associated
// to the server uuid
var c;
if (__version_num < 80027) {
    c = dba.createCluster('test', {gtidSetIsComplete: true});
} else {
    c = dba.createCluster('test', {gtidSetIsComplete: true, communicationStack: "xcom"});
}

//@<> BUG#28064729: Verify that there are no GTIDs associated to the server uuid (create cluster).
EXPECT_FALSE(gtid_contains_server_uuid(session));

//@<> BUG#28064729: Verify that there are GTIDS associated to the GR group name (create cluster).
EXPECT_TRUE(gtid_contains_gr_group_name(session));

//@<> BUG#28064729: add an instance.
c.addInstance(__sandbox_uri2);
session.close();
shell.connect(__sandbox_uri2);

//@<> BUG#28064729: Verify that there are no GTIDs associated to the server uuid (add instance).
EXPECT_FALSE(gtid_contains_server_uuid(session));

//@<> BUG#28064729: Verify that there are GTIDS associated to the GR group name (add instance).
EXPECT_TRUE(gtid_contains_gr_group_name(session));

//@<> BUG#28064729: Finalization.
c.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

// WL#12773 AdminAPI: Simplification of internal recovery accounts

//@<> WL#12773: Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname, server_id: 11111});
shell.connect(__sandbox_uri1);

//@ WL#12773: FR4 - The ipWhitelist shall not change the behavior defined by FR1
var result = session.runSql("SELECT COUNT(*) FROM mysql.user");
var old_account_number = result.fetchOne()[0];

if (__version_num < 80027) {
    testutil.callMysqlsh([__sandbox_uri1, "--", "dba", "create-cluster", "test", "--ip-whitelist=192.168.2.1/15,127.0.0.1," + hostname_ip])
} else {
    testutil.callMysqlsh([__sandbox_uri1, "--", "dba", "create-cluster", "test", "--ip-whitelist=192.168.2.1/15,127.0.0.1," + hostname_ip, "--communication-stack=xcom"])
}
print(get_all_gr_recovery_accounts(session));

result = session.runSql("SELECT COUNT(*) FROM mysql.user");
var new_account_number = result.fetchOne()[0];
EXPECT_EQ(old_account_number + 1, new_account_number);

//@<> WL#12773: Cleanup
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);

//@<> WL#13208: Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});

shell.connect(__sandbox_uri1);

//@ WL#13208: TS_FR1_2 validate errors for disableClone (only boolean values).
dba.createCluster('test', {disableClone: ""});
dba.createCluster('test', {disableClone: "not_a_bool"});
dba.createCluster('test', {disableClone: []});
dba.createCluster('test', {disableClone: {}});

//@ WL#13208: TS_FR1_3 validate default for disableClone is false.
var c = dba.createCluster('test');

//@<> WL#13208: TS_FR1_3 verify disableClone is false.
normalize_cluster_options(c.options());

//@<> WL#13208: TS_FR1_3 dissolve cluster.
c.dissolve();

//@ WL#13208: TS_FR1_1 disableClone option is available (set it to a valid value: true). {VER(>=8.0.17)}
var c = dba.createCluster('test', {disableClone: true});

//@ WL#13208: TS_FR1_1 verify disableClone match the value set (true). {VER(>=8.0.17)}
normalize_cluster_options(c.options());

//@<> WL#13208: TS_FR1_1 dissolve cluster. {VER(>=8.0.17)}
c.dissolve();

//@ WL#13208: TS_FR1_1 disableClone option is not supported for server version < 8.0.17. {VER(<8.0.17)}
dba.createCluster('test', {disableClone: true});

//@<> WL#13208: clean-up
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

//@<> Initialization canonical IPv6 addresses are supported WL#12758 {VER(>= 8.0.14)}
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: "::1"});
shell.connect(__sandbox_uri1);

//@ canonical IPv6 addresses are supported WL#12758 {VER(>= 8.0.14)}
// The following test requires the XCOM protocol stack and the validation uses __mysql_sandbox_gr_port1 so we must set it to the default value that is used when XCOM is used, otherwise, it'll be using the one for MySQL Comm Stack if the version is >= 8.0.27
__mysql_sandbox_gr_port1 = __mysql_sandbox_port1 * 10 + 1;

var c;
if (__version_num < 80027) {
    c = dba.createCluster("cluster");
} else {
    c = dba.createCluster("cluster", {communicationStack: "xcom"});
}

// Bug #30548843 Validate that IPv6 value stored on metadata for mysqlx is valid
print_metadata_instance_addresses(session);

//@<> Cleanup canonical IPv6 addresses are supported WL#12758 {VER(>= 8.0.14)}
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);

//@<> Initialization IPv6 addresses are supported on localAddress, groupSeeds and ipWhitelist WL#12758 {VER(>= 8.0.14)}
testutil.deploySandbox(__mysql_sandbox_port1, "root");
shell.connect(__sandbox_uri1);

//@ IPv6 addresses are supported on localAddress, groupSeeds and ipWhitelist WL#12758 {VER(>= 8.0.14)}
var local_address = "[::1]:" + __mysql_sandbox_gr_port1;
var ip_white_list = "::1, 127.0.0.1";
var group_seeds = "[::1]:" + __mysql_sandbox_gr_port1; + ", [::1]:" + __mysql_sandbox_gr_port2;
c = dba.createCluster("cluster", {ipWhitelist:ip_white_list, groupSeeds:group_seeds, localAddress:local_address, communicationStack: "xcom"});

//@<> If the target instance is >= 8.0.23, ipWhitelist is automatically used to set ipAllowlist {VER(>=8.0.23)}
shell.connect(__sandbox_uri1);
EXPECT_EQ(ip_white_list, get_sysvar(session, "group_replication_ip_allowlist"));

//@<> If the target instance is >= 8.0.23, ipWhitelist is automatically used to set ipAllowlist {VER(>=8.0.14) && VER(<8.0.23)}
shell.connect(__sandbox_uri1);
EXPECT_EQ(ip_white_list, get_sysvar(session, "group_replication_ip_whitelist"));

EXPECT_EQ(local_address, get_sysvar(session, "group_replication_local_address"));
EXPECT_EQ(group_seeds, get_sysvar(session, "group_replication_group_seeds"));

//@<> Dissolve the cluster {VER(>=8.0)}
c.dissolve({force: true});

//@<> Verify the new option ipAllowlist that deprecates ipWhitelist sets the allowlist {VER(>=8.0.23)}
if (__version_num < 80027) {
    c = dba.createCluster("cluster", {ipAllowlist: ip_white_list});
} else {
    c = dba.createCluster("cluster", {ipAllowlist: ip_white_list, communicationStack: "xcom"});
}

EXPECT_EQ(ip_white_list, get_sysvar(session, "group_replication_ip_allowlist"));

//@<> Cleanup IPv6 addresses are supported on localAddress, groupSeeds and ipWhitelist WL#12758 {VER(>= 8.0.14)}
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);

//@<> Initialization canonical IPv6 addresses are not supported below 8.0.14 WL#12758 {VER(< 8.0.14)}
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: "::1"});
shell.connect(__sandbox_uri1);

//@ canonical IPv6 addresses are not supported below 8.0.14 WL#12758 {VER(< 8.0.14)}
if (__version_num < 80027) {
   c = dba.createCluster("cluster");
} else {
    c = dba.createCluster("cluster", {communicationStack: "xcom"});
}

//@<> Cleanup canonical IPv6 addresses are not supported below 8.0.14 WL#12758 {VER(< 8.0.14)}
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);

//@<> Initialization IPv6 addresses are not supported below 8.0.14 WL#12758 {VER(< 8.0.14)}
testutil.deploySandbox(__mysql_sandbox_port1, "root");
shell.connect(__sandbox_uri1);

//@ IPv6 local_address is not supported below 8.0.14 WL#12758 {VER(< 8.0.14)}
var local_address = "[::1]:" + __mysql_sandbox_gr_port1;
var c = dba.createCluster("cluster", {localAddress:local_address});

//@ IPv6 on ipWhitelist is not supported below 8.0.14 WL#12758 {VER(< 8.0.14)}
var c = dba.createCluster("cluster", {ipWhitelist:"::1, 127.0.0.1"});

//@<> Cleanup IPv6 addresses are not supported below 8.0.14 WL#12758 {VER(< 8.0.14)}
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
