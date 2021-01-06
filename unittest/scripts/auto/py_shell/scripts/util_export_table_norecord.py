# imports
import hashlib
import json
import os
import os.path
import random
import re
import shutil
import stat
import urllib.parse

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
test_table_json = "metadata"
test_table_trigger = "sample_trigger"
test_schema_procedure = "sample_procedure"
test_schema_function = "sample_function"
test_schema_event = "sample_event"
test_view = "sample_view"

verification_schema = "wl13804_ver"

types_schema = "xtest"

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

all_schemas = [world_x_schema, types_schema, test_schema, verification_schema]
test_schema_tables = [test_table_primary, test_table_unique, test_table_non_unique, test_table_no_index, test_table_json]
test_schema_views = [test_view]

uri = __sandbox_uri1
xuri = __sandbox_uri1.replace("mysql://", "mysqlx://") + "0"

test_output_relative_parent = "dump_output"
test_output_relative = os.path.join(test_output_relative_parent, "data.txt")
test_output_absolute = os.path.abspath(test_output_relative)
test_output_absolute_parent = os.path.dirname(test_output_absolute)

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

def recreate_verification_schema():
    session.run_sql("DROP SCHEMA IF EXISTS !;", [ verification_schema ])
    session.run_sql("CREATE SCHEMA !;", [ verification_schema ])

def count_files_with_basename(directory, basename):
    cnt = 0
    for f in os.listdir(directory):
        if f.startswith(basename):
            cnt += 1
    return cnt

def has_file_with_basename(directory, basename):
    return count_files_with_basename(directory, basename) > 0

def count_files_with_extension(directory, ext):
    cnt = 0
    for f in os.listdir(directory):
        if f.endswith(ext):
            cnt += 1
    return cnt

def quote(schema, table = None):
    if table is None:
        return "`{0}`".format(schema.replace("`", "``"))
    else:
        return "{0}.{1}".format(quote(schema), quote(table))

def hash_file(path):
    size = 65536
    md5 = hashlib.md5()
    with open(path, "rb") as f:
        while True:
            contents = f.read(size)
            if not contents:
                break
            md5.update(contents)
        return md5.hexdigest()

def EXPECT_SUCCESS(table, outputUrl, options = {}):
    shutil.rmtree(test_output_absolute_parent, True)
    os.mkdir(test_output_absolute_parent)
    EXPECT_FALSE(os.path.isfile(test_output_absolute))
    util.export_table(table, outputUrl, options)
    EXPECT_TRUE(os.path.isfile(test_output_absolute))

def EXPECT_FAIL(error, msg, table, outputUrl, options = {}, expect_file_created = False):
    shutil.rmtree(test_output_absolute_parent, True)
    os.mkdir(test_output_absolute_parent)
    EXPECT_THROWS(lambda: util.export_table(table, outputUrl, options), "{0}: Util.export_table: {1}".format(error, msg))
    if expect_file_created:
        EXPECT_TRUE(os.path.isfile(test_output_absolute))

def TEST_BOOL_OPTION(option):
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type Bool, but is Null".format(option), quote(types_schema, types_schema_tables[0]), test_output_relative, { option: None })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' Bool expected, but value is String".format(option), quote(types_schema, types_schema_tables[0]), test_output_relative, { option: "dummy" })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type Bool, but is Array".format(option), quote(types_schema, types_schema_tables[0]), test_output_relative, { option: [] })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type Bool, but is Map".format(option), quote(types_schema, types_schema_tables[0]), test_output_relative, { option: {} })

def TEST_STRING_OPTION(option):
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type String, but is Null".format(option), quote(types_schema, types_schema_tables[0]), test_output_relative, { option: None })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type String, but is Integer".format(option), quote(types_schema, types_schema_tables[0]), test_output_relative, { option: 5 })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type String, but is Integer".format(option), quote(types_schema, types_schema_tables[0]), test_output_relative, { option: -5 })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type String, but is Array".format(option), quote(types_schema, types_schema_tables[0]), test_output_relative, { option: [] })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type String, but is Map".format(option), quote(types_schema, types_schema_tables[0]), test_output_relative, { option: {} })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type String, but is Bool".format(option), quote(types_schema, types_schema_tables[0]), test_output_relative, { option: False })

def TEST_UINT_OPTION(option):
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type UInteger, but is Null".format(option), quote(types_schema, types_schema_tables[0]), test_output_relative, { option: None })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' UInteger expected, but Integer value is out of range".format(option), quote(types_schema, types_schema_tables[0]), test_output_relative, { option: -5 })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' UInteger expected, but value is String".format(option), quote(types_schema, types_schema_tables[0]), test_output_relative, { option: "dummy" })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type UInteger, but is Array".format(option), quote(types_schema, types_schema_tables[0]), test_output_relative, { option: [] })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type UInteger, but is Map".format(option), quote(types_schema, types_schema_tables[0]), test_output_relative, { option: {} })

def TEST_ARRAY_OF_STRINGS_OPTION(option):
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type Array, but is Integer".format(option), quote(types_schema, types_schema_tables[0]), test_output_relative, { option: 5 })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type Array, but is Integer".format(option), quote(types_schema, types_schema_tables[0]), test_output_relative, { option: -5 })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type Array, but is String".format(option), quote(types_schema, types_schema_tables[0]), test_output_relative, { option: "dummy" })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type Array, but is Map".format(option), quote(types_schema, types_schema_tables[0]), test_output_relative, { option: {} })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type Array, but is Bool".format(option), quote(types_schema, types_schema_tables[0]), test_output_relative, { option: False })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' String expected, but value is Null".format(option), quote(types_schema, types_schema_tables[0]), test_output_relative, { option: [ None ] })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' String expected, but value is Integer".format(option), quote(types_schema, types_schema_tables[0]), test_output_relative, { option: [ 5 ] })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' String expected, but value is Integer".format(option), quote(types_schema, types_schema_tables[0]), test_output_relative, { option: [ -5 ] })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' String expected, but value is Map".format(option), quote(types_schema, types_schema_tables[0]), test_output_relative, { option: [ {} ] })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' String expected, but value is Bool".format(option), quote(types_schema, types_schema_tables[0]), test_output_relative, { option: [ False ] })

def get_all_columns(schema, table):
    columns = []
    for column in session.run_sql("SELECT COLUMN_NAME FROM information_schema.columns WHERE TABLE_SCHEMA = ? AND TABLE_NAME = ? ORDER BY ORDINAL_POSITION;", [schema, table]).fetch_all():
        columns.append(column[0])
    return columns

def compute_crc(schema, table, columns):
    session.run_sql("SET @crc = '';")
    session.run_sql("SELECT @crc := MD5(CONCAT_WS('#',@crc,{0})) FROM !.! ORDER BY !;".format(("!," * len(columns))[:-1]), columns + [schema, table, columns[0]])
    return session.run_sql("SELECT @crc;").fetch_one()[0]

def TEST_LOAD(schema, table, options = {}):
    print("---> testing: `{0}`.`{1}` with options: {2}".format(schema, table, options))
    # prepare the options
    target_table = "verification"
    run_options = { "showProgress": False }
    # add extra options
    run_options.update(options)
    # export the table
    EXPECT_SUCCESS(quote(schema, table), test_output_absolute, run_options)
    # create target table
    recreate_verification_schema()
    session.run_sql("CREATE TABLE !.! LIKE !.!;", [verification_schema, target_table, schema, table])
    # prepare options for load
    run_options.update({ "schema": verification_schema, "table": target_table, "characterSet": "utf8mb4" })
    # rename the character set key (if it was provided)
    if "defaultCharacterSet" in run_options:
        run_options["characterSet"] = run_options["defaultCharacterSet"]
        del run_options["defaultCharacterSet"]
    # add decoded columns
    all_columns = get_all_columns(schema, table)
    decoded_columns = {}
    for column in all_columns:
        ctype = session.run_sql("SELECT DATA_TYPE FROM information_schema.columns WHERE TABLE_SCHEMA = ? AND TABLE_NAME = ? AND COLUMN_NAME = ?;", [schema, table, column]).fetch_one()[0].lower()
        if (ctype.endswith("binary") or
            ctype.endswith("bit") or
            ctype.endswith("blob") or
            ctype.endswith("geometry") or
            ctype.endswith("geomcollection") or
            ctype.endswith("geometrycollection") or
            ctype.endswith("linestring") or
            ctype.endswith("point") or
            ctype.endswith("polygon")):
                decoded_columns[column] = "FROM_BASE64"
    if decoded_columns:
        run_options["columns"] = all_columns
        run_options["decodeColumns"] = decoded_columns
    # load in chunks
    run_options["bytesPerChunk"] = "1k"
    # load data
    util.import_table(test_output_absolute, run_options)
    # compute CRC
    EXPECT_EQ(compute_crc(schema, table, all_columns), compute_crc(verification_schema, target_table, all_columns))

def get_magic_number(path, count):
    with open(path, "rb") as f:
        return f.read(count).hex().upper()

GZIP_MAGIC_NUMBER = "1F8B"
ZSTD_MAGIC_NUMBER = "28B52FFD"

#@<> WL13804-FR2.1 - If there is no open global Shell session, an exception must be thrown. (no global session)
# WL13804-TSFR_2_1_2
EXPECT_FAIL("RuntimeError", "An open session is required to perform this operation.", quote('mysql', 'user'), test_output_relative)

#@<> deploy sandbox
testutil.deploy_raw_sandbox(__mysql_sandbox_port1, "root")

#@<> wait for server
testutil.wait_sandbox_alive(uri)
shell.connect(uri)

#@<> WL13804-FR2.1 - If there is no open global Shell session, an exception must be thrown. (no open session)
# WL13804-TSFR_2_1_1
session.close()
EXPECT_FAIL("RuntimeError", "An open session is required to perform this operation.", quote('mysql', 'user'), test_output_relative)

#@<> Setup
setup_session()

drop_all_schemas()
create_all_schemas()

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
# table with JSON values
session.run_sql("CREATE TABLE !.! (`description` JSON);", [ test_schema, test_table_json ])

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

session.run_sql(r"""INSERT INTO !.! (`description`) VALUES ('{"key1": "value1", "key2": "value2"}');""", [ test_schema, test_table_json ])
session.run_sql(r"""INSERT INTO !.! (`description`) VALUES ('[1, 2]');""", [ test_schema, test_table_json ])
session.run_sql(r"""INSERT INTO !.! (`description`) VALUES ('["abc", 10, null, true, false]');""", [ test_schema, test_table_json ])
session.run_sql(r"""INSERT INTO !.! (`description`) VALUES ('{"k1": "value", "k2": 10}');""", [ test_schema, test_table_json ])
session.run_sql(r"""INSERT INTO !.! (`description`) VALUES ('["12:18:29.000000", "2015-07-29", "2015-07-29 12:18:29.000000"]');""", [ test_schema, test_table_json ])
session.run_sql(r"""INSERT INTO !.! (`description`) VALUES ('[99, {"id": "HK500", "cost": 75.99}, ["hot", "cold"]]');""", [ test_schema, test_table_json ])
session.run_sql(r"""INSERT INTO !.! (`description`) VALUES ('{"k1": "value", "k2": [10, 20]}');""", [ test_schema, test_table_json ])

for table in [test_table_primary, test_table_unique, test_table_non_unique, test_table_no_index]:
    session.run_sql("ANALYZE TABLE !.!;", [ test_schema, table ])

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

#@<> WL13804-FR3 - The `table` parameter of the `util.exportTable()` function must be a string value which specifies the table to be dumped. This value may be given in the following forms: `table`, `schema.table`. Both `schema` and `table` must be valid MySQL identifiers and must be quoted with backtick (`` ` ``) character when required.
# WL13804-TSFR_3_1
EXPECT_FAIL("TypeError", "Argument #1 is expected to be a string", None, test_output_relative)
EXPECT_FAIL("TypeError", "Argument #1 is expected to be a string", 1, test_output_relative)
EXPECT_FAIL("TypeError", "Argument #1 is expected to be a string", [], test_output_relative)
EXPECT_FAIL("TypeError", "Argument #1 is expected to be a string", {}, test_output_relative)
EXPECT_FAIL("TypeError", "Argument #1 is expected to be a string", True, test_output_relative)

EXPECT_SUCCESS("{0}.{1}".format(types_schema, types_schema_tables[0]), test_output_absolute, { "showProgress": False })
EXPECT_SUCCESS("`{0}`.{1}".format(types_schema, types_schema_tables[0]), test_output_absolute, { "showProgress": False })
EXPECT_SUCCESS("{0}.`{1}`".format(types_schema, types_schema_tables[0]), test_output_absolute, { "showProgress": False })
EXPECT_SUCCESS("`{0}`.`{1}`".format(types_schema, types_schema_tables[0]), test_output_absolute, { "showProgress": False })

#@<> WL13804-FR4 - The `outputUrl` parameter of the `util.exportTable()` function must be a string value which specifies the output file, where the dump data is going to be stored.
# WL13804-TSFR_4_1
EXPECT_FAIL("TypeError", "Argument #2 is expected to be a string", quote(test_schema, test_table_non_unique), None)
EXPECT_FAIL("TypeError", "Argument #2 is expected to be a string", quote(test_schema, test_table_non_unique), 1)
EXPECT_FAIL("TypeError", "Argument #2 is expected to be a string", quote(test_schema, test_table_non_unique), [])
EXPECT_FAIL("TypeError", "Argument #2 is expected to be a string", quote(test_schema, test_table_non_unique), {})
EXPECT_FAIL("TypeError", "Argument #2 is expected to be a string", quote(test_schema, test_table_non_unique), True)

EXPECT_SUCCESS(quote(types_schema, types_schema_tables[0]), test_output_absolute, { "showProgress": False })

#@<> WL13804-FR5 - The `options` optional parameter of the `util.exportTable()` function must be a dictionary which contains options for the export operation.
EXPECT_FAIL("TypeError", "Argument #3 is expected to be a map", quote(test_schema, test_table_non_unique), test_output_relative, 1)
EXPECT_FAIL("TypeError", "Argument #3 is expected to be a map", quote(test_schema, test_table_non_unique), test_output_relative, "string")
EXPECT_FAIL("TypeError", "Argument #3 is expected to be a map", quote(test_schema, test_table_non_unique), test_output_relative, [])

#@<> WL13804-TSFR_1_5 - Call exportTable(): giving less parameters than allowed, giving more parameters than allowed
EXPECT_THROWS(lambda: util.export_table(), "ValueError: Util.export_table: Invalid number of arguments, expected 2 to 3 but got 0")
EXPECT_THROWS(lambda: util.export_table(quote(test_schema, test_table_non_unique), test_output_relative, {}, None), "ValueError: Util.export_table: Invalid number of arguments, expected 2 to 3 but got 4")

#@<> WL13804-FR3.1 - If schema is not specified in the `table` parameter, the current schema of the global Shell session must be used. If there is none, an exception must be raised.
# WL13804-TSFR_3_1_1
EXPECT_FAIL("ValueError", "The table was given without a schema and there is no active schema on the current session, unable to deduce which table to export.", types_schema_tables[0], test_output_absolute, { "showProgress": False })

# WL13804-TSFR_3_1_2
session.run_sql("USE !;", [ types_schema ])
EXPECT_SUCCESS(types_schema_tables[0], test_output_absolute, { "showProgress": False })
EXPECT_SUCCESS("`{0}`".format(types_schema_tables[0]), test_output_absolute, { "showProgress": False })

#@<> WL13804-FR3.2 - If table specified by the `table` parameter does not exist, an exception must be thrown.
# non-existent table in the current schema
EXPECT_FAIL("ValueError", "The requested table `{0}`.`dummy` was not found in the database.".format(types_schema), "dummy", test_output_absolute, { "showProgress": False })
EXPECT_FAIL("ValueError", "The requested table `{0}`.`dummy` was not found in the database.".format(types_schema), "`dummy`", test_output_absolute, { "showProgress": False })
# non-existent table in an existing schema
EXPECT_FAIL("ValueError", "The requested table `{0}`.`dummy` was not found in the database.".format(test_schema), quote(test_schema, "dummy"), test_output_absolute, { "showProgress": False })
# non-existent table in a non-existent schema
EXPECT_FAIL("ValueError", "The requested table `dummy`.`dummy` was not found in the database.", quote("dummy", "dummy"), test_output_absolute, { "showProgress": False })
# WL13804-TSFR_3_2_1
EXPECT_FAIL("ValueError", "The requested table `{0}`.`\n` was not found in the database.".format(types_schema), "`\n`", test_output_absolute, { "showProgress": False })

#@<> WL13804-FR4.1 - If the dump is going to be stored on the local filesystem, the `outputUrl` parameter may be optionally prefixed with `file://` scheme.
EXPECT_SUCCESS(quote(types_schema, types_schema_tables[0]), "file://" + test_output_absolute, { "showProgress": False })

#@<> WL13804-TSFR_4_1_2
EXPECT_FAIL("ValueError", "File handling for http protocol is not supported.", quote(types_schema, types_schema_tables[0]), "http://example.com", { "showProgress": False })
EXPECT_FAIL("ValueError", "File handling for HTTPS protocol is not supported.", quote(types_schema, types_schema_tables[0]), "HTTPS://www.example.com", { "showProgress": False })

#@<> WL13804-FR4.2 - If the dump is going to be stored on the local filesystem and the `outputUrl` parameter holds a relative path, its absolute value is computed as relative to the current working directory.
EXPECT_SUCCESS(quote(types_schema, types_schema_tables[0]), test_output_relative, { "showProgress": False })

#@<> WL13804-FR4.2 - relative with .
EXPECT_SUCCESS(quote(types_schema, types_schema_tables[0]), "./" + test_output_relative, { "showProgress": False })

#@<> WL13804-FR4.2 - relative with ..
EXPECT_SUCCESS(quote(types_schema, types_schema_tables[0]), "dummy/../" + test_output_relative, { "showProgress": False })

#@<> WL13804-FR4.1 + FR4.1
EXPECT_SUCCESS(quote(types_schema, types_schema_tables[0]), "file://" + test_output_relative, { "showProgress": False })

#@<> WL13804-FR4.3 - If the output file does not exist, it must be created if its parent directory exists. If it is not possible or parent directory does not exist, an exception must be thrown.
# parent directory does not exist
shutil.rmtree(test_output_absolute_parent, True)
EXPECT_FALSE(os.path.isdir(test_output_absolute_parent))
EXPECT_THROWS(lambda: util.export_table(quote(types_schema, types_schema_tables[0]), test_output_absolute, { "showProgress": False }), "Cannot proceed with the dump, the directory containing '{0}' does not exist at the target location {1}.".format(test_output_absolute, absolute_path_for_output(test_output_absolute_parent)))
EXPECT_FALSE(os.path.isdir(test_output_absolute_parent))

# unable to create file, directory with the same name exists
shutil.rmtree(test_output_absolute_parent, True)
EXPECT_FALSE(os.path.isdir(test_output_absolute_parent))
os.makedirs(test_output_absolute)
EXPECT_THROWS(lambda: util.export_table(quote(types_schema, types_schema_tables[0]), test_output_absolute, { "showProgress": False }), "RuntimeError: Util.export_table: Fatal error during dump")
EXPECT_STDOUT_CONTAINS("Cannot open file '{0}': ".format(absolute_path_for_output(test_output_absolute)))

#@<> WL13804-FR4.4 - If the output file exists, it must be overwritten.
# WL13804-TSFR_4_4_1 - plain file
shutil.rmtree(test_output_absolute_parent, True)
os.mkdir(test_output_absolute_parent)

with open(test_output_absolute, "w") as f:
    f.write("qwerty")

EXPECT_NO_THROWS(lambda: util.export_table(quote(types_schema, types_schema_tables[0]), test_output_absolute, { "showProgress": False }), "export should overwrite the existing file")
EXPECT_FILE_NOT_CONTAINS("qwerty", test_output_absolute)
plain_file_hash = hash_file(test_output_absolute)

# WL13804-TSFR_4_4_1 - compression
EXPECT_NO_THROWS(lambda: util.export_table(quote(types_schema, types_schema_tables[0]), test_output_absolute, { "compression": "zstd", "showProgress": False }), "export with compression should not append the extension")
EXPECT_NE(plain_file_hash, hash_file(test_output_absolute))

#@<> WL13804-FR5.1 - The `options` dictionary may contain a `maxRate` key with a string value, which specifies the limit of data read throughput in bytes per second per thread.
TEST_STRING_OPTION("maxRate")

# WL13804-TSFR_5_1_1_1
EXPECT_SUCCESS(quote(types_schema, types_schema_tables[0]), test_output_absolute, { "maxRate": "1", "showProgress": False })
EXPECT_SUCCESS(quote(types_schema, types_schema_tables[0]), test_output_absolute, { "maxRate": "1000k", "showProgress": False })
EXPECT_SUCCESS(quote(types_schema, types_schema_tables[0]), test_output_absolute, { "maxRate": "1000K", "showProgress": False })
EXPECT_SUCCESS(quote(types_schema, types_schema_tables[0]), test_output_absolute, { "maxRate": "1M", "showProgress": False })
EXPECT_SUCCESS(quote(types_schema, types_schema_tables[0]), test_output_absolute, { "maxRate": "1G", "showProgress": False })

#@<> WL13804-FR5.1.1 - The value of the `maxRate` option must use the same format as specified in WL#12193.
EXPECT_FAIL("ValueError", 'Argument #3: Wrong input number "xyz"', quote(types_schema, types_schema_tables[0]), test_output_absolute, { "maxRate": "xyz" })
EXPECT_FAIL("ValueError", 'Argument #3: Wrong input number "1xyz"', quote(types_schema, types_schema_tables[0]), test_output_absolute, { "maxRate": "1xyz" })
EXPECT_FAIL("ValueError", 'Argument #3: Wrong input number "2Mhz"', quote(types_schema, types_schema_tables[0]), test_output_absolute, { "maxRate": "2Mhz" })
# WL13804-TSFR_5_1_1_2
EXPECT_FAIL("ValueError", 'Argument #3: Wrong input number "hello world!"', quote(types_schema, types_schema_tables[0]), test_output_absolute, { "maxRate": "hello world!" })
EXPECT_FAIL("ValueError", 'Argument #3: Input number "-1" cannot be negative', quote(types_schema, types_schema_tables[0]), test_output_absolute, { "maxRate": "-1" })
EXPECT_FAIL("ValueError", 'Argument #3: Input number "-1234567890123456" cannot be negative', quote(types_schema, types_schema_tables[0]), test_output_absolute, { "maxRate": "-1234567890123456" })
EXPECT_FAIL("ValueError", 'Argument #3: Input number "-2K" cannot be negative', quote(types_schema, types_schema_tables[0]), test_output_absolute, { "maxRate": "-2K" })
EXPECT_FAIL("ValueError", 'Argument #3: Wrong input number "3m"', quote(types_schema, types_schema_tables[0]), test_output_absolute, { "maxRate": "3m" })
EXPECT_FAIL("ValueError", 'Argument #3: Wrong input number "4g"', quote(types_schema, types_schema_tables[0]), test_output_absolute, { "maxRate": "4g" })

EXPECT_SUCCESS(quote(types_schema, types_schema_tables[0]), test_output_absolute, { "maxRate": "1000000", "showProgress": False })

#@<> WL13804-FR5.1.1 - kilo.
EXPECT_SUCCESS(quote(types_schema, types_schema_tables[0]), test_output_absolute, { "maxRate": "1000k", "showProgress": False })

#@<> WL13804-FR5.1.1 - giga.
EXPECT_SUCCESS(quote(types_schema, types_schema_tables[0]), test_output_absolute, { "maxRate": "1G", "showProgress": False })

#@<> WL13804-FR5.1.2 - If the `maxRate` option is set to `"0"` or to an empty string, the read throughput must not be limited.
# WL13804-TSFR_5_1_2_1
EXPECT_SUCCESS(quote(types_schema, types_schema_tables[0]), test_output_absolute, { "maxRate": "0", "showProgress": False })

#@<> WL13804-FR5.1.2 - empty string.
# WL13804-TSFR_5_1_2_1
EXPECT_SUCCESS(quote(types_schema, types_schema_tables[0]), test_output_absolute, { "maxRate": "", "showProgress": False })

#@<> WL13804-FR5.1.2 - missing.
# WL13804-TSFR_5_1_3_1
EXPECT_SUCCESS(quote(types_schema, types_schema_tables[0]), test_output_absolute, { "showProgress": False })

#@<> WL13804-FR5.2 - The `options` dictionary may contain a `showProgress` key with a Boolean value, which specifies whether to display the progress of dump process.
# WL13804-TSFR_5_2_1_1
TEST_BOOL_OPTION("showProgress")

EXPECT_SUCCESS(quote(types_schema, types_schema_tables[0]), test_output_absolute, { "showProgress": True })

#@<> WL13804-FR5.2.1 - The information about the progress must include:
# * The estimated total number of rows to be dumped.
# * The number of rows dumped so far.
# * The current progress as a percentage.
# * The current throughput in rows per second.
# * The current throughput in bytes written per second.
# WL13804-TSFR_5_2_1_2
shutil.rmtree(test_output_absolute_parent, True)
os.mkdir(test_output_absolute_parent)
EXPECT_FALSE(os.path.isfile(test_output_relative))
rc = testutil.call_mysqlsh([uri, "--schema=" + types_schema , "--", "util", "export-table", types_schema_tables[0],  test_output_relative, '--show-progress'])
EXPECT_EQ(0, rc)
EXPECT_TRUE(os.path.isfile(test_output_relative))
EXPECT_STDOUT_MATCHES(re.compile(r'\d+% \(\d+\.?\d*[TGMK]? rows / ~\d+\.?\d*[TGMK]? rows\), \d+\.?\d*[TGMK]? rows?/s, \d+\.?\d* [TGMK]?B/s', re.MULTILINE))

#@<> WL13804-TSFR_5_2_2_1
shutil.rmtree(test_output_absolute_parent, True)
os.mkdir(test_output_absolute_parent)
EXPECT_FALSE(os.path.isfile(test_output_relative))
rc = testutil.call_mysqlsh([uri, "--json=pretty", "--schema=" + types_schema, "--", "util", "export-table", types_schema_tables[0], test_output_relative, "--show-progress"])
EXPECT_EQ(0, rc)
EXPECT_TRUE(os.path.isfile(test_output_relative))
EXPECT_STDOUT_MATCHES(re.compile(r'\"info\": \"\d+% \(\d+\.?\d*[TGMK]? rows / \d+\.?\d*[TGMK]? rows\), \d+\.?\d*[TGMK]? rows?/s, \d+\.?\d* [TGMK]?B/s\"', re.MULTILINE))

#@<> WL13804-FR5.2.2 - If the `showProgress` option is not given, a default value of `true` must be used instead if shell is used interactively. Otherwise, it is set to `false`.
shutil.rmtree(test_output_absolute_parent, True)
os.mkdir(test_output_absolute_parent)
EXPECT_FALSE(os.path.isfile(test_output_relative))
rc = testutil.call_mysqlsh([uri, "--schema=" + types_schema, "--", "util", "export-table", types_schema_tables[0], test_output_relative])
EXPECT_EQ(0, rc)
EXPECT_TRUE(os.path.isfile(test_output_relative))
EXPECT_STDOUT_NOT_CONTAINS("rows/s")

#@<>WL13804-FR5.3 - The `options` dictionary may contain a `compression` key with a string value, which specifies the compression type used when writing the data dump files.
# WL13804-TSFR_5_3_1_1
TEST_STRING_OPTION("compression")

# WL13804-TSFR_5_3_1_2
EXPECT_SUCCESS(quote(types_schema, types_schema_tables[0]), test_output_absolute, { "compression": "none", "showProgress": False })
EXPECT_NE(GZIP_MAGIC_NUMBER, get_magic_number(test_output_absolute, 2))
EXPECT_NE(ZSTD_MAGIC_NUMBER, get_magic_number(test_output_absolute, 4))

#@<> WL13804-FR5.3.1 - The allowed values for the `compression` option are:
# * `"none"` - no compression is used,
# * `"gzip"` - gzip compression is used.
# * `"zstd"` - zstd compression is used.
# WL13804-TSFR_5_3_1_3
EXPECT_SUCCESS(quote(types_schema, types_schema_tables[0]), test_output_absolute, { "compression": "gzip", "showProgress": False })
EXPECT_EQ(GZIP_MAGIC_NUMBER, get_magic_number(test_output_absolute, 2))

# WL13804-TSFR_5_3_1_4
EXPECT_SUCCESS(quote(types_schema, types_schema_tables[0]), test_output_absolute, { "compression": "zstd", "showProgress": False })
EXPECT_EQ(ZSTD_MAGIC_NUMBER, get_magic_number(test_output_absolute, 4))

EXPECT_FAIL("ValueError", "Argument #3: Unknown compression type: NoNE", quote(types_schema, types_schema_tables[0]), test_output_relative, { "compression": "NoNE"})
EXPECT_FAIL("ValueError", "Argument #3: Unknown compression type: gZIp", quote(types_schema, types_schema_tables[0]), test_output_relative, { "compression": "gZIp"})
EXPECT_FAIL("ValueError", "Argument #3: Unknown compression type: ZStd", quote(types_schema, types_schema_tables[0]), test_output_relative, { "compression": "ZStd"})

EXPECT_FAIL("ValueError", "Argument #3: The option 'compression' cannot be set to an empty string.", quote(types_schema, types_schema_tables[0]), test_output_relative, { "compression": "" })
EXPECT_FAIL("ValueError", "Argument #3: Unknown compression type: dummy", quote(types_schema, types_schema_tables[0]), test_output_relative, { "compression": "dummy" })
# WL13804-TSFR_5_3_1_1
EXPECT_FAIL("ValueError", "Argument #3: Unknown compression type: hello world!", quote(types_schema, types_schema_tables[0]), test_output_relative, { "compression": "hello world!" })

#@<> WL13804-FR5.3.2 - If the `compression` option is not given, a default value of `"none"` must be used instead.
# WL13804-TSFR_5_3_2_1
EXPECT_SUCCESS(quote(types_schema, types_schema_tables[0]), test_output_absolute, { "showProgress": False })
EXPECT_NE(GZIP_MAGIC_NUMBER, get_magic_number(test_output_absolute, 2))
EXPECT_NE(ZSTD_MAGIC_NUMBER, get_magic_number(test_output_absolute, 4))

#@<> WL13804-FR5.4 - The `options` dictionary may contain a `osBucketName` key with a string value, which specifies the OCI bucket name where the data dump files are going to be stored.
# WL13804-TSFR_5_4_1_1
TEST_STRING_OPTION("osBucketName")

#@<> WL13804-TSFR_5_4_2_1
EXPECT_SUCCESS(quote(types_schema, types_schema_tables[0]), test_output_absolute, { "osBucketName": "", "showProgress": False })

#@<> WL13804-TSFR_5_4_3_1
EXPECT_SUCCESS(quote(types_schema, types_schema_tables[0]), test_output_absolute, { "showProgress": False })

#@<> WL13804-FR5.5 - The `options` dictionary may contain a `osNamespace` key with a string value, which specifies the OCI namespace (tenancy name) where the OCI bucket is located.
# WL13804-TSFR_5_5_1_1
TEST_STRING_OPTION("osNamespace")

#@<> WL13804-FR5.5.2 - If the value of `osNamespace` option is a non-empty string and the value of `osBucketName` option is an empty string, an exception must be thrown.
# WL13804-TSFR_5_5_2_1
EXPECT_FAIL("ValueError", "Argument #3: The option 'osNamespace' cannot be used when the value of 'osBucketName' option is not set.", quote(types_schema, types_schema_tables[0]), test_output_relative, { "osNamespace": "namespace" })

#@<> WL13804-TSFR_5_5_3_2
EXPECT_SUCCESS(quote(types_schema, types_schema_tables[0]), test_output_absolute, { "osNamespace": "", "showProgress": False })

#@<> WL13804-FR5.6 - The `options` dictionary may contain a `ociConfigFile` key with a string value, which specifies the path to the OCI configuration file.
# WL13804-TSFR_5_6_1_1
TEST_STRING_OPTION("ociConfigFile")

EXPECT_FAIL("ValueError", "Argument #3: The option 'ociConfigFile' cannot be used when the value of 'osBucketName' option is not set.", quote(types_schema, types_schema_tables[0]), test_output_relative, { "ociConfigFile": "config" })
EXPECT_FAIL("ValueError", "Argument #3: The option 'ociConfigFile' cannot be used when the value of 'osBucketName' option is not set.", quote(types_schema, types_schema_tables[0]), test_output_relative, { "ociConfigFile": "config", "osBucketName": "" })

#@<> WL13804-TSFR_5_6_3_2
EXPECT_SUCCESS(quote(types_schema, types_schema_tables[0]), test_output_absolute, { "ociConfigFile": "", "showProgress": False })

#@<> WL13804-FR5.7 - The `options` dictionary may contain a `ociProfile` key with a string value, which specifies the name of the OCI profile to use.
# WL13804-TSFR_5_7_1
TEST_STRING_OPTION("ociProfile")

#@<> WL13804-FR5.7.2 - If the value of `ociProfile` option is a non-empty string and the value of `osBucketName` option is an empty string, an exception must be thrown.
EXPECT_FAIL("ValueError", "Argument #3: The option 'ociProfile' cannot be used when the value of 'osBucketName' option is not set.", quote(types_schema, types_schema_tables[0]), test_output_relative, { "ociProfile": "profile" })
#@<> WL13804-FR5.8 - The `options` dictionary may contain a `defaultCharacterSet` key with a string value, which specifies the character set to be used during the dump. The session variables `character_set_client`, `character_set_connection`, and `character_set_results` must be set to this value for each opened connection.
TEST_STRING_OPTION("defaultCharacterSet")

#@<> WL13804-TSFR_5_8_1_2
TEST_LOAD(world_x_schema, world_x_table, { "defaultCharacterSet": "utf8mb4" })
expected_hash = hash_file(test_output_absolute)

#@<> WL13804-TSFR_5_8_1_1
session.run_sql("SET NAMES 'latin1';")

EXPECT_SUCCESS(quote(world_x_schema, world_x_table), test_output_absolute, { "defaultCharacterSet": "utf8mb4", "showProgress": False })
EXPECT_EQ(expected_hash, hash_file(test_output_absolute))

session.run_sql("SET NAMES 'utf8mb4';")

#@<> WL13804-FR5.8.1 - If the value of the `defaultCharacterSet` option is not a character set supported by the MySQL server, an exception must be thrown.
# WL13804-TSFR_5_8_1_3
EXPECT_FAIL("MySQL Error (1115)", "Unknown character set: ''", quote(types_schema, types_schema_tables[0]), test_output_relative, { "defaultCharacterSet": "" })
EXPECT_FAIL("MySQL Error (1115)", "Unknown character set: 'dummy'", quote(types_schema, types_schema_tables[0]), test_output_relative, { "defaultCharacterSet": "dummy" })

#@<> WL13804-FR5.8.2 - If the `defaultCharacterSet` option is not given, a default value of `"utf8mb4"` must be used instead.
# WL13804-TSFR_5_8_2_1
EXPECT_SUCCESS(quote(world_x_schema, world_x_table), test_output_absolute, { "showProgress": False })
EXPECT_EQ(expected_hash, hash_file(test_output_absolute))

# WL13804-FR5.9 - The options which specify the output format of the data dump file, along with their default values, the same as in the `util.importTable()` function (specified in WL#12193), must be supported:
# * `fieldsTerminatedBy`,
# * `fieldsEnclosedBy`,
# * `fieldsEscapedBy`,
# * `fieldsOptionallyEnclosed`,
# * `linesTerminatedBy`,
# * `dialect`.

# WL13804-TSFR_5_9

#@<> WL13804-FR5.9 - various predefined dialects
for dialect in [ "default", "csv", "tsv", "csv-unix" ]:
    for table in types_schema_tables:
        TEST_LOAD(types_schema, table, { "dialect": dialect })

#@<> WL13804-FR5.9.1 - The JSON dialect must not be supported.
EXPECT_FAIL("ValueError", "Argument #3: The 'json' dialect is not supported.", quote(test_schema, test_table_json), test_output_relative, { "dialect": "json" })
#@<> WL13804-FR5.9 - custom dialect
for table in types_schema_tables:
    TEST_LOAD(types_schema, table, { "fieldsTerminatedBy": "a", "fieldsEnclosedBy": "b", "fieldsEscapedBy": "c", "linesTerminatedBy": "d", "fieldsOptionallyEnclosed": True })

#@<> WL13804-FR5.9 - more custom dialects
custom_dialect_table = "x"
session.run_sql("CREATE TABLE !.! (id int primary key AUTO_INCREMENT, p text, q text);", [ test_schema, custom_dialect_table ])
session.run_sql("INSERT INTO !.! VALUES(1, 'aaaaaa', 'bbbbb');", [ test_schema, custom_dialect_table ])
session.run_sql("INSERT INTO !.! VALUES(2, 'aabbaaaa', 'bbaabbb');", [ test_schema, custom_dialect_table ])
session.run_sql("INSERT INTO !.! VALUES(3, 'aabbaababaaa', 'bbaabbababbabab');", [ test_schema, custom_dialect_table ])
session.run_sql("INSERT INTO !.! (p, q) SELECT p, q FROM !.!;", [ test_schema, custom_dialect_table, test_schema, custom_dialect_table ])
session.run_sql("INSERT INTO !.! (p, q) SELECT p, q FROM !.!;", [ test_schema, custom_dialect_table, test_schema, custom_dialect_table ])
session.run_sql("INSERT INTO !.! (p, q) SELECT p, q FROM !.!;", [ test_schema, custom_dialect_table, test_schema, custom_dialect_table ])
session.run_sql("INSERT INTO !.! (p, q) SELECT p, q FROM !.!;", [ test_schema, custom_dialect_table, test_schema, custom_dialect_table ])
session.run_sql("INSERT INTO !.! (p, q) SELECT p, q FROM !.!;", [ test_schema, custom_dialect_table, test_schema, custom_dialect_table ])
session.run_sql("INSERT INTO !.! (p, q) SELECT p, q FROM !.!;", [ test_schema, custom_dialect_table, test_schema, custom_dialect_table ])
session.run_sql("INSERT INTO !.! (p, q) SELECT p, q FROM !.!;", [ test_schema, custom_dialect_table, test_schema, custom_dialect_table ])
session.run_sql("INSERT INTO !.! (p, q) SELECT p, q FROM !.!;", [ test_schema, custom_dialect_table, test_schema, custom_dialect_table ])
session.run_sql("INSERT INTO !.! (p, q) SELECT p, q FROM !.!;", [ test_schema, custom_dialect_table, test_schema, custom_dialect_table ])
session.run_sql("INSERT INTO !.! (p, q) SELECT p, q FROM !.!;", [ test_schema, custom_dialect_table, test_schema, custom_dialect_table ])
session.run_sql("INSERT INTO !.! (p, q) SELECT p, q FROM !.!;", [ test_schema, custom_dialect_table, test_schema, custom_dialect_table ])
session.run_sql("INSERT INTO !.! (p, q) SELECT p, q FROM !.!;", [ test_schema, custom_dialect_table, test_schema, custom_dialect_table ])

TEST_LOAD(test_schema, custom_dialect_table)
TEST_LOAD(test_schema, custom_dialect_table, { "fieldsTerminatedBy": "a" })
TEST_LOAD(test_schema, custom_dialect_table, { "linesTerminatedBy": "a" })
TEST_LOAD(test_schema, custom_dialect_table, { "fieldsEnclosedBy": '"', "fieldsOptionallyEnclosed": False , "linesTerminatedBy": "a"})
TEST_LOAD(test_schema, custom_dialect_table, { "fieldsEnclosedBy": '"', "fieldsOptionallyEnclosed": False , "linesTerminatedBy": "ab"})

session.run_sql("DROP TABLE !.!;", [ test_schema, custom_dialect_table ])

#@<> WL13804-FR5.9 - fixed-row format is not supported yet
EXPECT_FAIL("ValueError", "Argument #3: The fieldsTerminatedBy and fieldsEnclosedBy are both empty, resulting in a fixed-row format. This is currently not supported.", quote(types_schema, types_schema_tables[0]), test_output_absolute, { "fieldsTerminatedBy": "", "fieldsEnclosedBy": "" })

#@<> options param being a dictionary that contains an unknown key
EXPECT_FAIL("ValueError", "Argument #3: Invalid options: dummy", quote(types_schema, types_schema_tables[0]), test_output_relative, { "dummy": "fails" })
EXPECT_FAIL("ValueError", "Argument #3: Invalid options: indexColumn", quote(types_schema, types_schema_tables[0]), test_output_relative, { "indexColumn": "dummy" })
EXPECT_FAIL("ValueError", "Argument #3: Invalid options: consistent", quote(types_schema, types_schema_tables[0]), test_output_relative, { "consistent": "dummy" })
EXPECT_FAIL("ValueError", "Argument #3: Invalid options: triggers", quote(types_schema, types_schema_tables[0]), test_output_relative, { "triggers": "dummy" })
EXPECT_FAIL("ValueError", "Argument #3: Invalid options: events", quote(types_schema, types_schema_tables[0]), test_output_relative, { "events": "dummy" })
EXPECT_FAIL("ValueError", "Argument #3: Invalid options: routines", quote(types_schema, types_schema_tables[0]), test_output_relative, { "routines": "dummy" })
EXPECT_FAIL("ValueError", "Argument #3: Invalid options: users", quote(types_schema, types_schema_tables[0]), test_output_relative, { "users": "dummy" })
EXPECT_FAIL("ValueError", "Argument #3: Invalid options: excludeUsers", quote(types_schema, types_schema_tables[0]), test_output_relative, { "excludeUsers": "dummy" })
EXPECT_FAIL("ValueError", "Argument #3: Invalid options: includeUsers", quote(types_schema, types_schema_tables[0]), test_output_relative, { "includeUsers": "dummy" })
EXPECT_FAIL("ValueError", "Argument #3: Invalid options: tzUtc", quote(types_schema, types_schema_tables[0]), test_output_relative, { "tzUtc": "dummy" })
EXPECT_FAIL("ValueError", "Argument #3: Invalid options: ddlOnly", quote(types_schema, types_schema_tables[0]), test_output_relative, { "ddlOnly": "dummy" })
EXPECT_FAIL("ValueError", "Argument #3: Invalid options: dataOnly", quote(types_schema, types_schema_tables[0]), test_output_relative, { "dataOnly": "dummy" })
EXPECT_FAIL("ValueError", "Argument #3: Invalid options: dryRun", quote(types_schema, types_schema_tables[0]), test_output_relative, { "dryRun": "dummy" })
EXPECT_FAIL("ValueError", "Argument #3: Invalid options: chunking", quote(types_schema, types_schema_tables[0]), test_output_relative, { "chunking": "dummy" })
EXPECT_FAIL("ValueError", "Argument #3: Invalid options: bytesPerChunk", quote(types_schema, types_schema_tables[0]), test_output_relative, { "bytesPerChunk": "dummy" })
EXPECT_FAIL("ValueError", "Argument #3: Invalid options: threads", quote(types_schema, types_schema_tables[0]), test_output_relative, { "threads": "dummy" })
EXPECT_FAIL("ValueError", "Argument #3: Invalid options: excludeTables", quote(types_schema, types_schema_tables[0]), test_output_relative, { "excludeTables": "dummy" })
EXPECT_FAIL("ValueError", "Argument #3: Invalid options: excludeSchemas", quote(types_schema, types_schema_tables[0]), test_output_relative, { "excludeSchemas": "dummy" })
EXPECT_FAIL("ValueError", "Argument #3: Invalid options: ociParManifest", quote(types_schema, types_schema_tables[0]), test_output_relative, { "ociParManifest": "dummy" })
EXPECT_FAIL("ValueError", "Argument #3: Invalid options: ociParExpireTime", quote(types_schema, types_schema_tables[0]), test_output_relative, { "ociParExpireTime": "dummy" })

#@<> WL13804-FR15 - Once the dump is complete, the summary of the export process must be presented to the user. It must contain:
# * The number of rows written.
# * The number of bytes actually written to the data dump files.
# * The number of data bytes written to the data dump files. (only if `compression` option is not set to `none`)
# * The average throughout in bytes written per second.
EXPECT_SUCCESS(quote(types_schema, types_schema_tables[0]), test_output_absolute, { "compression": "zstd", "showProgress": False })

EXPECT_STDOUT_CONTAINS("Uncompressed data size: ")
EXPECT_STDOUT_CONTAINS("Compressed data size: ")
EXPECT_STDOUT_CONTAINS("Rows written: ")
EXPECT_STDOUT_CONTAINS("Bytes written: ")
EXPECT_STDOUT_CONTAINS("Average uncompressed throughput: ")
EXPECT_STDOUT_CONTAINS("Average compressed throughput: ")

# test without compression
EXPECT_SUCCESS(quote(types_schema, types_schema_tables[0]), test_output_absolute, { "showProgress": False })

EXPECT_STDOUT_CONTAINS("Data size:")
EXPECT_STDOUT_CONTAINS("Rows written: ")
EXPECT_STDOUT_CONTAINS("Bytes written: ")
EXPECT_STDOUT_CONTAINS("Average throughput: ")

#@<> WL13804-FR16 - All files should be created with permissions `rw-r-----`. {__os_type != "windows"}
# WL13804-TSFR_4_3_4
EXPECT_SUCCESS(quote(types_schema, types_schema_tables[0]), test_output_absolute, { "showProgress": False })

EXPECT_EQ(0o640, stat.S_IMODE(os.stat(test_output_absolute).st_mode))
EXPECT_EQ(os.getuid(), os.stat(test_output_absolute).st_uid)

#@<> test a single table
TEST_LOAD(world_x_schema, world_x_table)

#@<> test multiple tables
for table in test_schema_tables:
    TEST_LOAD(test_schema, table)

#@<> test multiple tables with various data types
for table in types_schema_tables:
    TEST_LOAD(types_schema, table)

#@<> dump table when different character set/SQL mode is used
session.run_sql("SET NAMES 'latin1';")
session.run_sql("SET GLOBAL SQL_MODE='ANSI_QUOTES,NO_AUTO_VALUE_ON_ZERO,NO_BACKSLASH_ESCAPES,NO_DIR_IN_CREATE,NO_ZERO_DATE,PAD_CHAR_TO_FULL_LENGTH';")

EXPECT_SUCCESS(quote(world_x_schema, world_x_table), test_output_absolute, { "defaultCharacterSet": "latin1", "showProgress": False })

session.run_sql("SET NAMES 'utf8mb4';")
# import table does not handle global SQL_MODE correctly, need to disable it first
session.run_sql("SET GLOBAL SQL_MODE='';")

recreate_verification_schema()
session.run_sql("CREATE TABLE !.! LIKE !.!;", [verification_schema, world_x_table, world_x_schema, world_x_table])
EXPECT_NO_THROWS(lambda: util.import_table(test_output_absolute, { "schema": verification_schema, "table": world_x_table, "characterSet": "latin1", "showProgress": False }), "importing latin1 data")

#@<> Bug #31188854: USING THE OCIPROFILE OPTION IN A DUMP MAKE THE DUMP TO ALWAYS FAIL {has_oci_environment('OS')}
# This error now confirms the reported issue is fixed
EXPECT_FAIL("RuntimeError", "Failed to get object list", quote(types_schema, types_schema_tables[0]), 'out.txt', {"osBucketName": "any-bucket", "ociProfile": "DEFAULT"})

#@<> An error should occur when dumping using oci+os://
EXPECT_FAIL("ValueError", "File handling for oci+os protocol is not supported.", quote(types_schema, types_schema_tables[0]), 'oci+os://sakila')

#@<> WL13804-TSFR_1_2
# WL13804-TSFR_1_3
tested_schema = 58 * "a"
# append tab character, identifier needs to be quoted
tested_schema = tested_schema + chr(9)
# append four random extended characters, we don't want to use too many, 'cause server may have problems creating the schema/table
# 64 characters in total
for i in range(1, 5):
    c = chr(0)
    while not c.isprintable():
        c = chr(random.randint(0x80, 0xFFFF))
    tested_schema = tested_schema + c
# identifier may not end with a space character
tested_schema = tested_schema + "a"

if __os_type == "windows":
    # dumper is using case-sensitive comparison, ensure schema name is lowercase
    tested_schema = tested_schema.lower()

tested_name = tested_schema

session.run_sql("CREATE SCHEMA !;", [tested_schema])
session.run_sql("CREATE TABLE !.! (! INT) ENGINE=InnoDB;", [ tested_schema, tested_name, tested_name ])
session.run_sql("INSERT INTO !.! (!) SELECT data FROM !.!;", [ tested_schema, tested_name, tested_name, test_schema, test_table_primary ])

TEST_LOAD(tested_schema, tested_name)

#@<> WL13804-TSFR_3_2
EXPECT_FAIL("ValueError", "Failed to parse table to be exported '{0}.{1}': Invalid object name, expected '.', but got: ".format(tested_schema, tested_name), "{0}.{1}".format(tested_schema, tested_name), test_output_absolute)

#@<> WL13804-TSFR_3_2_2
EXPECT_FAIL("ValueError", "The requested table `{0}`.`{1}a` was not found in the database.".format(tested_schema, tested_name), quote(tested_schema, tested_name + "a"), test_output_absolute)

#@<> WL13804-TSFR_1_1
# use 20 random ASCII characters, we don't want to use too many, 'cause server may have problems renaming the table
new_tested_name = ""
for i in range(1, 21):
    new_tested_name = new_tested_name + chr(random.randint(0x01, 0x7F))

# identifier may not end with a space character
new_tested_name = new_tested_name + "a"

if __os_type == "windows":
    # dumper is using case-sensitive comparison, ensure table name is lowercase
    new_tested_name = new_tested_name.lower()

session.run_sql("ALTER TABLE !.! CHANGE ! ! INT, RENAME TO !.!;", [ tested_schema, tested_name, tested_name, new_tested_name, tested_schema, new_tested_name ])

tested_name = new_tested_name

TEST_LOAD(tested_schema, tested_name)

#@<> WL13804-TSFR_2_1
# setup X session
setup_session(xuri)

# create test schema
tested_schema = "abcdef"
tested_name = tested_schema
session.run_sql("CREATE SCHEMA {0};".format(quote(tested_schema)))
session.run_sql("CREATE TABLE {0}.{1} ({1} INT) ENGINE=InnoDB;".format(quote(tested_schema), quote(tested_name)))

# start transaction, insert some data
session.start_transaction()
session.run_sql("INSERT INTO {0}.{1} ({1}) VALUES (123);".format(quote(tested_schema), quote(tested_name)))

# run export_table()
EXPECT_SUCCESS(quote(types_schema, types_schema_tables[0]), test_output_absolute, { "showProgress": False })

# transaction should be in progress, trying to change its characteristics should throw
EXPECT_THROWS(lambda: session.run_sql("SET TRANSACTION READ ONLY;"), "Transaction characteristics can't be changed while a transaction is in progress")

# commit and check data is there
session.commit()

tested_data = session.run_sql("SELECT * FROM {0}.{1};".format(quote(tested_schema), quote(tested_name))).fetch_all()
EXPECT_EQ(1, len(tested_data))
EXPECT_EQ(123, tested_data[0][0])

# delete schema
session.run_sql("DROP SCHEMA {0};".format(quote(tested_schema)))

#@<> WL13804-TSFR_2_2
# setup classic session
setup_session(uri)

# create test schema
tested_schema = "abcdef"
tested_name = tested_schema
session.run_sql("CREATE SCHEMA !;", [tested_schema])
session.run_sql("CREATE TABLE !.! (! INT) ENGINE=InnoDB;", [ tested_schema, tested_name, tested_name ])

# start transaction, insert some data
session.start_transaction()
session.run_sql("INSERT INTO !.! (!) VALUES (123);", [ tested_schema, tested_name, tested_name ])

# run export_table()
EXPECT_SUCCESS(quote(types_schema, types_schema_tables[0]), test_output_absolute, { "showProgress": False })

# transaction should be in progress, trying to change its characteristics should throw
EXPECT_THROWS(lambda: session.run_sql("SET TRANSACTION READ ONLY;"), "Transaction characteristics can't be changed while a transaction is in progress")

# commit and check data is there
session.commit()

tested_data = session.run_sql("SELECT * FROM !.!;", [ tested_schema, tested_name ]).fetch_all()
EXPECT_EQ(1, len(tested_data))
EXPECT_EQ(123, tested_data[0][0])

# delete schema
session.run_sql("DROP SCHEMA !;", [tested_schema])

#@<> WL13804-TSFR_3_3
session.run_sql("USE !;", [ test_schema ])

for table in ["a`b", "a'b", '"a"', "'a'"]:
    session.run_sql("CREATE TABLE !.! (`data` INT) ENGINE=InnoDB;", [ test_schema, table ])
    EXPECT_SUCCESS(quote(table), test_output_absolute, { "showProgress": False })
    session.run_sql("DROP TABLE !.!;", [ test_schema, table ])

#@<> WL13804-TSFR_3_4
session.run_sql("USE !;", [ test_schema ])
tested_name = '"a"'

session.run_sql("CREATE TABLE !.! (`data` INT) ENGINE=InnoDB;", [ test_schema, tested_name ])
session.run_sql("SET GLOBAL SQL_MODE='ANSI_QUOTES';")

EXPECT_SUCCESS(quote(tested_name), test_output_absolute, { "showProgress": False })

session.run_sql("DROP TABLE !.!;", [ test_schema, tested_name ])
session.run_sql("SET GLOBAL SQL_MODE='';")

#@<> WL13804-TSFR_3_5
session.run_sql("USE !;", [ test_schema ])
tested_name = "1234"

session.run_sql("CREATE TABLE !.! (`data` INT) ENGINE=InnoDB;", [ test_schema, tested_name ])

EXPECT_FAIL("ValueError", "Failed to parse table to be exported '{0}': Invalid identifier: identifiers may begin with a digit but unless quoted may not consist solely of digits.".format(tested_name), tested_name, test_output_absolute)

session.run_sql("DROP TABLE !.!;", [ test_schema, tested_name ])

#@<> WL13804-TSFR_3_6
tested_schema = "."
tested_name = "1234"

session.run_sql("CREATE SCHEMA !;", [ tested_schema ])
session.run_sql("CREATE TABLE !.! (`data` INT) ENGINE=InnoDB;", [ tested_schema, tested_name ])

EXPECT_FAIL("ValueError", "Failed to parse table to be exported '{0}.{1}': Invalid character in identifier".format(tested_schema, tested_name), "{0}.{1}".format(tested_schema, tested_name), test_output_absolute)

session.run_sql("DROP SCHEMA !;", [ tested_schema ])

#@<> WL13804-TSFR_3_7
tested_name = "a"

for schema in [".", "..", "`"]:
    session.run_sql("CREATE SCHEMA !;", [ schema ])
    session.run_sql("CREATE TABLE !.! (`data` INT) ENGINE=InnoDB;", [ schema, tested_name ])
    EXPECT_SUCCESS(quote(schema, tested_name), test_output_absolute, { "showProgress": False })
    session.run_sql("DROP SCHEMA !;", [ schema ])

#@<> WL13804-TSFR_3_1_3
used_schema = "schema_a"
tested_schema = "schema_b"
tested_name = "test"

session.run_sql("CREATE SCHEMA !;", [ used_schema ])
session.run_sql("CREATE SCHEMA !;", [ tested_schema ])
session.run_sql("CREATE TABLE !.! (`data` INT) ENGINE=InnoDB;", [ used_schema, tested_name ])
session.run_sql("CREATE TABLE !.! (`data` INT) ENGINE=InnoDB;", [ tested_schema, tested_name ])
session.run_sql("INSERT INTO !.! VALUES (123);", [ used_schema, tested_name ])
session.run_sql("INSERT INTO !.! VALUES (321);", [ tested_schema, tested_name ])

session.run_sql("USE !;", [ used_schema ])
EXPECT_SUCCESS(quote(tested_schema, tested_name), test_output_absolute, { "showProgress": False })
EXPECT_FILE_CONTAINS("321", test_output_absolute)

session.run_sql("DROP SCHEMA !;", [ used_schema ])
session.run_sql("DROP SCHEMA !;", [ tested_schema ])

#@<> WL13804-TSFR_3_1_4
used_schema = "schema_a"
tested_schema = "schema_b"
tested_name = "test"

session.run_sql("CREATE SCHEMA !;", [ used_schema ])
session.run_sql("CREATE SCHEMA !;", [ tested_schema ])
session.run_sql("CREATE TABLE !.! (`data` INT) ENGINE=InnoDB;", [ used_schema, tested_name ])
session.run_sql("CREATE TABLE !.! (`data` INT) ENGINE=InnoDB;", [ tested_schema, tested_name ])
session.run_sql("INSERT INTO !.! VALUES (123);", [ used_schema, tested_name ])
session.run_sql("INSERT INTO !.! VALUES (321);", [ tested_schema, tested_name ])

session.run_sql("USE !;", [ used_schema ])
\sql USE `<<<tested_schema>>>`
EXPECT_SUCCESS(quote(tested_name), test_output_absolute, { "showProgress": False })
EXPECT_FILE_CONTAINS("321", test_output_absolute)

session.run_sql("DROP SCHEMA !;", [ used_schema ])
session.run_sql("DROP SCHEMA !;", [ tested_schema ])


#@<> WL13804-TSFR_3_1_5
used_schema = "schema_a"
tested_name = "test"

session.run_sql("CREATE SCHEMA !;", [ used_schema ])
session.run_sql("CREATE TABLE !.! (`data` INT) ENGINE=InnoDB;", [ used_schema, tested_name ])
session.run_sql("INSERT INTO !.! VALUES (123);", [ used_schema, tested_name ])

session.run_sql("USE !;", [ used_schema ])
\sql DROP SCHEMA `<<<used_schema>>>`
EXPECT_FAIL("ValueError", "The table was given without a schema and there is no active schema on the current session, unable to deduce which table to export.", quote(tested_name), test_output_absolute)

#@<> WL13804-TSFR_4_2 {__os_type != "windows"}
# create the file
shutil.rmtree(test_output_absolute_parent, True)
os.mkdir(test_output_absolute_parent)
open(test_output_absolute, 'a').close()
# change permissions to write only
os.chmod(test_output_absolute, stat.S_IWUSR | stat.S_IWGRP | stat.S_IWOTH)
# expect success as file is not readable but it's still writeable
EXPECT_NO_THROWS(lambda: util.export_table(quote(types_schema, types_schema_tables[0]), test_output_absolute, { "showProgress": False }), "WL13804-TSFR_4_2")

#@<> WL13804-TSFR_4_3 {__os_type != "windows"}
# WL13804-TSFR_4_4_2 - plain file
# create the file
shutil.rmtree(test_output_absolute_parent, True)
os.mkdir(test_output_absolute_parent)
open(test_output_absolute, 'a').close()
# change permissions to read only
os.chmod(test_output_absolute, stat.S_IRUSR | stat.S_IRGRP | stat.S_IROTH)
# expect failure
EXPECT_THROWS(lambda: util.export_table(quote(types_schema, types_schema_tables[0]), test_output_absolute), "RuntimeError: Util.export_table: Fatal error during dump")
EXPECT_STDOUT_CONTAINS("Cannot open file '{0}': Permission denied".format(absolute_path_for_output(test_output_absolute)))

#@<> WL13804-TSFR_4_4_2 - compressed file {__os_type != "windows"}
EXPECT_THROWS(lambda: util.export_table(quote(types_schema, types_schema_tables[0]), test_output_absolute, { "compression": "zstd" }), "RuntimeError: Util.export_table: Fatal error during dump")
EXPECT_STDOUT_CONTAINS("Cannot open file '{0}': Permission denied".format(absolute_path_for_output(test_output_absolute)))

#@<> WL13804-TSFR_4_1_1
for f in ["file", "file_with_ space", 'file"' if __os_type != "windows" else "fileX", "file'", "file-", "file--"]:
    f = os.path.join(test_output_absolute_parent, f)
    print("----> testing {0}".format(f))
    shutil.rmtree(test_output_absolute_parent, True)
    os.mkdir(test_output_absolute_parent)
    EXPECT_FALSE(os.path.isfile(f))
    EXPECT_NO_THROWS(lambda: util.export_table(quote(types_schema, types_schema_tables[0]), f, { "showProgress": False }), "WL13804-TSFR_4_1_1: {0}".format(f))
    EXPECT_TRUE(os.path.isfile(f))

for f in ["file", os.path.join("deeply", "nested", "file"), os.path.join("loooooooooooooooooooong", "looooooong", "name")]:
    f = os.path.join(test_output_absolute_parent, f)
    for prefix in ["file", "FiLe", "FILE"]:
        prefix = prefix + "://" + f
        print("----> testing {0}".format(prefix))
        shutil.rmtree(test_output_absolute_parent, True)
        os.makedirs(os.path.dirname(f))
        EXPECT_FALSE(os.path.isfile(f))
        EXPECT_NO_THROWS(lambda: util.export_table(quote(types_schema, types_schema_tables[0]), prefix, { "showProgress": False }), "WL13804-TSFR_4_1_1: {0}".format(prefix))
        EXPECT_TRUE(os.path.isfile(f))

#@<> WL13804-TSFR_4_1_3 {__os_type != "windows"}
# create the file
shutil.rmtree(test_output_absolute_parent, True)
os.mkdir(test_output_absolute_parent)
open(test_output_absolute, 'a').close()
# create the symlink
tested_symlink = os.path.join(test_output_absolute_parent, "symlink")
os.symlink(test_output_absolute, tested_symlink)
# expect success
EXPECT_NO_THROWS(lambda: util.export_table(quote(types_schema, types_schema_tables[0]), tested_symlink, { "showProgress": False }), "WL13804-TSFR_4_1_3")

#@<> WL13804-TSFR_4_2_1
for f in [os.path.join("some", "..", "path", "file"), os.path.join("some", "path", "..", "file_with_ space"), os.path.join("some", "path", "..", "..", "file"), os.path.join("some", "path", ".", " file"), os.path.join("some", "path", "file'")]:
    f = os.path.join(test_output_relative_parent, f)
    print("----> testing {0}".format(f))
    shutil.rmtree(test_output_absolute_parent, True)
    os.makedirs(os.path.dirname(f), exist_ok=True)
    EXPECT_FALSE(os.path.isfile(f))
    EXPECT_NO_THROWS(lambda: util.export_table(quote(types_schema, types_schema_tables[0]), f, { "showProgress": False }), "WL13804-TSFR_4_2_1: {0}".format(f))
    EXPECT_TRUE(os.path.isfile(f))

for f in [os.path.join("some", "..", "path", "file"), os.path.join("deeply", "deeply", "..", "..", "nested", "file"), os.path.join("loooooooooooooooooooong", "looooooong", "name")]:
    f = os.path.join(test_output_relative_parent, f)
    for prefix in ["file", "FiLe", "FILE"]:
        prefix = prefix + "://" + f
        print("----> testing {0}".format(prefix))
        shutil.rmtree(test_output_absolute_parent, True)
        os.makedirs(os.path.dirname(f), exist_ok=True)
        EXPECT_FALSE(os.path.isfile(f))
        EXPECT_NO_THROWS(lambda: util.export_table(quote(types_schema, types_schema_tables[0]), prefix, { "showProgress": False }), "WL13804-TSFR_4_2_1: {0}".format(prefix))
        EXPECT_TRUE(os.path.isfile(f))

#@<> WL13804-TSFR_4_3_2 {__os_type != "windows"}
# create the directory
shutil.rmtree(test_output_absolute_parent, True)
os.mkdir(test_output_absolute_parent)
tested_dir = os.path.join(test_output_absolute_parent, "test")
os.mkdir(tested_dir)
# change permissions to read only
os.chmod(tested_dir, stat.S_IRUSR | stat.S_IRGRP | stat.S_IROTH)
# expect failure
EXPECT_THROWS(lambda: util.export_table(quote(types_schema, types_schema_tables[0]), os.path.join(tested_dir, "test")), "RuntimeError: Util.export_table: Fatal error during dump")
EXPECT_STDOUT_CONTAINS("Cannot open file '{0}': Permission denied".format(absolute_path_for_output(os.path.join(tested_dir, "test"))))

#@<> WL13804-TSFR_4_3_3
# create the directory
shutil.rmtree(test_output_absolute_parent, True)
os.mkdir(test_output_absolute_parent)
tested_path = os.path.join(test_output_absolute_parent, "deeply", "nested", "none", "of", "which", "exists")
EXPECT_THROWS(lambda: util.export_table(quote(types_schema, types_schema_tables[0]), tested_path), "ValueError: Util.export_table: Cannot proceed with the dump, the directory containing '{0}' does not exist at the target location {1}.".format(tested_path, absolute_path_for_output(os.path.dirname(tested_path))))

#@<> WL13804-TSFR_4_3_x
shutil.rmtree(test_output_absolute_parent, True)
os.mkdir(test_output_absolute_parent)

with open(os.path.join(test_output_absolute_parent, "file_a"), "w") as f:
    f.write("111")

with open(os.path.join(test_output_absolute_parent, "file_b"), "w") as f:
    f.write("222")

os.mkdir(os.path.join(test_output_absolute_parent, "nested"))

with open(os.path.join(test_output_absolute_parent, "nested", "file_c"), "w") as f:
    f.write("333")

EXPECT_NO_THROWS(lambda: util.export_table(quote(types_schema, types_schema_tables[0]), test_output_absolute, { "showProgress": False }), "WL13804-TSFR_4_3_x")

EXPECT_FILE_CONTAINS("111", os.path.join(test_output_absolute_parent, "file_a"))
EXPECT_FILE_CONTAINS("222", os.path.join(test_output_absolute_parent, "file_b"))
EXPECT_FILE_CONTAINS("333", os.path.join(test_output_absolute_parent, "nested", "file_c"))

#@<> BUG#31552502 empty outputUrl should be disallowed
EXPECT_FAIL("ValueError", "The name of the output file cannot be empty.", quote(types_schema, types_schema_tables[0]), "")
EXPECT_FAIL("ValueError", "The name of the output file cannot be empty.", quote(types_schema, types_schema_tables[0]), "file://")
EXPECT_FAIL("ValueError", "The name of the output file cannot be empty.", quote(types_schema, types_schema_tables[0]), "FIle://")

#@<> BUG#31545679
# setup
tested_schema = "test"
tested_table= "t1"
session.run_sql("CREATE SCHEMA !;", [tested_schema])
session.run_sql("CREATE TABLE !.! (id INT NOT NULL PRIMARY KEY AUTO_INCREMENT, something BINARY)", [ tested_schema, tested_table ])
session.run_sql("INSERT INTO !.! (id) values (302)", [ tested_schema, tested_table ])
session.run_sql("INSERT INTO !.! (something) values (char(0))", [ tested_schema, tested_table ])

# export table data
EXPECT_SUCCESS(quote(tested_schema, tested_table), test_output_absolute, { "linesTerminatedBy": "\n", "fieldsTerminatedBy": "\0", "showProgress": False })

# capture and update the import command
output = testutil.fetch_captured_stdout(False)
index = output.find("util.")
EXPECT_NE(-1, index)
code = output[index:]
code = code.replace('"schema": "{0}"'.format(tested_schema), '"schema": "{0}"'.format(verification_schema))

# prepare verification table
recreate_verification_schema()
session.run_sql("CREATE TABLE !.! LIKE !.!", [ verification_schema, tested_table, tested_schema, tested_table ])

# import data
EXPECT_NO_THROWS(lambda: exec(code), "importing data")

# check data
all_columns = ["id", "something"]
EXPECT_EQ(compute_crc(tested_schema, tested_table, all_columns), compute_crc(verification_schema, tested_table, all_columns))

#@<> Cleanup
drop_all_schemas()
session.run_sql("SET GLOBAL local_infile = false;")
session.close()
testutil.destroy_sandbox(__mysql_sandbox_port1);
