//@ {hasAuthEnvironment('OCI_AUTH')}

//@<> BUG#34858763 - invalid value for the config file
testutil.callMysqlsh(["--", "shell", "options", "set-persist", "oci.configFile", os.path.join(__test_data_path, "missing.config.file")]);

testutil.callMysqlsh([`${OCI_AUTH_URI}`,    "--auth-method=authentication_oci_client",     "--quiet-start=2", "--sql", "-e", "SELECT 1"]);
EXPECT_OUTPUT_CONTAINS("Failed to set the OCI config file path on authentication_oci_client plugin.");

testutil.callMysqlsh(["--", "shell", "options", "unset-persist", "oci.configFile"]);

// TODO(pawel): add more tests for BUG#34858763

//@<> WL#15561 - invalid profile name test {OCI_AUTH_CONFIG_FILE.length != 0}

testutil.callMysqlsh(["--", "shell", "options", "set-persist", "oci.profile", "invalidProfile"]);

testutil.callMysqlsh([`${OCI_AUTH_URI}`,
    "--auth-method=authentication_oci_client",
    `--oci-config-file=${OCI_AUTH_CONFIG_FILE}`,
    "--quiet-start=2", "--sql", "-e", "SELECT 1"]);
EXPECT_OUTPUT_CONTAINS("[invalidProfile] is not present in config file");

testutil.callMysqlsh(["--", "shell", "options", "unset-persist", "oci.profile"]);

//@<> WL#15561 - correct config file test, using the DEFAULT profile {OCI_AUTH_POSITIVE_TESTS == 1}
connection_data = shell.parseUri(OCI_AUTH_URI);
config_file_path = OCI_AUTH_CONFIG_FILE;
var config_file = testutil.catFile(OCI_AUTH_CONFIG_FILE);
if (OCI_AUTH_PROFILE != 'DEFAULT') {
    config_file = config_file.replace("[DEFAULT]", "[SOMEPROFILE]");
    config_file = config_file.replace(`[${OCI_AUTH_PROFILE}]`, "[DEFAULT]");
    config_file_path = os.path.join(os.getcwd(), "otherconfig.cfg");
    testutil.createFile(config_file_path , config_file);
}
testutil.callMysqlsh([`${OCI_AUTH_URI}`,
    "--auth-method=authentication_oci_client",
    `--oci-config-file=${config_file_path}`,
    "--quiet-start=2",
    "--sql", "-e", "SELECT current_user()"]);

EXPECT_OUTPUT_CONTAINS(`${connection_data.user}@`);

//@<> WL#15561 - correct profile from config file test {OCI_AUTH_POSITIVE_TESTS == 1}
config_file_path = OCI_AUTH_CONFIG_FILE;
target_profile = OCI_AUTH_PROFILE;
if (OCI_AUTH_PROFILE == 'DEFAULT') {
    var config_file = testutil.catFile(OCI_AUTH_CONFIG_FILE);
    config_file = config_file.replace("[DEFAULT]", "[SOMEPROFILE]");
    config_file_path = os.path.join(os.getcwd(), "otherconfig.cfg");
    testutil.createFile(config_file_path , config_file);
    target_profile = 'SOMEPROFILE';
}
testutil.callMysqlsh([`${OCI_AUTH_URI}`,
    "--auth-method=authentication_oci_client",
    `--oci-config-file=${config_file_path}`,
    `--authentication-oci-client-config-profile=${target_profile}`,
    "--quiet-start=2", 
    "--sql", "-e", "SELECT current_user()"]);
EXPECT_OUTPUT_CONTAINS(`${connection_data.user}@`);

//@<> WL15561 Text Classic Connection OCI Authentication Plugin config file and profile {OCI_AUTH_POSITIVE_TESTS == 1}
connection_data['oci-config-file'] = OCI_AUTH_CONFIG_FILE;
connection_data['authentication-oci-client-config-profile'] = OCI_AUTH_PROFILE;
EXPECT_NO_THROWS(function(){
    var mySession = mysql.getClassicSession(connection_data);
    mySession.close();
});
