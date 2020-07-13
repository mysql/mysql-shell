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
session1.run_sql("create table sample.data_copy(id int, name varchar(20))")
session1.run_sql("insert into sample.data_copy values (5, 'Jane Doe')")
session1.close()

def put_object(namespace, bucket, name, content):
    config = oci.config.from_file(os.path.join(OCI_CONFIG_HOME, "config"))
    os_client = oci.object_storage.ObjectStorageClient(config)
    os_client.put_object(namespace, bucket, name, content)


def validate_load_progress(file_name):
    with open(file_name) as fp:
        while True:
            line = fp.readline()
            if line:
                content = json.loads(line)
                EXPECT_TRUE("op" in content)
                EXPECT_TRUE("done" in content)
                EXPECT_TRUE("schema" in content)
            else:
                break

# ------------
# Defaults
RFC3339 = True
shell.connect(__sandbox_uri1)

prepare_empty_bucket(k_bucket_name, OS_NAMESPACE)
util.dump_instance("shell-test", {"osBucketName":k_bucket_name, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "ociParManifest": True})

testutil.deploy_sandbox(__mysql_sandbox_port2, "root", {"local_infile":1})
shell.connect(__sandbox_uri2)

manifest_par=create_par(OS_NAMESPACE, k_bucket_name, "ObjectRead", "manifest-par", today_plus_days(1, RFC3339), "shell-test/@.manifest.json")

#@<> WL14154-TSFR7_1 - When doing a load using a PAR for a manifest file with the progressFile option not set. Validate that the load fail because progressFile option is mandatory.
EXPECT_THROWS(lambda:util.load_dump(manifest_par), "Util.load_dump: When using a PAR to a dump manifest, the progressFile option must be defined.")


#@<> WL14154-TSFR7_2 - When doing a load using a PAR for a manifest file with the progressFile option set to a file system. Validate that the load success and the file given stores the load progress.
EXPECT_NO_THROWS(lambda: util.load_dump(manifest_par, {"progressFile": "my_load_progress.txt"}), "load_dump() using local progress file")
EXPECT_STDOUT_CONTAINS("2 tables in 1 schemas were loaded")
validate_load_progress("my_load_progress.txt")
os.remove("my_load_progress.txt")
session.run_sql("drop schema sample")

#@<> WL14154-TSFR8_1 - When doing a load using a PAR for a manifest file with the progressFile option set to a file system. Validate that the load success and the file given stores the load progress.
progress_par=create_par(OS_NAMESPACE, k_bucket_name, "ObjectReadWrite", "manifest-par", today_plus_days(1, RFC3339), "shell-test/par-load-progress.json")
EXPECT_NO_THROWS(lambda: util.load_dump(manifest_par, {"progressFile": progress_par}), "load_dump() using PAR progress file")
EXPECT_STDOUT_CONTAINS("2 tables in 1 schemas were loaded")
testutil.download_oci_object(OS_NAMESPACE, k_bucket_name, "shell-test/par-load-progress.json", "par-load-progress.json")
validate_load_progress("par-load-progress.json")
session.run_sql("drop schema sample")
os.remove("par-load-progress.json")
delete_object(k_bucket_name, "shell-test/par-load-progress.json", OS_NAMESPACE)

#@<> WL14154-TSFR9_1 - When doing a load using a PAR for a manifest file with the progressFile option set to a read/write PAR object for a file that is not in the same bucket where the dump is. Validate that the load fails because the file to store the progress is not in the same bucket.
progress_par=create_par(OS_NAMESPACE, "shell-rut-priv", "ObjectReadWrite", "manifest-par", today_plus_days(1, RFC3339), "shell-test/another-bucket-progress.json")
EXPECT_THROWS(lambda:util.load_dump(manifest_par, {"progressFile": progress_par}), "Util.load_dump: The provided PAR must be a file on the dump location:")

#@<> WL14154-TSFR9_2 - When doing a load using a PAR for a manifest file with the progressFile option set to a read/write PAR object for a file that is in the same bucket but in diferent location where the dump is. Validate that the load fails because the file to store the progress is not in the same location in the bucket.
progress_par=create_par(OS_NAMESPACE, k_bucket_name, "ObjectReadWrite", "manifest-par", today_plus_days(1, RFC3339), "same-bucket-different-prefix.json")
EXPECT_THROWS(lambda:util.load_dump(manifest_par, {"progressFile": progress_par}), "Util.load_dump: The provided PAR must be a file on the dump location:")

#@<> WL14154-TSFR9_4 - When doing a load using a PAR for a manifest file with the progressFile option set to a read PAR object for a file that is in the same bucket and location where the dump is. Validate that the load fails because the file to store the progress has not the right permissions.
progress_par=create_par(OS_NAMESPACE, k_bucket_name, "ObjectRead", "manifest-par", today_plus_days(1, RFC3339), "shell-test/read-only-progress.json")
EXPECT_THROWS(lambda:util.load_dump(manifest_par, {"progressFile": progress_par}), "RuntimeError: Util.load_dump: Failed to put object")

#@<> WL14154-TSFR6_1
# create PAR for the progress file
progress_par=create_par(OS_NAMESPACE, k_bucket_name, "ObjectReadWrite", "manifest-par", today_plus_days(1, RFC3339), "shell-test/par-load-progress.json")

# download the manifest, remove some PARs, overwrite the manifest in the bucket
testutil.anycopy({"osBucketName":k_bucket_name, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "name":"shell-test/@.manifest.json"}, "@.manifest.json")

with open("@.manifest.json", encoding="utf-8") as json_file:
    manifest = json.load(json_file)

del manifest["endTime"]
manifest["lastUpdate"] = manifest["startTime"]
manifest["contents"] = manifest["contents"][:int(len(manifest["contents"]) / 2)]

with open("@.manifest.json.partial", "w", encoding="utf-8") as json_file:
    json.dump(manifest, json_file, indent=4)

testutil.anycopy("@.manifest.json.partial", {"osBucketName":k_bucket_name, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "name":"shell-test/@.manifest.json"})

# incomplete dump should require the 'waitDumpTimeout' option
EXPECT_THROWS(lambda: util.load_dump(manifest_par, {"progressFile": "progress.txt"}), "RuntimeError: Util.load_dump: Incomplete dump")

# asynchronously start the load process
proc = testutil.call_mysqlsh_async([__sandbox_uri2, "--py", "-e", "util.load_dump('{0}', {{'progressFile': '{1}', 'waitDumpTimeout': 60}})".format(manifest_par, progress_par)])

# wait a bit and reupload the manifest with all the PARs
time.sleep(5)
testutil.anycopy("@.manifest.json", {"osBucketName":k_bucket_name, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "name":"shell-test/@.manifest.json"})

# wait for the upload to finish (should catch up with the full manifest)
testutil.wait_mysqlsh_async(proc)

# verify if everything was OK
EXPECT_STDOUT_CONTAINS("2 tables in 1 schemas were loaded")
testutil.download_oci_object(OS_NAMESPACE, k_bucket_name, "shell-test/par-load-progress.json", "par-load-progress.json")
validate_load_progress("par-load-progress.json")

# cleanup
session.run_sql("drop schema sample")
os.remove("par-load-progress.json")
os.remove("@.manifest.json")
os.remove("@.manifest.json.partial")
delete_object(k_bucket_name, "shell-test/par-load-progress.json", OS_NAMESPACE)

#@<> BUG#31606223 - read only, progress file does not exist
progress_par=create_par(OS_NAMESPACE, k_bucket_name, "ObjectRead", "manifest-par", today_plus_days(1, RFC3339), "shell-test/par-load-progress.json")
EXPECT_THROWS(lambda: util.load_dump(manifest_par, {"progressFile": progress_par}), "RuntimeError: Util.load_dump: Failed to put object '{0}': Either the bucket named 'testbkt' does not exist in the namespace 'mysql2' or you are not authorized to access it (404)".format(progress_par[progress_par.find("/p/"):]))

#@<> BUG#31606223 - read only, empty progress file
progress_par=create_par(OS_NAMESPACE, k_bucket_name, "ObjectRead", "manifest-par", today_plus_days(1, RFC3339), "shell-test/par-load-progress.json")
open("par-load-progress.json", "w").close()
testutil.anycopy("par-load-progress.json", {"osBucketName":k_bucket_name, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "name":"shell-test/par-load-progress.json"})
EXPECT_THROWS(lambda: util.load_dump(manifest_par, {"progressFile": progress_par}), "RuntimeError: Util.load_dump: Failed to put object '{0}': Either the bucket named 'testbkt' does not exist in the namespace 'mysql2' or you are not authorized to access it (404)".format(progress_par[progress_par.find("/p/"):]))
os.remove("par-load-progress.json")

#@<> BUG#31606223 - write only, progress file does not exist
# this is a success, because reading from write-only PAR results in 404, code assumes that the file does not exist and continues with the load process
progress_par=create_par(OS_NAMESPACE, k_bucket_name, "ObjectWrite", "manifest-par", today_plus_days(1, RFC3339), "shell-test/par-load-progress.json")
EXPECT_NO_THROWS(lambda: util.load_dump(manifest_par, {"progressFile": progress_par}), "load_dump() using PAR progress file")
EXPECT_STDOUT_CONTAINS("2 tables in 1 schemas were loaded")
testutil.download_oci_object(OS_NAMESPACE, k_bucket_name, "shell-test/par-load-progress.json", "par-load-progress.json")
validate_load_progress("par-load-progress.json")
session.run_sql("drop schema sample")
os.remove("par-load-progress.json")
delete_object(k_bucket_name, "shell-test/par-load-progress.json", OS_NAMESPACE)

#@<> BUG#31606223 - write only, empty progress file
# this is a success, because reading from write-only PAR results in 404, code assumes that the file does not exist and continues with the load process
progress_par=create_par(OS_NAMESPACE, k_bucket_name, "ObjectWrite", "manifest-par", today_plus_days(1, RFC3339), "shell-test/par-load-progress.json")
open("par-load-progress.json", "w").close()
testutil.anycopy("par-load-progress.json", {"osBucketName":k_bucket_name, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "name":"shell-test/par-load-progress.json"})
EXPECT_NO_THROWS(lambda: util.load_dump(manifest_par, {"progressFile": progress_par}), "load_dump() using PAR progress file")
EXPECT_STDOUT_CONTAINS("2 tables in 1 schemas were loaded")
testutil.download_oci_object(OS_NAMESPACE, k_bucket_name, "shell-test/par-load-progress.json", "par-load-progress.json")
validate_load_progress("par-load-progress.json")
session.run_sql("drop schema sample")
os.remove("par-load-progress.json")
delete_object(k_bucket_name, "shell-test/par-load-progress.json", OS_NAMESPACE)

#@<> BUG#31606223 - read-write, progress file does not exist
progress_par=create_par(OS_NAMESPACE, k_bucket_name, "ObjectReadWrite", "manifest-par", today_plus_days(1, RFC3339), "shell-test/par-load-progress.json")
EXPECT_NO_THROWS(lambda: util.load_dump(manifest_par, {"progressFile": progress_par}), "load_dump() using PAR progress file")
EXPECT_STDOUT_CONTAINS("2 tables in 1 schemas were loaded")
testutil.download_oci_object(OS_NAMESPACE, k_bucket_name, "shell-test/par-load-progress.json", "par-load-progress.json")
validate_load_progress("par-load-progress.json")
session.run_sql("drop schema sample")
os.remove("par-load-progress.json")
delete_object(k_bucket_name, "shell-test/par-load-progress.json", OS_NAMESPACE)

#@<> BUG#31606223 - read-write, empty progress file
progress_par=create_par(OS_NAMESPACE, k_bucket_name, "ObjectReadWrite", "manifest-par", today_plus_days(1, RFC3339), "shell-test/par-load-progress.json")
open("par-load-progress.json", "w").close()
testutil.anycopy("par-load-progress.json", {"osBucketName":k_bucket_name, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "name":"shell-test/par-load-progress.json"})
EXPECT_NO_THROWS(lambda: util.load_dump(manifest_par, {"progressFile": progress_par}), "load_dump() using PAR progress file")
EXPECT_STDOUT_CONTAINS("2 tables in 1 schemas were loaded")
testutil.download_oci_object(OS_NAMESPACE, k_bucket_name, "shell-test/par-load-progress.json", "par-load-progress.json")
validate_load_progress("par-load-progress.json")
session.run_sql("drop schema sample")
os.remove("par-load-progress.json")
delete_object(k_bucket_name, "shell-test/par-load-progress.json", OS_NAMESPACE)

#@<> Cleanup
testutil.destroy_sandbox(__mysql_sandbox_port1)
testutil.destroy_sandbox(__mysql_sandbox_port2)
