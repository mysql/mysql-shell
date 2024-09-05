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

tgt_version = tgt_session.run_sql('SELECT @@version').fetch_one()[0]

encryption_commented_out_msg = "Database `sakila` had unsupported ENCRYPTION option commented out"
restricted_privileges_msg = f"User {test_user_account} is granted restricted privileges"
restricted_privileges_removed_msg = f"User {test_user_account} had restricted privileges"

#@<> WL15298_TSFR_4_4_2
clean_instance(tgt_session)
setup_session(__sandbox_uri1)

EXPECT_THROWS(lambda: util.copy_instance(MDS_URI, { "excludeUsers": [ "root" ], "showProgress": False }), "Error: Shell Error (52004): Compatibility issues were found", "copy should throw")

# WL15887-TSFR_7_1 - version of target instance is used during compatibility checks
EXPECT_STDOUT_CONTAINS(f"Checking for compatibility with MySQL HeatWave Service {tgt_version[:tgt_version.find('-')]}")

EXPECT_STDOUT_CONTAINS(encryption_commented_out_msg)
EXPECT_STDOUT_CONTAINS(restricted_privileges_msg)

if not supports_set_any_definer_privilege(tgt_version):
    EXPECT_STDOUT_CONTAINS(strip_definers_definer_clause("sakila", "get_customer_balance", "Function").error(True))
    EXPECT_STDOUT_CONTAINS(strip_definers_security_clause("sakila", "get_customer_balance", "Function").error(True))

#@<> WL15298 - copy with users succeeds
# WL15298_TSFR_1_4
clean_instance(tgt_session)
setup_session(__sandbox_uri1)

EXPECT_NO_THROWS(lambda: util.copy_instance(MDS_URI, { "compatibility": [ "strip_definers", "strip_restricted_grants" ], "excludeUsers": [ "root" ], "showProgress": False }), "copy should not throw")

EXPECT_STDOUT_CONTAINS(encryption_commented_out_msg)
EXPECT_STDOUT_CONTAINS(restricted_privileges_removed_msg)
EXPECT_STDOUT_CONTAINS(strip_definers_definer_clause("sakila", "get_customer_balance", "Function").fixed(True))
EXPECT_STDOUT_CONTAINS(strip_definers_security_clause("sakila", "get_customer_balance", "Function").fixed(True))

#@<> WL15298_TSFR_4_4_3
clean_instance(tgt_session)
setup_session(__sandbox_uri1)

EXPECT_THROWS(lambda: util.copy_instance(MDS_URI, { "users": False, "showProgress": False }), "Error: Shell Error (52004): Compatibility issues were found", "copy should throw")

EXPECT_STDOUT_CONTAINS(encryption_commented_out_msg)
EXPECT_STDOUT_NOT_CONTAINS(restricted_privileges_msg)
EXPECT_STDOUT_NOT_CONTAINS(restricted_privileges_removed_msg)

if not supports_set_any_definer_privilege(tgt_version):
    EXPECT_STDOUT_CONTAINS(strip_definers_definer_clause("sakila", "get_customer_balance", "Function").error(True))
    EXPECT_STDOUT_CONTAINS(strip_definers_security_clause("sakila", "get_customer_balance", "Function").error(True))

#@<> WL15298 - copy without users succeeds
# WL15298_TSFR_1_4
clean_instance(tgt_session)
setup_session(__sandbox_uri1)

EXPECT_NO_THROWS(lambda: util.copy_instance(MDS_URI, { "compatibility": [ "strip_definers" ], "users": False, "showProgress": False }), "copy should not throw")

EXPECT_STDOUT_CONTAINS(encryption_commented_out_msg)
EXPECT_STDOUT_NOT_CONTAINS(restricted_privileges_msg)
EXPECT_STDOUT_NOT_CONTAINS(restricted_privileges_removed_msg)
EXPECT_STDOUT_CONTAINS(strip_definers_definer_clause("sakila", "get_customer_balance", "Function").fixed(True))
EXPECT_STDOUT_CONTAINS(strip_definers_security_clause("sakila", "get_customer_balance", "Function").fixed(True))

#@<> WL15298 - copy back
# WL15298_TSFR_1_4
clean_instance(src_session)
analyze_tables(tgt_session)
setup_session(MDS_URI)

EXPECT_NO_THROWS(lambda: util.copy_instance(__sandbox_uri1, { "users": False, "showProgress": False }), "copy should not throw")

#@<> Cleanup
clean_instance(tgt_session)

testutil.destroy_sandbox(__mysql_sandbox_port1)
