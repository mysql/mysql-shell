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
session1.run_sql("create table sample.data_copy(id int, name varchar(20))")
session1.run_sql("insert into sample.data_copy values (5, 'Jane Doe')")
session1.close()

# ------------
# Defaults
RFC3339 = True
shell.connect(__sandbox_uri1)

prepare_empty_bucket(OS_BUCKET_NAME, OS_NAMESPACE)

PREPARE_PAR_IS_SECRET_TEST()
# not using --osNamespace=OS_NAMESPACE here, to trigger automatic detection of the namespace
EXPECT_EQ(0, testutil.call_mysqlsh([__sandbox_uri1, "--log-level=8", "--", "util", "dump-instance", "shell-test", "--showProgress", "true", "--osBucketName", OS_BUCKET_NAME, "--ociConfigFile", oci_config_file, "--ociParManifest", "true"]))
EXPECT_PAR_IS_SECRET()

# BUG#33181308 - shell should display an information that it is writing the manifest before the summary
EXPECT_STDOUT_MATCHES(re.compile(r'Writing manifest - done.*Dump duration: ', re.DOTALL))

testutil.deploy_sandbox(__mysql_sandbox_port2, "root", {"local_infile":1})
shell.connect(__sandbox_uri2)

manifest_par=create_par(OS_NAMESPACE, OS_BUCKET_NAME, "ObjectRead", "manifest-par", today_plus_days(1, RFC3339), "shell-test/@.manifest.json")

#@<> WL14154-TSFR7_1 - When doing a load using a PAR for a manifest file with the progressFile option not set. Validate that the load fail because progressFile option is mandatory.
PREPARE_PAR_IS_SECRET_TEST()
EXPECT_THROWS(lambda:util.load_dump(manifest_par), "Util.load_dump: When using a PAR to load a dump, the progressFile option must be defined")
EXPECT_PAR_IS_SECRET()

#@<> Attempt to load using a manifest PAR as progress PAR.
PREPARE_PAR_IS_SECRET_TEST()
EXPECT_THROWS(lambda:util.load_dump(manifest_par, {"progressFile": manifest_par}), "Util.load_dump: Invalid PAR for progress file, use a PAR to a specific file different than the manifest")
EXPECT_PAR_IS_SECRET()

#@<> WL14154-TSFR7_2 - When doing a load using a PAR for a manifest file with the progressFile option set to a file system. Validate that the load success and the file given stores the load progress.
remove_local_progress_file()

PREPARE_PAR_IS_SECRET_TEST()
EXPECT_NO_THROWS(lambda: util.load_dump(manifest_par, {"progressFile": local_progress_file}), "load_dump() using local progress file")
EXPECT_PAR_IS_SECRET()

EXPECT_STDOUT_CONTAINS("2 tables in 1 schemas were loaded")
validate_load_progress(local_progress_file)
session.run_sql("drop schema if exists sample")

#@<> WL14154-TSFR8_1 - When doing a load using a PAR for a manifest file with the progressFile option set to a file system. Validate that the load success and the file given stores the load progress.
progress_par=create_par(OS_NAMESPACE, OS_BUCKET_NAME, "ObjectReadWrite", "manifest-par", today_plus_days(1, RFC3339), "shell-test/par-load-progress.json")

PREPARE_PAR_IS_SECRET_TEST()
EXPECT_NO_THROWS(lambda: util.load_dump(manifest_par, {"progressFile": progress_par}), "load_dump() using PAR progress file")
EXPECT_PAR_IS_SECRET()

EXPECT_STDOUT_CONTAINS("2 tables in 1 schemas were loaded")
testutil.download_oci_object(OS_NAMESPACE, OS_BUCKET_NAME, "shell-test/par-load-progress.json", "par-load-progress.json")
validate_load_progress("par-load-progress.json")
session.run_sql("drop schema if exists sample")
os.remove("par-load-progress.json")
delete_object(OS_BUCKET_NAME, "shell-test/par-load-progress.json", OS_NAMESPACE)

#@<> WL14154-TSFR9_1 - When doing a load using a PAR for a manifest file with the progressFile option set to a read/write PAR object for a file that is not in the same bucket where the dump is. Validate that the load fails because the file to store the progress is not in the same bucket.
prepare_empty_bucket(OS_BUCKET_NAME + '-par-test', OS_NAMESPACE)
progress_par=create_par(OS_NAMESPACE, OS_BUCKET_NAME + '-par-test', "ObjectReadWrite", "manifest-par", today_plus_days(1, RFC3339), "shell-test/another-bucket-progress.json")

PREPARE_PAR_IS_SECRET_TEST()
EXPECT_THROWS(lambda:util.load_dump(manifest_par, {"progressFile": progress_par}), "Util.load_dump: The provided PAR must be a file on the dump location:")
EXPECT_PAR_IS_SECRET()

delete_bucket(OS_BUCKET_NAME + '-par-test', OS_NAMESPACE)

#@<> WL14154-TSFR9_2 - When doing a load using a PAR for a manifest file with the progressFile option set to a read/write PAR object for a file that is in the same bucket but in diferent location where the dump is. Validate that the load fails because the file to store the progress is not in the same location in the bucket.
progress_par=create_par(OS_NAMESPACE, OS_BUCKET_NAME, "ObjectReadWrite", "manifest-par", today_plus_days(1, RFC3339), "same-bucket-different-prefix.json")

PREPARE_PAR_IS_SECRET_TEST()
EXPECT_THROWS(lambda:util.load_dump(manifest_par, {"progressFile": progress_par}), "Util.load_dump: The provided PAR must be a file on the dump location:")
EXPECT_PAR_IS_SECRET()

#@<> WL14154-TSFR9_4 - When doing a load using a PAR for a manifest file with the progressFile option set to a read PAR object for a file that is in the same bucket and location where the dump is. Validate that the load fails because the file to store the progress has not the right permissions.
WIPE_SHELL_LOG()

progress_par=create_par(OS_NAMESPACE, OS_BUCKET_NAME, "ObjectRead", "manifest-par", today_plus_days(1, RFC3339), "shell-test/read-only-progress.json")

PREPARE_PAR_IS_SECRET_TEST()
EXPECT_THROWS(lambda:util.load_dump(manifest_par, {"progressFile": progress_par}), "RuntimeError: Util.load_dump: Failed to put object")
EXPECT_PAR_IS_SECRET()

# BUG#32593125 - check if details of failed operation are logged
EXPECT_SHELL_LOG_CONTAINS(f"Warning: Request failed: PUT /p/<secret>/n/{OS_NAMESPACE}/b/{OS_BUCKET_NAME}/o/shell-test/read-only-progress.json (404 - Not Found)")
EXPECT_SHELL_LOG_CONTAINS("Info: REQUEST HEADERS:")
EXPECT_SHELL_LOG_CONTAINS("Info: RESPONSE HEADERS:")
EXPECT_SHELL_LOG_CONTAINS("Info: RESPONSE BODY:")
EXPECT_SHELL_LOG_CONTAINS(f"""{{"code":"BucketNotFound","message":"Either the bucket named '{OS_BUCKET_NAME}' does not exist in the namespace '{OS_NAMESPACE}' or you are not authorized to access it"}}""")

#@<> WL14154-TSFR6_1
# create PAR for the progress file
progress_par=create_par(OS_NAMESPACE, OS_BUCKET_NAME, "ObjectReadWrite", "manifest-par", today_plus_days(1, RFC3339), "shell-test/par-load-progress.json")

# download the manifest, remove some PARs, overwrite the manifest in the bucket
testutil.anycopy({"osBucketName":OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "name":"shell-test/@.manifest.json"}, "@.manifest.json")

with open("@.manifest.json", encoding="utf-8") as json_file:
    manifest = json.load(json_file)

del manifest["endTime"]
manifest["lastUpdate"] = manifest["startTime"]
manifest["contents"] = manifest["contents"][:int(len(manifest["contents"]) / 2)]

with open("@.manifest.json.partial", "w", encoding="utf-8") as json_file:
    json.dump(manifest, json_file, indent=4)

testutil.anycopy("@.manifest.json.partial", {"osBucketName":OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "name":"shell-test/@.manifest.json"})

# incomplete dump should require the 'waitDumpTimeout' option
PREPARE_PAR_IS_SECRET_TEST()
EXPECT_THROWS(lambda: util.load_dump(manifest_par, {"progressFile": "progress.txt"}), "RuntimeError: Util.load_dump: Incomplete dump")
EXPECT_PAR_IS_SECRET()

# asynchronously start the load process
proc = testutil.call_mysqlsh_async([__sandbox_uri2, "--py", "-e", "util.load_dump('{0}', {{'progressFile': '{1}', 'waitDumpTimeout': 60}})".format(manifest_par, progress_par)])

# wait a bit and reupload the manifest with all the PARs
time.sleep(5)
testutil.anycopy("@.manifest.json", {"osBucketName":OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "name":"shell-test/@.manifest.json"})

# wait for the upload to finish (should catch up with the full manifest)
testutil.wait_mysqlsh_async(proc)

# verify if everything was OK
EXPECT_STDOUT_CONTAINS("2 tables in 1 schemas were loaded")
testutil.download_oci_object(OS_NAMESPACE, OS_BUCKET_NAME, "shell-test/par-load-progress.json", "par-load-progress.json")
validate_load_progress("par-load-progress.json")

# cleanup
session.run_sql("drop schema if exists sample")
os.remove("par-load-progress.json")
os.remove("@.manifest.json")
os.remove("@.manifest.json.partial")
delete_object(OS_BUCKET_NAME, "shell-test/par-load-progress.json", OS_NAMESPACE)

#@<> BUG#31606223 - read only, progress file does not exist
progress_par=create_par(OS_NAMESPACE, OS_BUCKET_NAME, "ObjectRead", "manifest-par", today_plus_days(1, RFC3339), "shell-test/par-load-progress.json")

PREPARE_PAR_IS_SECRET_TEST()
EXPECT_THROWS(lambda: util.load_dump(manifest_par, {"progressFile": progress_par}), f"RuntimeError: Util.load_dump: Failed to put object '/p/<secret>/n/{OS_NAMESPACE}/b/{OS_BUCKET_NAME}/o/shell-test/par-load-progress.json': Either the bucket named '{OS_BUCKET_NAME}' does not exist in the namespace '{OS_NAMESPACE}' or you are not authorized to access it (404)")
EXPECT_PAR_IS_SECRET()

#@<> BUG#31606223 - read only, empty progress file
progress_par=create_par(OS_NAMESPACE, OS_BUCKET_NAME, "ObjectRead", "manifest-par", today_plus_days(1, RFC3339), "shell-test/par-load-progress.json")
open("par-load-progress.json", "w").close()
testutil.anycopy("par-load-progress.json", {"osBucketName":OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "name":"shell-test/par-load-progress.json"})

PREPARE_PAR_IS_SECRET_TEST()
EXPECT_THROWS(lambda: util.load_dump(manifest_par, {"progressFile": progress_par}), f"RuntimeError: Util.load_dump: Failed to put object '/p/<secret>/n/{OS_NAMESPACE}/b/{OS_BUCKET_NAME}/o/shell-test/par-load-progress.json': Either the bucket named '{OS_BUCKET_NAME}' does not exist in the namespace '{OS_NAMESPACE}' or you are not authorized to access it (404)")
EXPECT_PAR_IS_SECRET()

os.remove("par-load-progress.json")

#@<> BUG#31606223 - write only, progress file does not exist
# this is a success, because reading from write-only PAR results in 404, code assumes that the file does not exist and continues with the load process
progress_par=create_par(OS_NAMESPACE, OS_BUCKET_NAME, "ObjectWrite", "manifest-par", today_plus_days(1, RFC3339), "shell-test/par-load-progress.json")

PREPARE_PAR_IS_SECRET_TEST()
EXPECT_NO_THROWS(lambda: util.load_dump(manifest_par, {"progressFile": progress_par}), "load_dump() using PAR progress file")
EXPECT_PAR_IS_SECRET()

EXPECT_STDOUT_CONTAINS("2 tables in 1 schemas were loaded")
testutil.download_oci_object(OS_NAMESPACE, OS_BUCKET_NAME, "shell-test/par-load-progress.json", "par-load-progress.json")
validate_load_progress("par-load-progress.json")
session.run_sql("drop schema if exists sample")
os.remove("par-load-progress.json")
delete_object(OS_BUCKET_NAME, "shell-test/par-load-progress.json", OS_NAMESPACE)

#@<> BUG#31606223 - write only, empty progress file
# this is a success, because reading from write-only PAR results in 404, code assumes that the file does not exist and continues with the load process
progress_par=create_par(OS_NAMESPACE, OS_BUCKET_NAME, "ObjectWrite", "manifest-par", today_plus_days(1, RFC3339), "shell-test/par-load-progress.json")
open("par-load-progress.json", "w").close()
testutil.anycopy("par-load-progress.json", {"osBucketName":OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "name":"shell-test/par-load-progress.json"})

PREPARE_PAR_IS_SECRET_TEST()
EXPECT_NO_THROWS(lambda: util.load_dump(manifest_par, {"progressFile": progress_par}), "load_dump() using PAR progress file")
EXPECT_PAR_IS_SECRET()

EXPECT_STDOUT_CONTAINS("2 tables in 1 schemas were loaded")
testutil.download_oci_object(OS_NAMESPACE, OS_BUCKET_NAME, "shell-test/par-load-progress.json", "par-load-progress.json")
validate_load_progress("par-load-progress.json")
session.run_sql("drop schema if exists sample")
os.remove("par-load-progress.json")
delete_object(OS_BUCKET_NAME, "shell-test/par-load-progress.json", OS_NAMESPACE)

#@<> BUG#31606223 - read-write, progress file does not exist
progress_par=create_par(OS_NAMESPACE, OS_BUCKET_NAME, "ObjectReadWrite", "manifest-par", today_plus_days(1, RFC3339), "shell-test/par-load-progress.json")

PREPARE_PAR_IS_SECRET_TEST()
EXPECT_NO_THROWS(lambda: util.load_dump(manifest_par, {"progressFile": progress_par}), "load_dump() using PAR progress file")
EXPECT_PAR_IS_SECRET()

EXPECT_STDOUT_CONTAINS("2 tables in 1 schemas were loaded")
testutil.download_oci_object(OS_NAMESPACE, OS_BUCKET_NAME, "shell-test/par-load-progress.json", "par-load-progress.json")
validate_load_progress("par-load-progress.json")
session.run_sql("drop schema if exists sample")
os.remove("par-load-progress.json")
delete_object(OS_BUCKET_NAME, "shell-test/par-load-progress.json", OS_NAMESPACE)

#@<> BUG#31606223 - read-write, empty progress file
progress_par=create_par(OS_NAMESPACE, OS_BUCKET_NAME, "ObjectReadWrite", "manifest-par", today_plus_days(1, RFC3339), "shell-test/par-load-progress.json")
open("par-load-progress.json", "w").close()
testutil.anycopy("par-load-progress.json", {"osBucketName":OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "name":"shell-test/par-load-progress.json"})

PREPARE_PAR_IS_SECRET_TEST()
EXPECT_NO_THROWS(lambda: util.load_dump(manifest_par, {"progressFile": progress_par}), "load_dump() using PAR progress file")
EXPECT_PAR_IS_SECRET()

EXPECT_STDOUT_CONTAINS("2 tables in 1 schemas were loaded")
testutil.download_oci_object(OS_NAMESPACE, OS_BUCKET_NAME, "shell-test/par-load-progress.json", "par-load-progress.json")
validate_load_progress("par-load-progress.json")
session.run_sql("drop schema if exists sample")
os.remove("par-load-progress.json")
delete_object(OS_BUCKET_NAME, "shell-test/par-load-progress.json", OS_NAMESPACE)

#@<> BUG#31605353 - PAR + resetProgress
progress_par=create_par(OS_NAMESPACE, OS_BUCKET_NAME, "ObjectReadWrite", "progress-par", today_plus_days(1, RFC3339), "shell-test/par-load-progress.json")
# load the dump
PREPARE_PAR_IS_SECRET_TEST()
EXPECT_NO_THROWS(lambda: util.load_dump(manifest_par, {"progressFile": progress_par}), "load_dump() using PAR progress file")
EXPECT_PAR_IS_SECRET()
# validate the status
EXPECT_STDOUT_CONTAINS("2 tables in 1 schemas were loaded")
testutil.download_oci_object(OS_NAMESPACE, OS_BUCKET_NAME, "shell-test/par-load-progress.json", "par-load-progress.json")
validate_load_progress("par-load-progress.json")
# clean the local stuff
session.run_sql("drop schema if exists sample")
os.remove("par-load-progress.json")
# load once again, reset the progress
PREPARE_PAR_IS_SECRET_TEST()
EXPECT_NO_THROWS(lambda: util.load_dump(manifest_par, {"progressFile": progress_par, "resetProgress": True}), "load_dump() using PAR progress file + resetProgress")
EXPECT_PAR_IS_SECRET()
# validate the status
EXPECT_STDOUT_CONTAINS("NOTE: Load progress file detected for the instance but 'resetProgress' option was enabled. Load progress will be discarded and the whole dump will be reloaded.")
testutil.download_oci_object(OS_NAMESPACE, OS_BUCKET_NAME, "shell-test/par-load-progress.json", "par-load-progress.json")
validate_load_progress("par-load-progress.json")
# clean the local stuff
session.run_sql("drop schema if exists sample")
os.remove("par-load-progress.json")
# delete the remove progress file
delete_object(OS_BUCKET_NAME, "shell-test/par-load-progress.json", OS_NAMESPACE)

#@<> BUG#33508311 - expired PARs should result in an exception
# download the manifest, overwrite the expireTime value
testutil.anycopy({"osBucketName":OS_BUCKET_NAME, "osNamespace":OS_NAMESPACE, "ociConfigFile":oci_config_file, "name":"shell-test/@.manifest.json"}, "@.manifest.json")

with open("@.manifest.json", encoding="utf-8") as json_file:
    manifest = json.load(json_file)

# current time in current timezone
current_time = datetime.datetime.now(datetime.timezone.utc).astimezone().isoformat()
manifest["expireTime"] = current_time

with open("@.manifest.json.expired", "w", encoding="utf-8") as json_file:
    json.dump(manifest, json_file, indent=4)

testutil.anycopy("@.manifest.json.expired", {"osBucketName":OS_BUCKET_NAME, "osNamespace":OS_NAMESPACE, "ociConfigFile":oci_config_file, "name":"shell-test/@.manifest.json"})

# expired PARs should result in an exception
remove_local_progress_file()
PREPARE_PAR_IS_SECRET_TEST()
EXPECT_THROWS(lambda: util.load_dump(manifest_par, {"progressFile": local_progress_file}), f"RuntimeError: Util.load_dump: The PARs in the manifest file have expired, the expiration time was set to: {current_time}")
EXPECT_PAR_IS_SECRET()

# cleanup
testutil.anycopy("@.manifest.json", {"osBucketName":OS_BUCKET_NAME, "osNamespace":OS_NAMESPACE, "ociConfigFile":oci_config_file, "name":"shell-test/@.manifest.json"})
os.remove("@.manifest.json")
os.remove("@.manifest.json.expired")

#@<> Cleanup
testutil.destroy_sandbox(__mysql_sandbox_port1)
testutil.destroy_sandbox(__mysql_sandbox_port2)
