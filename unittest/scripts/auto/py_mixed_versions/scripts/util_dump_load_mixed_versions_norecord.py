#@ {DEF(MYSQLD57_PATH) and VER(<9.0.0)}
# Tests where dump is generated in one version and loaded in another


#@<> INCLUDE dump_utils.inc

#@<> Setup
import os.path
import shutil
import time

try:
    testutil.rmdir(__tmp_dir+"/ldtest", True)
except:
    pass
testutil.mkdir(__tmp_dir+"/ldtest")

def prepare(sbport, version, load_data, mysqlaas=False):
    # load test schemas
    uri="mysql://root:root@localhost:%s"%sbport
    datadir=__tmp_dir+"/ldtest/datadirs%s-%s"%(sbport,version)
    testutil.mkdir(datadir)
    # 8.0 seems to create dirs automatically but not 5.7
    if version == 5:
        testutil.mkdir(datadir+"/testindexdir")
        testutil.mkdir(datadir+"/test datadir")
        testutil.deploy_sandbox(sbport, "root", {
            "loose_innodb_directories": datadir,
            "early_plugin_load": "keyring_file."+("dll" if __os_type=="windows" else "so"),
            "keyring_file_data": datadir+"/keyring",
            "local_infile": "1"
        }, {"mysqldPath": MYSQLD57_PATH})
    else:
        testutil.deploy_sandbox(sbport, "root", {
            "loose_innodb_directories": datadir,
            "early_plugin_load": "keyring_file."+("dll" if __os_type=="windows" else "so"),
            "keyring_file_data": datadir+"/keyring",
            "local_infile": "1",
            "loose_mysql_native_password": "OFF",
        })
    if load_data:
        testutil.import_data(uri, __data_path+"/sql/fieldtypes_all.sql", "", "utf8mb4") # xtest
        testutil.import_data(uri, __data_path+"/sql/sakila-schema.sql") # sakila
        testutil.import_data(uri, __data_path+"/sql/sakila-data.sql", "sakila") # sakila
        testutil.import_data(uri, __data_path+"/sql/misc_features.sql") # misc_features
        if mysqlaas:
            testutil.preprocess_file(__data_path+"/sql/mysqlaas_compat.sql",
                ["TMPDIR="+datadir], __tmp_dir+"/ldtest/mysqlaas_compat.sql")
            # Need to retry this one a few times because ENCRYPTION option requires keyring to be loaded,
            # but it may not yet be loaded.
            ok=False
            for i in range(5):
                try:
                    testutil.import_data(uri, __tmp_dir+"/ldtest/mysqlaas_compat.sql") # mysqlaas_compat
                    ok=True
                    break
                except Exception as error:
                    print(error)
                    # can fail if the keyring plugin didn't load yet, so we just retry
                    time.sleep(1)
                    print("Retrying...")
            EXPECT_TRUE(ok)
        shell.connect(uri)
        session.run_sql("CREATE USER loader@'%'")
        session.run_sql("GRANT ALL ON *.* TO loader@'%' WITH GRANT OPTION")
        session.run_sql("CREATE SCHEMA `empty`")
        session.run_sql("CREATE TABLE `empty`.`on_purpose` (a INT PRIMARY KEY)")

reference5=prepare(__mysql_sandbox_port1, 5, True)
prepare(__mysql_sandbox_port3, 5, False)

version_57=session.run_sql("SELECT @@version").fetch_one()[0]

reference8=prepare(__mysql_sandbox_port2, 8, True)
prepare(__mysql_sandbox_port4, 8, False)

version_80=session.run_sql("SELECT @@version").fetch_one()[0]

def EXPECT_DUMP_LOADED_IGNORE_ACCOUNTS(reference, session):
    EXPECT_JSON_EQ(strip_keys(strip_snapshot_data(reference), ["accounts"]), strip_keys(strip_snapshot_data(snapshot_instance(session)), ["accounts"]))

def EXPECT_DUMP_LOADED(reference, session):
    EXPECT_JSON_EQ(strip_snapshot_data(reference), strip_snapshot_data(snapshot_instance(session)))

def EXPECT_NO_CHANGES(session, before):
    EXPECT_JSON_EQ(before, snapshot_instance(session))

# 5.7 dump
shell.connect(__sandbox_uri1)
util.dump_instance(__tmp_dir+"/ldtest/dump1", { "checksum": True })

# current version dump
shell.connect(__sandbox_uri2)
util.dump_instance(__tmp_dir+"/ldtest/dump2")

# Test dumping in MySQL 5.7 and loading in 8.0

#@<> Plain dump in 5.7, load in 8.0 {VER(<9.0.0)}
# BUG#35359364 - loading from 5.7 into 8.0 should not require the `ignoreVersion` option
shell.connect(__sandbox_uri4)
util.load_dump(__tmp_dir+"/ldtest/dump1")

EXPECT_STDOUT_CONTAINS("Target is MySQL "+version_80+". Dump was produced from MySQL "+version_57)
EXPECT_STDOUT_CONTAINS("NOTE: Destination MySQL version is newer than the one where the dump was created.")
EXPECT_STDOUT_CONTAINS("0 warnings were reported during the load.")

#@<> WL15947 - verify checksums 5.7 -> 8.* {VER(<9.0.0)}
shell.connect(__sandbox_uri4)
EXPECT_NO_THROWS(lambda: util.load_dump(__tmp_dir+"/ldtest/dump1", { "loadData": False, "loadDdl": False, "checksum": True }), "checksums should match")

#@<> Test dumping in MySQL 8.0 and loading in 5.7 {VER(<9.0.0)}
# BUG#35359364 - loading from 5.7 into 8.0 should not require the `ignoreVersion` option
shell.connect(__sandbox_uri3)
# Currently expected failure
EXPECT_THROWS(lambda: util.load_dump(__tmp_dir+"/ldtest/dump2"), "Util.load_dump: Error loading dump")
EXPECT_STDOUT_CONTAINS("Unknown collation: 'utf8mb4_0900_ai_ci'")

EXPECT_STDOUT_CONTAINS("Target is MySQL "+version_57+". Dump was produced from MySQL "+version_80)
EXPECT_STDOUT_CONTAINS("NOTE: Destination MySQL version is older than the one where the dump was created.")

#@<> BUG#36552764 - update list of authentication plugins allowed in MHS
# constants
mysql_native_password_account = get_test_user_account("bug_36552764_mysql")
sha256_password_account = get_test_user_account("bug_36552764_sha256")
dump_dir = os.path.join(__tmp_dir, "ldtest", "bug_36552764")
dump_dir_ocimds = dump_dir + "_ocimds"

# setup
shell.connect(__sandbox_uri1)
session.run_sql(f"CREATE USER {mysql_native_password_account} IDENTIFIED WITH mysql_native_password BY 'pass'")
session.run_sql(f"CREATE USER {sha256_password_account} IDENTIFIED WITH sha256_password BY 'pass'")

EXPECT_NO_THROWS(lambda: util.dump_instance(dump_dir, { "users": True, "includeSchemas": [ "sakila" ], "includeUsers": [ mysql_native_password_account, sha256_password_account ] }), "dump should not fail")

#@<> BUG#36552764 - mysql_native_password and sha256_password should be reported as deprecated
shell.connect(__sandbox_uri1)
shutil.rmtree(dump_dir_ocimds, True)
EXPECT_NO_THROWS(lambda: util.dump_instance(dump_dir_ocimds, { "compatibility": ["strip_definers"], "targetVersion": "8.0.34", "ocimds": True, "users": True, "includeSchemas": [ "sakila" ], "includeUsers": [ mysql_native_password_account, sha256_password_account ], "ddlOnly": True }), "dump should not fail")
EXPECT_STDOUT_CONTAINS(deprecated_authentication_plugin(mysql_native_password_account, "mysql_native_password").warning())
EXPECT_STDOUT_CONTAINS(deprecated_authentication_plugin(sha256_password_account, "sha256_password").warning())

#@<> BUG#36552764 - sha256_password should be reported as deprecated, mysql_native_password with an extra message {__mysh_version_num >= 80400}
shell.connect(__sandbox_uri1)
shutil.rmtree(dump_dir_ocimds, True)
EXPECT_NO_THROWS(lambda: util.dump_instance(dump_dir_ocimds, { "targetVersion": "8.4.0", "ocimds": True, "users": True, "includeSchemas": [ "sakila" ], "includeUsers": [ mysql_native_password_account, sha256_password_account ], "ddlOnly": True }), "dump should not fail")
EXPECT_STDOUT_CONTAINS(deprecated_and_disabled_authentication_plugin(mysql_native_password_account).warning())
EXPECT_STDOUT_CONTAINS(deprecated_authentication_plugin(sha256_password_account, "sha256_password").warning())

#@<> BUG#36552764 - sha256_password should be reported as deprecated, mysql_native_password as an error {__mysh_version_num >= 90000}
shell.connect(__sandbox_uri1)
shutil.rmtree(dump_dir_ocimds, True)
EXPECT_THROWS(lambda: util.dump_instance(dump_dir_ocimds, { "targetVersion": "9.0.0", "ocimds": True, "users": True, "includeSchemas": [ "sakila" ], "includeUsers": [ mysql_native_password_account, sha256_password_account ], "ddlOnly": True }), "Compatibility issues were found")
EXPECT_STDOUT_CONTAINS(skip_invalid_accounts_plugin(mysql_native_password_account, "mysql_native_password").error())
EXPECT_STDOUT_CONTAINS(deprecated_authentication_plugin(sha256_password_account, "sha256_password").warning())

#@<> BUG#36552764 - sha256_password should be reported as deprecated, mysql_native_password is fixed {__mysh_version_num >= 90000}
shell.connect(__sandbox_uri1)
shutil.rmtree(dump_dir_ocimds, True)
EXPECT_NO_THROWS(lambda: util.dump_instance(dump_dir_ocimds, { "compatibility": ["skip_invalid_accounts"], "targetVersion": "9.0.0", "ocimds": True, "users": True, "includeSchemas": [ "sakila" ], "includeUsers": [ mysql_native_password_account, sha256_password_account ], "ddlOnly": True }), "dump should not fail")
EXPECT_STDOUT_CONTAINS(skip_invalid_accounts_plugin(mysql_native_password_account, "mysql_native_password").fixed())
EXPECT_STDOUT_CONTAINS(deprecated_authentication_plugin(sha256_password_account, "sha256_password").warning())

#@<> BUG#36552764 - loader should not ignore unknown plugin errors if target is not MHS {VER(>=8.4.0)}
shell.connect(__sandbox_uri4)
session.run_sql(f"DROP SCHEMA IF EXISTS sakila")
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "loadUsers": True, "ignoreVersion": True, "resetProgress": True }), "Plugin 'mysql_native_password' is not loaded")

#@<> BUG#36552764 - loader should ignore unknown plugin errors if target is MDS {VER(>=8.4.0) and not __dbug_off}
testutil.dbug_set("+d,dump_loader_force_mds")

shell.connect(__sandbox_uri4)
session.run_sql(f"DROP SCHEMA IF EXISTS sakila")
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "loadUsers": True, "ignoreVersion": True, "resetProgress": True }), "load should not fail")
EXPECT_STDOUT_CONTAINS(f"Plugin 'mysql_native_password' is not loaded: CREATE USER IF NOT EXISTS {mysql_native_password_account} IDENTIFIED WITH 'mysql_native_password' AS '****'")
EXPECT_STDOUT_CONTAINS(f"WARNING: Due to the above error the account {mysql_native_password_account} was not created, the load operation will continue.")
EXPECT_STDOUT_CONTAINS("1 accounts failed to load due to unsupported authentication plugin errors")

testutil.dbug_set("")

#@<> BUG#36552764 - cleanup
shell.connect(__sandbox_uri1)
session.run_sql(f"DROP USER {mysql_native_password_account}")
session.run_sql(f"DROP USER {sha256_password_account}")

#@<> BUG#36553849 - detect tables with non-standard foreign keys
# setup
tested_schema = "test_schema"
tested_table = "test_table"
dump_dir = os.path.join(__tmp_dir, "ldtest", "bug_36553849")

shell.connect(__sandbox_uri1)

session.run_sql("DROP SCHEMA IF EXISTS !", [tested_schema])
session.run_sql("CREATE SCHEMA !", [tested_schema])

session.run_sql(f"""CREATE TABLE !.`{tested_table}_parent` (
   a int,
   b int,
   PRIMARY KEY (a,b)
)
ENGINE=InnoDB""", [ tested_schema ])
session.run_sql(f"""CREATE TABLE !.! (
   c int PRIMARY KEY,
   a int,
   FOREIGN KEY (a) REFERENCES `{tested_table}_parent`(a)
)
ENGINE=InnoDB""", [ tested_schema, tested_table ])

#@<> BUG#36553849 - target server does not reject non-standard foreign keys
EXPECT_NO_THROWS(lambda: util.dump_schemas([tested_schema], dump_dir, { "ocimds": True, "targetVersion": "8.3.99", "showProgress": False }), "dump should not fail")

shutil.rmtree(dump_dir, True)

#@<> BUG#36553849 - target server rejects non-standard foreign keys
EXPECT_THROWS(lambda: util.dump_schemas([tested_schema], dump_dir, { "ocimds": True, "targetVersion": "8.4.0", "showProgress": False }), "Compatibility issues were found")
EXPECT_STDOUT_CONTAINS("""
ERROR: One or more tables with non-standard foreign keys (that reference non-unique keys or partial fields of composite keys) were found.

      Loading these tables into a DB System instance may fail, as instances are created with the 'restrict_fk_on_non_standard_key' system variable set to 'ON'.
      To continue with the dump you must do one of the following:

      * Fix the tables listed above, i.e. create the missing unique key.

      * Add the "force_non_standard_fks" to the "compatibility" option to ignore these issues. Please note that creation of foreign keys with non-standard keys may break the replication.
""")

shutil.rmtree(dump_dir, True)

#@<> BUG#36553849 - target server rejects non-standard foreign keys - compatibility option
EXPECT_NO_THROWS(lambda: util.dump_schemas([tested_schema], dump_dir, { "compatibility": ["force_non_standard_fks"], "ocimds": True, "targetVersion": "8.4.0", "showProgress": False }), "dump should not fail")

#@<> BUG#36553849 - target server rejects non-standard foreign keys - load the dump {VER(>=8.4.0)}
shell.connect(__sandbox_uri4)
session.run_sql("SET @saved_restrict_fk_on_non_standard_key = @@GLOBAL.restrict_fk_on_non_standard_key")
session.run_sql("SET @@GLOBAL.restrict_fk_on_non_standard_key = ON")

EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "showProgress": False }), "load should not fail")
EXPECT_STDOUT_CONTAINS("WARNING: The dump was created with the 'force_non_standard_fks' compatibility option set, the 'restrict_fk_on_non_standard_key' session variable will be set to 'OFF'. Please note that creation of foreign keys with non-standard keys may break the replication.")

session.run_sql("SET @@GLOBAL.restrict_fk_on_non_standard_key = @saved_restrict_fk_on_non_standard_key")

#@<> BUG#36553849 - cleanup
for uri in [__sandbox_uri1, __sandbox_uri4]:
    shell.connect(uri)
    session.run_sql("DROP SCHEMA IF EXISTS !", [tested_schema])

#@<> Cleanup
testutil.destroy_sandbox(__mysql_sandbox_port1)
testutil.destroy_sandbox(__mysql_sandbox_port2)
testutil.destroy_sandbox(__mysql_sandbox_port3)
testutil.destroy_sandbox(__mysql_sandbox_port4)
testutil.rmdir(__tmp_dir+"/ldtest", True)
