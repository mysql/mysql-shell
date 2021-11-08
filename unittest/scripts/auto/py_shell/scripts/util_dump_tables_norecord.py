#@<> INCLUDE dump_utils.inc

#@<> entry point

# imports
import json
import os
import os.path
import random
import re
import shutil
import stat
import string
import time

# constants
world_x_schema = "world_x_cities"
world_x_table = "cities"

# schema name can consist of 51 reserved characters max, server supports up to
# 64 chars but when it creates the directory for schema, it encodes non-letters
# ASCII using five bytes, meaning the directory name is going to be 255
# characters long (max on most # platforms)
# schema name consists of 49 reserved characters + UTF-8 character
test_schema = "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ó"
test_table_primary = "first"
test_table_unique = "second"
# mysql_data_home + schema + table name + path separators cannot exceed 512
# characters
# table name consists of 48 reserved characters + UTF-8 character
test_table_non_unique = "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ą"
test_table_no_index = "fóurth"
test_table_trigger = "sample_trigger"
test_schema_procedure = "sample_procedure"
test_schema_function = "sample_function"
test_schema_event = "sample_event"
test_view = "sample_view"

verification_schema = "wl13804_ver"

types_schema = "xtest"

incompatible_schema = "mysqlaas"
incompatible_table_wrong_engine = "wrong_engine"
incompatible_table_encryption = "has_encryption"
incompatible_table_data_directory = "has_data_dir"
incompatible_table_index_directory = "has_index_dir"
incompatible_tablespace = "sample_tablespace"
incompatible_table_tablespace = "use_tablespace"
incompatible_view = "has_definer"
incompatible_schema_tables = [incompatible_table_wrong_engine, incompatible_table_encryption, incompatible_table_data_directory, incompatible_table_index_directory, incompatible_table_tablespace]
incompatible_schema_views = [incompatible_view]

incompatible_table_directory = os.path.join(__tmp_dir, "incompatible")
table_data_directory = os.path.join(incompatible_table_directory, "data")
table_index_directory = os.path.join(incompatible_table_directory, "index")

shutil.rmtree(incompatible_table_directory, True)
os.makedirs(incompatible_table_directory)
os.mkdir(table_data_directory)
os.mkdir(table_index_directory)

if __os_type == "windows":
    # on windows server would have to be installed in the root folder to
    # handle long schema/table names
    if __version_num >= 80000:
        test_schema = "@ó"
        test_table_non_unique = "@ą"
    else:
        # when using latin1 encoding, UTF-8 characters sent by shell are converted by server
        # to latin1 representation, then upper case characters are converted to lower case,
        # which leads to invalid UTF-8 sequences when names are transfered back to shell
        # use ASCII in this case
        test_schema = "@o"
        test_table_non_unique = "@a"
        test_table_no_index = "fourth"

all_schemas = [world_x_schema, types_schema, test_schema, verification_schema, incompatible_schema]
test_schema_tables = [test_table_primary, test_table_unique, test_table_non_unique, test_table_no_index]
test_schema_views = [test_view]

uri = __sandbox_uri1
xuri = __sandbox_uri1.replace("mysql://", "mysqlx://") + "0"

test_output_relative = "dump_output"
test_output_absolute = os.path.abspath(test_output_relative)

# helpers
if __os_type != "windows":
    def filename_for_file(filename):
        return filename
else:
    def filename_for_file(filename):
        return filename.replace("\\", "/")

if __os_type != "windows":
    def absolute_path_for_output(path):
        return path
else:
    def absolute_path_for_output(path):
        long_path_prefix = r"\\?" "\\"
        return long_path_prefix + path

def setup_session(u = uri):
    shell.connect(u)
    session.run_sql("SET NAMES 'utf8mb4';")
    session.run_sql("SET GLOBAL local_infile = true;")

def drop_all_schemas(exclude=[]):
    for schema in session.run_sql("SELECT SCHEMA_NAME FROM information_schema.schemata;").fetch_all():
        if schema[0] not in ["information_schema", "mysql", "performance_schema", "sys"] + exclude:
            session.run_sql("DROP SCHEMA IF EXISTS !;", [ schema[0] ])

def create_all_schemas():
    for schema in all_schemas:
        session.run_sql("CREATE SCHEMA !;", [ schema ])

def create_user():
    session.run_sql(f"DROP USER IF EXISTS {test_user_account};")
    session.run_sql(f"CREATE USER IF NOT EXISTS {test_user_account} IDENTIFIED BY ?;", [test_user_pwd])

def recreate_verification_schema():
    session.run_sql("DROP SCHEMA IF EXISTS !;", [ verification_schema ])
    session.run_sql("CREATE SCHEMA !;", [ verification_schema ])

def EXPECT_SUCCESS(schema, tables, output_url, options = {}, views = [], expected_length = None):
    WIPE_STDOUT()
    shutil.rmtree(test_output_absolute, True)
    EXPECT_FALSE(os.path.isdir(test_output_absolute))
    util.dump_tables(schema, tables + views, output_url, options)
    if not "dryRun" in options:
        EXPECT_TRUE(os.path.isdir(test_output_absolute))
        EXPECT_STDOUT_CONTAINS("Tables dumped: {0}".format(len(tables) if expected_length is None else expected_length))

def EXPECT_FAIL(error, msg, schema, tables, output_url, options = {}, expect_dir_created = False):
    shutil.rmtree(test_output_absolute, True)
    full_msg = "{0}: Util.dump_tables: {1}".format(error, msg.pattern if is_re_instance(msg) else msg)
    if is_re_instance(msg):
        full_msg = re.compile("^" + full_msg)
    EXPECT_THROWS(lambda: util.dump_tables(schema, tables, output_url, options), full_msg)
    EXPECT_EQ(expect_dir_created, os.path.isdir(test_output_absolute))

def TEST_BOOL_OPTION(option):
    EXPECT_FAIL("TypeError", "Argument #4: Option '{0}' is expected to be of type Bool, but is Null".format(option), types_schema, types_schema_tables, test_output_relative, { option: None })
    EXPECT_FAIL("TypeError", "Argument #4: Option '{0}' Bool expected, but value is String".format(option), types_schema, types_schema_tables, test_output_relative, { option: "dummy" })
    EXPECT_FAIL("TypeError", "Argument #4: Option '{0}' is expected to be of type Bool, but is Array".format(option), types_schema, types_schema_tables, test_output_relative, { option: [] })
    EXPECT_FAIL("TypeError", "Argument #4: Option '{0}' is expected to be of type Bool, but is Map".format(option), types_schema, types_schema_tables, test_output_relative, { option: {} })

def TEST_STRING_OPTION(option):
    EXPECT_FAIL("TypeError", "Argument #4: Option '{0}' is expected to be of type String, but is Null".format(option), types_schema, types_schema_tables, test_output_relative, { option: None })
    EXPECT_FAIL("TypeError", "Argument #4: Option '{0}' is expected to be of type String, but is Integer".format(option), types_schema, types_schema_tables, test_output_relative, { option: 5 })
    EXPECT_FAIL("TypeError", "Argument #4: Option '{0}' is expected to be of type String, but is Integer".format(option), types_schema, types_schema_tables, test_output_relative, { option: -5 })
    EXPECT_FAIL("TypeError", "Argument #4: Option '{0}' is expected to be of type String, but is Array".format(option), types_schema, types_schema_tables, test_output_relative, { option: [] })
    EXPECT_FAIL("TypeError", "Argument #4: Option '{0}' is expected to be of type String, but is Map".format(option), types_schema, types_schema_tables, test_output_relative, { option: {} })
    EXPECT_FAIL("TypeError", "Argument #4: Option '{0}' is expected to be of type String, but is Bool".format(option), types_schema, types_schema_tables, test_output_relative, { option: False })

def TEST_UINT_OPTION(option):
    EXPECT_FAIL("TypeError", "Argument #4: Option '{0}' is expected to be of type UInteger, but is Null".format(option), types_schema, types_schema_tables, test_output_relative, { option: None })
    EXPECT_FAIL("TypeError", "Argument #4: Option '{0}' UInteger expected, but Integer value is out of range".format(option), types_schema, types_schema_tables, test_output_relative, { option: -5 })
    EXPECT_FAIL("TypeError", "Argument #4: Option '{0}' UInteger expected, but value is String".format(option), types_schema, types_schema_tables, test_output_relative, { option: "dummy" })
    EXPECT_FAIL("TypeError", "Argument #4: Option '{0}' is expected to be of type UInteger, but is Array".format(option), types_schema, types_schema_tables, test_output_relative, { option: [] })
    EXPECT_FAIL("TypeError", "Argument #4: Option '{0}' is expected to be of type UInteger, but is Map".format(option), types_schema, types_schema_tables, test_output_relative, { option: {} })

def TEST_ARRAY_OF_STRINGS_OPTION(option):
    EXPECT_FAIL("TypeError", "Argument #4: Option '{0}' is expected to be of type Array, but is Integer".format(option), types_schema, types_schema_tables, test_output_relative, { option: 5 })
    EXPECT_FAIL("TypeError", "Argument #4: Option '{0}' is expected to be of type Array, but is Integer".format(option), types_schema, types_schema_tables, test_output_relative, { option: -5 })
    EXPECT_FAIL("TypeError", "Argument #4: Option '{0}' is expected to be of type Array, but is String".format(option), types_schema, types_schema_tables, test_output_relative, { option: "dummy" })
    EXPECT_FAIL("TypeError", "Argument #4: Option '{0}' is expected to be of type Array, but is Map".format(option), types_schema, types_schema_tables, test_output_relative, { option: {} })
    EXPECT_FAIL("TypeError", "Argument #4: Option '{0}' is expected to be of type Array, but is Bool".format(option), types_schema, types_schema_tables, test_output_relative, { option: False })
    EXPECT_FAIL("TypeError", "Argument #4: Option '{0}' String expected, but value is Null".format(option), types_schema, types_schema_tables, test_output_relative, { option: [ None ] })
    EXPECT_FAIL("TypeError", "Argument #4: Option '{0}' String expected, but value is Integer".format(option), types_schema, types_schema_tables, test_output_relative, { option: [ 5 ] })
    EXPECT_FAIL("TypeError", "Argument #4: Option '{0}' String expected, but value is Integer".format(option), types_schema, types_schema_tables, test_output_relative, { option: [ -5 ] })
    EXPECT_FAIL("TypeError", "Argument #4: Option '{0}' String expected, but value is Map".format(option), types_schema, types_schema_tables, test_output_relative, { option: [ {} ] })
    EXPECT_FAIL("TypeError", "Argument #4: Option '{0}' String expected, but value is Bool".format(option), types_schema, types_schema_tables, test_output_relative, { option: [ False ] })

def get_load_columns(options):
    decode = options["decodeColumns"] if "decodeColumns" in options else {}
    columns = []
    variables = []
    for c in options["columns"]:
        if c in decode:
            var = "@var{0}".format(len(variables))
            variables.append("{0} = {1}({2})".format(c, decode[c], var))
            columns.append(var)
        else:
            columns.append(c)
    statement = "(" + ",".join(columns) + ")"
    if variables:
        statement += " SET " + ",".join(variables)
    return statement

def get_all_columns(schema, table):
    columns = []
    for column in session.run_sql("SELECT COLUMN_NAME FROM information_schema.columns WHERE TABLE_SCHEMA = ? AND TABLE_NAME = ? ORDER BY ORDINAL_POSITION;", [schema, table]).fetch_all():
        columns.append(column[0])
    return columns

def get_all_tables(schema):
    tables = []
    for table in session.run_sql("SELECT TABLE_NAME FROM information_schema.tables WHERE TABLE_SCHEMA = ?;", [schema]).fetch_all():
        tables.append(table[0])
    return tables

def compute_crc(schema, table, columns):
    session.run_sql("SET @crc = '';")
    session.run_sql("SELECT @crc := MD5(CONCAT_WS('#',@crc,{0})) FROM !.! ORDER BY {0};".format(("!," * len(columns))[:-1]), columns + [schema, table] + columns)
    return session.run_sql("SELECT @crc;").fetch_one()[0]

def compute_checksum(schema, table):
    return session.run_sql("CHECKSUM TABLE !.!", [ schema, table ]).fetch_one()[1]

def TEST_LOAD(schema, table):
    print("---> testing: `{0}`.`{1}`".format(schema, table))
    # only uncompressed files are supported
    basename = encode_table_basename(schema, table)
    # read global options
    with open(os.path.join(test_output_absolute, "@.json"), encoding="utf-8") as json_file:
        tz_utc = json.load(json_file)["tzUtc"]
    # read options from metadata file
    with open(os.path.join(test_output_absolute, basename + ".json"), encoding="utf-8") as json_file:
        options = json.load(json_file)["options"]
    columns_statement = get_load_columns(options)
    local_infile_statement = """'
    INTO TABLE `{0}`.`{1}`
    CHARACTER SET '{2}'
    FIELDS TERMINATED BY {3} {4}ENCLOSED BY {5} ESCAPED BY {6}
    LINES TERMINATED BY {7}
    {8};\n""".format(verification_schema, table, options["defaultCharacterSet"], repr(options["fieldsTerminatedBy"]), "OPTIONALLY " if options["fieldsOptionallyEnclosed"] else "", repr(options["fieldsEnclosedBy"]), repr(options["fieldsEscapedBy"]), repr(options["linesTerminatedBy"]), columns_statement)
    # get files with data
    data_files = []
    for f in os.listdir(test_output_absolute):
        if f == basename + ".tsv" or (f.startswith(basename + "@") and f.endswith(".tsv")):
            data_files.append(f)
    # on Windows file path in LOAD DATA is passed directly to the function which opens the file
    # (without any encoding/decoding), rename them to avoid problems
    if __os_type == "windows":
        backup_data_files = {}
        for idx, old_name in enumerate(data_files):
            new_name = old_name.encode("ascii", "replace").decode().replace("?", "_")
            os.rename(os.path.join(test_output_absolute, old_name), os.path.join(test_output_absolute, new_name))
            backup_data_files[new_name] = old_name
            data_files[idx] = new_name
    # write SQL file which will load everything
    sql_file = os.path.join(test_output_absolute, "data_" + basename + ".sql")
    with open(sql_file, "w", encoding="utf-8") as f:
        f.write("SET NAMES '{0}';\n".format(options["defaultCharacterSet"]))
        f.write("SET SQL_MODE='';\n")
        if tz_utc:
            f.write("SET TIME_ZONE = '+00:00';\n")
        f.write("source {0} ;\n".format(filename_for_file(os.path.join(test_output_absolute, basename + ".sql"))))
        for data in data_files:
            f.write("LOAD DATA LOCAL INFILE '")
            f.write(filename_for_file(os.path.join(test_output_absolute, data)))
            f.write(local_infile_statement)
    # load data
    recreate_verification_schema()
    EXPECT_EQ(0, testutil.call_mysqlsh([uri + "/" + verification_schema + "?local-infile=true", "--sql", "--file", sql_file]))
    # rename back the data files
    if __os_type == "windows":
        for new_name, old_name in backup_data_files.items():
            os.rename(os.path.join(test_output_absolute, new_name), os.path.join(test_output_absolute, old_name))
    #compute CRC
    all_columns = get_all_columns(schema, table)
    EXPECT_EQ(compute_crc(schema, table, all_columns), compute_crc(verification_schema, table, all_columns))

#@<> WL13804-FR7.1 - If there is no open global Shell session, an exception must be thrown. (no global session)
# WL13804-TSFR_7_1_1
EXPECT_FAIL("RuntimeError", "An open session is required to perform this operation.", 'mysql', ['user'], test_output_relative)

#@<> deploy sandbox
testutil.deploy_raw_sandbox(__mysql_sandbox_port1, "root", {
    "loose_innodb_directories": filename_for_file(table_data_directory),
    "early-plugin-load": "keyring_file." + ("dll" if __os_type == "windows" else "so"),
    "keyring_file_data": filename_for_file(os.path.join(incompatible_table_directory, "keyring")),
    "server_id": 1234567890,
    "log_bin": 1,
    "enforce_gtid_consistency": "ON",
    "gtid_mode": "ON",
})

#@<> wait for server
testutil.wait_sandbox_alive(uri)
shell.connect(uri)

#@<> WL13804-FR7.1 - If there is no open global Shell session, an exception must be thrown. (no open session)
# WL13804-TSFR_7_1_1
session.close()
EXPECT_FAIL("RuntimeError", "An open session is required to perform this operation.", 'mysql', ['user'], test_output_relative)

#@<> Setup
setup_session()

drop_all_schemas()
create_all_schemas()

create_user()

session.run_sql("CREATE TABLE !.! (`ID` int(11) NOT NULL AUTO_INCREMENT, `Name` char(64) NOT NULL DEFAULT '', `CountryCode` char(3) NOT NULL DEFAULT '', `District` char(64) NOT NULL DEFAULT '', `Info` json DEFAULT NULL, PRIMARY KEY (`ID`)) ENGINE=InnoDB AUTO_INCREMENT=4080 DEFAULT CHARSET=utf8mb4;", [ world_x_schema, world_x_table ])
util.import_table(os.path.join(__import_data_path, "world_x_cities.dump"), { "schema": world_x_schema, "table": world_x_table, "characterSet": "utf8mb4", "showProgress": False })

rc = testutil.call_mysqlsh([uri, "--sql", "--file", os.path.join(__data_path, "sql", "fieldtypes_all.sql")])
EXPECT_EQ(0, rc)

# table with primary key, unique key, generated columns
session.run_sql("CREATE TABLE !.! (`id` MEDIUMINT NOT NULL AUTO_INCREMENT PRIMARY KEY, `data` INT NOT NULL UNIQUE, `vdata` INT GENERATED ALWAYS AS (data + 1) VIRTUAL, `sdata` INT GENERATED ALWAYS AS (data + 2) STORED) ENGINE=InnoDB;", [ test_schema, test_table_primary ])
# table with unique key
session.run_sql("CREATE TABLE !.! (`id` MEDIUMINT NOT NULL UNIQUE, `data` INT) ENGINE=InnoDB;", [ test_schema, test_table_unique ])
# table with non-unique key
session.run_sql("CREATE TABLE !.! (`id` MEDIUMINT, `data` INT, KEY (`id`)) ENGINE=InnoDB;", [ test_schema, test_table_non_unique ])
# table without key
session.run_sql("CREATE TABLE !.! (`id` MEDIUMINT, `data` INT, `tdata` INT) ENGINE=InnoDB;", [ test_schema, test_table_no_index ])

session.run_sql("CREATE VIEW !.! AS SELECT `tdata` FROM !.!;", [ test_schema, test_view, test_schema, test_table_no_index ])

session.run_sql("CREATE TRIGGER !.! BEFORE UPDATE ON ! FOR EACH ROW BEGIN SET NEW.tdata = NEW.data + 3; END;", [ test_schema, test_table_trigger, test_table_no_index ])

session.run_sql("""
CREATE PROCEDURE !.!()
BEGIN
  DECLARE i INT DEFAULT 1;

  WHILE i < 10000 DO
    INSERT INTO !.! (`data`) VALUES (i);
    SET i = i + 1;
  END WHILE;
END""", [ test_schema, test_schema_procedure, test_schema, test_table_primary ])

session.run_sql("CREATE FUNCTION !.!(s CHAR(20)) RETURNS CHAR(50) DETERMINISTIC RETURN CONCAT('Hello, ',s,'!');", [ test_schema, test_schema_function ])

session.run_sql("CREATE EVENT !.! ON SCHEDULE EVERY 1 HOUR STARTS CURRENT_TIMESTAMP + INTERVAL 1 WEEK DO DELETE FROM !.!;", [ test_schema, test_schema_event, test_schema, test_table_no_index ])

session.run_sql("""INSERT INTO !.! (`data`)
SELECT (tth * 10000 + th * 1000 + h * 100 + t * 10 + u + 1) x FROM
    (SELECT 0 tth UNION SELECT 1 UNION SELECT 2 UNION SELECT 3 UNION SELECT 4 UNION SELECT 5 UNION SELECT 6 UNION SELECT 7 UNION SELECT 8 UNION SELECT 9) E,
    (SELECT 0 th UNION SELECT 1 UNION SELECT 2 UNION SELECT 3 UNION SELECT 4 UNION SELECT 5 UNION SELECT 6 UNION SELECT 7 UNION SELECT 8 UNION SELECT 9) D,
    (SELECT 0 h UNION SELECT 1 UNION SELECT 2 UNION SELECT 3 UNION SELECT 4 UNION SELECT 5 UNION SELECT 6 UNION SELECT 7 UNION SELECT 8 UNION SELECT 9) C,
    (SELECT 0 t UNION SELECT 1 UNION SELECT 2 UNION SELECT 3 UNION SELECT 4 UNION SELECT 5 UNION SELECT 6 UNION SELECT 7 UNION SELECT 8 UNION SELECT 9) B,
    (SELECT 0 u UNION SELECT 1 UNION SELECT 2 UNION SELECT 3 UNION SELECT 4 UNION SELECT 5 UNION SELECT 6 UNION SELECT 7 UNION SELECT 8 UNION SELECT 9) A
ORDER BY x;""", [ test_schema, test_table_primary ])

session.run_sql("INSERT INTO !.! (`id`, `data`) SELECT `id`, `data` FROM !.!;", [ test_schema, test_table_unique, test_schema, test_table_primary ])
session.run_sql("INSERT INTO !.! (`id`, `data`) SELECT `id`, `data` FROM !.!;", [ test_schema, test_table_non_unique, test_schema, test_table_primary ])
# duplicate row, NULL row
session.run_sql("INSERT INTO !.! (`id`, `data`) VALUES (1, 1), (NULL, NULL);", [ test_schema, test_table_non_unique ])
session.run_sql("INSERT INTO !.! (`id`, `data`) SELECT `id`, `data` FROM !.!;", [ test_schema, test_table_no_index, test_schema, test_table_primary ])
# duplicate row, NULL row
session.run_sql("INSERT INTO !.! (`id`, `data`) VALUES (1, 1), (NULL, NULL);", [ test_schema, test_table_no_index ])

for table in [test_table_primary, test_table_unique, test_table_non_unique, test_table_no_index]:
    session.run_sql("ANALYZE TABLE !.!;", [ test_schema, table ])

#@<> schema with MySQLaaS incompatibilities
session.run_sql("ALTER DATABASE ! CHARACTER SET latin1;", [ incompatible_schema ])
session.run_sql("CREATE TABLE !.! (`id` MEDIUMINT, `data` INT) ENGINE=MyISAM DEFAULT CHARSET=latin1;", [ incompatible_schema, incompatible_table_wrong_engine ])
session.run_sql("CREATE TABLE !.! (`id` MEDIUMINT, `data` INT) ENGINE=InnoDB ENCRYPTION = 'Y' DEFAULT CHARSET=latin1;", [ incompatible_schema, incompatible_table_encryption ])
session.run_sql("CREATE TABLE !.! (`id` MEDIUMINT, `data` INT) ENGINE=InnoDB DATA DIRECTORY = '{0}' DEFAULT CHARSET=latin1;".format(filename_for_file(table_data_directory)), [ incompatible_schema, incompatible_table_data_directory ])
session.run_sql("CREATE TABLE !.! (`id` MEDIUMINT, `data` INT) ENGINE=MyISAM INDEX DIRECTORY = '{0}' DEFAULT CHARSET=latin1;".format(filename_for_file(table_index_directory)), [ incompatible_schema, incompatible_table_index_directory ])
session.run_sql("CREATE TABLESPACE ! ADD DATAFILE 't_s_1.ibd' ENGINE=INNODB;", [ incompatible_tablespace ])
session.run_sql("CREATE TABLE !.! (`id` MEDIUMINT, `data` INT) TABLESPACE ! DEFAULT CHARSET=latin1;", [ incompatible_schema, incompatible_table_tablespace, incompatible_tablespace ])
session.run_sql("CREATE VIEW !.! AS SELECT `data` FROM !.!;", [ incompatible_schema, incompatible_view, incompatible_schema, incompatible_table_wrong_engine ])

#@<> count types tables
types_schema_tables = []
types_schema_views = []

for table in session.run_sql("SELECT TABLE_NAME, TABLE_TYPE FROM information_schema.tables WHERE TABLE_SCHEMA = ?", [types_schema]).fetch_all():
    if table[1] == "BASE TABLE":
        types_schema_tables.append(table[0])
    else:
        types_schema_views.append(table[0])

#@<> Analyze the table {VER(>=8.0.0)}
session.run_sql("ANALYZE TABLE !.! UPDATE HISTOGRAM ON `id`;", [ test_schema, test_table_no_index ])

#@<> WL13804-FR8 - The `schema` parameter of the `util.dumpTables()` function must be a string value which specifies the schema to be dumped.
EXPECT_FAIL("TypeError", "Argument #1 is expected to be a string", None, [test_table_non_unique], test_output_relative)
EXPECT_FAIL("TypeError", "Argument #1 is expected to be a string", 1, [test_table_non_unique], test_output_relative)
EXPECT_FAIL("TypeError", "Argument #1 is expected to be a string", [], [test_table_non_unique], test_output_relative)
EXPECT_FAIL("TypeError", "Argument #1 is expected to be a string", {}, [test_table_non_unique], test_output_relative)
EXPECT_FAIL("TypeError", "Argument #1 is expected to be a string", True, [test_table_non_unique], test_output_relative)

#@<> WL13804-FR9 - The `tables` parameter of the `util.dumpTables()` function must be an array of string values which specifies the tables or views to be dumped.
# WL13804-TSFR_9_1_1
EXPECT_FAIL("TypeError", "Argument #2 is expected to be an array of strings", test_schema, None, test_output_relative)
EXPECT_FAIL("TypeError", "Argument #2 is expected to be an array", test_schema, 1, test_output_relative)
EXPECT_FAIL("TypeError", "Argument #2 is expected to be an array", test_schema, True, test_output_relative)
EXPECT_FAIL("TypeError", "Argument #2 is expected to be an array", test_schema, test_table_non_unique, test_output_relative)
EXPECT_FAIL("TypeError", "Argument #2 is expected to be an array", test_schema, {}, test_output_relative)
EXPECT_FAIL("TypeError", "Argument #2 is expected to be an array of strings", test_schema, [1], test_output_relative)
EXPECT_FAIL("TypeError", "Argument #2 is expected to be an array of strings", test_schema, [test_table_non_unique, 1], test_output_relative)
EXPECT_FAIL("TypeError", "Argument #2 is expected to be an array of strings", test_schema, [test_table_non_unique, True], test_output_relative)
EXPECT_FAIL("TypeError", "Argument #2 is expected to be an array of strings", test_schema, [test_table_non_unique, False], test_output_relative)
EXPECT_FAIL("TypeError", "Argument #2 is expected to be an array of strings", test_schema, [test_view, 1], test_output_relative)
EXPECT_FAIL("TypeError", "Argument #2 is expected to be an array of strings", test_schema, [test_view, True], test_output_relative)
EXPECT_FAIL("TypeError", "Argument #2 is expected to be an array of strings", test_schema, [test_view, False], test_output_relative)

#@<> WL13804-FR10 - The `outputUrl` parameter of the `util.dumpTables()` function must be a string value which specifies the output directory, where the dump data is going to be stored.
# WL13804-TSFR_10_1_1
EXPECT_FAIL("TypeError", "Argument #3 is expected to be a string", test_schema, [test_table_non_unique], None)
EXPECT_FAIL("TypeError", "Argument #3 is expected to be a string", test_schema, [test_table_non_unique], 1)
EXPECT_FAIL("TypeError", "Argument #3 is expected to be a string", test_schema, [test_table_non_unique], [])
EXPECT_FAIL("TypeError", "Argument #3 is expected to be a string", test_schema, [test_table_non_unique], {})
EXPECT_FAIL("TypeError", "Argument #3 is expected to be a string", test_schema, [test_table_non_unique], True)

#@<> WL13804-FR11 - The `options` optional parameter of the `util.dumpTables()` function must be a dictionary which contains options for the dump operation.
# WL13804-TSFR_11_1_1
EXPECT_FAIL("TypeError", "Argument #4 is expected to be a map", test_schema, [test_table_non_unique], test_output_relative, 1)
EXPECT_FAIL("TypeError", "Argument #4 is expected to be a map", test_schema, [test_table_non_unique], test_output_relative, "string")
EXPECT_FAIL("TypeError", "Argument #4 is expected to be a map", test_schema, [test_table_non_unique], test_output_relative, [])

#@<> WL13804-TSFR_6_2 - Call dumpTables(): giving less parameters than allowed, giving more parameters than allowed
EXPECT_THROWS(lambda: util.dump_tables(), "ValueError: Util.dump_tables: Invalid number of arguments, expected 3 to 4 but got 0")
EXPECT_THROWS(lambda: util.dump_tables(test_schema, [test_table_non_unique], test_output_relative, {}, None), "ValueError: Util.dump_tables: Invalid number of arguments, expected 3 to 4 but got 5")

#@<> WL13804-FR8.1 - If schema specified by the `schema` parameter does not exist, an exception must be thrown.
# WL13804-TSFR_8_1
EXPECT_FAIL("ValueError", "The requested schema '1234' was not found in the database.", "1234", ["bogus"], test_output_relative)
EXPECT_FAIL("ValueError", "The requested schema 'dummy' was not found in the database.", "dummy", ["bogus"], test_output_relative)

#@<> WL13804-FR9.1 - If the `tables` parameter is an empty array, an exception must be thrown.
# WL13804-TSFR_9_1_1
EXPECT_FAIL("ValueError", "The 'tables' parameter cannot be an empty list.", test_schema, [], test_output_relative)

#@<> WL13804-FR9.2 - If any of the tables specified by the `tables` parameter does not exist, an exception must be thrown.
# WL13804-TSFR_9_1_1
EXPECT_FAIL("ValueError", "Following tables were not found in the schema '{0}': 'dummy'".format(test_schema), test_schema, ["dummy"], test_output_relative)
EXPECT_FAIL("ValueError", "Following tables were not found in the schema '{0}': 'dummy'".format(test_schema), test_schema, [test_table_non_unique, "dummy"], test_output_relative)
EXPECT_FAIL("ValueError", "Following tables were not found in the schema '{0}': 'dummy'".format(test_schema), test_schema, ["dummy", test_table_non_unique], test_output_relative)
EXPECT_FAIL("ValueError", "Following tables were not found in the schema '{0}': 'bogus', 'dummy'".format(test_schema), test_schema, ["bogus", test_table_non_unique, "dummy"], test_output_relative)
EXPECT_FAIL("ValueError", "Following tables were not found in the schema '{0}': 'bogus', 'dummy'".format(test_schema), test_schema, ["bogus", test_view, "dummy"], test_output_relative)
EXPECT_FAIL("ValueError", "Following tables were not found in the schema '{0}': ''".format(test_schema), test_schema, [""], test_output_relative)

# WL13804-FR10 - The `outputUrl` parameter of the `util.dumpTables()` function must be a string value which specifies the output directory, where the dump data is going to be stored.
# * WL13804-FR10.1 - The requirements FR1.1.1-FR1.1.6 specified in WL#13807 apply here.

#@<> WL13804: WL13807-FR1.1 - The `outputUrl` parameter must be a string value which specifies the output directory, where the dump data is going to be stored.
# WL13804-TSFR_7_1
# WL13804-TSFR_8_2
EXPECT_SUCCESS(types_schema, types_schema_tables, test_output_absolute, { "ddlOnly": True, "showProgress": False })
# WL13804-TSFR_11_1_3
EXPECT_SUCCESS(types_schema, types_schema_tables, test_output_absolute, {})

#@<> WL13804: WL13807-FR1.1.1 - If the dump is going to be stored on the local filesystem, the `outputUrl` parameter may be optionally prefixed with `file://` scheme.
EXPECT_SUCCESS(types_schema, types_schema_tables, "file://" + test_output_absolute, { "ddlOnly": True, "showProgress": False })

#@<> WL13804-TSFR_10_1_5
EXPECT_FAIL("ValueError", "Directory handling for http protocol is not supported.", types_schema, types_schema_tables, "http://example.com", { "showProgress": False })
EXPECT_FAIL("ValueError", "Invalid PAR, expected: https://objectstorage.<region>.oraclecloud.com/p/<secret>/n/<namespace>/b/<bucket>/o/[<prefix>/][@.manifest.json]", types_schema, types_schema_tables, "HTTPS://www.example.com", { "showProgress": False })

#@<> WL13804: WL13807-FR1.1.2 - If the dump is going to be stored on the local filesystem and the `outputUrl` parameter holds a relative path, its absolute value is computed as relative to the current working directory.
EXPECT_SUCCESS(types_schema, types_schema_tables, test_output_relative, { "ddlOnly": True, "showProgress": False })

#@<> WL13804: WL13807-FR1.1.2 - relative with .
EXPECT_SUCCESS(types_schema, types_schema_tables, "./" + test_output_relative, { "ddlOnly": True, "showProgress": False })

#@<> WL13804: WL13807-FR1.1.2 - relative with ..
EXPECT_SUCCESS(types_schema, types_schema_tables, "dummy/../" + test_output_relative, { "ddlOnly": True, "showProgress": False })

#@<> WL13804: WL13807-FR1.1.1 + WL13807-FR1.1.2
EXPECT_SUCCESS(types_schema, types_schema_tables, "file://" + test_output_relative, { "ddlOnly": True, "showProgress": False })

#@<> WL13804: WL13807-FR1.1.3 - If the output directory does not exist, it must be created if its parent directory exists. If it is not possible or parent directory does not exist, an exception must be thrown.
# parent directory does not exist
shutil.rmtree(test_output_absolute, True)
EXPECT_FALSE(os.path.isdir(test_output_absolute))
EXPECT_FAIL("RuntimeError", "Could not create directory ", types_schema, types_schema_tables, os.path.join(test_output_absolute, "dummy"), { "showProgress": False })
EXPECT_FALSE(os.path.isdir(test_output_absolute))

# unable to create directory, file with the same name exists
shutil.rmtree(test_output_absolute, True)
EXPECT_FALSE(os.path.isdir(test_output_absolute))
open(test_output_relative, 'a').close()
EXPECT_FAIL("RuntimeError", "Could not create directory ", types_schema, types_schema_tables, test_output_absolute, { "showProgress": False })
EXPECT_FALSE(os.path.isdir(test_output_absolute))
os.remove(test_output_relative)

#@<> WL13804: WL13807-FR1.1.4 - If a local directory is used and the output directory must be created, `rwxr-x---` permissions should be used on the created directory. The owner of the directory is the user running the Shell. {__os_type != "windows"}
# WL13804-TSFR_10_1_10
EXPECT_SUCCESS(types_schema, types_schema_tables, test_output_absolute, { "ddlOnly": True, "showProgress": False })
# get the umask value
umask = os.umask(0o777)
os.umask(umask)
# use the umask value to compute the expected access rights
EXPECT_EQ(0o750 & ~umask, stat.S_IMODE(os.stat(test_output_absolute).st_mode))
EXPECT_EQ(os.getuid(), os.stat(test_output_absolute).st_uid)

#@<> WL13804: WL13807-FR1.1.5 - If the output directory exists and is not empty, an exception must be thrown.
# dump into empty directory
shutil.rmtree(test_output_absolute, True)
EXPECT_FALSE(os.path.isdir(test_output_absolute))
os.mkdir(test_output_absolute)
testutil.call_mysqlsh([uri, '--', 'util', 'dump-tables', types_schema, ','.join(types_schema_tables), '--output-url=' + test_output_absolute, '--ddl-only', '--show-progress=false'])
EXPECT_STDOUT_CONTAINS("Tables dumped: {0}".format(len(types_schema_tables)))

#@<> dump once again to the same directory, should fail
EXPECT_THROWS(lambda: util.dump_tables(types_schema, types_schema_tables, test_output_relative, { "showProgress": False }), "ValueError: Util.dump_tables: Cannot proceed with the dump, the specified directory '{0}' already exists at the target location {1} and is not empty.".format(test_output_relative, absolute_path_for_output(test_output_absolute)))

# WL13804-FR11.1 - The `options` parameter must accept the options described in WL#13807, FR3.
# WL13804: WL13807-FR3 - Both new functions must accept the following options specified in WL#13804, FR5:
# * The `maxRate` option specified in WL#13804, FR5.1.
# * The `showProgress` option specified in WL#13804, FR5.2.
# * The `compression` option specified in WL#13804, FR5.3, with the modification of FR5.3.2, the default value must be`"zstd"`.
# * The `osBucketName` option specified in WL#13804, FR5.4.
# * The `osNamespace` option specified in WL#13804, FR5.5.
# * The `ociConfigFile` option specified in WL#13804, FR5.6.
# * The `ociProfile` option specified in WL#13804, FR5.7.
# * The `defaultCharacterSet` option specified in WL#13804, FR5.8.

#@<> WL13804-FR5.1 - The `options` dictionary may contain a `maxRate` key with a string value, which specifies the limit of data read throughput in bytes per second per thread.
TEST_STRING_OPTION("maxRate")

EXPECT_SUCCESS(types_schema, types_schema_tables, test_output_absolute, { "maxRate": "1", "ddlOnly": True, "showProgress": False })
EXPECT_SUCCESS(types_schema, types_schema_tables, test_output_absolute, { "maxRate": "1000k", "ddlOnly": True, "showProgress": False })
EXPECT_SUCCESS(types_schema, types_schema_tables, test_output_absolute, { "maxRate": "1000K", "ddlOnly": True, "showProgress": False })
EXPECT_SUCCESS(types_schema, types_schema_tables, test_output_absolute, { "maxRate": "1M", "ddlOnly": True, "showProgress": False })
EXPECT_SUCCESS(types_schema, types_schema_tables, test_output_absolute, { "maxRate": "1G", "ddlOnly": True, "showProgress": False })

#@<> WL13804-FR5.1.1 - The value of the `maxRate` option must use the same format as specified in WL#12193.
EXPECT_FAIL("ValueError", 'Argument #4: Wrong input number "xyz"', types_schema, types_schema_tables, test_output_absolute, { "maxRate": "xyz" })
EXPECT_FAIL("ValueError", 'Argument #4: Wrong input number "1xyz"', types_schema, types_schema_tables, test_output_absolute, { "maxRate": "1xyz" })
EXPECT_FAIL("ValueError", 'Argument #4: Wrong input number "2Mhz"', types_schema, types_schema_tables, test_output_absolute, { "maxRate": "2Mhz" })
EXPECT_FAIL("ValueError", 'Argument #4: Wrong input number "hello world!"', types_schema, types_schema_tables, test_output_absolute, { "maxRate": "hello world!" })
EXPECT_FAIL("ValueError", 'Argument #4: Input number "-1" cannot be negative', types_schema, types_schema_tables, test_output_absolute, { "maxRate": "-1" })
EXPECT_FAIL("ValueError", 'Argument #4: Input number "-1234567890123456" cannot be negative', types_schema, types_schema_tables, test_output_absolute, { "maxRate": "-1234567890123456" })
EXPECT_FAIL("ValueError", 'Argument #4: Input number "-2K" cannot be negative', types_schema, types_schema_tables, test_output_absolute, { "maxRate": "-2K" })
EXPECT_FAIL("ValueError", 'Argument #4: Wrong input number "3m"', types_schema, types_schema_tables, test_output_absolute, { "maxRate": "3m" })
EXPECT_FAIL("ValueError", 'Argument #4: Wrong input number "4g"', types_schema, types_schema_tables, test_output_absolute, { "maxRate": "4g" })

EXPECT_SUCCESS(types_schema, types_schema_tables, test_output_absolute, { "maxRate": "1000000", "ddlOnly": True, "showProgress": False })

#@<> WL13804-FR5.1.1 - kilo.
EXPECT_SUCCESS(types_schema, types_schema_tables, test_output_absolute, { "maxRate": "1000k", "ddlOnly": True, "showProgress": False })

#@<> WL13804-FR5.1.1 - giga.
EXPECT_SUCCESS(types_schema, types_schema_tables, test_output_absolute, { "maxRate": "1G", "ddlOnly": True, "showProgress": False })

#@<> WL13804-FR5.1.2 - If the `maxRate` option is set to `"0"` or to an empty string, the read throughput must not be limited.
EXPECT_SUCCESS(types_schema, types_schema_tables, test_output_absolute, { "maxRate": "0", "ddlOnly": True, "showProgress": False })

#@<> WL13804-FR5.1.2 - empty string.
EXPECT_SUCCESS(types_schema, types_schema_tables, test_output_absolute, { "maxRate": "", "ddlOnly": True, "showProgress": False })

#@<> WL13804-FR5.1.2 - missing.
EXPECT_SUCCESS(types_schema, types_schema_tables, test_output_absolute, { "ddlOnly": True, "showProgress": False })

#@<> WL13804-FR5.2 - The `options` dictionary may contain a `showProgress` key with a Boolean value, which specifies whether to display the progress of dump process.
TEST_BOOL_OPTION("showProgress")

EXPECT_SUCCESS(types_schema, types_schema_tables, test_output_absolute, { "showProgress": True, "ddlOnly": True })

#@<> WL14297 - TSFR_5_1_3_1 - CLI tests dump tables passing url without the named parameter
shutil.rmtree(test_output_absolute, True)
EXPECT_FALSE(os.path.isdir(test_output_absolute))
rc = testutil.call_mysqlsh([uri, "--", "util", "dump-tables", types_schema] + types_schema_tables + [test_output_relative, "--showProgress", "true"])
EXPECT_STDOUT_CONTAINS("The following required option is missing: --output-url")

#@<> WL13804-FR5.2.1 - The information about the progress must include:
# * The estimated total number of rows to be dumped.
# * The number of rows dumped so far.
# * The current progress as a percentage.
# * The current throughput in rows per second.
# * The current throughput in bytes written per second.
rc = testutil.call_mysqlsh([uri, "--", "util", "dump-tables", types_schema] + types_schema_tables + ["--outputUrl", test_output_relative, "--showProgress", "true"])
EXPECT_EQ(0, rc)
EXPECT_TRUE(os.path.isdir(test_output_absolute))
EXPECT_STDOUT_MATCHES(re.compile(r'\d+% \(\d+\.?\d*[TGMK]? rows / ~\d+\.?\d*[TGMK]? rows\), \d+\.?\d*[TGMK]? rows?/s, \d+\.?\d* [TGMK]?B/s', re.MULTILINE))

#@<> WL13804-FR5.2.2 - If the `showProgress` option is not given, a default value of `true` must be used instead if shell is used interactively. Otherwise, it is set to `false`.
shutil.rmtree(test_output_absolute, True)
EXPECT_FALSE(os.path.isdir(test_output_absolute))
rc = testutil.call_mysqlsh([uri, "--", "util", "dump-tables", types_schema, json.dumps(types_schema_tables[:2], separators=(',', ':')), "--outputUrl", test_output_relative])
EXPECT_EQ(0, rc)
EXPECT_TRUE(os.path.isdir(test_output_absolute))
EXPECT_STDOUT_NOT_CONTAINS("rows/s")

#@<>WL13804-FR5.3 - The `options` dictionary may contain a `compression` key with a string value, which specifies the compression type used when writing the data dump files.
TEST_STRING_OPTION("compression")

EXPECT_SUCCESS(types_schema, types_schema_tables, test_output_absolute, { "compression": "none", "chunking": False, "showProgress": False })
EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(types_schema, types_schema_tables[0]) + ".tsv")))

#@<> WL13804-FR5.3.1 - The allowed values for the `compression` option are:
# * `"none"` - no compression is used,
# * `"gzip"` - gzip compression is used.
# * `"zstd"` - zstd compression is used.
EXPECT_SUCCESS(types_schema, types_schema_tables, test_output_absolute, { "compression": "gzip", "chunking": False, "showProgress": False })
EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(types_schema, types_schema_tables[0]) + ".tsv.gz")))

EXPECT_SUCCESS(types_schema, types_schema_tables, test_output_absolute, { "compression": "zstd", "chunking": False, "showProgress": False })
EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(types_schema, types_schema_tables[0]) + ".tsv.zst")))

EXPECT_FAIL("ValueError", "Argument #4: The option 'compression' cannot be set to an empty string.", types_schema, types_schema_tables, test_output_relative, { "compression": "" })
EXPECT_FAIL("ValueError", "Argument #4: Unknown compression type: dummy", types_schema, types_schema_tables, test_output_relative, { "compression": "dummy" })

#@<> WL13804-FR5.3.2 - If the `compression` option is not given, a default value of `"none"` must be used instead.
# * The `compression` option specified in WL#13804, FR5.3, with the modification of FR5.3.2, the default value must be`"zstd"`.
EXPECT_SUCCESS(types_schema, types_schema_tables, test_output_absolute, { "chunking": False, "showProgress": False })
EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(types_schema, types_schema_tables[0]) + ".tsv.zst")))

#@<> WL13804-FR5.4 - The `options` dictionary may contain a `osBucketName` key with a string value, which specifies the OCI bucket name where the data dump files are going to be stored.
TEST_STRING_OPTION("osBucketName")

#@<> WL13804-FR5.5 - The `options` dictionary may contain a `osNamespace` key with a string value, which specifies the OCI namespace (tenancy name) where the OCI bucket is located.
TEST_STRING_OPTION("osNamespace")

#@<> WL13804-FR5.5.2 - If the value of `osNamespace` option is a non-empty string and the value of `osBucketName` option is an empty string, an exception must be thrown.
EXPECT_FAIL("ValueError", "Argument #4: The option 'osNamespace' cannot be used when the value of 'osBucketName' option is not set.", types_schema, types_schema_tables, test_output_relative, { "osNamespace": "namespace" })

#@<> WL13804-FR5.6 - The `options` dictionary may contain a `ociConfigFile` key with a string value, which specifies the path to the OCI configuration file.
TEST_STRING_OPTION("ociConfigFile")

#@<> WL13804-FR5.6.2 - If the value of `ociConfigFile` option is a non-empty string and the value of `osBucketName` option is an empty string, an exception must be thrown.
EXPECT_FAIL("ValueError", "Argument #4: The option 'ociConfigFile' cannot be used when the value of 'osBucketName' option is not set.", types_schema, types_schema_tables, test_output_relative, { "ociConfigFile": "config" })
EXPECT_FAIL("ValueError", "Argument #4: The option 'ociConfigFile' cannot be used when the value of 'osBucketName' option is not set.", types_schema, types_schema_tables, test_output_relative, { "ociConfigFile": "config", "osBucketName": "" })

#@<> WL13804-FR5.7 - The `options` dictionary may contain a `ociProfile` key with a string value, which specifies the name of the OCI profile to use.
TEST_STRING_OPTION("ociProfile")

#@<> WL13804-FR5.7.2 - If the value of `ociProfile` option is a non-empty string and the value of `osBucketName` option is an empty string, an exception must be thrown.
EXPECT_FAIL("ValueError", "Argument #4: The option 'ociProfile' cannot be used when the value of 'osBucketName' option is not set.", types_schema, types_schema_tables, test_output_relative, { "ociProfile": "profile" })

#@<> Validate that the option ociParManifest only take boolean values as valid values.
TEST_BOOL_OPTION("ociParManifest")

#@<> Validate that the option ociParManifest is valid only when doing a dump to OCI.
EXPECT_FAIL("ValueError", "Argument #4: The option 'ociParManifest' cannot be used when the value of 'osBucketName' option is not set.", types_schema, types_schema_tables, test_output_relative, { "ociParManifest": True })
EXPECT_FAIL("ValueError", "Argument #4: The option 'ociParManifest' cannot be used when the value of 'osBucketName' option is not set.", types_schema, types_schema_tables, test_output_relative, { "ociParManifest": False })

#@<> Doing a dump to file system with ociParManifest set to True and ociParExpireTime set to a valid value. Validate that dump fails because ociParManifest is not valid if osBucketName is not specified.
EXPECT_FAIL("ValueError", "Argument #4: The option 'ociParManifest' cannot be used when the value of 'osBucketName' option is not set.", types_schema, types_schema_tables, test_output_relative, { "ociParManifest": True, "ociParExpireTime": "2021-01-01" })

#@<> Validate that the option ociParExpireTime only take string values
TEST_STRING_OPTION("ociParExpireTime")

#@<> Doing a dump to OCI ociParManifest not set or set to False and ociParExpireTime set to a valid value. Validate that the dump fail because ociParExpireTime it's valid only when ociParManifest is set to True.
EXPECT_FAIL("ValueError", "Argument #4: The option 'ociParExpireTime' cannot be used when the value of 'ociParManifest' option is not True.", types_schema, types_schema_tables, test_output_relative, { "ociParExpireTime": "2021-01-01" })
EXPECT_FAIL("ValueError", "Argument #4: The option 'ociParExpireTime' cannot be used when the value of 'ociParManifest' option is not True.", types_schema, types_schema_tables, test_output_relative, { "osBucketName": "bucket", "ociParExpireTime": "2021-01-01" })
EXPECT_FAIL("ValueError", "Argument #4: The option 'ociParExpireTime' cannot be used when the value of 'ociParManifest' option is not True.", types_schema, types_schema_tables, test_output_relative, { "osBucketName": "bucket", "ociParManifest": False, "ociParExpireTime": "2021-01-01" })

#@<> WL13804-FR5.8 - The `options` dictionary may contain a `defaultCharacterSet` key with a string value, which specifies the character set to be used during the dump. The session variables `character_set_client`, `character_set_connection`, and `character_set_results` must be set to this value for each opened connection.
TEST_STRING_OPTION("defaultCharacterSet")

EXPECT_SUCCESS(test_schema, test_schema_tables, test_output_absolute, { "defaultCharacterSet": "utf8mb4", "ddlOnly": True, "showProgress": False })
# name should be correctly encoded using UTF-8
EXPECT_FILE_CONTAINS("CREATE TABLE IF NOT EXISTS `{0}`".format(test_table_non_unique), os.path.join(test_output_absolute, encode_table_basename(test_schema, test_table_non_unique) + ".sql"))

#@<> WL13804-FR5.8.1 - If the value of the `defaultCharacterSet` option is not a character set supported by the MySQL server, an exception must be thrown.
EXPECT_FAIL("MySQL Error (1115)", "Unknown character set: ''", types_schema, types_schema_tables, test_output_relative, { "defaultCharacterSet": "" })
EXPECT_FAIL("MySQL Error (1115)", "Unknown character set: 'dummy'", types_schema, types_schema_tables, test_output_relative, { "defaultCharacterSet": "dummy" })

#@<> WL13804-FR5.8.2 - If the `defaultCharacterSet` option is not given, a default value of `"utf8mb4"` must be used instead.
EXPECT_SUCCESS(test_schema, test_schema_tables, test_output_absolute, { "ddlOnly": True, "showProgress": False })
# name should be correctly encoded using UTF-8
EXPECT_FILE_CONTAINS("CREATE TABLE IF NOT EXISTS `{0}`".format(test_table_non_unique), os.path.join(test_output_absolute, encode_table_basename(test_schema, test_table_non_unique) + ".sql"))

# WL13804-FR11.2 - The `options` parameter must accept the following options described in WL#13807:
# * The `consistent` option specified in WL#13807, FR4.1.
# * The `triggers` option specified in WL#13807, FR4.5.
# * The `tzUtc` option specified in WL#13807, FR4.6.
# * The `ddlOnly` option specified in WL#13807, FR4.7.
# * The `dataOnly` option specified in WL#13807, FR4.8.
# * The `dryRun` option specified in WL#13807, FR4.9.
# * The `chunking` option specified in WL#13807, FR4.12.
# * The `bytesPerChunk` option specified in WL#13807, FR4.13.
# * The `threads` option specified in WL#13807, FR4.14.

#@<> WL13804: WL13807-FR4.1 - The `options` dictionary may contain a `consistent` key with a Boolean value, which specifies whether the data from the tables should be dumped consistently.
# WL13804-TSFR_11_2_1
TEST_BOOL_OPTION("consistent")

# WL13804-TSFR_11_2_2
EXPECT_SUCCESS(types_schema, types_schema_tables, test_output_absolute, { "consistent": False, "showProgress": False })

#@<> WL13804: WL13807-FR4.5 - The `options` dictionary may contain a `triggers` key with a Boolean value, which specifies whether to include triggers in the DDL file of each table.
# WL13804-TSFR_11_2_6
TEST_BOOL_OPTION("triggers")

# WL13804-TSFR_11_2_5
EXPECT_SUCCESS(test_schema, test_schema_tables, test_output_absolute, { "triggers": False, "ddlOnly": True, "showProgress": False })
# WL13804-TSFR_12_6
EXPECT_FALSE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(test_schema, test_table_no_index) + ".triggers.sql")))

EXPECT_STDOUT_CONTAINS("4 tables and 0 views will be dumped.")

#@<> WL13804: WL13807-FR4.5.1 - If the `triggers` option is not given, a default value of `true` must be used instead.
# WL13804-TSFR_11_2_4
EXPECT_SUCCESS(test_schema, test_schema_tables, test_output_absolute, { "ddlOnly": True, "showProgress": False })
EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(test_schema, test_table_no_index) + ".triggers.sql")))

#@<> BUG#33396153 - dumpInstance() + triggers
EXPECT_SUCCESS(test_schema, test_schema_tables, test_output_absolute, { "excludeTriggers": [ f"`{test_schema}`.`{test_table_no_index}`" ], "dryRun": True, "showProgress": False })
EXPECT_STDOUT_CONTAINS("4 tables and 0 views will be dumped and within them 0 out of 1 trigger.")

#@<> WL13804: WL13807-FR4.6 - The `options` dictionary may contain a `tzUtc` key with a Boolean value, which specifies whether to set the time zone to UTC and include a `SET TIME_ZONE='+00:00'` statement in the DDL files and to execute this statement before the data dump is started. This allows dumping TIMESTAMP data when a server has data in different time zones or data is being moved between servers with different time zones.
# WL13804-TSFR_11_2_10
TEST_BOOL_OPTION("tzUtc")

EXPECT_SUCCESS(types_schema, types_schema_tables, test_output_absolute, { "tzUtc": False, "ddlOnly": True, "showProgress": False })
# WL13804-TSFR_11_2_9
with open(os.path.join(test_output_absolute, "@.json"), encoding="utf-8") as json_file:
    EXPECT_FALSE(json.load(json_file)["tzUtc"])

#@<> WL13804: WL13807-FR4.6.1 - If the `tzUtc` option is not given, a default value of `true` must be used instead.
# WL13804-TSFR_11_2_8
EXPECT_SUCCESS(types_schema, types_schema_tables, test_output_absolute, { "ddlOnly": True, "showProgress": False })
with open(os.path.join(test_output_absolute, "@.json"), encoding="utf-8") as json_file:
    EXPECT_TRUE(json.load(json_file)["tzUtc"])

#@<> WL13804: WL13807-FR4.7 - The `options` dictionary may contain a `ddlOnly` key with a Boolean value, which specifies whether the data dump should be skipped.
# WL13804-TSFR_11_2_13
TEST_BOOL_OPTION("ddlOnly")

# WL13804: WL13807-FR4.7.1 - If the `ddlOnly` option is set to `true`, only the DDL files must be written.
# WL13804-TSFR_11_2_12
EXPECT_SUCCESS(test_schema, test_schema_tables, test_output_absolute, { "ddlOnly": True, "showProgress": False })
EXPECT_EQ(0, count_files_with_extension(test_output_absolute, ".zst"))
EXPECT_NE(0, count_files_with_extension(test_output_absolute, ".sql"))

#@<> WL13804: WL13807-FR4.7.2 - If the `ddlOnly` option is not given, a default value of `false` must be used instead.
# WL13804-TSFR_11_2_11
EXPECT_SUCCESS(test_schema, test_schema_tables, test_output_absolute, { "showProgress": False })
EXPECT_NE(0, count_files_with_extension(test_output_absolute, ".zst"))
EXPECT_NE(0, count_files_with_extension(test_output_absolute, ".sql"))

#@<> WL13804: WL13807-FR4.8 - The `options` dictionary may contain a `dataOnly` key with a Boolean value, which specifies whether the DDL files should be written.
# WL13804-TSFR_11_2_16
TEST_BOOL_OPTION("dataOnly")

# WL13804: WL13807-FR4.8.1 - If the `dataOnly` option is set to `true`, only the data dump files must be written.
# WL13804-TSFR_11_2_15
EXPECT_SUCCESS(test_schema, test_schema_tables, test_output_absolute, { "dataOnly": True, "showProgress": False })
EXPECT_NE(0, count_files_with_extension(test_output_absolute, ".zst"))
# WL13804-TSFR_12_3
# WL13804-TSFR_12_8
EXPECT_EQ(0, count_files_with_extension(test_output_absolute, ".sql"))

#@<> WL13804: WL13807-FR4.8.2 - If both `ddlOnly` and `dataOnly` options are set to `true`, an exception must be raised.
EXPECT_FAIL("ValueError", "Argument #4: The 'ddlOnly' and 'dataOnly' options cannot be both set to true.", types_schema, types_schema_tables, test_output_relative, { "ddlOnly": True, "dataOnly": True })

#@<> WL13804: WL13807-FR4.8.3 - If the `dataOnly` option is not given, a default value of `false` must be used instead.
# WL13804-TSFR_11_2_14
EXPECT_SUCCESS(test_schema, test_schema_tables, test_output_absolute, { "showProgress": False })
EXPECT_NE(0, count_files_with_extension(test_output_absolute, ".zst"))
EXPECT_NE(0, count_files_with_extension(test_output_absolute, ".sql"))

#@<> WL13804: WL13807-FR4.9 - The `options` dictionary may contain a `dryRun` key with a Boolean value, which specifies whether a dry run should be performed.
# WL13804-TSFR_11_2_18
TEST_BOOL_OPTION("dryRun")

# WL13804: WL13807-FR4.9.1 - If the `dryRun` option is set to `true`, Shell must print information about what would be performed/dumped, including the compatibility notes for `ocimds` if available, but do not dump anything.
# WL13804-TSFR_11_2_17
shutil.rmtree(test_output_absolute, True)
EXPECT_FALSE(os.path.isdir(test_output_absolute))
WIPE_SHELL_LOG()
util.dump_tables(test_schema, test_schema_tables, test_output_absolute, { "dryRun": True, "showProgress": False })
EXPECT_FALSE(os.path.isdir(test_output_absolute))
EXPECT_STDOUT_NOT_CONTAINS("Tables dumped: ")
EXPECT_STDOUT_CONTAINS("Writing global DDL files")
EXPECT_STDOUT_CONTAINS("dryRun enabled, no locks will be acquired and no files will be created.")

for table in test_schema_tables:
    EXPECT_SHELL_LOG_CONTAINS("Writing DDL for table `{0}`.`{1}`".format(test_schema, table))
    EXPECT_SHELL_LOG_CONTAINS("Preparing data dump for table `{0}`.`{1}`".format(test_schema, table))

#@<> WL13804: WL13807-FR4.9.2 - If the `dryRun` option is not given, a default value of `false` must be used instead.
EXPECT_SUCCESS(test_schema, test_schema_tables, test_output_absolute, { "ddlOnly": True, "showProgress": False })

#@<> WL13804: WL13807-FR4.12 - The `options` dictionary may contain a `chunking` key with a Boolean value, which specifies whether to split the dump data into multiple files.
# WL13804-TSFR_11_2_19
TEST_BOOL_OPTION("chunking")

EXPECT_SUCCESS(test_schema, test_schema_tables, test_output_absolute, { "chunking": False, "showProgress": False })
EXPECT_FALSE(has_file_with_basename(test_output_absolute, encode_table_basename(test_schema, test_table_primary) + "@"))
EXPECT_FALSE(has_file_with_basename(test_output_absolute, encode_table_basename(test_schema, test_table_unique) + "@"))

#@<> WL13804: WL13807-FR4.12.3 - If the `chunking` option is not given, a default value of `true` must be used instead.
# WL13804-TSFR_11_2_27
EXPECT_SUCCESS(test_schema, test_schema_tables, test_output_absolute, { "showProgress": False })

# WL13804: WL13807-FR4.12.1 - If the `chunking` option is set to `true` and the index column cannot be selected automatically as described in FR3.1, the data must to written to a single dump file. A warning should be displayed to the user.
# WL13804: WL13807-FR3.1 - For each table dumped, its index column (name of the column used to order the data and perform the chunking) must be selected automatically as the first column used in the primary key, or if there is no primary key, as the first column used in the first unique index. If the table to be dumped does not contain a primary key and does not contain an unique index, the index column will not be defined.
EXPECT_STDOUT_CONTAINS("NOTE: Could not select columns to be used as an index for table `{0}`.`{1}`. Chunking has been disabled for this table, data will be dumped to a single file.".format(test_schema, test_table_non_unique))
EXPECT_STDOUT_CONTAINS("NOTE: Could not select columns to be used as an index for table `{0}`.`{1}`. Chunking has been disabled for this table, data will be dumped to a single file.".format(test_schema, test_table_no_index))

EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(test_schema, test_table_non_unique) + ".tsv.zst")))
EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(test_schema, test_table_no_index) + ".tsv.zst")))

# WL13804: WL13807-FR4.12.2 - If the `chunking` option is set to `true` and the index column can be selected automatically as described in FR3.1, the data must to written to multiple dump files. The data is partitioned into chunks using values from the index column.
EXPECT_TRUE(has_file_with_basename(test_output_absolute, encode_table_basename(test_schema, test_table_primary) + "@"))
EXPECT_TRUE(has_file_with_basename(test_output_absolute, encode_table_basename(test_schema, test_table_unique) + "@"))
number_of_dump_files = count_files_with_basename(test_output_absolute, encode_table_basename(test_schema, test_table_primary) + "@")

#@<> WL13804: WL13807-FR4.13 - The `options` dictionary may contain a `bytesPerChunk` key with a string value, which specifies the average estimated number of bytes to be written per each data dump file.
TEST_STRING_OPTION("bytesPerChunk")

# WL13804-TSFR_11_2_27_1
EXPECT_SUCCESS(test_schema, test_schema_tables, test_output_absolute, { "bytesPerChunk": "128k", "showProgress": False })

# WL13804: WL13807-FR4.13.1 - If the `bytesPerChunk` option is given, it must implicitly set the `chunking` option to `true`.
EXPECT_TRUE(has_file_with_basename(test_output_absolute, encode_table_basename(test_schema, test_table_primary) + "@"))
EXPECT_TRUE(has_file_with_basename(test_output_absolute, encode_table_basename(test_schema, test_table_unique) + "@"))

# WL13804: WL13807-FR4.13.4 - If the `bytesPerChunk` option is not given and the `chunking` option is set to `true`, a default value of `256M` must be used instead.
# this dump used smaller chunk size, number of files should be greater
EXPECT_TRUE(count_files_with_basename(test_output_absolute, encode_table_basename(test_schema, test_table_primary) + "@") > number_of_dump_files)

#@<> WL13804: WL13807-FR4.13.2 - The value of the `bytesPerChunk` option must use the same format as specified in WL#12193.
EXPECT_FAIL("ValueError", 'Argument #4: Wrong input number "xyz"', types_schema, types_schema_tables, test_output_absolute, { "bytesPerChunk": "xyz" })
EXPECT_FAIL("ValueError", 'Argument #4: Wrong input number "1xyz"', types_schema, types_schema_tables, test_output_absolute, { "bytesPerChunk": "1xyz" })
EXPECT_FAIL("ValueError", 'Argument #4: Wrong input number "2Mhz"', types_schema, types_schema_tables, test_output_absolute, { "bytesPerChunk": "2Mhz" })
# WL13804-TSFR_11_2_31
EXPECT_FAIL("ValueError", 'Argument #4: Wrong input number "0.1k"', types_schema, types_schema_tables, test_output_absolute, { "bytesPerChunk": "0.1k" })
EXPECT_FAIL("ValueError", 'Argument #4: Wrong input number "0,3M"', types_schema, types_schema_tables, test_output_absolute, { "bytesPerChunk": "0,3M" })
EXPECT_FAIL("ValueError", 'Argument #4: Input number "-1G" cannot be negative', types_schema, types_schema_tables, test_output_absolute, { "bytesPerChunk": "-1G" })
EXPECT_FAIL("ValueError", 'Argument #4: Wrong input number "hello"', types_schema, types_schema_tables, test_output_absolute, { "bytesPerChunk": "hello" })
EXPECT_FAIL("ValueError", "Argument #4: The option 'bytesPerChunk' cannot be set to an empty string.", types_schema, types_schema_tables, test_output_absolute, { "bytesPerChunk": "" })
# WL13804-TSFR_11_2_28
EXPECT_FAIL("ValueError", "Argument #4: The option 'bytesPerChunk' cannot be used if the 'chunking' option is set to false.", types_schema, types_schema_tables, test_output_absolute, { "bytesPerChunk": "128k", "chunking": False })

# WL13804-TSFR_11_2_29
EXPECT_SUCCESS(types_schema, types_schema_tables, test_output_absolute, { "bytesPerChunk": "1000k", "ddlOnly": True, "showProgress": False })
EXPECT_SUCCESS(types_schema, types_schema_tables, test_output_absolute, { "bytesPerChunk": "1M", "ddlOnly": True, "showProgress": False })
EXPECT_SUCCESS(types_schema, types_schema_tables, test_output_absolute, { "bytesPerChunk": "1G", "ddlOnly": True, "showProgress": False })
EXPECT_SUCCESS(types_schema, types_schema_tables, test_output_absolute, { "bytesPerChunk": "128000", "ddlOnly": True, "showProgress": False })

#@<> WL13804: WL13807-FR4.13.3 - If the value of the `bytesPerChunk` option is smaller than `128k`, an exception must be thrown.
# WL13804-TSFR_11_2_32
EXPECT_FAIL("ValueError", "Argument #4: The value of 'bytesPerChunk' option must be greater than or equal to 128k.", types_schema, types_schema_tables, test_output_relative, { "bytesPerChunk": "127k" })
EXPECT_FAIL("ValueError", "Argument #4: The value of 'bytesPerChunk' option must be greater than or equal to 128k.", types_schema, types_schema_tables, test_output_relative, { "bytesPerChunk": "127999" })
EXPECT_FAIL("ValueError", "Argument #4: The value of 'bytesPerChunk' option must be greater than or equal to 128k.", types_schema, types_schema_tables, test_output_relative, { "bytesPerChunk": "1" })
EXPECT_FAIL("ValueError", "Argument #4: The value of 'bytesPerChunk' option must be greater than or equal to 128k.", types_schema, types_schema_tables, test_output_relative, { "bytesPerChunk": "0" })
EXPECT_FAIL("ValueError", 'Argument #4: Input number "-1" cannot be negative', types_schema, types_schema_tables, test_output_relative, { "bytesPerChunk": "-1" })

#@<> WL13804: WL13807-FR4.14 - The `options` dictionary may contain a `threads` key with an unsigned integer value, which specifies the number of threads to be used to perform the dump.
# WL13804-TSFR_11_2_34
TEST_UINT_OPTION("threads")

# WL13804-TSFR_11_2_35
EXPECT_SUCCESS(types_schema, types_schema_tables, test_output_absolute, { "threads": 2, "ddlOnly": True, "showProgress": False })
EXPECT_STDOUT_CONTAINS("Running data dump using 2 threads.")

#@<> WL13804: WL13807-FR4.14.1 - If the value of the `threads` option is set to `0`, an exception must be thrown.
# WL13804-TSFR_11_2_34
EXPECT_FAIL("ValueError", "Argument #4: The value of 'threads' option must be greater than 0.", types_schema, types_schema_tables, test_output_relative, { "threads": 0 })

#@<> WL13804: WL13807-FR4.14.2 - If the `threads` option is not given, a default value of `4` must be used instead.
# WL13804-TSFR_11_2_36
EXPECT_SUCCESS(types_schema, types_schema_tables, test_output_absolute, { "ddlOnly": True, "showProgress": False })
EXPECT_STDOUT_CONTAINS("Running data dump using 4 threads.")

#@<> WL13804-FR11.3 - The `options` dictionary may contain an `all` key with a Boolean value, which specifies whether to dump all views and tables from the schema specified by the `schema` parameter.
# WL13804-TSFR_11_3_1_2
TEST_BOOL_OPTION("all")

#@<> WL13804-FR11.3.1 - If the `all` option is set to `true` and the `tables` parameter is an empty array, all views and tables from the schema specified by the `schema` parameter must be dumped.
# WL13804-TSFR_11_3_1_1
EXPECT_SUCCESS(test_schema, [], test_output_absolute, { "all": True, "ddlOnly": True, "showProgress": False }, expected_length=len(test_schema_tables))

for table in test_schema_tables:
    EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(test_schema, table) + ".sql")))

for view in test_schema_views:
    EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(test_schema, view) + ".sql")))

#@<> WL13804-TSFR_11_3_1_3
for dump_all in [-1, 5, 1.2]:
    EXPECT_SUCCESS(test_schema, [], test_output_absolute, { "all": dump_all, "ddlOnly": True, "showProgress": False }, expected_length=len(test_schema_tables))
    for table in test_schema_tables:
        EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(test_schema, table) + ".sql")))
    for view in test_schema_views:
        EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(test_schema, view) + ".sql")))

#@<> WL13804-FR11.3.1 - when both `all` and `dataOnly` are both true, SQL files for views should not be created
EXPECT_SUCCESS(test_schema, [], test_output_absolute, { "all": True, "dataOnly": True, "chunking": False, "showProgress": False }, expected_length=len(test_schema_tables))

for table in test_schema_tables:
    EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(test_schema, table) + ".tsv.zst")))

for view in test_schema_views:
    EXPECT_FALSE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(test_schema, view) + ".sql")))

#@<> WL13804-FR11.3.2 - If the `all` option is set to `true` and the `tables` parameter is not an empty array, an exception must be thrown.
EXPECT_FAIL("ValueError", "When the 'all' parameter is set to true, the 'tables' parameter must be an empty list.", types_schema, [types_schema_tables[0]], test_output_relative, { "all": True })
# WL13804-TSFR_11_3_2_1
EXPECT_FAIL("ValueError", "When the 'all' parameter is set to true, the 'tables' parameter must be an empty list.", types_schema, types_schema_tables + test_schema_views, test_output_relative, { "all": True })

#@<> WL13804-FR11.3.3 - If the `all` option is set to `false`, it is ignored.
# `all` is false and `tables` parameter is empty -> throw an exception
EXPECT_FAIL("ValueError", "The 'tables' parameter cannot be an empty list.", test_schema, [], test_output_relative, { "all": False })

#@<> `all` is false and `tables` parameter is not empty -> dump the selected table
# WL13804-TSFR_11_3_3_1
EXPECT_SUCCESS(test_schema, [test_schema_tables[0]], test_output_absolute, { "all": False, "ddlOnly": True, "showProgress": False }, test_schema_views)

EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(test_schema, test_schema_tables[0]) + ".sql")))

for table in test_schema_tables[1:]:
    EXPECT_FALSE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(test_schema, table) + ".sql")))

for view in test_schema_views:
    EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(test_schema, view) + ".sql")))

#@<> WL13804-FR11.3.4 - If the `all` option is not given, a default value of `false` must be used instead.
# `all` is not given and `tables` parameter is not empty -> act as if it was false and dump the selected table
# WL13804-TSFR_11_3_4_1
EXPECT_SUCCESS(test_schema, [test_schema_tables[0]], test_output_absolute, { "ddlOnly": True, "showProgress": False }, test_schema_views)

EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(test_schema, test_schema_tables[0]) + ".sql")))

for table in test_schema_tables[1:]:
    EXPECT_FALSE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(test_schema, table) + ".sql")))

for view in test_schema_views:
    EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(test_schema, view) + ".sql")))

#@<> options param being a dictionary that contains an unknown key
# WL13804-TSFR_11_1_2
for param in { "dummy", "users", "excludeUsers", "includeUsers", "indexColumn", "fieldsTerminatedBy", "fieldsEnclosedBy", "fieldsEscapedBy", "fieldsOptionallyEnclosed", "linesTerminatedBy", "dialect", "excludeSchemas", "includeSchemas", "excludeTables", "includeTables", "events", "excludeEvents", "includeEvents", "routines", "excludeRoutines", "includeRoutines" }:
    EXPECT_FAIL("ValueError", f"Argument #4: Invalid options: {param}", types_schema, types_schema_tables, test_output_relative, { param: "fails" })

# FR12 - The `util.dumpTables()` function must create:
# * table data dumps, as specified in WL#13807, FR7,
# * table DDL dumps, as specified in WL#13807, FR10,
# * view DDL dumps, as specified in WL#13807, FR11,
# * additional SQL files, as specified in WL#13807, FR12 excluding FR12.1.

#@<> run dump for all SQL-related tests below
EXPECT_SUCCESS(test_schema, test_schema_tables, test_output_absolute, { "ddlOnly": True, "showProgress": False }, test_schema_views)

#@<> WL13804: WL13807-FR10 - For each table dumped, a DDL file must be created with a base name as specified in FR7.1, and `.sql` extension.
# * The table DDL file must contain all objects being dumped.
# WL13804-TSFR_12_2
for table in test_schema_tables:
    EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(test_schema, table) + ".sql")))

#@<> WL13804: WL13807-FR10.1 - If the `triggers` option was set to `true` and the dumped table has triggers, a DDL file must be created with a base name as specified in FR7.1, and `.triggers.sql` extension. File must contain all triggers for that table.
# WL13804-TSFR_12_5
EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(test_schema, test_table_no_index) + ".triggers.sql")))

for table in [t for t in test_schema_tables if t != test_table_no_index]:
    EXPECT_FALSE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(test_schema, table) + ".triggers.sql")))

#@<> WL13804: WL13807-FR11 - For each view dumped, a DDL file must be created with a base name as specified in FR7.1, and `.sql` extension.
# WL13804-TSFR_12_7
for view in test_schema_views:
    EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(test_schema, view) + ".sql")))

#@<> WL13804: WL13807-FR11.1 - For each view dumped, a DDL file must be created with a base name as specified in FR7.1, and `.pre.sql` extension. This file must contain SQL statements which create a table with the same structure as the dumped view.
# WL13804-TSFR_12_9
for view in test_schema_views:
    EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(test_schema, view) + ".pre.sql")))

#@<> WL13804: WL13807-FR12 - The following DDL files must be created in the output directory:
# * A file with the name `@.sql`, containing the SQL statements which should be executed before the whole dump is imported.
# * A file with the name `@.post.sql`, containing the SQL statements which should be executed after the whole dump is imported.
# WL13804-TSFR_12_10
for f in [ "@.sql", "@.post.sql" ]:
    EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, f)))

#@<> WL13804-FR13 - It must be possible to load the dump created by `util.dumpTables()` function using `util.loadDump()` function specified in WL#13808.
# create a full dump to test util.load_dump() options
EXPECT_SUCCESS(test_schema, test_schema_tables, test_output_absolute, { "showProgress": False }, test_schema_views)

test_schema_tables_crc = []

for table in test_schema_tables:
    crc = {}
    crc["table"] = table
    crc["columns"] = get_all_columns(test_schema, table)
    crc["crc"] = compute_crc(test_schema, table, crc["columns"])
    test_schema_tables_crc.append(crc)

#@<> no active schema, dump should be loaded into the original one
session.run_sql("DROP SCHEMA !;", [ test_schema ])

EXPECT_NO_THROWS(lambda: util.load_dump(test_output_absolute, { "showProgress": False }), "loading the dump without active schema in global shell session")

# expect tables and views to be created
EXPECT_EQ(sorted(test_schema_tables + test_schema_views), sorted(get_all_tables(test_schema)))

# expect data to be correct
for crc in test_schema_tables_crc:
    print("---> checking: `{0}`.`{1}`".format(test_schema, crc["table"]))
    EXPECT_EQ(crc["crc"], compute_crc(test_schema, crc["table"], crc["columns"]))

# remove the progress file
testutil.rmfile(os.path.join(test_output_absolute, "load-progress*"))

#@<> active schema, dump should be loaded into the original one
session.run_sql("DROP SCHEMA !;", [ test_schema ])
recreate_verification_schema()

session.run_sql("USE !;", [ verification_schema ])

EXPECT_NO_THROWS(lambda: util.load_dump(test_output_absolute, { "showProgress": False }), "loading the dump with active schema in global shell session")

# verification schema should remain empty
EXPECT_EQ([], sorted(get_all_tables(verification_schema)))

# expect tables and views to be created
EXPECT_EQ(sorted(test_schema_tables + test_schema_views), sorted(get_all_tables(test_schema)))

# expect data to be correct
for crc in test_schema_tables_crc:
    print("---> checking: `{0}`.`{1}`".format(test_schema, crc["table"]))
    EXPECT_EQ(crc["crc"], compute_crc(test_schema, crc["table"], crc["columns"]))

# remove the progress file
testutil.rmfile(os.path.join(test_output_absolute, "load-progress*"))

#@<> WL13804-FR13.2 - The `util.loadDump()` function must accept a new option `schema` with a string value, which specifies the schema into which the dump should be loaded.
EXPECT_THROWS(lambda: util.load_dump(test_output_absolute, { "schema": None }), "TypeError: Util.load_dump: Argument #2: Option 'schema' is expected to be of type String, but is Null")
EXPECT_THROWS(lambda: util.load_dump(test_output_absolute, { "schema": 5 }), "TypeError: Util.load_dump: Argument #2: Option 'schema' is expected to be of type String, but is Integer")
EXPECT_THROWS(lambda: util.load_dump(test_output_absolute, { "schema": -5 }), "TypeError: Util.load_dump: Argument #2: Option 'schema' is expected to be of type String, but is Integer")
EXPECT_THROWS(lambda: util.load_dump(test_output_absolute, { "schema": [] }), "TypeError: Util.load_dump: Argument #2: Option 'schema' is expected to be of type String, but is Array")
EXPECT_THROWS(lambda: util.load_dump(test_output_absolute, { "schema": {} }), "TypeError: Util.load_dump: Argument #2: Option 'schema' is expected to be of type String, but is Map")
EXPECT_THROWS(lambda: util.load_dump(test_output_absolute, { "schema": False }), "TypeError: Util.load_dump: Argument #2: Option 'schema' is expected to be of type String, but is Bool")

#@<> choose the same active schema, use 'schema' option, expect success
# WL13804-TSFR_13_2_2
recreate_verification_schema()
session.run_sql("USE !;", [ verification_schema ])
EXPECT_NO_THROWS(lambda: util.load_dump(test_output_absolute, { "schema": verification_schema, "showProgress": False }), "loading the dump using 'schema' option")
# expect tables and views to be created
EXPECT_EQ(sorted(test_schema_tables + test_schema_views), sorted(get_all_tables(verification_schema)))
# expect data to be correct
for crc in test_schema_tables_crc:
    print("---> checking: `{0}`.`{1}`".format(verification_schema, crc["table"]))
    EXPECT_EQ(crc["crc"], compute_crc(verification_schema, crc["table"], crc["columns"]))
# remove the progress file
testutil.rmfile(os.path.join(test_output_absolute, "load-progress*"))

#@<> choose different active schema, use 'schema' option, expect success
# WL13804-TSFR_13_2_2
recreate_verification_schema()
session.run_sql("USE !;", [ "mysql" ])
EXPECT_NO_THROWS(lambda: util.load_dump(test_output_absolute, { "schema": verification_schema, "showProgress": False }), "loading the dump using 'schema' option")
# expect tables and views to be created
EXPECT_EQ(sorted(test_schema_tables + test_schema_views), sorted(get_all_tables(verification_schema)))
# expect data to be correct
for crc in test_schema_tables_crc:
    print("---> checking: `{0}`.`{1}`".format(verification_schema, crc["table"]))
    EXPECT_EQ(crc["crc"], compute_crc(verification_schema, crc["table"], crc["columns"]))
# remove the progress file
testutil.rmfile(os.path.join(test_output_absolute, "load-progress*"))

#@<> dump does not contain users information, trying to load it should result in a warning
recreate_verification_schema()

EXPECT_NO_THROWS(lambda: util.load_dump(test_output_absolute, { "loadUsers": True, "schema": verification_schema, "showProgress": False }), "loading the dump using 'loadUsers' option")
EXPECT_STDOUT_CONTAINS("WARNING: The 'loadUsers' option is set to true, but the dump does not contain the user data.")

testutil.rmfile(os.path.join(test_output_absolute, "load-progress*"))

#@<> WL13804-FR13.2.1 - If the specified schema does not exist, an exception must be thrown.
# WL13804-TSFR_13_2_1
EXPECT_THROWS(lambda: util.load_dump(test_output_absolute, { "schema": "dummy" }), "ValueError: Util.load_dump: The specified schema does not exist.")
EXPECT_THROWS(lambda: util.load_dump(test_output_absolute, { "schema": "1" }), "ValueError: Util.load_dump: The specified schema does not exist.")
EXPECT_THROWS(lambda: util.load_dump(test_output_absolute, { "schema": "!º\\" }), "ValueError: Util.load_dump: The specified schema does not exist.")
EXPECT_THROWS(lambda: util.load_dump(test_output_absolute, { "schema": "-1" }), "ValueError: Util.load_dump: The specified schema does not exist.")

# WL13804-FR13.2.2 - If dump was not created by `util.dumpTables()` function, and `schema` option is used, an exception must be thrown.
#@<> WL13804-FR13.2.2 - util.dump_schemas()
# WL13804-TSFR_13_2_3
shutil.rmtree(test_output_absolute, True)
EXPECT_NO_THROWS(lambda: util.dump_schemas([test_schema], test_output_absolute, { "ddlOnly": True, "showProgress": False }), "dumping a schema")
EXPECT_THROWS(lambda: util.load_dump(test_output_absolute, { "schema": verification_schema, "showProgress": False }), "ValueError: Util.load_dump: Invalid option: schema.")
EXPECT_STDOUT_CONTAINS("ERROR: The dump was not created by the util.dump_tables() function, the 'schema' option cannot be used.")

#@<> WL13804-FR13.2.2 - util.dump_instance()
# WL13804-TSFR_13_2_3
shutil.rmtree(test_output_absolute, True)
EXPECT_NO_THROWS(lambda: util.dump_instance(test_output_absolute, { "ddlOnly": True, "showProgress": False }), "dumping the whole instance")
EXPECT_THROWS(lambda: util.load_dump(test_output_absolute, { "schema": verification_schema, "showProgress": False }), "ValueError: Util.load_dump: Invalid option: schema.")
EXPECT_STDOUT_CONTAINS("ERROR: The dump was not created by the util.dump_tables() function, the 'schema' option cannot be used.")

# WL13804-FR13 - data verification tests
#@<> WL13804-FR13 - ddlOnly
recreate_verification_schema()
EXPECT_SUCCESS(test_schema, test_schema_tables, test_output_absolute, { "ddlOnly": True, "showProgress": False }, test_schema_views)
EXPECT_NO_THROWS(lambda: util.load_dump(test_output_absolute, { "schema": verification_schema, "showProgress": False }), "loading the dump using 'schema' option")
# expect tables and views to be created
EXPECT_EQ(sorted(test_schema_tables + test_schema_views), sorted(get_all_tables(verification_schema)))
# expect trigger to be created
EXPECT_EQ(test_table_trigger, session.run_sql("SELECT TRIGGER_NAME from information_schema.triggers WHERE EVENT_OBJECT_SCHEMA = ? AND EVENT_OBJECT_TABLE = ?;", [ test_schema, test_table_no_index ]).fetch_one()[0])

#@<> WL13804-FR13 - dataOnly
EXPECT_SUCCESS(test_schema, test_schema_tables, test_output_absolute, { "dataOnly": True, "showProgress": False }, test_schema_views)
# schema structure was created by the previous test, so ignore existing objects
EXPECT_NO_THROWS(lambda: util.load_dump(test_output_absolute, { "schema": verification_schema, "ignoreExistingObjects": True, "showProgress": False }), "loading the dump using 'schema' option")
# expect data to be correct
for crc in test_schema_tables_crc:
    print("---> checking: `{0}`.`{1}`".format(verification_schema, crc["table"]))
    EXPECT_EQ(crc["crc"], compute_crc(verification_schema, crc["table"], crc["columns"]))

#@<> WL13804-FR13 - full dump
recreate_verification_schema()
EXPECT_SUCCESS(test_schema, test_schema_tables, test_output_absolute, { "showProgress": False }, test_schema_views)
EXPECT_NO_THROWS(lambda: util.load_dump(test_output_absolute, { "schema": verification_schema, "showProgress": False }), "loading the dump using 'schema' option")
# expect tables and views to be created
EXPECT_EQ(sorted(test_schema_tables + test_schema_views), sorted(get_all_tables(verification_schema)))
# expect trigger to be created
EXPECT_EQ(test_table_trigger, session.run_sql("SELECT TRIGGER_NAME from information_schema.triggers WHERE EVENT_OBJECT_SCHEMA = ? AND EVENT_OBJECT_TABLE = ?;", [ test_schema, test_table_no_index ]).fetch_one()[0])
# expect data to be correct
for crc in test_schema_tables_crc:
    print("---> checking: `{0}`.`{1}`".format(verification_schema, crc["table"]))
    EXPECT_EQ(crc["crc"], compute_crc(verification_schema, crc["table"], crc["columns"]))

#<> simulate old dumpTables() behavior
# remove the SQL file for schema
testutil.rmfile(os.path.join(test_output_absolute, encode_schema_basename(test_schema) + ".sql"))

# change the global JSON config
global_config = os.path.join(test_output_absolute, "@.json")

with open(global_config, encoding="utf-8") as json_file:
    global_json = json.load(json_file)

del global_json["origin"]
global_json["tableOnly"] = True

with open(global_config, "w", encoding="utf-8") as json_file:
    json.dump(global_json, json_file)

# change the schema JSON config
schema_config = os.path.join(test_output_absolute, encode_schema_basename(test_schema) + ".json")

with open(schema_config, encoding="utf-8") as json_file:
    schema_json = json.load(json_file)

schema_json["includesDdl"] = False

with open(schema_config, "w", encoding="utf-8") as json_file:
    json.dump(schema_json, json_file)

#<> loading an old dumpTables() dump without 'schema' option should fail
recreate_verification_schema()
EXPECT_THROWS(lambda: util.load_dump(test_output_absolute, { "showProgress": False }), "ValueError: Util.load_dump: The target schema was not specified.")
EXPECT_STDOUT_CONTAINS("ERROR: The dump was created by an older version of the util.dump_tables() function and needs to be loaded into an existing schema. Please set the target schema using the 'schema' option.")

#<> loading an old dumpTables() dump with 'schema' option should succeed
# remove the progress file
testutil.rmfile(os.path.join(test_output_absolute, "load-progress*"))

EXPECT_NO_THROWS(lambda: util.load_dump(test_output_absolute, { "schema": verification_schema, "showProgress": False }), "loading the old dumpTables() using 'schema' option")

# expect tables and views to be created
EXPECT_EQ(sorted(test_schema_tables + test_schema_views), sorted(get_all_tables(verification_schema)))
# expect trigger to be created
EXPECT_EQ(test_table_trigger, session.run_sql("SELECT TRIGGER_NAME from information_schema.triggers WHERE EVENT_OBJECT_SCHEMA = ? AND EVENT_OBJECT_TABLE = ?;", [ test_schema, test_table_no_index ]).fetch_one()[0])
# expect data to be correct
for crc in test_schema_tables_crc:
    print("---> checking: `{0}`.`{1}`".format(verification_schema, crc["table"]))
    EXPECT_EQ(crc["crc"], compute_crc(verification_schema, crc["table"], crc["columns"]))

#@<> WL13804-FR15 - Once the dump is complete, the summary of the export process must be presented to the user. It must contain:
# * The number of rows written.
# * The number of bytes actually written to the data dump files.
# * The number of data bytes written to the data dump files. (only if `compression` option is not set to `none`)
# * The average throughout in bytes written per second.
#@<> WL13804: WL13807-FR13 - Once the dump is complete, in addition to the summary described in WL#13804, FR15, the following information must be presented to the user:
# * The number of schemas dumped.
# * The number of tables dumped.
# WL13804-TSFR_15_2
EXPECT_SUCCESS(test_schema, test_schema_tables, test_output_absolute, { "compression": "gzip", "ddlOnly": True, "showProgress": False })

EXPECT_STDOUT_CONTAINS("Schemas dumped: 1")
EXPECT_STDOUT_CONTAINS("Tables dumped: {0}".format(len(test_schema_tables)))
EXPECT_STDOUT_CONTAINS("Uncompressed data size: 0 bytes")
EXPECT_STDOUT_CONTAINS("Compressed data size: 0 bytes")
EXPECT_STDOUT_CONTAINS("Rows written: 0")
EXPECT_STDOUT_CONTAINS("Bytes written: 0 bytes")
EXPECT_STDOUT_CONTAINS("Average uncompressed throughput: 0.00 B/s")
EXPECT_STDOUT_CONTAINS("Average compressed throughput: 0.00 B/s")

#@<> test without compression
# WL13804-TSFR_15_1
EXPECT_SUCCESS(test_schema, test_schema_tables, test_output_absolute, { "compression": "none", "ddlOnly": True, "showProgress": False })

EXPECT_STDOUT_CONTAINS("Schemas dumped: 1")
EXPECT_STDOUT_CONTAINS("Tables dumped: {0}".format(len(test_schema_tables)))
EXPECT_STDOUT_CONTAINS("Data size:")
EXPECT_STDOUT_CONTAINS("Rows written: 0")
EXPECT_STDOUT_CONTAINS("Bytes written: 0 bytes")
EXPECT_STDOUT_CONTAINS("Average throughput: 0.00 B/s")

#@<> WL13804-FR16 - All files should be created with permissions `rw-r-----`. {__os_type != "windows"}
# WL13804-TSFR_16_1
EXPECT_SUCCESS(test_schema, test_schema_tables, test_output_absolute, { "showProgress": False })

for f in os.listdir(test_output_absolute):
    path = os.path.join(test_output_absolute, f)
    EXPECT_EQ(0o640, stat.S_IMODE(os.stat(path).st_mode))
    EXPECT_EQ(os.getuid(), os.stat(path).st_uid)


#@<> The `options` dictionary may contain a `ocimds` key with a Boolean or string value, which specifies whether the compatibility checks with `MySQL Database Service` and DDL substitutions should be done.
TEST_BOOL_OPTION("ocimds")

#@<> If the `ocimds` option is set to `true`, the following must be done:
# * General
#   * Add the `mysql` schema to the schema exclusion list
# * GRANT
#   * Check whether users or roles are granted the following privileges and error out if so.
#     * SUPER,
#     * FILE,
#     * RELOAD,
#     * BINLOG_ADMIN,
#     * SET_USER_ID.
# * CREATE TABLE
#   * If ENGINE is not `InnoDB`, an exception must be raised.
#   * DATA|INDEX DIRECTORY and ENCRYPTION options must be commented out.
#   * Same restrictions for partitions.
# * CHARSETS - checks whether db objects use any other character set than supported utf8mb4.
EXPECT_FAIL("RuntimeError", "Compatibility issues were found", incompatible_schema, incompatible_schema_tables + incompatible_schema_views, test_output_relative, { "ocimds": True })
EXPECT_STDOUT_CONTAINS("Checking for compatibility with MySQL Database Service {0}".format(__mysh_version_no_extra))

if __version_num < 80000:
    EXPECT_STDOUT_CONTAINS("NOTE: MySQL Server 5.7 detected, please consider upgrading to 8.0 first.")
    EXPECT_STDOUT_CONTAINS("NOTE: You can check for potential upgrade issues using util.check_for_server_upgrade().")

EXPECT_STDOUT_CONTAINS(comment_data_index_directory(incompatible_schema, incompatible_table_data_directory).fixed())

EXPECT_STDOUT_CONTAINS(comment_encryption(incompatible_schema, incompatible_table_encryption).fixed())

if __version_num < 80000 and __os_type != "windows":
    EXPECT_STDOUT_CONTAINS(comment_data_index_directory(incompatible_schema, incompatible_table_index_directory).fixed())

EXPECT_STDOUT_CONTAINS(force_innodb_unsupported_storage(incompatible_schema, incompatible_table_index_directory).error())

EXPECT_STDOUT_CONTAINS(strip_tablespaces(incompatible_schema, incompatible_table_tablespace).error())

EXPECT_STDOUT_CONTAINS(force_innodb_unsupported_storage(incompatible_schema, incompatible_table_wrong_engine).error())

EXPECT_STDOUT_CONTAINS(strip_definers_definer_clause(incompatible_schema, incompatible_view).error())
EXPECT_STDOUT_CONTAINS(strip_definers_security_clause(incompatible_schema, incompatible_view).error())

EXPECT_STDOUT_CONTAINS("Compatibility issues with MySQL Database Service {0} were found. Please use the 'compatibility' option to apply compatibility adaptations to the dumped DDL.".format(__mysh_version_no_extra))

# WL14506-FR1 - When a dump is executed with the ocimds option set to true, for each table that would be dumped which does not contain a primary key, an error must be reported.
# WL14506-TSFR_1_14
missing_pks = {
    "xtest": [
        "t_bigint",
        "t_bit",
        "t_char",
        "t_date",
        "t_decimal1",
        "t_decimal2",
        "t_decimal3",
        "t_double",
        "t_enum",
        "t_float",
        "t_geom",
        "t_geom_all",
        "t_int",
        "t_integer",
        "t_json",
        "t_lchar",
        "t_lob",
        "t_mediumint",
        "t_numeric1",
        "t_numeric2",
        "t_real",
        "t_set",
        "t_smallint",
        "t_tinyint",
    ],
    incompatible_schema: [
        incompatible_table_data_directory,
        incompatible_table_encryption,
        incompatible_table_index_directory,
        incompatible_table_tablespace,
        incompatible_table_wrong_engine,
    ],
    test_schema: [
        test_table_unique,
        test_table_non_unique,
        test_table_no_index
    ]
}

# only tables from incompatible_schema are included in dump
for table in missing_pks[incompatible_schema]:
    EXPECT_STDOUT_CONTAINS(create_invisible_pks(incompatible_schema, table).error())

# remaining tables are not included and should not be reported
for table in missing_pks["xtest"] + missing_pks[test_schema]:
    EXPECT_STDOUT_NOT_CONTAINS(table)

# WL14506-FR1.1 - The following message must be printed:
# WL14506-TSFR_1_14
EXPECT_STDOUT_CONTAINS("""
ERROR: One or more tables without Primary Keys were found.

       MySQL Database Service High Availability (MDS HA) requires Primary Keys to be present in all tables.
       To continue with the dump you must do one of the following:

       * Create PRIMARY keys in all tables before dumping them.
         MySQL 8.0.23 supports the creation of invisible columns to allow creating Primary Key columns with no impact to applications. For more details, see https://dev.mysql.com/doc/refman/en/invisible-columns.html.
         This is considered a best practice for both performance and usability and will work seamlessly with MDS.

       * Add the "create_invisible_pks" to the "compatibility" option.
         The dump will proceed and loader will automatically add Primary Keys to tables that don't have them when loading into MDS.
         This will make it possible to enable HA in MDS without application impact.
         However, Inbound Replication into an MDS HA instance (at the time of the release of MySQL Shell 8.0.24) will still not be possible.

       * Add the "ignore_missing_pks" to the "compatibility" option.
         This will disable this check and the dump will be produced normally, Primary Keys will not be added automatically.
         It will not be possible to load the dump in an HA enabled MDS instance.
""")

#@<> WL14506-TSFR_2_1
util.help("dump_tables")

EXPECT_STDOUT_CONTAINS("""
      create_invisible_pks - Each table which does not have a Primary Key will
      have one created when the dump is loaded. The following Primary Key is
      added to the table:
      `my_row_id` BIGINT UNSIGNED AUTO_INCREMENT INVISIBLE PRIMARY KEY

      At the time of the release of MySQL Shell 8.0.24, dumps created with this
      value cannot be used with Inbound Replication into an MySQL Database
      Service instance with High Availability. Mutually exclusive with the
      ignore_missing_pks value.
""")
EXPECT_STDOUT_CONTAINS("""
      ignore_missing_pks - Ignore errors caused by tables which do not have
      Primary Keys. Dumps created with this value cannot be used in MySQL
      Database Service instance with High Availability. Mutually exclusive with
      the create_invisible_pks value.
""")
EXPECT_STDOUT_CONTAINS("""
      At the time of the release of MySQL Shell 8.0.24, in order to use Inbound
      Replication into an MySQL Database Service instance with High
      Availability, all tables at the source server need to have Primary Keys.
      This needs to be fixed manually before running the dump. Starting with
      MySQL 8.0.23 invisible columns may be used to add Primary Keys without
      changing the schema compatibility, for more information see:
      https://dev.mysql.com/doc/refman/en/invisible-columns.html.

      In order to use MySQL Database Service instance with High Availability,
      all tables at the MDS server need to have Primary Keys. This can be fixed
      automatically using the create_invisible_pks compatibility value.
""")

#@<> If the `ocimds` option is not given, a default value of `false` must be used instead.
EXPECT_SUCCESS(incompatible_schema, incompatible_schema_tables, test_output_absolute, { "ddlOnly": True, "showProgress": False }, incompatible_schema_views)

#@<> The `options` dictionary may contain a `compatibility` key with an array of strings value, which specifies the `MySQL Database Service`-related compatibility modifications that should be applied when creating the DDL files.
TEST_ARRAY_OF_STRINGS_OPTION("compatibility")

EXPECT_FAIL("ValueError", "Argument #4: Unknown compatibility option: dummy", incompatible_schema, incompatible_schema_tables + incompatible_schema_views, test_output_relative, { "compatibility": [ "dummy" ] })
EXPECT_FAIL("ValueError", "Argument #4: Unknown compatibility option: ", incompatible_schema, incompatible_schema_tables + incompatible_schema_views, test_output_relative, { "compatibility": [ "" ] })

#@<> The `compatibility` option may contain the following values:
# * `force_innodb` - replace incompatible table engines with `InnoDB`,
# * `strip_restricted_grants` - remove disallowed grants.
# * `strip_tablespaces` - remove unsupported tablespace syntax.
# * `strip_definers` - remove DEFINER clause from views, triggers, events and routines and change SQL SECURITY property to INVOKER for views and routines.
# WL14506-FR2 - The compatibility option of the dump utilities must support a new value: ignore_missing_pks.
# WL14506-TSFR_1_15
EXPECT_SUCCESS(incompatible_schema, incompatible_schema_tables, test_output_absolute, { "ocimds": True, "compatibility": [ "force_innodb", "ignore_missing_pks", "strip_definers", "strip_restricted_grants", "strip_tablespaces" ] , "ddlOnly": True, "showProgress": False }, incompatible_schema_views)

# WL14506-FR2.2 - The following message must be printed:
# WL14506-TSFR_1_15
EXPECT_STDOUT_CONTAINS("""
NOTE: One or more tables without Primary Keys were found.

      This issue is ignored.
      This dump cannot be loaded into an MySQL Database Service instance with High Availability.
""")

# WL14506-FR3 - The compatibility option of the dump utilities must support a new value: create_invisible_pks.
# create_invisible_pks and ignore_missing_pks are mutually exclusive
# WL14506-TSFR_1_16
EXPECT_SUCCESS(incompatible_schema, incompatible_schema_tables, test_output_absolute, { "ocimds": True, "compatibility": [ "create_invisible_pks", "force_innodb", "strip_definers", "strip_restricted_grants", "strip_tablespaces" ] , "ddlOnly": True, "showProgress": False }, incompatible_schema_views)

# WL14506-FR3.1.1 - The following message must be printed:
# WL14506-TSFR_1_16
EXPECT_STDOUT_CONTAINS("""
NOTE: One or more tables without Primary Keys were found.

      Missing Primary Keys will be created automatically when this dump is loaded.
      This will make it possible to enable High Availability in MySQL Database Service instance without application impact.
      However, Inbound Replication into an MDS HA instance (at the time of the release of MySQL Shell 8.0.24) will still not be possible.
""")

#@<> WL14506-FR3.1 - When a dump is executed with the ocimds option set to true and the compatibility option contains the create_invisible_pks value, for each table that would be dumped which does not contain a primary key, an information should be printed that an invisible primary key will be created when loading the dump.
EXPECT_SUCCESS(incompatible_schema, incompatible_schema_tables, test_output_absolute, { "compatibility": [ "create_invisible_pks" ] , "ddlOnly": True, "showProgress": False }, incompatible_schema_views)

for table in missing_pks[incompatible_schema]:
    EXPECT_STDOUT_CONTAINS(create_invisible_pks(incompatible_schema, table).fixed())

#@<> WL14506-FR3.2 - When a dump is executed and the compatibility option contains the create_invisible_pks value, for each table that would be dumped which does not contain a primary key but has a column named my_row_id, an error must be reported.
# WL14506-TSFR_3.2_3
table = missing_pks[incompatible_schema][0]
session.run_sql("ALTER TABLE !.! ADD COLUMN my_row_id int;", [incompatible_schema, table])

EXPECT_FAIL("RuntimeError", "Fatal error during dump", incompatible_schema, incompatible_schema_tables + incompatible_schema_views, test_output_relative, { "compatibility": [ "create_invisible_pks" ] }, True)
EXPECT_STDOUT_CONTAINS(create_invisible_pks_name_conflict(incompatible_schema, table).error())
EXPECT_STDOUT_CONTAINS("Could not apply some of the compatibility options")

WIPE_OUTPUT()
EXPECT_FAIL("RuntimeError", "Compatibility issues were found", incompatible_schema, incompatible_schema_tables + incompatible_schema_views, test_output_relative, { "ocimds": True, "compatibility": [ "create_invisible_pks" ] })
EXPECT_STDOUT_CONTAINS(create_invisible_pks_name_conflict(incompatible_schema, table).error())

session.run_sql("ALTER TABLE !.! DROP COLUMN my_row_id;", [incompatible_schema, table])

#@<> WL14506-FR3.4 - When a dump is executed and the compatibility option contains the create_invisible_pks value, for each table that would be dumped which does not contain a primary key but has a column with an AUTO_INCREMENT attribute, an error must be reported.
table = missing_pks[incompatible_schema][0]
session.run_sql("ALTER TABLE !.! ADD COLUMN idx int AUTO_INCREMENT UNIQUE;", [incompatible_schema, table])

EXPECT_FAIL("RuntimeError", "Fatal error during dump", incompatible_schema, incompatible_schema_tables + incompatible_schema_views, test_output_relative, { "compatibility": [ "create_invisible_pks" ] }, True)
EXPECT_STDOUT_CONTAINS(create_invisible_pks_auto_increment_conflict(incompatible_schema, table).error())
EXPECT_STDOUT_CONTAINS("Could not apply some of the compatibility options")

WIPE_OUTPUT()
EXPECT_FAIL("RuntimeError", "Compatibility issues were found", incompatible_schema, incompatible_schema_tables + incompatible_schema_views, test_output_relative, { "ocimds": True, "compatibility": [ "create_invisible_pks" ] })
EXPECT_STDOUT_CONTAINS(create_invisible_pks_auto_increment_conflict(incompatible_schema, table).error())

session.run_sql("ALTER TABLE !.! DROP COLUMN idx;", [incompatible_schema, table])

#@<> WL14506-FR3.2 + WL14506-FR3.4 - same column
table = missing_pks[incompatible_schema][0]
session.run_sql("ALTER TABLE !.! ADD COLUMN my_row_id int AUTO_INCREMENT UNIQUE;", [incompatible_schema, table])

EXPECT_FAIL("RuntimeError", "Fatal error during dump", incompatible_schema, incompatible_schema_tables + incompatible_schema_views, test_output_relative, { "compatibility": [ "create_invisible_pks" ] }, True)
EXPECT_STDOUT_CONTAINS(create_invisible_pks_name_conflict(incompatible_schema, table).error())
EXPECT_STDOUT_CONTAINS(create_invisible_pks_auto_increment_conflict(incompatible_schema, table).error())
EXPECT_STDOUT_CONTAINS("Could not apply some of the compatibility options")

WIPE_OUTPUT()
EXPECT_FAIL("RuntimeError", "Compatibility issues were found", incompatible_schema, incompatible_schema_tables + incompatible_schema_views, test_output_relative, { "ocimds": True, "compatibility": [ "create_invisible_pks" ] })
EXPECT_STDOUT_CONTAINS(create_invisible_pks_name_conflict(incompatible_schema, table).error())
EXPECT_STDOUT_CONTAINS(create_invisible_pks_auto_increment_conflict(incompatible_schema, table).error())

session.run_sql("ALTER TABLE !.! DROP COLUMN my_row_id;", [incompatible_schema, table])

#@<> WL14506-FR3.2 + WL14506-FR3.4 - different columns
table = missing_pks[incompatible_schema][0]
session.run_sql("ALTER TABLE !.! ADD COLUMN my_row_id int;", [incompatible_schema, table])
session.run_sql("ALTER TABLE !.! ADD COLUMN idx int AUTO_INCREMENT UNIQUE;", [incompatible_schema, table])

EXPECT_FAIL("RuntimeError", "Fatal error during dump", incompatible_schema, incompatible_schema_tables + incompatible_schema_views, test_output_relative, { "compatibility": [ "create_invisible_pks" ] }, True)
EXPECT_STDOUT_CONTAINS(create_invisible_pks_name_conflict(incompatible_schema, table).error())
EXPECT_STDOUT_CONTAINS(create_invisible_pks_auto_increment_conflict(incompatible_schema, table).error())
EXPECT_STDOUT_CONTAINS("Could not apply some of the compatibility options")

WIPE_OUTPUT()
EXPECT_FAIL("RuntimeError", "Compatibility issues were found", incompatible_schema, incompatible_schema_tables + incompatible_schema_views, test_output_relative, { "ocimds": True, "compatibility": [ "create_invisible_pks" ] })
EXPECT_STDOUT_CONTAINS(create_invisible_pks_name_conflict(incompatible_schema, table).error())
EXPECT_STDOUT_CONTAINS(create_invisible_pks_auto_increment_conflict(incompatible_schema, table).error())

session.run_sql("ALTER TABLE !.! DROP COLUMN my_row_id;", [incompatible_schema, table])
session.run_sql("ALTER TABLE !.! DROP COLUMN idx;", [incompatible_schema, table])

#@<> WL14506-FR3.3 - When a dump is executed and the compatibility option contains both create_invisible_pks and ignore_missing_pks values, an error must be reported and process must be aborted.
# WL14506-TSFR_3.3_1
EXPECT_FAIL("ValueError", "Argument #4: The 'create_invisible_pks' and 'ignore_missing_pks' compatibility options cannot be used at the same time.", incompatible_schema, incompatible_schema_tables + incompatible_schema_views, test_output_relative, { "compatibility": [ "create_invisible_pks", "ignore_missing_pks" ] })

#@<> force_innodb
EXPECT_SUCCESS(incompatible_schema, incompatible_schema_tables, test_output_absolute, { "compatibility": [ "force_innodb" ] , "ddlOnly": True, "showProgress": False }, incompatible_schema_views)
EXPECT_STDOUT_CONTAINS(force_innodb_unsupported_storage(incompatible_schema, incompatible_table_index_directory).fixed())
EXPECT_STDOUT_CONTAINS(force_innodb_unsupported_storage(incompatible_schema, incompatible_table_wrong_engine).fixed())

#@<> WL14506-FR2.1 - When a dump is executed with the ocimds option set to true and the compatibility option contains the ignore_missing_pks value, for each table that would be dumped which does not contain a primary key, a note must be displayed, stating that this issue is ignored.
EXPECT_SUCCESS(incompatible_schema, incompatible_schema_tables, test_output_absolute, { "compatibility": [ "ignore_missing_pks" ] , "ddlOnly": True, "showProgress": False }, incompatible_schema_views)

for table in missing_pks[incompatible_schema]:
    EXPECT_STDOUT_CONTAINS(ignore_missing_pks(incompatible_schema, table).fixed())

#@<> strip_definers
EXPECT_SUCCESS(incompatible_schema, incompatible_schema_tables, test_output_absolute, { "compatibility": [ "strip_definers" ] , "ddlOnly": True, "showProgress": False }, incompatible_schema_views)
EXPECT_STDOUT_CONTAINS(strip_definers_definer_clause(incompatible_schema, incompatible_view).fixed())
EXPECT_STDOUT_CONTAINS(strip_definers_security_clause(incompatible_schema, incompatible_view).fixed())

#@<> strip_tablespaces
EXPECT_SUCCESS(incompatible_schema, incompatible_schema_tables, test_output_absolute, { "compatibility": [ "strip_tablespaces" ] , "ddlOnly": True, "showProgress": False }, incompatible_schema_views)
EXPECT_STDOUT_CONTAINS(strip_tablespaces(incompatible_schema, incompatible_table_tablespace).fixed())

#@<> If the `compatibility` option is not given, a default value of `[]` must be used instead.
EXPECT_SUCCESS(incompatible_schema, incompatible_schema_tables, test_output_absolute, { "ddlOnly": True, "showProgress": False }, incompatible_schema_views)
EXPECT_STDOUT_NOT_CONTAINS(force_innodb_unsupported_storage(incompatible_schema, incompatible_table_index_directory).fixed())
EXPECT_STDOUT_NOT_CONTAINS(force_innodb_unsupported_storage(incompatible_schema, incompatible_table_wrong_engine).fixed())
EXPECT_STDOUT_NOT_CONTAINS(strip_definers_definer_clause(incompatible_schema, incompatible_view).fixed())
EXPECT_STDOUT_NOT_CONTAINS(strip_definers_security_clause(incompatible_schema, incompatible_view).fixed())
EXPECT_STDOUT_NOT_CONTAINS(strip_tablespaces(incompatible_schema, incompatible_table_tablespace).fixed())

#@<> test a single table with no chunking
EXPECT_SUCCESS(world_x_schema, [world_x_table], test_output_absolute, { "chunking": False, "compression": "none", "showProgress": False })
TEST_LOAD(world_x_schema, world_x_table)

#@<> test multiple tables with chunking
EXPECT_SUCCESS(test_schema, test_schema_tables, test_output_absolute, { "bytesPerChunk": "1M", "compression": "none", "showProgress": False })
TEST_LOAD(test_schema, test_table_primary)
TEST_LOAD(test_schema, test_table_unique)
# these are not chunked because they don't have an appropriate column
TEST_LOAD(test_schema, test_table_non_unique)
TEST_LOAD(test_schema, test_table_no_index)

#@<> test multiple tables with various data types
EXPECT_SUCCESS(types_schema, types_schema_tables, test_output_absolute, { "chunking": False, "compression": "none", "showProgress": False })

for table in types_schema_tables:
    TEST_LOAD(types_schema, table)

#@<> dump table when different character set/SQL mode is used
session.run_sql("SET NAMES 'latin1';")
session.run_sql("SET GLOBAL SQL_MODE='ANSI_QUOTES,NO_AUTO_VALUE_ON_ZERO,NO_BACKSLASH_ESCAPES,NO_DIR_IN_CREATE,NO_ZERO_DATE,PAD_CHAR_TO_FULL_LENGTH';")

EXPECT_SUCCESS(world_x_schema, [world_x_table], test_output_absolute, { "defaultCharacterSet": "latin1", "bytesPerChunk": "1M", "compression": "none", "showProgress": False })
TEST_LOAD(world_x_schema, world_x_table)

session.run_sql("SET NAMES 'utf8mb4';")
session.run_sql("SET GLOBAL SQL_MODE='';")

#@<> prepare user privileges, switch user
session.run_sql(f"GRANT ALL ON *.* TO {test_user_account};")
session.run_sql(f"REVOKE RELOAD /*!80023 , FLUSH_TABLES */ ON *.* FROM {test_user_account};")
shell.connect(test_user_uri(__mysql_sandbox_port1))

#@<> try to run consistent dump using a user which does not have required privileges for FTWRL but LOCK TABLES are ok
EXPECT_SUCCESS(types_schema, types_schema_tables, test_output_absolute, { "showProgress": False })
EXPECT_STDOUT_CONTAINS("WARNING: The current user lacks privileges to acquire a global read lock using 'FLUSH TABLES WITH READ LOCK'. Falling back to LOCK TABLES...")

#@<> revoke lock tables from mysql.* {VER(>=8.0.16)}
setup_session()
session.run_sql("SET GLOBAL partial_revokes=1")
session.run_sql(f"REVOKE LOCK TABLES ON mysql.* FROM {test_user_account};")
shell.connect(test_user_uri(__mysql_sandbox_port1))

#@<> try again, this time it should succeed but without locking mysql.* tables {VER(>=8.0.16)}
EXPECT_SUCCESS(world_x_schema, [world_x_table], test_output_absolute, { "showProgress": False })
EXPECT_STDOUT_CONTAINS("WARNING: The current user lacks privileges to acquire a global read lock using 'FLUSH TABLES WITH READ LOCK'. Falling back to LOCK TABLES...")
EXPECT_STDOUT_CONTAINS(f"WARNING: Could not lock mysql system tables: User {test_user_account} is missing the following privilege(s) for schema `mysql`: LOCK TABLES.")

#@<> revoke lock tables from the rest
setup_session()
session.run_sql(f"REVOKE LOCK TABLES ON *.* FROM {test_user_account};")
shell.connect(test_user_uri(__mysql_sandbox_port1))

#@<> try to run consistent dump using a user which does not have any required privileges
EXPECT_FAIL("RuntimeError", re.compile(r"Unable to lock tables: User {0} is missing the following privilege\(s\) for table `.+`\.`.+`: LOCK TABLES.".format(test_user_account)), types_schema, types_schema_tables, test_output_absolute, { "showProgress": False })
EXPECT_STDOUT_CONTAINS("WARNING: The current user lacks privileges to acquire a global read lock using 'FLUSH TABLES WITH READ LOCK'. Falling back to LOCK TABLES...")
EXPECT_STDOUT_CONTAINS("ERROR: Unable to acquire global read lock neither table read locks")

#@<> using the same user, run inconsistent dump
EXPECT_SUCCESS(types_schema, types_schema_tables, test_output_absolute, { "consistent": False, "ddlOnly": True, "showProgress": False })

#@<> reconnect to the user with full privilages, restore test user account
setup_session()
create_user()

#@<> An error should occur when dumping using oci+os://
EXPECT_FAIL("ValueError", "Directory handling for oci+os protocol is not supported.", types_schema, types_schema_tables, 'oci+os://sakila')

#@<> WL13804-TSFR_9_1_2
tested_schema = test_schema
tested_table = "tsfr_9_1_2"

session.run_sql("""CREATE TABLE !.! (
    ID INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    SMALLINT_FIELD SMALLINT DEFAULT 1,
    DECIMAL_FIELD DECIMAL DEFAULT 1.1,
    NUMERIC_FIELD NUMERIC DEFAULT 5,
    FLOAT_FIELD FLOAT /*!80013 DEFAULT (RAND() * RAND())*/,
    DOUBLEPRECISION_FIELD DOUBLE PRECISION,
    REAL_FIELD REAL,
    BIT_FIELD BIT,
    TINYINT_FIELD TINYINT,
    BOOLEAN_FIELD BOOLEAN,
    MEDIUMFIELD_FIELD MEDIUMINT,
    BIGINT_FIELD BIGINT,
    DOUBLE_FIELD DOUBLE
);""", [ tested_schema, tested_table ])

session.run_sql("""INSERT INTO !.! VALUES (
    1,
    -32768,
    5.3,
    10.5,
    20.56,
    50.389,
    500.345,
    b'1',
    -128,
    true,
    8388607,
    838860783886078388,
    8388607.8388607
);""", [ tested_schema, tested_table ])

EXPECT_SUCCESS(tested_schema, [tested_table], test_output_absolute, { "chunking": False, "compression": "none", "showProgress": False })
TEST_LOAD(tested_schema, tested_table)

session.run_sql("DROP TABLE !.!;", [ tested_schema, tested_table ])

#@<> WL13804-TSFR_9_1_3
tested_schema = test_schema
tested_table = "tsfr_9_1_3"

session.run_sql("""CREATE TABLE !.! (
    ID INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    DATE_FIELD DATE /*!80013 DEFAULT (CURRENT_DATE + INTERVAL 1 YEAR)*/,
    TIME_FIELD TIME /*!80013 DEFAULT NOW()*/ DEFAULT '2000-01-01 01:01:01',
    DATETIME_FIELD DATETIME,
    TIMESTAMP_FIELD TIMESTAMP DEFAULT '2000-01-01 01:01:01',
    YEAR_FIELD YEAR,
    CONSTRAINT CHECK_TSFR_9_1_3 CHECK(YEAR_FIELD > YEAR("1900-01-01"))
);""", [ tested_schema, tested_table ])

session.run_sql("""INSERT INTO !.! VALUES (
    1,
    NOW(),
    NOW(),
    NOW(),
    NOW(),
    NOW()
);""", [ tested_schema, tested_table ])

EXPECT_SUCCESS(tested_schema, [tested_table], test_output_absolute, { "chunking": False, "compression": "none", "showProgress": False })
TEST_LOAD(tested_schema, tested_table)

session.run_sql("DROP TABLE !.!;", [ tested_schema, tested_table ])

#@<> WL13804-TSFR_9_1_4
tested_schema = test_schema
tested_table = "tsfr_9_1_4"

session.run_sql("""CREATE TABLE !.! (
    ID INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    VARCHAR_FIELD VARCHAR(20) CHARACTER SET utf8,
    TEXT_FIELD TEXT CHARACTER SET latin1 COLLATE latin1_general_cs,
    ENUM_FIELD ENUM('a','b','c') CHARACTER SET binary,
    CHAR_FIELD CHAR(5),
    BINARY_FIELD BINARY /*!80013 DEFAULT (UUID_TO_BIN(UUID()))*/,
    TINYBLOB_FIELD TINYBLOB,
    VARBINARY_FIELD VARBINARY(100),
    TINYTEXT_FIELD TINYTEXT,
    BLOB_FIELD BLOB,
    MEDIUMBLOB_FIELD MEDIUMBLOB,
    MEDIUMTEXT_FIELD MEDIUMTEXT CHARACTER SET latin1 COLLATE latin1_general_cs,
    LONGBLOB_FIELD LONGBLOB,
    LONGTEXT_FIELD LONGTEXT,
    SET_FIELD SET('a', 'b', 'c')
);""", [ tested_schema, tested_table ])

session.run_sql("""INSERT INTO !.! VALUES (
    1,
    "TEST",
    "TEST",
    "a",
    "XYZ",
    b'1',
    LOAD_FILE("{0}"),
    0x12345,
    "2",
    LOAD_FILE("{0}"),
    LOAD_FILE("{0}"),
    "test",
    LOAD_FILE("{0}"),
    "testtesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttest",
    "a"
);""".format(os.path.join(__data_path, "sql", "fieldtypes_all.sql")), [ tested_schema, tested_table ])

EXPECT_SUCCESS(tested_schema, [tested_table], test_output_absolute, { "chunking": False, "compression": "none", "showProgress": False })
TEST_LOAD(tested_schema, tested_table)

session.run_sql("DROP TABLE !.!;", [ tested_schema, tested_table ])

#@<> WL13804-TSFR_9_1_5
tested_schema = test_schema
tested_table = "tsfr_9_1_5"

session.run_sql("""CREATE TABLE !.! (
    ID INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    GEOMETRY_FIELD GEOMETRY NOT NULL /*!80000 SRID 0*/,
    POINT_FIELD POINT /*!80013 DEFAULT (POINT(0,0))*/,
    LINESTRING_FIELD LINESTRING,
    POLYGON_FIELD POLYGON,
    MULTIPOINT_FIELD MULTIPOINT,
    MULTILINESTRING_FIELD MULTILINESTRING,
    MULTIPOLYGON_FIELD MULTIPOLYGON,
    GEOMETRYCOLLECTION_FIELD GEOMETRYCOLLECTION,
    JSON_FIELD JSON /*!80013 DEFAULT (JSON_ARRAY())*/
);""", [ tested_schema, tested_table ])

session.run_sql("""INSERT INTO !.! VALUES (
    1,
    ST_GeomFromText('POINT(1 1)'),
    ST_PointFromText('POINT(1 1)'),
    ST_LineStringFromText('LINESTRING(0 0,1 1,2 2)'),
    ST_PolygonFromText('POLYGON((0 0,10 0,10 10,0 10,0 0),(5 5,7 5,7 7,5 7, 5 5))'),
    ST_MPointFromText('MULTIPOINT (1 1, 2 2, 3 3)'),
    ST_GeomFromText('MULTILINESTRING((10 10, 20 20), (15 15, 30 15))'),
    ST_GeomFromText('MULTIPOLYGON(((0 0,10 0,10 10,0 10,0 0)),((5 5,7 5,7 7,5 7, 5 5)))'),
    ST_GeomFromText('GEOMETRYCOLLECTION(POINT(10 10), POINT(30 30), LINESTRING(15 15, 20 20))'),
    '{"k1": "value", "k2": 10}'
);""", [ tested_schema, tested_table ])

EXPECT_SUCCESS(tested_schema, [tested_table], test_output_absolute, { "chunking": False, "compression": "none", "showProgress": False })
TEST_LOAD(tested_schema, tested_table)

session.run_sql("DROP TABLE !.!;", [ tested_schema, tested_table ])

#@<> WL13804-TSFR_9_1_6
tested_schema = test_schema
tested_table = "temp_tsfr_9_1_6"

session.run_sql("""CREATE TEMPORARY TABLE !.! (
    ID INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    GEOMETRY_FIELD GEOMETRY NOT NULL /*!80000 SRID 0*/,
    VARCHAR_FIELD VARCHAR(20) CHARACTER SET utf8,
    DATE_FIELD DATE /*!80013 DEFAULT (CURRENT_DATE + INTERVAL 1 YEAR)*/,
    SMALLINT_FIELD SMALLINT DEFAULT 1
);""", [ tested_schema, tested_table ])

session.run_sql("""INSERT INTO !.! VALUES(
    1,
    ST_GeomFromText('POINT(1 1)'),
     "TEST",
     NOW(),
     -32768
);""", [ tested_schema, tested_table ])

EXPECT_FAIL("ValueError", "Following tables were not found in the schema '{0}': '{1}'".format(tested_schema, tested_table), tested_schema, [tested_table], test_output_absolute)

session.run_sql("DROP TABLE !.!;", [ tested_schema, tested_table ])

#@<> WL13804-TSFR_9_1_7
tested_schema = test_schema
tested_table = "tsfr_9_1_7"

session.run_sql("""CREATE TABLE !.! (
    firstname VARCHAR(25) NOT NULL,
    lastname VARCHAR(25) NOT NULL,
    username VARCHAR(16) NOT NULL,
    email VARCHAR(35),
    joined DATE NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='HELLO WORLD'
PARTITION BY RANGE( YEAR(joined) ) (
    PARTITION p0 VALUES LESS THAN (1960),
    PARTITION p1 VALUES LESS THAN (1970),
    PARTITION p2 VALUES LESS THAN (1980),
    PARTITION p3 VALUES LESS THAN (1990),
    PARTITION p4 VALUES LESS THAN MAXVALUE
);""", [ tested_schema, tested_table ])

session.run_sql("""INSERT INTO !.! VALUES (
    "firstname", "lastname", "username", "email", NOW()
);""", [ tested_schema, tested_table ])

EXPECT_SUCCESS(tested_schema, [tested_table], test_output_absolute, { "chunking": False, "compression": "none", "showProgress": False })
TEST_LOAD(tested_schema, tested_table)

session.run_sql("DROP TABLE !.!;", [ tested_schema, tested_table ])

#@<> WL13804-TSFR_9_1_8
tested_schema = test_schema
tested_view = "tsfr_9_1_8_view"

session.run_sql("""CREATE TABLE !.TSFR_9_1_8 (
    firstname VARCHAR(25) NOT NULL,
    lastname VARCHAR(25) NOT NULL,
    username VARCHAR(16) NOT NULL,
    email VARCHAR(35),
    joined DATE NOT NULL
);""", [ tested_schema ])

session.run_sql("""INSERT INTO !.TSFR_9_1_8 VALUES ("firstname", "lastname", "username", "email", NOW());""", [ tested_schema ])
session.run_sql("""INSERT INTO !.TSFR_9_1_8 VALUES ("firstname", "lastname", "username", "email", DATE('2000-01-01'));""", [ tested_schema ])
session.run_sql("""INSERT INTO !.TSFR_9_1_8 VALUES ("firstname", "lastname", "username", "email", DATE('1990-01-01'));""", [ tested_schema ])
session.run_sql("""INSERT INTO !.TSFR_9_1_8 VALUES ("firstname", "lastname", "username", "email", DATE('1980-01-01'));""", [ tested_schema ])
session.run_sql("""INSERT INTO !.TSFR_9_1_8 VALUES ("firstname", "lastname", "username", "email", DATE('1910-01-01'));""", [ tested_schema ])

session.run_sql("""CREATE VIEW !.! AS (
    SELECT * FROM !.TSFR_9_1_8 WHERE joined > DATE('1980-01-01') ORDER BY JOINED DESC
);""", [ tested_schema, tested_view, tested_schema ])

EXPECT_SUCCESS(tested_schema, [], test_output_absolute, { "chunking": False, "compression": "none", "showProgress": False }, [tested_view])

session.run_sql("DROP TABLE !.TSFR_9_1_8;", [ tested_schema ])
session.run_sql("DROP VIEW !.!;", [ tested_schema, tested_view ])

#@<> WL13804-TSFR_10_1_2 {__os_type != "windows"}
# create the directory
shutil.rmtree(test_output_absolute, True)
os.mkdir(test_output_absolute)
# change permissions to write only
os.chmod(test_output_absolute, stat.S_IWUSR | stat.S_IXUSR | stat.S_IWGRP | stat.S_IXGRP | stat.S_IWOTH | stat.S_IXOTH)
# expect failure
EXPECT_THROWS(lambda: util.dump_tables(types_schema, [types_schema_tables[0]], test_output_absolute, { "showProgress": False }), "RuntimeError: Util.dump_tables: {0}: Permission denied".format(test_output_absolute))
# remove the directory
os.chmod(test_output_absolute, stat.S_IRWXU)
shutil.rmtree(test_output_absolute, True)

#@<> WL13804-TSFR_10_1_3 {__os_type != "windows"}
# create the directory
shutil.rmtree(test_output_absolute, True)
os.mkdir(test_output_absolute)
# change permissions to write only
os.chmod(test_output_absolute, stat.S_IRUSR | stat.S_IXUSR | stat.S_IRGRP | stat.S_IXGRP | stat.S_IROTH | stat.S_IXOTH)
# expect failure
EXPECT_THROWS(lambda: util.dump_tables(types_schema, [types_schema_tables[0]], test_output_absolute, { "showProgress": False }), "Cannot open file '{0}': Permission denied".format(absolute_path_for_output(os.path.join(test_output_absolute, "@.json"))))
# remove the directory
os.chmod(test_output_absolute, stat.S_IRWXU)
shutil.rmtree(test_output_absolute, True)

#@<> WL13804-TSFR_10_1_4
for d in ["directory", "directory_with_ space", "", '"' if __os_type != "windows" else "x", "'", "-", "--"]:
    d = os.path.join(test_output_absolute, d)
    print("----> testing {0}".format(d))
    shutil.rmtree(test_output_absolute, True)
    os.makedirs(os.path.dirname(d))
    EXPECT_NO_THROWS(lambda: util.dump_tables(types_schema, [types_schema_tables[0]], d, { "showProgress": False }), "WL13804-TSFR_10_1_4: {0}".format(d))
    EXPECT_TRUE(os.path.isdir(d))

for d in ["directory", "", os.path.join("deeply", "nested", "directory"), os.path.join("loooooooooooooooooooong", "looooooong", "name")]:
    d = os.path.join(test_output_absolute, d)
    for prefix in ["file", "FiLe", "FILE"]:
        prefix = prefix + "://" + d
        print("----> testing {0}".format(prefix))
        shutil.rmtree(test_output_absolute, True)
        os.makedirs(os.path.dirname(d))
        EXPECT_NO_THROWS(lambda: util.dump_tables(types_schema, [types_schema_tables[0]], prefix, { "showProgress": False }), "WL13804-TSFR_10_1_4: {0}".format(prefix))
        EXPECT_TRUE(os.path.isdir(d))

#@<> WL13804-TSFR_10_1_6 {__os_type != "windows"}
# create the directory
tested_dir = os.path.join(test_output_absolute, "test")
shutil.rmtree(test_output_absolute, True)
os.makedirs(tested_dir)
# create the symlink
tested_symlink = os.path.join(test_output_absolute, "symlink")
os.symlink(tested_dir, tested_symlink)
# expect success
EXPECT_NO_THROWS(lambda: util.dump_tables(types_schema, [types_schema_tables[0]], tested_symlink, { "showProgress": False }), "WL13804-TSFR_10_1_6")

#@<> WL13804-TSFR_10_1_8 {__os_type != "windows"}
# create the directory
tested_dir = os.path.join(test_output_absolute, "test")
shutil.rmtree(test_output_absolute, True)
os.makedirs(tested_dir)
# change permissions to read only
os.chmod(tested_dir, stat.S_IRUSR | stat.S_IXUSR | stat.S_IRGRP | stat.S_IXGRP | stat.S_IROTH | stat.S_IXOTH)
# expect failure
EXPECT_THROWS(lambda: util.dump_tables(types_schema, [types_schema_tables[0]], os.path.join(tested_dir, "test")), "RuntimeError: Util.dump_tables: Could not create directory {0}: Permission denied".format(absolute_path_for_output(os.path.join(tested_dir, "test"))))
# remove the directory
os.chmod(tested_dir, stat.S_IRWXU)
shutil.rmtree(tested_dir, True)

#@<> WL13804-TSFR_11_2_7
tested_schema = "test"
tested_tables = [ "test", "test2" ]

# create the test environment
EXPECT_EQ(0, testutil.call_mysqlsh([uri, "--sql", "--file", os.path.join(__data_path, "sql", "create_triggers.sql")]))

# dump the tables
EXPECT_SUCCESS(tested_schema, tested_tables, test_output_absolute, { "triggers": True, "showProgress": False })

# load the dump
recreate_verification_schema()
EXPECT_NO_THROWS(lambda: util.load_dump(test_output_absolute, { "schema": verification_schema, "showProgress": False }), "WL13804-TSFR_11_2_7")

# expect data to be correct
for table in tested_tables:
    print("---> checking: `{0}`.`{1}`".format(verification_schema, table))
    all_columns = get_all_columns(tested_schema, table)
    EXPECT_EQ(compute_crc(tested_schema, table, all_columns), compute_crc(verification_schema, table, all_columns))

# check if all triggers were dumped and loaded
expected_triggers = sorted([x[0] for x in session.run_sql("SELECT TRIGGER_NAME from information_schema.triggers WHERE EVENT_OBJECT_SCHEMA = ? AND EVENT_OBJECT_TABLE = ?;", [ tested_schema, tested_tables[0] ]).fetch_all()])
actual_triggers = sorted([x[0] for x in session.run_sql("SELECT TRIGGER_NAME from information_schema.triggers WHERE EVENT_OBJECT_SCHEMA = ? AND EVENT_OBJECT_TABLE = ?;", [ verification_schema, tested_tables[0] ]).fetch_all()])
EXPECT_EQ(expected_triggers, actual_triggers)

# check if the loaded triggers are correct
for column in ["SQL_MODE", "CHARACTER_SET_CLIENT", "COLLATION_CONNECTION", "EVENT_MANIPULATION", "ACTION_ORDER", "ACTION_CONDITION", "ACTION_STATEMENT", "ACTION_ORIENTATION", "ACTION_TIMING", "ACTION_REFERENCE_OLD_TABLE", "ACTION_REFERENCE_NEW_TABLE", "ACTION_REFERENCE_OLD_ROW", "ACTION_REFERENCE_NEW_ROW", "DEFINER", "DATABASE_COLLATION"]:
    for trigger in expected_triggers:
        print("---> checking: trigger {0}, column: {1}".format(trigger, column))
        expected = session.run_sql("SELECT ! from information_schema.triggers WHERE EVENT_OBJECT_SCHEMA = ? AND EVENT_OBJECT_TABLE = ? AND TRIGGER_NAME = ?;", [ column, tested_schema, tested_tables[0], trigger ]).fetch_one()[0]
        actual = session.run_sql("SELECT ! from information_schema.triggers WHERE EVENT_OBJECT_SCHEMA = ? AND EVENT_OBJECT_TABLE = ? AND TRIGGER_NAME = ?;", [ column, verification_schema, tested_tables[0], trigger ]).fetch_one()[0]
        EXPECT_EQ(expected, actual)

# grant test user privileges required to run the test, open a session for the test user, verify if triggers are working ok
session.run_sql("GRANT SELECT, INSERT, UPDATE, DELETE ON !.* TO test@'%'", [verification_schema])

test_session = shell.open_session("mysql://{0}:{1}@{2}:{3}".format("test", "test", __host, __mysql_sandbox_port1))

test_session.run_sql("INSERT INTO !.! (id) VALUES (1);", [ verification_schema, tested_tables[1] ])
test_session.run_sql("INSERT INTO !.! (col) VALUES ('First'), ('Second'), ('Third');", [ verification_schema, tested_tables[0] ])
test_session.run_sql("UPDATE !.! SET col = 'Fourth' WHERE col = 'Third';", [ verification_schema, tested_tables[0] ])
EXPECT_THROWS(lambda: test_session.run_sql("DELETE FROM !.! WHERE col = 'Fourth';", [ verification_schema, tested_tables[0] ]), "ClassicSession.run_sql: TRIGGER command denied to user 'test'@'{0}' for table '{1}'".format(__host, tested_tables[0]))
EXPECT_EQ([ 1, 3, 1, 0, 3, 2, 0, "" ], [x for x in test_session.run_sql("SELECT * FROM !.!;", [ verification_schema, tested_tables[1] ]).fetch_one()])

# drop the trigger using root
session.run_sql("DROP TRIGGER !.t9;", [ verification_schema ])

test_session.run_sql("DELETE FROM !.! WHERE col = 'Fourth';", [ verification_schema, tested_tables[0] ])
EXPECT_EQ([ 1, 3, 1, 1, 3, 2, 4, " t7 t10 t8 t6" ], [x for x in test_session.run_sql("SELECT * FROM !.!;", [ verification_schema, tested_tables[1] ]).fetch_one()])

# cleanup
EXPECT_EQ(0, testutil.call_mysqlsh([uri, "--sql", "--file", os.path.join(__data_path, "sql", "create_triggers_cleanup.sql")]))
test_session.close()

#@<> WL13804-TSFR_11_2_20
tested_schema = "test_schema"
tested_table = "test"

session.run_sql("CREATE SCHEMA !;", [ tested_schema ])
session.run_sql("CREATE TABLE !.! (id INT);", [ tested_schema, tested_table ])
session.run_sql("INSERT INTO !.! VALUES (1), (2), (3);", [ tested_schema, tested_table ])

EXPECT_SUCCESS(tested_schema, [tested_table], test_output_absolute, { "chunking": True, "showProgress": False })
EXPECT_STDOUT_CONTAINS("NOTE: Could not select columns to be used as an index for table `{0}`.`{1}`. Chunking has been disabled for this table, data will be dumped to a single file.".format(tested_schema, tested_table))
EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(tested_schema, tested_table) + ".tsv.zst")))

session.run_sql("DROP SCHEMA !;", [ tested_schema ])

#@<> WL13804-TSFR_11_2_21
tested_schema = "test_schema"
tested_table = "test"

session.run_sql("CREATE SCHEMA !;", [ tested_schema ])
session.run_sql("CREATE TABLE !.! (id INT NOT NULL, PRIMARY KEY (id DESC));", [ tested_schema, tested_table ])
session.run_sql("INSERT INTO !.! VALUES (1), (2), (3);", [ tested_schema, tested_table ])

EXPECT_SUCCESS(tested_schema, [tested_table], test_output_absolute, { "chunking": True, "showProgress": False })
EXPECT_TRUE(has_file_with_basename(test_output_absolute, encode_table_basename(tested_schema, tested_table) + "@"))

session.run_sql("DROP SCHEMA !;", [ tested_schema ])

#@<> WL13804-TSFR_11_2_22
tested_schema = "test_schema"
tested_table = "test"

session.run_sql("CREATE SCHEMA !;", [ tested_schema ])
session.run_sql("CREATE TABLE !.! (a INT, b CHAR(3), c INT, PRIMARY KEY (a, b));", [ tested_schema, tested_table ])
session.run_sql("INSERT INTO !.! VALUES (1, 'a', 1), (2, 'b', 2), (3, 'c', 3);", [ tested_schema, tested_table ])

EXPECT_SUCCESS(tested_schema, [tested_table], test_output_absolute, { "chunking": True, "showProgress": False })
EXPECT_TRUE(has_file_with_basename(test_output_absolute, encode_table_basename(tested_schema, tested_table) + "@"))

session.run_sql("DROP SCHEMA !;", [ tested_schema ])

#@<> WL13804-TSFR_11_2_23
tested_schema = "test_schema"
tested_table = "test"

session.run_sql("CREATE SCHEMA !;", [ tested_schema ])
session.run_sql("CREATE TABLE !.! (a INT, b INT, UNIQUE KEY b(b));", [ tested_schema, tested_table ])
session.run_sql("INSERT INTO !.! VALUES (1, 1), (2, 2), (3, 3);", [ tested_schema, tested_table ])

EXPECT_SUCCESS(tested_schema, [tested_table], test_output_absolute, { "chunking": True, "showProgress": False })
EXPECT_TRUE(has_file_with_basename(test_output_absolute, encode_table_basename(tested_schema, tested_table) + "@"))

session.run_sql("DROP SCHEMA !;", [ tested_schema ])

#@<> WL13804-TSFR_11_2_24
tested_schema = "test_schema"
tested_table = "test"

session.run_sql("CREATE SCHEMA !;", [ tested_schema ])
session.run_sql("CREATE TABLE !.! (a INT, b INT, UNIQUE KEY ba(b, a), UNIQUE KEY ab(a,b));", [ tested_schema, tested_table ])
session.run_sql("INSERT INTO !.! VALUES (1, 1), (2, 2), (3, 3);", [ tested_schema, tested_table ])

EXPECT_SUCCESS(tested_schema, [tested_table], test_output_absolute, { "chunking": True, "showProgress": False })
EXPECT_TRUE(has_file_with_basename(test_output_absolute, encode_table_basename(tested_schema, tested_table) + "@"))

session.run_sql("DROP SCHEMA !;", [ tested_schema ])

#@<> WL13804-TSFR_11_2_25
tested_schema = "test_schema"
tested_table = "test"

session.run_sql("CREATE SCHEMA !;", [ tested_schema ])
session.run_sql("CREATE TABLE !.! (a INT, b INT, UNIQUE KEY ba(b, a), UNIQUE KEY ab(a,b), PRIMARY KEY (a));", [ tested_schema, tested_table ])
session.run_sql("INSERT INTO !.! VALUES (1, 1), (2, 2), (3, 3);", [ tested_schema, tested_table ])

EXPECT_SUCCESS(tested_schema, [tested_table], test_output_absolute, { "chunking": True, "showProgress": False })
EXPECT_TRUE(has_file_with_basename(test_output_absolute, encode_table_basename(tested_schema, tested_table) + "@"))

session.run_sql("DROP SCHEMA !;", [ tested_schema ])

#@<> WL13804-TSFR_11_2_26
tested_schema = "test_schema"
tested_table = "test"

session.run_sql("CREATE SCHEMA !;", [ tested_schema ])
session.run_sql("CREATE TABLE !.! (a INT, b INT, UNIQUE KEY b(b DESC));", [ tested_schema, tested_table ])
session.run_sql("INSERT INTO !.! VALUES (1, 1), (2, 2), (3, 3);", [ tested_schema, tested_table ])

EXPECT_SUCCESS(tested_schema, [tested_table], test_output_absolute, { "chunking": True, "showProgress": False })
EXPECT_TRUE(has_file_with_basename(test_output_absolute, encode_table_basename(tested_schema, tested_table) + "@"))

session.run_sql("DROP SCHEMA !;", [ tested_schema ])

#@<> WL13804-TSFR_12_1
tested_schema = "test_schema"
tested_table = "test"

session.run_sql("CREATE SCHEMA !;", [ tested_schema ])
session.run_sql("CREATE TABLE !.! (a INT PRIMARY KEY, b VARCHAR(32), c VARCHAR(32));", [ tested_schema, tested_table ])
session.run_sql("INSERT INTO !.! VALUES (1, 'qwerty\tqwerty', 'dummy\ndummy'), (2, 'abc\ndef', 'ghi\txyz');", [ tested_schema, tested_table ])

EXPECT_SUCCESS(tested_schema, [tested_table], test_output_absolute, { "chunking": False, "compression": "none", "showProgress": False })
EXPECT_FILE_CONTAINS("1\tqwerty\\tqwerty\tdummy\\ndummy\n", os.path.join(test_output_absolute, encode_table_basename(tested_schema, tested_table) + ".tsv"))
EXPECT_FILE_CONTAINS("2\tabc\\ndef\tghi\\txyz\n", os.path.join(test_output_absolute, encode_table_basename(tested_schema, tested_table) + ".tsv"))

session.run_sql("DROP SCHEMA !;", [ tested_schema ])

#@<> BUG#31896448
tested_schema = "test_schema"
tested_table = "test"

session.run_sql("CREATE SCHEMA !;", [ tested_schema ])
session.run_sql("CREATE TABLE !.! (a BIGINT PRIMARY KEY, b VARCHAR(32));", [ tested_schema, tested_table ])
session.run_sql("""INSERT INTO !.! VALUES
(-9223372036854775808, 'a'),
(-6917529027641081856, 'b'),
(-4611686018427387904, 'c'),
(-2305843009213693952, 'd'),
(0, 'e'),
(2305843009213693952, 'f'),
(4611686018427387904, 'g'),
(6917529027641081856, 'h'),
(9223372036854775807, 'i');""", [ tested_schema, tested_table ])
session.run_sql("ANALYZE TABLE !.!;", [ tested_schema, tested_table ])

EXPECT_SUCCESS(tested_schema, [ tested_table ], test_output_absolute, { "bytesPerChunk": "128k", "compression": "none", "showProgress": False })
TEST_LOAD(tested_schema, tested_table)

session.run_sql("DROP SCHEMA !;", [ tested_schema ])

#@<> BUG#31766490 {not __dbug_off}
tested_schema = "test_schema"
tested_table = "test"

session.run_sql("CREATE SCHEMA !;", [ tested_schema ])
session.run_sql("CREATE TABLE !.! (c INT PRIMARY KEY);", [ tested_schema, tested_table ])
session.run_sql("INSERT INTO !.! VALUES " + ','.join("(" + str(k) + ")" for k in range(1024)), [ tested_schema, tested_table ])

expected_msg = "NOTE: Table statistics not available for `{0}`.`{1}`, chunking operation may be not optimal. Please consider running 'ANALYZE TABLE `{0}`.`{1}`;' first.".format(tested_schema, tested_table)

testutil.dbug_set("+d,dumper_average_row_length_0")

EXPECT_SUCCESS(tested_schema, [ tested_table ], test_output_absolute, { "showProgress": False })
EXPECT_STDOUT_CONTAINS(expected_msg)

testutil.dbug_set("")

session.run_sql("ANALYZE TABLE !.!;", [ tested_schema, tested_table ])

WIPE_STDOUT()
EXPECT_SUCCESS(tested_schema, [ tested_table ], test_output_absolute, { "showProgress": False })
EXPECT_STDOUT_NOT_CONTAINS(expected_msg)

session.run_sql("DROP SCHEMA !;", [ tested_schema ])

#@<> BUG#32430402 metadata should contain information about binlog
EXPECT_SUCCESS(types_schema, [types_schema_tables[0]], test_output_absolute, { "ddlOnly": True, "showProgress": False })

with open(os.path.join(test_output_absolute, "@.json"), encoding="utf-8") as json_file:
    metadata = json.load(json_file)
    EXPECT_EQ(True, "binlogFile" in metadata, "'binlogFile' should be in metadata")
    EXPECT_EQ(True, "binlogPosition" in metadata, "'binlogPosition' should be in metadata")

#@<> BUG#32773468 setup
def validate_size(path, ext):
    size = 0
    for f in os.listdir(path):
        if f.endswith(ext):
            size += os.path.getsize(os.path.join(path, f))
    EXPECT_STDOUT_CONTAINS(f"Compressed data size: {size} bytes")

def test_bug_32773468(data):
    session.run_sql("TRUNCATE TABLE !.!;", [ tested_schema, tested_table ])
    session.run_sql(f"""INSERT INTO !.! VALUES {",".join([f"({v}, '{random.choices(string.ascii_letters + string.digits)[0]}')" for v in data])};""", [ tested_schema, tested_table ])
    session.run_sql("ANALYZE TABLE !.!;", [ tested_schema, tested_table ])
    EXPECT_SUCCESS(tested_schema, [ tested_table ], test_output_absolute, { "showProgress": False })
    validate_size(test_output_absolute, ".zst")

tested_schema = "test_schema"
session.run_sql("CREATE SCHEMA !;", [ tested_schema ])

#@<> BUG#32773468 setup table with signed integers
tested_table = "test_signed"
session.run_sql("CREATE TABLE !.! (a BIGINT PRIMARY KEY, b VARCHAR(32));", [ tested_schema, tested_table ])

#@<> BUG#32773468 table holds values close to signed minimum
test_bug_32773468(range(-9223372036854775808, -9223372036854775802))

#@<> BUG#32773468 table holds values close to signed minimum, data range overflows the max
test_bug_32773468(list(range(-9223372036854775808, -9223372036854775802)) + [ 1 ])

#@<> BUG#32773468 table holds values close to signed maximum
test_bug_32773468(range(9223372036854775802, 9223372036854775808))

#@<> BUG#32773468 table holds values close to signed maximum, data range overflows the max
test_bug_32773468([ -1 ] + list(range(9223372036854775802, 9223372036854775808)))

#@<> BUG#32773468 setup table with unsigned integers
tested_table = "test_unsigned"
session.run_sql("CREATE TABLE !.! (a BIGINT UNSIGNED PRIMARY KEY, b VARCHAR(32));", [ tested_schema, tested_table ])

#@<> BUG#32773468 table holds values close to unsigned minimum
test_bug_32773468(range(0, 6))

#@<> BUG#32773468 data range is maximum possible
test_bug_32773468(list(range(0, 6)) + [ 18446744073709551615 ])

#@<> BUG#32773468 table holds values close to unsigned maximum
test_bug_32773468(range(18446744073709551610, 18446744073709551616))

#@<> BUG#32773468 cleanup
session.run_sql("DROP SCHEMA !;", [ tested_schema ])

#@<> BUG#32602325 setup
tested_schema = "test_schema"
tested_table = "test_table"

session.run_sql("CREATE SCHEMA !;", [ tested_schema ])
session.run_sql("CREATE TABLE !.! (a BIGINT PRIMARY KEY, b text);", [ tested_schema, tested_table ])

gap_start = 1
gap_items = 10000
gap_step = 1000

def test_bug_32602325(step):
    def insert(r):
        session.run_sql(f"""INSERT INTO !.! VALUES {",".join([f"({v}, '')" for v in r])};""", [ tested_schema, tested_table ])
    def generate_gaps():
        value = gap_start
        while True:
            yield value
            value += next(step)
    gen = generate_gaps()
    # clear the data
    session.run_sql("TRUNCATE TABLE !.!;", [ tested_schema, tested_table ])
    # insert rows with gaps in the PK
    insert([next(gen) for i in range(gap_items)])
    # all the remaining data is continuous
    start = next(gen)
    insert(range(start, start + gap_items))
    # use some dummy data
    session.run_sql("UPDATE !.! SET b = repeat('x', 5000);", [ tested_schema, tested_table ])
    # analyze the table for optimum results
    session.run_sql("ANALYZE TABLE !.!;", [ tested_schema, tested_table ])
    # run the test
    EXPECT_SUCCESS(tested_schema, [ tested_table ], test_output_absolute, { "bytesPerChunk": "1M", "compression": "none", "showProgress": False })
    # expect at least 320 chunks, we're dealing with random data, allow for some chunks which are smaller
    CHECK_OUTPUT_SANITY(test_output_absolute, 200000, 320, 4)

#@<> BUG#32602325 - equal gaps
def equal_gaps():
    while True:
        yield gap_step

test_bug_32602325(equal_gaps())

#@<> BUG#32602325 - random gaps
def random_gaps():
    while True:
        yield random.randrange(1, gap_step)

test_bug_32602325(random_gaps())

#@<> BUG#32602325 - increasing gaps
def increasing_gaps():
    step = 1
    while True:
        yield step
        step += 1

test_bug_32602325(increasing_gaps())

#@<> BUG#32602325 - decreasing gaps
def decreasing_gaps():
    step = gap_step
    while True:
        yield step
        if step > 2:
            step -= 1

test_bug_32602325(decreasing_gaps())

#@<> BUG#32602325 cleanup
session.run_sql("DROP SCHEMA !;", [ tested_schema ])

#@<> composite non-integer key - setup
tested_schema = "test_schema"
tested_table = "char_hashes_4"
items = 10000

session.run_sql("CREATE SCHEMA !;", [ tested_schema ])
session.run_sql("""CREATE TABLE !.! (
  `md5_1` varchar(8) NOT NULL,
  `md5_2` varchar(8) NOT NULL,
  `md5_3` varchar(8) GENERATED ALWAYS AS (substr(md5(email), 17, 8)) VIRTUAL NOT NULL,
  `md5_4` varchar(8) GENERATED ALWAYS AS (substr(md5(email), 25, 8)) STORED NOT NULL,
  `email` varchar(100) DEFAULT NULL,
  UNIQUE KEY `pk` (`md5_1`,`md5_2`,`md5_3`,`md5_4`)
);""", [ tested_schema, tested_table ])
session.run_sql(f"""INSERT INTO !.! (`md5_1`,`md5_2`, `email`) VALUES {",".join([f"('{md5sum(email)[0:8]}', '{md5sum(email)[8:16]}', '{email}')" for email in [random_email() for i in range(items)]])};""", [ tested_schema, tested_table ])
session.run_sql("ANALYZE TABLE !.!;", [ tested_schema, tested_table ])

#@<> composite non-integer key - test
WIPE_SHELL_LOG()
EXPECT_SUCCESS(tested_schema, [ tested_table ], test_output_absolute, { "bytesPerChunk": "128k", "compression": "none", "showProgress": False })
EXPECT_SHELL_LOG_CONTAINS(f"Data dump for table `{tested_schema}`.`{tested_table}` will be chunked using columns `md5_1`, `md5_2`, `md5_3`, `md5_4`")
CHECK_OUTPUT_SANITY(test_output_absolute, 55000, 10)
TEST_LOAD(tested_schema, tested_table)

#@<> composite non-integer key - cleanup
session.run_sql("DROP SCHEMA !;", [ tested_schema ])

#@<> BUG#32926856 setup
tested_schema = "test_schema"
tested_table = "test_table"

session.run_sql("CREATE SCHEMA !;", [ tested_schema ])
session.run_sql("SET @saved_sql_mode = @@SQL_MODE;")
session.run_sql("SET SQL_MODE='no_auto_value_on_zero';")
session.run_sql("CREATE TABLE !.! (a INT PRIMARY KEY AUTO_INCREMENT);", [ tested_schema, tested_table ])
session.run_sql("INSERT INTO !.! VALUES (0), (1);", [ tested_schema, tested_table ])
session.run_sql("SET @@SQL_MODE = @saved_sql_mode;")

checksum = compute_checksum(tested_schema, tested_table)

#@<> BUG#32926856 - test
EXPECT_SUCCESS(tested_schema, [ tested_table ], test_output_absolute, { "showProgress": False })

session.run_sql("DROP SCHEMA !;", [ tested_schema ])
EXPECT_NO_THROWS(lambda: util.load_dump(test_output_absolute), "dump should be loaded without problems")

EXPECT_EQ(checksum, compute_checksum(tested_schema, tested_table), "checksum mismatch")

#@<> BUG#32926856 cleanup
session.run_sql("DROP SCHEMA !;", [ tested_schema ])

#@<> BUG#33232480 setup
tested_schema = "test_schema"
tested_table = "test_table"

session.run_sql("CREATE SCHEMA !;", [ tested_schema ])
session.run_sql("CREATE TABLE !.! (`c (from, to)` varchar(16) NOT NULL, `c (to, from)` varchar(16) NOT NULL, `data` blob, PRIMARY KEY (`c (from, to)`, `c (to, from)`));", [ tested_schema, tested_table ])
session.run_sql("INSERT INTO !.! (`c (from, to)`, `c (to, from)`, `data`) VALUES (REPEAT('a',15), REPEAT('a',15), REPEAT('a',115));", [ tested_schema, tested_table ])
session.run_sql("INSERT INTO !.! (`c (from, to)`, `c (to, from)`, `data`) VALUES (REPEAT('z',15), REPEAT('z',15), REPEAT('z',115));", [ tested_schema, tested_table ])
session.run_sql("ANALYZE TABLE !.!;", [ tested_schema, tested_table ])

#@<> BUG#33232480 - test
EXPECT_SUCCESS(tested_schema, [ tested_table ], test_output_absolute, { "showProgress": False })

#@<> BUG#33232480 cleanup
session.run_sql("DROP SCHEMA !;", [ tested_schema ])

#@<> BUG#32954757 setup
tested_schema = "test_schema"
tested_table = "test_table"

session.run_sql("CREATE SCHEMA !;", [ tested_schema ])
session.run_sql("CREATE TABLE !.! (data INT PRIMARY KEY);", [ tested_schema, tested_table ])
for i in range(10):
    session.run_sql(f"INSERT INTO !.! VALUES (-{i});", [ tested_schema, tested_table ])
session.run_sql("ANALYZE TABLE !.!;", [ tested_schema, tested_table ])

#@<> BUG#32954757 - test
# constantly insert values to the table
insert_values = f"""v = 1
while True:
    session.run_sql("INSERT {tested_schema}.{tested_table} VALUES({{0}})".format(v))
    v = v + 1
session.close()
"""

# run a process which constantly inserts some values
pid = testutil.call_mysqlsh_async(["--py", uri], insert_values)

# wait a bit for process to start
time.sleep(1)

# run a dump, expect a warning that gtid_executed has changed
EXPECT_SUCCESS(tested_schema, [ tested_table ], test_output_absolute, { "consistent": False, "threads": 2, "showProgress": False })
EXPECT_STDOUT_CONTAINS("WARNING: The value of the gtid_executed system variable has changed during the dump, from: ")

# terminate the process immediately, it will not stop on its own
testutil.wait_mysqlsh_async(pid, 0)

#@<> BUG#32954757 cleanup
session.run_sql("DROP SCHEMA !;", [ tested_schema ])

#@<> WL14244 - help entries
util.help('dump_tables')

# WL14244-TSFR_7_3
EXPECT_STDOUT_CONTAINS("""
      - includeTriggers: list of strings (default: empty) - List of triggers to
        be included in the dump in the format of schema.table (all triggers
        from the specified table) or schema.table.trigger (the individual
        trigger).
""")

# WL14244-TSFR_8_3
EXPECT_STDOUT_CONTAINS("""
      - excludeTriggers: list of strings (default: empty) - List of triggers to
        be excluded from the dump in the format of schema.table (all triggers
        from the specified table) or schema.table.trigger (the individual
        trigger).
""")

#@<> WL14244 - helpers
def dump_and_load(options):
    WIPE_STDOUT()
    # remove everything from the server
    wipeout_server(session)
    # create sample DB structure
    session.run_sql("CREATE SCHEMA schema_1")
    session.run_sql("CREATE TABLE schema_1.existing_table_1 (id INT)")
    session.run_sql("CREATE TABLE schema_1.existing_table_2 (id INT)")
    session.run_sql("CREATE VIEW schema_1.existing_view AS SELECT * FROM schema_1.existing_table_1")
    session.run_sql("CREATE EVENT schema_1.existing_event ON SCHEDULE EVERY 1 YEAR DO BEGIN END")
    session.run_sql("CREATE FUNCTION schema_1.existing_routine() RETURNS INT DETERMINISTIC RETURN 1")
    session.run_sql("CREATE TRIGGER schema_1.existing_trigger_1 AFTER DELETE ON schema_1.existing_table_1 FOR EACH ROW BEGIN END")
    session.run_sql("CREATE TRIGGER schema_1.existing_trigger_2 AFTER DELETE ON schema_1.existing_table_2 FOR EACH ROW BEGIN END")
    session.run_sql("CREATE SCHEMA not_specified_schema")
    session.run_sql("CREATE TABLE not_specified_schema.table (id INT)")
    session.run_sql("CREATE VIEW not_specified_schema.view AS SELECT * FROM not_specified_schema.table")
    session.run_sql("CREATE EVENT not_specified_schema.event ON SCHEDULE EVERY 1 YEAR DO BEGIN END")
    session.run_sql("CREATE PROCEDURE not_specified_schema.routine() DETERMINISTIC BEGIN END")
    session.run_sql("CREATE TRIGGER not_specified_schema.trigger AFTER DELETE ON not_specified_schema.table FOR EACH ROW BEGIN END")
    # we're only interested in DDL, progress is not important
    options["ddlOnly"] = True
    options["showProgress"] = False
    # do the dump
    shutil.rmtree(test_output_absolute, True)
    util.dump_tables("schema_1", ["existing_table_1", "existing_table_2"], test_output_absolute, options)
    # remove everything from the server once again, load the dump
    wipeout_server(session)
    util.load_dump(test_output_absolute, { "showProgress": False })
    # fetch data about the current DB structure
    return snapshot_instance(session)

def entries(snapshot, keys = []):
    entry = snapshot["schemas"]
    for key in keys:
        entry = entry[key]
    return sorted(list(entry.keys()))

#@<> WL14244 - includeTriggers - invalid values
EXPECT_FAIL("ValueError", "Argument #4: The trigger to be included must be in the following form: schema.table or schema.table.trigger, with optional backtick quotes, wrong value: 'trigger'.", types_schema, types_schema_tables, test_output_absolute, { "includeTriggers": [ "trigger" ] })
EXPECT_FAIL("ValueError", "Argument #4: Failed to parse trigger to be included 'schema.@': Invalid character in identifier", types_schema, types_schema_tables, test_output_absolute, { "includeTriggers": [ "schema.@" ] })

#@<> WL14244-TSFR_7_7
snapshot = dump_and_load({})
EXPECT_EQ(["existing_trigger_1"], entries(snapshot, ["schema_1", "tables", "existing_table_1", "triggers"]))
EXPECT_EQ(["existing_trigger_2"], entries(snapshot, ["schema_1", "tables", "existing_table_2", "triggers"]))

snapshot = dump_and_load({ "includeTriggers": [] })
EXPECT_EQ(["existing_trigger_1"], entries(snapshot, ["schema_1", "tables", "existing_table_1", "triggers"]))
EXPECT_EQ(["existing_trigger_2"], entries(snapshot, ["schema_1", "tables", "existing_table_2", "triggers"]))

#@<> WL14244-TSFR_7_11
snapshot = dump_and_load({ "includeTriggers": ['schema_1.existing_table_1', 'schema_1.existing_table_2.existing_trigger_2', 'schema_1.existing_table_2.non_existing_trigger'] })
EXPECT_EQ(["existing_trigger_1"], entries(snapshot, ["schema_1", "tables", "existing_table_1", "triggers"]))
EXPECT_EQ(["existing_trigger_2"], entries(snapshot, ["schema_1", "tables", "existing_table_2", "triggers"]))

#@<> BUG#33402791 - objects for schemas which are not included in the dump are reported as errors - includeTriggers
EXPECT_FAIL("ValueError", "Conflicting filtering options", types_schema, types_schema_tables, test_output_absolute, { "includeTriggers": [ 'non_existing_schema.table', 'not_specified_schema.table.trigger' ] })
EXPECT_STDOUT_CONTAINS("ERROR: The includeTriggers option contains a filter `non_existing_schema`.`table` which refers to a schema which was not included in the dump.")
EXPECT_STDOUT_CONTAINS("ERROR: The includeTriggers option contains a trigger `not_specified_schema`.`table`.`trigger` which refers to a schema which was not included in the dump.")
EXPECT_STDOUT_CONTAINS("ERROR: The includeTriggers option contains a filter `non_existing_schema`.`table` which refers to a table which was not included in the dump.")
EXPECT_STDOUT_CONTAINS("ERROR: The includeTriggers option contains a trigger `not_specified_schema`.`table`.`trigger` which refers to a table which was not included in the dump.")

#@<> WL14244 - excludeTriggers - invalid values
EXPECT_FAIL("ValueError", "Argument #4: The trigger to be excluded must be in the following form: schema.table or schema.table.trigger, with optional backtick quotes, wrong value: 'trigger'.", types_schema, types_schema_tables, test_output_absolute, { "excludeTriggers": [ "trigger" ] })
EXPECT_FAIL("ValueError", "Argument #4: Failed to parse trigger to be excluded 'schema.@': Invalid character in identifier", types_schema, types_schema_tables, test_output_absolute, { "excludeTriggers": [ "schema.@" ] })

#@<> WL14244-TSFR_8_7
snapshot = dump_and_load({})
EXPECT_EQ(["existing_trigger_1"], entries(snapshot, ["schema_1", "tables", "existing_table_1", "triggers"]))
EXPECT_EQ(["existing_trigger_2"], entries(snapshot, ["schema_1", "tables", "existing_table_2", "triggers"]))

snapshot = dump_and_load({ "excludeTriggers": [] })
EXPECT_EQ(["existing_trigger_1"], entries(snapshot, ["schema_1", "tables", "existing_table_1", "triggers"]))
EXPECT_EQ(["existing_trigger_2"], entries(snapshot, ["schema_1", "tables", "existing_table_2", "triggers"]))

#@<> WL14244-TSFR_8_11
snapshot = dump_and_load({ "excludeTriggers": ['schema_1.existing_table_1', 'schema_1.existing_table_2.existing_trigger_2', 'schema_1.existing_table_2.non_existing_trigger'] })
EXPECT_EQ([], entries(snapshot, ["schema_1", "tables", "existing_table_1", "triggers"]))
EXPECT_EQ([], entries(snapshot, ["schema_1", "tables", "existing_table_2", "triggers"]))

#@<> BUG#33402791 - objects for schemas which are not included in the dump are reported as errors - excludeTriggers
EXPECT_FAIL("ValueError", "Conflicting filtering options", types_schema, types_schema_tables, test_output_absolute, { "excludeTriggers": [ 'non_existing_schema.table', 'not_specified_schema.table.trigger' ] })
EXPECT_STDOUT_CONTAINS("ERROR: The excludeTriggers option contains a filter `non_existing_schema`.`table` which refers to a schema which was not included in the dump.")
EXPECT_STDOUT_CONTAINS("ERROR: The excludeTriggers option contains a trigger `not_specified_schema`.`table`.`trigger` which refers to a schema which was not included in the dump.")
EXPECT_STDOUT_CONTAINS("ERROR: The excludeTriggers option contains a filter `non_existing_schema`.`table` which refers to a table which was not included in the dump.")
EXPECT_STDOUT_CONTAINS("ERROR: The excludeTriggers option contains a trigger `not_specified_schema`.`table`.`trigger` which refers to a table which was not included in the dump.")

#@<> includeX + excludeX conflicts - setup
wipeout_server(session)
# create sample DB structure
session.run_sql("CREATE SCHEMA a")
session.run_sql("CREATE TABLE a.t (id INT)")
session.run_sql("CREATE TABLE a.t1 (id INT)")

#@<> includeX + excludeX conflicts - helpers
def dump_with_conflicts(options, throws = True):
    WIPE_STDOUT()
    # we're only interested in warnings
    options["dryRun"] = True
    options["showProgress"] = False
    # do the dump
    shutil.rmtree(test_output_absolute, True)
    if throws:
        EXPECT_THROWS(lambda: util.dump_tables("a", [ "t" , "t1"], test_output_absolute, options), "ValueError: Util.dump_tables: Conflicting filtering options")
    else:
        # there could be some other exceptions, we ignore them
        try:
            util.dump_tables("a", [ "t" , "t1"], test_output_absolute, options)
        except Exception as e:
            print("Exception:", e)

#@<> includeTriggers + excludeTriggers conflicts
# no conflicts
dump_with_conflicts({ "includeTriggers": [], "excludeTriggers": [] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

dump_with_conflicts({ "includeTriggers": [ "a.t" ], "excludeTriggers": [] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

dump_with_conflicts({ "includeTriggers": [ "a.t.t" ], "excludeTriggers": [] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

dump_with_conflicts({ "includeTriggers": [], "excludeTriggers": [ "a.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

dump_with_conflicts({ "includeTriggers": [], "excludeTriggers": [ "a.t.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

dump_with_conflicts({ "includeTriggers": [ "a.t" ], "excludeTriggers": [ "a.t1" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

dump_with_conflicts({ "includeTriggers": [ "a.t.t" ], "excludeTriggers": [ "a.t1" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

dump_with_conflicts({ "includeTriggers": [ "a.t" ], "excludeTriggers": [ "a.t1.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

dump_with_conflicts({ "includeTriggers": [ "a.t.t" ], "excludeTriggers": [ "a.t1.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

dump_with_conflicts({ "includeTriggers": [ "a.t" ], "excludeTriggers": [ "a.t.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

# conflicts
dump_with_conflicts({ "includeTriggers": [ "a.t" ], "excludeTriggers": [ "a.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeTriggers and excludeTriggers options contain a filter `a`.`t`.")

dump_with_conflicts({ "includeTriggers": [ "a.t", "a.t1" ], "excludeTriggers": [ "a.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeTriggers and excludeTriggers options contain a filter `a`.`t`.")
EXPECT_STDOUT_NOT_CONTAINS("`a`.`t1`")

dump_with_conflicts({ "includeTriggers": [ "a.t" ], "excludeTriggers": [ "a.t", "a.t1" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeTriggers and excludeTriggers options contain a filter `a`.`t`.")
EXPECT_STDOUT_NOT_CONTAINS("`a`.`t1`")

dump_with_conflicts({ "includeTriggers": [ "a.t.t" ], "excludeTriggers": [ "a.t.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeTriggers and excludeTriggers options contain a trigger `a`.`t`.`t`.")

dump_with_conflicts({ "includeTriggers": [ "a.t.t", "a.t1.t" ], "excludeTriggers": [ "a.t.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeTriggers and excludeTriggers options contain a trigger `a`.`t`.`t`.")
EXPECT_STDOUT_NOT_CONTAINS("`a`.`t1`")

dump_with_conflicts({ "includeTriggers": [ "a.t.t" ], "excludeTriggers": [ "a.t.t", "a.t1.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeTriggers and excludeTriggers options contain a trigger `a`.`t`.`t`.")
EXPECT_STDOUT_NOT_CONTAINS("`a`.`t1`")

dump_with_conflicts({ "includeTriggers": [ "a.t.t" ], "excludeTriggers": [ "a.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: The includeTriggers option contains a trigger `a`.`t`.`t` which is excluded by the value of the excludeTriggers option: `a`.`t`.")

dump_with_conflicts({ "includeTriggers": [ "a.t.t", "a.t1.t" ], "excludeTriggers": [ "a.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: The includeTriggers option contains a trigger `a`.`t`.`t` which is excluded by the value of the excludeTriggers option: `a`.`t`.")
EXPECT_STDOUT_NOT_CONTAINS("`a`.`t1`")

#@<> BUG#33400387 setup
tested_schema = "test_schema"
tested_table = "test_table"

decimal_digits = 0

session.run_sql("CREATE SCHEMA !;", [ tested_schema ])
session.run_sql(f"CREATE TABLE !.! (a DECIMAL({20 + decimal_digits},{decimal_digits}) PRIMARY KEY, b text);", [ tested_schema, tested_table ])

gap_start = 18446744073709551615
gap_items = 10000
gap_step = 1000

def test_bug_33400387(step):
    def insert(r):
        session.run_sql(f"""INSERT INTO !.! VALUES {",".join([f"('{v}{'.' + str(random.randint(0, (10 ** decimal_digits) - 1)) if decimal_digits else ''}', '')" for v in r])};""", [ tested_schema, tested_table ])
    def generate_gaps():
        value = gap_start
        while True:
            yield value
            value += next(step)
    gen = generate_gaps()
    # clear the data
    session.run_sql("TRUNCATE TABLE !.!;", [ tested_schema, tested_table ])
    # insert rows with gaps in the PK
    insert([next(gen) for i in range(gap_items)])
    # all the remaining data is continuous
    start = next(gen)
    insert(range(start, start + gap_items))
    # use some dummy data
    session.run_sql("UPDATE !.! SET b = repeat('x', 5000);", [ tested_schema, tested_table ])
    # analyze the table for optimum results
    session.run_sql("ANALYZE TABLE !.!;", [ tested_schema, tested_table ])
    # run the test
    EXPECT_SUCCESS(tested_schema, [ tested_table ], test_output_absolute, { "bytesPerChunk": "1M", "compression": "none", "showProgress": False })
    # expect at least 320 chunks, we're dealing with random data, allow for some chunks which are smaller
    CHECK_OUTPUT_SANITY(test_output_absolute, 180000, 320, 4)
    # load the dump
    recreate_verification_schema()
    EXPECT_NO_THROWS(lambda: util.load_dump(test_output_absolute, { "schema": verification_schema, "showProgress": False }), "dump should be loaded without problems")
    # verify the checksums
    EXPECT_EQ(compute_checksum(tested_schema, tested_table), compute_checksum(verification_schema, tested_table), "checksum mismatch")

#@<> BUG#33400387 - equal gaps
test_bug_33400387(equal_gaps())

#@<> BUG#33400387 - random gaps
test_bug_33400387(random_gaps())

#@<> BUG#33400387 - increasing gaps
test_bug_33400387(increasing_gaps())

#@<> BUG#33400387 - decreasing gaps
test_bug_33400387(decreasing_gaps())

#@<> BUG#33400387 - create schema with some decimal digits
gap_start = -2 * 18446744073709551615
decimal_digits = 2
session.run_sql("DROP SCHEMA !;", [ tested_schema ])
session.run_sql("CREATE SCHEMA !;", [ tested_schema ])
session.run_sql(f"CREATE TABLE !.! (a DECIMAL({20 + decimal_digits},{decimal_digits}) PRIMARY KEY, b text);", [ tested_schema, tested_table ])

#@<> BUG#33400387 - equal gaps + decimal digits
test_bug_33400387(equal_gaps())

#@<> BUG#33400387 - random gaps + decimal digits
test_bug_33400387(random_gaps())

#@<> BUG#33400387 - increasing gaps + decimal digits
test_bug_33400387(increasing_gaps())

#@<> BUG#33400387 - decreasing gaps + decimal digits
test_bug_33400387(decreasing_gaps())

#@<> BUG#33400387 cleanup
session.run_sql("DROP SCHEMA !;", [ tested_schema ])

#@<> Cleanup
drop_all_schemas()
session.run_sql("SET GLOBAL local_infile = false;")
session.close()
testutil.destroy_sandbox(__mysql_sandbox_port1)
shutil.rmtree(incompatible_table_directory, True)
