#@ {has_oci_environment('OS')}
# Test dump and load into/from OCI ObjectStorage

#@<> INCLUDE oci_utils.inc

#@<> INCLUDE dump_utils.inc

#@<> Setup

import oci
import os
import json
import datetime

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

# Prepare dump on bucket location (using prefix)
prepare_empty_bucket(OS_BUCKET_NAME, OS_NAMESPACE)

PREPARE_PAR_IS_SECRET_TEST()
util.dump_instance("shell-test", {"osBucketName":OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file})
EXPECT_PAR_IS_SECRET()

testutil.deploy_sandbox(__mysql_sandbox_port2, "root", {"local_infile":1})
shell.connect(__sandbox_uri2)

#@<> WL14645-TSFR_1_2 - Successfully load dump with prefix AnyObjectRead PAR
all_read_par=create_par(OS_NAMESPACE, OS_BUCKET_NAME, "AnyObjectRead", "all-read-par", today_plus_days(1, RFC3339), "shell-test/", "ListObjects")
# BUG#35548572 - PARs using dedicated endpoints
all_read_par_converted = convert_par(all_read_par)

for par in [all_read_par, all_read_par_converted]:
    print(f"Testing PAR: {par}")
    remove_local_progress_file()
    PREPARE_PAR_IS_SECRET_TEST()
    EXPECT_NO_THROWS(lambda: util.load_dump(par, {"progressFile": local_progress_file}), "load_dump() using local progress file")
    EXPECT_PAR_IS_SECRET()
    EXPECT_STDOUT_CONTAINS("2 tables in 1 schemas were loaded")
    validate_load_progress(local_progress_file)
    session.run_sql("drop schema if exists sample")

#@<> WL14645-TSFR_1_4 - Successfully load dump with prefix AnyObjectReadWrite PAR
all_read_write_par=create_par(OS_NAMESPACE, OS_BUCKET_NAME, "AnyObjectReadWrite", "all-read-write-par", today_plus_days(1, RFC3339), "shell-test/", "ListObjects")

remove_local_progress_file()

PREPARE_PAR_IS_SECRET_TEST()
EXPECT_NO_THROWS(lambda: util.load_dump(all_read_write_par, {"progressFile": local_progress_file}), "load_dump() using local progress file")
EXPECT_PAR_IS_SECRET()

EXPECT_STDOUT_CONTAINS("2 tables in 1 schemas were loaded")
validate_load_progress(local_progress_file)
session.run_sql("drop schema if exists sample")

#@<> WL14645-TSFR_1_6 - Failed load dump with prefix AnyObjectWrite PAR
all_write_par=create_par(OS_NAMESPACE, OS_BUCKET_NAME, "AnyObjectWrite", "all-read-par", today_plus_days(1, RFC3339), "shell-test/")

remove_local_progress_file()

PREPARE_PAR_IS_SECRET_TEST()
EXPECT_THROWS(lambda: util.load_dump(all_write_par, {"progressFile": local_progress_file}), f"Error: Shell Error (54404): While 'Opening dump': Failed to fetch size of object 'https://{OS_NAMESPACE}.objectstorage.{oci_region}.oci.customer-oci.com/p/<secret>/n/{OS_NAMESPACE}/b/{OS_BUCKET_NAME}/o/shell-test/@.json': Not Found (404)")
EXPECT_PAR_IS_SECRET()

#@<> WL14645-TSFR_1_8 - Failed load dump with prefix AnyObjectRead PAR without ListObjects
all_read_par_no_list=create_par(OS_NAMESPACE, OS_BUCKET_NAME, "AnyObjectRead", "all-read-par", today_plus_days(1, RFC3339), "shell-test/")

remove_local_progress_file()

# WL14841-TSFR_3_2
# WL14841-TSFR_3_3
PREPARE_PAR_IS_SECRET_TEST()
EXPECT_THROWS(lambda: util.load_dump(all_read_par_no_list, {"progressFile": local_progress_file}),
  f"Error: Shell Error (54404): While 'Listing files': Could not access 'https://{OS_NAMESPACE}.objectstorage.{oci_region}.oci.customer-oci.com/p/<secret>/n/{OS_NAMESPACE}/b/{OS_BUCKET_NAME}/o/': BucketNotFound: Either the bucket named '{OS_BUCKET_NAME}' does not exist in the namespace '{OS_NAMESPACE}' or you are not authorized to access it")
EXPECT_PAR_IS_SECRET()

#@<> WL14645-TSFR_1_10 - Failed load dump with prefix AnyObjectWrite PAR and ListObjects
all_write_par_and_list=create_par(OS_NAMESPACE, OS_BUCKET_NAME, "AnyObjectWrite", "all-read-par", today_plus_days(1, RFC3339), "shell-test/", "ListObjects")

remove_local_progress_file()

PREPARE_PAR_IS_SECRET_TEST()
EXPECT_THROWS(lambda: util.load_dump(all_write_par_and_list, {"progressFile": local_progress_file}), f"Error: Shell Error (54404): While 'Opening dump': Failed to fetch size of object 'https://{OS_NAMESPACE}.objectstorage.{oci_region}.oci.customer-oci.com/p/<secret>/n/{OS_NAMESPACE}/b/{OS_BUCKET_NAME}/o/shell-test/@.json': Not Found (404)")
EXPECT_PAR_IS_SECRET()

#@<> WL14645-TSFR_1_12 - Failed load dump, missing progress file
all_write_par_and_list=create_par(OS_NAMESPACE, OS_BUCKET_NAME, "AnyObjectRead", "all-read-par", today_plus_days(1, RFC3339), "shell-test/", "ListObjects")

PREPARE_PAR_IS_SECRET_TEST()
EXPECT_THROWS(lambda: util.load_dump(all_write_par_and_list),
  "When using a PAR to load a dump, the progressFile option must be defined")
EXPECT_PAR_IS_SECRET()

#@<> using remote progress file
progress_par = create_par(OS_NAMESPACE, OS_BUCKET_NAME, "ObjectReadWrite", "manifest-par", today_plus_days(1, RFC3339), "random-folder/par-load-progress.json")

PREPARE_PAR_IS_SECRET_TEST()
EXPECT_NO_THROWS(lambda: util.load_dump(all_write_par_and_list, {"progressFile": progress_par}), "load_dump() using remote progress file")
EXPECT_PAR_IS_SECRET()

# verify if everything was OK
EXPECT_STDOUT_CONTAINS("2 tables in 1 schemas were loaded")
testutil.download_oci_object(OS_NAMESPACE, OS_BUCKET_NAME, "random-folder/par-load-progress.json", "par-load-progress.json")
validate_load_progress("par-load-progress.json")

# cleanup
session.run_sql("drop schema if exists sample")
os.remove("par-load-progress.json")
delete_object(OS_BUCKET_NAME, "random-folder/par-load-progress.json", OS_NAMESPACE)

#@<> WL14645-TSFR_1_14 - Failed load dump, missing dump
all_write_par_and_list_no_dump=create_par(OS_NAMESPACE, OS_BUCKET_NAME, "AnyObjectRead", "all-read-par", today_plus_days(1, RFC3339), "random-folder/", "ListObjects")

remove_local_progress_file()

PREPARE_PAR_IS_SECRET_TEST()
EXPECT_THROWS(lambda: util.load_dump(all_write_par_and_list_no_dump, {"progressFile": local_progress_file}), f"Error: Shell Error (54404): While 'Opening dump': Failed to fetch size of object 'https://{OS_NAMESPACE}.objectstorage.{oci_region}.oci.customer-oci.com/p/<secret>/n/{OS_NAMESPACE}/b/{OS_BUCKET_NAME}/o/random-folder/@.json': Not Found (404)")
EXPECT_PAR_IS_SECRET()

#@<> BUG#33332080 - load a dump which is still in progress
all_read_par=create_par(OS_NAMESPACE, OS_BUCKET_NAME, "AnyObjectRead", "all-read-par", today_plus_days(1, RFC3339), "shell-test/", "ListObjects")

# download the @.done.json file, remove it from the bucket
testutil.anycopy({"osBucketName":OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "name":"shell-test/@.done.json"}, "@.done.json")
delete_object(OS_BUCKET_NAME, "shell-test/@.done.json", OS_NAMESPACE)

# incomplete dump should require the 'waitDumpTimeout' option
remove_local_progress_file()

PREPARE_PAR_IS_SECRET_TEST()
EXPECT_THROWS(lambda: util.load_dump(all_read_par, {"progressFile": local_progress_file}), "Error: Shell Error (53008): Incomplete dump")
EXPECT_PAR_IS_SECRET()

# asynchronously start the load process
remove_local_progress_file()
proc = testutil.call_mysqlsh_async([__sandbox_uri2, "--py", "-e", f"util.load_dump('{all_read_par}', {{'progressFile': '{local_progress_file}', 'waitDumpTimeout': 60}})"])

# wait a bit and reupload the @.done.json file
time.sleep(5)
testutil.anycopy("@.done.json", {"osBucketName":OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "name":"shell-test/@.done.json"})

# wait for the upload to finish (should catch up with the @.done.json file)
testutil.wait_mysqlsh_async(proc)

# verify if everything was OK
EXPECT_STDOUT_CONTAINS("2 tables in 1 schemas were loaded")
validate_load_progress(local_progress_file)
session.run_sql("drop schema if exists sample")

#@<> BUG#33122234 - setup
tested_schema = "test_schema"
tested_table = "test_table_"
# each table has 4 files associated with it, this should add up to more than 1000 files
n_tables = 250

# create schema with lots of tables
shell.connect(__sandbox_uri1)
session.run_sql("DROP SCHEMA IF EXISTS !;", [ tested_schema ])
session.run_sql("CREATE SCHEMA !;", [ tested_schema ])
session.run_sql("CREATE TABLE !.! (id INT PRIMARY KEY);", [ tested_schema, tested_table ])
session.run_sql("INSERT INTO !.! (id) VALUES (1234);", [ tested_schema, tested_table ])
session.run_sql("ANALYZE TABLE !.!;", [ tested_schema, tested_table ])

for i in range(n_tables):
    table_name = f"{tested_table}{i}"
    session.run_sql("CREATE TABLE !.! (PRIMARY KEY(id)) AS SELECT * FROM !.!;", [ tested_schema, table_name, tested_schema, tested_table ])
    session.run_sql("ANALYZE TABLE !.!;", [ tested_schema, table_name ])

# prepare the dump
prepare_empty_bucket(OS_BUCKET_NAME, OS_NAMESPACE)
util.dump_schemas([tested_schema], "shell-test", {"osBucketName":OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file})

shell.connect(__sandbox_uri2)

#@<> BUG#33122234 - test
all_read_par=create_par(OS_NAMESPACE, OS_BUCKET_NAME, "AnyObjectRead", "all-read-par", today_plus_days(1, RFC3339), "shell-test/", "ListObjects")

PREPARE_PAR_IS_SECRET_TEST()
EXPECT_NO_THROWS(lambda: util.load_dump(all_read_par, {"progressFile": ""}), "load_dump() with lots of files")
EXPECT_PAR_IS_SECRET()

EXPECT_STDOUT_CONTAINS(f"{n_tables + 1} tables in 1 schemas were loaded")

#@<> BUG#33122234 - cleanup
session.run_sql("DROP SCHEMA IF EXISTS !;", [ tested_schema ])

#@<> BUG#34599319 - dump&load when paths contain spaces and other characters that need to be URL-encoded
tested_schema = "test schema"
tested_table = "fish & chips"

shell.connect(__sandbox_uri1)
session.run_sql("DROP SCHEMA IF EXISTS !;", [ tested_schema ])
session.run_sql("CREATE SCHEMA !;", [ tested_schema ])
session.run_sql("CREATE TABLE !.! (id INT PRIMARY KEY);", [ tested_schema, tested_table ])
session.run_sql("INSERT INTO !.! (id) VALUES (1234);", [ tested_schema, tested_table ])
session.run_sql("ANALYZE TABLE !.!;", [ tested_schema, tested_table ])

# prepare the dump
prepare_empty_bucket(OS_BUCKET_NAME, OS_NAMESPACE)
util.dump_schemas([tested_schema], "shell test/dump & load", {"osBucketName": OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile": oci_config_file})

#@<> BUG#34599319 - test
all_read_par = create_par(OS_NAMESPACE, OS_BUCKET_NAME, "AnyObjectRead", "all-read-par", today_plus_days(1, RFC3339), "shell test/dump & load/", "ListObjects")
shell.connect(__sandbox_uri2)

PREPARE_PAR_IS_SECRET_TEST()
EXPECT_NO_THROWS(lambda: util.load_dump(all_read_par, {"progressFile": ""}), "load_dump() with URL-encoded file names")
EXPECT_PAR_IS_SECRET()

EXPECT_STDOUT_CONTAINS(f"1 tables in 1 schemas were loaded")

#@<> BUG#34599319 - cleanup
session.run_sql("DROP SCHEMA IF EXISTS !;", [ tested_schema ])

#@<> automatically convert a file PAR to a prefix PAR
tested_schema = "test-schema"
tested_table = "test-table"

# read-write PAR
prepare_empty_bucket(OS_BUCKET_NAME, OS_NAMESPACE)
generic_par = create_par(OS_NAMESPACE, OS_BUCKET_NAME, "AnyObjectReadWrite", "read-write-par", today_plus_days(1, RFC3339), "no-slash", "ListObjects")

# prepare the dump
shell.connect(__sandbox_uri1)
create_test_table(session, tested_schema, tested_table, "(id INT PRIMARY KEY)")

PREPARE_PAR_IS_SECRET_TEST()
EXPECT_NO_THROWS(lambda: util.dump_schemas([tested_schema], generic_par, {"showProgress": False}), "dump to a generic PAR")
EXPECT_PAR_IS_SECRET()

# load the dump
shell.connect(__sandbox_uri2)

for sample_par in [ generic_par, convert_par(generic_par) ]:
    PREPARE_PAR_IS_SECRET_TEST()
    EXPECT_NO_THROWS(lambda: util.load_dump(sample_par, {"progressFile": "", "showProgress": False}), "load from a generic PAR")
    EXPECT_PAR_IS_SECRET()
    session.run_sql("DROP SCHEMA IF EXISTS !;", [ tested_schema ])

#@<> Cleanup
testutil.destroy_sandbox(__mysql_sandbox_port1)
testutil.destroy_sandbox(__mysql_sandbox_port2)
