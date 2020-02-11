//@{testutil.validateOciConfig()}
testutil.uploadOciObject('shell-rut-priv', 'world_x_cities.dump', __import_data_path + '/world_x_cities.dump');

testutil.deployRawSandbox(__mysql_sandbox_port1, 'root', {report_host: hostname});

const target_schema = 'world_x';

shell.connect(__sandbox_uri1);
session.runSql('DROP SCHEMA IF EXISTS ' + target_schema);
session.runSql('CREATE SCHEMA ' + target_schema);
session.runSql('USE ' + target_schema);
session.runSql('DROP TABLE IF EXISTS `cities`');
session.runSql("CREATE TABLE `cities` (`ID` int(11) NOT NULL AUTO_INCREMENT, `Name` char(64) NOT NULL DEFAULT '', `CountryCode` char(3) NOT NULL DEFAULT '', `District` char(64) NOT NULL DEFAULT '', `Info` json DEFAULT NULL, PRIMARY KEY (`ID`)) ENGINE=InnoDB AUTO_INCREMENT=4080 DEFAULT CHARSET=utf8mb4");
session.runSql('SET GLOBAL local_infile = true');

var oci_config = testutil.getOciConfig();
const file_path = 'oci+os://' + oci_config.region + '/mysql2/shell-rut-priv/world_x_cities.dump';

var rc = testutil.callMysqlsh([__sandbox_uri1, '--schema=' + target_schema, '--', 'util', 'import-table', file_path, '--table=cities']);

testutil.deleteOciObject('shell-rut-priv', 'world_x_cities.dump');

EXPECT_STDOUT_CONTAINS("File 'world_x_cities.dump' (209.75 KB) was imported in ");
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".cities: Records: 4079  Deleted: 0  Skipped: 0  Warnings: 0");

testutil.destroySandbox(__mysql_sandbox_port1);

