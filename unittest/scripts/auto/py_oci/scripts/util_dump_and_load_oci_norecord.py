#@ {has_oci_environment('OS')}
# Test dump and load into/from OCI ObjectStorage

#@<> INCLUDE oci_utils.inc

#@<> INCLUDE dump_utils.inc

#@<> Setup

import oci

k_bucket_name="testbkt"
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

#@<> Create a dump with defaults into OCI, no prefix
prepare_empty_bucket(k_bucket_name, OS_NAMESPACE)

shell.connect(__sandbox_uri1)
util.dump_instance("", {"osBucketName":k_bucket_name, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file})

#@<> Load the dump from OCI with defaults
shell.connect(__sandbox_uri2)
util.load_dump("", {"osBucketName":k_bucket_name, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file})

compare_servers(session1, session2)

wipeout_server(session2)

#@<> Create a dump with defaults into OCI, with prefix
prepare_empty_bucket(k_bucket_name, OS_NAMESPACE)

shell.connect(__sandbox_uri1)
util.dump_instance("mydump", {"osBucketName":k_bucket_name, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file})

#@<> Try another dump in the same bucket with a different prefix
util.dump_schemas(["world"], "another/dump", {"osBucketName":k_bucket_name, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file})

#@<> Load the dump from OCI with defaults, with prefix
shell.connect(__sandbox_uri2)
util.load_dump("mydump", {"osBucketName":k_bucket_name, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file})

compare_servers(session1, session2)
delete_object(k_bucket_name, "mydump/load-progress."+uuid+".json", OS_NAMESPACE)
wipeout_server(session2)

#@<> Load different dump in the same bucket
util.load_dump("another/dump", {"osBucketName":k_bucket_name, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file})

compare_schema(session1, session2, "world")
res = session2.run_sql("select schema_name from information_schema.schemata where schema_name in ('world', 'sakila')").fetch_all()
EXPECT_EQ(1, len(res))
EXPECT_EQ("world", res[0][0])

wipeout_server(session2)

#@<> Cause a partial load
shell.connect(__sandbox_uri2)
testutil.create_file("sakila@film_text@@0.tsv.zst", "badfile")
testutil.anycopy({"osBucketName":k_bucket_name, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "name":"mydump/sakila@film_text@@0.tsv.zst"}, "backup")
testutil.anycopy("sakila@film_text@@0.tsv.zst", {"osBucketName":k_bucket_name, "osNamespace": OS_NAMESPACE, "ociConfigFile":oci_config_file, "name":"mydump/sakila@film_text@@0.tsv.zst"})

EXPECT_THROWS(lambda: util.load_dump("mydump", {"osBucketName":k_bucket_name, "osNamespace":OS_NAMESPACE, "ociConfigFile":oci_config_file}), "Error loading dump")

EXPECT_STDOUT_CONTAINS("sakila@film_text@@0.tsv.zst: zstd.read: Unknown frame descriptor")

testutil.anycopy("backup", {"osBucketName":k_bucket_name, "osNamespace":OS_NAMESPACE, "ociConfigFile":oci_config_file, "name":"mydump/sakila@film_text@@0.tsv.zst"})

#@<> Resume partial load
util.load_dump("mydump", {"osBucketName":k_bucket_name, "osNamespace":OS_NAMESPACE, "ociConfigFile":oci_config_file})

EXPECT_STDOUT_NOT_CONTAINS("Executing DDL script for ")
EXPECT_STDOUT_CONTAINS("sakila@film_text@@0.tsv.zst: Records: ")
compare_servers(session1, session2)
wipeout_server(session2)

#@<> Load dump with a local progress file
testutil.rmfile("progress.txt")
util.load_dump("mydump", {"osBucketName":k_bucket_name, "osNamespace":OS_NAMESPACE,  "ociConfigFile":oci_config_file, "progressFile":"progress.txt"})
open("progress.txt").read()

EXPECT_STDOUT_CONTAINS("Executing DDL script for ")
EXPECT_STDOUT_CONTAINS("sakila@film_text@@0.tsv.zst: Records: ")

#@<> Cleanup
testutil.rmfile("progress.txt")
testutil.destroy_sandbox(__mysql_sandbox_port1)
testutil.destroy_sandbox(__mysql_sandbox_port2)

