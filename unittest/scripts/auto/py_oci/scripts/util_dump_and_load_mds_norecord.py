#@ {has_oci_environment('MDS')}
# Test dump and load into/from OCI ObjectStorage

#@<> INCLUDE dump_utils.inc
#@<> INCLUDE oci_utils.inc

#@<> Setup

oci_config_file=os.path.join(OCI_CONFIG_HOME, "config")

datadir = __tmp_dir + "/mdstest-datadir-%s-%s" % (__version, __mysql_sandbox_port1)
testutil.mkdir(datadir)

testutil.deploy_sandbox(__mysql_sandbox_port1, "root", {'loose_innodb_directories': datadir}, {'enableKeyringPlugin':True})

# load test schemas
testutil.mkdir(datadir+"/testindexdir")
testutil.mkdir(datadir+"/test datadir")

testutil.preprocess_file(__data_path+"/sql/mysqlaas_compat.sql",
    ["TMPDIR="+datadir], datadir+"/mysqlaas_compat.sql")

testutil.import_data(__sandbox_uri1, datadir+"/mysqlaas_compat.sql")

# BUG#36552764 - check if PROXY privilege can be loaded into MDS
shell.connect(__sandbox_uri1)
session.run_sql("CREATE USER 'employee_ext'@'localhost' IDENTIFIED BY 'Pa$$W0rd'")
session.run_sql("CREATE USER 'employee'@'localhost' IDENTIFIED BY 'Pa$$W0rd!!!!'")
session.run_sql("GRANT PROXY ON 'employee'@'localhost' TO 'employee_ext'@'localhost'")

employee_ext_snapshot = snapshot_account(session, "employee_ext", "localhost")
print(employee_ext_snapshot)

def delete_progress_file(name=None, prefix=""):
    def prepend_prefix(prefix, name):
        result = name
        if prefix:
            if not prefix.endswith("/"):
                result = "/" + result
            result = prefix + result
        return result
    if name is None:
        name = list_oci_objects(OS_NAMESPACE, OS_BUCKET_NAME, prepend_prefix(prefix, "load-progress."))[0].name
        prefix = ""
    delete_object(OS_BUCKET_NAME, prepend_prefix(prefix, name), OS_NAMESPACE)

def cleanup_mds(s):
    session.run_sql("DROP SCHEMA IF EXISTS mysqlaas_compat")
    session.run_sql("DROP USER IF EXISTS 'employee_ext'@'localhost'")
    session.run_sql("DROP USER IF EXISTS 'employee'@'localhost'")

# ------------
# Defaults

#@<> Dump mysqlaas_compat to OS without MDS compatibility
prepare_empty_bucket(OS_BUCKET_NAME, OS_NAMESPACE)

shell.connect(__sandbox_uri1)
util.dump_schemas(["mysqlaas_compat"], '', {"osBucketName":OS_BUCKET_NAME, "osNamespace":OS_NAMESPACE, "ociConfigFile":oci_config_file})
EXPECT_STDOUT_CONTAINS("Schemas dumped: 1")

#@<> Loading incompatible mysqlaas_compat will fail
shell.connect(MDS_URI)
cleanup_mds(session)

EXPECT_THROWS(lambda: util.load_dump('', {'osBucketName':OS_BUCKET_NAME, 'osNamespace':OS_NAMESPACE}), "Dump is not compatible with MySQL HeatWave Service")

#@<> Dump mysqlaas_compat to OS with MDS compatibility
prepare_empty_bucket(OS_BUCKET_NAME, OS_NAMESPACE)

shell.connect(__sandbox_uri1)
util.dump_instance('',
                  {
                    "osBucketName":OS_BUCKET_NAME,
                    "osNamespace":OS_NAMESPACE,
                    "ociConfigFile":oci_config_file,
                    "ocimds": True,
                    "compatibility":['strip_definers', 'strip_restricted_grants', 'force_innodb', 'strip_tablespaces'],
                    "includeSchemas":["mysqlaas_compat"],
                    "includeUsers": ["'employee_ext'@'localhost'", "'employee'@'localhost'"]
                  })

EXPECT_STDOUT_CONTAINS("Schemas dumped: 1")

#@<> Loading mysqlaas_compat will now succeed
shell.connect(MDS_URI)
cleanup_mds(session)

util.load_dump('', {'loadUsers': True, 'osBucketName':OS_BUCKET_NAME, 'osNamespace':OS_NAMESPACE})
EXPECT_STDOUT_CONTAINS(f"Loading DDL, Data and Users from OCI ObjectStorage bucket={OS_BUCKET_NAME}, prefix='' using 4 threads.")
EXPECT_STDOUT_CONTAINS("0 warnings were reported during the load.")

delete_progress_file()

# BUG#36552764 - verification
EXPECT_JSON_EQ(employee_ext_snapshot, snapshot_account(session, "employee_ext", "localhost"))

#@<> BUG#32009225 use 'updateGtidSet' when loading a dump into MDS, set it to 'append'
shell.connect(MDS_URI)
cleanup_mds(session)

util.load_dump('', {'osBucketName':OS_BUCKET_NAME, 'osNamespace':OS_NAMESPACE, 'updateGtidSet': 'append'})
EXPECT_STDOUT_CONTAINS("0 warnings were reported during the load.")

delete_progress_file()

# NOTE: This test has been disabled as it doesn't work on an MDS instance where tests are executed
# once and again
#@<> BUG#32009225 use 'updateGtidSet' when loading a dump into MDS, set it to 'replace' {False}
shell.connect(MDS_URI)
cleanup_mds(session)

util.load_dump('', {'osBucketName':OS_BUCKET_NAME, 'osNamespace':OS_NAMESPACE, 'updateGtidSet': 'replace'})
EXPECT_STDOUT_CONTAINS("0 warnings were reported during the load.")

delete_progress_file()

#@<> BUG#36552764 - authentication_oci is an allowed authentication plugin
# NOTE: this also ensures that dumping from MHS with ocimds:true succeeds
prepare_empty_bucket(OS_BUCKET_NAME, OS_NAMESPACE)
shell.connect(MDS_URI)

EXPECT_NO_THROWS(lambda: util.dump_instance('mhs-test', {"ocimds": True, "osBucketName": OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile": oci_config_file}), "dump should not fail")

#@<> cleanup
shell.connect(MDS_URI)
cleanup_mds(session)
session.close()

testutil.rmdir(datadir, True)
testutil.destroy_sandbox(__mysql_sandbox_port1)
