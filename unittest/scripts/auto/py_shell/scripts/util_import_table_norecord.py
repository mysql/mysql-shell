# This is unit test file for WL12193 Parallel data import
target_port = __mysql_port
target_xport = __port
target_schema = 'wl12193'
uri = "mysql://" + __mysqluripwd
xuri = "mysqlx://" + __uripwd

def wait_for_server(uri):
    max_retries = 60 * 5
    sleep_time = 1;  # second
    while max_retries > 0:
        max_retries -= 1
        try:
            if shell.connect(uri):
                return True
            os.sleep(sleep_time)
        except Exception as error:
            print(error)

#<>@ Throw if session is empty
EXPECT_THROWS(lambda: util.import_table(__import_data_path + '/world_x_cities.dump', { "table": 'cities' }),
    "A classic protocol session is required to perform this operation.")


wait_for_server(xuri)

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
# session.run_sql('DROP SCHEMA IF EXISTS ' + target_schema);
# session.run_sql('CREATE SCHEMA ' + target_schema);
session.run_sql('USE ' + target_schema)
session.run_sql('DROP TABLE IF EXISTS `cities`')
session.run_sql("""CREATE TABLE `cities` (`ID` int(11) NOT NULL AUTO_INCREMENT, `Name` char(64) NOT NULL DEFAULT '', `CountryCode` char(3) NOT NULL DEFAULT '', `District` char(64) NOT NULL DEFAULT '', `Info` json DEFAULT NULL, PRIMARY KEY (`ID`)) ENGINE=InnoDB AUTO_INCREMENT=4080 DEFAULT CHARSET=utf8mb4""")


#@<> Throw if MySQL Server config option local_infile is false
session.run_sql('SET GLOBAL local_infile = false');
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
EXPECT_STDOUT_CONTAINS("File '" + __import_data_path + "/world_x_cities.dump' (209.75 KB) was imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".cities: Records: 4079  Deleted: 0  Skipped: 0  Warnings: 0")
session.run_sql('TRUNCATE TABLE ' + target_schema + '.cities')

#/@<> TSF3 - Match user specified threads number with performance_schema
#/ Not aplicable

#@<> TSF8_1: Using util.import_table, validate that a file that meets the default value for field
#/ and line terminators can be imported in parallel without specifying field and line terminators.
#/ LOAD DATA with default dialect
util.import_table(__import_data_path + '/world_x_cities.dump', { "schema": target_schema, "table": 'cities' })
EXPECT_STDOUT_CONTAINS("File '" + __import_data_path + "/world_x_cities.dump' (209.75 KB) was imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".cities: Records: 4079  Deleted: 0  Skipped: 0  Warnings: 0")

#@<> Ignore on duplicate primary key
util.import_table(__import_data_path + '/world_x_cities.dump', { "schema": target_schema, "table": 'cities' })
EXPECT_STDOUT_CONTAINS("File '" + __import_data_path + "/world_x_cities.dump' (209.75 KB) was imported in ")
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
EXPECT_STDOUT_CONTAINS("File '" + __import_data_path + "/world_x_cities.csv' (250.53 KB) was imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".cities: Records: 4079  Deleted: 0  Skipped: 4079  Warnings: 4079")

#@<> TSET_4: Using the function util.import_table, set the option 'replace' to true. Validate that new rows replace old rows if both have the same Unique Key Value.
#/ Replace on duplicate primary key, import csv
util.import_table(__import_data_path + '/world_x_cities.csv', {
    "schema": target_schema, "table": 'cities',
    "fieldsTerminatedBy": ',', "fieldsEnclosedBy": '"', "fieldsOptionallyEnclosed": True, "linesTerminatedBy": '\n', "replaceDuplicates": True
})

EXPECT_STDOUT_CONTAINS("wl12193.cities: Records: 4079  Deleted: 0  Skipped: 0  Warnings: 0")
EXPECT_STDOUT_CONTAINS("File '" + __import_data_path + "/world_x_cities.csv' (250.53 KB) was imported in ")
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

#@<> Throw on unknown database on 5.7.24+ {VER(>=5.7.24) and VER(<8.0)}
EXPECT_THROWS(lambda: util.import_table(__import_data_path + '/world_x_cities.dump', { "schema": 'non_existing_schema', "table": 'cities' }),
    "Table 'non_existing_schema.cities' doesn't exist")

#@<> Throw on unknown database {VER(<5.7.24) or VER(>=8.0)}
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
EXPECT_STDOUT_CONTAINS("File '" + __import_data_path + "/primer-dataset-id.json' (11.29 MB) was imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".document_store: Records: 25359  Deleted: 0  Skipped: 0  Warnings: 0")

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
EXPECT_STDOUT_CONTAINS("File '" + __import_data_path + "/primer-dataset-id.json' (11.29 MB) was imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".document_store: Records: 25359  Deleted: 0  Skipped: 25359  Warnings: 25359")


#/@<> Max bytes per chunk - minimum value is 2 * BUFFER_SIZE = 65KB * 2 = 128KB
util.import_table(__import_data_path + '/world_x_cities.dump', {
    "schema": target_schema, "table": 'cities',
    "bytesPerChunk": '1'
})

EXPECT_STDOUT_CONTAINS("world_x_cities.dump: Records: 1523  Deleted: 0  Skipped: 1523  Warnings: 1523")
EXPECT_STDOUT_CONTAINS("world_x_cities.dump: Records: 2556  Deleted: 0  Skipped: 2556  Warnings: 2556")
EXPECT_STDOUT_CONTAINS("File '" + __import_data_path + "/world_x_cities.dump' (209.75 KB) was imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".document_store: Records: 25359  Deleted: 0  Skipped: 25359  Warnings: 25359")


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
EXPECT_STDOUT_CONTAINS("File '" + __import_data_path + "/world_x_cities.dump' (209.75 KB) was imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".cities: Records: 4079  Deleted: 0  Skipped: 4079  Warnings: 4079")

#@<> BUG29822312 Import of table with foreign keys
session.run_sql("CREATE TABLE `employee` (`id` int(11) NOT NULL AUTO_INCREMENT, `boss` int(11) DEFAULT NULL, PRIMARY KEY (`id`), KEY `boss` (`boss`), CONSTRAINT `employee_ibfk_1` FOREIGN KEY (`boss`) REFERENCES `employee` (`id`)) ENGINE=InnoDB;")
util.import_table(__import_data_path + '/employee_boss.csv', {'table': 'employee', 'fieldsTerminatedBy': ','})
EXPECT_STDOUT_CONTAINS("wl12193.employee: Records: 7  Deleted: 0  Skipped: 0  Warnings: 0")
EXPECT_STDOUT_CONTAINS("File '" + __import_data_path + "/employee_boss.csv' (28 bytes) was imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".employee: Records: 7  Deleted: 0  Skipped: 0  Warnings: 0")

#@<OUT> Show employee table
original_output_format = shell.options.resultFormat
shell.options.resultFormat = 'tabbed'
session.run_sql('select * from ' + target_schema + '.employee order by boss asc');
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

#@<> Teardown
session.run_sql("DROP SCHEMA IF EXISTS " + target_schema)
session.close()

#<>@ Throw if session is closed
EXPECT_THROWS(lambda: util.import_table(__import_data_path + '/world_x_cities.dump', { "table": 'cities' }), "A classic protocol session is required to perform this operation.");
