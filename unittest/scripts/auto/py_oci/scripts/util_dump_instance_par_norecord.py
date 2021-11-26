#@ {has_oci_environment('OS')}
# Test dump and load into/from OCI ObjectStorage

#@<> INCLUDE oci_utils.inc

#@<> INCLUDE dump_utils.inc

#@<> Setup
import oci
import os
import json
import datetime

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
prepare_empty_bucket(OS_BUCKET_NAME, OS_NAMESPACE)
EXPECT_THROWS(lambda:util.dump_instance(prefix, {"osBucketName":OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "ociParManifest": True, "ociParExpireTime":"InvalidValue"}),
    "Error: Shell Error (52011): Util.dump_instance: While 'Gathering information': Failed creating PAR for object '{}/@.json': Failed to create ObjectRead PAR for object {}/@.json: PAR expiration must conform to RFC 3339: InvalidValue".format(prefix, prefix))

#@<> Doing a dump to OCI with ocimds set to True. The ociParManifest is not set and defaults to false, manifest is not created.
prepare_empty_bucket(OS_BUCKET_NAME, OS_NAMESPACE)
util.dump_instance(prefix, {"osBucketName":OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "ocimds": True, "compatibility":["strip_restricted_grants", "ignore_missing_pks"]})
EXPECT_THROWS(lambda:testutil.download_oci_object(OS_NAMESPACE, OS_BUCKET_NAME, prefix + '/@.manifest.json', "@.manifest.json"),
    f"Error: Shell Error (54404): Testutils.download_oci_object: Failed opening object '{prefix}/@.manifest.json' in READ mode: Failed to get summary for object '{prefix}/@.manifest.json': Not Found (404)")

#@<> WL14154-TSFR1_6 - Doing a dump to OCI with ociParManifest set to True. Validate that PAR objects are generated for each file of the dump.
prepare_empty_bucket(OS_BUCKET_NAME, OS_NAMESPACE)
util.dump_instance(prefix, {"osBucketName":OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "ociParManifest": True})
validate_full_dump(OS_NAMESPACE, OS_BUCKET_NAME, prefix, today_plus_days(7))

#@<> WL14154-TSFR2_3 - Doing a dump to OCI ociParManifest set to True and ociParExpireTime set to a valid value. Validate that the dump success and the expiration date for the PAR objects matches the set to the option ociParExpireTime.
prepare_empty_bucket(OS_BUCKET_NAME, OS_NAMESPACE)
tomorrow = today_plus_days(1, RFC3339)
util.dump_instance(prefix, {"osBucketName":OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "ociParManifest": True, "ociParExpireTime":tomorrow})
validate_full_dump(OS_NAMESPACE, OS_BUCKET_NAME, prefix, tomorrow)

#@<> WL14154-TSFR2_8
prepare_empty_bucket(OS_BUCKET_NAME, OS_NAMESPACE)
util.dump_instance(prefix, {"osBucketName":OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "ocimds": True, "compatibility":["strip_restricted_grants", "ignore_missing_pks"], "ociParManifest": False})
EXPECT_THROWS(lambda:testutil.download_oci_object(OS_NAMESPACE, OS_BUCKET_NAME, prefix + '/@.manifest.json', "@.manifest.json"),
    f"Error: Shell Error (54404): Testutils.download_oci_object: Failed opening object '{prefix}/@.manifest.json' in READ mode: Failed to get summary for object '{prefix}/@.manifest.json': Not Found (404)")

#@<> Doing a dump to OCI ociParManifest not set, ocimds set to True and ociParExpireTime set to a valid value. The ociParManifest is not set and defaults to false, ociParExpireTime cannot be used.
prepare_empty_bucket(OS_BUCKET_NAME, OS_NAMESPACE)
expire_time = today_plus_days(7, RFC3339)
EXPECT_THROWS(lambda:util.dump_instance(prefix, {"osBucketName":OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "ocimds": True, "compatibility":["strip_restricted_grants", "ignore_missing_pks"], "ociParExpireTime": expire_time}),
    "Util.dump_instance: Argument #2: The option 'ociParExpireTime' cannot be used when the value of 'ociParManifest' option is not True.")

#@<> WL14154-TSFR2_10 - Doing a dump to OCI ociParManifest set to False, ocimds set to True and ociParExpireTime set to a valid value. Validate that the dump fail because ociParExpireTime it's valid only when ociParManifest is set to True.
prepare_empty_bucket(OS_BUCKET_NAME, OS_NAMESPACE)
EXPECT_THROWS(lambda:util.dump_instance(prefix, {"osBucketName":OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "ocimds": True, "compatibility":["strip_restricted_grants", "ignore_missing_pks"], "ociParManifest": False, "ociParExpireTime":today_plus_days(1, RFC3339)}),
    "Util.dump_instance: Argument #2: The option 'ociParExpireTime' cannot be used when the value of 'ociParManifest' option is not True.")

#@<> WL14154-TSFR3_2 - ZSTD compression {not __dbug_off}
testutil.set_trap("par_manifest", ["name == {0}/sample@data.tsv.zst".format(prefix)], {"code": 404, "msg": "Injected exception"})

prepare_empty_bucket(OS_BUCKET_NAME, OS_NAMESPACE)
EXPECT_THROWS(lambda:util.dump_instance(prefix, {"osBucketName":OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "ociParManifest": True, "chunking": False}),
    re.compile(r"Error: Shell Error \(52006\): Util.dump_instance: While '.*': Fatal error during dump"))
EXPECT_STDOUT_CONTAINS("Failed creating PAR for object '{0}/sample@data.tsv.zst': Injected exception".format(prefix))

testutil.clear_traps("par_manifest")

#@<> WL14154-TSFR3_2 - GZIP compression {not __dbug_off}
testutil.set_trap("par_manifest", ["name == {0}/sample@data.tsv.gz".format(prefix)], {"code": 404, "msg": "Injected exception"})

prepare_empty_bucket(OS_BUCKET_NAME, OS_NAMESPACE)
EXPECT_THROWS(lambda:util.dump_instance(prefix, {"osBucketName":OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "ociParManifest": True, "chunking": False, "compression": "gzip"}),
    re.compile(r"Error: Shell Error \(52006\): Util.dump_instance: While '.*': Fatal error during dump"))
EXPECT_STDOUT_CONTAINS("Failed creating PAR for object '{0}/sample@data.tsv.gz': Injected exception".format(prefix))

testutil.clear_traps("par_manifest")

#@<> WL14154-TSFR3_2 - no compression {not __dbug_off}
testutil.set_trap("par_manifest", ["name == {0}/sample@data.tsv".format(prefix)], {"code": 404, "msg": "Injected exception"})

prepare_empty_bucket(OS_BUCKET_NAME, OS_NAMESPACE)
EXPECT_THROWS(lambda:util.dump_instance(prefix, {"osBucketName":OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "ociParManifest": True, "chunking": False, "compression": "none"}),
    re.compile(r"Error: Shell Error \(52006\): Util.dump_instance: While '.*': Fatal error during dump"))
EXPECT_STDOUT_CONTAINS("Failed creating PAR for object '{0}/sample@data.tsv': Injected exception".format(prefix))

testutil.clear_traps("par_manifest")

#@<> WL14154-TSFR3_2 - one of the text files {not __dbug_off}
testutil.set_trap("par_manifest", ["name == {0}/sample.json".format(prefix)], {"code": 404, "msg": "Injected exception"})

prepare_empty_bucket(OS_BUCKET_NAME, OS_NAMESPACE)
EXPECT_THROWS(lambda:util.dump_instance(prefix, {"osBucketName":OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "ociParManifest": True}),
    re.compile(r"Error: Shell Error \(52006\): Util.dump_instance: While 'Writing .*': Fatal error during dump"))
EXPECT_STDOUT_CONTAINS("Failed creating PAR for object '{0}/sample.json': Injected exception".format(prefix))

testutil.clear_traps("par_manifest")

#@<> WL14841-TSFR_3_1 - @.json file {not __dbug_off}
# WL14841-TSFR_3_3
testutil.set_trap("oci_put_object", [f"name == {prefix}/@.json"], {"code": 404, "msg": "Injected exception"})

prepare_empty_bucket(OS_BUCKET_NAME, OS_NAMESPACE)
EXPECT_THROWS(lambda:util.dump_instance(prefix, {"osBucketName": OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "ociParManifest": True}),
    f"Error: Shell Error (54404): Util.dump_instance: While 'Gathering information': Failed to put object '{prefix}/@.json': Injected exception (404)")

testutil.clear_traps("oci_put_object")
#@<> Cleanup
testutil.destroy_sandbox(__mysql_sandbox_port1)
