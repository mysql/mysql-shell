#@{VER(<8.0.27)}

#@<> WL15973-TSFR_2_3_1
testutil.deploy_sandbox(__mysql_sandbox_port1, "root", {"default-authentication-plugin": "sha256_password"})

util.check_for_server_upgrade(__sandbox_uri1, {"targetVersion": "8.3.0"})
EXPECT_STDOUT_CONTAINS("Warning: default_authentication_plugin - sha256_password authentication")

util.check_for_server_upgrade(__sandbox_uri1, {"targetVersion": "8.1.0"})
EXPECT_STDOUT_CONTAINS("Warning: default_authentication_plugin - sha256_password authentication")

testutil.destroy_sandbox(__mysql_sandbox_port1)

testutil.deploy_sandbox(__mysql_sandbox_port1, "root", {"default-authentication-plugin": "mysql_native_password"})

util.check_for_server_upgrade(__sandbox_uri1, {"targetVersion": "8.3.0"})
EXPECT_STDOUT_CONTAINS("Warning: default_authentication_plugin - mysql_native_password authentication")

util.check_for_server_upgrade(__sandbox_uri1, {"targetVersion": "8.1.0"})
EXPECT_STDOUT_CONTAINS("Warning: default_authentication_plugin - mysql_native_password authentication")

testutil.destroy_sandbox(__mysql_sandbox_port1)
