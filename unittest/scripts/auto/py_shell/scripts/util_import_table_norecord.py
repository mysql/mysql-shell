#@<> INCLUDE dump_utils.inc

#@<> This is unit test file for WL12193 Parallel data import
import os
import os.path
import shutil

target_port = __mysql_port
target_xport = __port
target_schema = 'wl12193'
target_table = 'cities'
uri = "mysql://" + __mysqluripwd
xuri = "mysqlx://" + __uripwd
world_x_cities_dump = os.path.join(__import_data_path, "world_x_cities.dump")
world_x_cities_csv = os.path.join(__import_data_path, "world_x_cities.csv")

if __os_type != "windows":
    def filename_for_output(filename):
        return filename
else:
    def filename_for_output(filename):
        long_path_prefix = r"\\?" "\\"
        return long_path_prefix + filename.replace("/", "\\")

def qualified_table_name():
    return f"{target_schema}.{target_table}"

def truncate_table():
    session.run_sql('TRUNCATE TABLE ' + qualified_table_name())

def primary_key_prefix(table_name):
    if __version_num < 80019:
        return ""
    else:
        return table_name + "."

#@<> Throw if session is empty
EXPECT_THROWS(lambda: util.import_table(world_x_cities_dump, { "table": target_table }),
    "An open session is required to perform this operation.")


testutil.wait_sandbox_alive(xuri)

#@<> Setup test
shell.connect(xuri)
#/ Create collection for json import
session.drop_schema(target_schema)
schema_ = session.create_schema(target_schema)
schema_.create_collection('document_store')
session.close()

#/ Create wl12193.cities table
shell.connect(uri)
session.run_sql('USE ' + target_schema)
session.run_sql('DROP TABLE IF EXISTS ' + target_table)
session.run_sql(f"""CREATE TABLE `{target_table}` (`ID` int(11) NOT NULL AUTO_INCREMENT, `Name` char(64) NOT NULL DEFAULT '', `CountryCode` char(3) NOT NULL DEFAULT '', `District` char(64) NOT NULL DEFAULT '', `Info` json DEFAULT NULL, PRIMARY KEY (`ID`)) ENGINE=InnoDB AUTO_INCREMENT=4080 DEFAULT CHARSET=utf8mb4""")

#@<> Throw if MySQL Server config option local_infile is false
session.run_sql('SET GLOBAL local_infile = false')
EXPECT_THROWS(lambda: util.import_table(world_x_cities_dump, { "schema": target_schema, "table": target_table }),
    "local_infile disabled in server")

EXPECT_STDOUT_CONTAINS("The 'local_infile' global system variable must be set to ON in the target server, after the server is verified to be trusted.")

#@<> Set local_infile to true
session.run_sql('SET GLOBAL local_infile = true')


#@<> BUG#34582616 when global session is using X protocol, importTable should still work
shell.connect(xuri)

EXPECT_NO_THROWS(lambda: util.import_table(world_x_cities_dump,
        { "schema": target_schema, "table": target_table }),
"Should create a classic connection")

shell.connect(uri)
truncate_table()

#@<> TSF1_1 and TSF1_2
#/ TSF1_1: Validate that the load data operation is done using the current
#/ active session in MySQL Shell for both methods: util.import_table
#/ TSF1_2:
#/ Validate that the load data operation is done using the session created
#/ through the command line for method util.import_table
rc = testutil.call_mysqlsh([uri, '--schema=' + target_schema, '--', 'util', 'import-table', world_x_cities_dump, '--table=' + target_table])
EXPECT_EQ(0, rc)
EXPECT_STDOUT_CONTAINS(f"File '{filename_for_output(world_x_cities_dump)}' (209.75 KB) was imported in ")
EXPECT_STDOUT_CONTAINS(f"Total rows affected in {qualified_table_name()}: Records: 4079  Deleted: 0  Skipped: 0  Warnings: 0")

#@<OUT> Show world_x.cities table
# verify contents of the table
original_output_format = shell.options.resultFormat
shell.options.resultFormat = 'table'
session.run_sql(f'select * from {qualified_table_name()} order by ID asc')
shell.options.resultFormat = original_output_format

#@<> calculate checksum for further checks
target_table_checksum = md5_table(session, target_schema, target_table)
truncate_table()

#/@<> TSF3 - Match user specified threads number with performance_schema
#/ Not applicable

#@<> TSF8_1: Using util.import_table, validate that a file that meets the default value for field
#/ and line terminators can be imported in parallel without specifying field and line terminators.
#/ LOAD DATA with default dialect
util.import_table(world_x_cities_dump, { "schema": target_schema, "table": target_table })
EXPECT_STDOUT_CONTAINS(f"File '{filename_for_output(world_x_cities_dump)}' (209.75 KB) was imported in ")
EXPECT_STDOUT_CONTAINS(f"Total rows affected in {qualified_table_name()}: Records: 4079  Deleted: 0  Skipped: 0  Warnings: 0")
EXPECT_EQ(target_table_checksum, md5_table(session, target_schema, target_table))

#@<> Ignore on duplicate primary key
util.import_table(world_x_cities_dump, { "schema": target_schema, "table": target_table })
EXPECT_STDOUT_CONTAINS(f"File '{filename_for_output(world_x_cities_dump)}' (209.75 KB) was imported in ")
EXPECT_STDOUT_CONTAINS(f"Total rows affected in {qualified_table_name()}: Records: 4079  Deleted: 0  Skipped: 4079  Warnings: 4079")
EXPECT_EQ(target_table_checksum, md5_table(session, target_schema, target_table))

#@<> TSF6_5: Validate that arbitrary record dump files can be imported in parallel to tables using util.import_table through the command line.
#/ Ignore on duplicate primary key, import csv
# We are importing here the same data as 'world_x_cities.dump'
# but in csv format. All record should be reported as duplicates and skipped.
util.import_table(world_x_cities_csv, {
    "schema": target_schema, "table": target_table,
    "fieldsTerminatedBy": ',', "fieldsEnclosedBy": '"', "fieldsOptionallyEnclosed": True, "linesTerminatedBy": '\n'
})

prefix = primary_key_prefix(target_table)

EXPECT_STDOUT_CONTAINS(f"{qualified_table_name()}: Records: 4079  Deleted: 0  Skipped: 4079  Warnings: 4079")
EXPECT_STDOUT_CONTAINS(f"WARNING: world_x_cities.csv error 1062: Duplicate entry '1' for key '{prefix}PRIMARY'")
EXPECT_STDOUT_CONTAINS(f"WARNING: world_x_cities.csv error 1062: Duplicate entry '2' for key '{prefix}PRIMARY'")
EXPECT_STDOUT_CONTAINS(f"WARNING: world_x_cities.csv error 1062: Duplicate entry '3' for key '{prefix}PRIMARY'")
EXPECT_STDOUT_CONTAINS(f"WARNING: world_x_cities.csv error 1062: Duplicate entry '4' for key '{prefix}PRIMARY'")
EXPECT_STDOUT_CONTAINS(f"WARNING: world_x_cities.csv error 1062: Duplicate entry '5' for key '{prefix}PRIMARY'")
EXPECT_STDOUT_CONTAINS(f"File '{filename_for_output(world_x_cities_csv)}' (250.53 KB) was imported in ")
EXPECT_STDOUT_CONTAINS(f"Total rows affected in {qualified_table_name()}: Records: 4079  Deleted: 0  Skipped: 4079  Warnings: 4079")
EXPECT_EQ(target_table_checksum, md5_table(session, target_schema, target_table))

#@<> TSET_4: Using the function util.import_table, set the option 'replace' to true. Validate that new rows replace old rows if both have the same Unique Key Value.
#/ Replace on duplicate primary key, import csv
util.import_table(world_x_cities_csv, {
    "schema": target_schema, "table": target_table,
    "fieldsTerminatedBy": ',', "fieldsEnclosedBy": '"', "fieldsOptionallyEnclosed": True, "linesTerminatedBy": '\n', "replaceDuplicates": True
})

EXPECT_STDOUT_CONTAINS(f"{qualified_table_name()}: Records: 4079  Deleted: 0  Skipped: 0  Warnings: 0")
EXPECT_STDOUT_CONTAINS(f"File '{filename_for_output(world_x_cities_csv)}' (250.53 KB) was imported in ")
EXPECT_STDOUT_CONTAINS(f"Total rows affected in {qualified_table_name()}: Records: 4079  Deleted: 0  Skipped: 0  Warnings: 0")
EXPECT_EQ(target_table_checksum, md5_table(session, target_schema, target_table))


#@<> fieldsEnclosedBy must be empty or a char.
EXPECT_THROWS(lambda: util.import_table(world_x_cities_csv, {
        "schema": target_schema, "table": target_table,
        "fieldsTerminatedBy": ',', "fieldsEnclosedBy": 'more_than_one_char', "linesTerminatedBy": '\n', "replaceDuplicates": True
    }), "fieldsEnclosedBy must be empty or a char.")


#@<> "Separators cannot be the same or be a prefix of another."
EXPECT_THROWS(lambda: util.import_table(world_x_cities_csv, {
        "schema": target_schema, "table": target_table,
        "fieldsTerminatedBy": ',', "fieldsEscapedBy": '\r', "linesTerminatedBy": '\r\n'
    }), "Separators cannot be the same or be a prefix of another.")

#@<> Throw on unknown database
session.run_sql("DROP SCHEMA IF EXISTS non_existing_schema")
EXPECT_THROWS(lambda: util.import_table(world_x_cities_dump, { "schema": 'non_existing_schema', "table": target_table }),
    "Unknown database 'non_existing_schema'")


#@<> TSF6_2: Validate that JSON files can be imported in parallel to tables using util.import_table. JSON document must be single line.
# Import JSON - one document per line
primer_dataset_id = os.path.join(__import_data_path, 'primer-dataset-id.json')
util.import_table(primer_dataset_id, {
    "schema": target_schema, "table": 'document_store',
    "columns": ['doc'], "fieldsTerminatedBy": '\n', "fieldsEnclosedBy": '',
    "fieldsEscapedBy": '', "linesTerminatedBy": '\n', "bytesPerChunk": '20M', "threads": 1})

EXPECT_STDOUT_CONTAINS(f"{target_schema}.document_store: Records: 25359  Deleted: 0  Skipped: 0  Warnings: 0")
EXPECT_STDOUT_CONTAINS(f"File '{filename_for_output(primer_dataset_id)}' (11.29 MB) was imported in ")
EXPECT_STDOUT_CONTAINS(f"Total rows affected in {target_schema}.document_store: Records: 25359  Deleted: 0  Skipped: 0  Warnings: 0")

WIPE_OUTPUT()
util.import_table(primer_dataset_id, {
    "schema": target_schema, "table": 'document_store',
    "columns": ['doc'], "dialect": 'json', "bytesPerChunk": '20M'})

prefix = primary_key_prefix("document_store")

EXPECT_STDOUT_CONTAINS(f"{target_schema}.document_store: Records: 25359  Deleted: 0  Skipped: 25359  Warnings: 25359")
EXPECT_STDOUT_CONTAINS(f"WARNING: primer-dataset-id.json error 1062: Duplicate entry '000000000001' for key '{prefix}PRIMARY'")
EXPECT_STDOUT_CONTAINS(f"WARNING: primer-dataset-id.json error 1062: Duplicate entry '000000000002' for key '{prefix}PRIMARY'")
EXPECT_STDOUT_CONTAINS(f"WARNING: primer-dataset-id.json error 1062: Duplicate entry '000000000003' for key '{prefix}PRIMARY'")
EXPECT_STDOUT_CONTAINS(f"WARNING: primer-dataset-id.json error 1062: Duplicate entry '000000000004' for key '{prefix}PRIMARY'")
EXPECT_STDOUT_CONTAINS(f"WARNING: primer-dataset-id.json error 1062: Duplicate entry '000000000005' for key '{prefix}PRIMARY'")
EXPECT_STDOUT_CONTAINS(f"File '{filename_for_output(primer_dataset_id)}' (11.29 MB) was imported in ")
EXPECT_STDOUT_CONTAINS(f"Total rows affected in {target_schema}.document_store: Records: 25359  Deleted: 0  Skipped: 25359  Warnings: 25359")

#@<> Max bytes per chunk - minimum value is 2 * BUFFER_SIZE = 65KB * 2 = 128KB
util.import_table(world_x_cities_dump, {
    "schema": target_schema, "table": target_table,
    "bytesPerChunk": '1'
})

EXPECT_STDOUT_CONTAINS("world_x_cities.dump: Records: 1523  Deleted: 0  Skipped: 1523  Warnings: 1523")
EXPECT_STDOUT_CONTAINS("world_x_cities.dump: Records: 2556  Deleted: 0  Skipped: 2556  Warnings: 2556")
EXPECT_STDOUT_CONTAINS(f"File '{filename_for_output(world_x_cities_dump)}' (209.75 KB) was imported in ")
EXPECT_STDOUT_CONTAINS(f"Total rows affected in {qualified_table_name()}: Records: 4079  Deleted: 0  Skipped: 4079  Warnings: 4079")
EXPECT_EQ(target_table_checksum, md5_table(session, target_schema, target_table))

#@<> TSF5
#/ Validate that the data transfer speeds is not greater than the value configured for maxRate by the user.
# Not aplicable.


#@<> Throw if no active schema and schema is not set in options as well
session.close()
shell.connect(uri)
EXPECT_THROWS(lambda: util.import_table(world_x_cities_dump, { "table": target_table }),
    "There is no active schema on the current session, the target schema for the import operation must be provided in the options.")


#@<> Get schema from current active schema
shell.set_current_schema(target_schema)
util.import_table(world_x_cities_dump, { "table": target_table })
EXPECT_STDOUT_CONTAINS(f"{qualified_table_name()}: Records: 4079  Deleted: 0  Skipped: 4079  Warnings: 4079")
EXPECT_STDOUT_CONTAINS(f"File '{filename_for_output(world_x_cities_dump)}' (209.75 KB) was imported in ")
EXPECT_STDOUT_CONTAINS(f"Total rows affected in {qualified_table_name()}: Records: 4079  Deleted: 0  Skipped: 4079  Warnings: 4079")
EXPECT_EQ(target_table_checksum, md5_table(session, target_schema, target_table))

#@<> BUG29822312 Import of table with foreign keys
employee_boss_csv = os.path.join(__import_data_path, 'employee_boss.csv')
session.run_sql("CREATE TABLE `employee` (`id` int(11) NOT NULL AUTO_INCREMENT, `boss` int(11) DEFAULT NULL, PRIMARY KEY (`id`), KEY `boss` (`boss`), CONSTRAINT `employee_ibfk_1` FOREIGN KEY (`boss`) REFERENCES `employee` (`id`)) ENGINE=InnoDB;")
util.import_table(employee_boss_csv, {'table': 'employee', 'fieldsTerminatedBy': ','})
EXPECT_STDOUT_CONTAINS(f"{target_schema}.employee: Records: 7  Deleted: 0  Skipped: 0  Warnings: 0")
EXPECT_STDOUT_CONTAINS(f"File '{filename_for_output(employee_boss_csv)}' (28 bytes) was imported in ")
EXPECT_STDOUT_CONTAINS(f"Total rows affected in {target_schema}.employee: Records: 7  Deleted: 0  Skipped: 0  Warnings: 0")

#@<OUT> Show employee table
original_output_format = shell.options.resultFormat
shell.options.resultFormat = 'tabbed'
session.run_sql(f'select * from {target_schema}.employee order by boss asc')
shell.options.resultFormat = original_output_format

#@<> Create latin2 charset table
session.run_sql("create table cities_latin2(id integer primary key, name text) CHARACTER SET = latin2")

#@<OUT> Import to table with utf8 character set
util.import_table(os.path.join(__import_data_path, 'cities_pl_utf8.dump'), {'table':'cities_latin2', 'characterSet': 'utf8mb4'})
shell.dump_rows(session.run_sql('select hex(id), hex(name) from cities_latin2'), "tabbed")
session.run_sql("truncate table cities_latin2")

#@<OUT> Import to table with latin2 character set
util.import_table(os.path.join(__import_data_path, 'cities_pl_latin2.dump'), {'table':'cities_latin2', 'characterSet': 'latin2'})
shell.dump_rows(session.run_sql('select hex(id), hex(name) from cities_latin2'), "tabbed")

#@<> BUG#31407133 IF SQL_MODE='NO_BACKSLASH_ESCAPES' IS SET, UTIL.IMPORTTABLE FAILS TO IMPORT DATA
original_global_sqlmode = session.run_sql("SELECT @@global.sql_mode").fetch_one()[0]
session.run_sql("SET GLOBAL SQL_MODE='NO_BACKSLASH_ESCAPES'")

truncate_table()
EXPECT_NO_THROWS(lambda: util.import_table(world_x_cities_dump, { "schema": target_schema, "table": target_table }), "importing when SQL mode is set")
EXPECT_EQ(target_table_checksum, md5_table(session, target_schema, target_table))

session.run_sql("SET GLOBAL SQL_MODE=?", [original_global_sqlmode])

#@<> BUG#33360787 loading when auto-commit set to OFF
truncate_table()

original_global_autocommit = session.run_sql("SELECT @@GLOBAL.autocommit").fetch_one()[0]
session.run_sql("SET @@GLOBAL.autocommit = OFF")

EXPECT_NO_THROWS(lambda: util.import_table(world_x_cities_dump, { "schema": target_schema, "table": target_table }), "importing when auto-commit is OFF")

session.run_sql("SET @@GLOBAL.autocommit = ?", [original_global_autocommit])

EXPECT_EQ(target_table_checksum, md5_table(session, target_schema, target_table))

#@<> BUG#31412330 UTIL.IMPORTTABLE(): UNABLE TO IMPORT DATA INTO A TABLE WITH NON-ASCII NAME
original_global_sqlmode = session.run_sql("SELECT @@global.sql_mode").fetch_one()[0]
session.run_sql("SET GLOBAL SQL_MODE=''")
old_cs_client = session.run_sql("SELECT @@session.character_set_client").fetch_one()[0]
old_cs_connection = session.run_sql("SELECT @@session.character_set_connection").fetch_one()[0]
old_cs_results = session.run_sql("SELECT @@session.character_set_results").fetch_one()[0]

# non-ASCII character has to have the UTF-8 representation which consists of bytes which correspond to non-capital Unicode letters,
# because on Windows capital letters will be converted to small letters, resulting in invalid UTF-8 sequence, messing up with reported errors
target_table_latin = 'citi→s'
target_character_set = 'latin1'

session.run_sql("SET NAMES ?", [ target_character_set ])
session.run_sql("CREATE TABLE !.! LIKE !.!;", [ target_schema, target_table_latin, target_schema, target_table ])

EXPECT_THROWS(lambda: util.import_table(world_x_cities_dump, { "schema": target_schema, "table": target_table_latin }), f"Table '{target_schema}.{target_table_latin}' doesn't exist")

EXPECT_NO_THROWS(lambda: util.import_table(world_x_cities_dump, { "schema": target_schema, "table": target_table_latin, "characterSet": target_character_set }), "importing with characterSet")

session.run_sql("DROP TABLE !.!;", [ target_schema, target_table_latin ])

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
    }), "Argument #2: The 'columns' option is required when 'decodeColumns' is set.");

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
    }), "Argument #2: The 'columns' option must be a non-empty list.");

#@<> TEST_STRING_OPTION
def TEST_STRING_OPTION(option):
    EXPECT_THROWS(lambda: util.import_table(world_x_cities_dump, { option: None, "schema": target_schema, "table": target_table }), f"TypeError: Argument #2: Option '{option}' is expected to be of type String, but is Null")
    EXPECT_THROWS(lambda: util.import_table(world_x_cities_dump, { option: 5, "schema": target_schema, "table": target_table }), f"TypeError: Argument #2: Option '{option}' is expected to be of type String, but is Integer")
    EXPECT_THROWS(lambda: util.import_table(world_x_cities_dump, { option: -5, "schema": target_schema, "table": target_table }), f"TypeError: Argument #2: Option '{option}' is expected to be of type String, but is Integer")
    EXPECT_THROWS(lambda: util.import_table(world_x_cities_dump, { option: [], "schema": target_schema, "table": target_table }), f"TypeError: Argument #2: Option '{option}' is expected to be of type String, but is Array")
    EXPECT_THROWS(lambda: util.import_table(world_x_cities_dump, { option: {}, "schema": target_schema, "table": target_table }), f"TypeError: Argument #2: Option '{option}' is expected to be of type String, but is Map")
    EXPECT_THROWS(lambda: util.import_table(world_x_cities_dump, { option: False, "schema": target_schema, "table": target_table }), f"TypeError: Argument #2: Option '{option}' is expected to be of type String, but is Bool")

#@<> WL15884-TSFR_1_1 - `ociAuth` help text
help_text = """
      - ociAuth: string (default: not set) - Use the specified authentication
        method when connecting to the OCI. Allowed values: api_key (used when
        not explicitly set), instance_obo_user, instance_principal,
        resource_principal, security_token.
"""
EXPECT_TRUE(help_text in util.help("import_table"))

#@<> WL15884-TSFR_1_2 - `ociAuth` is a string option
TEST_STRING_OPTION("ociAuth")

#@<> WL15884-TSFR_2_1 - `ociAuth` set to an empty string is ignored
truncate_table()
EXPECT_NO_THROWS(lambda: util.import_table(world_x_cities_dump, { "ociAuth": "", "schema": target_schema, "table": target_table }), "should not fail")

#@<> WL15884-TSFR_3_1 - `ociAuth` cannot be used without `osBucketName`
EXPECT_THROWS(lambda: util.import_table(world_x_cities_dump, { "osBucketName": "", "ociAuth": "api_key", "schema": target_schema, "table": target_table }), "ValueError: Argument #2: The option 'ociAuth' cannot be used when the value of 'osBucketName' option is not set")
EXPECT_THROWS(lambda: util.import_table(world_x_cities_dump, { "ociAuth": "api_key", "schema": target_schema, "table": target_table }), "ValueError: Argument #2: The option 'ociAuth' cannot be used when the value of 'osBucketName' option is not set")

#@<> WL15884-TSFR_6_1_1 - `ociAuth` set to instance_principal cannot be used with `ociConfigFile` or `ociProfile`
EXPECT_THROWS(lambda: util.import_table(world_x_cities_dump, { "osBucketName": "bucket", "ociAuth": "instance_principal", "ociConfigFile": "file", "schema": target_schema, "table": target_table }), "ValueError: Argument #2: The option 'ociConfigFile' cannot be used when the 'ociAuth' option is set to: instance_principal.")
EXPECT_THROWS(lambda: util.import_table(world_x_cities_dump, { "osBucketName": "bucket", "ociAuth": "instance_principal", "ociProfile": "profile", "schema": target_schema, "table": target_table }), "ValueError: Argument #2: The option 'ociProfile' cannot be used when the 'ociAuth' option is set to: instance_principal.")

#@<> WL15884-TSFR_7_1_1 - `ociAuth` set to resource_principal cannot be used with `ociConfigFile` or `ociProfile`
EXPECT_THROWS(lambda: util.import_table(world_x_cities_dump, { "osBucketName": "bucket", "ociAuth": "resource_principal", "ociConfigFile": "file", "schema": target_schema, "table": target_table }), "ValueError: Argument #2: The option 'ociConfigFile' cannot be used when the 'ociAuth' option is set to: resource_principal.")
EXPECT_THROWS(lambda: util.import_table(world_x_cities_dump, { "osBucketName": "bucket", "ociAuth": "resource_principal", "ociProfile": "profile", "schema": target_schema, "table": target_table }), "ValueError: Argument #2: The option 'ociProfile' cannot be used when the 'ociAuth' option is set to: resource_principal.")

#@<> WL15884-TSFR_9_1 - `ociAuth` set to an invalid value
EXPECT_THROWS(lambda: util.import_table(world_x_cities_dump, { "osBucketName": "bucket", "ociAuth": "unknown", "schema": target_schema, "table": target_table }), "ValueError: Argument #2: Invalid value of 'ociAuth' option, expected one of: api_key, instance_obo_user, instance_principal, resource_principal, security_token, but got: unknown.")

#@<> WL14387-TSFR_1_1_1 - s3BucketName - string option
TEST_STRING_OPTION("s3BucketName")

#@<> WL14387-TSFR_1_2_1 - s3BucketName and osBucketName cannot be used at the same time
EXPECT_THROWS(lambda: util.import_table(world_x_cities_dump, { "s3BucketName": "one", "osBucketName": "two", "schema": target_schema, "table": target_table }), "ValueError: Argument #2: The option 's3BucketName' cannot be used when the value of 'osBucketName' option is set")

#@<> WL14387-TSFR_1_1_3 - s3BucketName set to an empty string loads from a local directory
truncate_table()
EXPECT_NO_THROWS(lambda: util.import_table(world_x_cities_dump, { "s3BucketName": "", "schema": target_schema, "table": target_table }), "should not fail")

#@<> s3CredentialsFile - string option
TEST_STRING_OPTION("s3CredentialsFile")

#@<> WL14387-TSFR_3_1_1_1 - s3CredentialsFile cannot be used without s3BucketName
EXPECT_THROWS(lambda: util.import_table(world_x_cities_dump, { "s3CredentialsFile": "file", "schema": target_schema, "table": target_table }), "ValueError: Argument #2: The option 's3CredentialsFile' cannot be used when the value of 's3BucketName' option is not set")

#@<> s3BucketName and s3CredentialsFile both set to an empty string loads from a local directory
truncate_table()
EXPECT_NO_THROWS(lambda: util.import_table(world_x_cities_dump, { "s3BucketName": "", "s3CredentialsFile": "", "schema": target_schema, "table": target_table }), "should not fail")

#@<> s3ConfigFile - string option
TEST_STRING_OPTION("s3ConfigFile")

#@<> WL14387-TSFR_4_1_1_1 - s3ConfigFile cannot be used without s3BucketName
EXPECT_THROWS(lambda: util.import_table(world_x_cities_dump, { "s3ConfigFile": "file", "schema": target_schema, "table": target_table }), "ValueError: Argument #2: The option 's3ConfigFile' cannot be used when the value of 's3BucketName' option is not set")

#@<> s3BucketName and s3ConfigFile both set to an empty string loads from a local directory
truncate_table()
EXPECT_NO_THROWS(lambda: util.import_table(world_x_cities_dump, { "s3BucketName": "", "s3ConfigFile": "", "schema": target_schema, "table": target_table }), "should not fail")

#@<> WL14387-TSFR_2_1_1 - s3Profile - string option
TEST_STRING_OPTION("s3Profile")

#@<> WL14387-TSFR_2_1_1_2 - s3Profile cannot be used without s3BucketName
EXPECT_THROWS(lambda: util.import_table(world_x_cities_dump, { "s3Profile": "profile", "schema": target_schema, "table": target_table }), "ValueError: Argument #2: The option 's3Profile' cannot be used when the value of 's3BucketName' option is not set")

#@<> WL14387-TSFR_2_1_2_1 - s3BucketName and s3Profile both set to an empty string loads from a local directory
truncate_table()
EXPECT_NO_THROWS(lambda: util.import_table(world_x_cities_dump, { "s3BucketName": "", "s3Profile": "", "schema": target_schema, "table": target_table }), "should not fail")

#@<> s3EndpointOverride - string option
TEST_STRING_OPTION("s3EndpointOverride")

#@<> WL14387-TSFR_6_1_1 - s3EndpointOverride cannot be used without s3BucketName
EXPECT_THROWS(lambda: util.import_table(world_x_cities_dump, { "s3EndpointOverride": "http://example.org", "schema": target_schema, "table": target_table }), "ValueError: Argument #2: The option 's3EndpointOverride' cannot be used when the value of 's3BucketName' option is not set")

#@<> s3BucketName and s3EndpointOverride both set to an empty string loads from a local directory
truncate_table()
EXPECT_NO_THROWS(lambda: util.import_table(world_x_cities_dump, { "s3BucketName": "", "s3EndpointOverride": "", "schema": target_schema, "table": target_table }), "should not fail")

#@<> s3EndpointOverride is missing a scheme
EXPECT_THROWS(lambda: util.import_table(world_x_cities_dump, { "s3BucketName": "bucket", "s3EndpointOverride": "endpoint", "schema": target_schema, "table": target_table }), "ValueError: Argument #2: The value of the option 's3EndpointOverride' is missing a scheme, expected: http:// or https://.")

#@<> s3EndpointOverride is using wrong scheme
EXPECT_THROWS(lambda: util.import_table(world_x_cities_dump, { "s3BucketName": "bucket", "s3EndpointOverride": "FTp://endpoint", "schema": target_schema, "table": target_table }), "ValueError: Argument #2: The value of the option 's3EndpointOverride' uses an invalid scheme 'FTp://', expected: http:// or https://.")

#@<> BUG#35895247 - importing a file with escaped wildcard characters should load it in chunks {__os_type != "windows"}
full_path = os.path.join(__tmp_dir, "will *this work?")
shutil.copyfile(world_x_cities_dump, full_path)
truncate_table()

EXPECT_NO_THROWS(lambda: util.import_table(os.path.join(__tmp_dir, "will \\*this work\\?"), { "schema": target_schema, "table": target_table, "showProgress": False }), "import should not fail")
EXPECT_STDOUT_CONTAINS(f"Importing from file '{filename_for_output(full_path)}' to table ")

os.remove(full_path)

#@<> Teardown
session.run_sql("DROP SCHEMA IF EXISTS " + target_schema)
session.close()

#@<> Throw if session is closed
EXPECT_THROWS(lambda: util.import_table(world_x_cities_dump, { "table": target_table }), "An open session is required to perform this operation.")

#@<> BUG#35018278 skipRows=X should be applied even if a compressed file or multiple files are loaded
# setup
test_schema = "bug_35018278"
test_table = "t"
test_table_qualified = quote_identifier(test_schema, test_table)
test_rows = 10
output_dir = os.path.join(__tmp_dir, test_schema)
testutil.mkdir(output_dir, True)

shell.connect(uri)
session.run_sql("DROP SCHEMA IF EXISTS !", [ test_schema ])
session.run_sql("CREATE SCHEMA !", [ test_schema ])
session.run_sql(f"CREATE TABLE {test_table_qualified} (k INT PRIMARY KEY, v TEXT)")

for i in range(test_rows):
    session.run_sql(f"INSERT INTO {test_table_qualified} VALUES ({i}, REPEAT('a', 10000))")

for compression, extension in { "none": "", "zstd": ".zst" }.items():
    util.export_table(test_table_qualified, os.path.join(output_dir, f"1.tsv{extension}"), { "fieldsEnclosedBy": "'", "linesTerminatedBy": "a", "compression": compression, "where": f"k < {test_rows / 2}", "showProgress": False })
    util.export_table(test_table_qualified, os.path.join(output_dir, f"2.tsv{extension}"), { "fieldsEnclosedBy": "'", "linesTerminatedBy": "a", "compression": compression, "where": f"k >= {test_rows / 2}", "showProgress": False })

#@<> BUG#35018278 - tests
for extension in [ "", ".zst" ]:
    for files in [ [ "1.tsv", "2.tsv" ], [ "*.tsv" ] ]:
        for skip in range(int(test_rows / 2) + 2):
            context = f"skip: {skip}"
            session.run_sql(f"TRUNCATE TABLE {test_table_qualified}")
            for f in files:
                EXPECT_NO_THROWS(lambda: util.import_table(os.path.join(output_dir, f"{f}{extension}"), { "skipRows": skip, "schema": test_schema, "table": test_table, "fieldsEnclosedBy": "'", "linesTerminatedBy": "a", "showProgress": False }), f"file: {f}{extension}, {context}")
            EXPECT_EQ(max(test_rows - 2 * skip, 0), session.run_sql(f"SELECT COUNT(*) FROM {test_table_qualified}").fetch_one()[0], f"files: {files}, extension: {extension}, {context}")

#@<> BUG#35018278 - cleanup
session.run_sql("DROP SCHEMA IF EXISTS !", [ test_schema ])
testutil.rmdir(output_dir, True)

#@<> BUG#33970577 - output issues
# setup
test_schema = "bug_33970577"
test_table = "t"
test_table_qualified = quote_identifier(test_schema, test_table)
test_rows = 400
output_dir = os.path.join(__tmp_dir, test_schema)
testutil.mkdir(output_dir, True)

shell.connect(uri)
session.run_sql("DROP SCHEMA IF EXISTS !", [ test_schema ])
session.run_sql("CREATE SCHEMA !", [ test_schema ])
session.run_sql(f"CREATE TABLE {test_table_qualified} (k INT PRIMARY KEY, v TEXT)")

for i in range(test_rows):
    session.run_sql(f"INSERT INTO {test_table_qualified} VALUES ({i}, '{random_string(1000)}')")

util.export_table(test_table_qualified, os.path.join(output_dir, "1.tsv.zst"), { "compression": "zstd", "where": f"k < {test_rows / 2}", "showProgress": False })
util.export_table(test_table_qualified, os.path.join(output_dir, "2.tsv.gz"), { "compression": "gzip", "where": f"k >= {test_rows / 2}", "showProgress": False })

#@<> BUG#33970577 - progress goes over 100% if a compressed file is imported; multiple threads are reported, even though just one file is imported
session.run_sql(f"TRUNCATE TABLE {test_table_qualified}")
rc = testutil.call_mysqlsh([uri, "--", "util", "import-table", os.path.join(output_dir, "1.tsv.zst").replace("\\", "/"), "--bytes-per-chunk", "131072", "--schema", test_schema, "--table", test_table, "--show-progress"])
EXPECT_EQ(0, rc)
# BUG#BUG35279351 - compressed files are read by multiple threads
EXPECT_STDOUT_CONTAINS("using 8 threads")
EXPECT_STDOUT_CONTAINS("100%")
EXPECT_STDOUT_CONTAINS(f"Total rows affected in {test_schema}.{test_table}: Records: {int(test_rows / 2)}  Deleted: 0  Skipped: 0  Warnings: 0")

#@<> BUG#33970577 - if files are subchunked, wrong number of files is reported, wrong subchunk ordinal number is reported
session.run_sql(f"TRUNCATE TABLE {test_table_qualified}")
EXPECT_NO_THROWS(lambda: util.import_table(os.path.join(output_dir, "*.tsv.*"), { "threads": 1, "maxBytesPerTransaction": "4096", "schema": test_schema, "table": test_table, "showProgress": False }), "import should not fail")
EXPECT_STDOUT_CONTAINS("1.tsv.zst: Records: 4  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 50 sub-chunks")
EXPECT_STDOUT_CONTAINS("2.tsv.gz: Records: 4  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 50 sub-chunks")
EXPECT_STDOUT_MATCHES(re.compile(r"2 files \(.* uncompressed, .* compressed\) were imported in .* sec at .*/s uncompressed, .*/s compressed"))
EXPECT_STDOUT_CONTAINS(f"Total rows affected in {test_schema}.{test_table}: Records: {test_rows}  Deleted: 0  Skipped: 0  Warnings: 0")

#@<> BUG#33970577 - cleanup
session.run_sql("DROP SCHEMA IF EXISTS !", [ test_schema ])
testutil.rmdir(output_dir, True)

#@<> BUG#35279351 - same field and line termination characters
# setup
test_schema = "bug_35279351"
test_table = "t"
test_table_qualified = quote_identifier(test_schema, test_table)
test_rows = 40000
output_dir = os.path.join(__tmp_dir, test_schema)
testutil.mkdir(output_dir, True)

shell.connect(uri)
session.run_sql("DROP SCHEMA IF EXISTS !", [ test_schema ])
session.run_sql("CREATE SCHEMA !", [ test_schema ])
session.run_sql(f"CREATE TABLE {test_table_qualified} (k INT PRIMARY KEY, v TEXT)")

session.run_sql(f"""INSERT INTO {test_table_qualified} VALUES {",".join([f"({i}, '{random_string(7, 13)}')" for i in range(test_rows)])}""")

checksum = md5_table(session, test_schema, test_table)

util.export_table(test_table_qualified, os.path.join(output_dir, "1.tsv"), { "fieldsTerminatedBy": ",", "linesTerminatedBy": ",", "compression": "none", "showProgress": False })
util.export_table(test_table_qualified, os.path.join(output_dir, "1.tsv.zst"), { "fieldsTerminatedBy": ",", "linesTerminatedBy": ",", "compression": "zstd", "showProgress": False })

#@<> BUG#33970577 - chunking should be disabled
session.run_sql(f"TRUNCATE TABLE {test_table_qualified}")
EXPECT_NO_THROWS(lambda: util.import_table(os.path.join(output_dir, "1.tsv"), { "threads": 4, "bytesPerChunk": "131072", "fieldsTerminatedBy": ",", "linesTerminatedBy": ",", "schema": test_schema, "table": test_table, "showProgress": False }), "import should not fail")
EXPECT_STDOUT_CONTAINS("using 1 thread")
EXPECT_STDOUT_CONTAINS(f"Total rows affected in {test_schema}.{test_table}: Records: {test_rows}  Deleted: 0  Skipped: 0  Warnings: 0")
EXPECT_EQ(checksum, md5_table(session, test_schema, test_table))

#@<> BUG#33970577 - subchunking should be disabled
session.run_sql(f"TRUNCATE TABLE {test_table_qualified}")
EXPECT_NO_THROWS(lambda: util.import_table(os.path.join(output_dir, "1.tsv.zst"), { "threads": 1, "maxBytesPerTransaction": "4096", "fieldsTerminatedBy": ",", "linesTerminatedBy": ",", "schema": test_schema, "table": test_table, "showProgress": False }), "import should not fail")
EXPECT_STDOUT_CONTAINS("loading finished in 1 sub-chunk")
EXPECT_STDOUT_CONTAINS(f"Total rows affected in {test_schema}.{test_table}: Records: {test_rows}  Deleted: 0  Skipped: 0  Warnings: 0")
EXPECT_EQ(checksum, md5_table(session, test_schema, test_table))

#@<> BUG#35541522 - loading large file into non-existing schema takes long to fail
full_path = os.path.join(output_dir, "2.tsv")
util.export_table(test_table_qualified, full_path, { "compression": "none", "showProgress": False })
EXPECT_THROWS(lambda: util.import_table(full_path, { "threads": 1, "bytesPerChunk": "131072", "schema": "invalid-schema", "table": "invalid-table", "showProgress": False }), "Unknown database 'invalid-schema'")

#@<> BUG#35279351 - cleanup
session.run_sql("DROP SCHEMA IF EXISTS !", [ test_schema ])
testutil.rmdir(output_dir, True)
