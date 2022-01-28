#@ {has_oci_environment('OS')}

#@<> INCLUDE oci_utils.inc

#@<> Setup
import os

OCI_CONFIG_FILE = os.path.join(OCI_CONFIG_HOME, "config")
TARGET_SCHEMA = 'world_x'
SOURCE_FILE = 'world_x_cities.dump'

testutil.deploy_raw_sandbox(__mysql_sandbox_port1, 'root', {'report_host': hostname})

shell.connect(__sandbox_uri1)
session.run_sql('DROP SCHEMA IF EXISTS ' + TARGET_SCHEMA)
session.run_sql('CREATE SCHEMA ' + TARGET_SCHEMA)
session.run_sql('USE ' + TARGET_SCHEMA)
session.run_sql('DROP TABLE IF EXISTS `cities`')
session.run_sql("CREATE TABLE `cities` (`ID` int(11) NOT NULL AUTO_INCREMENT, `Name` char(64) NOT NULL DEFAULT '', `CountryCode` char(3) NOT NULL DEFAULT '', `District` char(64) NOT NULL DEFAULT '', `Info` json DEFAULT NULL, PRIMARY KEY (`ID`)) ENGINE=InnoDB AUTO_INCREMENT=4080 DEFAULT CHARSET=utf8mb4")
session.run_sql('DROP TABLE IF EXISTS `lorem`')
session.run_sql("CREATE TABLE `lorem` (`id` int primary key, `part` text) ENGINE=InnoDB CHARSET=utf8mb4")
session.run_sql('SET GLOBAL local_infile = true')

prepare_empty_bucket(OS_BUCKET_NAME, OS_NAMESPACE)
testutil.anycopy(os.path.join(__import_data_path, SOURCE_FILE), {'osBucketName': OS_BUCKET_NAME, 'osNamespace': OS_NAMESPACE, 'ociConfigFile': OCI_CONFIG_FILE, 'name': SOURCE_FILE})

#@<> Retrieve directory content
chunked_dir = os.path.join(__import_data_path, "chunked")
dircontent = os.listdir(chunked_dir)
raw_files = sorted(list(filter(lambda x: x.endswith(".tsv"), dircontent)))
gz_files = sorted(list(filter(lambda x: x.endswith(".gz"), dircontent)))
zst_files = sorted(list(filter(lambda x: x.endswith(".zst"), dircontent)))
print("raw_files", raw_files)
print("gz_files", gz_files)
print("zst_files", zst_files)

#@<> Upload multiple files
for f in os.listdir(chunked_dir):
    print("Uploading", f)
    testutil.anycopy(os.path.join(chunked_dir, f), {'osBucketName': OS_BUCKET_NAME, 'osNamespace': OS_NAMESPACE, 'ociConfigFile': OCI_CONFIG_FILE, 'name': f})
    testutil.anycopy(os.path.join(chunked_dir, f), {'osBucketName': OS_BUCKET_NAME, 'osNamespace': OS_NAMESPACE, 'ociConfigFile': OCI_CONFIG_FILE, 'name': "parts/" + f})

#@<> Import from bucket root dir
util.import_table('lorem_a*', {'schema': TARGET_SCHEMA, 'table': 'lorem', 'osBucketName': OS_BUCKET_NAME, 'osNamespace': OS_NAMESPACE, 'ociConfigFile': OCI_CONFIG_FILE, 'replaceDuplicates': True})
EXPECT_STDOUT_CONTAINS("6 file(s) (14.75 KB) was imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in {0}.lorem: Records: 600  Deleted: 0  Skipped: 0  Warnings: 0".format(TARGET_SCHEMA))

#@<> Single file
util.import_table(raw_files[0], {'schema': TARGET_SCHEMA, 'table': 'lorem', 'osBucketName': OS_BUCKET_NAME, 'osNamespace': OS_NAMESPACE, 'ociConfigFile': OCI_CONFIG_FILE, 'replaceDuplicates': True})
EXPECT_STDOUT_CONTAINS("File '" + raw_files[0] + "' (2.39 KB) was imported in")
EXPECT_STDOUT_CONTAINS("Total rows affected in {0}.lorem: Records: 100  Deleted: 0  Skipped: 0  Warnings: 0".format(TARGET_SCHEMA))

#@<> Multiple files
util.import_table([raw_files[0], "parts/" + raw_files[1]], {'schema': TARGET_SCHEMA, 'table': 'lorem', 'osBucketName': OS_BUCKET_NAME, 'osNamespace': OS_NAMESPACE, 'ociConfigFile': OCI_CONFIG_FILE, 'replaceDuplicates': True})
EXPECT_STDOUT_CONTAINS("2 file(s) (4.88 KB) was imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in {0}.lorem: Records: 200  Deleted: 0  Skipped: 0  Warnings: 0".format(TARGET_SCHEMA))

#@<> Empty wildcard expansion
util.import_table(['lorem_xxx*', 'lorem_yyy.*'], {'schema': TARGET_SCHEMA, 'table': 'lorem', 'osBucketName': OS_BUCKET_NAME, 'osNamespace': OS_NAMESPACE, 'ociConfigFile': OCI_CONFIG_FILE, 'replaceDuplicates': True})
EXPECT_STDOUT_CONTAINS("0 file(s) (0 bytes) was imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in {0}.lorem: Records: 0  Deleted: 0  Skipped: 0  Warnings: 0".format(TARGET_SCHEMA))

#@<> Empty wildcard expansion and non-existing file
EXPECT_THROWS(lambda: util.import_table(['lorem_xxx*', 'lorem_yyy.tsv.zst'], {'schema': TARGET_SCHEMA, 'table': 'lorem', 'osBucketName': OS_BUCKET_NAME, 'osNamespace': OS_NAMESPACE, 'ociConfigFile': OCI_CONFIG_FILE, 'replaceDuplicates': True}),
    "Util.import_table: File lorem_yyy.tsv.zst does not exist."
)
EXPECT_STDOUT_CONTAINS("ERROR: File lorem_yyy.tsv.zst does not exist.")
EXPECT_STDOUT_CONTAINS("0 file(s) (0 bytes) was imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in {0}.lorem: Records: 0  Deleted: 0  Skipped: 0  Warnings: 0".format(TARGET_SCHEMA))

#@<> Wildcard on multiple compressed files
util.import_table(['*.gz', '*.zst'], {'schema': TARGET_SCHEMA, 'table': 'lorem', 'osBucketName': OS_BUCKET_NAME, 'osNamespace': OS_NAMESPACE, 'ociConfigFile': OCI_CONFIG_FILE, 'replaceDuplicates': True})
EXPECT_STDOUT_CONTAINS("11 file(s) (27.41 KB) was imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in {0}.lorem: Records: 1100  Deleted: 0  Skipped: 0  Warnings: 0".format(TARGET_SCHEMA))

#@<> Import from bucket subdirectory
util.import_table(['parts/*.gz', 'parts/*.zst'], {'schema': TARGET_SCHEMA, 'table': 'lorem', 'osBucketName': OS_BUCKET_NAME, 'osNamespace': OS_NAMESPACE, 'ociConfigFile': OCI_CONFIG_FILE, 'replaceDuplicates': True})
EXPECT_STDOUT_CONTAINS("11 file(s) (27.41 KB) was imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in {0}.lorem: Records: 1100  Deleted: 0  Skipped: 0  Warnings: 0".format(TARGET_SCHEMA))

#@<> Expand wildcard to empty file list in bucket subdirectory
util.import_table(['parts/xyz*.gz', 'parts/abc*.zst'], {'schema': TARGET_SCHEMA, 'table': 'lorem', 'osBucketName': OS_BUCKET_NAME, 'osNamespace': OS_NAMESPACE, 'ociConfigFile': OCI_CONFIG_FILE, 'replaceDuplicates': True})
EXPECT_STDOUT_CONTAINS("0 file(s) (0 bytes) was imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in {0}.lorem: Records: 0  Deleted: 0  Skipped: 0  Warnings: 0".format(TARGET_SCHEMA))

#@<> single file non-existing file bucket directory
EXPECT_THROWS(lambda: util.import_table('parts/lorem_xxx.gz', {'schema': TARGET_SCHEMA, 'table': 'lorem', 'osBucketName': OS_BUCKET_NAME, 'osNamespace': OS_NAMESPACE, 'ociConfigFile': OCI_CONFIG_FILE, 'replaceDuplicates': True}),
    "Error: Shell Error (54404): Util.import_table: Failed opening object 'parts/lorem_xxx.gz' in READ mode: Failed to get summary for object 'parts/lorem_xxx.gz': Not Found (404)"
)

#@<> single file from non existing bucket directory
EXPECT_THROWS(lambda: util.import_table('nonexisting/' + raw_files[0], {'schema': TARGET_SCHEMA, 'table': 'lorem', 'osBucketName': OS_BUCKET_NAME, 'osNamespace': OS_NAMESPACE, 'ociConfigFile': OCI_CONFIG_FILE, 'replaceDuplicates': True}),
    "Error: Shell Error (54404): Util.import_table: Failed opening object 'nonexisting/lorem_a1.tsv' in READ mode: Failed to get summary for object 'nonexisting/lorem_a1.tsv': Not Found (404)"
)

#@<> expand wildcard from non existing bucket directory
EXPECT_THROWS(lambda: util.import_table(['nonexisting/lorem*.gz', '', 'parts/*.gz', 'parts/*.zst'], {'schema': TARGET_SCHEMA, 'table': 'lorem', 'osBucketName': OS_BUCKET_NAME, 'osNamespace': OS_NAMESPACE, 'ociConfigFile': OCI_CONFIG_FILE, 'replaceDuplicates': True}),
    "Util.import_table: Directory nonexisting does not exist."
)
EXPECT_STDOUT_CONTAINS("Directory nonexisting does not exist.")

#@<> oci+os:// scheme is not supported
# util.import_table was moved from expose() to add_method() and it cause change in exception message
# EXPECT_THROWS(lambda: util.import_table('oci+os://region/tenancy/bucket/file'), 'ArgumentError: Util.import_table: File handling for oci+os protocol is not supported.')
EXPECT_THROWS(lambda: util.import_table('oci+os://region/tenancy/bucket/file'), 'Util.import_table: File handling for oci+os protocol is not supported.')

#@<> test import using OCI
rc = testutil.call_mysqlsh([__sandbox_uri1, '--schema=' + TARGET_SCHEMA, '--', 'util', 'import-table', SOURCE_FILE, '--table=cities', '--os-bucket-name=' + OS_BUCKET_NAME, '--os-namespace=' + OS_NAMESPACE, '--oci-config-file=' + OCI_CONFIG_FILE])

EXPECT_EQ(0, rc)
EXPECT_STDOUT_CONTAINS("File '{0}' (209.75 KB) was imported in ".format(SOURCE_FILE))
EXPECT_STDOUT_CONTAINS('Total rows affected in {0}.cities: Records: 4079  Deleted: 0  Skipped: 0  Warnings: 0'.format(TARGET_SCHEMA))

#@<> Cleanup
delete_bucket(OS_BUCKET_NAME)

session.close()
testutil.destroy_sandbox(__mysql_sandbox_port1)
