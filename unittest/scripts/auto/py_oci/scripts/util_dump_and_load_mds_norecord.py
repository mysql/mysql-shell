#@ {has_oci_environment('MDS')}
# Test dump and load into/from OCI ObjectStorage

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

# ------------
# Defaults

#@<> Dump mysqlaas_compat to OS without MDS compatibility
prepare_empty_bucket(OS_BUCKET_NAME, OS_NAMESPACE)

shell.connect(__sandbox_uri1)
util.dump_schemas(["mysqlaas_compat"], '', {"osBucketName":OS_BUCKET_NAME, "osNamespace":OS_NAMESPACE, "ociConfigFile":oci_config_file})
EXPECT_STDOUT_CONTAINS("Schemas dumped: 1")
WIPE_OUTPUT()

#@<> Loading incompatible mysqlaas_compat will fail
execute_oci_shell("--sql -e \"drop schema if exists mysqlaas_compat\"")
execute_oci_shell("-e \"util.loadDump('', {osBucketName:'%s', osNamespace:'%s'})\"" % (OS_BUCKET_NAME, OS_NAMESPACE))
EXPECT_STDERR_CONTAINS("Util.loadDump: Dump is not MDS compatible (RuntimeError)")
WIPE_OUTPUT()

#@<> Dump mysqlaas_compat to OS with MDS compatibility
prepare_empty_bucket(OS_BUCKET_NAME, OS_NAMESPACE)

shell.connect(__sandbox_uri1)
util.dump_schemas(["mysqlaas_compat"], '',
                  { "osBucketName":OS_BUCKET_NAME,
                    "osNamespace":OS_NAMESPACE,
                    "ociConfigFile":oci_config_file,
                    "ocimds": True,
                    "compatibility":['strip_definers', 'strip_restricted_grants', 'force_innodb', 'strip_tablespaces']})

EXPECT_STDOUT_CONTAINS("Schemas dumped: 1")
WIPE_OUTPUT()

#@<> Loading incompatible mysqlaas_compat will succeed
execute_oci_shell("--sql -e \"drop schema if exists mysqlaas_compat\"")
execute_oci_shell("-e \"util.loadDump('', {osBucketName:'%s', osNamespace:'%s'})\"" % (OS_BUCKET_NAME, OS_NAMESPACE))
EXPECT_STDERR_CONTAINS("Loading DDL and Data from OCI ObjectStorage bucket=mds-dump-test-bucket, prefix='' using 4 threads.")
EXPECT_STDERR_CONTAINS("0 warnings were reported during the load.")
WIPE_OUTPUT()
delete_progress_file()

#@<> BUG#32009225 use 'updateGtidSet' when loading a dump into MDS, set it to 'append'
execute_oci_shell("--sql -e \"drop schema if exists mysqlaas_compat\"")
execute_oci_shell("-e \"util.loadDump('', {osBucketName:'{0}', osNamespace:'{1}', 'updateGtidSet': 'append'})\"".format(OS_BUCKET_NAME, OS_NAMESPACE))
EXPECT_STDERR_CONTAINS("0 warnings were reported during the load.")
WIPE_OUTPUT()
delete_progress_file()

#@<> BUG#32009225 use 'updateGtidSet' when loading a dump into MDS, set it to 'replace'
execute_oci_shell("--sql -e \"drop schema if exists mysqlaas_compat\"")
execute_oci_shell("-e \"util.loadDump('', {osBucketName:'{0}', osNamespace:'{1}', 'updateGtidSet': 'replace'})\"".format(OS_BUCKET_NAME, OS_NAMESPACE))
EXPECT_STDERR_CONTAINS("0 warnings were reported during the load.")
WIPE_OUTPUT()
delete_progress_file()

#@<> cleanup
delete_bucket(OS_BUCKET_NAME, OS_NAMESPACE)
testutil.rmdir(datadir, True)
testutil.destroy_sandbox(__mysql_sandbox_port1)
