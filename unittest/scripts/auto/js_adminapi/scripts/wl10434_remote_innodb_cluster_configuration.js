// FR1 - When dba.configureInstance() is called on a MySQL instance with
// version >= 8.0.11, it should leave it ready for InnoDB Cluster, regardless of whether the instance is local or not.

// FR1_1 - Configure [REMOTE|LOCAL] instance not valid for InnoDB cluster using dba.configureInstance() with VALID server >= [8.0.11]
// connect to [REMOTE|LOCAL] Instance and configure using dba.configureInstance() using server version >=[8.0.11]
// configure the Instance and check that it works correctly

//@ FR1_1 SETUP {VER(>=8.0.11)}
testutil.deployRawSandbox(__mysql_sandbox_port1, 'root', {'report_host': hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.stopSandbox(__mysql_sandbox_port1);
testutil.changeSandboxConf(__mysql_sandbox_port1, "binlog_checksum", "CRC32");
testutil.startSandbox(__mysql_sandbox_port1);

//@ ConfigureInstance should fail if there's no session nor parameters provided
dba.configureInstance();

//@ FR_1 Configure instance not valid for InnoDB cluster {VER(>=8.0.11)}
// Also covers: ET_4
// Verify that the instance is not valid using dba.checkInstanceConfiguration()
dba.checkInstanceConfiguration(__sandbox_uri1);
// Configure the instance
dba.configureInstance(__sandbox_uri1, {interactive: false});
// Verify that the instance is not valid by restarting it and
// using dba.checkInstanceConfiguration()
testutil.restartSandbox(__mysql_sandbox_port1);
dba.checkInstanceConfiguration(__sandbox_uri1);

//@ FR1_1 TEARDOWN {VER(>=8.0.11)}
testutil.destroySandbox(__mysql_sandbox_port1);

// FR1.1_1 - Try to configure [REMOTE] instance using dba.configureInstance() with variable cannot
// be remotely persisted (for example: log_bin) and VALID servers >= [8.0.11]
// Should raise Message to the user to run the command locally in the target instance

//@ FR1.1_1 SETUP {VER(>=8.0.11)}
testutil.deployRawSandbox(__mysql_sandbox_port1, 'root', {'report_host': hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.stopSandbox(__mysql_sandbox_port1);
testutil.changeSandboxConf(__mysql_sandbox_port1, "skip_log_bin", "ON");
testutil.removeFromSandboxConf(__mysql_sandbox_port1, "log_bin");
testutil.startSandbox(__mysql_sandbox_port1);

//@ FR1.1_1 Configure instance using dba.configureInstance() with variable that cannot remotely persisted {VER(>=8.0.11)}
// Configure the instance
dba.configureInstance(__sandbox_uri1, {interactive: false});

// FR1.1_2 - Try to configure [LOCAL] instance using dba.configureInstance() with variable persisted and VALID servers >= [8.0.11]
//It should succeed configuring

//@# FR1.1_2 Configure instance using dba.configureInstance() with variable that cannot remotely persisted {VER(>=8.0.11)}
var sandbox_cnf1 = testutil.getSandboxConfPath(__mysql_sandbox_port1);
dba.configureInstance(__sandbox_uri1, {interactive: false, mycnfPath: sandbox_cnf1});
// Verify that the instance was successfully configured
testutil.restartSandbox(__mysql_sandbox_port1);
dba.checkInstanceConfiguration(__sandbox_uri1);

//@ FR1.1 TEARDOWN {VER(>=8.0.11)}
testutil.destroySandbox(__mysql_sandbox_port1);

// FR2 - When dba.configureInstance() is called remotely for a MySQL instance which has 'persisted-globals-load'
// set to 'OFF' an error is issued indicating the user to run the command locally in the target instance.

// FR2_1 - Try to configure [REMOTE] instance using dba.configureInstance() and VALID servers >= [8.0.11]
// with server having 'persisted-globals-load' set to 'OFF'
// Should raise Error Message to the user to run the command locally in the target instance
// NOTE: Impossible to test

// FR2_2 - Try to configure [LOCAL] instance using dba.configureInstance() and VALID servers >=[8.0.11]
// with server having 'persisted-globals-load' set to 'OFF'
//Should succeed running the command locally in the target instance

//@ FR2_2 SETUP {VER(>=8.0.11)}
testutil.deployRawSandbox(__mysql_sandbox_port1, 'root', {'report_host': hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.stopSandbox(__mysql_sandbox_port1);
testutil.changeSandboxConf(__mysql_sandbox_port1, "persisted-globals-load", "OFF");
testutil.changeSandboxConf(__mysql_sandbox_port1, "binlog_checksum", "CRC32");
testutil.startSandbox(__mysql_sandbox_port1);

//@ FR2_2 - Configure local instance using dba.configureInstance() with 'persisted-globals-load' set to 'OFF' but no cnf file path {VER(>=8.0.11)}
dba.configureInstance(__sandbox_uri1, {interactive: false});

//@ FR2_2 - Configure local instance using dba.configureInstance() with 'persisted-globals-load' set to 'OFF' with cnf file path {VER(>=8.0.11)}
// Also covers: FR3.2 - If the instance is local and the MySQL config file path is provided by the user as a parameter.
dba.configureInstance(__sandbox_uri1, {interactive: false, mycnfPath: sandbox_cnf1});

//@ FR2_2 TEARDOWN {VER(>=8.0.11)}
testutil.destroySandbox(__mysql_sandbox_port1);

// FR3 - When dba.configureInstance() is called on a MySQL instance with version < 8.0.11 or with
// 'persisted-globals-load' set to 'OFF', it should leave it ready for InnoDB Cluster
// NOTE: 'persisted-globals-load' does not exists in versions < 8.0.11 so we cannot set it

//FR3.1_1 - If the instance is local and the MySQL config file path can be detected automatically.

//@ FR3.1_1 SETUP {VER(<8.0.11)}
testutil.deployRawSandbox(__mysql_sandbox_port1, 'root', {'report_host': hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.stopSandbox(__mysql_sandbox_port1);
testutil.changeSandboxConf(__mysql_sandbox_port1, "binlog_checksum", "CRC32");
testutil.startSandbox(__mysql_sandbox_port1);

//@FR3.1_1 - Configure local instance with 'persisted-globals-load' set to 'OFF' {VER(<8.0.11)}
var sandbox_cnf1 = testutil.getSandboxConfPath(__mysql_sandbox_port1);
testutil.expectPrompt("Please select an option [1]: ", "3");
testutil.expectPrompt("Do you want to perform the required configuration changes? [y/n]:", "y");
dba.configureInstance(__sandbox_uri1, {interactive: true, mycnfPath: sandbox_cnf1});
// Verify that the instance was successfully configured
dba.checkInstanceConfiguration(__sandbox_uri1);

//@ FR3.1_1 TEARDOWN {VER(<8.0.11)}
testutil.destroySandbox(__mysql_sandbox_port1);

//FR3.2_1 - If the instance is local and the MySQL config file path is provided by the user as a parameter.

//@ FR3.2_1 SETUP {VER(<8.0.11)}
testutil.deployRawSandbox(__mysql_sandbox_port1, 'root', {'report_host': hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.stopSandbox(__mysql_sandbox_port1);
testutil.changeSandboxConf(__mysql_sandbox_port1, "binlog_checksum", "CRC32");
testutil.startSandbox(__mysql_sandbox_port1);

//@# FR3.2_1 - Configure local instance with 'persisted-globals-load' set to 'OFF' providing mycnfPath {VER(<8.0.11)}
var sandbox_cnf1 = testutil.getSandboxConfPath(__mysql_sandbox_port1);
dba.configureInstance(__sandbox_uri1, {interactive: false, mycnfPath: sandbox_cnf1});
// Verify that the instance was successfully configured
dba.checkInstanceConfiguration(__sandbox_uri1);

//@ FR3.2_1 TEARDOWN {VER(<8.0.11)}
testutil.destroySandbox(__mysql_sandbox_port1);

//FR3.3_1 - If the instance does not require configuration changes, regardless of whether the instance is local or remote.

//@ FR3.3_1 SETUP {VER(<8.0.11)}
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port1);

//@# FR3.3_1 - Configure local instance that does not require changes {VER(<8.0.11)}
testutil.expectPrompt("Please select an option [1]: ", "3");
dba.configureInstance(__sandbox_uri1, {interactive: true, mycnfPath:testutil.getSandboxConfPath(__mysql_sandbox_port1)});

//@ FR3.3_1 TEARDOWN {VER(<8.0.11)}
testutil.destroySandbox(__mysql_sandbox_port1);

// FR4 - When dba.configureInstance() is not successful because the instance is
// remote and either does not support 'SET PERSIST' or any of the variables to
// update cannot be changed remotely, an error is issued indicating the user to
// run the command locally in the target instance.
// NOTE: Impossible to test due to lack of support for deploying remote instances

// FR5 - A status JSON document must be defined that represents the output information:
// FR5.1 - If the instance is already valid or not.
// FR5.2 - If the instance is invalid, a list of all the configuration settings which were updated and an indication if a restart is required or not.
// FR5.3 - The final status of the operation.

//@ FR5 SETUP {VER(>=8.0.11)}
testutil.deployRawSandbox(__mysql_sandbox_port1, 'root', {'report_host': hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.stopSandbox(__mysql_sandbox_port1);
testutil.changeSandboxConf(__mysql_sandbox_port1, "binlog_checksum", "CRC32");
testutil.startSandbox(__mysql_sandbox_port1);

//@ FR5 Configure instance not valid for InnoDB cluster, with interaction enabled {VER(>=8.0.11)}
// Also covers: FR6 - The function must maintain the current optional parameters available in dba.configureLocalInstance() and include a new boolean optional flag 'interactive'.
//              FR6_1 - Configure LOCAL instance using dba.configureInstance() with the NEW optional parameter 'interactive' on VALID servers >= [8.0.11]
// Also covers: ET_3 and ET_9
testutil.expectPrompt("Please select an option [1]: ", "3");
testutil.expectPrompt("Do you want to perform the required configuration changes?", "y");
testutil.expectPrompt("Do you want to restart the instance after configuring it?", "y");
dba.configureInstance(__sandbox_uri1, {interactive: true});

//@ FR5 TEARDOWN {VER(>=8.0.11)}
testutil.destroySandbox(__mysql_sandbox_port1);

// FR7 - When dba.configureInstance() is called on an instance which belongs to
// an InnoDB Cluster it won't persist the GR configurations as
// dba.configureLocalInstance() did, but terminate with an error.

//@ FR7 SETUP {VER(>=8.0.11)}
// Deploy a pre-configured sandbox since we're not testing configure itself
testutil.deploySandbox(__mysql_sandbox_port1, "root");
shell.connect(__sandbox_uri1);
var cluster = dba.createCluster("C");
cluster.disconnect();

//@ FR7 Configure instance already belonging to cluster {VER(>=8.0.11)}
dba.configureInstance(__sandbox_uri1, {interactive: true});

//@ FR7 TEARDOWN {VER(>=8.0.11)}
cluster.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);

// Extra tests

//@ ET SETUP
testutil.deployRawSandbox(__mysql_sandbox_port1, 'root', {'report_host': hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.stopSandbox(__mysql_sandbox_port1);
testutil.changeSandboxConf(__mysql_sandbox_port1, "binlog_checksum", "CRC32");
testutil.startSandbox(__mysql_sandbox_port1);

// ET_5 - Missing username. if interactive ENABLED, the value for the username
// is prompted having a default (if enter is pressed) of 'root'

//@ ET_5 - Call dba.configureInstance() with interactive flag set to true and not specifying username {VER(>=8.0.11)}
var uri1 = localhost + ":" + __mysql_sandbox_port1;
var root_uri1 = "root@" + uri1;
testutil.expectPassword("Please provide the password for '" + root_uri1 + "': ", "wrongpwd");
dba.configureInstance(uri1, {interactive: true});

// ET_6 - Missing username. if interactive DISABLED, an error is thrown regarding the missing value for the username.
// Impossible to test: inability to get the current system user from the test framework

// ET_7 - Missing password. if interactive ENABLED, the value for the password is prompted
//@ ET_7 - Call dba.configureInstance() with interactive flag set to true and not specifying a password {VER(>=8.0.11)}
testutil.expectPassword("Please provide the password for '" + root_uri1 + "': ", "wrongpwd");
dba.configureInstance(root_uri1, {interactive: true});

// ET_8 - Missing password. if interactive DISABLED, an error is thrown regarding the missing value for the password.
//@ ET_8 - Call dba.configureInstance() with interactive flag set to false and not specifying a password {VER(>=8.0.11)}
dba.configureInstance(root_uri1, {interactive: false});

// ET_10 - Missing administration account password and interactive is ENABLED
// the password is not provided in 'clusterAdminPassword', the Shell prompts the user
// for the password to be used in the administration account created.
//@ ET_10 - Call dba.configuereInstance() with interactive flag set to true and using clusterAdmin {VER(>=8.0.11)}
testutil.expectPassword("Password for new account: ", "newPwd");
testutil.expectPassword("Confirm password: ", "newPwd");
testutil.expectPrompt("Do you want to perform the required configuration changes? [y/n]: ", "y");
testutil.expectPrompt("Do you want to restart the instance after configuring it? [y/n]: ", "y");
dba.configureInstance(__sandbox_uri1, {interactive: true, clusterAdmin: "clusterAdminAccount"});

// ET_12 - Super read-only enabled and 'clearReadOnly' is not set with interactive is ENABLED
// prompts the user if wants to disable super_read_only to continue with the operation.
shell.connect(__sandbox_uri1);
set_sysvar(session, "super_read_only", 1);
EXPECT_EQ('ON', get_sysvar(session, "super_read_only"));
//@ ET_12 - Call dba.configuereInstance() with interactive flag set to true, clusterAdmin option and super_read_only=1 {VER(>=8.0.11)}
testutil.expectPassword("Password for new account: ", "newPwd");
testutil.expectPassword("Confirm password: ", "newPwd");
testutil.expectPrompt("Do you want to perform the required configuration changes?", "Y");
testutil.expectPrompt("Do you want to restart the instance after configuring it?", "y");
testutil.expectPrompt("Do you want to disable super_read_only and continue?", "y");
dba.configureInstance(__sandbox_uri1, {interactive: true, clusterAdmin: "newClusterAdminAccount"});
session.close();

//@ ET_12_alt - Super read-only enabled and 'clearReadOnly' is set {VER(>=8.0.11)}
shell.connect(__sandbox_uri1);
set_sysvar(session, "super_read_only", 1);
EXPECT_EQ('ON', get_sysvar(session, "super_read_only"));
testutil.expectPassword("Password for new account: ", "newPwd");
testutil.expectPassword("Confirm password: ", "newPwd");
testutil.expectPrompt("Do you want to perform the required configuration changes?", "Y");
testutil.expectPrompt("Do you want to restart the instance after configuring it?", "y");
dba.configureInstance(__sandbox_uri1, {interactive: true, mycnfPath: testutil.getSandboxConfPath(__mysql_sandbox_port1), clusterAdmin: "newClusterAdminAccount2", clearReadOnly: true});

//@ ET_12_alt - Super read-only enabled and 'clearReadOnly' is set 5.7 {VER(<8.0.11)}
shell.connect(__sandbox_uri1);
set_sysvar(session, "super_read_only", 1);
EXPECT_EQ('ON', get_sysvar(session, "super_read_only"));
testutil.expectPassword("Password for new account: ", "newPwd");
testutil.expectPassword("Confirm password: ", "newPwd");
testutil.expectPrompt("Do you want to perform the required configuration changes?", "Y");
dba.configureInstance(__sandbox_uri1, {interactive: true, mycnfPath: testutil.getSandboxConfPath(__mysql_sandbox_port1), clusterAdmin: "newClusterAdminAccount2", clearReadOnly: true});

// ET_13 - Super read-only enabled and 'clearReadOnly' is not set with interactive is DISABLED
// prompts the user if wants to disable super_read_only to continue with the operation.
set_sysvar(session, "super_read_only", 1);
EXPECT_EQ('ON', get_sysvar(session, "super_read_only"));
//@ ET_13 - Call dba.configuereInstance() with interactive flag set to false, clusterAdmin option and super_read_only=1 {VER(>=8.0.11)}
dba.configureInstance(__sandbox_uri1, {interactive: false, mycnfPath: testutil.getSandboxConfPath(__mysql_sandbox_port1), clusterAdmin: "newClusterAdminAccount3", clusterAdminPassword: "pwd"});

//@ ET TEARDOWN
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
