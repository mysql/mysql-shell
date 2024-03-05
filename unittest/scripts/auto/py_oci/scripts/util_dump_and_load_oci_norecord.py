#@ {has_oci_environment('OS')}
# Test dump and load into/from OCI ObjectStorage

#@<> INCLUDE oci_utils.inc

#@<> INCLUDE dump_utils.inc

#@<> Setup

import oci
import os
import shutil

oci_config_file=os.path.join(OCI_CONFIG_HOME, "config")

testutil.deploy_sandbox(__mysql_sandbox_port1, "root")
testutil.deploy_sandbox(__mysql_sandbox_port2, "root", {"local_infile":1})

session1=mysql.get_session(__sandbox_uri1)
session2=mysql.get_session(__sandbox_uri2)
uuid = session2.run_sql("select @@server_uuid").fetch_one()[0]

session1.run_sql("create schema world")

# load test schemas
testutil.import_data(__sandbox_uri1, __data_path+"/sql/sakila-schema.sql")
testutil.import_data(__sandbox_uri1, __data_path+"/sql/sakila-data.sql", "sakila")
testutil.import_data(__sandbox_uri1, __data_path+"/sql/world.sql", "world")

testutil.mkdir(__tmp_dir+"/dumps")

# ------------
# Defaults

prepare_empty_bucket(OS_BUCKET_NAME, OS_NAMESPACE)
shell.connect(__sandbox_uri1)

#@<> Option conflict, attempt creating a dump using PAR and osBucketName

# Sample PAR URLs to test option conflict with the supported object storages (Oci/Aws/Azure)
sample_bucket_par = "https://objectstorage.us-ashburn-1.oraclecloud.com/p/secret-token-string/n/namespace/b/bucket/o/"
sample_prefix_par = "https://objectstorage.us-ashburn-1.oraclecloud.com/p/secret-token-string/n/namespace/b/bucket/o/prefix/"

for sample_par in [ sample_bucket_par, convert_par(sample_bucket_par), sample_prefix_par, convert_par(sample_prefix_par) ]:
    EXPECT_THROWS(lambda: util.dump_instance(sample_par, {"osBucketName":OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file}), "The option 'osBucketName' can not be used when using a PAR as the target output url.")
    EXPECT_THROWS(lambda: util.dump_schemas(["world"], sample_par, {"osBucketName":OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file}), "The option 'osBucketName' can not be used when using a PAR as the target output url.")
    EXPECT_THROWS(lambda: util.dump_tables("sakila", ["actor"], sample_par, {"osBucketName":OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file}), "The option 'osBucketName' can not be used when using a PAR as the target output url.")

#@<> Create a dump with defaults into OCI, no prefix
util.dump_instance("", {"osBucketName":OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file})

#@<> Load the dump from OCI with defaults
shell.connect(__sandbox_uri2)
util.load_dump("", {"osBucketName":OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file})

compare_servers(session1, session2)

wipeout_server(session2)

#@<> Create a dump with defaults into OCI, with prefix
prepare_empty_bucket(OS_BUCKET_NAME, OS_NAMESPACE)

shell.connect(__sandbox_uri1)
util.dump_instance("mydump", {"osBucketName":OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file})

#@<> Try another dump in the same bucket with a different prefix
util.dump_schemas(["world"], "another/dump", {"osBucketName":OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file})

#@<> Load the dump from OCI with defaults, with prefix
shell.connect(__sandbox_uri2)
util.load_dump("mydump", {"osBucketName":OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file})

compare_servers(session1, session2)
delete_object(OS_BUCKET_NAME, "mydump/load-progress."+uuid+".json", OS_NAMESPACE)
wipeout_server(session2)

#@<> Load different dump in the same bucket
util.load_dump("another/dump", {"osBucketName":OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file})

compare_schema(session1, session2, "world")
res = session2.run_sql("select schema_name from information_schema.schemata where schema_name in ('world', 'sakila')").fetch_all()
EXPECT_EQ(1, len(res))
EXPECT_EQ("world", res[0][0])

wipeout_server(session2)

#@<> Cause a partial load
shell.connect(__sandbox_uri2)

backup_file = "backup"
if os.path.isfile(backup_file):
    os.remove(backup_file)
else:
    shutil.rmtree(backup_file, ignore_errors=True)

testutil.create_file("sakila@film_text@@0.tsv.zst", "badfile")
testutil.anycopy({"osBucketName":OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "name":"mydump/sakila@film_text@@0.tsv.zst"}, backup_file)
testutil.anycopy("sakila@film_text@@0.tsv.zst", {"osBucketName":OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "name":"mydump/sakila@film_text@@0.tsv.zst"})

EXPECT_THROWS(lambda: util.load_dump("mydump", {"osBucketName":OS_BUCKET_NAME, "osNamespace":OS_NAMESPACE, "ociConfigFile":oci_config_file}), "Error loading dump")

EXPECT_STDOUT_CONTAINS("sakila@film_text@@0.tsv.zst: MySQL Error 2000 (00000): zstd.read: Unknown frame descriptor")

testutil.anycopy(backup_file, {"osBucketName":OS_BUCKET_NAME, "osNamespace":OS_NAMESPACE, "ociConfigFile":oci_config_file, "name":"mydump/sakila@film_text@@0.tsv.zst"})

os.remove(backup_file)
os.remove("sakila@film_text@@0.tsv.zst")

#@<> Resume partial load
WIPE_SHELL_LOG()
util.load_dump("mydump", {"osBucketName":OS_BUCKET_NAME, "osNamespace":OS_NAMESPACE, "ociConfigFile":oci_config_file})

EXPECT_SHELL_LOG_NOT_CONTAINS("Executing DDL script for ")
EXPECT_SHELL_LOG_CONTAINS("sakila@film_text@@0.tsv.zst: Records: ")
compare_servers(session1, session2)
wipeout_server(session2)

#@<> Load dump with a local progress file
testutil.rmfile("progress.txt")
WIPE_SHELL_LOG()
util.load_dump("mydump", {"osBucketName":OS_BUCKET_NAME, "osNamespace":OS_NAMESPACE,  "ociConfigFile":oci_config_file, "progressFile":"progress.txt"})
open("progress.txt").read()

EXPECT_SHELL_LOG_CONTAINS("Executing DDL script for ")
EXPECT_SHELL_LOG_CONTAINS("sakila@film_text@@0.tsv.zst: Records: ")

#@<> Bad Bucket Name Option
EXPECT_THROWS(lambda: util.load_dump("mydump", {"osBucketName":"bukkit"}), "Shell Error (54404): Util.load_dump: While 'Opening dump': Failed opening object 'mydump/@.json' in READ mode: Failed to get summary for object 'mydump/@.json': Not Found (404)")

#@<> BUG#32734880 progress file is not removed when resetProgress is used
dump_dir = "mydump"
delete_object(OS_BUCKET_NAME, "mydump/load-progress."+uuid+".json", OS_NAMESPACE)

# wipe the destination server
wipeout_server(session2)

# load the full dump
shell.connect(__sandbox_uri2)
WIPE_OUTPUT()
WIPE_SHELL_LOG()
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "osBucketName": OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile": oci_config_file, "showProgress": False }), "Loading the dump should not fail")
EXPECT_SHELL_LOG_CONTAINS(".tsv.zst: Records: 4079  Deleted: 0  Skipped: 0  Warnings: 0")

# wipe the destination server again
wipeout_server(session2)

# reset the progress, load the dump again, this time just the DDL
WIPE_OUTPUT()
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "loadData": False, "resetProgress": True, "osBucketName": OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile": oci_config_file, "showProgress": False }), "Loading the dump should not fail")
EXPECT_STDOUT_CONTAINS("NOTE: Load progress file detected for the instance but 'resetProgress' option was enabled. Load progress will be discarded and the whole dump will be reloaded.")

# load the data without resetting the progress
WIPE_OUTPUT()
WIPE_SHELL_LOG()
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "loadDdl": False, "osBucketName": OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile": oci_config_file, "showProgress": False }), "Loading the dump should not fail")
EXPECT_STDOUT_CONTAINS("NOTE: Load progress file detected. Load will be resumed from where it was left, assuming no external updates were made.")
# ensure data was loaded
EXPECT_SHELL_LOG_CONTAINS(".tsv.zst: Records: 4079  Deleted: 0  Skipped: 0  Warnings: 0")

#@<> Bug #31188854: USING THE OCIPROFILE OPTION IN A DUMP MAKE THE DUMP TO ALWAYS FAIL
# This error now confirms the reported issue is fixed
dump_dir = "mydump-31188854"
EXPECT_THROWS(lambda: util.dump_instance(dump_dir, {"osBucketName": "any-bucket", "ociProfile": "DEFAULT"}), "Failed to list objects using prefix")
EXPECT_THROWS(lambda: util.dump_schemas(["world"], dump_dir, {"osBucketName": "any-bucket", "ociProfile": "DEFAULT"}), "Failed to list objects using prefix")
EXPECT_THROWS(lambda: util.dump_tables("sakila", ["actor"], dump_dir, {"osBucketName": "any-bucket", "ociProfile": "DEFAULT"}), "Failed to list objects using prefix")
EXPECT_THROWS(lambda: util.export_table("sakila.actor", os.path.join(dump_dir, "out.txt"), {"osBucketName": "any-bucket", "ociProfile": "DEFAULT"}), "Failed to list objects using prefix")

#@<> BUG#34599319 - dump&load when paths contain spaces and other characters that need to be URL-encoded
tested_schema = "test schema"
tested_table = "fish & chips"
dump_dir = "shell test/dump & load"

shell.connect(__sandbox_uri1)
session.run_sql("DROP SCHEMA IF EXISTS !;", [ tested_schema ])
session.run_sql("CREATE SCHEMA !;", [ tested_schema ])
session.run_sql("CREATE TABLE !.! (id INT PRIMARY KEY);", [ tested_schema, tested_table ])
session.run_sql("INSERT INTO !.! (id) VALUES (1234);", [ tested_schema, tested_table ])
session.run_sql("ANALYZE TABLE !.!;", [ tested_schema, tested_table ])

# prepare the dump
prepare_empty_bucket(OS_BUCKET_NAME, OS_NAMESPACE)
util.dump_schemas([tested_schema], dump_dir, {"osBucketName": OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile": oci_config_file})

#@<> BUG#34599319 - test
shell.connect(__sandbox_uri2)
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, {"osBucketName": OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile": oci_config_file, "progressFile": ""}), "load_dump() with URL-encoded file names")
EXPECT_STDOUT_CONTAINS(f"1 tables in 1 schemas were loaded")

#@<> BUG#34599319 - cleanup
shell.connect(__sandbox_uri1)
session.run_sql("DROP SCHEMA IF EXISTS !;", [ tested_schema ])

#@<> BUG#34657730 - util.export_table() should output remote options needed to import this dump
# setup
tested_schema = "tested_schema"
tested_table = "tested_table"
dump_dir = "bug_34657730"

shell.connect(__sandbox_uri1)
session.run_sql("DROP SCHEMA IF EXISTS !;", [ tested_schema ])
session.run_sql("CREATE SCHEMA !;", [ tested_schema ])
session.run_sql("CREATE TABLE !.! (id INT NOT NULL PRIMARY KEY AUTO_INCREMENT, something BINARY)", [ tested_schema, tested_table ])
session.run_sql("INSERT INTO !.! (id) VALUES (302)", [ tested_schema, tested_table ])
session.run_sql("INSERT INTO !.! (something) VALUES (char(0))", [ tested_schema, tested_table ])

# prepare the dump
prepare_empty_bucket(OS_BUCKET_NAME, OS_NAMESPACE)
WIPE_OUTPUT()
util.export_table(quote_identifier(tested_schema, tested_table), dump_dir, {"osBucketName": OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile": oci_config_file})

# capture the import command
full_stdout_output = testutil.fetch_captured_stdout(False)
index_of_util = full_stdout_output.find("util.")
EXPECT_NE(-1, index_of_util)
util_import_table_code = full_stdout_output[index_of_util:]

#@<> BUG#34657730 - test
shell.connect(__sandbox_uri2)
wipeout_server(session2)

session.run_sql("CREATE SCHEMA !;", [ tested_schema ])
session.run_sql("CREATE TABLE !.! (id INT NOT NULL PRIMARY KEY AUTO_INCREMENT, something BINARY)", [ tested_schema, tested_table ])

EXPECT_NO_THROWS(lambda: exec(util_import_table_code), "importing data")
EXPECT_EQ(md5_table(session1, tested_schema, tested_table), md5_table(session2, tested_schema, tested_table))

#@<> BUG#34657730 - cleanup
shell.connect(__sandbox_uri1)
session.run_sql("DROP SCHEMA IF EXISTS !;", [ tested_schema ])

#@<> BUG#35462985 - util.export_table() to a remote location should not require a prefix to exist
EXPECT_NO_THROWS(lambda: util.export_table("sakila.actor", "new-dir/actors.tsv", {"osBucketName": OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "ociConfigFile": oci_config_file, "showProgress": False}), "export_table() to non-existing prefix")

#@<> Cleanup
testutil.rmfile("progress.txt")
testutil.destroy_sandbox(__mysql_sandbox_port1)
testutil.destroy_sandbox(__mysql_sandbox_port2)
