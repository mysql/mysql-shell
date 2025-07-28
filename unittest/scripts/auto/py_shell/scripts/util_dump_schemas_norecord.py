#@<> INCLUDE dump_utils.inc

#@<> entry point

# imports
import json
import os
import os.path
import re
import shutil
import stat
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
test_schema_library = "sample_library"
test_schema_library_procedure = "sample_library_procedure"
test_schema_library_function = "sample_library_function"
test_view = "sample_view"
test_privilege = "FILE"

verification_schema = "wl13807_ver"

types_schema = "xtest"

incompatible_schema = "mysqlaas"
incompatible_table_wrong_engine = "wrong_engine"
incompatible_table_encryption = "has_encryption"
incompatible_table_data_directory = "has_data_dir"
incompatible_table_index_directory = "has_index_dir"
incompatible_tablespace = "sample_tablespace"
incompatible_table_tablespace = "use_tablespace"
incompatible_view = "has_definer"

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

uri = __sandbox_uri1
xuri = __sandbox_uri1.replace("mysql://", "mysqlx://") + "0"

test_output_relative = "dump_output"
test_output_absolute = os.path.abspath(test_output_relative)

# helpers
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
        session.run_sql("CREATE SCHEMA IF NOT EXISTS !;", [ schema ])

def create_user():
    session.run_sql(f"DROP USER IF EXISTS {test_user_account};")
    session.run_sql(f"CREATE USER IF NOT EXISTS {test_user_account} IDENTIFIED BY ?;", [test_user_pwd])
    session.run_sql(f"GRANT {test_privilege} ON *.* TO {test_user_account};")

def recreate_verification_schema():
    session.run_sql("DROP SCHEMA IF EXISTS !;", [ verification_schema ])
    session.run_sql("CREATE SCHEMA !;", [ verification_schema ])

def EXPECT_SUCCESS(schemas, outputUrl, options = {}):
    WIPE_STDOUT()
    shutil.rmtree(test_output_absolute, True)
    EXPECT_FALSE(os.path.isdir(test_output_absolute))
    util.dump_schemas(schemas, outputUrl, options)
    if not "dryRun" in options:
        EXPECT_TRUE(os.path.isdir(test_output_absolute))
        EXPECT_STDOUT_CONTAINS("Schemas dumped: {0}".format(len(schemas)))

def EXPECT_FAIL(error, msg, schemas, outputUrl, options = {}, expect_dir_created = False):
    shutil.rmtree(test_output_absolute, True)
    is_re = is_re_instance(msg)
    full_msg = "{0}: {1}".format(re.escape(error) if is_re else error, msg.pattern if is_re else msg)
    if is_re:
        full_msg = re.compile("^" + full_msg)
    EXPECT_THROWS(lambda: util.dump_schemas(schemas, outputUrl, options), full_msg)
    EXPECT_EQ(expect_dir_created, os.path.isdir(test_output_absolute))

def TEST_BOOL_OPTION(option):
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type Bool, but is Null".format(option), [types_schema], test_output_relative, { option: None })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' Bool expected, but value is String".format(option), [types_schema], test_output_relative, { option: "dummy" })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type Bool, but is Array".format(option), [types_schema], test_output_relative, { option: [] })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type Bool, but is Map".format(option), [types_schema], test_output_relative, { option: {} })

def TEST_STRING_OPTION(option):
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type String, but is Null".format(option), [types_schema], test_output_relative, { option: None })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type String, but is Integer".format(option), [types_schema], test_output_relative, { option: 5 })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type String, but is Integer".format(option), [types_schema], test_output_relative, { option: -5 })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type String, but is Array".format(option), [types_schema], test_output_relative, { option: [] })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type String, but is Map".format(option), [types_schema], test_output_relative, { option: {} })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type String, but is Bool".format(option), [types_schema], test_output_relative, { option: False })

def TEST_UINT_OPTION(option):
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type UInteger, but is Null".format(option), [types_schema], test_output_relative, { option: None })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' UInteger expected, but Integer value is out of range".format(option), [types_schema], test_output_relative, { option: -5 })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' UInteger expected, but value is String".format(option), [types_schema], test_output_relative, { option: "dummy" })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type UInteger, but is Array".format(option), [types_schema], test_output_relative, { option: [] })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type UInteger, but is Map".format(option), [types_schema], test_output_relative, { option: {} })

def TEST_ARRAY_OF_STRINGS_OPTION(option):
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type Array, but is Integer".format(option), [types_schema], test_output_relative, { option: 5 })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type Array, but is Integer".format(option), [types_schema], test_output_relative, { option: -5 })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type Array, but is String".format(option), [types_schema], test_output_relative, { option: "dummy" })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type Array, but is Map".format(option), [types_schema], test_output_relative, { option: {} })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type Array, but is Bool".format(option), [types_schema], test_output_relative, { option: False })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be an array of strings".format(option), [types_schema], test_output_relative, { option: [ None ] })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be an array of strings".format(option), [types_schema], test_output_relative, { option: [ 5 ] })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be an array of strings".format(option), [types_schema], test_output_relative, { option: [ -5 ] })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be an array of strings".format(option), [types_schema], test_output_relative, { option: [ {} ] })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be an array of strings".format(option), [types_schema], test_output_relative, { option: [ False ] })

def TEST_MAP_OF_STRINGS_OPTION(option):
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type Map, but is Integer".format(option), [types_schema], test_output_relative, { option: 5 })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type Map, but is Integer".format(option), [types_schema], test_output_relative, { option: -5 })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type Map, but is String".format(option), [types_schema], test_output_relative, { option: "dummy" })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type Map, but is Array".format(option), [types_schema], test_output_relative, { option: [] })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type Map, but is Bool".format(option), [types_schema], test_output_relative, { option: False })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be a map of strings".format(option), [types_schema], test_output_relative, { option: { "key": None } })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be a map of strings".format(option), [types_schema], test_output_relative, { option: { "key": 5 } })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be a map of strings".format(option), [types_schema], test_output_relative, { option: { "key": -5 } })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be a map of strings".format(option), [types_schema], test_output_relative, { option: { "key": {} } })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be a map of strings".format(option), [types_schema], test_output_relative, { option: { "key": False } })

def TEST_MAP_OF_ARRAY_OF_STRINGS_OPTION(option):
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type Map, but is Integer".format(option), [types_schema], test_output_relative, { option: 5 })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type Map, but is Integer".format(option), [types_schema], test_output_relative, { option: -5 })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type Map, but is String".format(option), [types_schema], test_output_relative, { option: "dummy" })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type Map, but is Array".format(option), [types_schema], test_output_relative, { option: [] })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be of type Map, but is Bool".format(option), [types_schema], test_output_relative, { option: False })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be a map of arrays of strings".format(option), [types_schema], test_output_relative, { option: { "key": 5 } })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be a map of arrays of strings".format(option), [types_schema], test_output_relative, { option: { "key": -5 } })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be a map of arrays of strings".format(option), [types_schema], test_output_relative, { option: { "key": {} } })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be a map of arrays of strings".format(option), [types_schema], test_output_relative, { option: { "key": False } })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be a map of arrays of strings".format(option), [types_schema], test_output_relative, { option: { "key": [ 5 ] } })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be a map of arrays of strings".format(option), [types_schema], test_output_relative, { option: { "key": [ -5 ] } })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be a map of arrays of strings".format(option), [types_schema], test_output_relative, { option: { "key": [ {} ] } })
    EXPECT_FAIL("TypeError", "Argument #3: Option '{0}' is expected to be a map of arrays of strings".format(option), [types_schema], test_output_relative, { option: { "key": [ False ] } })

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

def compute_crc(schema, table, columns):
    session.run_sql("SET @crc = '';")
    session.run_sql("SELECT @crc := MD5(CONCAT_WS('#',@crc,{0})) FROM !.! ORDER BY {0};".format(("!," * len(columns))[:-1]), columns + [schema, table] + columns)
    return session.run_sql("SELECT @crc;").fetch_one()[0]

def TEST_LOAD(schema, table, where = "", partitions = [], recreate_schema = True):
    print("---> testing: `{0}`.`{1}`".format(schema, table))
    # load data
    if recreate_schema:
        recreate_verification_schema()
    EXPECT_NO_THROWS(lambda: util.load_dump(test_output_absolute, { "showProgress": False, "loadUsers": False, "includeSchemas": [ schema ], "includeTables" : [ quote_identifier(schema, table) ], "schema": verification_schema, "resetProgress": True }), "loading should not throw")
    # compute CRC
    EXPECT_EQ(md5_table(session, schema, table, where, partitions), md5_table(session, verification_schema, table))

def TEST_DUMP_AND_LOAD(schemas, options = {}):
    EXPECT_SUCCESS(schemas, test_output_absolute, options)
    where = options.get("where", {})
    partitions = options.get("partitions", {})
    recreate_verification_schema()
    for schema in schemas:
        for row in session.run_sql("SELECT TABLE_NAME FROM information_schema.tables WHERE table_schema = ? and table_type = 'BASE TABLE'", [schema]).fetch_all():
            table = row[0]
            quoted = quote_identifier(schema, table)
            TEST_LOAD(schema, table, where.get(quoted, ""), partitions.get(quoted, []), False)

#@<> WL13807-FR2.4 - an exception must be thrown if there is no global session
EXPECT_FAIL("RuntimeError", "An open session is required to perform this operation.", ['mysql'], test_output_relative)

#@<> deploy sandbox
testutil.deploy_raw_sandbox(__mysql_sandbox_port1, "root", {
    "loose_innodb_directories": filename_for_file(table_data_directory),
    "early-plugin-load": "keyring_file." + ("dll" if __os_type == "windows" else "so"),
    "keyring_file_data": filename_for_file(os.path.join(incompatible_table_directory, "keyring")),
    "server_id": str(random.randint(1, 4294967295)),
    "log_bin": 1,
    "enforce_gtid_consistency": "ON",
    "gtid_mode": "ON",
    "innodb_doublewrite": "OFF"
})

#@<> wait for server
testutil.wait_sandbox_alive(uri)
shell.connect(uri)
session.run_sql("/*!80021 alter instance disable innodb redo_log */")

#@<> WL13807-FR2.4 - an exception must be thrown if there is no open global session
session.close()
EXPECT_FAIL("RuntimeError", "An open session is required to perform this operation.", ['mysql'], test_output_relative)

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

if instance_supports_libraries:
    # make sure that the MLE component is disabled, tests will enable it when needed
    # WL16731-TSFR_1_1_2 - tests are executed with MLE component disabled
    disable_mle(session)
    session.run_sql("""
        CREATE LIBRARY !.! LANGUAGE JAVASCRIPT
        AS $$
            export function squared(n) {
                return n * n;
            }
        $$
        """, [ test_schema, test_schema_library ])
    session.run_sql("""
        CREATE FUNCTION !.!(n INTEGER) RETURNS INTEGER DETERMINISTIC LANGUAGE JAVASCRIPT
        USING (!.! AS mylib)
        AS $$
            return mylib.squared(n);
        $$
        """, [ test_schema, test_schema_library_function, test_schema, test_schema_library ])
    session.run_sql(f"""
        CREATE PROCEDURE !.!(n INTEGER) LANGUAGE JAVASCRIPT
        USING (!.!)
        AS $$
            {test_schema_library}.squared(n);
        $$
        """, [ test_schema, test_schema_library_procedure, test_schema, test_schema_library ])

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

#@<> first parameter
EXPECT_FAIL("TypeError", "Argument #1 is expected to be an array of strings", None, test_output_relative)
EXPECT_FAIL("TypeError", "Argument #1 is expected to be an array", 1, test_output_relative)
EXPECT_FAIL("TypeError", "Argument #1 is expected to be an array", types_schema, test_output_relative)
EXPECT_FAIL("TypeError", "Argument #1 is expected to be an array", {}, test_output_relative)
EXPECT_FAIL("TypeError", "Argument #1 is expected to be an array of strings", [1], test_output_relative)

#@<> WL13807-TSFR_2_3_3 - second parameter
EXPECT_FAIL("TypeError", "Argument #2 is expected to be a string", [types_schema], None)
EXPECT_FAIL("TypeError", "Argument #2 is expected to be a string", [types_schema], 1)
EXPECT_FAIL("TypeError", "Argument #2 is expected to be a string", [types_schema], [])
EXPECT_FAIL("TypeError", "Argument #2 is expected to be a string", [types_schema], {})
EXPECT_FAIL("TypeError", "Argument #2 is expected to be a string", [types_schema], True)

#@<> WL13807-TSFR_3_1 - third parameter
EXPECT_FAIL("TypeError", "Argument #3 is expected to be a map", [types_schema], test_output_relative, 1)
EXPECT_FAIL("TypeError", "Argument #3 is expected to be a map", [types_schema], test_output_relative, "string")
EXPECT_FAIL("TypeError", "Argument #3 is expected to be a map", [types_schema], test_output_relative, [])

#@<> WL13807-TSFR_2_2 - Call dumpSchemas(): giving less parameters than allowed, giving more parameters than allowed
EXPECT_THROWS(lambda: util.dump_schemas(), "ValueError: Invalid number of arguments, expected 2 to 3 but got 0")
EXPECT_THROWS(lambda: util.dump_schemas(["mysql"], test_output_relative, {}, None), "ValueError: Invalid number of arguments, expected 2 to 3 but got 4")

#@<> WL13807-FR2.1 - If the `schemas` parameter is an empty list, an exception must be thrown.
EXPECT_FAIL("ValueError", "The 'schemas' parameter cannot be an empty list.", [], test_output_relative)

#@<> WL13807-FR2.2 - If the `schemas` parameter contains a name of the schema that does not exist, an exception must be thrown.
EXPECT_FAIL("ValueError", "Following schemas were not found in the database: 'mysqlmysql'", ["mysqlmysql"], test_output_relative)
EXPECT_FAIL("ValueError", "Following schemas were not found in the database: 'mysqlmysql'", [types_schema, "mysqlmysql"], test_output_relative)
EXPECT_FAIL("ValueError", "Following schemas were not found in the database: 'mysqlmysql'", ["mysqlmysql", types_schema], test_output_relative)
# WL13807-TSFR_2_1_1
EXPECT_FAIL("ValueError", "Following schemas were not found in the database: 'dummy', 'mysqlmysql'", ["mysqlmysql", types_schema, "dummy"], test_output_relative)
# WL13807-TSFR_2_1_2
EXPECT_FAIL("ValueError", "Following schemas were not found in the database: ''", [""], test_output_relative)

# WL13807-FR2.3 - The `outputUrl` parameter must be handled the same as specified in FR1.1, FR1.1.1 - FR1.1.6.

#@<> WL13807-FR1.1 - The `outputUrl` parameter must be a string value which specifies the output directory, where the dump data is going to be stored.
EXPECT_SUCCESS([types_schema], test_output_absolute, { "ddlOnly": True, "showProgress": False })

#@<> WL13807-FR1.1.1 - If the dump is going to be stored on the local filesystem, the `outputUrl` parameter may be optionally prefixed with `file://` scheme.
EXPECT_SUCCESS([types_schema], "file://" + test_output_absolute, { "ddlOnly": True, "showProgress": False })

#@<> WL13807-FR1.1.2 - If the dump is going to be stored on the local filesystem and the `outputUrl` parameter holds a relative path, its absolute value is computed as relative to the current working directory.
EXPECT_SUCCESS([types_schema], test_output_relative, { "ddlOnly": True, "showProgress": False })

#@<> WL13807-FR1.1.2 - relative with .
EXPECT_SUCCESS([types_schema], "./" + test_output_relative, { "ddlOnly": True, "showProgress": False })

#@<> WL13807-FR1.1.2 - relative with ..
EXPECT_SUCCESS([types_schema], "dummy/../" + test_output_relative, { "ddlOnly": True, "showProgress": False })

#@<> WL13807-FR1.1.1 + WL13807-FR1.1.2
EXPECT_SUCCESS([types_schema], "file://" + test_output_relative, { "ddlOnly": True, "showProgress": False })

#@<> WL13807-FR1.1.3 - If the output directory does not exist, it must be created if its parent directory exists. If it is not possible or parent directory does not exist, an exception must be thrown.
# parent directory does not exist
shutil.rmtree(test_output_absolute, True)
EXPECT_FALSE(os.path.isdir(test_output_absolute))
EXPECT_FAIL("RuntimeError", "Could not create directory ", [types_schema], os.path.join(test_output_absolute, "dummy"), { "showProgress": False })
EXPECT_FALSE(os.path.isdir(test_output_absolute))

# unable to create directory, file with the same name exists
shutil.rmtree(test_output_absolute, True)
EXPECT_FALSE(os.path.isdir(test_output_absolute))
open(test_output_relative, 'a').close()
EXPECT_FAIL("RuntimeError", "Could not create directory ", [types_schema], test_output_absolute, { "showProgress": False })
EXPECT_FALSE(os.path.isdir(test_output_absolute))
os.remove(test_output_relative)

#@<> WL13807-FR1.1.4 - If a local directory is used and the output directory must be created, `rwxr-x---` permissions should be used on the created directory. The owner of the directory is the user running the Shell. {__os_type != "windows"}
EXPECT_SUCCESS([types_schema], test_output_absolute, { "ddlOnly": True, "showProgress": False })
# get the umask value
umask = os.umask(0o777)
os.umask(umask)
# use the umask value to compute the expected access rights
EXPECT_EQ(0o750 & ~umask, stat.S_IMODE(os.stat(test_output_absolute).st_mode))
EXPECT_EQ(os.getuid(), os.stat(test_output_absolute).st_uid)

#@<> WL13807-FR1.1.5 - If the output directory exists and is not empty, an exception must be thrown.
# dump into empty directory
shutil.rmtree(test_output_absolute, True)
EXPECT_FALSE(os.path.isdir(test_output_absolute))
os.mkdir(test_output_absolute)
util.dump_schemas([types_schema], test_output_absolute, { "ddlOnly": True, "showProgress": False })
EXPECT_STDOUT_CONTAINS("Schemas dumped: 1")

#@<> dump once again to the same directory, should fail
EXPECT_THROWS(lambda: util.dump_schemas([types_schema], test_output_relative, { "showProgress": False }), "ValueError: Cannot proceed with the dump, the specified directory '{0}' already exists at the target location {1} and is not empty.".format(test_output_relative, absolute_path_for_output(test_output_absolute)))

# WL13807-FR3 - Both new functions must accept the following options specified in WL#13804, FR5:
# * The `maxRate` option specified in WL#13804, FR5.1.
# * The `showProgress` option specified in WL#13804, FR5.2.
# * The `compression` option specified in WL#13804, FR5.3, with the modification of FR5.3.2, the default value must be`"zstd"`.
# * The `osBucketName` option specified in WL#13804, FR5.4.
# * The `osNamespace` option specified in WL#13804, FR5.5.
# * The `ociConfigFile` option specified in WL#13804, FR5.6.
# * The `ociProfile` option specified in WL#13804, FR5.7.
# * The `defaultCharacterSet` option specified in WL#13804, FR5.8.

#@<> WWL13807-FR4.12 - The `options` dictionary may contain a `chunking` key with a Boolean value, which specifies whether to split the dump data into multiple files.
# WL13807-TSFR_3_52
TEST_BOOL_OPTION("chunking")

EXPECT_SUCCESS([test_schema], test_output_absolute, { "chunking": False, "showProgress": False })
EXPECT_FALSE(has_file_with_basename(test_output_absolute, encode_table_basename(test_schema, test_table_primary) + "@"))
EXPECT_FALSE(has_file_with_basename(test_output_absolute, encode_table_basename(test_schema, test_table_unique) + "@"))

#@<> WL13807-FR4.12.3 - If the `chunking` option is not given, a default value of `true` must be used instead.
WIPE_SHELL_LOG()
EXPECT_SUCCESS([test_schema], test_output_absolute, { "showProgress": False })

# WL13807-FR4.12.1 - If the `chunking` option is set to `true` and the index column cannot be selected automatically as described in FR3.1, the data must to written to a single dump file. A warning should be displayed to the user.
# WL13807-FR3.1 - For each table dumped, its index column (name of the column used to order the data and perform the chunking) must be selected automatically as the first column used in the primary key, or if there is no primary key, as the first column used in the first unique index. If the table to be dumped does not contain a primary key and does not contain an unique index, the index column will not be defined.
# WL13807-TSFR_3_521_1
# BUG#34195250 - if index column cannot be selected, table is dumped to multiple files by a single thread
EXPECT_SHELL_LOG_CONTAINS(f"Could not select columns to be used as an index for table `{test_schema}`.`{test_table_non_unique}`. Data will be dumped to multiple files by a single thread.")
EXPECT_SHELL_LOG_CONTAINS(f"Could not select columns to be used as an index for table `{test_schema}`.`{test_table_no_index}`. Data will be dumped to multiple files by a single thread.")

EXPECT_TRUE(has_file_with_basename(test_output_absolute, encode_table_basename(test_schema, test_table_non_unique) + "@"))
EXPECT_TRUE(has_file_with_basename(test_output_absolute, encode_table_basename(test_schema, test_table_no_index) + "@"))

# WL13807-FR4.12.2 - If the `chunking` option is set to `true` and the index column can be selected automatically as described in FR3.1, the data must to written to multiple dump files. The data is partitioned into chunks using values from the index column.
# WL13807-TSFR_3_522_1
EXPECT_TRUE(has_file_with_basename(test_output_absolute, encode_table_basename(test_schema, test_table_primary) + "@"))
# WL13807-TSFR_3_522_3
EXPECT_TRUE(has_file_with_basename(test_output_absolute, encode_table_basename(test_schema, test_table_unique) + "@"))
number_of_dump_files = count_files_with_basename(test_output_absolute, encode_table_basename(test_schema, test_table_primary) + "@")

#@<> WL13807-FR4.13 - The `options` dictionary may contain a `bytesPerChunk` key with a string value, which specifies the average estimated number of bytes to be written per each data dump file.
TEST_STRING_OPTION("bytesPerChunk")

EXPECT_SUCCESS([test_schema], test_output_absolute, { "bytesPerChunk": "128k", "showProgress": False })

# WL13807-FR4.13.1 - If the `bytesPerChunk` option is given, it must implicitly set the `chunking` option to `true`.
# WL13807-TSFR_3_531_1
EXPECT_TRUE(has_file_with_basename(test_output_absolute, encode_table_basename(test_schema, test_table_primary) + "@"))
EXPECT_TRUE(has_file_with_basename(test_output_absolute, encode_table_basename(test_schema, test_table_unique) + "@"))

# WL13807-FR4.13.4 - If the `bytesPerChunk` option is not given and the `chunking` option is set to `true`, a default value of `256M` must be used instead.
# this dump used smaller chunk size, number of files should be greater
EXPECT_TRUE(count_files_with_basename(test_output_absolute, encode_table_basename(test_schema, test_table_primary) + "@") > number_of_dump_files)

#@<> WL13807-FR4.13.2 - The value of the `bytesPerChunk` option must use the same format as specified in WL#12193.
EXPECT_FAIL("ValueError", 'Argument #3: Wrong input number "xyz"', [types_schema], test_output_absolute, { "bytesPerChunk": "xyz" })
EXPECT_FAIL("ValueError", 'Argument #3: Wrong input number "1xyz"', [types_schema], test_output_absolute, { "bytesPerChunk": "1xyz" })
EXPECT_FAIL("ValueError", 'Argument #3: Wrong input number "2Mhz"', [types_schema], test_output_absolute, { "bytesPerChunk": "2Mhz" })
# WL13807-TSFR_3_532_3
EXPECT_FAIL("ValueError", 'Argument #3: Wrong input number "0.1k"', [types_schema], test_output_absolute, { "bytesPerChunk": "0.1k" })
EXPECT_FAIL("ValueError", 'Argument #3: Wrong input number "0,3M"', [types_schema], test_output_absolute, { "bytesPerChunk": "0,3M" })
EXPECT_FAIL("ValueError", 'Argument #3: Input number "-1G" cannot be negative', [types_schema], test_output_absolute, { "bytesPerChunk": "-1G" })
EXPECT_FAIL("ValueError", "Argument #3: The option 'bytesPerChunk' cannot be set to an empty string.", [types_schema], test_output_absolute, { "bytesPerChunk": "" })
# WL13807-TSFR_3_532_2
EXPECT_FAIL("ValueError", "Argument #3: The option 'bytesPerChunk' cannot be used if the 'chunking' option is set to false.", [types_schema], test_output_absolute, { "bytesPerChunk": "128k", "chunking": False })

#@<> WL13807-TSFR_3_532_1
EXPECT_SUCCESS([types_schema], test_output_absolute, { "bytesPerChunk": "1000k", "ddlOnly": True, "showProgress": False })
EXPECT_SUCCESS([types_schema], test_output_absolute, { "bytesPerChunk": "1M", "ddlOnly": True, "showProgress": False })
EXPECT_SUCCESS([types_schema], test_output_absolute, { "bytesPerChunk": "1G", "ddlOnly": True, "showProgress": False })
EXPECT_SUCCESS([types_schema], test_output_absolute, { "bytesPerChunk": "128000", "ddlOnly": True, "showProgress": False })

#@<> WL13807-FR4.13.3 - If the value of the `bytesPerChunk` option is smaller than `128k`, an exception must be thrown.
# WL13807-TSFR_3_533_1
EXPECT_FAIL("ValueError", "Argument #3: The value of 'bytesPerChunk' option must be greater than or equal to 128k.", [types_schema], test_output_relative, { "bytesPerChunk": "127k" })
EXPECT_FAIL("ValueError", "Argument #3: The value of 'bytesPerChunk' option must be greater than or equal to 128k.", [types_schema], test_output_relative, { "bytesPerChunk": "127999" })
EXPECT_FAIL("ValueError", "Argument #3: The value of 'bytesPerChunk' option must be greater than or equal to 128k.", [types_schema], test_output_relative, { "bytesPerChunk": "1" })
EXPECT_FAIL("ValueError", "Argument #3: The value of 'bytesPerChunk' option must be greater than or equal to 128k.", [types_schema], test_output_relative, { "bytesPerChunk": "0" })
EXPECT_FAIL("ValueError", 'Argument #3: Input number "-1" cannot be negative', [types_schema], test_output_relative, { "bytesPerChunk": "-1" })

#@<> WL13807-FR4.14 - The `options` dictionary may contain a `threads` key with an unsigned integer value, which specifies the number of threads to be used to perform the dump.
# WL13807-TSFR_3_54
TEST_UINT_OPTION("threads")

# WL13807-TSFR_3_54_1
EXPECT_SUCCESS([types_schema], test_output_absolute, { "threads": 2, "ddlOnly": True, "showProgress": False })
EXPECT_STDOUT_CONTAINS("Running data dump using 2 threads.")

#@<> WL13807-FR4.14.1 - If the value of the `threads` option is set to `0`, an exception must be thrown.
# WL13807-TSFR_3_54
EXPECT_FAIL("ValueError", "Argument #3: The value of 'threads' option must be greater than 0.", [types_schema], test_output_relative, { "threads": 0 })

#@<> WL13807-FR4.14.2 - If the `threads` option is not given, a default value of `4` must be used instead.
# WL13807-TSFR_3_542
EXPECT_SUCCESS([types_schema], test_output_absolute, { "ddlOnly": True, "showProgress": False })
EXPECT_STDOUT_CONTAINS("Running data dump using 4 threads.")

#@<> WL13807: WL13804-FR5.1 - The `options` dictionary may contain a `maxRate` key with a string value, which specifies the limit of data read throughput in bytes per second per thread.
TEST_STRING_OPTION("maxRate")

# WL13807-TSFR_3_551_1
EXPECT_SUCCESS([types_schema], test_output_absolute, { "maxRate": "1", "ddlOnly": True, "showProgress": False })
EXPECT_SUCCESS([types_schema], test_output_absolute, { "maxRate": "1000k", "ddlOnly": True, "showProgress": False })
EXPECT_SUCCESS([types_schema], test_output_absolute, { "maxRate": "1000K", "ddlOnly": True, "showProgress": False })
EXPECT_SUCCESS([types_schema], test_output_absolute, { "maxRate": "1M", "ddlOnly": True, "showProgress": False })
EXPECT_SUCCESS([types_schema], test_output_absolute, { "maxRate": "1G", "ddlOnly": True, "showProgress": False })

#@<> WL13807: WL13804-FR5.1.1 - The value of the `maxRate` option must use the same format as specified in WL#12193.
EXPECT_FAIL("ValueError", 'Argument #3: Wrong input number "xyz"', [types_schema], test_output_absolute, { "maxRate": "xyz" })
EXPECT_FAIL("ValueError", 'Argument #3: Wrong input number "1xyz"', [types_schema], test_output_absolute, { "maxRate": "1xyz" })
EXPECT_FAIL("ValueError", 'Argument #3: Wrong input number "2Mhz"', [types_schema], test_output_absolute, { "maxRate": "2Mhz" })
# WL13807-TSFR_3_552
EXPECT_FAIL("ValueError", 'Argument #3: Wrong input number "hello world!"', [types_schema], test_output_absolute, { "maxRate": "hello world!" })
EXPECT_FAIL("ValueError", 'Argument #3: Input number "-1" cannot be negative', [types_schema], test_output_absolute, { "maxRate": "-1" })
EXPECT_FAIL("ValueError", 'Argument #3: Input number "-1234567890123456" cannot be negative', [types_schema], test_output_absolute, { "maxRate": "-1234567890123456" })
EXPECT_FAIL("ValueError", 'Argument #3: Input number "-2K" cannot be negative', [types_schema], test_output_absolute, { "maxRate": "-2K" })
EXPECT_FAIL("ValueError", 'Argument #3: Wrong input number "3m"', [types_schema], test_output_absolute, { "maxRate": "3m" })
EXPECT_FAIL("ValueError", 'Argument #3: Wrong input number "4g"', [types_schema], test_output_absolute, { "maxRate": "4g" })

EXPECT_SUCCESS([types_schema], test_output_absolute, { "maxRate": "1000000", "ddlOnly": True, "showProgress": False })

#@<> WL13807: WL13804-FR5.1.1 - kilo.
EXPECT_SUCCESS([types_schema], test_output_absolute, { "maxRate": "1000k", "ddlOnly": True, "showProgress": False })

#@<> WL13807: WL13804-FR5.1.1 - giga.
EXPECT_SUCCESS([types_schema], test_output_absolute, { "maxRate": "1G", "ddlOnly": True, "showProgress": False })

#@<> WL13807: WL13804-FR5.1.2 - If the `maxRate` option is set to `"0"` or to an empty string, the read throughput must not be limited.
# WL13807-TSFR_3_552
EXPECT_SUCCESS([types_schema], test_output_absolute, { "maxRate": "0", "ddlOnly": True, "showProgress": False })

#@<> WL13807: WL13804-FR5.1.2 - empty string.
# WL13807-TSFR_3_552
EXPECT_SUCCESS([types_schema], test_output_absolute, { "maxRate": "", "ddlOnly": True, "showProgress": False })

#@<> WL13807: WL13804-FR5.1.2 - missing.
# WL13807-TSFR_3_552
EXPECT_SUCCESS([types_schema], test_output_absolute, { "ddlOnly": True, "showProgress": False })

#@<> WL13807: WL13804-FR5.6 - The `options` dictionary may contain a `showProgress` key with a Boolean value, which specifies whether to display the progress of dump process.
TEST_BOOL_OPTION("showProgress")

EXPECT_SUCCESS([types_schema], test_output_absolute, { "showProgress": True, "ddlOnly": True })

#@<> WL13807: WL13804-FR5.2.1 - The information about the progress must include:
# * The estimated total number of rows to be dumped.
# * The number of rows dumped so far.
# * The current progress as a percentage.
# * The current throughput in rows per second.
# * The current throughput in bytes written per second.
shutil.rmtree(test_output_absolute, True)
EXPECT_FALSE(os.path.isdir(test_output_absolute))
rc = testutil.call_mysqlsh([uri, "--", "util", "dump-schemas", types_schema, "--outputUrl", test_output_relative, "--showProgress", "true"])
EXPECT_EQ(0, rc)
EXPECT_TRUE(os.path.isdir(test_output_absolute))
EXPECT_STDOUT_MATCHES(re.compile(r'\d+% \(\d+\.?\d*[TGMK]? rows / ~\d+\.?\d*[TGMK]? rows\), \d+\.?\d*[TGMK]? rows?/s, \d+\.?\d* [TGMK]?B/s', re.MULTILINE))

#@<> WL13807: WL13804-FR5.2.2 - If the `showProgress` option is not given, a default value of `true` must be used instead if shell is used interactively. Otherwise, it is set to `false`.
shutil.rmtree(test_output_absolute, True)
EXPECT_FALSE(os.path.isdir(test_output_absolute))
rc = testutil.call_mysqlsh([uri, "--", "util", "dump-schemas", types_schema, "--outputUrl", test_output_relative])
EXPECT_EQ(0, rc)
EXPECT_TRUE(os.path.isdir(test_output_absolute))
EXPECT_STDOUT_NOT_CONTAINS("rows/s")

#@<> WL13807: WL13804-FR5.3 - The `options` dictionary may contain a `compression` key with a string value, which specifies the compression type used when writing the data dump files.
# WL13807-TSFR_3_57
# WL13807-TSFR_3_571_1
TEST_STRING_OPTION("compression")

# WL13807-TSFR_3_571_2
EXPECT_SUCCESS([types_schema], test_output_absolute, { "compression": "none", "chunking": False, "showProgress": False })
EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(types_schema, types_schema_tables[0]) + ".tsv")))

EXPECT_STDOUT_CONTAINS("Data size: ")
EXPECT_STDOUT_CONTAINS("Rows written: ")
EXPECT_STDOUT_CONTAINS("Bytes written: ")
EXPECT_STDOUT_CONTAINS("Average throughput: ")

#@<> WL13807: WL13804-FR5.3.1 - The allowed values for the `compression` option are:
# * `"none"` - no compression is used,
# * `"gzip"` - gzip compression is used.
# * `"zstd"` - zstd compression is used.
# WL13807-TSFR_3_571_3
EXPECT_SUCCESS([types_schema], test_output_absolute, { "compression": "gzip", "chunking": False, "showProgress": False })
EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(types_schema, types_schema_tables[0]) + ".tsv.gz")))

EXPECT_STDOUT_CONTAINS("Uncompressed data size: ")
EXPECT_STDOUT_CONTAINS("Compressed data size: ")
EXPECT_STDOUT_CONTAINS("Rows written: ")
EXPECT_STDOUT_CONTAINS("Bytes written: ")
EXPECT_STDOUT_CONTAINS("Average uncompressed throughput: ")
EXPECT_STDOUT_CONTAINS("Average compressed throughput: ")

EXPECT_SUCCESS([types_schema], test_output_absolute, { "compression": "zstd", "chunking": False, "showProgress": False })
EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(types_schema, types_schema_tables[0]) + ".tsv.zst")))

EXPECT_FAIL("ValueError", "Argument #3: The option 'compression' cannot be set to an empty string.", [types_schema], test_output_relative, { "compression": "" })
EXPECT_FAIL("ValueError", "Argument #3: Unknown compression type: dummy", [types_schema], test_output_relative, { "compression": "dummy" })

#@<> WL13807: WL13804-FR5.3.2 - If the `compression` option is not given, a default value of `"none"` must be used instead.
# WL13807-FR3 - Both new functions must accept the following options specified in WL#13804, FR5:
# * The `compression` option specified in WL#13804, FR5.3, with the modification of FR5.3.2, the default value must be`"zstd"`.
# WL13807-TSFR_3_572_1
EXPECT_SUCCESS([types_schema], test_output_absolute, { "chunking": False, "showProgress": False })
EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(types_schema, types_schema_tables[0]) + ".tsv.zst")))

#@<> Check compression level option
EXPECT_FAIL("ValueError", "Argument #3: Compression options not supported", [types_schema], test_output_relative, {"compression": "none;level=3"})
EXPECT_FAIL("ValueError", "Argument #3: Invalid compression level for zstd: 9000", [types_schema], test_output_relative, {"compression": "zstd;level=9000"})
EXPECT_FAIL("ValueError", "Argument #3: Invalid compression level for gzip: 12", [types_schema], test_output_relative, {"compression": "gzip;level=12"})
EXPECT_FAIL("ValueError", "Argument #3: Invalid compression option for zstd: superfast", [types_schema], test_output_relative, {"compression": "zstd;superfast=1"})
EXPECT_FAIL("ValueError", "Argument #3: Invalid compression option for gzip: superfast", [types_schema], test_output_relative, {"compression": "gzip;superfast=1"})
EXPECT_FAIL("ValueError", "Argument #3: Invalid compression option for zstd: superfast", [types_schema], test_output_relative, {"compression": "zstd;level=2;superfast=1"})
EXPECT_SUCCESS([types_schema], test_output_absolute, { "compression": "zstd;level=20", "showProgress": False })
EXPECT_SUCCESS([types_schema], test_output_absolute, { "compression": "gzip;level=9", "showProgress": False })

#@<> WL13807: WL13804-FR5.4 - The `options` dictionary may contain a `osBucketName` key with a string value, which specifies the OCI bucket name where the data dump files are going to be stored.
# WL13807-TSFR_3_58
TEST_STRING_OPTION("osBucketName")

#@<> WL13807: WL13804-FR5.5 - The `options` dictionary may contain a `osNamespace` key with a string value, which specifies the OCI namespace (tenancy name) where the OCI bucket is located.
# WL13807-TSFR_3_59
TEST_STRING_OPTION("osNamespace")

#@<> WL13807: WL13804-FR5.5.2 - If the value of `osNamespace` option is a non-empty string and the value of `osBucketName` option is an empty string, an exception must be thrown.
# WL13807-TSFR_3_592_1
EXPECT_FAIL("ValueError", "Argument #3: The option 'osNamespace' cannot be used when the value of 'osBucketName' option is not set.", [types_schema], test_output_relative, { "osNamespace": "namespace" })

#@<> WL13807: WL13804-FR5.6 - The `options` dictionary may contain a `ociConfigFile` key with a string value, which specifies the path to the OCI configuration file.
# WL13807-TSFR_3_510_1
TEST_STRING_OPTION("ociConfigFile")

#@<> WL13807: WL13804-FR5.6.2 - If the value of `ociConfigFile` option is a non-empty string and the value of `osBucketName` option is an empty string, an exception must be thrown.
# WL13807-TSFR_3_5102
EXPECT_FAIL("ValueError", "Argument #3: The option 'ociConfigFile' cannot be used when the value of 'osBucketName' option is not set.", [types_schema], test_output_relative, { "ociConfigFile": "config" })

#@<> WL13807: WL13804-FR5.7 - The `options` dictionary may contain a `ociProfile` key with a string value, which specifies the name of the OCI profile to use.
TEST_STRING_OPTION("ociProfile")

#@<> WL13807: WL13804-FR5.7.2 - If the value of `ociProfile` option is a non-empty string and the value of `osBucketName` option is an empty string, an exception must be thrown.
EXPECT_FAIL("ValueError", "Argument #3: The option 'ociProfile' cannot be used when the value of 'osBucketName' option is not set.", [types_schema], test_output_relative, { "ociProfile": "profile" })

#@<> WL15884-TSFR_1_1 - `ociAuth` help text
help_text = """
      - ociAuth: string (default: not set) - Use the specified authentication
        method when connecting to the OCI. Allowed values: api_key (used when
        not explicitly set), instance_obo_user, instance_principal,
        resource_principal, security_token.
"""
EXPECT_TRUE(help_text in util.help("dump_schemas"))

#@<> WL15884-TSFR_1_2 - `ociAuth` is a string option
TEST_STRING_OPTION("ociAuth")

#@<> WL15884-TSFR_2_1 - `ociAuth` set to an empty string is ignored
EXPECT_SUCCESS([types_schema], test_output_absolute, { "ociAuth": "", "showProgress": False })

#@<> WL15884-TSFR_3_1 - `ociAuth` cannot be used without `osBucketName`
EXPECT_FAIL("ValueError", "Argument #3: The option 'ociAuth' cannot be used when the value of 'osBucketName' option is not set", [types_schema], test_output_relative, { "osBucketName": "", "ociAuth": "api_key" })
EXPECT_FAIL("ValueError", "Argument #3: The option 'ociAuth' cannot be used when the value of 'osBucketName' option is not set", [types_schema], test_output_relative, { "ociAuth": "api_key" })

#@<> WL15884-TSFR_6_1_1 - `ociAuth` set to instance_principal cannot be used with `ociConfigFile` or `ociProfile`
EXPECT_FAIL("ValueError", "Argument #3: The option 'ociConfigFile' cannot be used when the 'ociAuth' option is set to: instance_principal.", [types_schema], test_output_relative, { "osBucketName": "bucket", "ociAuth": "instance_principal", "ociConfigFile": "file" })
EXPECT_FAIL("ValueError", "Argument #3: The option 'ociProfile' cannot be used when the 'ociAuth' option is set to: instance_principal.", [types_schema], test_output_relative, { "osBucketName": "bucket", "ociAuth": "instance_principal", "ociProfile": "profile" })

#@<> WL15884-TSFR_7_1_1 - `ociAuth` set to resource_principal cannot be used with `ociConfigFile` or `ociProfile`
EXPECT_FAIL("ValueError", "Argument #3: The option 'ociConfigFile' cannot be used when the 'ociAuth' option is set to: resource_principal.", [types_schema], test_output_relative, { "osBucketName": "bucket", "ociAuth": "resource_principal", "ociConfigFile": "file" })
EXPECT_FAIL("ValueError", "Argument #3: The option 'ociProfile' cannot be used when the 'ociAuth' option is set to: resource_principal.", [types_schema], test_output_relative, { "osBucketName": "bucket", "ociAuth": "resource_principal", "ociProfile": "profile" })

#@<> WL15884-TSFR_9_1 - `ociAuth` set to an invalid value
EXPECT_FAIL("ValueError", "Argument #3: Invalid value of 'ociAuth' option, expected one of: api_key, instance_obo_user, instance_principal, resource_principal, security_token, but got: unknown.", [types_schema], test_output_relative, { "osBucketName": "bucket", "ociAuth": "unknown" })

#@<> WL13807: WL13804-FR5.8 - The `options` dictionary may contain a `defaultCharacterSet` key with a string value, which specifies the character set to be used during the dump. The session variables `character_set_client`, `character_set_connection`, and `character_set_results` must be set to this value for each opened connection.
# WL13807-TSFR4_43
TEST_STRING_OPTION("defaultCharacterSet")

#@<> WL13807-TSFR4_40
EXPECT_SUCCESS([test_schema], test_output_absolute, { "defaultCharacterSet": "utf8mb4", "ddlOnly": True, "showProgress": False })
# name should be correctly encoded using UTF-8
EXPECT_FILE_CONTAINS("CREATE TABLE IF NOT EXISTS `{0}`".format(test_table_non_unique), os.path.join(test_output_absolute, encode_table_basename(test_schema, test_table_non_unique) + ".sql"))

#@<> WL13807: WL13804-FR5.8.1 - If the value of the `defaultCharacterSet` option is not a character set supported by the MySQL server, an exception must be thrown.
# WL13807-TSFR4_41
EXPECT_FAIL("MySQL Error (1115)", "Unknown character set: ''", [types_schema], test_output_relative, { "defaultCharacterSet": "" })
EXPECT_FAIL("MySQL Error (1115)", "Unknown character set: 'dummy'", [types_schema], test_output_relative, { "defaultCharacterSet": "dummy" })

#@<> WL13807: WL13804-FR5.8.2 - If the `defaultCharacterSet` option is not given, a default value of `"utf8mb4"` must be used instead.
# WL13807-TSFR4_39
EXPECT_SUCCESS([test_schema], test_output_absolute, { "ddlOnly": True, "showProgress": False })
# name should be correctly encoded using UTF-8
EXPECT_FILE_CONTAINS("CREATE TABLE IF NOT EXISTS `{0}`".format(test_table_non_unique), os.path.join(test_output_absolute, encode_table_basename(test_schema, test_table_non_unique) + ".sql"))

#@<> WL15311_TSFR_5_1
help_text = """
      - fieldsTerminatedBy: string (default: "\\t") - This option has the same
        meaning as the corresponding clause for SELECT ... INTO OUTFILE.
      - fieldsEnclosedBy: char (default: '') - This option has the same meaning
        as the corresponding clause for SELECT ... INTO OUTFILE.
      - fieldsEscapedBy: char (default: '\\') - This option has the same meaning
        as the corresponding clause for SELECT ... INTO OUTFILE.
      - fieldsOptionallyEnclosed: bool (default: false) - Set to true if the
        input values are not necessarily enclosed within quotation marks
        specified by fieldsEnclosedBy option. Set to false if all fields are
        quoted by character specified by fieldsEnclosedBy option.
      - linesTerminatedBy: string (default: "\\n") - This option has the same
        meaning as the corresponding clause for SELECT ... INTO OUTFILE. See
        Section 13.2.10.1, "SELECT ... INTO Statement".
      - dialect: enum (default: "default") - Setup fields and lines options
        that matches specific data file format. Can be used as base dialect and
        customized with fieldsTerminatedBy, fieldsEnclosedBy, fieldsEscapedBy,
        fieldsOptionallyEnclosed and linesTerminatedBy options. Must be one of
        the following values: default, csv, tsv, csv-unix or csv-rfc-unix.
"""
EXPECT_TRUE(help_text in util.help("dump_schemas"))

#@<> WL15311 - option types
TEST_STRING_OPTION("fieldsTerminatedBy")
TEST_STRING_OPTION("fieldsEnclosedBy")
TEST_STRING_OPTION("fieldsEscapedBy")
TEST_BOOL_OPTION("fieldsOptionallyEnclosed")
TEST_STRING_OPTION("linesTerminatedBy")
TEST_STRING_OPTION("dialect")

#@<> WL15311_TSFR_5_1_1, WL15311_TSFR_5_2_1 - various predefined dialects and their extensions
for dialect, ext in { "default": "tsv", "csv": "csv", "tsv": "tsv", "csv-unix": "csv", "csv-rfc-unix": "csv" }.items():
    TEST_DUMP_AND_LOAD([types_schema], { "dialect": dialect })
    EXPECT_TRUE(count_files_with_extension(test_output_relative, f".{ext}.zst") > 0)

#@<> WL15311 - The JSON dialect must not be supported.
EXPECT_FAIL("ValueError", "Argument #3: The 'json' dialect is not supported.", [types_schema], test_output_relative, { "dialect": "json" })

#@<> WL15311_TSFR_5_3_1 - custom dialect
TEST_DUMP_AND_LOAD([types_schema], { "fieldsTerminatedBy": "a", "fieldsEnclosedBy": "b", "fieldsEscapedBy": "c", "linesTerminatedBy": "d", "fieldsOptionallyEnclosed": True })
EXPECT_TRUE(count_files_with_extension(test_output_relative, ".txt.zst") > 0)

#@<> WL15311_TSFR_5_2, WL15311_TSFR_5_3, WL15311_TSFR_5_4, WL15311_TSFR_5_5, WL15311_TSFR_5_6 - more custom dialects
custom_dialect_schema = "wl15311_tsfr_5"
custom_dialect_table = "x"

session.run_sql("CREATE SCHEMA !;", [ custom_dialect_schema ])
session.run_sql("CREATE TABLE !.! (id int primary key AUTO_INCREMENT, p text, q text);", [ custom_dialect_schema, custom_dialect_table ])
session.run_sql("INSERT INTO !.! VALUES (1, 'aaaaaa', 'bbbbb'), (2, 'aabbaaaa', 'bbaabbb'), (3, 'aabbaababaaa', 'bbaabbababbabab');", [ custom_dialect_schema, custom_dialect_table ])

for i in range(10):
    session.run_sql("INSERT INTO !.! (p, q) SELECT p, q FROM !.!;", [ custom_dialect_schema, custom_dialect_table, custom_dialect_schema, custom_dialect_table ])

session.run_sql("ANALYZE TABLE !.!;", [ custom_dialect_schema, custom_dialect_table ])

TEST_DUMP_AND_LOAD([ custom_dialect_schema ])
TEST_DUMP_AND_LOAD([ custom_dialect_schema ], { "fieldsTerminatedBy": "a" })
TEST_DUMP_AND_LOAD([ custom_dialect_schema ], { "linesTerminatedBy": "a" })
TEST_DUMP_AND_LOAD([ custom_dialect_schema ], { "fieldsEnclosedBy": '"', "fieldsOptionallyEnclosed": False , "linesTerminatedBy": "a"})
TEST_DUMP_AND_LOAD([ custom_dialect_schema ], { "fieldsEnclosedBy": '"', "fieldsOptionallyEnclosed": False , "linesTerminatedBy": "ab"})

session.run_sql("DROP SCHEMA !;", [ custom_dialect_schema ])

#@<> WL15311 - fixed-row format is not supported yet
EXPECT_FAIL("ValueError", "Argument #3: The fieldsTerminatedBy and fieldsEnclosedBy are both empty, resulting in a fixed-row format. This is currently not supported.", [types_schema], test_output_absolute, { "fieldsTerminatedBy": "", "fieldsEnclosedBy": "" })

#@<> WL14387-TSFR_1_1_1 - s3BucketName - string option
TEST_STRING_OPTION("s3BucketName")

#@<> WL14387-TSFR_1_2_1 - s3BucketName and osBucketName cannot be used at the same time
EXPECT_FAIL("ValueError", "Argument #3: The option 's3BucketName' cannot be used when the value of 'osBucketName' option is set", [types_schema], test_output_relative, { "s3BucketName": "one", "osBucketName": "two" })

#@<> WL14387-TSFR_1_1_3 - s3BucketName set to an empty string dumps to a local directory
EXPECT_SUCCESS([types_schema], test_output_absolute, { "s3BucketName": "", "showProgress": False })

#@<> s3CredentialsFile - string option
TEST_STRING_OPTION("s3CredentialsFile")

#@<> WL14387-TSFR_3_1_1_1 - s3CredentialsFile cannot be used without s3BucketName
EXPECT_FAIL("ValueError", "Argument #3: The option 's3CredentialsFile' cannot be used when the value of 's3BucketName' option is not set", [types_schema], test_output_relative, { "s3CredentialsFile": "file" })

#@<> s3BucketName and s3CredentialsFile both set to an empty string dumps to a local directory
EXPECT_SUCCESS([types_schema], test_output_absolute, { "s3BucketName": "", "s3CredentialsFile": "", "showProgress": False })

#@<> s3ConfigFile - string option
TEST_STRING_OPTION("s3ConfigFile")

#@<> WL14387-TSFR_4_1_1_1 - s3ConfigFile cannot be used without s3BucketName
EXPECT_FAIL("ValueError", "Argument #3: The option 's3ConfigFile' cannot be used when the value of 's3BucketName' option is not set", [types_schema], test_output_relative, { "s3ConfigFile": "file" })

#@<> s3BucketName and s3ConfigFile both set to an empty string dumps to a local directory
EXPECT_SUCCESS([types_schema], test_output_absolute, { "s3BucketName": "", "s3ConfigFile": "", "showProgress": False })

#@<> WL14387-TSFR_2_1_1 - s3Profile - string option
TEST_STRING_OPTION("s3Profile")

#@<> WL14387-TSFR_2_1_1_2 - s3Profile cannot be used without s3BucketName
EXPECT_FAIL("ValueError", "Argument #3: The option 's3Profile' cannot be used when the value of 's3BucketName' option is not set", [types_schema], test_output_relative, { "s3Profile": "profile" })

#@<> WL14387-TSFR_2_1_2_1 - s3BucketName and s3Profile both set to an empty string dumps to a local directory
EXPECT_SUCCESS([types_schema], test_output_absolute, { "s3BucketName": "", "s3Profile": "", "showProgress": False })

#@<> s3Region - string option
TEST_STRING_OPTION("s3Region")

#@<> s3Region cannot be used without s3BucketName
EXPECT_FAIL("ValueError", "Argument #3: The option 's3Region' cannot be used when the value of 's3BucketName' option is not set", [types_schema], test_output_relative, { "s3Region": "region" })

#@<> s3BucketName and s3Region both set to an empty string dumps to a local directory
EXPECT_SUCCESS([types_schema], test_output_absolute, { "s3BucketName": "", "s3Region": "", "showProgress": False })

#@<> s3EndpointOverride - string option
TEST_STRING_OPTION("s3EndpointOverride")

#@<> WL14387-TSFR_6_1_1 - s3EndpointOverride cannot be used without s3BucketName
EXPECT_FAIL("ValueError", "Argument #3: The option 's3EndpointOverride' cannot be used when the value of 's3BucketName' option is not set", [types_schema], test_output_relative, { "s3EndpointOverride": "http://example.org" })

#@<> s3BucketName and s3EndpointOverride both set to an empty string dumps to a local directory
EXPECT_SUCCESS([types_schema], test_output_absolute, { "s3BucketName": "", "s3EndpointOverride": "", "showProgress": False })

#@<> s3EndpointOverride is missing a scheme
EXPECT_FAIL("ValueError", "Argument #3: The value of the option 's3EndpointOverride' is missing a scheme, expected: http:// or https://.", [types_schema], test_output_absolute, { "s3BucketName": "bucket", "s3EndpointOverride": "endpoint", "showProgress": False })

#@<> s3EndpointOverride is using wrong scheme
EXPECT_FAIL("ValueError", "Argument #3: The value of the option 's3EndpointOverride' uses an invalid scheme 'FTp://', expected: http:// or https://.", [types_schema], test_output_absolute, { "s3BucketName": "bucket", "s3EndpointOverride": "FTp://endpoint", "showProgress": False })

#@<> WL13807-TSFR_3_2 - options param being a dictionary that contains an unknown key
for param in { "dummy", "users", "excludeUsers", "includeUsers", "indexColumn", "excludeSchemas", "includeSchemas", "ociParManifest", "ociParExpireTime" }:
    EXPECT_FAIL("ValueError", f"Argument #3: Invalid options: {param}", [types_schema], test_output_relative, { param: "fails" })

# WL13807-FR4 - Both new functions must accept a set of additional options:

#@<> WL13807-FR4.1 - The `options` dictionary may contain a `consistent` key with a Boolean value, which specifies whether the data from the tables should be dumped consistently.
TEST_BOOL_OPTION("consistent")

EXPECT_SUCCESS([types_schema], test_output_absolute, { "consistent": False, "showProgress": False })

#@<> BUG#32107327 verify if consistent dump works

# setup
session.run_sql("CREATE DATABASE db1")
session.run_sql("CREATE TABLE db1.t1 (pk INT PRIMARY KEY AUTO_INCREMENT, c INT NOT NULL)")
session.run_sql("CREATE DATABASE db2")
session.run_sql("CREATE TABLE db2.t1 (pk INT PRIMARY KEY AUTO_INCREMENT, c INT NOT NULL)")

# insert values to both tables in the same transaction
insert_values = """v = 0
session.run_sql("TRUNCATE db1.t1")
session.run_sql("TRUNCATE db2.t1")
while True:
    session.run_sql("BEGIN")
    session.run_sql("INSERT INTO db1.t1 (c) VALUES({0})".format(v))
    session.run_sql("INSERT INTO db2.t1 (c) VALUES({0})".format(v))
    session.run_sql("COMMIT")
    v = v + 1
session.close()
"""

# run a process which constantly inserts some values
pid = testutil.call_mysqlsh_async(["--py", uri], insert_values)

# wait a bit for process to start
time.sleep(1)

# check if dumps for both tables have the same number of entries
for i in range(10):
    EXPECT_SUCCESS(["db1", "db2"], test_output_absolute, { "consistent": True, "showProgress": False, "compression": "none", "chunking": False })
    db1 = os.path.join(test_output_absolute, encode_table_basename("db1", "t1") + ".tsv")
    db2 = os.path.join(test_output_absolute, encode_table_basename("db2", "t1") + ".tsv")
    EXPECT_TRUE(os.path.isfile(db1), "File '{0}' should exist".format(db1))
    EXPECT_TRUE(os.path.isfile(db2), "File '{0}' should exist".format(db2))
    EXPECT_EQ(os.stat(db1).st_size, os.stat(db2).st_size, "Files '{0}' and '{1}' should have the same size".format(db1, db2))

# terminate immediately, process will not stop on its own
testutil.wait_mysqlsh_async(pid, 0)

# cleanup
session.run_sql("DROP DATABASE db1")
session.run_sql("DROP DATABASE db2")

#@<> BUG#34556560 - skipConsistencyChecks options
TEST_BOOL_OPTION("skipConsistencyChecks")

#@<> WL13807-FR4.3 - The `options` dictionary may contain a `events` key with a Boolean value, which specifies whether to include events in the DDL file of each schema.
TEST_BOOL_OPTION("events")

EXPECT_SUCCESS([test_schema], test_output_absolute, { "events": False, "ddlOnly": True, "showProgress": False })
EXPECT_FILE_CONTAINS("CREATE DATABASE /*!32312 IF NOT EXISTS*/ `{0}`".format(test_schema), os.path.join(test_output_absolute, schema_ddl_file(test_schema, Schema_ddl.Schema)))

if instance_supports_libraries:
    EXPECT_FALSE(os.path.isfile(os.path.join(test_output_absolute, schema_ddl_file(test_schema, Schema_ddl.Events))))
else:
    EXPECT_FILE_NOT_CONTAINS("CREATE DEFINER=`{0}`@`{1}` EVENT IF NOT EXISTS `{2}`".format(__user, __host, test_schema_event), os.path.join(test_output_absolute, schema_ddl_file(test_schema, Schema_ddl.Events)))

if instance_supports_libraries:
    EXPECT_STDOUT_CONTAINS("1 schemas will be dumped and within them 4 tables, 1 view, 1 library, 4 routines, 1 trigger.")
else:
    EXPECT_STDOUT_CONTAINS("1 schemas will be dumped and within them 4 tables, 1 view, 2 routines, 1 trigger.")

#@<> WL13807-FR4.3.1 - If the `events` option is not given, a default value of `true` must be used instead.
EXPECT_SUCCESS([test_schema], test_output_absolute, { "ddlOnly": True, "showProgress": False })
EXPECT_FILE_CONTAINS("CREATE DEFINER=`{0}`@`{1}` EVENT IF NOT EXISTS `{2}`".format(__user, __host, test_schema_event), os.path.join(test_output_absolute, schema_ddl_file(test_schema, Schema_ddl.Events)))

#@<> BUG#33396153 - dumpInstance() + events
EXPECT_SUCCESS([test_schema], test_output_absolute, { "excludeEvents": [ f"`{test_schema}`.`{test_schema_event}`" ], "dryRun": True, "showProgress": False })

if instance_supports_libraries:
    EXPECT_STDOUT_CONTAINS("1 schemas will be dumped and within them 4 tables, 1 view, 0 out of 1 event, 1 library, 4 routines, 1 trigger.")
else:
    EXPECT_STDOUT_CONTAINS("1 schemas will be dumped and within them 4 tables, 1 view, 0 out of 1 event, 2 routines, 1 trigger.")

#@<> WL13807-FR4.4 - The `options` dictionary may contain a `routines` key with a Boolean value, which specifies whether to include functions and stored procedures in the DDL file of each schema. User-defined functions must not be included.
TEST_BOOL_OPTION("routines")

EXPECT_SUCCESS([test_schema], test_output_absolute, { "routines": False, "ddlOnly": True, "showProgress": False })
EXPECT_FILE_CONTAINS("CREATE DATABASE /*!32312 IF NOT EXISTS*/ `{0}`".format(test_schema), os.path.join(test_output_absolute, schema_ddl_file(test_schema, Schema_ddl.Schema)))

if instance_supports_libraries:
    EXPECT_FALSE(os.path.isfile(os.path.join(test_output_absolute, schema_ddl_file(test_schema, Schema_ddl.Routines))))
else:
    EXPECT_FILE_NOT_CONTAINS("CREATE DEFINER=`{0}`@`{1}` PROCEDURE `{2}`".format(__user, __host, test_schema_procedure), os.path.join(test_output_absolute, schema_ddl_file(test_schema, Schema_ddl.Routines)))
    EXPECT_FILE_NOT_CONTAINS("CREATE DEFINER=`{0}`@`{1}` FUNCTION `{2}`".format(__user, __host, test_schema_function), os.path.join(test_output_absolute, schema_ddl_file(test_schema, Schema_ddl.Routines)))

if instance_supports_libraries:
    EXPECT_STDOUT_CONTAINS("1 schemas will be dumped and within them 4 tables, 1 view, 1 event, 1 library, 1 trigger.")
else:
    EXPECT_STDOUT_CONTAINS("1 schemas will be dumped and within them 4 tables, 1 view, 1 event, 1 trigger.")

#@<> WL13807-FR4.4.1 - If the `routines` option is not given, a default value of `true` must be used instead.
EXPECT_SUCCESS([test_schema], test_output_absolute, { "ddlOnly": True, "showProgress": False })
EXPECT_FILE_CONTAINS("CREATE DEFINER=`{0}`@`{1}` PROCEDURE `{2}`".format(__user, __host, test_schema_procedure), os.path.join(test_output_absolute, schema_ddl_file(test_schema, Schema_ddl.Routines)))
EXPECT_FILE_CONTAINS("CREATE DEFINER=`{0}`@`{1}` FUNCTION `{2}`".format(__user, __host, test_schema_function), os.path.join(test_output_absolute, schema_ddl_file(test_schema, Schema_ddl.Routines)))

#@<> BUG#33396153 - dumpInstance() + routines
EXPECT_SUCCESS([test_schema], test_output_absolute, { "excludeRoutines": [ f"`{test_schema}`.`{test_schema_procedure}`" ], "dryRun": True, "showProgress": False })

if instance_supports_libraries:
    EXPECT_STDOUT_CONTAINS("1 schemas will be dumped and within them 4 tables, 1 view, 1 event, 1 library, 3 out of 4 routines, 1 trigger.")
else:
    EXPECT_STDOUT_CONTAINS("1 schemas will be dumped and within them 4 tables, 1 view, 1 event, 1 out of 2 routines, 1 trigger.")

#@<> WL16731-G1 - `libraries` - invalid values
TEST_BOOL_OPTION("libraries")

#@<> WL16731-G2 - `libraries` - disabled
EXPECT_SUCCESS([test_schema], test_output_absolute, { "libraries": False, "ddlOnly": True, "showProgress": False })

if instance_supports_libraries:
    # WL16731-TSFR_1_7_1 - note about routine using an excluded library is printed
    EXPECT_STDOUT_CONTAINS(routine_references_excluded_library("Function", test_schema, test_schema_library_function, test_schema, test_schema_library).note())
    EXPECT_FILE_CONTAINS(f"CREATE DEFINER=`{__user}`@`{__host}` FUNCTION `{test_schema_library_function}`", os.path.join(test_output_absolute, schema_ddl_file(test_schema, Schema_ddl.Routines)))
    EXPECT_STDOUT_CONTAINS(routine_references_excluded_library("Procedure", test_schema, test_schema_library_procedure, test_schema, test_schema_library).note())
    EXPECT_FILE_CONTAINS(f"CREATE DEFINER=`{__user}`@`{__host}` PROCEDURE `{test_schema_library_procedure}`", os.path.join(test_output_absolute, schema_ddl_file(test_schema, Schema_ddl.Routines)))

EXPECT_FILE_CONTAINS("CREATE DATABASE /*!32312 IF NOT EXISTS*/ `{0}`".format(test_schema), os.path.join(test_output_absolute, schema_ddl_file(test_schema, Schema_ddl.Schema)))

EXPECT_FALSE(os.path.isfile(os.path.join(test_output_absolute, schema_ddl_file(test_schema, Schema_ddl.Libraries))))

if instance_supports_libraries:
    EXPECT_STDOUT_CONTAINS("1 schemas will be dumped and within them 4 tables, 1 view, 1 event, 4 routines, 1 trigger.")
else:
    EXPECT_STDOUT_CONTAINS("1 schemas will be dumped and within them 4 tables, 1 view, 1 event, 2 routines, 1 trigger.")

#@<> WL16731-G3 - `libraries` - default value
EXPECT_SUCCESS([test_schema], test_output_absolute, { "targetVersion": "9.1.0", "ddlOnly": True, "showProgress": False })

if instance_supports_libraries:
    EXPECT_FILE_CONTAINS(f"CREATE LIBRARY `{test_schema_library}`", os.path.join(test_output_absolute, schema_ddl_file(test_schema, Schema_ddl.Libraries)))
    # WL16731-TSFR_1_1_2_1 - target version does not support libraries, warning is printed
    EXPECT_STDOUT_CONTAINS(warn_target_version_does_not_support_libraries("9.1.0"))
else:
    EXPECT_FALSE(os.path.isfile(os.path.join(test_output_absolute, schema_ddl_file(test_schema, Schema_ddl.Libraries))))
    # WL16731-TSFR_1_1_2_1 - target version does not support libraries, but there are no libraries, warning is not printed
    EXPECT_STDOUT_NOT_CONTAINS(warn_target_version_does_not_support_libraries("9.1.0"))

#@<> WL16731-G2 - `libraries` - filtering
# WL16731-TSFR_1_1_1 - test is executed regardless of server version
EXPECT_SUCCESS([test_schema], test_output_absolute, { "targetVersion": "9.2.0", "libraries": True, "excludeLibraries": [ f"`{test_schema}`.`{test_schema_library}`" ], "dryRun": True, "showProgress": False })

if instance_supports_libraries:
    EXPECT_STDOUT_CONTAINS("1 schemas will be dumped and within them 4 tables, 1 view, 1 event, 0 out of 1 library, 4 routines, 1 trigger.")
else:
    EXPECT_STDOUT_CONTAINS("1 schemas will be dumped and within them 4 tables, 1 view, 1 event, 2 routines, 1 trigger.")

# WL16731-TSFR_1_1_2_2 - target version supports libraries, warning is not printed
EXPECT_STDOUT_NOT_CONTAINS(warn_target_version_does_not_support_libraries("9.2.0"))

#@<> WL16731-TSFR_1_7_1_2 - `libraries` - routine uses a library which does not exist {instance_supports_libraries}
# constants
tested_schema = "wl16731"
tested_library = "my_library"
tested_function = "my_function"
tested_procedure = "my_procedure"

# setup
session.run_sql("DROP SCHEMA IF EXISTS !", [tested_schema])
session.run_sql("CREATE SCHEMA !", [tested_schema])
session.run_sql("CREATE LIBRARY !.! LANGUAGE JAVASCRIPT AS $$ $$", [tested_schema, tested_library])
session.run_sql("CREATE FUNCTION !.!() RETURNS INTEGER DETERMINISTIC LANGUAGE JAVASCRIPT USING (!.!) AS $$ $$", [tested_schema, tested_function, tested_schema, tested_library])
session.run_sql("CREATE PROCEDURE !.!() LANGUAGE JAVASCRIPT USING (!.!) AS $$ $$", [tested_schema, tested_procedure, tested_schema, tested_library])
session.run_sql("DROP LIBRARY !.!", [tested_schema, tested_library])

# test
EXPECT_SUCCESS([tested_schema], test_output_absolute, { "dryRun": True, "showProgress": False })
EXPECT_STDOUT_CONTAINS(routine_references_missing_library("Function", tested_schema, tested_function, tested_schema, tested_library).warning())
EXPECT_STDOUT_CONTAINS(routine_references_missing_library("Procedure", tested_schema, tested_procedure, tested_schema, tested_library).warning())

# cleanup
session.run_sql("DROP SCHEMA IF EXISTS !", [tested_schema])

#@<> WL13807-FR4.5 - The `options` dictionary may contain a `triggers` key with a Boolean value, which specifies whether to include triggers in the DDL file of each table.
# WL13807-TSFR4_15
TEST_BOOL_OPTION("triggers")

# WL13807-TSFR4_14
# WL13807-TSFR10_5
EXPECT_SUCCESS([test_schema], test_output_absolute, { "triggers": False, "ddlOnly": True, "showProgress": False })
EXPECT_FALSE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(test_schema, test_table_no_index) + ".triggers.sql")))

if instance_supports_libraries:
    EXPECT_STDOUT_CONTAINS("1 schemas will be dumped and within them 4 tables, 1 view, 1 event, 1 library, 4 routines.")
else:
    EXPECT_STDOUT_CONTAINS("1 schemas will be dumped and within them 4 tables, 1 view, 1 event, 2 routines.")

#@<> WL13807-FR4.5.1 - If the `triggers` option is not given, a default value of `true` must be used instead.
# WL13807-TSFR4_13
EXPECT_SUCCESS([test_schema], test_output_absolute, { "ddlOnly": True, "showProgress": False })
EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(test_schema, test_table_no_index) + ".triggers.sql")))

#@<> BUG#33396153 - dumpInstance() + triggers
EXPECT_SUCCESS([test_schema], test_output_absolute, { "excludeTriggers": [ f"`{test_schema}`.`{test_table_no_index}`" ], "dryRun": True, "showProgress": False })

if instance_supports_libraries:
    EXPECT_STDOUT_CONTAINS("1 schemas will be dumped and within them 4 tables, 1 view, 1 event, 1 library, 4 routines, 0 out of 1 trigger.")
else:
    EXPECT_STDOUT_CONTAINS("1 schemas will be dumped and within them 4 tables, 1 view, 1 event, 2 routines, 0 out of 1 trigger.")

#@<> WL13807-FR4.6 - The `options` dictionary may contain a `tzUtc` key with a Boolean value, which specifies whether to set the time zone to UTC and include a `SET TIME_ZONE='+00:00'` statement in the DDL files and to execute this statement before the data dump is started. This allows dumping TIMESTAMP data when a server has data in different time zones or data is being moved between servers with different time zones.
# WL13807-TSFR4_18
TEST_BOOL_OPTION("tzUtc")

# WL13807-TSFR4_17
EXPECT_SUCCESS([test_schema], test_output_absolute, { "tzUtc": False, "ddlOnly": True, "showProgress": False })
with open(os.path.join(test_output_absolute, "@.json"), encoding="utf-8") as json_file:
    EXPECT_FALSE(json.load(json_file)["tzUtc"])

#@<> WL13807-FR4.6.1 - If the `tzUtc` option is not given, a default value of `true` must be used instead.
# WL13807-TSFR4_16
EXPECT_SUCCESS([test_schema], test_output_absolute, { "ddlOnly": True, "showProgress": False })
with open(os.path.join(test_output_absolute, "@.json"), encoding="utf-8") as json_file:
    EXPECT_TRUE(json.load(json_file)["tzUtc"])

#@<> WL13807-FR4.7 - The `options` dictionary may contain a `ddlOnly` key with a Boolean value, which specifies whether the data dump should be skipped.
# WL13807-TSFR4_21
TEST_BOOL_OPTION("ddlOnly")

# WL13807-FR4.7.1 - If the `ddlOnly` option is set to `true`, only the DDL files must be written.
# WL13807-TSFR4_20
EXPECT_SUCCESS([test_schema], test_output_absolute, { "ddlOnly": True, "showProgress": False })
EXPECT_EQ(0, count_files_with_extension(test_output_absolute, ".zst"))
EXPECT_NE(0, count_files_with_extension(test_output_absolute, ".sql"))

#@<> WL13807-FR4.7.2 - If the `ddlOnly` option is not given, a default value of `false` must be used instead.
# WL13807-TSFR4_19
EXPECT_SUCCESS([test_schema], test_output_absolute, { "showProgress": False })
EXPECT_NE(0, count_files_with_extension(test_output_absolute, ".zst"))
EXPECT_NE(0, count_files_with_extension(test_output_absolute, ".sql"))

#@<> WL13807-FR4.8 - The `options` dictionary may contain a `dataOnly` key with a Boolean value, which specifies whether the DDL files should be written.
# WL13807-TSFR4_24
TEST_BOOL_OPTION("dataOnly")

# WL13807-FR4.8.1 - If the `dataOnly` option is set to `true`, only the data dump files must be written.
# WL13807-TSFR4_23
EXPECT_SUCCESS([test_schema], test_output_absolute, { "dataOnly": True, "showProgress": False })
EXPECT_NE(0, count_files_with_extension(test_output_absolute, ".zst"))
# WL13807-TSFR9_2
# WL13807-TSFR10_2
# WL13807-TSFR11_2
EXPECT_EQ(0, count_files_with_extension(test_output_absolute, ".sql"))

#@<> WL13807-FR4.8.2 - If both `ddlOnly` and `dataOnly` options are set to `true`, an exception must be raised.
# WL13807-TSFR4_25
EXPECT_FAIL("ValueError", "Argument #3: The 'ddlOnly' and 'dataOnly' options cannot be both set to true.", [types_schema], test_output_relative, { "ddlOnly": True, "dataOnly": True })

#@<> WL13807-FR4.8.3 - If the `dataOnly` option is not given, a default value of `false` must be used instead.
# WL13807-TSFR4_22
EXPECT_SUCCESS([test_schema], test_output_absolute, { "showProgress": False })
EXPECT_NE(0, count_files_with_extension(test_output_absolute, ".zst"))
EXPECT_NE(0, count_files_with_extension(test_output_absolute, ".sql"))

#@<> WL13807-FR4.9 - The `options` dictionary may contain a `dryRun` key with a Boolean value, which specifies whether a dry run should be performed.
# WL13807-TSFR4_28
TEST_BOOL_OPTION("dryRun")

# WL13807-FR4.9.1 - If the `dryRun` option is set to `true`, Shell must print information about what would be performed/dumped, including the compatibility notes for `ocimds` if available, but do not dump anything.
# WL13807-TSFR4_27
shutil.rmtree(test_output_absolute, True)
EXPECT_FALSE(os.path.isdir(test_output_absolute))
util.dump_schemas([test_schema], test_output_absolute, { "dryRun": True, "showProgress": False })
EXPECT_FALSE(os.path.isdir(test_output_absolute))
EXPECT_STDOUT_NOT_CONTAINS("Schemas dumped: 1")
EXPECT_STDOUT_CONTAINS("NOTE: dryRun enabled, no locks will be acquired and no files will be created.")

#@<> WL13807-FR4.9.2 - If the `dryRun` option is not given, a default value of `false` must be used instead.
# WL13807-TSFR4_26
EXPECT_SUCCESS([test_schema], test_output_absolute, { "ddlOnly": True, "showProgress": False })

#@<> WL13807-FR4.11 - The `options` dictionary may contain an `excludeTables` key with a list of strings, which specifies the names of tables to be excluded from the dump.
# WL13807-TSFR4_38
TEST_ARRAY_OF_STRINGS_OPTION("excludeTables")

EXPECT_SUCCESS([test_schema], test_output_absolute, { "excludeTables": [], "ddlOnly": True, "showProgress": False })

# WL13807-TSFR4_33
# WL13807-TSFR4_35
# WL13807-TSFR4_37
EXPECT_SUCCESS([test_schema], test_output_absolute, { "excludeTables": [ "`{0}`.`{1}`".format(test_schema, test_table_non_unique) ], "ddlOnly": True, "showProgress": False })
# WL13807-TSFR10_3
EXPECT_FALSE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(test_schema, test_table_non_unique) + ".sql")))
EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(test_schema, test_table_primary) + ".sql")))
EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(test_schema, test_table_unique) + ".sql")))
EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(test_schema, test_table_no_index) + ".sql")))

#@<> WL13807-FR4.11.1 - The table names must be in form `schema.table`. Both `schema` and `table` must be valid MySQL identifiers and must be quoted with backtick (`` ` ``) character when required.
# WL13807-TSFR4_36
EXPECT_FAIL("ValueError", "Argument #3: The table to be excluded must be in the following form: schema.table, with optional backtick quotes, wrong value: 'dummy'.", [types_schema], test_output_relative, { "excludeTables": [ "dummy" ] })
EXPECT_FAIL("ValueError", "Argument #3: Failed to parse table to be excluded 'dummy.dummy.dummy': Invalid object name, expected end of name, but got: '.'", [types_schema], test_output_relative, { "excludeTables": [ "dummy.dummy.dummy" ] })
EXPECT_FAIL("ValueError", "Argument #3: Failed to parse table to be excluded '@.dummy': Invalid character in identifier", [types_schema], test_output_relative, { "excludeTables": [ "@.dummy" ] })
EXPECT_FAIL("ValueError", "Argument #3: Failed to parse table to be excluded 'dummy.@': Invalid character in identifier", [types_schema], test_output_relative, { "excludeTables": [ "dummy.@" ] })
EXPECT_FAIL("ValueError", "Argument #3: Failed to parse table to be excluded '@.@': Invalid character in identifier", [types_schema], test_output_relative, { "excludeTables": [ "@.@" ] })
EXPECT_FAIL("ValueError", "Argument #3: Failed to parse table to be excluded '1.dummy': Invalid identifier: identifiers may begin with a digit but unless quoted may not consist solely of digits.", [types_schema], test_output_relative, { "excludeTables": [ "1.dummy" ] })
EXPECT_FAIL("ValueError", "Argument #3: Failed to parse table to be excluded 'dummy.1': Invalid identifier: identifiers may begin with a digit but unless quoted may not consist solely of digits.", [types_schema], test_output_relative, { "excludeTables": [ "dummy.1" ] })
EXPECT_FAIL("ValueError", "Argument #3: Failed to parse table to be excluded '2.1': Invalid identifier: identifiers may begin with a digit but unless quoted may not consist solely of digits.", [types_schema], test_output_relative, { "excludeTables": [ "2.1" ] })

#@<> BUG#33402791 - objects for schemas which are not included in the dump are reported as errors - excludeTables
EXPECT_FAIL("ValueError", "Conflicting filtering options", [test_schema], test_output_absolute, { "excludeTables": [ "`{0}`.`dummy`".format(test_schema), "`@`.dummy", "dummy.`@`", "`@`.`@`", "`1`.dummy", "dummy.`1`", "`2`.`1`" ], "ddlOnly": True, "showProgress": False })
EXPECT_STDOUT_CONTAINS("ERROR: The excludeTables option contains a table `2`.`1` which refers to a schema which was not included in the dump.")
EXPECT_STDOUT_CONTAINS("ERROR: The excludeTables option contains a table `1`.`dummy` which refers to a schema which was not included in the dump.")
EXPECT_STDOUT_CONTAINS("ERROR: The excludeTables option contains a table `dummy`.`1` which refers to a schema which was not included in the dump.")
EXPECT_STDOUT_CONTAINS("ERROR: The excludeTables option contains a table `dummy`.`@` which refers to a schema which was not included in the dump.")
EXPECT_STDOUT_CONTAINS("ERROR: The excludeTables option contains a table `@`.`@` which refers to a schema which was not included in the dump.")
EXPECT_STDOUT_CONTAINS("ERROR: The excludeTables option contains a table `@`.`dummy` which refers to a schema which was not included in the dump.")

#@<> WL13807-FR4.11.3 - If the `excludeTables` option is not given, a default value of an empty list must be used instead.
# WL13807-TSFR4_32
EXPECT_SUCCESS([test_schema], test_output_absolute, { "ddlOnly": True, "showProgress": False })
EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(test_schema, test_table_non_unique) + ".sql")))
EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(test_schema, test_table_primary) + ".sql")))
EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(test_schema, test_table_unique) + ".sql")))
EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(test_schema, test_table_no_index) + ".sql")))

#@<> WL13807-FR6 - The data dumps for the following tables must be excluded from any dumps that include their respective schemas. DDL must still be generated:
# * `mysql.apply_status`
# * `mysql.general_log`
# * `mysql.schema`
# * `mysql.slow_log`
# WL13807-TSFR6_1
EXPECT_SUCCESS(["mysql"], test_output_absolute, { "chunking": False, "showProgress": False })

for table in ["apply_status", "general_log", "schema", "slow_log"]:
    EXPECT_FALSE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename("mysql", table) + ".tsv.zst")))
    exists = os.path.isfile(os.path.join(test_output_absolute, encode_table_basename("mysql", table) + ".sql"))
    if 1 == session.run_sql("SELECT COUNT(*) FROM information_schema.tables WHERE TABLE_SCHEMA = 'mysql' AND TABLE_NAME = ?", [table]).fetch_one()[0]:
        EXPECT_TRUE(exists)
    else:
        EXPECT_FALSE(exists)

EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename("mysql", "user") + ".sql")))
EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename("mysql", "user") + ".tsv.zst")))

#@<> run dump for all SQL-related tests below
EXPECT_SUCCESS([types_schema, test_schema], test_output_absolute, { "ddlOnly": True, "showProgress": False })

#@<> WL13807-FR9 - For each schema dumped, a DDL file with the base name as specified in FR8 and `.sql` extension must be created in the output directory.
# * The schema DDL file must contain all objects being dumped, including routines and events if enabled by options.
# WL13807-TSFR9_1
EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, schema_ddl_file(types_schema, Schema_ddl.Schema))))
EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, schema_ddl_file(test_schema, Schema_ddl.Schema))))

#@<> WL13807-FR10 - For each table dumped, a DDL file must be created with a base name as specified in FR7.1, and `.sql` extension.
# * The table DDL file must contain all objects being dumped.
# WL13807-TSFR10_1
for table in types_schema_tables:
    EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(types_schema, table) + ".sql")))

for table in [test_table_primary, test_table_unique, test_table_non_unique, test_table_no_index]:
    EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(test_schema, table) + ".sql")))

#@<> WL13807-FR10.1 - If the `triggers` option was set to `true` and the dumped table has triggers, a DDL file must be created with a base name as specified in FR7.1, and `.triggers.sql` extension. File must contain all triggers for that table.
# WL13807-TSFR10_4
EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(test_schema, test_table_no_index) + ".triggers.sql")))

for table in [test_table_primary, test_table_unique, test_table_non_unique]:
    EXPECT_FALSE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(test_schema, table) + ".triggers.sql")))

#@<> WL13807-FR11 - For each view dumped, a DDL file must be created with a base name as specified in FR7.1, and `.sql` extension.
# WL13807-TSFR11_1
for view in types_schema_views:
    EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(types_schema, view) + ".sql")))

for view in [test_view]:
    EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(test_schema, view) + ".sql")))

#@<> WL13807-FR11.1 - For each view dumped, a DDL file must be created with a base name as specified in FR7.1, and `.pre.sql` extension. This file must contain SQL statements which create a table with the same structure as the dumped view.
# WL13807-TSFR11_3
for view in types_schema_views:
    EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(types_schema, view) + ".pre.sql")))

for view in [test_view]:
    EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, encode_table_basename(test_schema, view) + ".pre.sql")))

#@<> WL13807-FR12 - The following DDL files must be created in the output directory:
# * A file with the name `@.sql`, containing the SQL statements which should be executed before the whole dump is imported.
# * A file with the name `@.post.sql`, containing the SQL statements which should be executed after the whole dump is imported.
# WL13807-TSFR12_1
for f in [ "@.sql", "@.post.sql" ]:
    EXPECT_TRUE(os.path.isfile(os.path.join(test_output_absolute, f)))

#@<> WL13807-FR13 - Once the dump is complete, in addition to the summary described in WL#13804, FR15, the following information must be presented to the user:
# * The number of schemas dumped.
# * The number of tables dumped.
# WL13807: WL13804-FR15 - Once the dump is complete, the summary of the export process must be presented to the user. It must contain:
# * The number of rows written.
# * The number of bytes actually written to the data dump files.
# * The number of data bytes written to the data dump files. (only if `compression` option is not set to `none`)
# * The average throughout in bytes written per second.
# WL13807-TSFR13_1
EXPECT_SUCCESS([types_schema, test_schema], test_output_absolute, { "ddlOnly": True, "showProgress": False })

EXPECT_STDOUT_CONTAINS("Schemas dumped: 2")
EXPECT_STDOUT_CONTAINS("Tables dumped: {0}".format(len(types_schema_tables) + 4))

# test without compression
EXPECT_SUCCESS([types_schema, test_schema], test_output_absolute, { "compression": "none", "ddlOnly": True, "showProgress": False })

EXPECT_STDOUT_CONTAINS("Schemas dumped: 2")
EXPECT_STDOUT_CONTAINS("Tables dumped: {0}".format(len(types_schema_tables) + 4))

#@<> WL13807-FR14 - SQL scripts for DDL must be idempotent. That is, executing the same script multiple times (including when some of the attempts have failed) should result in the same schema that would be left by a single successful execution.
# * All SQL statements which create objects must not fail if object with the same type and name already exist.
# * Existing schemas and tables with the same names as the ones being dumped must not be deleted.
# * Existing views with the same names as the ones being dumped may be deleted.
# * Existing functions, stored procedures and events with the same names as the ones being dumped must be deleted.
# * If the `triggers` option was set to `true`, existing triggers with the same names as the ones being dumped must be deleted.
EXPECT_SUCCESS([test_schema], test_output_absolute, { "ddlOnly": True, "showProgress": False })

all_sql_files = []
# order is important
all_sql_files.append("@.sql")
all_sql_files.append(schema_ddl_file(test_schema, Schema_ddl.Schema))

for table in [test_table_primary, test_table_unique, test_table_non_unique, test_table_no_index]:
    all_sql_files.append(encode_table_basename(test_schema, table) + ".sql")

for view in [test_view]:
    all_sql_files.append(encode_table_basename(test_schema, view) + ".pre.sql")

for view in [test_view]:
    all_sql_files.append(encode_table_basename(test_schema, view) + ".sql")

all_sql_files.append(encode_table_basename(test_schema, test_table_no_index) + ".triggers.sql")
all_sql_files.append("@.post.sql")

recreate_verification_schema()

for f in all_sql_files:
    for i in range(0, 2):
        EXPECT_EQ(0, testutil.call_mysqlsh([uri + "/" + verification_schema, "--sql", "--file", os.path.join(test_output_absolute, f)]))

drop_all_schemas(all_schemas)

#@<> WL13807-FR15 - All created files should have permissions set to rw-r-----, owner should be set to the user running Shell. {__os_type != "windows"}
# WL13807-TSFR15_1
EXPECT_SUCCESS([test_schema], test_output_absolute, { "showProgress": False })

for f in os.listdir(test_output_absolute):
    path = os.path.join(test_output_absolute, f)
    EXPECT_EQ(0o640, stat.S_IMODE(os.stat(path).st_mode))
    EXPECT_EQ(os.getuid(), os.stat(path).st_uid)

#@<> WL13807-FR16.1 - The `options` dictionary may contain a `ocimds` key with a Boolean or string value, which specifies whether the compatibility checks with `MySQL HeatWave Service` and DDL substitutions should be done.
TEST_BOOL_OPTION("ocimds")

#@<> tables with missing PKs
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

#@<> WL13807-FR16.1.1 - If the `ocimds` option is set to `true`, the following must be done:
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
# WL13807-TSFR16_1
# WL14506-TSFR_1_6
excluded_tables = []

for table in missing_pks[test_schema]:
    excluded_tables.append(quote_identifier(test_schema, table))

# references excluded table
excluded_tables.append(quote_identifier(test_schema, test_view))

target_version = "8.1.0"
EXPECT_FAIL("Error: Shell Error (52004)", "Compatibility issues were found", [incompatible_schema, test_schema], test_output_relative, { "targetVersion": target_version, "ocimds": True, "excludeTables": excluded_tables })
EXPECT_STDOUT_CONTAINS(f"Checking for compatibility with MySQL HeatWave Service {target_version}")

if __version_num < 80000:
    EXPECT_STDOUT_CONTAINS("NOTE: MySQL Server 5.7 detected, please consider upgrading to 8.0 first.")
    EXPECT_STDOUT_CONTAINS("Checking for potential upgrade issues.")

EXPECT_STDOUT_CONTAINS(comment_data_index_directory(incompatible_schema, incompatible_table_data_directory).fixed())

EXPECT_STDOUT_CONTAINS(comment_encryption(incompatible_schema, incompatible_table_encryption).fixed())

if __version_num < 80000 and __os_type != "windows":
    EXPECT_STDOUT_CONTAINS(comment_data_index_directory(incompatible_schema, incompatible_table_index_directory).fixed())

EXPECT_STDOUT_CONTAINS(force_innodb_unsupported_storage(incompatible_schema, incompatible_table_index_directory).error())

EXPECT_STDOUT_CONTAINS(strip_tablespaces(incompatible_schema, incompatible_table_tablespace).error())

EXPECT_STDOUT_CONTAINS(force_innodb_unsupported_storage(incompatible_schema, incompatible_table_wrong_engine).error())

EXPECT_STDOUT_CONTAINS(strip_definers_definer_clause(incompatible_schema, incompatible_view).error())
EXPECT_STDOUT_CONTAINS(strip_definers_security_clause(incompatible_schema, incompatible_view).error())

EXPECT_STDOUT_CONTAINS(f"Compatibility issues with MySQL HeatWave Service {target_version} were found. Please use the 'compatibility' option to apply compatibility adaptations to the dumped DDL.")

# WL14506-FR1 - When a dump is executed with the ocimds option set to true, for each table that would be dumped which does not contain a primary key, an error must be reported.
# WL14506-TSFR_1_11
for table in missing_pks[incompatible_schema]:
    EXPECT_STDOUT_CONTAINS(create_invisible_pks(incompatible_schema, table).error())

# WL14506-TSFR_1_6
for table in missing_pks["xtest"] + missing_pks[test_schema]:
    EXPECT_STDOUT_NOT_CONTAINS(table)

# WL14506-FR1.1 - The following message must be printed:
# WL14506-TSFR_1_11
EXPECT_STDOUT_CONTAINS("""
ERROR: One or more tables without Primary Keys were found.

       MySQL HeatWave Service High Availability (MySQL HeatWave Service HA) requires Primary Keys to be present in all tables.
       To continue with the dump you must do one of the following:

       * Create PRIMARY keys (regular or invisible) in all tables before dumping them.
         MySQL 8.0.23 supports the creation of invisible columns to allow creating Primary Key columns with no impact to applications. For more details, see https://dev.mysql.com/doc/refman/en/invisible-columns.html.
         This is considered a best practice for both performance and usability and will work seamlessly with MySQL HeatWave Service.

       * Add the "create_invisible_pks" to the "compatibility" option.
         The dump will proceed and loader will automatically add Primary Keys to tables that don't have them when loading into MySQL HeatWave Service.
         This will make it possible to enable HA in MySQL HeatWave Service without application impact and without changes to the source database.
         Inbound Replication into a DB System HA instance will also be possible, as long as the instance has version 8.0.32 or newer. For more information, see https://docs.oracle.com/en-us/iaas/mysql-database/doc/creating-replication-channel.html.

       * Add the "ignore_missing_pks" to the "compatibility" option.
         This will disable this check and the dump will be produced normally, Primary Keys will not be added automatically.
         It will not be possible to load the dump in an HA enabled DB System instance.
""")

#@<> WL14506-TSFR_2_1
util.help("dump_schemas")

EXPECT_STDOUT_CONTAINS("""
      create_invisible_pks - Each table which does not have a Primary Key will
      have one created when the dump is loaded. The following Primary Key is
      added to the table:
      `my_row_id` BIGINT UNSIGNED AUTO_INCREMENT INVISIBLE PRIMARY KEY

      Dumps created with this value can be used with Inbound Replication into
      an MySQL HeatWave Service DB System instance with High Availability, as
      long as target instance has version 8.0.32 or newer. Mutually exclusive
      with the ignore_missing_pks value.
""")
EXPECT_STDOUT_CONTAINS("""
      ignore_missing_pks - Ignore errors caused by tables which do not have
      Primary Keys. Dumps created with this value cannot be used in MySQL
      HeatWave Service DB System instance with High Availability. Mutually
      exclusive with the create_invisible_pks value.
""")
EXPECT_STDOUT_CONTAINS("""
      In order to use Inbound Replication into an MySQL HeatWave Service DB
      System instance with High Availability where instance has version older
      than 8.0.32, all tables at the source server need to have Primary Keys.
      This needs to be fixed manually before running the dump. Starting with
      MySQL 8.0.23 invisible columns may be used to add Primary Keys without
      changing the schema compatibility, for more information see:
      https://dev.mysql.com/doc/refman/en/invisible-columns.html.

      In order to use Inbound Replication into an MySQL HeatWave Service DB
      System instance with High Availability, please see
      https://docs.oracle.com/en-us/iaas/mysql-database/doc/creating-replication-channel.html.

      In order to use MySQL HeatWave Service DB Service instance with High
      Availability, all tables must have a Primary Key. This can be fixed
      automatically using the create_invisible_pks compatibility value.
""")

#@<> WL14506-TSFR_1_7
EXPECT_SUCCESS([incompatible_schema], test_output_absolute, { "ocimds": True, "dataOnly": True, "showProgress": False })

#@<> WL13807-FR16.1.2 - If the `ocimds` option is not given, a default value of `false` must be used instead.
# WL13807-TSFR16_2
# WL14506-TSFR_1_5
EXPECT_SUCCESS([incompatible_schema], test_output_absolute, { "ddlOnly": True, "showProgress": False })

#@<> WL13807-FR16.2 - The `options` dictionary may contain a `compatibility` key with an array of strings value, which specifies the `MySQL HeatWave Service`-related compatibility modifications that should be applied when creating the DDL files.
TEST_ARRAY_OF_STRINGS_OPTION("compatibility")

EXPECT_FAIL("ValueError", "Argument #3: Unknown compatibility option: dummy", [incompatible_schema], test_output_relative, { "compatibility": [ "dummy" ] })
EXPECT_FAIL("ValueError", "Argument #3: Unknown compatibility option: ", [incompatible_schema], test_output_relative, { "compatibility": [ "" ] })

#@<> WL13807-FR16.2.1 - The `compatibility` option may contain the following values:
# * `force_innodb` - replace incompatible table engines with `InnoDB`,
# * `strip_restricted_grants` - remove disallowed grants.
# * `strip_tablespaces` - remove unsupported tablespace syntax.
# * `strip_definers` - remove DEFINER clause from views, triggers, events and routines and change SQL SECURITY property to INVOKER for views and routines.
# WL14506-FR2 - The compatibility option of the dump utilities must support a new value: ignore_missing_pks.
# WL14506-TSFR_1_12
EXPECT_SUCCESS([incompatible_schema], test_output_absolute, { "ocimds": True, "compatibility": [ "force_innodb", "ignore_missing_pks", "strip_definers", "strip_restricted_grants", "strip_tablespaces" ] , "ddlOnly": True, "showProgress": False })

# WL14506-FR2.2 - The following message must be printed:
# WL14506-TSFR_1_12
EXPECT_STDOUT_CONTAINS("""
NOTE: One or more tables without Primary Keys were found.

      This issue is ignored.
      This dump cannot be loaded into an MySQL HeatWave Service DB System instance with High Availability.
""")

# WL14506-FR3 - The compatibility option of the dump utilities must support a new value: create_invisible_pks.
# create_invisible_pks and ignore_missing_pks are mutually exclusive
# WL14506-TSFR_1_13
EXPECT_SUCCESS([incompatible_schema], test_output_absolute, { "ocimds": True, "compatibility": [ "create_invisible_pks", "force_innodb", "strip_definers", "strip_restricted_grants", "strip_tablespaces" ] , "ddlOnly": True, "showProgress": False })

# WL14506-FR3.1.1 - The following message must be printed:
# WL14506-TSFR_1_13
EXPECT_STDOUT_CONTAINS("""
NOTE: One or more tables without Primary Keys were found.

      Missing Primary Keys will be created automatically when this dump is loaded.
      This will make it possible to enable High Availability in MySQL HeatWave Service DB System instance without application impact and without changes to the source database.
      Inbound Replication into a DB System HA instance will also be possible, as long as the instance has version 8.0.32 or newer. For more information, see https://docs.oracle.com/en-us/iaas/mysql-database/doc/creating-replication-channel.html.
""")

#@<> WL14506-FR3.1 - When a dump is executed with the ocimds option set to true and the compatibility option contains the create_invisible_pks value, for each table that would be dumped which does not contain a primary key, an information should be printed that an invisible primary key will be created when loading the dump.
EXPECT_SUCCESS([incompatible_schema], test_output_absolute, { "compatibility": [ "create_invisible_pks" ] , "ddlOnly": True, "showProgress": False })

for table in missing_pks[incompatible_schema]:
    EXPECT_STDOUT_CONTAINS(create_invisible_pks(incompatible_schema, table).fixed())

#@<> WL14506-FR3.2 - When a dump is executed and the compatibility option contains the create_invisible_pks value, for each table that would be dumped which does not contain a primary key but has a column named my_row_id, an error must be reported.
# WL14506-TSFR_3.2_2
table = missing_pks[incompatible_schema][0]
session.run_sql("ALTER TABLE !.! ADD COLUMN my_row_id int;", [incompatible_schema, table])

EXPECT_FAIL("Error: Shell Error (52006)", re.compile(r"While '.*': Fatal error during dump"), [incompatible_schema], test_output_relative, { "compatibility": [ "create_invisible_pks" ] }, True)
EXPECT_STDOUT_CONTAINS(create_invisible_pks_name_conflict(incompatible_schema, table).error())
EXPECT_STDOUT_CONTAINS("Could not apply some of the compatibility options")

WIPE_OUTPUT()
EXPECT_FAIL("Error: Shell Error (52004)", "Compatibility issues were found", [incompatible_schema], test_output_relative, { "ocimds": True, "compatibility": [ "create_invisible_pks" ] })
EXPECT_STDOUT_CONTAINS(create_invisible_pks_name_conflict(incompatible_schema, table).error())

session.run_sql("ALTER TABLE !.! DROP COLUMN my_row_id;", [incompatible_schema, table])

#@<> WL14506-FR3.4 - When a dump is executed and the compatibility option contains the create_invisible_pks value, for each table that would be dumped which does not contain a primary key but has a column with an AUTO_INCREMENT attribute, an error must be reported.
table = missing_pks[incompatible_schema][0]
session.run_sql("ALTER TABLE !.! ADD COLUMN idx int AUTO_INCREMENT UNIQUE;", [incompatible_schema, table])

EXPECT_FAIL("Error: Shell Error (52006)", re.compile(r"While '.*': Fatal error during dump"), [incompatible_schema], test_output_relative, { "compatibility": [ "create_invisible_pks" ] }, True)
EXPECT_STDOUT_CONTAINS(create_invisible_pks_auto_increment_conflict(incompatible_schema, table).error())
EXPECT_STDOUT_CONTAINS("Could not apply some of the compatibility options")

WIPE_OUTPUT()
EXPECT_FAIL("Error: Shell Error (52004)", "Compatibility issues were found", [incompatible_schema], test_output_relative, { "ocimds": True, "compatibility": [ "create_invisible_pks" ] })
EXPECT_STDOUT_CONTAINS(create_invisible_pks_auto_increment_conflict(incompatible_schema, table).error())

session.run_sql("ALTER TABLE !.! DROP COLUMN idx;", [incompatible_schema, table])

#@<> WL14506-FR3.2 + WL14506-FR3.4 - same column
table = missing_pks[incompatible_schema][0]
session.run_sql("ALTER TABLE !.! ADD COLUMN my_row_id int AUTO_INCREMENT UNIQUE;", [incompatible_schema, table])

EXPECT_FAIL("Error: Shell Error (52006)", re.compile(r"While '.*': Fatal error during dump"), [incompatible_schema], test_output_relative, { "compatibility": [ "create_invisible_pks" ] }, True)
EXPECT_STDOUT_CONTAINS(create_invisible_pks_name_conflict(incompatible_schema, table).error())
EXPECT_STDOUT_CONTAINS(create_invisible_pks_auto_increment_conflict(incompatible_schema, table).error())
EXPECT_STDOUT_CONTAINS("Could not apply some of the compatibility options")

WIPE_OUTPUT()
EXPECT_FAIL("Error: Shell Error (52004)", "Compatibility issues were found", [incompatible_schema], test_output_relative, { "ocimds": True, "compatibility": [ "create_invisible_pks" ] })
EXPECT_STDOUT_CONTAINS(create_invisible_pks_name_conflict(incompatible_schema, table).error())
EXPECT_STDOUT_CONTAINS(create_invisible_pks_auto_increment_conflict(incompatible_schema, table).error())

session.run_sql("ALTER TABLE !.! DROP COLUMN my_row_id;", [incompatible_schema, table])

#@<> WL14506-FR3.2 + WL14506-FR3.4 - different columns
table = missing_pks[incompatible_schema][0]
session.run_sql("ALTER TABLE !.! ADD COLUMN my_row_id int;", [incompatible_schema, table])
session.run_sql("ALTER TABLE !.! ADD COLUMN idx int AUTO_INCREMENT UNIQUE;", [incompatible_schema, table])

EXPECT_FAIL("Error: Shell Error (52006)", re.compile(r"While '.*': Fatal error during dump"), [incompatible_schema], test_output_relative, { "compatibility": [ "create_invisible_pks" ] }, True)
EXPECT_STDOUT_CONTAINS(create_invisible_pks_name_conflict(incompatible_schema, table).error())
EXPECT_STDOUT_CONTAINS(create_invisible_pks_auto_increment_conflict(incompatible_schema, table).error())
EXPECT_STDOUT_CONTAINS("Could not apply some of the compatibility options")

WIPE_OUTPUT()
EXPECT_FAIL("Error: Shell Error (52004)", "Compatibility issues were found", [incompatible_schema], test_output_relative, { "ocimds": True, "compatibility": [ "create_invisible_pks" ] })
EXPECT_STDOUT_CONTAINS(create_invisible_pks_name_conflict(incompatible_schema, table).error())
EXPECT_STDOUT_CONTAINS(create_invisible_pks_auto_increment_conflict(incompatible_schema, table).error())

session.run_sql("ALTER TABLE !.! DROP COLUMN my_row_id;", [incompatible_schema, table])
session.run_sql("ALTER TABLE !.! DROP COLUMN idx;", [incompatible_schema, table])

#@<> WL14506-FR3.3 - When a dump is executed and the compatibility option contains both create_invisible_pks and ignore_missing_pks values, an error must be reported and process must be aborted.
# WL14506-TSFR_3.3_1
EXPECT_FAIL("ValueError", "Argument #3: The 'create_invisible_pks' and 'ignore_missing_pks' compatibility options cannot be used at the same time.", [incompatible_schema], test_output_relative, { "compatibility": [ "create_invisible_pks", "ignore_missing_pks" ] })

#@<> WL13807-FR16.2.1 - force_innodb
# WL13807-TSFR16_3
EXPECT_SUCCESS([incompatible_schema], test_output_absolute, { "compatibility": [ "force_innodb" ] , "ddlOnly": True, "showProgress": False })
EXPECT_STDOUT_CONTAINS(force_innodb_unsupported_storage(incompatible_schema, incompatible_table_index_directory).fixed())
EXPECT_STDOUT_CONTAINS(force_innodb_unsupported_storage(incompatible_schema, incompatible_table_wrong_engine).fixed())

#@<> WL14506-FR2.1 - When a dump is executed with the ocimds option set to true and the compatibility option contains the ignore_missing_pks value, for each table that would be dumped which does not contain a primary key, a note must be displayed, stating that this issue is ignored.
EXPECT_SUCCESS([incompatible_schema], test_output_absolute, { "compatibility": [ "ignore_missing_pks" ] , "ddlOnly": True, "showProgress": False })

for table in missing_pks[incompatible_schema]:
    EXPECT_STDOUT_CONTAINS(ignore_missing_pks(incompatible_schema, table).fixed())

#@<> WL13807-FR16.2.1 - strip_definers
EXPECT_SUCCESS([incompatible_schema], test_output_absolute, { "compatibility": [ "strip_definers" ] , "ddlOnly": True, "showProgress": False })
EXPECT_STDOUT_CONTAINS(strip_definers_definer_clause(incompatible_schema, incompatible_view).fixed())
EXPECT_STDOUT_CONTAINS(strip_definers_security_clause(incompatible_schema, incompatible_view).fixed())

#@<> WL13807-FR16.2.1 - strip_tablespaces
# WL13807-TSFR16_4
EXPECT_SUCCESS([incompatible_schema], test_output_absolute, { "compatibility": [ "strip_tablespaces" ] , "ddlOnly": True, "showProgress": False })
EXPECT_STDOUT_CONTAINS(strip_tablespaces(incompatible_schema, incompatible_table_tablespace).fixed())

#@<> WL13807-FR16.2.2 - If the `compatibility` option is not given, a default value of `[]` must be used instead.
# WL13807-TSFR16_6
EXPECT_SUCCESS([incompatible_schema], test_output_absolute, { "ddlOnly": True, "showProgress": False })
EXPECT_STDOUT_NOT_CONTAINS(force_innodb_unsupported_storage(incompatible_schema, incompatible_table_index_directory).fixed())
EXPECT_STDOUT_NOT_CONTAINS(force_innodb_unsupported_storage(incompatible_schema, incompatible_table_wrong_engine).fixed())
EXPECT_STDOUT_NOT_CONTAINS(strip_definers_definer_clause(incompatible_schema, incompatible_view).fixed())
EXPECT_STDOUT_NOT_CONTAINS(strip_definers_security_clause(incompatible_schema, incompatible_view).fixed())
EXPECT_STDOUT_NOT_CONTAINS(strip_tablespaces(incompatible_schema, incompatible_table_tablespace).fixed())

# WL13807-FR7 - The table data dump must be written in the output directory, to a file with a base name as specified in FR7.1, and a `.tsv` extension.
# For WL13807-FR7.1 please see above.
# WL13807-FR7.2 - If multiple table data dump files are to be created (the `chunking` option is set to `true`), the base name of each file must consist of a base name, as specified in FR6, followed by `@N` (or `@@N` in case of the last file), where `N` is the zero-based ordinal number of the file.
# WL13807-FR7.3 - The format of an output file must be the same as the default format used by the `util.importTable()` function, as specified in WL#12193.
# WL13807-FR7.4 - While the data is being written to an output file, the data dump file must be suffixed with a `.dumping` extension. Once writing is finished, the file must be renamed to its original name.
# Data consistency tests.

#@<> test a single table with no chunking
EXPECT_SUCCESS([world_x_schema], test_output_absolute, { "chunking": False, "compression": "none", "showProgress": False })
TEST_LOAD(world_x_schema, world_x_table)

#@<> test multiple tables with chunking
EXPECT_SUCCESS([test_schema], test_output_absolute, { "bytesPerChunk": "1M", "compression": "none", "showProgress": False })
TEST_LOAD(test_schema, test_table_primary)
TEST_LOAD(test_schema, test_table_unique)
# these are not chunked because they don't have an appropriate column
TEST_LOAD(test_schema, test_table_non_unique)
TEST_LOAD(test_schema, test_table_no_index)

#@<> test multiple tables with various data types
EXPECT_SUCCESS([types_schema], test_output_absolute, { "chunking": False, "compression": "none", "showProgress": False })

for table in types_schema_tables:
    TEST_LOAD(types_schema, table)

#@<> dump table when different character set/SQL mode is used
session.run_sql("SET NAMES 'latin1';")
session.run_sql("SET GLOBAL SQL_MODE='ANSI_QUOTES,NO_AUTO_VALUE_ON_ZERO,NO_BACKSLASH_ESCAPES,NO_DIR_IN_CREATE,NO_ZERO_DATE,PAD_CHAR_TO_FULL_LENGTH';")

EXPECT_SUCCESS([world_x_schema], test_output_absolute, { "defaultCharacterSet": "latin1", "bytesPerChunk": "1M", "compression": "none", "showProgress": False })
TEST_LOAD(world_x_schema, world_x_table)

session.run_sql("SET NAMES 'utf8mb4';")
session.run_sql("SET GLOBAL SQL_MODE='';")

#@<> prepare user privileges, switch user
session.run_sql(f"GRANT ALL ON *.* TO {test_user_account};")
session.run_sql(f"REVOKE RELOAD /*!80023 , FLUSH_TABLES */ ON *.* FROM {test_user_account};")
shell.connect(test_user_uri(__mysql_sandbox_port1))

#@<> try to run consistent dump using a user which does not have required privileges for FTWRL but LOCK TABLES are ok
EXPECT_SUCCESS([types_schema], test_output_absolute, { "showProgress": False })
EXPECT_STDOUT_CONTAINS("WARNING: The current user lacks privileges to acquire a global read lock using 'FLUSH TABLES WITH READ LOCK'. Falling back to LOCK TABLES...")

# BUG#37226153 - if LOCK INSTANCE FOR BACKUP was executed, do not lock the mysql tables
if __version_num >= 80000:
    EXPECT_STDOUT_CONTAINS("NOTE: Instance locked for backup, skipping mysql system tables locks")

#@<> BUG#37226153 - revoke BACKUP_ADMIN, mysql tables will be locked {VER(>=8.0.0)}
setup_session()
session.run_sql(f"REVOKE BACKUP_ADMIN ON *.* FROM {test_user_account};")
shell.connect(test_user_uri(__mysql_sandbox_port1))

#@<> revoke lock tables from mysql.* {VER(>=8.0.16)}
setup_session()
session.run_sql("SET GLOBAL partial_revokes=1")
session.run_sql(f"REVOKE LOCK TABLES ON mysql.* FROM {test_user_account};")
shell.connect(test_user_uri(__mysql_sandbox_port1))

#@<> try again, this time it should succeed but without locking mysql.* tables {VER(>=8.0.16)}
EXPECT_SUCCESS([types_schema], test_output_absolute, { "showProgress": False })
EXPECT_STDOUT_CONTAINS("WARNING: The current user lacks privileges to acquire a global read lock using 'FLUSH TABLES WITH READ LOCK'. Falling back to LOCK TABLES...")
EXPECT_STDOUT_CONTAINS(f"WARNING: Could not lock mysql system tables: User {test_user_account} is missing the following privilege(s) for schema `mysql`: LOCK TABLES.")

#@<> revoke lock tables from the rest
setup_session()
session.run_sql(f"REVOKE LOCK TABLES ON *.* FROM {test_user_account};")
shell.connect(test_user_uri(__mysql_sandbox_port1))

#@<> try to run consistent dump using a user which does not have any required privileges
EXPECT_FAIL("Error: Shell Error (52002)", re.compile(r"While 'Initializing': Unable to lock tables: User {0} is missing the following privilege\(s\) for table `.+`\.`.+`: LOCK TABLES.".format(test_user_account)), [types_schema], test_output_absolute, { "showProgress": False })
EXPECT_STDOUT_CONTAINS("WARNING: The current user lacks privileges to acquire a global read lock using 'FLUSH TABLES WITH READ LOCK'. Falling back to LOCK TABLES...")
EXPECT_STDOUT_CONTAINS("ERROR: Unable to acquire global read lock neither table read locks")

#@<> using the same user, run inconsistent dump
EXPECT_SUCCESS([types_schema], test_output_absolute, { "consistent": False, "ddlOnly": True, "showProgress": False })

#@<> BUG#33697289 additional consistency checks if user is missing some of the privileges
# setup
setup_session()
session.run_sql(f"GRANT ALL ON *.* TO {test_user_account};")
# user cannot execute FTWRL and LOCK INSTANCE FOR BACKUP
session.run_sql(f"REVOKE RELOAD /*!80023 , FLUSH_TABLES */ /*!80000 , BACKUP_ADMIN */ ON *.* FROM {test_user_account};")

tested_schema = "test_schema"
tested_table = "test_table"
reason = f"available to the account {test_user_account}" if __version_num >= 80000 else "supported in MySQL 5.7"

# constantly create tables
constantly_create_tables = Generate_transactions(uri, tested_schema, tested_table)

# connect
shell.connect(test_user_uri(__mysql_sandbox_port1))

#@<> BUG#33697289 create a process which will add tables in the background
constantly_create_tables.generate_ddl()

#@<> BUG#33697289 test - backup lock is not available
EXPECT_SUCCESS([ tested_schema ], test_output_absolute, { "consistent": True, "showProgress": False, "threads": 1 })
EXPECT_STDOUT_MATCHES(re.compile(r"NOTE: The value of the gtid_executed system variable has changed during the dump, from: '.+' to: '.+'."))
EXPECT_STDOUT_MATCHES(re.compile(r"NOTE: Checking \d+ recent transactions for schema changes, use the 'skipConsistencyChecks' option to skip this check."))
EXPECT_STDOUT_CONTAINS("WARNING: DDL changes detected during DDL dump without a lock.")
EXPECT_STDOUT_CONTAINS(f"WARNING: Backup lock is not {reason} and DDL changes were not blocked. The consistency of the dump cannot be guaranteed.")

#@<> BUG#33697289 terminate the process immediately
constantly_create_tables.stop()

#@<> BUG#33697289 create a process which will add tables in the background (2) {not __dbug_off}
constantly_create_tables.generate_ddl()

#@<> BUG#33697289 fail if gtid is disabled and DDL changes {not __dbug_off}
testutil.dbug_set("+d,dumper_gtid_disabled")

EXPECT_SUCCESS([ tested_schema ], test_output_absolute, { "consistent": True, "showProgress": False, "threads": 1 })
EXPECT_STDOUT_MATCHES(re.compile(r"NOTE: The binlog position has changed during the dump, from: '.+' to: '.+'."))
EXPECT_STDOUT_CONTAINS("NOTE: Checking recent transactions for schema changes, use the 'skipConsistencyChecks' option to skip this check.")
EXPECT_STDOUT_CONTAINS("WARNING: DDL changes detected during DDL dump without a lock.")
EXPECT_STDOUT_CONTAINS(f"WARNING: Backup lock is not {reason} and DDL changes were not blocked. The consistency of the dump cannot be guaranteed.")

testutil.dbug_set("")

#@<> BUG#33697289 terminate the process immediately, it will not stop on its own {not __dbug_off}
constantly_create_tables.stop(drop_schema=False)

#@<> BUG#33697289 warning if binlog and gtid are disabled {not __dbug_off}
testutil.dbug_set("+d,dumper_binlog_disabled,dumper_gtid_disabled")

EXPECT_SUCCESS([ tested_schema ], test_output_absolute, { "consistent": True, "showProgress": False })
EXPECT_STDOUT_CONTAINS(f"""
WARNING: The current user does not have required privileges to execute FLUSH TABLES WITH READ LOCK.
    Backup lock is not {reason} and DDL changes cannot be blocked.
    The gtid_mode system variable is set to OFF or OFF_PERMISSIVE.
    The log_bin system variable is set to OFF or the current user does not have required privileges to execute SHOW MASTER STATUS.
The consistency of the dump cannot be guaranteed.
""")

testutil.dbug_set("")

#@<> BUG#33697289 cleanup
session.run_sql("DROP SCHEMA IF EXISTS !;", [ tested_schema ])

#@<> reconnect to the user with full privilages, restore test user account
setup_session()
create_user()

#@<> An error should occur when dumping using oci+os://
EXPECT_FAIL("ValueError", "Directory handling for oci+os protocol is not supported.", [test_schema], 'oci+os://sakila')

#@<> BUG#32430402 metadata should contain information about binlog
EXPECT_SUCCESS([types_schema], test_output_absolute, { "ddlOnly": True, "showProgress": False })

with open(os.path.join(test_output_absolute, "@.json"), encoding="utf-8") as json_file:
    metadata = json.load(json_file)
    EXPECT_EQ(True, "binlogFile" in metadata, "'binlogFile' should be in metadata")
    EXPECT_EQ(True, "binlogPosition" in metadata, "'binlogPosition' should be in metadata")

#@<> WL14244 - help entries
util.help('dump_schemas')

# WL14244-TSFR_2_2
EXPECT_STDOUT_CONTAINS("""
      - includeTables: list of strings (default: empty) - List of tables or
        views to be included in the dump in the format of schema.table.
""")

# WL14244-TSFR_3_2
EXPECT_STDOUT_CONTAINS("""
      - includeRoutines: list of strings (default: empty) - List of routines to
        be included in the dump in the format of schema.routine.
""")

# WL14244-TSFR_4_2
EXPECT_STDOUT_CONTAINS("""
      - excludeRoutines: list of strings (default: empty) - List of routines to
        be excluded from the dump in the format of schema.routine.
""")

# WL16731-G1 - help entries - libraries
EXPECT_STDOUT_CONTAINS("""
      - libraries: bool (default: true) - Include library objects for each
        dumped schema.
""")

# WL16731-TSFR_1_2_1 - help entries - includeLibraries
EXPECT_STDOUT_CONTAINS("""
      - includeLibraries: list of strings (default: empty) - List of library
        objects to be included in the dump in the format of schema.library.
""")

# WL16731-TSFR_1_3_1 - help entries - excludeLibraries
EXPECT_STDOUT_CONTAINS("""
      - excludeLibraries: list of strings (default: empty) - List of library
        objects to be excluded from the dump in the format of schema.library.
""")

# WL14244-TSFR_5_2
EXPECT_STDOUT_CONTAINS("""
      - includeEvents: list of strings (default: empty) - List of events to be
        included in the dump in the format of schema.event.
""")

# WL14244-TSFR_6_2
EXPECT_STDOUT_CONTAINS("""
      - excludeEvents: list of strings (default: empty) - List of events to be
        excluded from the dump in the format of schema.event.
""")

# WL14244-TSFR_7_2
EXPECT_STDOUT_CONTAINS("""
      - includeTriggers: list of strings (default: empty) - List of triggers to
        be included in the dump in the format of schema.table (all triggers
        from the specified table) or schema.table.trigger (the individual
        trigger).
""")

# WL14244-TSFR_8_2
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
    session.run_sql("CREATE SCHEMA existing_schema_1")
    session.run_sql("CREATE TABLE existing_schema_1.existing_table (id INT)")
    session.run_sql("CREATE VIEW existing_schema_1.existing_view AS SELECT * FROM existing_schema_1.existing_table")
    session.run_sql("CREATE EVENT existing_schema_1.existing_event ON SCHEDULE EVERY 1 YEAR DO BEGIN END")
    session.run_sql("CREATE FUNCTION existing_schema_1.existing_routine() RETURNS INT DETERMINISTIC RETURN 1")
    session.run_sql("CREATE TRIGGER existing_schema_1.existing_trigger AFTER DELETE ON existing_schema_1.existing_table FOR EACH ROW BEGIN END")
    session.run_sql("CREATE SCHEMA existing_schema_2")
    session.run_sql("CREATE TABLE existing_schema_2.existing_table (id INT)")
    session.run_sql("CREATE VIEW existing_schema_2.existing_view AS SELECT * FROM existing_schema_2.existing_table")
    session.run_sql("CREATE EVENT existing_schema_2.existing_event ON SCHEDULE EVERY 1 YEAR DO BEGIN END")
    session.run_sql("CREATE PROCEDURE existing_schema_2.existing_routine() DETERMINISTIC BEGIN END")
    session.run_sql("CREATE TRIGGER existing_schema_2.existing_trigger AFTER DELETE ON existing_schema_2.existing_table FOR EACH ROW BEGIN END")
    session.run_sql("CREATE SCHEMA not_specified_schema")
    session.run_sql("CREATE TABLE not_specified_schema.table (id INT)")
    session.run_sql("CREATE VIEW not_specified_schema.view AS SELECT * FROM not_specified_schema.table")
    session.run_sql("CREATE EVENT not_specified_schema.event ON SCHEDULE EVERY 1 YEAR DO BEGIN END")
    session.run_sql("CREATE PROCEDURE not_specified_schema.routine() DETERMINISTIC BEGIN END")
    session.run_sql("CREATE TRIGGER not_specified_schema.trigger AFTER DELETE ON not_specified_schema.table FOR EACH ROW BEGIN END")
    session.run_sql("CREATE SCHEMA existing_schema_3")
    session.run_sql("CREATE SCHEMA existing_schema_4")
    if instance_supports_libraries:
        session.run_sql("""
CREATE LIBRARY existing_schema_3.existing_library LANGUAGE JAVASCRIPT
AS $$
    export function squared(n) {
        return n * n;
    }
$$
            """)
        session.run_sql("""
CREATE FUNCTION existing_schema_3.existing_library_routine(n INTEGER) RETURNS INTEGER DETERMINISTIC LANGUAGE JAVASCRIPT
USING (existing_schema_3.existing_library)
AS $$
    return existing_library.squared(n);
$$
            """)
        session.run_sql("""
CREATE LIBRARY existing_schema_4.existing_library LANGUAGE JAVASCRIPT
AS $$
    export function pow(n, m) {
        return Math.pow(n, m);
    }
$$
            """)
        session.run_sql(f"""
CREATE PROCEDURE existing_schema_4.existing_library_routine(n INTEGER) LANGUAGE JAVASCRIPT
USING (existing_schema_3.existing_library AS l_1, existing_schema_4.existing_library AS l_2)
AS $$
    l_1.squared(n) + l_2.pow(n, 2);
$$
            """)
    # we're only interested in DDL, progress is not important
    options["ddlOnly"] = True
    options["showProgress"] = False
    # do the dump
    shutil.rmtree(test_output_absolute, True)
    util.dump_schemas(["existing_schema_1", "existing_schema_2", "existing_schema_3", "existing_schema_4"], test_output_absolute, options)
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

#@<> WL14244 - includeTables - invalid values
EXPECT_FAIL("ValueError", "Argument #3: The table to be included must be in the following form: schema.table, with optional backtick quotes, wrong value: 'table'.", [incompatible_schema], test_output_absolute, { "includeTables": [ "table" ] })
EXPECT_FAIL("ValueError", "Argument #3: Failed to parse table to be included 'table.@': Invalid character in identifier", [incompatible_schema], test_output_absolute, { "includeTables": [ "table.@" ] })

#@<> WL14244-TSFR_2_4
snapshot = dump_and_load({})
EXPECT_EQ(["existing_table"], entries(snapshot, ["existing_schema_1", "tables"]))
EXPECT_EQ(["existing_view"], entries(snapshot, ["existing_schema_1", "views"]))
EXPECT_EQ(["existing_table"], entries(snapshot, ["existing_schema_2", "tables"]))
EXPECT_EQ(["existing_view"], entries(snapshot, ["existing_schema_2", "views"]))

snapshot = dump_and_load({ "includeTables": [] })
EXPECT_EQ(["existing_table"], entries(snapshot, ["existing_schema_1", "tables"]))
EXPECT_EQ(["existing_view"], entries(snapshot, ["existing_schema_1", "views"]))
EXPECT_EQ(["existing_table"], entries(snapshot, ["existing_schema_2", "tables"]))
EXPECT_EQ(["existing_view"], entries(snapshot, ["existing_schema_2", "views"]))

#@<> WL14244-TSFR_2_6
snapshot = dump_and_load({ "includeTables": ['existing_schema_1.existing_table', 'existing_schema_1.non_existing_table', 'existing_schema_1.existing_view', 'existing_schema_1.non_existing_view'] })
EXPECT_EQ(["existing_table"], entries(snapshot, ["existing_schema_1", "tables"]))
EXPECT_EQ([], entries(snapshot, ["existing_schema_2", "tables"]))
EXPECT_EQ(["existing_view"], entries(snapshot, ["existing_schema_1", "views"]))
EXPECT_EQ([], entries(snapshot, ["existing_schema_2", "views"]))

#@<> BUG#33402791 - objects for schemas which are not included in the dump are reported as errors - includeTables
EXPECT_FAIL("ValueError", "Conflicting filtering options", [incompatible_schema], test_output_absolute, { "includeTables": [ 'non_existing_schema.view', 'non_existing_schema.table', 'not_specified_schema.table' ] })
EXPECT_STDOUT_CONTAINS("ERROR: The includeTables option contains a table `not_specified_schema`.`table` which refers to a schema which was not included in the dump.")
EXPECT_STDOUT_CONTAINS("ERROR: The includeTables option contains a table `non_existing_schema`.`table` which refers to a schema which was not included in the dump.")
EXPECT_STDOUT_CONTAINS("ERROR: The includeTables option contains a table `non_existing_schema`.`view` which refers to a schema which was not included in the dump.")

#@<> WL14244 - includeRoutines - invalid values
EXPECT_FAIL("ValueError", "Argument #3: The routine to be included must be in the following form: schema.routine, with optional backtick quotes, wrong value: 'routine'.", [incompatible_schema], test_output_absolute, { "includeRoutines": [ "routine" ] })
EXPECT_FAIL("ValueError", "Argument #3: Failed to parse routine to be included 'schema.@': Invalid character in identifier", [incompatible_schema], test_output_absolute, { "includeRoutines": [ "schema.@" ] })

#@<> WL14244-TSFR_3_5
snapshot = dump_and_load({})
EXPECT_EQ(["existing_routine"], entries(snapshot, ["existing_schema_1", "functions"]))
EXPECT_EQ([], entries(snapshot, ["existing_schema_1", "procedures"]))
EXPECT_EQ([], entries(snapshot, ["existing_schema_2", "functions"]))
EXPECT_EQ(["existing_routine"], entries(snapshot, ["existing_schema_2", "procedures"]))

snapshot = dump_and_load({ "includeRoutines": [] })
EXPECT_EQ(["existing_routine"], entries(snapshot, ["existing_schema_1", "functions"]))
EXPECT_EQ([], entries(snapshot, ["existing_schema_1", "procedures"]))
EXPECT_EQ([], entries(snapshot, ["existing_schema_2", "functions"]))
EXPECT_EQ(["existing_routine"], entries(snapshot, ["existing_schema_2", "procedures"]))

#@<> WL14244-TSFR_3_8
snapshot = dump_and_load({ "includeRoutines": ['existing_schema_1.existing_routine', 'existing_schema_1.non_existing_routine'] })
EXPECT_EQ(["existing_routine"], entries(snapshot, ["existing_schema_1", "functions"]))
EXPECT_EQ([], entries(snapshot, ["existing_schema_1", "procedures"]))
EXPECT_EQ([], entries(snapshot, ["existing_schema_2", "functions"]))
EXPECT_EQ([], entries(snapshot, ["existing_schema_2", "procedures"]))

#@<> BUG#33402791 - objects for schemas which are not included in the dump are reported as errors - includeRoutines
EXPECT_FAIL("ValueError", "Conflicting filtering options", [incompatible_schema], test_output_absolute, { "includeRoutines": [ 'non_existing_schema.routine', 'not_specified_schema.routine' ] })
EXPECT_STDOUT_CONTAINS("ERROR: The includeRoutines option contains a routine `not_specified_schema`.`routine` which refers to a schema which was not included in the dump.")
EXPECT_STDOUT_CONTAINS("ERROR: The includeRoutines option contains a routine `non_existing_schema`.`routine` which refers to a schema which was not included in the dump.")

#@<> WL16731-TSFR_1_7_1 - dumping without libraries + includeRoutines, note about routine using an excluded library is printed {instance_supports_libraries}
snapshot = dump_and_load({ "includeRoutines": ['existing_schema_4.existing_library_routine'], "libraries": False })
EXPECT_EQ([], entries(snapshot, ["existing_schema_3", "functions"]))
EXPECT_EQ([], entries(snapshot, ["existing_schema_3", "procedures"]))
EXPECT_EQ([], entries(snapshot, ["existing_schema_4", "functions"]))
# routine is dumped, but it's skipped by the loader, as it has a missing dependency
EXPECT_EQ([], entries(snapshot, ["existing_schema_4", "procedures"]))

EXPECT_STDOUT_CONTAINS(routine_references_excluded_library("Procedure", "existing_schema_4", "existing_library_routine", "existing_schema_3", "existing_library").note())
EXPECT_STDOUT_CONTAINS(routine_references_excluded_library("Procedure", "existing_schema_4", "existing_library_routine", "existing_schema_4", "existing_library").note())

#@<> WL14244 - excludeRoutines - invalid values
EXPECT_FAIL("ValueError", "Argument #3: The routine to be excluded must be in the following form: schema.routine, with optional backtick quotes, wrong value: 'routine'.", [incompatible_schema], test_output_absolute, { "excludeRoutines": [ "routine" ] })
EXPECT_FAIL("ValueError", "Argument #3: Failed to parse routine to be excluded 'schema.@': Invalid character in identifier", [incompatible_schema], test_output_absolute, { "excludeRoutines": [ "schema.@" ] })

#@<> WL14244-TSFR_4_5
snapshot = dump_and_load({})
EXPECT_EQ(["existing_routine"], entries(snapshot, ["existing_schema_1", "functions"]))
EXPECT_EQ([], entries(snapshot, ["existing_schema_1", "procedures"]))
EXPECT_EQ([], entries(snapshot, ["existing_schema_2", "functions"]))
EXPECT_EQ(["existing_routine"], entries(snapshot, ["existing_schema_2", "procedures"]))

snapshot = dump_and_load({ "excludeRoutines": [] })
EXPECT_EQ(["existing_routine"], entries(snapshot, ["existing_schema_1", "functions"]))
EXPECT_EQ([], entries(snapshot, ["existing_schema_1", "procedures"]))
EXPECT_EQ([], entries(snapshot, ["existing_schema_2", "functions"]))
EXPECT_EQ(["existing_routine"], entries(snapshot, ["existing_schema_2", "procedures"]))

#@<> WL14244-TSFR_4_8
snapshot = dump_and_load({ "excludeRoutines": ['existing_schema_1.existing_routine', 'existing_schema_1.non_existing_routine' ] })
EXPECT_EQ([], entries(snapshot, ["existing_schema_1", "functions"]))
EXPECT_EQ([], entries(snapshot, ["existing_schema_1", "procedures"]))
EXPECT_EQ([], entries(snapshot, ["existing_schema_2", "functions"]))
EXPECT_EQ(["existing_routine"], entries(snapshot, ["existing_schema_2", "procedures"]))

#@<> BUG#33402791 - objects for schemas which are not included in the dump are reported as errors - excludeRoutines
EXPECT_FAIL("ValueError", "Conflicting filtering options", [incompatible_schema], test_output_absolute, { "excludeRoutines": [ 'non_existing_schema.routine', 'not_specified_schema.routine' ] })
EXPECT_STDOUT_CONTAINS("ERROR: The excludeRoutines option contains a routine `not_specified_schema`.`routine` which refers to a schema which was not included in the dump.")
EXPECT_STDOUT_CONTAINS("ERROR: The excludeRoutines option contains a routine `non_existing_schema`.`routine` which refers to a schema which was not included in the dump.")

#@<> WL16731-TSFR_1_7_1 - dumping without libraries + excludeRoutines, note about routine using an excluded library is printed {instance_supports_libraries}
snapshot = dump_and_load({ "excludeRoutines": ['existing_schema_4.existing_library_routine'], "libraries": False })
# routine is dumped, but it's skipped by the loader, as it has a missing dependency
EXPECT_EQ([], entries(snapshot, ["existing_schema_3", "functions"]))
EXPECT_EQ([], entries(snapshot, ["existing_schema_3", "procedures"]))
EXPECT_EQ([], entries(snapshot, ["existing_schema_4", "functions"]))
EXPECT_EQ([], entries(snapshot, ["existing_schema_4", "procedures"]))

EXPECT_STDOUT_CONTAINS(routine_references_excluded_library("Function", "existing_schema_3", "existing_library_routine", "existing_schema_3", "existing_library").note())

#@<> WL16731-TSFR_1_2_1 - includeLibraries - invalid values
TEST_ARRAY_OF_STRINGS_OPTION("includeLibraries")
# WL16731-TSFR_1_4_2 - invalid format of includeLibraries entries
EXPECT_FAIL("ValueError", "Argument #3: The library to be included must be in the following form: schema.library, with optional backtick quotes, wrong value: 'library'.", [incompatible_schema], test_output_absolute, { "includeLibraries": [ "library" ] })
EXPECT_FAIL("ValueError", "Argument #3: Failed to parse library to be included 'schema.@': Invalid character in identifier", [incompatible_schema], test_output_absolute, { "includeLibraries": [ "schema.@" ] })

#@<> WL16731-G4 - includeLibraries - full dump
# WL16731-TSFR_1_2_1_2 - includeLibraries not set / set to an empty array
expected_libraries = []
if instance_supports_libraries:
    expected_libraries.append("existing_library")

snapshot = dump_and_load({})
EXPECT_EQ(expected_libraries, entries(snapshot, ["existing_schema_3", "libraries"]))
EXPECT_EQ(expected_libraries, entries(snapshot, ["existing_schema_4", "libraries"]))

snapshot = dump_and_load({ "includeLibraries": [] })
EXPECT_EQ(expected_libraries, entries(snapshot, ["existing_schema_3", "libraries"]))
EXPECT_EQ(expected_libraries, entries(snapshot, ["existing_schema_4", "libraries"]))

metadata_file = os.path.join(test_output_absolute, "@.json")

# WL16731-TSFR_1_9_1 - test the capability
if instance_supports_libraries:
    EXPECT_CAPABILITIES(metadata_file, [ multifile_schema_ddl_capability ])
else:
    EXPECT_NO_CAPABILITIES(metadata_file, [ multifile_schema_ddl_capability ])

#@<> WL16731-TSFR_1_7_2 - schema is not dumped, note about routine using an excluded library is printed {instance_supports_libraries}
EXPECT_SUCCESS(["existing_schema_4"], test_output_absolute, { "ddlOnly": True, "showProgress": False })

EXPECT_STDOUT_CONTAINS(routine_references_excluded_library("Procedure", "existing_schema_4", "existing_library_routine", "existing_schema_3", "existing_library").note())

#@<> WL16731-TSFR_1_4_1 - includeLibraries - filtered dump
# WL16731-G4, WL16731-TSFR_1_5_1 - non-existing objects
snapshot = dump_and_load({ "includeLibraries": ['existing_schema_3.existing_library', 'existing_schema_3.non_existing_library'] })
EXPECT_EQ(expected_libraries, entries(snapshot, ["existing_schema_3", "libraries"]))
EXPECT_EQ([], entries(snapshot, ["existing_schema_4", "libraries"]))

if instance_supports_libraries:
    # WL16731-TSFR_1_7_1 - note about routine using an excluded library is printed
    EXPECT_STDOUT_CONTAINS(routine_references_excluded_library("Procedure", "existing_schema_4", "existing_library_routine", "existing_schema_4", "existing_library").note())
    EXPECT_FILE_CONTAINS(f"CREATE DEFINER=`{__user}`@`{__host}` PROCEDURE `existing_library_routine`", os.path.join(test_output_absolute, schema_ddl_file("existing_schema_4", Schema_ddl.Routines)))

#@<> WL16731 - objects for schemas which are not included in the dump are reported as errors - includeLibraries
EXPECT_FAIL("ValueError", "Conflicting filtering options", [incompatible_schema], test_output_absolute, { "includeLibraries": [ 'non_existing_schema.library', 'not_specified_schema.library' ] })
EXPECT_STDOUT_CONTAINS("ERROR: The includeLibraries option contains a library `not_specified_schema`.`library` which refers to a schema which was not included in the dump.")
EXPECT_STDOUT_CONTAINS("ERROR: The includeLibraries option contains a library `non_existing_schema`.`library` which refers to a schema which was not included in the dump.")

#@<> WL16731-TSFR_1_3_1 - excludeLibraries - invalid values
TEST_ARRAY_OF_STRINGS_OPTION("excludeLibraries")
# WL16731-TSFR_1_4_2 - invalid format of excludeLibraries entries
EXPECT_FAIL("ValueError", "Argument #3: The library to be excluded must be in the following form: schema.library, with optional backtick quotes, wrong value: 'library'.", [incompatible_schema], test_output_absolute, { "excludeLibraries": [ "library" ] })
EXPECT_FAIL("ValueError", "Argument #3: Failed to parse library to be excluded 'schema.@': Invalid character in identifier", [incompatible_schema], test_output_absolute, { "excludeLibraries": [ "schema.@" ] })

#@<> WL16731-TSFR_1_3_1 - excludeLibraries - full dump
# WL16731-TSFR_1_3_1_1 - excludeLibraries not set / set to an empty array
snapshot = dump_and_load({})
EXPECT_EQ(expected_libraries, entries(snapshot, ["existing_schema_3", "libraries"]))
EXPECT_EQ(expected_libraries, entries(snapshot, ["existing_schema_4", "libraries"]))

snapshot = dump_and_load({ "excludeLibraries": [] })
EXPECT_EQ(expected_libraries, entries(snapshot, ["existing_schema_3", "libraries"]))
EXPECT_EQ(expected_libraries, entries(snapshot, ["existing_schema_4", "libraries"]))

#@<> WL16731-TSFR_1_4_1 - excludeLibraries - filtered dump
# WL16731-TSFR_1_3_1, WL16731-TSFR_1_5_1 - non-existing objects
snapshot = dump_and_load({ "excludeLibraries": ['existing_schema_3.existing_library', 'existing_schema_3.non_existing_library'] })
EXPECT_EQ([], entries(snapshot, ["existing_schema_3", "libraries"]))
EXPECT_EQ(expected_libraries, entries(snapshot, ["existing_schema_4", "libraries"]))

if instance_supports_libraries:
    # WL16731-TSFR_1_7_1 - note about routine using an excluded library is printed
    EXPECT_STDOUT_CONTAINS(routine_references_excluded_library("Function", "existing_schema_3", "existing_library_routine", "existing_schema_3", "existing_library").note())
    EXPECT_FILE_CONTAINS(f"CREATE DEFINER=`{__user}`@`{__host}` FUNCTION `existing_library_routine`", os.path.join(test_output_absolute, schema_ddl_file("existing_schema_3", Schema_ddl.Routines)))
    EXPECT_STDOUT_CONTAINS(routine_references_excluded_library("Procedure", "existing_schema_4", "existing_library_routine", "existing_schema_3", "existing_library").note())
    EXPECT_FILE_CONTAINS(f"CREATE DEFINER=`{__user}`@`{__host}` PROCEDURE `existing_library_routine`", os.path.join(test_output_absolute, schema_ddl_file("existing_schema_4", Schema_ddl.Routines)))

#@<> WL16731 - objects for schemas which are not included in the dump are reported as errors - excludeLibraries
EXPECT_FAIL("ValueError", "Conflicting filtering options", [incompatible_schema], test_output_absolute, { "excludeLibraries": [ 'non_existing_schema.library', 'not_specified_schema.library' ] })
EXPECT_STDOUT_CONTAINS("ERROR: The excludeLibraries option contains a library `not_specified_schema`.`library` which refers to a schema which was not included in the dump.")
EXPECT_STDOUT_CONTAINS("ERROR: The excludeLibraries option contains a library `non_existing_schema`.`library` which refers to a schema which was not included in the dump.")

#@<> WL14244 - includeEvents - invalid values
EXPECT_FAIL("ValueError", "Argument #3: The event to be included must be in the following form: schema.event, with optional backtick quotes, wrong value: 'event'.", [incompatible_schema], test_output_absolute, { "includeEvents": [ "event" ] })
EXPECT_FAIL("ValueError", "Argument #3: Failed to parse event to be included 'schema.@': Invalid character in identifier", [incompatible_schema], test_output_absolute, { "includeEvents": [ "schema.@" ] })

#@<> WL14244-TSFR_5_5
snapshot = dump_and_load({})
EXPECT_EQ(["existing_event"], entries(snapshot, ["existing_schema_1", "events"]))
EXPECT_EQ(["existing_event"], entries(snapshot, ["existing_schema_2", "events"]))

snapshot = dump_and_load({ "includeEvents": [] })
EXPECT_EQ(["existing_event"], entries(snapshot, ["existing_schema_1", "events"]))
EXPECT_EQ(["existing_event"], entries(snapshot, ["existing_schema_2", "events"]))

#@<> WL14244-TSFR_5_8
snapshot = dump_and_load({ "includeEvents": ['existing_schema_1.existing_event', 'existing_schema_1.non_existing_event'] })
EXPECT_EQ(["existing_event"], entries(snapshot, ["existing_schema_1", "events"]))
EXPECT_EQ([], entries(snapshot, ["existing_schema_2", "events"]))

#@<> BUG#33402791 - objects for schemas which are not included in the dump are reported as errors - includeEvents
EXPECT_FAIL("ValueError", "Conflicting filtering options", [incompatible_schema], test_output_absolute, { "includeEvents": [ 'non_existing_schema.event', 'not_specified_schema.event' ] })
EXPECT_STDOUT_CONTAINS("ERROR: The includeEvents option contains an event `not_specified_schema`.`event` which refers to a schema which was not included in the dump.")
EXPECT_STDOUT_CONTAINS("ERROR: The includeEvents option contains an event `non_existing_schema`.`event` which refers to a schema which was not included in the dump.")

#@<> WL14244 - excludeEvents - invalid values
EXPECT_FAIL("ValueError", "Argument #3: The event to be excluded must be in the following form: schema.event, with optional backtick quotes, wrong value: 'event'.", [incompatible_schema], test_output_absolute, { "excludeEvents": [ "event" ] })
EXPECT_FAIL("ValueError", "Argument #3: Failed to parse event to be excluded 'schema.@': Invalid character in identifier", [incompatible_schema], test_output_absolute, { "excludeEvents": [ "schema.@" ] })

#@<> WL14244-TSFR_6_5
snapshot = dump_and_load({})
EXPECT_EQ(["existing_event"], entries(snapshot, ["existing_schema_1", "events"]))
EXPECT_EQ(["existing_event"], entries(snapshot, ["existing_schema_2", "events"]))

snapshot = dump_and_load({ "excludeEvents": [] })
EXPECT_EQ(["existing_event"], entries(snapshot, ["existing_schema_1", "events"]))
EXPECT_EQ(["existing_event"], entries(snapshot, ["existing_schema_2", "events"]))

#@<> WL14244-TSFR_6_8
snapshot = dump_and_load({ "excludeEvents": ['existing_schema_1.existing_event', 'existing_schema_1.non_existing_event'] })
EXPECT_EQ([], entries(snapshot, ["existing_schema_1", "events"]))
EXPECT_EQ(["existing_event"], entries(snapshot, ["existing_schema_2", "events"]))

#@<> BUG#33402791 - objects for schemas which are not included in the dump are reported as errors - excludeEvents
EXPECT_FAIL("ValueError", "Conflicting filtering options", [incompatible_schema], test_output_absolute, { "excludeEvents": [ 'non_existing_schema.event', 'not_specified_schema.event' ] })
EXPECT_STDOUT_CONTAINS("ERROR: The excludeEvents option contains an event `not_specified_schema`.`event` which refers to a schema which was not included in the dump.")
EXPECT_STDOUT_CONTAINS("ERROR: The excludeEvents option contains an event `non_existing_schema`.`event` which refers to a schema which was not included in the dump.")

#@<> WL14244 - includeTriggers - invalid values
EXPECT_FAIL("ValueError", "Argument #3: The trigger to be included must be in the following form: schema.table or schema.table.trigger, with optional backtick quotes, wrong value: 'trigger'.", [incompatible_schema], test_output_absolute, { "includeTriggers": [ "trigger" ] })
EXPECT_FAIL("ValueError", "Argument #3: Failed to parse trigger to be included 'schema.@': Invalid character in identifier", [incompatible_schema], test_output_absolute, { "includeTriggers": [ "schema.@" ] })

#@<> WL14244-TSFR_7_6
snapshot = dump_and_load({})
EXPECT_EQ(["existing_trigger"], entries(snapshot, ["existing_schema_1", "tables", "existing_table", "triggers"]))
EXPECT_EQ(["existing_trigger"], entries(snapshot, ["existing_schema_2", "tables", "existing_table", "triggers"]))

snapshot = dump_and_load({ "includeTriggers": [] })
EXPECT_EQ(["existing_trigger"], entries(snapshot, ["existing_schema_1", "tables", "existing_table", "triggers"]))
EXPECT_EQ(["existing_trigger"], entries(snapshot, ["existing_schema_2", "tables", "existing_table", "triggers"]))

#@<> WL14244-TSFR_7_10
snapshot = dump_and_load({ "includeTriggers": ['existing_schema_1.existing_table', 'existing_schema_1.non_existing_table', 'existing_schema_2.existing_table.existing_trigger', 'existing_schema_1.existing_table.non_existing_trigger' ] })
EXPECT_EQ(["existing_trigger"], entries(snapshot, ["existing_schema_1", "tables", "existing_table", "triggers"]))
EXPECT_EQ(["existing_trigger"], entries(snapshot, ["existing_schema_2", "tables", "existing_table", "triggers"]))

#@<> BUG#33402791 - objects for schemas which are not included in the dump are reported as errors - includeTriggers
EXPECT_FAIL("ValueError", "Conflicting filtering options", [incompatible_schema], test_output_absolute, { "includeTables": [ incompatible_schema + "." + incompatible_table_wrong_engine ], "includeTriggers": [ 'non_existing_schema.table', 'not_specified_schema.table.trigger' ] })
EXPECT_STDOUT_CONTAINS("ERROR: The includeTriggers option contains a filter `non_existing_schema`.`table` which refers to a schema which was not included in the dump.")
EXPECT_STDOUT_CONTAINS("ERROR: The includeTriggers option contains a trigger `not_specified_schema`.`table`.`trigger` which refers to a schema which was not included in the dump.")
EXPECT_STDOUT_CONTAINS("ERROR: The includeTriggers option contains a filter `non_existing_schema`.`table` which refers to a table which was not included in the dump.")
EXPECT_STDOUT_CONTAINS("ERROR: The includeTriggers option contains a trigger `not_specified_schema`.`table`.`trigger` which refers to a table which was not included in the dump.")

#@<> WL14244 - excludeTriggers - invalid values
EXPECT_FAIL("ValueError", "Argument #3: The trigger to be excluded must be in the following form: schema.table or schema.table.trigger, with optional backtick quotes, wrong value: 'trigger'.", [incompatible_schema], test_output_absolute, { "excludeTriggers": [ "trigger" ] })
EXPECT_FAIL("ValueError", "Argument #3: Failed to parse trigger to be excluded 'schema.@': Invalid character in identifier", [incompatible_schema], test_output_absolute, { "excludeTriggers": [ "schema.@" ] })

#@<> WL14244-TSFR_8_6
snapshot = dump_and_load({})
EXPECT_EQ(["existing_trigger"], entries(snapshot, ["existing_schema_1", "tables", "existing_table", "triggers"]))
EXPECT_EQ(["existing_trigger"], entries(snapshot, ["existing_schema_2", "tables", "existing_table", "triggers"]))

snapshot = dump_and_load({ "excludeTriggers": [] })
EXPECT_EQ(["existing_trigger"], entries(snapshot, ["existing_schema_1", "tables", "existing_table", "triggers"]))
EXPECT_EQ(["existing_trigger"], entries(snapshot, ["existing_schema_2", "tables", "existing_table", "triggers"]))

#@<> WL14244-TSFR_8_10
snapshot = dump_and_load({ "excludeTriggers": ['existing_schema_1.existing_table', 'existing_schema_1.non_existing_table', 'existing_schema_2.existing_table.existing_trigger', 'existing_schema_1.existing_table.non_existing_trigger'] })
EXPECT_EQ([], entries(snapshot, ["existing_schema_1", "tables", "existing_table", "triggers"]))
EXPECT_EQ([], entries(snapshot, ["existing_schema_2", "tables", "existing_table", "triggers"]))

#@<> BUG#33402791 - objects for schemas which are not included in the dump are reported as errors - excludeTriggers
EXPECT_FAIL("ValueError", "Conflicting filtering options", [incompatible_schema], test_output_absolute, { "excludeTriggers": [ 'non_existing_schema.table', 'not_specified_schema.table.trigger' ] })
EXPECT_FAIL("ValueError", "Conflicting filtering options", [incompatible_schema], test_output_absolute, { "includeTables": [ incompatible_schema + "." + incompatible_table_wrong_engine ], "excludeTriggers": [ 'non_existing_schema.table', 'not_specified_schema.table.trigger' ] })
EXPECT_STDOUT_CONTAINS("ERROR: The excludeTriggers option contains a filter `non_existing_schema`.`table` which refers to a schema which was not included in the dump.")
EXPECT_STDOUT_CONTAINS("ERROR: The excludeTriggers option contains a trigger `not_specified_schema`.`table`.`trigger` which refers to a schema which was not included in the dump.")
EXPECT_STDOUT_CONTAINS("ERROR: The excludeTriggers option contains a filter `non_existing_schema`.`table` which refers to a table which was not included in the dump.")
EXPECT_STDOUT_CONTAINS("ERROR: The excludeTriggers option contains a trigger `not_specified_schema`.`table`.`trigger` which refers to a table which was not included in the dump.")

#@<> WL14244 - cleanup
session.run_sql("DROP SCHEMA IF EXISTS existing_schema_1")
session.run_sql("DROP SCHEMA IF EXISTS existing_schema_2")
session.run_sql("DROP SCHEMA IF EXISTS existing_schema_3")
session.run_sql("DROP SCHEMA IF EXISTS existing_schema_4")

#@<> includeX + excludeX conflicts - setup
wipeout_server(session)
# create sample DB structure
session.run_sql("CREATE SCHEMA a")
session.run_sql("CREATE TABLE a.t (id INT)")
session.run_sql("CREATE TABLE a.t1 (id INT)")
session.run_sql("CREATE SCHEMA b")
session.run_sql("CREATE TABLE b.t (id INT)")
session.run_sql("CREATE TABLE b.t1 (id INT)")

#@<> includeX + excludeX conflicts - helpers
def dump_with_conflicts(options, throws = True):
    WIPE_STDOUT()
    # we're only interested in warnings
    options["dryRun"] = True
    options["showProgress"] = False
    # do the dump
    shutil.rmtree(test_output_absolute, True)
    if throws:
        EXPECT_THROWS(lambda: util.dump_schemas([ "a", "b" ], test_output_absolute, options), "ValueError: Conflicting filtering options")
    else:
        # there could be some other exceptions, we ignore them
        try:
            util.dump_schemas([ "a", "b" ], test_output_absolute, options)
        except Exception as e:
            print("Exception:", e)

#@<> includeTables + excludeTables conflicts
# no conflicts
dump_with_conflicts({ "includeTables": [], "excludeTables": [] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTables")
EXPECT_STDOUT_NOT_CONTAINS("excludeTables")

dump_with_conflicts({ "includeTables": [ "a.t" ], "excludeTables": [] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTables")
EXPECT_STDOUT_NOT_CONTAINS("excludeTables")

dump_with_conflicts({ "includeTables": [], "excludeTables": [ "a.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTables")
EXPECT_STDOUT_NOT_CONTAINS("excludeTables")

dump_with_conflicts({ "includeTables": [ "a.t" ], "excludeTables": [ "b.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTables")
EXPECT_STDOUT_NOT_CONTAINS("excludeTables")

dump_with_conflicts({ "includeTables": [ "a.t" ], "excludeTables": [ "a.t1" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTables")
EXPECT_STDOUT_NOT_CONTAINS("excludeTables")

# conflicts
dump_with_conflicts({ "includeTables": [ "a.t" ], "excludeTables": [ "a.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeTables and excludeTables options contain a table `a`.`t`.")

dump_with_conflicts({ "includeTables": [ "a.t", "b.t" ], "excludeTables": [ "a.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeTables and excludeTables options contain a table `a`.`t`.")
EXPECT_STDOUT_NOT_CONTAINS("`b`.`t`")

dump_with_conflicts({ "includeTables": [ "a.t", "a.t1" ], "excludeTables": [ "a.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeTables and excludeTables options contain a table `a`.`t`.")
EXPECT_STDOUT_NOT_CONTAINS("`a`.`t1`")

dump_with_conflicts({ "includeTables": [ "a.t" ], "excludeTables": [ "a.t", "b.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeTables and excludeTables options contain a table `a`.`t`.")
EXPECT_STDOUT_NOT_CONTAINS("`b`.`t`")

dump_with_conflicts({ "includeTables": [ "a.t" ], "excludeTables": [ "a.t", "a.t1" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeTables and excludeTables options contain a table `a`.`t`.")
EXPECT_STDOUT_NOT_CONTAINS("`a`.`t1`")

#@<> includeEvents + excludeEvents conflicts
# no conflicts
dump_with_conflicts({ "includeEvents": [], "excludeEvents": [] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeEvents")
EXPECT_STDOUT_NOT_CONTAINS("excludeEvents")

dump_with_conflicts({ "includeEvents": [ "a.e" ], "excludeEvents": [] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeEvents")
EXPECT_STDOUT_NOT_CONTAINS("excludeEvents")

dump_with_conflicts({ "includeEvents": [], "excludeEvents": [ "a.e" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeEvents")
EXPECT_STDOUT_NOT_CONTAINS("excludeEvents")

dump_with_conflicts({ "includeEvents": [ "a.e" ], "excludeEvents": [ "b.e" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeEvents")
EXPECT_STDOUT_NOT_CONTAINS("excludeEvents")

dump_with_conflicts({ "includeEvents": [ "a.e" ], "excludeEvents": [ "a.e1" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeEvents")
EXPECT_STDOUT_NOT_CONTAINS("excludeEvents")

# conflicts
dump_with_conflicts({ "includeEvents": [ "a.e" ], "excludeEvents": [ "a.e" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeEvents and excludeEvents options contain an event `a`.`e`.")

dump_with_conflicts({ "includeEvents": [ "a.e", "b.e" ], "excludeEvents": [ "a.e" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeEvents and excludeEvents options contain an event `a`.`e`.")
EXPECT_STDOUT_NOT_CONTAINS("`b`.`e`")

dump_with_conflicts({ "includeEvents": [ "a.e", "a.e1" ], "excludeEvents": [ "a.e" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeEvents and excludeEvents options contain an event `a`.`e`.")
EXPECT_STDOUT_NOT_CONTAINS("`a`.`e1`")

dump_with_conflicts({ "includeEvents": [ "a.e" ], "excludeEvents": [ "a.e", "b.e" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeEvents and excludeEvents options contain an event `a`.`e`.")
EXPECT_STDOUT_NOT_CONTAINS("`b`.`e`")

dump_with_conflicts({ "includeEvents": [ "a.e" ], "excludeEvents": [ "a.e", "a.e1" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeEvents and excludeEvents options contain an event `a`.`e`.")
EXPECT_STDOUT_NOT_CONTAINS("`a`.`e1`")

#@<> includeRoutines + excludeRoutines conflicts
# no conflicts
dump_with_conflicts({ "includeRoutines": [], "excludeRoutines": [] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeRoutines")
EXPECT_STDOUT_NOT_CONTAINS("excludeRoutines")

dump_with_conflicts({ "includeRoutines": [ "a.r" ], "excludeRoutines": [] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeRoutines")
EXPECT_STDOUT_NOT_CONTAINS("excludeRoutines")

dump_with_conflicts({ "includeRoutines": [], "excludeRoutines": [ "a.r" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeRoutines")
EXPECT_STDOUT_NOT_CONTAINS("excludeRoutines")

dump_with_conflicts({ "includeRoutines": [ "a.r" ], "excludeRoutines": [ "b.r" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeRoutines")
EXPECT_STDOUT_NOT_CONTAINS("excludeRoutines")

dump_with_conflicts({ "includeRoutines": [ "a.r" ], "excludeRoutines": [ "a.r1" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeRoutines")
EXPECT_STDOUT_NOT_CONTAINS("excludeRoutines")

# conflicts
dump_with_conflicts({ "includeRoutines": [ "a.r" ], "excludeRoutines": [ "a.r" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeRoutines and excludeRoutines options contain a routine `a`.`r`.")

dump_with_conflicts({ "includeRoutines": [ "a.r", "b.r" ], "excludeRoutines": [ "a.r" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeRoutines and excludeRoutines options contain a routine `a`.`r`.")
EXPECT_STDOUT_NOT_CONTAINS("`b`.`r`")

dump_with_conflicts({ "includeRoutines": [ "a.r", "a.r1" ], "excludeRoutines": [ "a.r" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeRoutines and excludeRoutines options contain a routine `a`.`r`.")
EXPECT_STDOUT_NOT_CONTAINS("`a`.`r1`")

dump_with_conflicts({ "includeRoutines": [ "a.r" ], "excludeRoutines": [ "a.r", "b.r" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeRoutines and excludeRoutines options contain a routine `a`.`r`.")
EXPECT_STDOUT_NOT_CONTAINS("`b`.`r`")

dump_with_conflicts({ "includeRoutines": [ "a.r" ], "excludeRoutines": [ "a.r", "a.r1" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeRoutines and excludeRoutines options contain a routine `a`.`r`.")
EXPECT_STDOUT_NOT_CONTAINS("`a`.`r1`")

#@<> WL16731 - includeLibraries + excludeLibraries conflicts
# no conflicts
dump_with_conflicts({ "includeLibraries": [], "excludeLibraries": [] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeLibraries")
EXPECT_STDOUT_NOT_CONTAINS("excludeLibraries")

dump_with_conflicts({ "includeLibraries": [ "a.l" ], "excludeLibraries": [] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeLibraries")
EXPECT_STDOUT_NOT_CONTAINS("excludeLibraries")

dump_with_conflicts({ "includeLibraries": [], "excludeLibraries": [ "a.l" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeLibraries")
EXPECT_STDOUT_NOT_CONTAINS("excludeLibraries")

dump_with_conflicts({ "includeLibraries": [ "a.l" ], "excludeLibraries": [ "b.l" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeLibraries")
EXPECT_STDOUT_NOT_CONTAINS("excludeLibraries")

dump_with_conflicts({ "includeLibraries": [ "a.l" ], "excludeLibraries": [ "a.l1" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeLibraries")
EXPECT_STDOUT_NOT_CONTAINS("excludeLibraries")

dump_with_conflicts({ "excludeSchemas": [ "b" ], "includeLibraries": [ "a.l" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeLibraries")
EXPECT_STDOUT_NOT_CONTAINS("excludeLibraries")

dump_with_conflicts({ "excludeSchemas": [], "includeLibraries": [ "a.l" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeLibraries")
EXPECT_STDOUT_NOT_CONTAINS("excludeLibraries")

dump_with_conflicts({ "includeSchemas": [], "includeLibraries": [ "a.l" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeLibraries")
EXPECT_STDOUT_NOT_CONTAINS("excludeLibraries")

dump_with_conflicts({ "includeSchemas": [ "a" ], "includeLibraries": [ "a.l" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeLibraries")
EXPECT_STDOUT_NOT_CONTAINS("excludeLibraries")

dump_with_conflicts({ "excludeSchemas": [ "a" ], "excludeLibraries": [ "a.l" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeLibraries")
EXPECT_STDOUT_NOT_CONTAINS("excludeLibraries")

dump_with_conflicts({ "excludeSchemas": [ "b" ], "excludeLibraries": [ "a.l" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeLibraries")
EXPECT_STDOUT_NOT_CONTAINS("excludeLibraries")

dump_with_conflicts({ "excludeSchemas": [], "excludeLibraries": [ "a.l" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeLibraries")
EXPECT_STDOUT_NOT_CONTAINS("excludeLibraries")

dump_with_conflicts({ "includeSchemas": [], "excludeLibraries": [ "a.l" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeLibraries")
EXPECT_STDOUT_NOT_CONTAINS("excludeLibraries")

dump_with_conflicts({ "includeSchemas": [ "a" ], "excludeLibraries": [ "a.l" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeLibraries")
EXPECT_STDOUT_NOT_CONTAINS("excludeLibraries")

# WL16731-TSFR_1_6_1 - conflicts
dump_with_conflicts({ "includeLibraries": [ "a.l" ], "excludeLibraries": [ "a.l" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeLibraries and excludeLibraries options contain a library `a`.`l`.")

dump_with_conflicts({ "includeLibraries": [ "a.l", "b.l" ], "excludeLibraries": [ "a.l" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeLibraries and excludeLibraries options contain a library `a`.`l`.")
EXPECT_STDOUT_NOT_CONTAINS("`b`.`l`")

dump_with_conflicts({ "includeLibraries": [ "a.l", "a.l1" ], "excludeLibraries": [ "a.l" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeLibraries and excludeLibraries options contain a library `a`.`l`.")
EXPECT_STDOUT_NOT_CONTAINS("`a`.`l1`")

dump_with_conflicts({ "includeLibraries": [ "a.l" ], "excludeLibraries": [ "a.l", "b.l" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeLibraries and excludeLibraries options contain a library `a`.`l`.")
EXPECT_STDOUT_NOT_CONTAINS("`b`.`l`")

dump_with_conflicts({ "includeLibraries": [ "a.l" ], "excludeLibraries": [ "a.l", "a.l1" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeLibraries and excludeLibraries options contain a library `a`.`l`.")
EXPECT_STDOUT_NOT_CONTAINS("`a`.`l1`")

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

dump_with_conflicts({ "includeTriggers": [ "a.t" ], "excludeTriggers": [ "b.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

dump_with_conflicts({ "includeTriggers": [ "a.t.t" ], "excludeTriggers": [ "b.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

dump_with_conflicts({ "includeTriggers": [ "a.t" ], "excludeTriggers": [ "b.t.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

dump_with_conflicts({ "includeTriggers": [ "a.t.t" ], "excludeTriggers": [ "b.t.t" ] }, False)
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

dump_with_conflicts({ "includeTriggers": [ "a.t", "b.t" ], "excludeTriggers": [ "a.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeTriggers and excludeTriggers options contain a filter `a`.`t`.")
EXPECT_STDOUT_NOT_CONTAINS("`b`.`T`")

dump_with_conflicts({ "includeTriggers": [ "a.t", "a.t1" ], "excludeTriggers": [ "a.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeTriggers and excludeTriggers options contain a filter `a`.`t`.")
EXPECT_STDOUT_NOT_CONTAINS("`a`.`t1`")

dump_with_conflicts({ "includeTriggers": [ "a.t" ], "excludeTriggers": [ "a.t", "b.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeTriggers and excludeTriggers options contain a filter `a`.`t`.")
EXPECT_STDOUT_NOT_CONTAINS("`b`.`T`")

dump_with_conflicts({ "includeTriggers": [ "a.t" ], "excludeTriggers": [ "a.t", "a.t1" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeTriggers and excludeTriggers options contain a filter `a`.`t`.")
EXPECT_STDOUT_NOT_CONTAINS("`a`.`t1`")

dump_with_conflicts({ "includeTriggers": [ "a.t.t" ], "excludeTriggers": [ "a.t.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeTriggers and excludeTriggers options contain a trigger `a`.`t`.`t`.")

dump_with_conflicts({ "includeTriggers": [ "a.t.t", "b.t.t" ], "excludeTriggers": [ "a.t.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeTriggers and excludeTriggers options contain a trigger `a`.`t`.`t`.")
EXPECT_STDOUT_NOT_CONTAINS("`b`.`T`")

dump_with_conflicts({ "includeTriggers": [ "a.t.t", "a.t1.t" ], "excludeTriggers": [ "a.t.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeTriggers and excludeTriggers options contain a trigger `a`.`t`.`t`.")
EXPECT_STDOUT_NOT_CONTAINS("`a`.`t1`")

dump_with_conflicts({ "includeTriggers": [ "a.t.t" ], "excludeTriggers": [ "a.t.t", "b.t.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeTriggers and excludeTriggers options contain a trigger `a`.`t`.`t`.")
EXPECT_STDOUT_NOT_CONTAINS("`b`.`T`")

dump_with_conflicts({ "includeTriggers": [ "a.t.t" ], "excludeTriggers": [ "a.t.t", "a.t1.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeTriggers and excludeTriggers options contain a trigger `a`.`t`.`t`.")
EXPECT_STDOUT_NOT_CONTAINS("`a`.`t1`")

dump_with_conflicts({ "includeTriggers": [ "a.t.t" ], "excludeTriggers": [ "a.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: The includeTriggers option contains a trigger `a`.`t`.`t` which is excluded by the value of the excludeTriggers option: `a`.`t`.")

dump_with_conflicts({ "includeTriggers": [ "a.t.t", "b.t.t" ], "excludeTriggers": [ "a.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: The includeTriggers option contains a trigger `a`.`t`.`t` which is excluded by the value of the excludeTriggers option: `a`.`t`.")
EXPECT_STDOUT_NOT_CONTAINS("`b`.`T`")

dump_with_conflicts({ "includeTriggers": [ "a.t.t", "a.t1.t" ], "excludeTriggers": [ "a.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: The includeTriggers option contains a trigger `a`.`t`.`t` which is excluded by the value of the excludeTriggers option: `a`.`t`.")
EXPECT_STDOUT_NOT_CONTAINS("`a`.`t1`")

#@<> BUG#34052980 run upgrade checker if server is 5.7 and ocimds option is used {VER(<8.0.0)}
# setup
wipeout_server(session)
tested_schema = "test_schema"
tested_table = "test_table"
session.run_sql("CREATE SCHEMA !", [ tested_schema ])
session.run_sql("CREATE TABLE !.! (a int primary key, e enum('aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddeeeeee'))", [ tested_schema, tested_table ])
session.run_sql("CREATE TABLE !.not_so_large (a int primary key, e enum('aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbccccccccccccccccccccccccccccccccc','ccccccccccccccccccccccccccccccccccccffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddeeeeee', 'zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz'))", [ tested_schema ])

# dumping the whole schema should fail
EXPECT_FAIL("Error: Shell Error (52004)", "Compatibility issues were found", [ tested_schema ], test_output_absolute, { "ocimds": True, "showProgress": False })
EXPECT_STDOUT_CONTAINS(f"""
7) ENUM/SET column definitions containing elements longer than 255 characters
(enumSetElementLength)
   Error: The following columns are defined as either ENUM or SET and contain
   at least one element longer that 255 characters. They need to be altered so
   that all elements fit into the 255 characters limit.

   {tested_schema}.{tested_table}.e - ENUM contains element longer than 255 characters

   More information:
     https://dev.mysql.com/doc/refman/en/string-type-syntax.html
""")
EXPECT_STDOUT_CONTAINS("ERROR: 1 errors were found. Please correct these issues before upgrading to avoid compatibility issues.")

# dumping without the problematic table should succeed
WIPE_OUTPUT()
EXPECT_SUCCESS([ tested_schema ], test_output_absolute, { "excludeTables": [ f"{tested_schema}.{tested_table}" ], "ocimds": True, "showProgress": False })
EXPECT_STDOUT_CONTAINS("NOTE: No fatal errors were found that would prevent an upgrade, but some potential issues were detected. Please ensure that the reported issues are not significant before upgrading.")

session.run_sql("DROP SCHEMA !;", [tested_schema])

#@<> WL15311 - setup
schema_name = "wl15311"
no_partitions_table_name = "no_partitions"
no_partitions_table_name_quoted = quote_identifier(schema_name, no_partitions_table_name)
partitions_table_name = "partitions"
partitions_table_name_quoted = quote_identifier(schema_name, partitions_table_name)
subpartitions_table_name = "subpartitions"
subpartitions_table_name_quoted = quote_identifier(schema_name, subpartitions_table_name)
all_tables = [ no_partitions_table_name, partitions_table_name, subpartitions_table_name ]
subpartition_prefix = "@o" if __os_type == "windows" else "@ó"

create_all_schemas()

session.run_sql("DROP SCHEMA IF EXISTS !", [schema_name])
session.run_sql("CREATE SCHEMA IF NOT EXISTS !", [schema_name])

session.run_sql("CREATE TABLE !.! (`id` int NOT NULL AUTO_INCREMENT PRIMARY KEY, `data` blob)", [ schema_name, no_partitions_table_name ])

session.run_sql("""CREATE TABLE !.!
(`id` int NOT NULL AUTO_INCREMENT PRIMARY KEY, `data` blob)
PARTITION BY RANGE (`id`)
(PARTITION x0 VALUES LESS THAN (10000),
 PARTITION x1 VALUES LESS THAN (20000),
 PARTITION x2 VALUES LESS THAN (30000),
 PARTITION x3 VALUES LESS THAN MAXVALUE)""", [ schema_name, partitions_table_name ])

session.run_sql(f"""CREATE TABLE !.!
(`id` int, `data` blob)
PARTITION BY RANGE (`id`)
SUBPARTITION BY KEY (id)
SUBPARTITIONS 2
(PARTITION `{subpartition_prefix}0` VALUES LESS THAN (10000),
 PARTITION `{subpartition_prefix}1` VALUES LESS THAN (20000),
 PARTITION `{subpartition_prefix}2` VALUES LESS THAN (30000),
 PARTITION `{subpartition_prefix}3` VALUES LESS THAN MAXVALUE)""", [ schema_name, subpartitions_table_name ])

for x in range(4):
    session.run_sql(f"""INSERT INTO !.! (`data`) VALUES {",".join([f"('{random_string(100,200)}')" for i in range(10000)])}""", [ schema_name, partitions_table_name ])
session.run_sql("INSERT INTO !.! SELECT * FROM !.!", [ schema_name, subpartitions_table_name, schema_name, partitions_table_name ])
session.run_sql("INSERT INTO !.! SELECT * FROM !.!", [ schema_name, no_partitions_table_name, schema_name, partitions_table_name ])

for table in all_tables:
    session.run_sql("ANALYZE TABLE !.!;", [ schema_name, table ])

#@<> WL15311_TSFR_3_1
help_text = """
      - where: dictionary (default: not set) - A key-value pair of a table name
        in the format of schema.table and a valid SQL condition expression used
        to filter the data being exported.
"""
EXPECT_TRUE(help_text in util.help("dump_schemas"))

#@<> WL15311_TSFR_3_1_1, WL15311_TSFR_3_2_1
TEST_MAP_OF_STRINGS_OPTION("where")

#@<> WL15311_TSFR_3_1_1_1
EXPECT_FAIL("ValueError", "Argument #3: The table name key of the 'where' option must be in the following form: schema.table, with optional backtick quotes, wrong value: 'no_dot'.", [ schema_name ], test_output_absolute, {"where": { "no_dot": "1 = 1" }})

#@<> WL15311_TSFR_3_1_1_2
TEST_DUMP_AND_LOAD([ schema_name ], { "where": { no_partitions_table_name_quoted: "id > 12345", subpartitions_table_name_quoted: "" } })
EXPECT_GT(count_rows(schema_name, no_partitions_table_name), count_rows(verification_schema, no_partitions_table_name))
# WL15311_TSFR_3_2_5_1
EXPECT_EQ(count_rows(schema_name, partitions_table_name), count_rows(verification_schema, partitions_table_name))
EXPECT_EQ(count_rows(schema_name, subpartitions_table_name), count_rows(verification_schema, subpartitions_table_name))

TEST_DUMP_AND_LOAD([ schema_name ], { "where": { no_partitions_table_name_quoted: "id > 12345 AND (id < 23456)", partitions_table_name_quoted: "id > 12345" } })
EXPECT_GT(count_rows(schema_name, no_partitions_table_name), count_rows(verification_schema, no_partitions_table_name))
EXPECT_GT(count_rows(schema_name, partitions_table_name), count_rows(verification_schema, partitions_table_name))

#@<> WL15311_TSFR_3_1_2_1
EXPECT_SUCCESS([ schema_name ], test_output_absolute, { "ddlOnly": True, "where": { no_partitions_table_name_quoted: "id > 12345" }, "showProgress": False })

#@<> WL15311_TSFR_3_1_2_2
EXPECT_SUCCESS([ schema_name ], test_output_absolute, { "dryRun": True, "where": { no_partitions_table_name_quoted: "id > 12345" }, "showProgress": False })

#@<> WL15311_TSFR_3_1_2_3
EXPECT_SUCCESS([ schema_name ], test_output_absolute, { "includeTables": [ partitions_table_name_quoted ], "where": { no_partitions_table_name_quoted: "id > 12345" }, "showProgress": False })

#@<> WL15311_TSFR_3_1_2_4
EXPECT_SUCCESS([ schema_name ], test_output_absolute, { "excludeTables": [ no_partitions_table_name_quoted ], "where": { no_partitions_table_name_quoted: "id > 12345" }, "showProgress": False })

#@<> WL15311_TSFR_3_1_2_6
TEST_DUMP_AND_LOAD([ schema_name, types_schema ], { "where": { no_partitions_table_name_quoted: "id > 12345" }, "showProgress": False })

#@<> WL15311_TSFR_3_1_2_10 - schema or table do not exist
TEST_DUMP_AND_LOAD([ schema_name ], { "where": { quote_identifier("schema", "table"): "1 = 2", quote_identifier(schema_name, "table"): "1 = 2" } })

#@<> WL15311_TSFR_3_2_1_1
TEST_DUMP_AND_LOAD([ schema_name ], { "where": { no_partitions_table_name_quoted: "1 = 2", partitions_table_name_quoted: "1 = 1" } })
EXPECT_EQ(0, count_rows(verification_schema, no_partitions_table_name))
EXPECT_EQ(count_rows(schema_name, partitions_table_name), count_rows(verification_schema, partitions_table_name))

#@<> WL15311_TSFR_3_2_3_1
EXPECT_FAIL("Error: Shell Error (52006)", re.compile(r"While '.*': Fatal error during dump"), [ schema_name ], test_output_absolute, { "where": { no_partitions_table_name_quoted: "THIS_IS_NO_SQL" }, "showProgress": False }, True)
EXPECT_STDOUT_CONTAINS("MySQL Error 1054 (42S22): Unknown column 'THIS_IS_NO_SQL' in 'where clause'")

WIPE_STDOUT()
EXPECT_FAIL("Error: Shell Error (52006)", re.compile(r"While '.*': Fatal error during dump"), [ schema_name ], test_output_absolute, { "where": { no_partitions_table_name_quoted: "1 = 1 ; DROP TABLE mysql.user ; SELECT 1 FROM DUAL" }, "showProgress": False }, True)
EXPECT_STDOUT_CONTAINS("MySQL Error 1064 (42000): You have an error in your SQL syntax; check the manual that corresponds to your MySQL server version for the right syntax to use near '; DROP TABLE mysql.user ; SELECT 1 FROM DUAL) ORDER BY")

WIPE_STDOUT()
EXPECT_FAIL("ValueError", f"Argument #3: Malformed condition used for table '{schema_name}'.'{no_partitions_table_name}': 1 = 1) --", [ schema_name ], test_output_absolute, { "where": { no_partitions_table_name_quoted: "1 = 1) --" }, "showProgress": False })

WIPE_STDOUT()
EXPECT_FAIL("ValueError", f"Argument #3: Malformed condition used for table '{schema_name}'.'{no_partitions_table_name}': (1 = 1", [ schema_name ], test_output_absolute, { "where": { no_partitions_table_name_quoted: "(1 = 1" }, "showProgress": False })

#@<> WL15311_TSFR_3_2_4_1
TEST_DUMP_AND_LOAD([ schema_name ], {})
EXPECT_EQ(count_rows(schema_name, no_partitions_table_name), count_rows(verification_schema, no_partitions_table_name))

TEST_DUMP_AND_LOAD([ schema_name ], { "where": {} })
EXPECT_EQ(count_rows(schema_name, no_partitions_table_name), count_rows(verification_schema, no_partitions_table_name))

TEST_DUMP_AND_LOAD([ schema_name ], { "where": { no_partitions_table_name_quoted: "" } })
EXPECT_EQ(count_rows(schema_name, no_partitions_table_name), count_rows(verification_schema, no_partitions_table_name))

#@<> WL15311_TSFR_4_1
help_text = """
      - partitions: dictionary (default: not set) - A key-value pair of a table
        name in the format of schema.table and a list of valid partition names
        used to limit the data export to just the specified partitions.
"""
EXPECT_TRUE(help_text in util.help("dump_schemas"))

#@<> WL15311_TSFR_4_1_1, WL15311_TSFR_4_2_1, WL15311_TSFR_4_2_2
TEST_MAP_OF_ARRAY_OF_STRINGS_OPTION("partitions")

#@<> WL15311_TSFR_4_1_1_1
EXPECT_FAIL("ValueError", "Argument #3: The table name key of the 'partitions' option must be in the following form: schema.table, with optional backtick quotes, wrong value: 'no_dot'.", [ schema_name ], test_output_absolute, {"partitions": { "no_dot": [ "p0" ] }})

#@<> WL15311_TSFR_4_1_1_2
TEST_DUMP_AND_LOAD([ schema_name ], { "where": { partitions_table_name_quoted: "1 = 1" }, "partitions": { partitions_table_name_quoted: [ "x1" ] } })
EXPECT_EQ(count_rows(schema_name, no_partitions_table_name), count_rows(verification_schema, no_partitions_table_name))
EXPECT_GT(count_rows(schema_name, partitions_table_name), count_rows(verification_schema, partitions_table_name))

TEST_DUMP_AND_LOAD([ schema_name ], { "where": { partitions_table_name_quoted: "id > 12345" }, "partitions": { partitions_table_name_quoted: [ "x1" ] } })
EXPECT_EQ(count_rows(schema_name, no_partitions_table_name), count_rows(verification_schema, no_partitions_table_name))
EXPECT_GT(count_rows(schema_name, partitions_table_name), count_rows(verification_schema, partitions_table_name))

TEST_DUMP_AND_LOAD([ schema_name ], { "where": { partitions_table_name_quoted: "id > 12345" }, "partitions": { partitions_table_name_quoted: [ "x1", "x2" ] } })
EXPECT_EQ(count_rows(schema_name, no_partitions_table_name), count_rows(verification_schema, no_partitions_table_name))
EXPECT_GT(count_rows(schema_name, partitions_table_name), count_rows(verification_schema, partitions_table_name))

TEST_DUMP_AND_LOAD([ schema_name ], { "where": { partitions_table_name_quoted: "id > 12345" }, "partitions": { partitions_table_name_quoted: [ "x0" ] } })
EXPECT_EQ(count_rows(schema_name, no_partitions_table_name), count_rows(verification_schema, no_partitions_table_name))
EXPECT_EQ(0, count_rows(verification_schema, partitions_table_name))

TEST_DUMP_AND_LOAD([ schema_name ], { "where": { partitions_table_name_quoted: "id > 12345" }, "partitions": { subpartitions_table_name_quoted: [ f"{subpartition_prefix}1" ] } })
EXPECT_EQ(count_rows(schema_name, no_partitions_table_name), count_rows(verification_schema, no_partitions_table_name))
EXPECT_GT(count_rows(schema_name, partitions_table_name), count_rows(verification_schema, partitions_table_name))
EXPECT_GT(count_rows(schema_name, subpartitions_table_name), count_rows(verification_schema, subpartitions_table_name))

#@<> WL15311_TSFR_4_2_1_1, WL15311_TSFR_4_2_2_1
TEST_DUMP_AND_LOAD([ schema_name ], { "partitions": { subpartitions_table_name_quoted: [ f"{subpartition_prefix}1", f"{subpartition_prefix}2" ] } })
EXPECT_GT(count_rows(schema_name, subpartitions_table_name), count_rows(verification_schema, subpartitions_table_name))

TEST_DUMP_AND_LOAD([ schema_name ], { "partitions": { subpartitions_table_name_quoted: [ f"{subpartition_prefix}1", f"{subpartition_prefix}2sp0" ] } })
EXPECT_GT(count_rows(schema_name, subpartitions_table_name), count_rows(verification_schema, subpartitions_table_name))

TEST_DUMP_AND_LOAD([ schema_name ], { "partitions": { subpartitions_table_name_quoted: [ f"{subpartition_prefix}1sp0", f"{subpartition_prefix}2sp0" ] } })
EXPECT_GT(count_rows(schema_name, subpartitions_table_name), count_rows(verification_schema, subpartitions_table_name))

#@<> WL15311_TSFR_4_1_2_1
EXPECT_SUCCESS([ schema_name ], test_output_absolute, { "ddlOnly": True, "partitions": { partitions_table_name_quoted: [ "x1" ] }, "showProgress": False })

#@<> WL15311_TSFR_4_1_2_2
EXPECT_SUCCESS([ schema_name ], test_output_absolute, { "dryRun": True, "partitions": { partitions_table_name_quoted: [ "x1" ] }, "showProgress": False })

#@<> WL15311_TSFR_4_1_2_3
EXPECT_SUCCESS([ schema_name ], test_output_absolute, { "includeTables": [ subpartitions_table_name_quoted ], "partitions": { partitions_table_name_quoted: [ "x1" ] }, "showProgress": False })

#@<> WL15311_TSFR_4_1_2_4
EXPECT_SUCCESS([ schema_name ], test_output_absolute, { "excludeTables": [ partitions_table_name_quoted ], "partitions": { partitions_table_name_quoted: [ "x1" ] }, "showProgress": False })

#@<> WL15311_TSFR_4_1_2_6
TEST_DUMP_AND_LOAD([ schema_name, types_schema ], { "partitions": { partitions_table_name_quoted: [ "x1" ] }, "showProgress": False })

#@<> WL15311_TSFR_4_1_2_10 - schema or table do not exist
TEST_DUMP_AND_LOAD([ schema_name ], { "partitions": { quote_identifier("schema", "table"): [ "x1" ], quote_identifier(schema_name, "table"): [ "x1" ] } })

#@<> WL15311_TSFR_4_2_3_1
EXPECT_FAIL("ValueError", "Invalid partitions", [ schema_name ], test_output_absolute, { "partitions": { subpartitions_table_name_quoted: [ "SELECT 1" ] }, "showProgress": False })
EXPECT_STDOUT_CONTAINS(f"ERROR: Following partitions were not found in table '{schema_name}'.'{subpartitions_table_name}': 'SELECT 1'")

WIPE_STDOUT()
EXPECT_FAIL("ValueError", "Invalid partitions", [ schema_name ], test_output_absolute, { "partitions": { subpartitions_table_name_quoted: [ f"{subpartition_prefix}9" ] }, "showProgress": False })
EXPECT_STDOUT_CONTAINS(f"ERROR: Following partitions were not found in table '{schema_name}'.'{subpartitions_table_name}': '{subpartition_prefix}9'")

WIPE_STDOUT()
EXPECT_FAIL("ValueError", "Invalid partitions", [ schema_name ], test_output_absolute, { "partitions": { subpartitions_table_name_quoted: [ f"{subpartition_prefix}9", f"{subpartition_prefix}1sp9" ] }, "showProgress": False })
EXPECT_STDOUT_CONTAINS(f"ERROR: Following partitions were not found in table '{schema_name}'.'{subpartitions_table_name}': '{subpartition_prefix}1sp9', '{subpartition_prefix}9'")

WIPE_STDOUT()
EXPECT_FAIL("ValueError", "Invalid partitions", [ schema_name ], test_output_absolute, { "partitions": { partitions_table_name_quoted: [ "x6" ], subpartitions_table_name_quoted: [ f"{subpartition_prefix}9" ] }, "showProgress": False })
EXPECT_STDOUT_CONTAINS(f"ERROR: Following partitions were not found in table '{schema_name}'.'{subpartitions_table_name}': '{subpartition_prefix}9'")
EXPECT_STDOUT_CONTAINS(f"ERROR: Following partitions were not found in table '{schema_name}'.'{partitions_table_name}': 'x6'")

#@<> WL15311_TSFR_4_2_4_1, WL15311_TSFR_4_2_5_1
TEST_DUMP_AND_LOAD([ schema_name ], {})
EXPECT_EQ(count_rows(schema_name, subpartitions_table_name), count_rows(verification_schema, subpartitions_table_name))

TEST_DUMP_AND_LOAD([ schema_name ], { "partitions": {} })
EXPECT_EQ(count_rows(schema_name, subpartitions_table_name), count_rows(verification_schema, subpartitions_table_name))

TEST_DUMP_AND_LOAD([ schema_name ], { "partitions": { subpartitions_table_name_quoted: [] } })
EXPECT_EQ(count_rows(schema_name, subpartitions_table_name), count_rows(verification_schema, subpartitions_table_name))

TEST_DUMP_AND_LOAD([ schema_name ], { "partitions": { partitions_table_name_quoted: [ "x1" ], subpartitions_table_name_quoted: [] } })
EXPECT_GT(count_rows(schema_name, partitions_table_name), count_rows(verification_schema, partitions_table_name))
EXPECT_EQ(count_rows(schema_name, subpartitions_table_name), count_rows(verification_schema, subpartitions_table_name))

#@<> WL15311 - cleanup
session.run_sql("DROP SCHEMA !;", [schema_name])

#@<> WL15887 - setup
schema_name = "wl15887"

def setup_db(account):
    session.run_sql("DROP SCHEMA IF EXISTS !", [schema_name])
    session.run_sql("CREATE SCHEMA !", [schema_name])
    session.run_sql("CREATE TABLE !.! (`id` MEDIUMINT NOT NULL AUTO_INCREMENT PRIMARY KEY) ENGINE=InnoDB;", [ schema_name, test_table_primary ])
    session.run_sql(f"CREATE DEFINER={account} EVENT !.! ON SCHEDULE EVERY 1 HOUR DO BEGIN END;", [ schema_name, test_schema_event ])
    session.run_sql(f"CREATE DEFINER={account} FUNCTION !.!(s CHAR(20)) RETURNS CHAR(50) DETERMINISTIC RETURN CONCAT('Hello, ',s,'!');", [ schema_name, test_schema_function ])
    session.run_sql(f"CREATE DEFINER={account} PROCEDURE !.!() SQL SECURITY DEFINER BEGIN END", [ schema_name, test_schema_procedure ])
    session.run_sql(f"CREATE DEFINER={account} TRIGGER !.! BEFORE UPDATE ON ! FOR EACH ROW BEGIN END;", [ schema_name, test_table_trigger, test_table_primary ])
    session.run_sql(f"CREATE DEFINER={account} SQL SECURITY INVOKER VIEW !.! AS SELECT * FROM !.!;", [ schema_name, test_view, schema_name, test_table_primary ])

create_all_schemas()
setup_db(test_user_account)

#@<> WL15887-TSFR_1_1
help_text = """
      - targetVersion: string (default: current version of Shell) - Specifies
        version of the destination MySQL server.
"""
EXPECT_TRUE(help_text in util.help("dump_schemas"))

#@<> WL15887-TSFR_1_1_2 - option type
TEST_STRING_OPTION("targetVersion")

#@<> WL15887-TSFR_1_1_1 - invalid values
EXPECT_FAIL("ValueError", "Argument #3: Invalid value of the 'targetVersion' option: '8.1.', Error parsing version", [ schema_name ], test_output_absolute, { "targetVersion": "8.1.", "showProgress": False })
EXPECT_FAIL("ValueError", "Argument #3: Invalid value of the 'targetVersion' option: 'abc', Error parsing version", [ schema_name ], test_output_absolute, { "targetVersion": "abc", "showProgress": False })
EXPECT_FAIL("ValueError", "Argument #3: Invalid value of the 'targetVersion' option: empty", [ schema_name ], test_output_absolute, { "targetVersion": "", "showProgress": False })

#@<> WL15887-TSFR_1_2_1 - wrong values - greater
for i in range(3):
    version = __mysh_version.split(".")
    version[i] = str(int(version[i]) + 1)
    version = ".".join(version)
    if i < 2:
        EXPECT_FAIL("ValueError", f"Argument #3: Target MySQL version '{version}' is newer than the maximum version '{'.'.join(__mysh_version.split('.')[:2])}.*' supported by this version of MySQL Shell", [ schema_name ], test_output_absolute, { "targetVersion": version, "showProgress": False })
    else:
        # BUG#38107377 - patch version is not checked
        EXPECT_SUCCESS([ schema_name ], test_output_absolute, { "targetVersion": version, "dryRun": True, "showProgress": False })

#@<> WL15887-TSFR_1_3_1 - wrong values - lower
EXPECT_FAIL("ValueError", "Argument #3: Target MySQL version '8.0.24' is older than the minimum version '8.0.25' supported by this version of MySQL Shell", [ schema_name ], test_output_absolute, { "targetVersion": "8.0.24", "showProgress": False })
EXPECT_FAIL("ValueError", "Argument #3: Target MySQL version '7.9.26' is older than the minimum version '8.0.25' supported by this version of MySQL Shell", [ schema_name ], test_output_absolute, { "targetVersion": "7.9.26", "showProgress": False })

#@<> WL15887 - valid values
EXPECT_SUCCESS([ schema_name ], test_output_absolute, { "targetVersion": "8.0.25", "dryRun": True, "showProgress": False })
EXPECT_SUCCESS([ schema_name ], test_output_absolute, { "targetVersion": __mysh_version, "dryRun": True, "showProgress": False })

#@<> WL15887-TSFR_1_4_1 - implict value of targetVersion
EXPECT_SUCCESS([ schema_name ], test_output_relative, { "ocimds": True, "dryRun": True, "showProgress": False })
EXPECT_STDOUT_CONTAINS(f"Checking for compatibility with MySQL HeatWave Service {__mysh_version}")

# WL15887-TSFR_2_1 - implict value of targetVersion, warnings
EXPECT_STDOUT_CONTAINS(strip_definers_definer_clause(schema_name, test_schema_event, "Event", test_user_account).warning())
EXPECT_STDOUT_CONTAINS(strip_definers_definer_clause(schema_name, test_schema_function, "Function", test_user_account).warning())
EXPECT_STDOUT_CONTAINS(strip_definers_definer_clause(schema_name, test_schema_procedure, "Procedure", test_user_account).warning())
EXPECT_STDOUT_CONTAINS(strip_definers_definer_clause(schema_name, test_table_trigger, "Trigger", test_user_account).warning())
EXPECT_STDOUT_CONTAINS(strip_definers_definer_clause(schema_name, test_view, "View", test_user_account).warning())

EXPECT_STDOUT_CONTAINS(strip_definers_security_clause(schema_name, test_schema_function, "Function").warning())
EXPECT_STDOUT_CONTAINS(strip_definers_security_clause(schema_name, test_schema_procedure, "Procedure").warning())
EXPECT_STDOUT_NOT_CONTAINS(strip_definers_security_clause(schema_name, test_view, "View").warning())

EXPECT_STDOUT_CONTAINS(f"""
NOTE: One or more objects with the DEFINER clause were found.

      The 'targetVersion' option was not set and compatibility was checked with the MySQL HeatWave Service {__mysh_version}.
      Loading the dump will fail if it is loaded into an DB System instance that does not support the SET_ANY_DEFINER privilege, which was introduced in 8.2.0.
""")

#@<> WL15887-TSFR_3_1_1 - restricted accounts
for account in ["mysql.infoschema", "mysql.session", "mysql.sys", "ociadmin", "ocidbm", "ocirpl"]:
    account = f"`{account}`@`localhost`"
    setup_db(account)
    WIPE_OUTPUT()
    EXPECT_FAIL("Error: Shell Error (52004)", "Compatibility issues were found", [ schema_name ], test_output_relative, { "targetVersion": __mysh_version, "ocimds": True, "dryRun": True, "showProgress": False })
    EXPECT_STDOUT_CONTAINS(definer_clause_uses_restricted_user_name(schema_name, test_schema_event, "Event", account).error())
    EXPECT_STDOUT_CONTAINS(definer_clause_uses_restricted_user_name(schema_name, test_schema_function, "Function", account).error())
    EXPECT_STDOUT_CONTAINS(definer_clause_uses_restricted_user_name(schema_name, test_schema_procedure, "Procedure", account).error())
    EXPECT_STDOUT_CONTAINS(definer_clause_uses_restricted_user_name(schema_name, test_table_trigger, "Trigger", account).error())
    EXPECT_STDOUT_CONTAINS(definer_clause_uses_restricted_user_name(schema_name, test_view, "View", account).error())

# restore schema
setup_db(test_user_account)

#@<> WL15887-TSFR_3_2_1 - valid account
EXPECT_SUCCESS([ schema_name ], test_output_relative, { "targetVersion": __mysh_version, "ocimds": True, "dryRun": True, "showProgress": False })

# no warnings about DEFINER=
EXPECT_STDOUT_NOT_CONTAINS(strip_definers_definer_clause(schema_name, test_schema_event, "Event", test_user_account).warning())
EXPECT_STDOUT_NOT_CONTAINS(strip_definers_definer_clause(schema_name, test_schema_function, "Function", test_user_account).warning())
EXPECT_STDOUT_NOT_CONTAINS(strip_definers_definer_clause(schema_name, test_schema_procedure, "Procedure", test_user_account).warning())
EXPECT_STDOUT_NOT_CONTAINS(strip_definers_definer_clause(schema_name, test_table_trigger, "Trigger", test_user_account).warning())
EXPECT_STDOUT_NOT_CONTAINS(strip_definers_definer_clause(schema_name, test_view, "View", test_user_account).warning())

# WL15887-TSFR_3_5_1 - no warnings about SQL SECURITY
EXPECT_STDOUT_NOT_CONTAINS(strip_definers_security_clause(schema_name, test_schema_function, "Function").warning())
EXPECT_STDOUT_NOT_CONTAINS(strip_definers_security_clause(schema_name, test_schema_procedure, "Procedure").warning())
EXPECT_STDOUT_NOT_CONTAINS(strip_definers_security_clause(schema_name, test_view, "View").warning())

# WL15887-TSFR_3_3_1 - no account is not included in the dump
EXPECT_STDOUT_CONTAINS(definer_clause_uses_unknown_account_once().warning())

#@<> WL15887-TSFR_4_1 - note about strip_definers
EXPECT_SUCCESS([ schema_name ], test_output_relative, { "compatibility": [ "strip_definers" ], "targetVersion": __mysh_version, "ocimds": True, "dryRun": True, "showProgress": False })
EXPECT_STDOUT_CONTAINS(f"NOTE: The 'targetVersion' option is set to {__mysh_version}. This version supports the SET_ANY_DEFINER privilege, using the 'strip_definers' compatibility option is unnecessary.")

#@<> WL15887 - cleanup
session.run_sql("DROP SCHEMA IF EXISTS !;", [schema_name])

#@<> BUG#35415976 - invalid views were not detected
# setup
schema_name = "test_35415976"
session.run_sql("DROP SCHEMA IF EXISTS !", [schema_name])
session.run_sql("CREATE SCHEMA !", [schema_name])
session.run_sql("CREATE TABLE !.t1 (a int)", [schema_name])
session.run_sql("INSERT INTO !.t1 (a) VALUES (1), (2), (3)", [schema_name])
session.run_sql("CREATE VIEW !.v3 AS SELECT max(a) FROM !.t1", [schema_name, schema_name])
session.run_sql("CREATE VIEW !.v1 AS SELECT * FROM !.v3", [schema_name, schema_name])
session.run_sql("CREATE VIEW !.v2 AS SELECT * FROM !.v3", [schema_name, schema_name])
session.run_sql("DROP VIEW !.v3", [schema_name])

#@<> BUG#35415976 - test
EXPECT_FAIL("Error: Shell Error (52039)", "While 'Gathering information': Dump contains one or more invalid views. Fix them manually, or use the 'excludeTables' option to exclude them.", [ schema_name ], test_output_absolute, { "showProgress": False })
EXPECT_STDOUT_CONTAINS(f"View '{schema_name}.v1' references invalid table(s) or column(s) or function(s) or definer/invoker of view lack rights to use them")
EXPECT_STDOUT_CONTAINS(f"View '{schema_name}.v2' references invalid table(s) or column(s) or function(s) or definer/invoker of view lack rights to use them")

# if view is excluded, dump succeeds
EXPECT_SUCCESS([ schema_name ], test_output_absolute, { "excludeTables": [f"{schema_name}.v1", f"{schema_name}.v2"], "showProgress": False })

#@<> BUG#35415976 - cleanup
session.run_sql("DROP SCHEMA !", [schema_name])

#@<> WL15947 - setup
schema_name = "wl15947"
test_table_unique_null = test_table_non_unique
test_table_partitioned = "part"
test_table_gipk = "gipk"
gipk_supported = __version_num >= 80030

def setup_db():
    session.run_sql("DROP SCHEMA IF EXISTS !", [schema_name])
    session.run_sql("CREATE SCHEMA !", [schema_name])
    session.run_sql("CREATE TABLE !.! (`id` MEDIUMINT NOT NULL PRIMARY KEY) ENGINE=InnoDB;", [ schema_name, test_table_primary ])
    session.run_sql("CREATE TABLE !.! (`id` MEDIUMINT NOT NULL UNIQUE KEY) ENGINE=InnoDB;", [ schema_name, test_table_unique ])
    session.run_sql("CREATE TABLE !.! (`id` MEDIUMINT UNIQUE KEY) ENGINE=InnoDB;", [ schema_name, test_table_unique_null ])
    session.run_sql("CREATE TABLE !.! (`id` MEDIUMINT) ENGINE=InnoDB;", [ schema_name, test_table_no_index ])
    session.run_sql("CREATE TABLE !.! (`id` MEDIUMINT NOT NULL PRIMARY KEY) ENGINE=InnoDB PARTITION BY RANGE (`id`) (PARTITION p0 VALUES LESS THAN (500), PARTITION p1 VALUES LESS THAN (1000), PARTITION p2 VALUES LESS THAN MAXVALUE);", [ schema_name, test_table_partitioned ])
    session.run_sql(f"INSERT INTO !.! (`id`) VALUES {','.join(f'({i})' for i in range(1500))}", [ schema_name, test_table_primary ])
    session.run_sql("INSERT INTO !.! SELECT * FROM !.!;", [ schema_name, test_table_unique, schema_name, test_table_primary ])
    session.run_sql("INSERT INTO !.! SELECT * FROM !.!;", [ schema_name, test_table_unique_null, schema_name, test_table_primary ])
    session.run_sql("INSERT INTO !.! SELECT * FROM !.!;", [ schema_name, test_table_no_index, schema_name, test_table_primary ])
    session.run_sql("INSERT INTO !.! SELECT * FROM !.!;", [ schema_name, test_table_partitioned, schema_name, test_table_primary ])
    session.run_sql("ANALYZE TABLE !.!;", [ schema_name, test_table_primary ])
    session.run_sql("ANALYZE TABLE !.!;", [ schema_name, test_table_unique ])
    session.run_sql("ANALYZE TABLE !.!;", [ schema_name, test_table_unique_null ])
    session.run_sql("ANALYZE TABLE !.!;", [ schema_name, test_table_no_index ])
    session.run_sql("ANALYZE TABLE !.!;", [ schema_name, test_table_partitioned ])
    if gipk_supported:
        session.run_sql("SET @@SESSION.sql_generate_invisible_primary_key=ON")
        session.run_sql("CREATE TABLE !.! (`id` MEDIUMINT) ENGINE=InnoDB;", [ schema_name, test_table_gipk ])
        session.run_sql("INSERT INTO !.! SELECT * FROM !.!;", [ schema_name, test_table_gipk, schema_name, test_table_primary ])
        session.run_sql("ANALYZE TABLE !.!;", [ schema_name, test_table_gipk ])
        session.run_sql("SET @@SESSION.sql_generate_invisible_primary_key=OFF")

create_all_schemas()
setup_db()

checksum_file = checksum_file_path(test_output_absolute)

#@<> WL15947-TSFR_1_1 - help text
help_text = """
      - checksum: bool (default: false) - Compute and include checksum of the
        dumped data.
"""
EXPECT_TRUE(help_text in util.help("dump_schemas"))

#@<> WL15947-TSFR_1_1_1 - no checksum option
EXPECT_SUCCESS([ schema_name ], test_output_absolute, { "showProgress": False })
EXPECT_STDOUT_NOT_CONTAINS("Checksum")
EXPECT_FALSE(os.path.isfile(checksum_file))

#@<> WL15947-TSFR_1_1_1 - checksum option set to false
EXPECT_SUCCESS([ schema_name ], test_output_absolute, { "checksum": False, "showProgress": False })
EXPECT_STDOUT_NOT_CONTAINS("Checksum")
EXPECT_FALSE(os.path.isfile(checksum_file))

#@<> WL15947-TSFR_1_1_2 - option type
TEST_BOOL_OPTION("checksum")

#@<> WL15947-TSFR_1_2_1_1 - checksum option set to true
EXPECT_SUCCESS([ schema_name ], test_output_absolute, { "checksum": True, "showProgress": False })
# checksums are generated
EXPECT_STDOUT_CONTAINS("Checksum")
# file is written
EXPECT_TRUE(os.path.isfile(checksum_file))

if __os_type != "windows":
    # permissions are the same as in case of other files
    actual_file = os.path.join(test_output_absolute, "@.done.json")
    EXPECT_EQ(stat.S_IMODE(os.stat(actual_file).st_mode), stat.S_IMODE(os.stat(checksum_file).st_mode))
    EXPECT_EQ(os.stat(actual_file).st_uid, os.stat(checksum_file).st_uid)

# checksum file is a valid json
EXPECT_NO_THROWS(lambda: read_json(checksum_file), "checksum file should be a valid json")

#@<> WL15947-TSFR_1_3_1 - "checksum": True, "ddlOnly": False, "chunking": True, not partitioned table
EXPECT_SUCCESS([ schema_name ], test_output_absolute, { "checksum": True, "ddlOnly": False, "chunking": True, "includeTables": [ quote_identifier(schema_name, test_table_primary) ], "showProgress": False })
checksums = read_json(checksum_file)
# table is not partitioned - partition name is empty, chunking is enabled - valid chunk ID is used
EXPECT_TRUE("0" in checksums["data"][schema_name][test_table_primary]["partitions"][""])

#@<> WL15947-TSFR_1_3_2 - "checksum": True, "ddlOnly": False, "chunking": True, partitioned table
EXPECT_SUCCESS([ schema_name ], test_output_absolute, { "checksum": True, "ddlOnly": False, "chunking": True, "includeTables": [ quote_identifier(schema_name, test_table_partitioned) ], "showProgress": False })
checksums = read_json(checksum_file)
# table is partitioned - partition name is used, chunking is enabled - valid chunk ID is used
EXPECT_TRUE("0" in checksums["data"][schema_name][test_table_partitioned]["partitions"]["p0"])

#@<> WL15947-TSFR_1_4_1 - "checksum": True, "ddlOnly": False, "chunking": False, not partitioned table
EXPECT_SUCCESS([ schema_name ], test_output_absolute, { "checksum": True, "ddlOnly": False, "chunking": False, "includeTables": [ quote_identifier(schema_name, test_table_primary) ], "showProgress": False })
checksums = read_json(checksum_file)
# table is not partitioned - partition name is empty, chunking is disabled - chunk ID is not used
EXPECT_TRUE("-1" in checksums["data"][schema_name][test_table_primary]["partitions"][""])

#@<> WL15947-TSFR_1_4_2 - "checksum": True, "ddlOnly": False, "chunking": False, partitioned table
EXPECT_SUCCESS([ schema_name ], test_output_absolute, { "checksum": True, "ddlOnly": False, "chunking": False, "includeTables": [ quote_identifier(schema_name, test_table_partitioned) ], "showProgress": False })
checksums = read_json(checksum_file)
# table is partitioned - partition name is used, chunking is disabled - chunk ID is no used
EXPECT_TRUE("-1" in checksums["data"][schema_name][test_table_partitioned]["partitions"]["p0"])

#@<> WL15947-TSFR_1_5_1 - "checksum": True, "ddlOnly": True, not partitioned table
for chunking in [ True, False ]:
    EXPECT_SUCCESS([ schema_name ], test_output_absolute, { "checksum": True, "ddlOnly": True, "chunking": chunking, "includeTables": [ quote_identifier(schema_name, test_table_primary) ], "showProgress": False })
    checksums = read_json(checksum_file)
    # table is not partitioned - partition name is empty, data is not dumped - chunk ID is not used
    EXPECT_TRUE("-1" in checksums["data"][schema_name][test_table_primary]["partitions"][""])

#@<> WL15947-TSFR_1_5_2 - "checksum": True, "ddlOnly": True, partitioned table
for chunking in [ True, False ]:
    EXPECT_SUCCESS([ schema_name ], test_output_absolute, { "checksum": True, "ddlOnly": True, "chunking": chunking, "includeTables": [ quote_identifier(schema_name, test_table_partitioned) ], "showProgress": False })
    checksums = read_json(checksum_file)
    # table is partitioned - partition name is used, data is not dumped - chunk ID is no used
    EXPECT_TRUE("-1" in checksums["data"][schema_name][test_table_partitioned]["partitions"]["p0"])

#@<> WL15947-TSFR_1_6_1 - "checksum": True, table without an index
EXPECT_SUCCESS([ schema_name ], test_output_absolute, { "checksum": True, "includeTables": [ quote_identifier(schema_name, test_table_no_index) ], "showProgress": False })

checksums = read_json(checksum_file)
# checksum information present
EXPECT_TRUE(test_table_no_index in checksums["data"][schema_name])

#@<> WL15947-TSFR_1_6_2 - "checksum": True, table with a non-NULL unique index
EXPECT_SUCCESS([ schema_name ], test_output_absolute, { "checksum": True, "includeTables": [ quote_identifier(schema_name, test_table_unique) ], "showProgress": False })

checksums = read_json(checksum_file)
# checksum information present
EXPECT_TRUE(test_table_unique in checksums["data"][schema_name])

#@<> WL15947-TSFR_1_6_3 - "checksum": True, GIPK {gipk_supported}
session.run_sql("SET @@GLOBAL.show_gipk_in_create_table_and_information_schema = OFF")
EXPECT_SUCCESS([ schema_name ], test_output_absolute, { "checksum": True, "includeTables": [ quote_identifier(schema_name, test_table_gipk) ], "showProgress": False })

checksums = read_json(checksum_file)
# checksum information present
EXPECT_TRUE(test_table_gipk in checksums["data"][schema_name])

#@<> WL15947-TSFR_1_6_4 - "checksum": True, GIPK {gipk_supported}
session.run_sql("SET @@GLOBAL.show_gipk_in_create_table_and_information_schema = ON")
EXPECT_SUCCESS([ schema_name ], test_output_absolute, { "checksum": True, "includeTables": [ quote_identifier(schema_name, test_table_gipk) ], "showProgress": False })

checksums = read_json(checksum_file)
# checksum information present
EXPECT_TRUE(test_table_gipk in checksums["data"][schema_name])

#@<> WL15947-TSFR_1_6_5 - "checksum": True, table without an index, with ignore_missing_pks
EXPECT_SUCCESS([ schema_name ], test_output_absolute, { "checksum": True, "compatibility": [ "ignore_missing_pks" ], "includeTables": [ quote_identifier(schema_name, test_table_no_index) ], "showProgress": False })

checksums = read_json(checksum_file)
# checksum information present
EXPECT_TRUE(test_table_no_index in checksums["data"][schema_name])

#@<> WL15947-TSFR_1_6_6 - "checksum": True, table with a NULL unique index
EXPECT_SUCCESS([ schema_name ], test_output_absolute, { "checksum": True, "includeTables": [ quote_identifier(schema_name, test_table_unique_null) ], "showProgress": False })

checksums = read_json(checksum_file)
# checksum information present
EXPECT_TRUE(test_table_unique_null in checksums["data"][schema_name])

#@<> WL15947 - dry run
EXPECT_SUCCESS([ schema_name ], test_output_absolute, { "dryRun": True, "checksum": True, "includeTables": [ quote_identifier(schema_name, test_table_unique_null) ], "showProgress": False })
EXPECT_STDOUT_CONTAINS("Checksumming enabled.")

#@<> WL15947 - cleanup
session.run_sql("DROP SCHEMA IF EXISTS !;", [schema_name])

#@<> BUG#36509026 - warn about views which reference unknown tables
# setup
schema_name = "test_36509026"
session.run_sql("DROP SCHEMA IF EXISTS !", [schema_name])
session.run_sql("CREATE SCHEMA !", [schema_name])

session.run_sql("CREATE TABLE !.t1 (a int)", [schema_name])
session.run_sql("INSERT INTO !.t1 (a) VALUES (1), (2), (3)", [schema_name])
session.run_sql("CREATE VIEW !.v1 AS SELECT * FROM !.t1", [schema_name, schema_name])

session.run_sql("CREATE TABLE !.t2 (a int)", [schema_name])
session.run_sql("INSERT INTO !.t2 (a) VALUES (1), (2), (3)", [schema_name])
session.run_sql("CREATE VIEW !.v2 AS SELECT * FROM !.t2", [schema_name, schema_name])

lower_case_table_names = session.run_sql("SELECT @@lower_case_table_names").fetch_one()[0]

if 2 == lower_case_table_names:
    # MacOS: Table and database names are stored on disk using the lettercase
    # specified in the CREATE TABLE or CREATE DATABASE statement, but MySQL
    # converts them to lowercase on lookup. Name comparisons are not
    # case-sensitive.
    session.run_sql("CREATE TABLE !.T3 (a int)", [schema_name])
    session.run_sql("INSERT INTO !.T3 (a) VALUES (1), (2), (3)", [schema_name])
    session.run_sql("CREATE VIEW !.v3 AS SELECT * FROM !.t3", [schema_name, schema_name])

#@<> BUG#36509026 - test
EXPECT_SUCCESS([ schema_name ], test_output_absolute, { "excludeTables": [f"{schema_name}.t2"], "showProgress": False })

EXPECT_STDOUT_NOT_CONTAINS(view_references_excluded_table(schema_name, "v1", schema_name, "t1").warning())
EXPECT_STDOUT_CONTAINS(view_references_excluded_table(schema_name, "v2", schema_name, "t2").warning())

if 2 == lower_case_table_names:
    EXPECT_STDOUT_CONTAINS(view_references_invalid_table(schema_name, "v3", schema_name, "t3").warning())
    EXPECT_STDOUT_CONTAINS("""
NOTE: One or more views that reference tables using the wrong case were found.
Loading them in a system that uses lower_case_table_names=0 (such as in the MySQL HeatWave Service) will fail unless they are fixed.
""")

#@<> BUG#36509026 - test with ocimds:true on MacOS {2 == lower_case_table_names}
EXPECT_FAIL("Shell Error (52004)", "Compatibility issues were found", [ schema_name ], test_output_absolute, { "ocimds": True, "excludeTables": [f"{schema_name}.t2"], "showProgress": False })

EXPECT_STDOUT_NOT_CONTAINS(view_references_excluded_table(schema_name, "v1", schema_name, "t1").warning())
EXPECT_STDOUT_CONTAINS(view_references_excluded_table(schema_name, "v2", schema_name, "t2").warning())

EXPECT_STDOUT_CONTAINS(view_references_invalid_table(schema_name, "v3", schema_name, "t3").error())
EXPECT_STDOUT_CONTAINS("""
ERROR: One or more views that reference tables using the wrong case were found.
Loading them in a system that uses lower_case_table_names=0 (such as in the MySQL HeatWave Service) will fail unless they are fixed.
""")

#@<> BUG#36509026 - cleanup
session.run_sql("DROP SCHEMA IF EXISTS !;", [schema_name])

#@<> BUG#37892879 - error out when dumping with `create_invisible_pks` compatibility option and partitioned table doesn't have a primary key
schema_name = "test_37892879"
session.run_sql("DROP SCHEMA IF EXISTS !", [schema_name])
session.run_sql("CREATE SCHEMA !", [schema_name])

session.run_sql("""
CREATE TABLE !.t1 (
    col1 INT NOT NULL,
    col2 DATE NOT NULL,
    col3 INT NOT NULL,
    col4 INT NOT NULL,
    UNIQUE KEY (col1)
)
PARTITION BY HASH(col1)
PARTITIONS 4;
""", [schema_name])

#@<> BUG#37892879 - test with ocimds:true - table is reported as invalid, as it does not contain a PK
EXPECT_FAIL("Shell Error (52004)", "Compatibility issues were found", [ schema_name ], test_output_absolute, { "ocimds": True, "showProgress": False })
EXPECT_STDOUT_CONTAINS(create_invisible_pks(schema_name, "t1").error())

#@<> BUG#37892879 - test `create_invisible_pks` compatibility option - table is reported as invalid, as it's partitioned and does not contain a PK
EXPECT_FAIL("Shell Error (52004)", "Compatibility issues were found", [ schema_name ], test_output_absolute, { "compatibility": [ "create_invisible_pks" ], "ocimds": True, "showProgress": False })
EXPECT_STDOUT_CONTAINS(create_invisible_pks_partitioned_table(schema_name, "t1").error())

#@<> BUG#37892879 - cleanup
session.run_sql("DROP SCHEMA IF EXISTS !;", [schema_name])

#@<> BUG#37904121 - disable off-loading to HeatWave
# setup
schema_name = "test_37904121"
table_name = "t1"
heatwave_table_name = "t2"
supports_secondary_engine = __version_num >= 80013

session.run_sql("DROP SCHEMA IF EXISTS !", [schema_name])
session.run_sql("CREATE SCHEMA !", [schema_name])

session.run_sql("CREATE TABLE !.! (a int primary key)", [schema_name, table_name])

if supports_secondary_engine:
    session.run_sql("CREATE TABLE !.! (a int primary key) SECONDARY_engine='raPID'", [schema_name, heatwave_table_name])

old_log_sql = shell.options["logSql"]
shell.options["logSql"] = "unfiltered"

#@<> BUG#37904121 - test
WIPE_SHELL_LOG()
EXPECT_SUCCESS([ schema_name ], test_output_absolute, { "showProgress": False })
EXPECT_SHELL_LOG_CONTAINS(f"""SELECT {"SQL_NO_CACHE " if __version_num < 80000 else ""}`a` FROM `{schema_name}`.`{table_name}` ORDER BY `a` LIMIT 1""")

if supports_secondary_engine:
    EXPECT_SHELL_LOG_CONTAINS(f"""SELECT /*+ SET_VAR(use_secondary_engine='OFF') {"SET_VAR(optimizer_switch='hypergraph_optimizer=OFF') " if __version_num >= 80400 and __version_num < 90000 else ""}*/ `a` FROM `{schema_name}`.`{heatwave_table_name}` ORDER BY `a` LIMIT 1""")

#@<> BUG#37904121 - cleanup
shell.options["logSql"] = old_log_sql

session.run_sql("DROP SCHEMA IF EXISTS !;", [schema_name])

#@<> Cleanup
drop_all_schemas()
session.run_sql("SET GLOBAL local_infile = false;")
session.close()
testutil.destroy_sandbox(__mysql_sandbox_port1)
shutil.rmtree(incompatible_table_directory, True)
