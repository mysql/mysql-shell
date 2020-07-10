#@ {has_oci_environment('OS')}
# Test dump and load into/from OCI ObjectStorage

#@<> INCLUDE oci_utils.inc

#@<> INCLUDE dump_utils.inc

#@<> Setup
import oci
import os
import json
import datetime

k_bucket_name="testbkt"
oci_config_file=os.path.join(OCI_CONFIG_HOME, "config")

testutil.deploy_sandbox(__mysql_sandbox_port1, "root")

session1=mysql.get_session(__sandbox_uri1)

session1.run_sql("create schema sample")
session1.run_sql("create table sample.data(id int, name varchar(20))")
session1.run_sql("insert into sample.data values (10, 'John Doe')")

# ------------
# Defaults
RFC3339 = True

shell.connect(__sandbox_uri1)

prefix = 'instance'

#@<> WL14154-TSFR2_2 Validate that the option ociParExpireTime only take string values with RFC3339 format
prepare_empty_bucket(k_bucket_name, OS_NAMESPACE)
EXPECT_THROWS(lambda:util.dump_instance(prefix, {"osBucketName":k_bucket_name, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "ociParManifest": True, "ociParExpireTime":"InvalidValue"}),
    "Util.dump_instance: Failed creating PAR for object '{}/@.json': Failed to create ObjectRead PAR for object {}/@.json: PAR expiration must conform to RFC 3339: InvalidValue".format(prefix, prefix))

#@<> WL14154-TSFR1_5 - Doing a dump to OCI with ocimds set to True. Validate that PAR objects are generated for each file of the dump.
prepare_empty_bucket(k_bucket_name, OS_NAMESPACE)
util.dump_instance(prefix, {"osBucketName":k_bucket_name, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "ocimds": True, "compatibility":["strip_restricted_grants"]})
validate_full_dump(OS_NAMESPACE, k_bucket_name, prefix, today_plus_days(7))

#@<> WL14154-TSFR1_6 - Doing a dump to OCI with ociParManifest set to True. Validate that PAR objects are generated for each file of the dump.
prepare_empty_bucket(k_bucket_name, OS_NAMESPACE)
util.dump_instance(prefix, {"osBucketName":k_bucket_name, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "ociParManifest": True})
validate_full_dump(OS_NAMESPACE, k_bucket_name, prefix, today_plus_days(7))

#@<> WL14154-TSFR2_3 - Doing a dump to OCI ociParManifest set to True and ociParExpireTime set to a valid value. Validate that the dump success and the expiration date for the PAR objects matches the set to the option ociParExpireTime.
prepare_empty_bucket(k_bucket_name, OS_NAMESPACE)
tomorrow = today_plus_days(1, RFC3339)
util.dump_instance(prefix, {"osBucketName":k_bucket_name, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "ociParManifest": True, "ociParExpireTime":tomorrow})
validate_full_dump(OS_NAMESPACE, k_bucket_name, prefix, tomorrow)

#@<> WL14154-TSFR2_8
prepare_empty_bucket(k_bucket_name, OS_NAMESPACE)
util.dump_instance(prefix, {"osBucketName":k_bucket_name, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "ocimds": True, "compatibility":["strip_restricted_grants"], "ociParManifest": False})
EXPECT_THROWS(lambda:testutil.download_oci_object(OS_NAMESPACE, k_bucket_name, prefix + '/@.manifest.json', "@.manifest.json"),
    "Testutils.download_oci_object: Failed opening object '{}/@.manifest.json' in READ mode: Not Found (404)".format(prefix))

#@<> WL14154-TSFR2_9 - Doing a dump to OCI ociParManifest not set, ocimds set to True and ociParExpireTime set to a valid value. Validate that the dump success and the expiration date for the PAR objects matches the set to the option ociParExpireTime.
prepare_empty_bucket(k_bucket_name, OS_NAMESPACE)
expire_time = today_plus_days(7, RFC3339)
util.dump_instance(prefix, {"osBucketName":k_bucket_name, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "ocimds": True, "compatibility":["strip_restricted_grants"], "ociParExpireTime": expire_time})
validate_full_dump(OS_NAMESPACE, k_bucket_name, prefix, expire_time)

#@<> WL14154-TSFR2_10 - Doing a dump to OCI ociParManifest set to False, ocimds set to True and ociParExpireTime set to a valid value. Validate that the dump fail because ociParExpireTime it's valid only when ociParManifest is set to True.
prepare_empty_bucket(k_bucket_name, OS_NAMESPACE)
EXPECT_THROWS(lambda:util.dump_instance(prefix, {"osBucketName":k_bucket_name, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "ocimds": True, "compatibility":["strip_restricted_grants"], "ociParManifest": False, "ociParExpireTime":today_plus_days(1, RFC3339)}),
    "Util.dump_instance: The option 'ociParExpireTime' cannot be used when the value of 'ociParManifest' option is not True.")

#@<> WL14154-TSFR3_2 - ZSTD compression {not __dbug_off and not __recording and not __replaying}
testutil.set_trap("par_manifest", ["name == {0}/sample@data.tsv.zst".format(prefix)], {"code": 404, "msg": "Injected failure"})

prepare_empty_bucket(k_bucket_name, OS_NAMESPACE)
EXPECT_THROWS(lambda:util.dump_instance(prefix, {"osBucketName":k_bucket_name, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "ociParManifest": True, "chunking": False}),
    "RuntimeError: Util.dump_instance: Fatal error during dump")
EXPECT_STDOUT_CONTAINS("Failed creating PAR for object 'instance/sample@data.tsv.zst': Injected failure")

testutil.clear_traps("par_manifest")

#@<> WL14154-TSFR3_2 - GZIP compression {not __dbug_off and not __recording and not __replaying}
testutil.set_trap("par_manifest", ["name == {0}/sample@data.tsv.gz".format(prefix)], {"code": 404, "msg": "Injected failure"})

prepare_empty_bucket(k_bucket_name, OS_NAMESPACE)
EXPECT_THROWS(lambda:util.dump_instance(prefix, {"osBucketName":k_bucket_name, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "ociParManifest": True, "chunking": False, "compression": "gzip"}),
    "RuntimeError: Util.dump_instance: Fatal error during dump")
EXPECT_STDOUT_CONTAINS("Failed creating PAR for object 'instance/sample@data.tsv.gz': Injected failure")

testutil.clear_traps("par_manifest")

#@<> WL14154-TSFR3_2 - no compression {not __dbug_off and not __recording and not __replaying}
testutil.set_trap("par_manifest", ["name == {0}/sample@data.tsv".format(prefix)], {"code": 404, "msg": "Injected failure"})

prepare_empty_bucket(k_bucket_name, OS_NAMESPACE)
EXPECT_THROWS(lambda:util.dump_instance(prefix, {"osBucketName":k_bucket_name, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "ociParManifest": True, "chunking": False, "compression": "none"}),
    "RuntimeError: Util.dump_instance: Fatal error during dump")
EXPECT_STDOUT_CONTAINS("Failed creating PAR for object 'instance/sample@data.tsv': Injected failure")

testutil.clear_traps("par_manifest")

#@<> WL14154-TSFR3_2 - one of the text files {not __dbug_off and not __recording and not __replaying}
testutil.set_trap("par_manifest", ["name == {0}/sample@data.json".format(prefix)], {"code": 404, "msg": "Injected failure"})

prepare_empty_bucket(k_bucket_name, OS_NAMESPACE)
EXPECT_THROWS(lambda:util.dump_instance(prefix, {"osBucketName":k_bucket_name, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "ociParManifest": True}),
    "RuntimeError: Util.dump_instance: Fatal error during dump")
EXPECT_STDOUT_CONTAINS("Failed creating PAR for object 'instance/sample@data.json': Injected failure")

testutil.clear_traps("par_manifest")

#@<> Cleanup
testutil.destroy_sandbox(__mysql_sandbox_port1)
