//@ {hasAuthEnvironment('OCI_AUTH')}

//@<> BUG#34858763 - invalid value for the config file
testutil.callMysqlsh(["--", "shell", "options", "set-persist", "oci.configFile", os.path.join(__test_data_path, "missing.config.file")]);

testutil.callMysqlsh([`${OCI_AUTH_URI}`,    "--auth-method=authentication_oci_client",     "--quiet-start=2", "--sql", "-e", "SELECT 1"]);
EXPECT_OUTPUT_CONTAINS("Failed to set the OCI config file path on authentication_oci_client plugin.");

testutil.callMysqlsh(["--", "shell", "options", "unset-persist", "oci.configFile"]);

// TODO(pawel): add more tests for BUG#34858763
