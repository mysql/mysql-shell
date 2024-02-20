#@ {DEF(MYSQLD57_PATH) and VER(<9.0.0)}
# Tests where dump is generated in one version and loaded in another


#@<> INCLUDE dump_utils.inc

#@<> Setup
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
            "local_infile": "1"
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

# Test dumping in MySQL 5.7 and loading in 8.0

#@<> Plain dump in 5.7, load in 8.0
shell.connect(__sandbox_uri1)
util.dump_instance(__tmp_dir+"/ldtest/dump1", { "checksum": True })

# BUG#35359364 - loading from 5.7 into 8.0 should not require the `ignoreVersion` option
shell.connect(__sandbox_uri4)
util.load_dump(__tmp_dir+"/ldtest/dump1")

EXPECT_STDOUT_CONTAINS("Target is MySQL "+version_80+". Dump was produced from MySQL "+version_57)
EXPECT_STDOUT_CONTAINS("NOTE: Destination MySQL version is newer than the one where the dump was created.")
EXPECT_STDOUT_CONTAINS("0 warnings were reported during the load.")

#@<> WL15947 - verify checksums 5.7 -> 8.*
shell.connect(__sandbox_uri4)
EXPECT_NO_THROWS(lambda: util.load_dump(__tmp_dir+"/ldtest/dump1", { "loadData": False, "loadDdl": False, "checksum": True }), "checksums should match")

#@<> Test dumping in MySQL 8.0 and loading in 5.7
shell.connect(__sandbox_uri2)
util.dump_instance(__tmp_dir+"/ldtest/dump2")

# BUG#35359364 - loading from 5.7 into 8.0 should not require the `ignoreVersion` option
shell.connect(__sandbox_uri3)
# Currently expected failure
EXPECT_THROWS(lambda: util.load_dump(__tmp_dir+"/ldtest/dump2"), "Util.load_dump: Error loading dump")
EXPECT_STDOUT_CONTAINS("Unknown collation: 'utf8mb4_0900_ai_ci'")

EXPECT_STDOUT_CONTAINS("Target is MySQL "+version_57+". Dump was produced from MySQL "+version_80)
EXPECT_STDOUT_CONTAINS("NOTE: Destination MySQL version is older than the one where the dump was created.")

#@<> Cleanup
testutil.destroy_sandbox(__mysql_sandbox_port1)
testutil.destroy_sandbox(__mysql_sandbox_port2)
testutil.destroy_sandbox(__mysql_sandbox_port3)
testutil.destroy_sandbox(__mysql_sandbox_port4)
testutil.rmdir(__tmp_dir+"/ldtest", True)
