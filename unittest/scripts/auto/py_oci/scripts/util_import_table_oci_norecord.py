#@ {has_oci_environment('OS')}

#@<> INCLUDE oci_utils.inc

#@<> INCLUDE dump_utils.inc

#@<> Setup
import os
import os.path

OCI_CONFIG_FILE = os.path.join(OCI_CONFIG_HOME, "config")
TARGET_SCHEMA = 'world_x'
TARGET_TABLE = 'cities'
SOURCE_FILE = 'world_x_cities.dump'

testutil.deploy_raw_sandbox(__mysql_sandbox_port1, 'root', {'report_host': hostname})

shell.connect(__sandbox_uri1)
session.run_sql('DROP SCHEMA IF EXISTS ' + TARGET_SCHEMA)
session.run_sql('CREATE SCHEMA ' + TARGET_SCHEMA)
session.run_sql('USE ' + TARGET_SCHEMA)
session.run_sql('DROP TABLE IF EXISTS ' + TARGET_TABLE)
session.run_sql("CREATE TABLE ! (`ID` int(11) NOT NULL AUTO_INCREMENT, `Name` char(64) NOT NULL DEFAULT '', `CountryCode` char(3) NOT NULL DEFAULT '', `District` char(64) NOT NULL DEFAULT '', `Info` json DEFAULT NULL, PRIMARY KEY (`ID`)) ENGINE=InnoDB AUTO_INCREMENT=4080 DEFAULT CHARSET=utf8mb4", [TARGET_TABLE])
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
EXPECT_STDOUT_CONTAINS("6 files (14.75 KB) were imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in {0}.lorem: Records: 600  Deleted: 0  Skipped: 0  Warnings: 0".format(TARGET_SCHEMA))

#@<> Single file
util.import_table(raw_files[0], {'schema': TARGET_SCHEMA, 'table': 'lorem', 'osBucketName': OS_BUCKET_NAME, 'osNamespace': OS_NAMESPACE, 'ociConfigFile': OCI_CONFIG_FILE, 'replaceDuplicates': True})
EXPECT_STDOUT_CONTAINS("File '" + raw_files[0] + "' (2.39 KB) was imported in")
EXPECT_STDOUT_CONTAINS("Total rows affected in {0}.lorem: Records: 100  Deleted: 0  Skipped: 0  Warnings: 0".format(TARGET_SCHEMA))

#@<> Multiple files
util.import_table([raw_files[0], "parts/" + raw_files[1]], {'schema': TARGET_SCHEMA, 'table': 'lorem', 'osBucketName': OS_BUCKET_NAME, 'osNamespace': OS_NAMESPACE, 'ociConfigFile': OCI_CONFIG_FILE, 'replaceDuplicates': True})
EXPECT_STDOUT_CONTAINS("2 files (4.88 KB) were imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in {0}.lorem: Records: 200  Deleted: 0  Skipped: 0  Warnings: 0".format(TARGET_SCHEMA))

#@<> Empty wildcard expansion
util.import_table(['lorem_xxx*', 'lorem_yyy.*'], {'schema': TARGET_SCHEMA, 'table': 'lorem', 'osBucketName': OS_BUCKET_NAME, 'osNamespace': OS_NAMESPACE, 'ociConfigFile': OCI_CONFIG_FILE, 'replaceDuplicates': True})
EXPECT_STDOUT_CONTAINS("0 files (0 bytes) were imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in {0}.lorem: Records: 0  Deleted: 0  Skipped: 0  Warnings: 0".format(TARGET_SCHEMA))

#@<> Empty wildcard expansion and non-existing file
EXPECT_THROWS(lambda: util.import_table(['lorem_xxx*', 'lorem_yyy.tsv.zst'], {'schema': TARGET_SCHEMA, 'table': 'lorem', 'osBucketName': OS_BUCKET_NAME, 'osNamespace': OS_NAMESPACE, 'ociConfigFile': OCI_CONFIG_FILE, 'replaceDuplicates': True}),
    "File lorem_yyy.tsv.zst does not exist."
)
EXPECT_STDOUT_CONTAINS("ERROR: File lorem_yyy.tsv.zst does not exist.")
EXPECT_STDOUT_CONTAINS("0 files (0 bytes) were imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in {0}.lorem: Records: 0  Deleted: 0  Skipped: 0  Warnings: 0".format(TARGET_SCHEMA))

#@<> Wildcard on multiple compressed files
util.import_table(['*.gz', '*.zst'], {'schema': TARGET_SCHEMA, 'table': 'lorem', 'osBucketName': OS_BUCKET_NAME, 'osNamespace': OS_NAMESPACE, 'ociConfigFile': OCI_CONFIG_FILE, 'replaceDuplicates': True})
EXPECT_STDOUT_CONTAINS("11 files (27.41 KB uncompressed, 13.79 KB compressed) were imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in {0}.lorem: Records: 1100  Deleted: 0  Skipped: 0  Warnings: 0".format(TARGET_SCHEMA))

#@<> Import from bucket subdirectory
util.import_table(['parts/*.gz', 'parts/*.zst'], {'schema': TARGET_SCHEMA, 'table': 'lorem', 'osBucketName': OS_BUCKET_NAME, 'osNamespace': OS_NAMESPACE, 'ociConfigFile': OCI_CONFIG_FILE, 'replaceDuplicates': True})
EXPECT_STDOUT_CONTAINS("11 files (27.41 KB uncompressed, 13.79 KB compressed) were imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in {0}.lorem: Records: 1100  Deleted: 0  Skipped: 0  Warnings: 0".format(TARGET_SCHEMA))

#@<> Expand wildcard to empty file list in bucket subdirectory
util.import_table(['parts/xyz*.gz', 'parts/abc*.zst'], {'schema': TARGET_SCHEMA, 'table': 'lorem', 'osBucketName': OS_BUCKET_NAME, 'osNamespace': OS_NAMESPACE, 'ociConfigFile': OCI_CONFIG_FILE, 'replaceDuplicates': True})
EXPECT_STDOUT_CONTAINS("0 files (0 bytes) were imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in {0}.lorem: Records: 0  Deleted: 0  Skipped: 0  Warnings: 0".format(TARGET_SCHEMA))

#@<> single file non-existing file bucket directory
EXPECT_THROWS(lambda: util.import_table('parts/lorem_xxx.gz', {'schema': TARGET_SCHEMA, 'table': 'lorem', 'osBucketName': OS_BUCKET_NAME, 'osNamespace': OS_NAMESPACE, 'ociConfigFile': OCI_CONFIG_FILE, 'replaceDuplicates': True}),
    "Error: Shell Error (54404): Failed opening object 'parts/lorem_xxx.gz' in READ mode: Failed to get summary for object 'parts/lorem_xxx.gz': Not Found (404)"
)

#@<> single file from non existing bucket directory
EXPECT_THROWS(lambda: util.import_table('nonexisting/' + raw_files[0], {'schema': TARGET_SCHEMA, 'table': 'lorem', 'osBucketName': OS_BUCKET_NAME, 'osNamespace': OS_NAMESPACE, 'ociConfigFile': OCI_CONFIG_FILE, 'replaceDuplicates': True}),
    "Error: Shell Error (54404): Failed opening object 'nonexisting/lorem_a1.tsv' in READ mode: Failed to get summary for object 'nonexisting/lorem_a1.tsv': Not Found (404)"
)

#@<> expand wildcard from non existing bucket directory
EXPECT_THROWS(lambda: util.import_table(['nonexisting/lorem*.gz', '', 'parts/*.gz', 'parts/*.zst'], {'schema': TARGET_SCHEMA, 'table': 'lorem', 'osBucketName': OS_BUCKET_NAME, 'osNamespace': OS_NAMESPACE, 'ociConfigFile': OCI_CONFIG_FILE, 'replaceDuplicates': True}),
    "Directory nonexisting does not exist."
)
EXPECT_STDOUT_CONTAINS("Directory nonexisting does not exist.")

#@<> oci+os:// scheme is not supported
EXPECT_THROWS(lambda: util.import_table('oci+os://region/tenancy/bucket/file'), 'File handling for oci+os protocol is not supported.')

#@<> test import using OCI
rc = testutil.call_mysqlsh([__sandbox_uri1, '--schema=' + TARGET_SCHEMA, '--', 'util', 'import-table', SOURCE_FILE, '--table=' + TARGET_TABLE, '--os-bucket-name=' + OS_BUCKET_NAME, '--os-namespace=' + OS_NAMESPACE, '--oci-config-file=' + OCI_CONFIG_FILE])

EXPECT_EQ(0, rc)
EXPECT_STDOUT_CONTAINS("File '{0}' (209.75 KB) was imported in ".format(SOURCE_FILE))
EXPECT_STDOUT_CONTAINS('Total rows affected in {0}.{1}: Records: 4079  Deleted: 0  Skipped: 0  Warnings: 0'.format(TARGET_SCHEMA, TARGET_TABLE))

#@<> BUG#35018278 skipRows=X should be applied even if a compressed file or multiple files are loaded
# setup
test_schema = "bug_35018278"
test_table = "t"
test_table_qualified = quote_identifier(test_schema, test_table)
test_rows = 10
output_dir = test_schema
# create the directory
put_object(OS_NAMESPACE, OS_BUCKET_NAME, f"{output_dir}/tmp", "")

session.run_sql("DROP SCHEMA IF EXISTS !", [ test_schema ])
session.run_sql("CREATE SCHEMA !", [ test_schema ])
session.run_sql(f"CREATE TABLE {test_table_qualified} (k INT PRIMARY KEY, v TEXT)")

for i in range(test_rows):
    session.run_sql(f"INSERT INTO {test_table_qualified} VALUES ({i}, REPEAT('a', 10000))")

for compression, extension in { "none": "", "zstd": ".zst" }.items():
    util.export_table(test_table_qualified, f"{output_dir}/1.tsv{extension}", { "fieldsEnclosedBy": "'", "linesTerminatedBy": "a", "compression": compression, "where": f"k < {test_rows / 2}", "showProgress": False, 'osBucketName': OS_BUCKET_NAME, 'osNamespace': OS_NAMESPACE, 'ociConfigFile': OCI_CONFIG_FILE })
    util.export_table(test_table_qualified, f"{output_dir}/2.tsv{extension}", { "fieldsEnclosedBy": "'", "linesTerminatedBy": "a", "compression": compression, "where": f"k >= {test_rows / 2}", "showProgress": False, 'osBucketName': OS_BUCKET_NAME, 'osNamespace': OS_NAMESPACE, 'ociConfigFile': OCI_CONFIG_FILE })

#@<> BUG#35018278 - tests
for extension in [ "", ".zst" ]:
    for files in [ [ "1.tsv", "2.tsv" ], [ "*.tsv" ] ]:
        for skip in range(int(test_rows / 2) + 2):
            context = f"skip: {skip}"
            session.run_sql(f"TRUNCATE TABLE {test_table_qualified}")
            for f in files:
                EXPECT_NO_THROWS(lambda: util.import_table(f"{output_dir}/{f}{extension}", { "skipRows": skip, "schema": test_schema, "table": test_table, "fieldsEnclosedBy": "'", "linesTerminatedBy": "a", "showProgress": False, 'osBucketName': OS_BUCKET_NAME, 'osNamespace': OS_NAMESPACE, 'ociConfigFile': OCI_CONFIG_FILE }), f"file: {f}{extension}, {context}")
            EXPECT_EQ(max(test_rows - 2 * skip, 0), session.run_sql(f"SELECT COUNT(*) FROM {test_table_qualified}").fetch_one()[0], f"files: {files}, extension: {extension}, {context}")

#@<> BUG#35018278 - cleanup
session.run_sql("DROP SCHEMA IF EXISTS !", [ test_schema ])

#@<> BUG#35313366 - exception when importing a single uncompressed file crashes shell {not __dbug_off}
testutil.set_trap("os_bucket", ["op == get_object", f"name == {raw_files[0]}"], {"code": 404, "msg": "Injected exception"})

EXPECT_THROWS(lambda: util.import_table(raw_files[0], {'schema': TARGET_SCHEMA, 'table': 'lorem', 'osBucketName': OS_BUCKET_NAME, 'osNamespace': OS_NAMESPACE, 'ociConfigFile': OCI_CONFIG_FILE, 'replaceDuplicates': True}), "Injected exception (404)")

testutil.clear_traps("os_bucket")

#@<> BUG#35895247 - importing a file with escaped wildcard characters should load it in chunks {__os_type != "windows"}
testutil.anycopy(os.path.join(__import_data_path, SOURCE_FILE), {'osBucketName': OS_BUCKET_NAME, 'osNamespace': OS_NAMESPACE, 'ociConfigFile': OCI_CONFIG_FILE, 'name': "will *this work?"})
session.run_sql(f"TRUNCATE TABLE {quote_identifier(TARGET_SCHEMA, TARGET_TABLE)}")

EXPECT_NO_THROWS(lambda: util.import_table("will \\*this work\\?", { "schema": TARGET_SCHEMA, "table": TARGET_TABLE, "showProgress": False, 'osBucketName': OS_BUCKET_NAME, 'osNamespace': OS_NAMESPACE, 'ociConfigFile': OCI_CONFIG_FILE }), "import should not fail")
EXPECT_STDOUT_CONTAINS(f"Importing from file 'will *this work?' to table ")

#@<> WL15884-TSFR_4_1 - export/import without `ociAuth`
prepare_empty_bucket(OS_BUCKET_NAME, OS_NAMESPACE)
EXPECT_NO_THROWS(lambda: util.export_table(quote_identifier(TARGET_SCHEMA, TARGET_TABLE), "exported.tsv", {"osBucketName": OS_BUCKET_NAME, "ociConfigFile": OCI_CONFIG_FILE, "showProgress": False}), "export_table() without ociAuth")

session.run_sql(f"TRUNCATE TABLE {quote_identifier(TARGET_SCHEMA, TARGET_TABLE)}")
EXPECT_NO_THROWS(lambda: util.import_table("exported.tsv", {"osBucketName": OS_BUCKET_NAME, "ociConfigFile": OCI_CONFIG_FILE, "schema": TARGET_SCHEMA, "table": TARGET_TABLE, "showProgress": False}), "import_table() without ociAuth")

#@<> WL15884-TSFR_5_1 - export/import with `ociAuth` = 'api_key'
prepare_empty_bucket(OS_BUCKET_NAME, OS_NAMESPACE)
EXPECT_NO_THROWS(lambda: util.export_table(quote_identifier(TARGET_SCHEMA, TARGET_TABLE), "exported.tsv", {"ociAuth": "api_key", "osBucketName": OS_BUCKET_NAME, "ociConfigFile": OCI_CONFIG_FILE, "showProgress": False}), "export_table() with `ociAuth` = 'api_key'")

session.run_sql(f"TRUNCATE TABLE {quote_identifier(TARGET_SCHEMA, TARGET_TABLE)}")
EXPECT_NO_THROWS(lambda: util.import_table("exported.tsv", {"ociAuth": "api_key", "osBucketName": OS_BUCKET_NAME, "ociConfigFile": OCI_CONFIG_FILE, "schema": TARGET_SCHEMA, "table": TARGET_TABLE, "showProgress": False}), "import_table() with `ociAuth` = 'api_key'")

#@<> WL15884 - check if this host supports 'instance_principal' authentication
prepare_empty_bucket(OS_BUCKET_NAME, OS_NAMESPACE)

try:
    util.export_table(quote_identifier(TARGET_SCHEMA, TARGET_TABLE), "exported.tsv", {"ociAuth": "instance_principal", "osBucketName": OS_BUCKET_NAME, "showProgress": False})
    instance_principal_host = True
except Exception as e:
    print(e)
    instance_principal_host = False

#@<> WL15884-TSFR_6_1 - export/import with `ociAuth` = 'instance_principal' {instance_principal_host}
prepare_empty_bucket(OS_BUCKET_NAME, OS_NAMESPACE)
EXPECT_NO_THROWS(lambda: util.export_table(quote_identifier(TARGET_SCHEMA, TARGET_TABLE), "exported.tsv", {"ociAuth": "instance_principal", "osBucketName": OS_BUCKET_NAME, "showProgress": False}), "export_table() with `ociAuth` = 'instance_principal'")

session.run_sql(f"TRUNCATE TABLE {quote_identifier(TARGET_SCHEMA, TARGET_TABLE)}")
EXPECT_NO_THROWS(lambda: util.import_table("exported.tsv", {"ociAuth": "instance_principal", "osBucketName": OS_BUCKET_NAME, "osNamespace": OS_NAMESPACE, "schema": TARGET_SCHEMA, "table": TARGET_TABLE, "showProgress": False}), "import_table() with `ociAuth` = 'instance_principal'")

#@<> WL15884-TSFR_8_1 - export/import with `ociAuth` = 'security_token'
# prepare token and config file
token_path = os.path.join(__tmp_dir, "oci_security_token")

with open(token_path, "w") as f:
    f.write(get_session_token(OCI_CONFIG_FILE))

current_config = read_config_file(OCI_CONFIG_FILE)
new_config = {"security_token_file": token_path, "region": current_config["region"], "tenancy": current_config["tenancy"], "key_file": current_config["key_file"]}
if "pass_phrase" in current_config:
    new_config["pass_phrase"] = current_config["pass_phrase"]
config_path = os.path.join(__tmp_dir, "oci_config_file_with_token")
write_config_file(config_path, new_config)

# tests
prepare_empty_bucket(OS_BUCKET_NAME, OS_NAMESPACE)
EXPECT_NO_THROWS(lambda: util.export_table(quote_identifier(TARGET_SCHEMA, TARGET_TABLE), "exported.tsv", {"ociAuth": "security_token", "osBucketName": OS_BUCKET_NAME, "ociConfigFile": config_path, "showProgress": False}), "export_table() with `ociAuth` = 'security_token'")

session.run_sql(f"TRUNCATE TABLE {quote_identifier(TARGET_SCHEMA, TARGET_TABLE)}")
EXPECT_NO_THROWS(lambda: util.import_table("exported.tsv", {"ociAuth": "security_token", "osBucketName": OS_BUCKET_NAME, "ociConfigFile": config_path, "schema": TARGET_SCHEMA, "table": TARGET_TABLE, "showProgress": False}), "import_table() with `ociAuth` = 'security_token'")

#@<> Cleanup
delete_bucket(OS_BUCKET_NAME)

session.close()
testutil.destroy_sandbox(__mysql_sandbox_port1)
