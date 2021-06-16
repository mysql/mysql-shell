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


# Prepare dump on bucket location (using prefix)
prepare_empty_bucket(OS_BUCKET_NAME, OS_NAMESPACE)
util.dump_instance("shell-test", {"osBucketName":OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file})

testutil.deploy_sandbox(__mysql_sandbox_port2, "root", {"local_infile":1})
shell.connect(__sandbox_uri2)

def remove_file(name):
  try:
      os.remove(name)
  except:
      pass

remove_file("my_load_progress.txt")
remove_file("http_progress.txt")

#@<> WL14645-TSFR_1_2 - Successfully load dump with prefix AnyObjectRead PAR
all_read_par=create_par(OS_NAMESPACE, OS_BUCKET_NAME, "AnyObjectRead", "all-read-par", today_plus_days(1, RFC3339), "shell-test/", "ListObjects")

EXPECT_NO_THROWS(lambda: util.load_dump(all_read_par, {"progressFile": "http_progress.txt"}), "load_dump() using local progress file")

EXPECT_STDOUT_CONTAINS("2 tables in 1 schemas were loaded")
validate_load_progress("http_progress.txt")
os.remove("http_progress.txt")
session.run_sql("drop schema if exists sample")


#@<> WL14645-TSFR_1_4 - Successfully load dump with prefix AnyObjectReadWrite PAR
all_read_write_par=create_par(OS_NAMESPACE, OS_BUCKET_NAME, "AnyObjectReadWrite", "all-read-write-par", today_plus_days(1, RFC3339), "shell-test/", "ListObjects")

EXPECT_NO_THROWS(lambda: util.load_dump(all_read_write_par, {"progressFile": "my_load_progress.txt"}), "load_dump() using local progress file")

EXPECT_STDOUT_CONTAINS("2 tables in 1 schemas were loaded")
validate_load_progress("my_load_progress.txt")
os.remove("my_load_progress.txt")
session.run_sql("drop schema if exists sample")

#@<> WL14645-TSFR_1_6 - Failed load dump with prefix AnyObjectWrite PAR
all_write_par=create_par(OS_NAMESPACE, OS_BUCKET_NAME, "AnyObjectWrite", "all-read-par", today_plus_days(1, RFC3339), "shell-test/")

EXPECT_THROWS(lambda: util.load_dump(all_write_par, {"progressFile": "my_load_progress.txt"}), "Util.load_dump: Not Found")

#@<> WL14645-TSFR_1_8 - Failed load dump with prefix AnyObjectRead PAR without ListObjects
all_read_par_no_list=create_par(OS_NAMESPACE, OS_BUCKET_NAME, "AnyObjectRead", "all-read-par", today_plus_days(1, RFC3339), "shell-test/")

EXPECT_THROWS(lambda: util.load_dump(all_read_par_no_list, {"progressFile": "my_load_progress.txt"}),
  f"Util.load_dump: Either the bucket named '{OS_BUCKET_NAME}' does not exist in the namespace '{OS_NAMESPACE}' or you are not authorized to access it")

#@<> WL14645-TSFR_1_10 - Failed load dump with prefix AnyObjectWrite PAR and ListObjects
all_write_par_and_list=create_par(OS_NAMESPACE, OS_BUCKET_NAME, "AnyObjectWrite", "all-read-par", today_plus_days(1, RFC3339), "shell-test/", "ListObjects")

EXPECT_THROWS(lambda: util.load_dump(all_write_par_and_list, {"progressFile": "my_load_progress.txt"}), "Util.load_dump: Not Found")

#@<> WL14645-TSFR_1_12 - Failed load dump, missing progress file
all_write_par_and_list=create_par(OS_NAMESPACE, OS_BUCKET_NAME, "AnyObjectRead", "all-read-par", today_plus_days(1, RFC3339), "shell-test/", "ListObjects")

EXPECT_THROWS(lambda: util.load_dump(all_write_par_and_list),
  "Util.load_dump: When using a PAR to load a dump, the progressFile option must be defined")

EXPECT_THROWS(lambda: util.load_dump(all_write_par_and_list, {"progressFile": "http://whatever-file_starting-with-http"}),
  "Util.load_dump: When using a prefix PAR to load a dump, the progressFile option must be a local file")

#@<> WL14645-TSFR_1_14 - Failed load dump, missing dump
all_write_par_and_list_no_dump=create_par(OS_NAMESPACE, OS_BUCKET_NAME, "AnyObjectRead", "all-read-par", today_plus_days(1, RFC3339), "random-folder/", "ListObjects")

EXPECT_THROWS(lambda: util.load_dump(all_write_par_and_list_no_dump, {"progressFile": "my_load_progress.txt"}),
  "Util.load_dump: Not Found")

#@<> Cleanup
testutil.destroy_sandbox(__mysql_sandbox_port1)
testutil.destroy_sandbox(__mysql_sandbox_port2)
