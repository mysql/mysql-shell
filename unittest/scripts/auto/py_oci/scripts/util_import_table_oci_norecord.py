#@ {has_oci_environment('OS')}

#@<> INCLUDE oci_utils.inc

#@<> Setup
import os

PRIVATE_BUCKET = 'testbkt'
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
session.run_sql('SET GLOBAL local_infile = true')

prepare_empty_bucket(PRIVATE_BUCKET, OS_NAMESPACE)
testutil.anycopy(os.path.join(__import_data_path, SOURCE_FILE), {'osBucketName': PRIVATE_BUCKET, 'osNamespace': OS_NAMESPACE, 'ociConfigFile': OCI_CONFIG_FILE, 'name': SOURCE_FILE})

#@<> oci+os:// scheme is not supported
EXPECT_THROWS(lambda: util.import_table('oci+os://region/tenancy/bucket/file'), 'ArgumentError: Util.import_table: File handling for oci+os protocol is not supported.')

#@<> test import using OCI
rc = testutil.call_mysqlsh([__sandbox_uri1, '--schema=' + TARGET_SCHEMA, '--', 'util', 'import-table', SOURCE_FILE, '--table=cities', '--os-bucket-name=' + PRIVATE_BUCKET, '--os-namespace=' + OS_NAMESPACE, '--oci-config-file=' + OCI_CONFIG_FILE])

EXPECT_EQ(0, rc)
EXPECT_STDOUT_CONTAINS("File '{0}' (209.75 KB) was imported in ".format(SOURCE_FILE))
EXPECT_STDOUT_CONTAINS('Total rows affected in {0}.cities: Records: 4079  Deleted: 0  Skipped: 0  Warnings: 0'.format(TARGET_SCHEMA))

#@<> Cleanup
delete_bucket(PRIVATE_BUCKET)

session.close()
testutil.destroy_sandbox(__mysql_sandbox_port1);
