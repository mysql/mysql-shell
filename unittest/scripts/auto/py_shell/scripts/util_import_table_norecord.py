# This is unit test file for WL12193 Parallel data import
import os

target_port = __mysql_port
target_xport = __port
target_schema = 'wl12193'
uri = "mysql://" + __mysqluripwd
xuri = "mysqlx://" + __uripwd

if __os_type != "windows":
    def filename_for_output(filename):
        return filename
else:
    def filename_for_output(filename):
        long_path_prefix = r"\\?" "\\"
        return long_path_prefix + filename.replace("/", "\\")

#@<> Throw if session is empty
EXPECT_THROWS(lambda: util.import_table(__import_data_path + '/world_x_cities.dump', { "table": 'cities' }),
    "A classic protocol session is required to perform this operation.")


testutil.wait_sandbox_alive(xuri)

#@<> LOAD DATA is supported only by Classic Protocol
shell.connect(xuri)
EXPECT_THROWS(lambda: util.import_table(__import_data_path + '/world_x_cities.dump',
        { "schema": target_schema }),
"A classic protocol session is required to perform this operation.")

#@<> Setup test
#/ Create collection for json import
session.drop_schema(target_schema)
schema_ = session.create_schema(target_schema)
schema_.create_collection('document_store')
session.close()

#/ Create wl12193.cities table
shell.connect(uri)
# session.run_sql('DROP SCHEMA IF EXISTS ' + target_schema)
# session.run_sql('CREATE SCHEMA ' + target_schema)
session.run_sql('USE ' + target_schema)
session.run_sql('DROP TABLE IF EXISTS `cities`')
session.run_sql("""CREATE TABLE `cities` (`ID` int(11) NOT NULL AUTO_INCREMENT, `Name` char(64) NOT NULL DEFAULT '', `CountryCode` char(3) NOT NULL DEFAULT '', `District` char(64) NOT NULL DEFAULT '', `Info` json DEFAULT NULL, PRIMARY KEY (`ID`)) ENGINE=InnoDB AUTO_INCREMENT=4080 DEFAULT CHARSET=utf8mb4""")


#@<> Throw if MySQL Server config option local_infile is false
session.run_sql('SET GLOBAL local_infile = false')
EXPECT_THROWS(lambda: util.import_table(__import_data_path + '/world_x_cities.dump', { "schema": target_schema, "table": 'cities' }),
    "Invalid preconditions")

EXPECT_STDOUT_CONTAINS("The 'local_infile' global system variable must be set to ON in the target server, after the server is verified to be trusted.")


#@<> Set local_infile to true
session.run_sql('SET GLOBAL local_infile = true')

#@<> TSF1_1 and TSF1_2
#/ TSF1_1: Validate that the load data operation is done using the current
#/ active session in MySQL Shell for both methods: util.import_table
#/ TSF1_2:
#/ Validate that the load data operation is done using the session created
#/ through the command line for method util.import_table
rc = testutil.call_mysqlsh([uri, '--schema=' + target_schema, '--', 'util', 'import-table', __import_data_path + '/world_x_cities.dump', '--table=cities'])
EXPECT_EQ(0, rc)
EXPECT_STDOUT_CONTAINS("File '" + filename_for_output(__import_data_path + "/world_x_cities.dump") + "' (209.75 KB) was imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".cities: Records: 4079  Deleted: 0  Skipped: 0  Warnings: 0")
session.run_sql('TRUNCATE TABLE ' + target_schema + '.cities')

#/@<> TSF3 - Match user specified threads number with performance_schema
#/ Not aplicable

#@<> TSF8_1: Using util.import_table, validate that a file that meets the default value for field
#/ and line terminators can be imported in parallel without specifying field and line terminators.
#/ LOAD DATA with default dialect
util.import_table(__import_data_path + '/world_x_cities.dump', { "schema": target_schema, "table": 'cities' })
EXPECT_STDOUT_CONTAINS("File '" + filename_for_output(__import_data_path + "/world_x_cities.dump") + "' (209.75 KB) was imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".cities: Records: 4079  Deleted: 0  Skipped: 0  Warnings: 0")

#@<> Ignore on duplicate primary key
util.import_table(__import_data_path + '/world_x_cities.dump', { "schema": target_schema, "table": 'cities' })
EXPECT_STDOUT_CONTAINS("File '" + filename_for_output(__import_data_path + "/world_x_cities.dump") + "' (209.75 KB) was imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".cities: Records: 4079  Deleted: 0  Skipped: 4079  Warnings: 4079")


#@<OUT> Show world_x.cities table
original_output_format = shell.options.resultFormat
shell.options.resultFormat = 'table'
session.run_sql('select * from ' + target_schema + '.cities order by ID asc')
shell.options.resultFormat = original_output_format

#@<> TSF6_5: Validate that arbitrary record dump files can be imported in parallel to tables using util.import_table through the command line.
#/ Ignore on duplicate primary key, import csv
# We are importing here the same data as 'world_x_cities.dump'
# but in csv format. All record should be reported as duplicates and skipped.
util.import_table(__import_data_path + '/world_x_cities.csv', {
    "schema": target_schema, "table": 'cities',
    "fieldsTerminatedBy": ',', "fieldsEnclosedBy": '"', "fieldsOptionallyEnclosed": True, "linesTerminatedBy": '\n'
})

if testutil.version_check(__version, '<', '8.0.19'):
  prefix = ""
else:
  prefix = "cities."

EXPECT_STDOUT_CONTAINS("wl12193.cities: Records: 4079  Deleted: 0  Skipped: 4079  Warnings: 4079")
EXPECT_STDOUT_CONTAINS("WARNING: world_x_cities.csv error 1062: Duplicate entry '1' for key '{0}PRIMARY'".format(prefix))
EXPECT_STDOUT_CONTAINS("WARNING: world_x_cities.csv error 1062: Duplicate entry '2' for key '{0}PRIMARY'".format(prefix))
EXPECT_STDOUT_CONTAINS("WARNING: world_x_cities.csv error 1062: Duplicate entry '3' for key '{0}PRIMARY'".format(prefix))
EXPECT_STDOUT_CONTAINS("WARNING: world_x_cities.csv error 1062: Duplicate entry '4' for key '{0}PRIMARY'".format(prefix))
EXPECT_STDOUT_CONTAINS("WARNING: world_x_cities.csv error 1062: Duplicate entry '5' for key '{0}PRIMARY'".format(prefix))
EXPECT_STDOUT_CONTAINS("File '" + filename_for_output(__import_data_path + "/world_x_cities.csv") + "' (250.53 KB) was imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".cities: Records: 4079  Deleted: 0  Skipped: 4079  Warnings: 4079")

#@<> TSET_4: Using the function util.import_table, set the option 'replace' to true. Validate that new rows replace old rows if both have the same Unique Key Value.
#/ Replace on duplicate primary key, import csv
util.import_table(__import_data_path + '/world_x_cities.csv', {
    "schema": target_schema, "table": 'cities',
    "fieldsTerminatedBy": ',', "fieldsEnclosedBy": '"', "fieldsOptionallyEnclosed": True, "linesTerminatedBy": '\n', "replaceDuplicates": True
})

EXPECT_STDOUT_CONTAINS("wl12193.cities: Records: 4079  Deleted: 0  Skipped: 0  Warnings: 0")
EXPECT_STDOUT_CONTAINS("File '" + filename_for_output(__import_data_path + "/world_x_cities.csv") + "' (250.53 KB) was imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".cities: Records: 4079  Deleted: 0  Skipped: 0  Warnings: 0")


#@<> fieldsEnclosedBy must be empty or a char.
EXPECT_THROWS(lambda: util.import_table(__import_data_path + '/world_x_cities.csv', {
        "schema": target_schema, "table": 'cities',
        "fieldsTerminatedBy": ',', "fieldsEnclosedBy": 'more_than_one_char', "linesTerminatedBy": '\n', "replaceDuplicates": True
    }), "fieldsEnclosedBy must be empty or a char.")


#@<> "Separators cannot be the same or be a prefix of another."
EXPECT_THROWS(lambda: util.import_table(__import_data_path + '/world_x_cities.csv', {
        "schema": target_schema, "table": 'cities',
        "fieldsTerminatedBy": ',', "fieldsEscapedBy": '\r', "linesTerminatedBy": '\r\n'
    }), "Separators cannot be the same or be a prefix of another.")


#@<> Ensure that non_existing_schema does not exists
session.run_sql("DROP SCHEMA IF EXISTS non_existing_schema")

#@<> Throw on unknown database
session.run_sql("DROP SCHEMA IF EXISTS non_existing_schema")
EXPECT_THROWS(lambda: util.import_table(__import_data_path + '/world_x_cities.dump', { "schema": 'non_existing_schema', "table": 'cities' }),
    "Unknown database 'non_existing_schema'")


#@<> TSF6_2: Validate that JSON files can be imported in parallel to tables using util.import_table. JSON document must be single line.
# Import JSON - one document per line
util.import_table(__import_data_path + '/primer-dataset-id.json', {
    "schema": target_schema, "table": 'document_store',
    "columns": ['doc'], "fieldsTerminatedBy": '\n', "fieldsEnclosedBy": '',
    "fieldsEscapedBy": '', "linesTerminatedBy": '\n', "bytesPerChunk": '20M', "threads": 1})

EXPECT_STDOUT_CONTAINS("wl12193.document_store: Records: 25359  Deleted: 0  Skipped: 0  Warnings: 0")
EXPECT_STDOUT_CONTAINS("File '" + filename_for_output(__import_data_path + "/primer-dataset-id.json") + "' (11.29 MB) was imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".document_store: Records: 25359  Deleted: 0  Skipped: 0  Warnings: 0")

WIPE_OUTPUT()
util.import_table(__import_data_path + '/primer-dataset-id.json', {
    "schema": target_schema, "table": 'document_store',
    "columns": ['doc'], "dialect": 'json', "bytesPerChunk": '20M'})

if testutil.version_check(__version, '<', '8.0.19'):
  prefix = ""
else:
  prefix = "document_store."

EXPECT_STDOUT_CONTAINS("wl12193.document_store: Records: 25359  Deleted: 0  Skipped: 25359  Warnings: 25359")
EXPECT_STDOUT_CONTAINS("WARNING: primer-dataset-id.json error 1062: Duplicate entry '000000000001' for key '{0}PRIMARY'".format(prefix))
EXPECT_STDOUT_CONTAINS("WARNING: primer-dataset-id.json error 1062: Duplicate entry '000000000002' for key '{0}PRIMARY'".format(prefix))
EXPECT_STDOUT_CONTAINS("WARNING: primer-dataset-id.json error 1062: Duplicate entry '000000000003' for key '{0}PRIMARY'".format(prefix))
EXPECT_STDOUT_CONTAINS("WARNING: primer-dataset-id.json error 1062: Duplicate entry '000000000004' for key '{0}PRIMARY'".format(prefix))
EXPECT_STDOUT_CONTAINS("WARNING: primer-dataset-id.json error 1062: Duplicate entry '000000000005' for key '{0}PRIMARY'".format(prefix))
EXPECT_STDOUT_CONTAINS("File '" + filename_for_output(__import_data_path + "/primer-dataset-id.json") + "' (11.29 MB) was imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".document_store: Records: 25359  Deleted: 0  Skipped: 25359  Warnings: 25359")


#@<> Max bytes per chunk - minimum value is 2 * BUFFER_SIZE = 65KB * 2 = 128KB
util.import_table(__import_data_path + '/world_x_cities.dump', {
    "schema": target_schema, "table": 'cities',
    "bytesPerChunk": '1'
})

EXPECT_STDOUT_CONTAINS("world_x_cities.dump: Records: 1523  Deleted: 0  Skipped: 1523  Warnings: 1523")
EXPECT_STDOUT_CONTAINS("world_x_cities.dump: Records: 2556  Deleted: 0  Skipped: 2556  Warnings: 2556")
EXPECT_STDOUT_CONTAINS("File '" + filename_for_output(__import_data_path + "/world_x_cities.dump") + "' (209.75 KB) was imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".cities: Records: 4079  Deleted: 0  Skipped: 4079  Warnings: 4079")


#@<> TSF5
#/ Validate that the data transfer speeds is not greater than the value configured for maxRate by the user.
# Not aplicable.


#@<> Throw if no active schema and schema is not set in options as well
session.close()
shell.connect(uri)
EXPECT_THROWS(lambda: util.import_table(__import_data_path + '/world_x_cities.dump', { "table": 'cities' }),
    "There is no active schema on the current session, the target schema for the import operation must be provided in the options.")


#@<> Get schema from current active schema
shell.set_current_schema(target_schema)
util.import_table(__import_data_path + '/world_x_cities.dump', { "table": 'cities' })
EXPECT_STDOUT_CONTAINS("wl12193.cities: Records: 4079  Deleted: 0  Skipped: 4079  Warnings: 4079")
EXPECT_STDOUT_CONTAINS("File '" + filename_for_output(__import_data_path + "/world_x_cities.dump") + "' (209.75 KB) was imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".cities: Records: 4079  Deleted: 0  Skipped: 4079  Warnings: 4079")

#@<> BUG29822312 Import of table with foreign keys
session.run_sql("CREATE TABLE `employee` (`id` int(11) NOT NULL AUTO_INCREMENT, `boss` int(11) DEFAULT NULL, PRIMARY KEY (`id`), KEY `boss` (`boss`), CONSTRAINT `employee_ibfk_1` FOREIGN KEY (`boss`) REFERENCES `employee` (`id`)) ENGINE=InnoDB;")
util.import_table(__import_data_path + '/employee_boss.csv', {'table': 'employee', 'fieldsTerminatedBy': ','})
EXPECT_STDOUT_CONTAINS("wl12193.employee: Records: 7  Deleted: 0  Skipped: 0  Warnings: 0")
EXPECT_STDOUT_CONTAINS("File '" + filename_for_output(__import_data_path + "/employee_boss.csv") + "' (28 bytes) was imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".employee: Records: 7  Deleted: 0  Skipped: 0  Warnings: 0")

#@<OUT> Show employee table
original_output_format = shell.options.resultFormat
shell.options.resultFormat = 'tabbed'
session.run_sql('select * from ' + target_schema + '.employee order by boss asc')
shell.options.resultFormat = original_output_format

#@<> Create latin2 charset table
session.run_sql("create table cities_latin2(id integer primary key, name text) CHARACTER SET = latin2")

#@<OUT> Import to table with utf8 character set
util.import_table(__import_data_path + '/cities_pl_utf8.dump', {'table':'cities_latin2', 'characterSet': 'utf8mb4'})
shell.dump_rows(session.run_sql('select hex(id), hex(name) from cities_latin2'), "tabbed")
session.run_sql("truncate table cities_latin2")

#@<OUT> Import to table with latin2 character set
util.import_table(__import_data_path + '/cities_pl_latin2.dump', {'table':'cities_latin2', 'characterSet': 'latin2'})
shell.dump_rows(session.run_sql('select hex(id), hex(name) from cities_latin2'), "tabbed")

#@<> BUG#31407133 IF SQL_MODE='NO_BACKSLASH_ESCAPES' IS SET, UTIL.IMPORTTABLE FAILS TO IMPORT DATA
original_global_sqlmode = session.run_sql("SELECT @@global.sql_mode").fetch_one()[0]
session.run_sql("SET GLOBAL SQL_MODE='NO_BACKSLASH_ESCAPES'")

session.run_sql('TRUNCATE TABLE !.cities', [ target_schema ])
EXPECT_NO_THROWS(lambda: util.import_table(os.path.join(__import_data_path, 'world_x_cities.dump'), { "schema": target_schema, "table": 'cities' }), "importing when SQL mode is set")

session.run_sql("SET GLOBAL SQL_MODE=?", [original_global_sqlmode])

#@<> BUG#33360787 loading when auto-commit set to OFF
original_global_autocommit = session.run_sql("SELECT @@GLOBAL.autocommit").fetch_one()[0]
session.run_sql("SET @@GLOBAL.autocommit = OFF")

checksum = session.run_sql('CHECKSUM TABLE !.cities', [ target_schema ]).fetch_one()[1]

session.run_sql('TRUNCATE TABLE !.cities', [ target_schema ])
EXPECT_NO_THROWS(lambda: util.import_table(os.path.join(__import_data_path, 'world_x_cities.dump'), { "schema": target_schema, "table": 'cities' }), "importing when auto-commit is OFF")
EXPECT_EQ(checksum, session.run_sql('CHECKSUM TABLE !.cities', [ target_schema ]).fetch_one()[1])

session.run_sql("SET @@GLOBAL.autocommit = ?", [original_global_autocommit])

#@<> BUG#31412330 UTIL.IMPORTTABLE(): UNABLE TO IMPORT DATA INTO A TABLE WITH NON-ASCII NAME
original_global_sqlmode = session.run_sql("SELECT @@global.sql_mode").fetch_one()[0]
session.run_sql("SET GLOBAL SQL_MODE=''")
old_cs_client = session.run_sql("SELECT @@session.character_set_client").fetch_one()[0]
old_cs_connection = session.run_sql("SELECT @@session.character_set_connection").fetch_one()[0]
old_cs_results = session.run_sql("SELECT @@session.character_set_results").fetch_one()[0]

# non-ASCII character has to have the UTF-8 representation which consists of bytes which correspond to non-capital Unicode letters,
# because on Windows capital letters will be converted to small letters, resulting in invalid UTF-8 sequence, messing up with reported errors
target_table = 'citi→s'
target_character_set = 'latin1'

session.run_sql("SET NAMES ?", [ target_character_set ])
session.run_sql("CREATE TABLE !.! LIKE !.!;", [ target_schema, target_table, target_schema, 'cities' ])

EXPECT_THROWS(lambda: util.import_table(os.path.join(__import_data_path, 'world_x_cities.dump'), { "schema": target_schema, "table": target_table }), "Table '{0}.{1}' doesn't exist".format(target_schema, target_table))

EXPECT_NO_THROWS(lambda: util.import_table(os.path.join(__import_data_path, 'world_x_cities.dump'), { "schema": target_schema, "table": target_table, "characterSet": target_character_set }), "importing with characterSet")

session.run_sql("DROP TABLE !.!;", [ target_schema, target_table ])

session.run_sql("SET @@session.character_set_client = ?", [ old_cs_client ])
session.run_sql("SET @@session.character_set_connection = ?", [ old_cs_connection ])
session.run_sql("SET @@session.character_set_results = ?", [ old_cs_results ])
session.run_sql("SET GLOBAL SQL_MODE=?", [original_global_sqlmode])

#@<> User defined operations
session.run_sql("CREATE TABLE IF NOT EXISTS `t_numbers` ("+
    "`a` integer," +
    "`b` integer," +
    "`sum` integer," +
    "`pow` integer," +
    "`mul` integer" +
  ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4")

#@<> decodeColums option requires columns option to be set
EXPECT_THROWS(lambda:
    util.import_table(os.path.join(__import_data_path, 'numbers.tsv'), {
        "schema": target_schema, "table": 't_numbers',
        "decodeColumns": {
            "a": "@1",
            "b": "@2",
            "sum": "@1 + @2",
            "pow": "pow(@1, @2)",
            "mul": "@1 * @2"
        }
    }), "Util.import_table: Argument #2: The 'columns' option is required when 'decodeColumns' is set.");

EXPECT_THROWS(lambda:
    util.import_table(os.path.join(__import_data_path, 'numbers.tsv'), {
        "schema": target_schema, "table": 't_numbers',
        "columns": [],
        "decodeColumns": {
            "a": "@1",
            "b": "@2",
            "sum": "@1 + @2",
            "pow": "pow(@1, @2)",
            "mul": "@1 * @2"
        }
    }), "Util.import_table: Argument #2: The 'columns' option must be a non-empty list.");

#@<> TEST_STRING_OPTION
def TEST_STRING_OPTION(option):
    EXPECT_THROWS(lambda: util.import_table(os.path.join(__import_data_path, 'world_x_cities.dump'), { option: None, "schema": target_schema, "table": 'cities' }), f"TypeError: Util.import_table: Argument #2: Option '{option}' is expected to be of type String, but is Null")
    EXPECT_THROWS(lambda: util.import_table(os.path.join(__import_data_path, 'world_x_cities.dump'), { option: 5, "schema": target_schema, "table": 'cities' }), f"TypeError: Util.import_table: Argument #2: Option '{option}' is expected to be of type String, but is Integer")
    EXPECT_THROWS(lambda: util.import_table(os.path.join(__import_data_path, 'world_x_cities.dump'), { option: -5, "schema": target_schema, "table": 'cities' }), f"TypeError: Util.import_table: Argument #2: Option '{option}' is expected to be of type String, but is Integer")
    EXPECT_THROWS(lambda: util.import_table(os.path.join(__import_data_path, 'world_x_cities.dump'), { option: [], "schema": target_schema, "table": 'cities' }), f"TypeError: Util.import_table: Argument #2: Option '{option}' is expected to be of type String, but is Array")
    EXPECT_THROWS(lambda: util.import_table(os.path.join(__import_data_path, 'world_x_cities.dump'), { option: {}, "schema": target_schema, "table": 'cities' }), f"TypeError: Util.import_table: Argument #2: Option '{option}' is expected to be of type String, but is Map")
    EXPECT_THROWS(lambda: util.import_table(os.path.join(__import_data_path, 'world_x_cities.dump'), { option: False, "schema": target_schema, "table": 'cities' }), f"TypeError: Util.import_table: Argument #2: Option '{option}' is expected to be of type String, but is Bool")

#@<> WL14387-TSFR_1_1_1 - s3BucketName - string option
TEST_STRING_OPTION("s3BucketName")

#@<> WL14387-TSFR_1_2_1 - s3BucketName and osBucketName cannot be used at the same time
EXPECT_THROWS(lambda: util.import_table(os.path.join(__import_data_path, 'world_x_cities.dump'), { "s3BucketName": "one", "osBucketName": "two", "schema": target_schema, "table": 'cities' }), "ValueError: Util.import_table: Argument #2: The option 's3BucketName' cannot be used when the value of 'osBucketName' option is set")

#@<> WL14387-TSFR_1_1_3 - s3BucketName set to an empty string loads from a local directory
session.run_sql('TRUNCATE TABLE !.cities', [target_schema])
EXPECT_NO_THROWS(lambda: util.import_table(os.path.join(__import_data_path, 'world_x_cities.dump'), { "s3BucketName": "", "schema": target_schema, "table": 'cities' }), "should not fail")

#@<> s3CredentialsFile - string option
TEST_STRING_OPTION("s3CredentialsFile")

#@<> WL14387-TSFR_3_1_1_1 - s3CredentialsFile cannot be used without s3BucketName
EXPECT_THROWS(lambda: util.import_table(os.path.join(__import_data_path, 'world_x_cities.dump'), { "s3CredentialsFile": "file", "schema": target_schema, "table": 'cities' }), "ValueError: Util.import_table: Argument #2: The option 's3CredentialsFile' cannot be used when the value of 's3BucketName' option is not set")

#@<> s3BucketName and s3CredentialsFile both set to an empty string loads from a local directory
session.run_sql('TRUNCATE TABLE !.cities', [target_schema])
EXPECT_NO_THROWS(lambda: util.import_table(os.path.join(__import_data_path, 'world_x_cities.dump'), { "s3BucketName": "", "s3CredentialsFile": "", "schema": target_schema, "table": 'cities' }), "should not fail")

#@<> s3ConfigFile - string option
TEST_STRING_OPTION("s3ConfigFile")

#@<> WL14387-TSFR_4_1_1_1 - s3ConfigFile cannot be used without s3BucketName
EXPECT_THROWS(lambda: util.import_table(os.path.join(__import_data_path, 'world_x_cities.dump'), { "s3ConfigFile": "file", "schema": target_schema, "table": 'cities' }), "ValueError: Util.import_table: Argument #2: The option 's3ConfigFile' cannot be used when the value of 's3BucketName' option is not set")

#@<> s3BucketName and s3ConfigFile both set to an empty string loads from a local directory
session.run_sql('TRUNCATE TABLE !.cities', [target_schema])
EXPECT_NO_THROWS(lambda: util.import_table(os.path.join(__import_data_path, 'world_x_cities.dump'), { "s3BucketName": "", "s3ConfigFile": "", "schema": target_schema, "table": 'cities' }), "should not fail")

#@<> WL14387-TSFR_2_1_1 - s3Profile - string option
TEST_STRING_OPTION("s3Profile")

#@<> WL14387-TSFR_2_1_1_2 - s3Profile cannot be used without s3BucketName
EXPECT_THROWS(lambda: util.import_table(os.path.join(__import_data_path, 'world_x_cities.dump'), { "s3Profile": "profile", "schema": target_schema, "table": 'cities' }), "ValueError: Util.import_table: Argument #2: The option 's3Profile' cannot be used when the value of 's3BucketName' option is not set")

#@<> WL14387-TSFR_2_1_2_1 - s3BucketName and s3Profile both set to an empty string loads from a local directory
session.run_sql('TRUNCATE TABLE !.cities', [target_schema])
EXPECT_NO_THROWS(lambda: util.import_table(os.path.join(__import_data_path, 'world_x_cities.dump'), { "s3BucketName": "", "s3Profile": "", "schema": target_schema, "table": 'cities' }), "should not fail")

#@<> s3EndpointOverride - string option
TEST_STRING_OPTION("s3EndpointOverride")

#@<> WL14387-TSFR_6_1_1 - s3EndpointOverride cannot be used without s3BucketName
EXPECT_THROWS(lambda: util.import_table(os.path.join(__import_data_path, 'world_x_cities.dump'), { "s3EndpointOverride": "http://example.org", "schema": target_schema, "table": 'cities' }), "ValueError: Util.import_table: Argument #2: The option 's3EndpointOverride' cannot be used when the value of 's3BucketName' option is not set")

#@<> s3BucketName and s3EndpointOverride both set to an empty string loads from a local directory
session.run_sql('TRUNCATE TABLE !.cities', [target_schema])
EXPECT_NO_THROWS(lambda: util.import_table(os.path.join(__import_data_path, 'world_x_cities.dump'), { "s3BucketName": "", "s3EndpointOverride": "", "schema": target_schema, "table": 'cities' }), "should not fail")

#@<> s3EndpointOverride is missing a scheme
EXPECT_THROWS(lambda: util.import_table(os.path.join(__import_data_path, 'world_x_cities.dump'), { "s3BucketName": "bucket", "s3EndpointOverride": "endpoint", "schema": target_schema, "table": 'cities' }), "ValueError: Util.import_table: Argument #2: The value of the option 's3EndpointOverride' is missing a scheme, expected: http:// or https://.")

#@<> s3EndpointOverride is using wrong scheme
EXPECT_THROWS(lambda: util.import_table(os.path.join(__import_data_path, 'world_x_cities.dump'), { "s3BucketName": "bucket", "s3EndpointOverride": "FTp://endpoint", "schema": target_schema, "table": 'cities' }), "ValueError: Util.import_table: Argument #2: The value of the option 's3EndpointOverride' uses an invalid scheme 'FTp://', expected: http:// or https://.")

#@<> Teardown
session.run_sql("DROP SCHEMA IF EXISTS " + target_schema)
session.close()

#@<> Throw if session is closed
EXPECT_THROWS(lambda: util.import_table(__import_data_path + '/world_x_cities.dump', { "table": 'cities' }), "A classic protocol session is required to perform this operation.")
