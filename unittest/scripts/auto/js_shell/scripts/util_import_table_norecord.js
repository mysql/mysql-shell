// This is unit test file for WL12193 Parallel data import
const target_port = __mysql_port
const target_xport = __port;
const target_schema = 'wl12193';
const uri = "mysql://" + __mysqluripwd;
const xuri = "mysqlx://" + __uripwd;

function wait_for_server(uri) {
    var max_retries = 60 * 5;
    const sleep_time = 1;  // second
    while (max_retries-- > 0) {
        try {
            if (shell.connect(uri)) {
                return true;
            }
            os.sleep(sleep_time);
        } catch (error) {
            println(error);
        }
    }
}

//<>@ Throw if session is empty
EXPECT_THROWS(function () {
    util.importTable(__import_data_path + '/world_x_cities.dump', { table: 'cities' });
}, "A classic protocol session is required to perform this operation.");


wait_for_server(xuri);

//@<> LOAD DATA is supported only by Classic Protocol
shell.connect(xuri)
EXPECT_THROWS(function () {
    util.importTable(__import_data_path + '/world_x_cities.dump',
        { schema: target_schema });
}, "A classic protocol session is required to perform this operation.");

//@<> Setup test
/// Create collection for json import
session.dropSchema(target_schema);
var schema_ = session.createSchema(target_schema);
schema_.createCollection('document_store');
session.close();

/// Create wl12193.cities table
shell.connect(uri);
// session.runSql('DROP SCHEMA IF EXISTS ' + target_schema);
// session.runSql('CREATE SCHEMA ' + target_schema);
session.runSql('USE ' + target_schema);
session.runSql('DROP TABLE IF EXISTS `cities`');
session.runSql("CREATE TABLE `cities` (`ID` int(11) NOT NULL AUTO_INCREMENT, `Name` char(64) NOT NULL DEFAULT '', `CountryCode` char(3) NOT NULL DEFAULT '', `District` char(64) NOT NULL DEFAULT '', `Info` json DEFAULT NULL, PRIMARY KEY (`ID`)) ENGINE=InnoDB AUTO_INCREMENT=4080 DEFAULT CHARSET=utf8mb4");


//@<> Throw if MySQL Server config option local_infile is false
session.runSql('SET GLOBAL local_infile = false');
EXPECT_THROWS(function () {
    util.importTable(__import_data_path + '/world_x_cities.dump', { schema: target_schema, table: 'cities' });
}, "Invalid preconditions");
EXPECT_STDOUT_CONTAINS("The 'local_infile' global system variable must be set to ON in the target server, after the server is verified to be trusted.");


//@<> Set local_infile to true
session.runSql('SET GLOBAL local_infile = true');

//@<> TSF1_1 and TSF1_2
/// TSF1_1: Validate that the load data operation is done using the current
/// active session in MySQL Shell for both methods: util.importTable and util.importJson.
/// TSF1_2:
/// Validate that the load data operation is done using the session created
/// through the command line for method util.importTable
var rc = testutil.callMysqlsh([uri, '--schema=' + target_schema, '--', 'util', 'import-table', __import_data_path + '/world_x_cities.dump', '--table=cities']);
EXPECT_EQ(0, rc);
EXPECT_STDOUT_CONTAINS("File '" + __import_data_path + "/world_x_cities.dump' (209.75 KB) was imported in ");
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".cities: Records: 4079  Deleted: 0  Skipped: 0  Warnings: 0");
session.runSql('TRUNCATE TABLE ' + target_schema + '.cities');

///@<> TSF3 - Match user specified threads number with performance_schema
/// Not aplicable

//@<> TSF8_1: Using util.importTable, validate that a file that meets the default value for field
/// and line terminators can be imported in parallel without specifying field and line terminators.
/// LOAD DATA with default dialect
util.importTable(__import_data_path + '/world_x_cities.dump', { schema: target_schema, table: 'cities' });
EXPECT_STDOUT_CONTAINS("File '" + __import_data_path + "/world_x_cities.dump' (209.75 KB) was imported in ");
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".cities: Records: 4079  Deleted: 0  Skipped: 0  Warnings: 0");

//@<> Ignore on duplicate primary key
util.importTable(__import_data_path + '/world_x_cities.dump', { schema: target_schema, table: 'cities' });
EXPECT_STDOUT_CONTAINS("File '" + __import_data_path + "/world_x_cities.dump' (209.75 KB) was imported in ");
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".cities: Records: 4079  Deleted: 0  Skipped: 4079  Warnings: 4079");


//@<OUT> Show world_x.cities table
original_output_format = shell.options.resultFormat
shell.options.resultFormat = 'tabbed'
session.runSql('select * from ' + target_schema + '.cities order by ID asc');
shell.options.resultFormat = original_output_format

//@<> TSF6_5: Validate that arbitrary record dump files can be imported in parallel to tables using util.importTable through the command line.
/// Ignore on duplicate primary key, import csv
// We are importing here the same data as 'world_x_cities.dump'
// but in csv format. All record should be reported as duplicates and skipped.
util.importTable(__import_data_path + '/world_x_cities.csv', {
    schema: target_schema, table: 'cities',
    fieldsTerminatedBy: ',', fieldsEnclosedBy: '"', fieldsOptionallyEnclosed: true, linesTerminatedBy: '\n'
});

EXPECT_STDOUT_CONTAINS("wl12193.cities: Records: 4079  Deleted: 0  Skipped: 4079  Warnings: 4079");
EXPECT_STDOUT_CONTAINS("WARNING: `wl12193`.`cities` error 1062: Duplicate entry '1' for key 'PRIMARY'");
EXPECT_STDOUT_CONTAINS("WARNING: `wl12193`.`cities` error 1062: Duplicate entry '2' for key 'PRIMARY'");
EXPECT_STDOUT_CONTAINS("WARNING: `wl12193`.`cities` error 1062: Duplicate entry '3' for key 'PRIMARY'");
EXPECT_STDOUT_CONTAINS("WARNING: `wl12193`.`cities` error 1062: Duplicate entry '4' for key 'PRIMARY'");
EXPECT_STDOUT_CONTAINS("WARNING: `wl12193`.`cities` error 1062: Duplicate entry '5' for key 'PRIMARY'");
EXPECT_STDOUT_CONTAINS("File '" + __import_data_path + "/world_x_cities.csv' (250.53 KB) was imported in ");
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".cities: Records: 4079  Deleted: 0  Skipped: 4079  Warnings: 4079");

//@<> TSF6_5 with skip header row
util.importTable(__import_data_path + '/world_x_cities_header.csv', {
    schema: target_schema, table: 'cities',
    fieldsTerminatedBy: ',', fieldsEnclosedBy: '"', fieldsOptionallyEnclosed: true, linesTerminatedBy: '\n', skipRows: 1
});

EXPECT_STDOUT_CONTAINS("wl12193.cities: Records: 4079  Deleted: 0  Skipped: 4079  Warnings: 4079");
EXPECT_STDOUT_CONTAINS("WARNING: `wl12193`.`cities` error 1062: Duplicate entry '1' for key 'PRIMARY'");
EXPECT_STDOUT_CONTAINS("WARNING: `wl12193`.`cities` error 1062: Duplicate entry '2' for key 'PRIMARY'");
EXPECT_STDOUT_CONTAINS("WARNING: `wl12193`.`cities` error 1062: Duplicate entry '3' for key 'PRIMARY'");
EXPECT_STDOUT_CONTAINS("WARNING: `wl12193`.`cities` error 1062: Duplicate entry '4' for key 'PRIMARY'");
EXPECT_STDOUT_CONTAINS("WARNING: `wl12193`.`cities` error 1062: Duplicate entry '5' for key 'PRIMARY'");
EXPECT_STDOUT_CONTAINS("File '" + __import_data_path + "/world_x_cities_header.csv' (250.57 KB) was imported in ");
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".cities: Records: 4079  Deleted: 0  Skipped: 4079  Warnings: 4079");


//@<> TSET_4: Using the function util.importTable, set the option 'replace' to true. Validate that new rows replace old rows if both have the same Unique Key Value.
/// Replace on duplicate primary key, import csv
util.importTable(__import_data_path + '/world_x_cities.csv', {
    schema: target_schema, table: 'cities',
    fieldsTerminatedBy: ',', fieldsEnclosedBy: '"', fieldsOptionallyEnclosed: true, linesTerminatedBy: '\n', replaceDuplicates: true
});

EXPECT_STDOUT_CONTAINS("wl12193.cities: Records: 4079  Deleted: 0  Skipped: 0  Warnings: 0");
EXPECT_STDOUT_CONTAINS("File '" + __import_data_path + "/world_x_cities.csv' (250.53 KB) was imported in ");
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".cities: Records: 4079  Deleted: 0  Skipped: 0  Warnings: 0");


//@<> fieldsEnclosedBy must be empty or a char.
EXPECT_THROWS(function () {
    util.importTable(__import_data_path + '/world_x_cities.csv', {
        schema: target_schema, table: 'cities',
        fieldsTerminatedBy: ',', fieldsEnclosedBy: 'more_than_one_char', linesTerminatedBy: '\n', replaceDuplicates: true
    });
}, "fieldsEnclosedBy must be empty or a char.");


//@<> "Separators cannot be the same or be a prefix of another."
EXPECT_THROWS(function () {
    util.importTable(__import_data_path + '/world_x_cities.csv', {
        schema: target_schema, table: 'cities',
        fieldsTerminatedBy: ',', fieldsEscapedBy: '\r', linesTerminatedBy: '\r\n'
    });
}, "Separators cannot be the same or be a prefix of another.");


//@<> Ensure that non_existing_schema does not exists
session.runSql("DROP SCHEMA IF EXISTS non_existing_schema");

//@<> Throw on unknown database on 5.7.24+ {VER(>=5.7.24) && VER(<8.0)}
EXPECT_THROWS(function () {
    util.importTable(__import_data_path + '/world_x_cities.dump', { schema: 'non_existing_schema', table: 'cities' });
}, "Table 'non_existing_schema.cities' doesn't exist");

//@<> Throw on unknown database {VER(<5.7.24) || VER(>=8.0)}
session.runSql("DROP SCHEMA IF EXISTS non_existing_schema");
EXPECT_THROWS(function () {
    util.importTable(__import_data_path + '/world_x_cities.dump', { schema: 'non_existing_schema', table: 'cities' });
}, "Unknown database 'non_existing_schema'");


//@<> TSF6_2: Validate that JSON files can be imported in parallel to tables using util.importTable. JSON document must be single line.
// Import JSON - one document per line
util.importTable(__import_data_path + '/primer-dataset-id.json', {
    schema: target_schema, table: 'document_store',
    columns: ['doc'], fieldsTerminatedBy: '\n', fieldsEnclosedBy: '',
    fieldsEscapedBy: '', linesTerminatedBy: '\n', bytesPerChunk: '20M', threads: 1
});
EXPECT_STDOUT_CONTAINS("wl12193.document_store: Records: 25359  Deleted: 0  Skipped: 0  Warnings: 0");
EXPECT_STDOUT_CONTAINS("File '" + __import_data_path + "/primer-dataset-id.json' (11.29 MB) was imported in ");
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".document_store: Records: 25359  Deleted: 0  Skipped: 0  Warnings: 0");

util.importTable(__import_data_path + '/primer-dataset-id.json', {
    schema: target_schema, table: 'document_store',
    columns: ['doc'], dialect: 'json', bytesPerChunk: '20M'
});
EXPECT_STDOUT_CONTAINS("wl12193.document_store: Records: 25359  Deleted: 0  Skipped: 25359  Warnings: 25359");
EXPECT_STDOUT_CONTAINS("WARNING: `wl12193`.`document_store` error 1062: Duplicate entry '000000000001' for key 'PRIMARY'");
EXPECT_STDOUT_CONTAINS("WARNING: `wl12193`.`document_store` error 1062: Duplicate entry '000000000002' for key 'PRIMARY'");
EXPECT_STDOUT_CONTAINS("WARNING: `wl12193`.`document_store` error 1062: Duplicate entry '000000000003' for key 'PRIMARY'");
EXPECT_STDOUT_CONTAINS("WARNING: `wl12193`.`document_store` error 1062: Duplicate entry '000000000004' for key 'PRIMARY'");
EXPECT_STDOUT_CONTAINS("WARNING: `wl12193`.`document_store` error 1062: Duplicate entry '000000000005' for key 'PRIMARY'");
EXPECT_STDOUT_CONTAINS("File '" + __import_data_path + "/primer-dataset-id.json' (11.29 MB) was imported in ");
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".document_store: Records: 25359  Deleted: 0  Skipped: 25359  Warnings: 25359");


///@<> Max bytes per chunk - minimum value is 2 * BUFFER_SIZE = 65KB * 2 = 128KB
util.importTable(__import_data_path + '/world_x_cities.dump', {
    schema: target_schema, table: 'cities',
    bytesPerChunk: '1'
});
EXPECT_STDOUT_CONTAINS("wl12193.cities: Records: 1523  Deleted: 0  Skipped: 1523  Warnings: 1523");
EXPECT_STDOUT_CONTAINS("wl12193.cities: Records: 2556  Deleted: 0  Skipped: 2556  Warnings: 2556");
EXPECT_STDOUT_CONTAINS("File '" + __import_data_path + "/world_x_cities.dump' (209.75 KB) was imported in ");
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".document_store: Records: 25359  Deleted: 0  Skipped: 25359  Warnings: 25359");


//@<> TSF5
/// Validate that the data transfer speeds is not greater than the value configured for maxRate by the user.
// Not aplicable.


//@<> Throw if no active schema and schema is not set in options as well
session.close();
shell.connect(uri);
EXPECT_THROWS(function () {
    util.importTable(__import_data_path + '/world_x_cities.dump', { table: 'cities' });
}, "There is no active schema on the current session, the target schema for the import operation must be provided in the options.");


//@<> Get schema from current active schema
shell.setCurrentSchema(target_schema);
util.importTable(__import_data_path + '/world_x_cities.dump', { table: 'cities' });
EXPECT_STDOUT_CONTAINS("wl12193.cities: Records: 4079  Deleted: 0  Skipped: 4079  Warnings: 4079");
EXPECT_STDOUT_CONTAINS("File '" + __import_data_path + "/world_x_cities.dump' (209.75 KB) was imported in ");
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".cities: Records: 4079  Deleted: 0  Skipped: 4079  Warnings: 4079");

//@<> BUG29822312 Import of table with foreign keys
session.runSql("CREATE TABLE `employee` (`id` int(11) NOT NULL AUTO_INCREMENT, `boss` int(11) DEFAULT NULL, PRIMARY KEY (`id`), KEY `boss` (`boss`), CONSTRAINT `employee_ibfk_1` FOREIGN KEY (`boss`) REFERENCES `employee` (`id`)) ENGINE=InnoDB;");
util.importTable(__import_data_path + '/employee_boss.csv', {table: 'employee', fieldsTerminatedBy: ','});
EXPECT_STDOUT_CONTAINS("wl12193.employee: Records: 7  Deleted: 0  Skipped: 0  Warnings: 0");
EXPECT_STDOUT_CONTAINS("File '" + __import_data_path + "/employee_boss.csv' (28 bytes) was imported in ");
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".employee: Records: 7  Deleted: 0  Skipped: 0  Warnings: 0");

//@<OUT> Show employee table
original_output_format = shell.options.resultFormat
shell.options.resultFormat = 'tabbed'
session.runSql('select * from ' + target_schema + '.employee order by boss asc');
shell.options.resultFormat = original_output_format


//@<> Teardown
session.runSql("DROP SCHEMA IF EXISTS " + target_schema);
session.close();

//<>@ Throw if session is closed
EXPECT_THROWS(function () {
    util.importTable(__import_data_path + '/world_x_cities.dump', { table: 'cities' });
}, "A classic protocol session is required to perform this operation.");
