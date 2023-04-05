//@ {hasAuthEnvironment('OCI_AUTH')}

//@<> BUG#34858763 - invalid value for the config file
testutil.callMysqlsh(["--", "shell", "options", "set-persist", "oci.configFile", os.path.join(__test_data_path, "missing.config.file")]);

testutil.callMysqlsh([`${OCI_AUTH_URI}`,
    "--auth-method=authentication_oci_client",
    "--quiet-start=2", "--sql", "-e", "SELECT 1"]);
EXPECT_OUTPUT_CONTAINS("Failed to set the OCI config file path on authentication_oci_client plugin.");

testutil.callMysqlsh(["--", "shell", "options", "unset-persist", "oci.configFile"]);

// TODO(pawel): add more tests for BUG#34858763

//@<> WL#15561 - invalid profile name test

testutil.callMysqlsh(["--", "shell", "options", "set-persist", "oci.profile", "invalidProfile"]);

testutil.callMysqlsh([`${OCI_AUTH_URI}`,
    "--auth-method=authentication_oci_client",
    "--quiet-start=2", "--sql", "-e", "SELECT 1"]);
EXPECT_OUTPUT_CONTAINS("Failed to set the OCI client config profile on authentication_oci_client plugin.");

testutil.callMysqlsh(["--", "shell", "options", "unset-persist", "oci.profile"]);

//@<> WL#15561 - correct config file test

testutil.callMysqlsh([`${OCI_AUTH_URI}`,
    "--auth-method=authentication_oci_client",
    `--oci-config-file=${OCI_AUTH_CONFIG_FILE}`,
    "--quiet-start=2", 
    "--sql", "-e", "SELECT current_user()"]);
EXPECT_OUTPUT_CONTAINS('test_oci_auth');

//@<> WL#15561 - correct profile from config file test

var config_file = testutil.catFile(OCI_AUTH_CONFIG_FILE);
config_file = config_file.replace("[DEFAULT]", "[SOMEPROFILE]");
config_file_path = os.path.join(os.getcwd(), "otherconfig.cfg");
testutil.createFile(config_file_path , config_file);

testutil.callMysqlsh([`${OCI_AUTH_URI}`,
    "--auth-method=authentication_oci_client",
    `--oci-config-file=${config_file_path}`,
    `--authentication-oci-client-config-profile=SOMEPROFILE`,
    "--quiet-start=2", 
    "--sql", "-e", "SELECT current_user()"]);
EXPECT_OUTPUT_CONTAINS('test_oci_auth');

//@ WL15561 Text Classic Connection OCI Authentication Plugin config file and profile
connection_data = shell.parseUri(OCI_AUTH_URI);
connection_data['oci-config-file'] = OCI_AUTH_CONFIG_FILE;
connection_data['authentication-oci-client-config-profile'] = 'DEFAULT';
EXPECT_NO_THROWS(function(){
    var mySession = mysql.getClassicSession(connection_data);
    mySession.close();
});
mySession.close();