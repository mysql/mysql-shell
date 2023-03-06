#@ {has_oci_environment('MDS')}

#@<> INCLUDE dump_utils.inc
#@<> INCLUDE copy_utils.inc

#@<> entry point

# helpers
def clean_instance(s):
    s.run_sql("DROP SCHEMA IF EXISTS sakila")
    s.run_sql(f"DROP USER IF EXISTS {test_user_account}")

#@<> Setup
testutil.deploy_sandbox(__mysql_sandbox_port1, "root", { "local_infile": 1 })
src_session = mysql.get_session(__sandbox_uri1)
tgt_session = mysql.get_session(MDS_URI)

testutil.import_data(__sandbox_uri1, os.path.join(__data_path, "sql", "sakila-schema.sql"), "", "utf8mb4")
testutil.import_data(__sandbox_uri1, os.path.join(__data_path, "sql", "sakila-data.sql"), "", "utf8mb4")

setup_session(__sandbox_uri1)
analyze_tables(session)
create_test_user(session)

#@<> WL15298_TSFR_4_4_2
clean_instance(tgt_session)
setup_session(__sandbox_uri1)

EXPECT_THROWS(lambda: util.copy_instance(MDS_URI, { "excludeUsers": [ "root" ], "showProgress": False }), "Error: Shell Error (52004): Util.copy_instance: While 'Validating MDS compatibility': Compatibility issues were found", "copy should throw")

EXPECT_STDOUT_CONTAINS("Database `sakila` had unsupported ENCRYPTION option commented out")
EXPECT_STDOUT_CONTAINS(f"User {test_user_account} is granted restricted privileges")
EXPECT_STDOUT_CONTAINS("Function `sakila`.`get_customer_balance` - definition uses DEFINER clause set to user `root`@`localhost` which can only be executed by this user or a user with SET_USER_ID or SUPER privileges (fix this with 'strip_definers' compatibility option)")
EXPECT_STDOUT_CONTAINS("Function `sakila`.`get_customer_balance` - definition does not use SQL SECURITY INVOKER characteristic, which is required (fix this with 'strip_definers' compatibility option)")

#@<> WL15298 - copy with users succeeds
# WL15298_TSFR_1_4
clean_instance(tgt_session)
setup_session(__sandbox_uri1)

EXPECT_NO_THROWS(lambda: util.copy_instance(MDS_URI, { "compatibility": [ "strip_definers", "strip_restricted_grants" ], "excludeUsers": [ "root" ], "showProgress": False }), "copy should not throw")

EXPECT_STDOUT_CONTAINS("Database `sakila` had unsupported ENCRYPTION option commented out")
EXPECT_STDOUT_CONTAINS(f"User {test_user_account} had restricted privileges")
EXPECT_STDOUT_CONTAINS("Function `sakila`.`get_customer_balance` had definer clause removed")
EXPECT_STDOUT_CONTAINS("Function `sakila`.`get_customer_balance` had SQL SECURITY characteristic set to INVOKER")

#@<> WL15298_TSFR_4_4_3
clean_instance(tgt_session)
setup_session(__sandbox_uri1)

EXPECT_THROWS(lambda: util.copy_instance(MDS_URI, { "users": False, "showProgress": False }), "Error: Shell Error (52004): Util.copy_instance: While 'Validating MDS compatibility': Compatibility issues were found", "copy should throw")

EXPECT_STDOUT_CONTAINS("Database `sakila` had unsupported ENCRYPTION option commented out")
EXPECT_STDOUT_NOT_CONTAINS(f"User {test_user_account} is granted restricted privileges")
EXPECT_STDOUT_CONTAINS("Function `sakila`.`get_customer_balance` - definition uses DEFINER clause set to user `root`@`localhost` which can only be executed by this user or a user with SET_USER_ID or SUPER privileges (fix this with 'strip_definers' compatibility option)")
EXPECT_STDOUT_CONTAINS("Function `sakila`.`get_customer_balance` - definition does not use SQL SECURITY INVOKER characteristic, which is required (fix this with 'strip_definers' compatibility option)")

#@<> WL15298 - copy without users succeeds
# WL15298_TSFR_1_4
clean_instance(tgt_session)
setup_session(__sandbox_uri1)

EXPECT_NO_THROWS(lambda: util.copy_instance(MDS_URI, { "compatibility": [ "strip_definers" ], "users": False, "showProgress": False }), "copy should not throw")

EXPECT_STDOUT_CONTAINS("Database `sakila` had unsupported ENCRYPTION option commented out")
EXPECT_STDOUT_NOT_CONTAINS(f"User {test_user_account} had restricted privileges")
EXPECT_STDOUT_CONTAINS("Function `sakila`.`get_customer_balance` had definer clause removed")
EXPECT_STDOUT_CONTAINS("Function `sakila`.`get_customer_balance` had SQL SECURITY characteristic set to INVOKER")

#@<> WL15298 - copy back
# WL15298_TSFR_1_4
clean_instance(src_session)
analyze_tables(tgt_session)
setup_session(MDS_URI)

EXPECT_NO_THROWS(lambda: util.copy_instance(__sandbox_uri1, { "users": False, "showProgress": False }), "copy should not throw")

#@<> Cleanup
clean_instance(tgt_session)

testutil.destroy_sandbox(__mysql_sandbox_port1)
