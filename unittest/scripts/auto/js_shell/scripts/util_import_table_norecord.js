// This is unit test file for WL12193 Parallel data import
const target_port = __mysql_port
const target_xport = __port;
const target_schema = 'wl12193';
const uri = "mysql://" + __mysqluripwd;
const xuri = "mysqlx://" + __uripwd;

var filename_for_output = __os_type == "windows" ? function (filename) { return "\\\\?\\" + filename.replace(/\//g, "\\"); } : function (filename) { return filename; };

function compute_crc(schema, table, columns) {
    session.runSql("SET @crc = '';");
    var columns_placeholder = Array.from({length: columns.length}, (k, v) => '!').join(",");
    session.runSql("SELECT @crc := MD5(CONCAT_WS('#',@crc,"+columns_placeholder+")) FROM !.! ORDER BY !;", columns.concat(schema, table, columns[0]));
    return session.runSql("SELECT @crc;").fetchOne()[0];
}

//@<> Throw if session is empty
EXPECT_THROWS(function () {
    util.importTable(__import_data_path + '/world_x_cities.dump', { table: 'cities' });
}, "A classic protocol session is required to perform this operation.");


testutil.waitSandboxAlive(xuri);

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
EXPECT_STDOUT_CONTAINS("File '" + filename_for_output(__import_data_path + "/world_x_cities.dump") + "' (209.75 KB) was imported in ");
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".cities: Records: 4079  Deleted: 0  Skipped: 0  Warnings: 0");
session.runSql('TRUNCATE TABLE ' + target_schema + '.cities');

///@<> TSF3 - Match user specified threads number with performance_schema
/// Not aplicable

//@<> TSF8_1: Using util.importTable, validate that a file that meets the default value for field
/// and line terminators can be imported in parallel without specifying field and line terminators.
/// LOAD DATA with default dialect
util.importTable(__import_data_path + '/world_x_cities.dump', { schema: target_schema, table: 'cities' });
EXPECT_STDOUT_CONTAINS("File '" + filename_for_output(__import_data_path + "/world_x_cities.dump") + "' (209.75 KB) was imported in ");
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".cities: Records: 4079  Deleted: 0  Skipped: 0  Warnings: 0");

//@<> Ignore on duplicate primary key
util.importTable(__import_data_path + '/world_x_cities.dump', { schema: target_schema, table: 'cities' });
EXPECT_STDOUT_CONTAINS("File '" + filename_for_output(__import_data_path + "/world_x_cities.dump") + "' (209.75 KB) was imported in ");
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".cities: Records: 4079  Deleted: 0  Skipped: 4079  Warnings: 4079");


//@<OUT> Show world_x.cities table
original_output_format = shell.options.resultFormat
shell.options.resultFormat = 'table'
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

var keyname = testutil.versionCheck(__version, '<', '8.0.19') ? `'PRIMARY'` : `'cities.PRIMARY'`;

EXPECT_STDOUT_CONTAINS("WARNING: world_x_cities.csv error 1062: Duplicate entry '1' for key " + keyname);
EXPECT_STDOUT_CONTAINS("WARNING: world_x_cities.csv error 1062: Duplicate entry '2' for key " + keyname);
EXPECT_STDOUT_CONTAINS("WARNING: world_x_cities.csv error 1062: Duplicate entry '3' for key " + keyname);
EXPECT_STDOUT_CONTAINS("WARNING: world_x_cities.csv error 1062: Duplicate entry '4' for key " + keyname);
EXPECT_STDOUT_CONTAINS("WARNING: world_x_cities.csv error 1062: Duplicate entry '5' for key " + keyname);
EXPECT_STDOUT_CONTAINS("File '" + filename_for_output(__import_data_path + "/world_x_cities.csv") + "' (250.53 KB) was imported in ");
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".cities: Records: 4079  Deleted: 0  Skipped: 4079  Warnings: 4079");

//@<> TSF6_5 with skip header row
util.importTable(__import_data_path + '/world_x_cities_header.csv', {
    schema: target_schema, table: 'cities',
    fieldsTerminatedBy: ',', fieldsEnclosedBy: '"', fieldsOptionallyEnclosed: true, linesTerminatedBy: '\n', skipRows: 1
});

EXPECT_STDOUT_CONTAINS("wl12193.cities: Records: 4079  Deleted: 0  Skipped: 4079  Warnings: 4079");
EXPECT_STDOUT_CONTAINS("WARNING: world_x_cities_header.csv error 1062: Duplicate entry '1' for key " + keyname);
EXPECT_STDOUT_CONTAINS("WARNING: world_x_cities_header.csv error 1062: Duplicate entry '2' for key " + keyname);
EXPECT_STDOUT_CONTAINS("WARNING: world_x_cities_header.csv error 1062: Duplicate entry '3' for key " + keyname);
EXPECT_STDOUT_CONTAINS("WARNING: world_x_cities_header.csv error 1062: Duplicate entry '4' for key " + keyname);
EXPECT_STDOUT_CONTAINS("WARNING: world_x_cities_header.csv error 1062: Duplicate entry '5' for key " + keyname);
EXPECT_STDOUT_CONTAINS("File '" + filename_for_output(__import_data_path + "/world_x_cities_header.csv") + "' (250.57 KB) was imported in ");
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".cities: Records: 4079  Deleted: 0  Skipped: 4079  Warnings: 4079");


//@<> TSET_4: Using the function util.importTable, set the option 'replace' to true. Validate that new rows replace old rows if both have the same Unique Key Value.
/// Replace on duplicate primary key, import csv
util.importTable(__import_data_path + '/world_x_cities.csv', {
    schema: target_schema, table: 'cities',
    fieldsTerminatedBy: ',', fieldsEnclosedBy: '"', fieldsOptionallyEnclosed: true, linesTerminatedBy: '\n', replaceDuplicates: true
});

EXPECT_STDOUT_CONTAINS("wl12193.cities: Records: 4079  Deleted: 0  Skipped: 0  Warnings: 0");
EXPECT_STDOUT_CONTAINS("File '" + filename_for_output(__import_data_path + "/world_x_cities.csv") + "' (250.53 KB) was imported in ");
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
EXPECT_STDOUT_CONTAINS("File '" + filename_for_output(__import_data_path + "/primer-dataset-id.json") + "' (11.29 MB) was imported in ");
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".document_store: Records: 25359  Deleted: 0  Skipped: 0  Warnings: 0");

WIPE_OUTPUT();
util.importTable(__import_data_path + '/primer-dataset-id.json', {
    schema: target_schema, table: 'document_store',
    columns: ['doc'], dialect: 'json', bytesPerChunk: '20M'
});

keyname = testutil.versionCheck(__version, '<', '8.0.19') ? `'PRIMARY'` : `'document_store.PRIMARY'`;

EXPECT_STDOUT_CONTAINS("wl12193.document_store: Records: 25359  Deleted: 0  Skipped: 25359  Warnings: 25359");
EXPECT_STDOUT_CONTAINS("WARNING: primer-dataset-id.json error 1062: Duplicate entry '000000000001' for key " + keyname);
EXPECT_STDOUT_CONTAINS("WARNING: primer-dataset-id.json error 1062: Duplicate entry '000000000002' for key " + keyname);
EXPECT_STDOUT_CONTAINS("WARNING: primer-dataset-id.json error 1062: Duplicate entry '000000000003' for key " + keyname);
EXPECT_STDOUT_CONTAINS("WARNING: primer-dataset-id.json error 1062: Duplicate entry '000000000004' for key " + keyname);
EXPECT_STDOUT_CONTAINS("WARNING: primer-dataset-id.json error 1062: Duplicate entry '000000000005' for key " + keyname);
EXPECT_STDOUT_CONTAINS("File '" + filename_for_output(__import_data_path + "/primer-dataset-id.json") + "' (11.29 MB) was imported in ");
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".document_store: Records: 25359  Deleted: 0  Skipped: 25359  Warnings: 25359");


//@<> Max bytes per chunk - minimum value is 2 * BUFFER_SIZE = 65KB * 2 = 128KB
util.importTable(__import_data_path + '/world_x_cities.dump', {
    schema: target_schema, table: 'cities',
    bytesPerChunk: '1'
});
EXPECT_STDOUT_CONTAINS("world_x_cities.dump: Records: 1523  Deleted: 0  Skipped: 1523  Warnings: 1523");
EXPECT_STDOUT_CONTAINS("world_x_cities.dump: Records: 2556  Deleted: 0  Skipped: 2556  Warnings: 2556");
EXPECT_STDOUT_CONTAINS("File '" + filename_for_output(__import_data_path + "/world_x_cities.dump") + "' (209.75 KB) was imported in ");
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".cities: Records: 4079  Deleted: 0  Skipped: 4079  Warnings: 4079");


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
EXPECT_STDOUT_CONTAINS("File '" + filename_for_output(__import_data_path + "/world_x_cities.dump") + "' (209.75 KB) was imported in ");
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".cities: Records: 4079  Deleted: 0  Skipped: 4079  Warnings: 4079");

//@<> BUG29822312 Import of table with foreign keys
session.runSql("CREATE TABLE `employee` (`id` int(11) NOT NULL AUTO_INCREMENT, `boss` int(11) DEFAULT NULL, PRIMARY KEY (`id`), KEY `boss` (`boss`), CONSTRAINT `employee_ibfk_1` FOREIGN KEY (`boss`) REFERENCES `employee` (`id`)) ENGINE=InnoDB;");
util.importTable(__import_data_path + '/employee_boss.csv', {table: 'employee', fieldsTerminatedBy: ','});
EXPECT_STDOUT_CONTAINS("wl12193.employee: Records: 7  Deleted: 0  Skipped: 0  Warnings: 0");
EXPECT_STDOUT_CONTAINS("File '" + filename_for_output(__import_data_path + "/employee_boss.csv") + "' (28 bytes) was imported in ");
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".employee: Records: 7  Deleted: 0  Skipped: 0  Warnings: 0");

//@<OUT> Show employee table
original_output_format = shell.options.resultFormat
shell.options.resultFormat = 'tabbed'
session.runSql('select * from ' + target_schema + '.employee order by boss asc');
shell.options.resultFormat = original_output_format

//@<> decodeColumns
session.runSql("CREATE TABLE IF NOT EXISTS `t_lob` ("+
    "`c1` tinyblob,"+
    "`c2` blob,"+
    "`c3` mediumblob,"+
    "`c4` longblob,"+
    "`c5` tinytext,"+
    "`c6` text,"+
    "`c7` mediumtext,"+
    "`c8` longtext,"+
    "`c9` tinytext CHARACTER SET utf8mb4 COLLATE utf8mb4_bin,"+
    "`c10` text CHARACTER SET utf8mb4 COLLATE utf8mb4_bin,"+
    "`c11` mediumtext CHARACTER SET utf8mb4 COLLATE utf8mb4_bin,"+
    "`c12` longtext CHARACTER SET utf8mb4 COLLATE utf8mb4_bin"+
  ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");

util.importTable(__import_data_path + '/xtest.t_lob.tsv', {
    schema: target_schema, table: 't_lob',
    "columns": [
        "c1",
        "c2",
        "c3",
        "c4",
        "c5",
        "c6",
        "c7",
        "c8",
        "c9",
        "c10",
        "c11",
        "c12"
    ],
    decodeColumns: {
        "c1": "UNHEX",
        "c2": "UNHEX",
        "c3": "UNHEX",
        "c4": "UNHEX"
    }
});

//@<OUT> dump blob data
shell.dumpRows(session.runSql("SELECT md5(c1), md5(c2), md5(c3), md5(c4), md5(c5), md5(c6), md5(c7), md5(c8), md5(c9), md5(c10), md5(c11), md5(c12) FROM "+target_schema+".t_lob"), "tabbed");

//@<> Create latin2 charset table
session.runSql("create table cities_latin2(id integer primary key, name text) CHARACTER SET = latin2")

//@<OUT> Import to table with utf8 character set
util.importTable(__import_data_path + '/cities_pl_utf8.dump', {table:'cities_latin2', characterSet: 'utf8mb4'})
shell.dumpRows(session.runSql('select hex(id), hex(name) from cities_latin2'), "tabbed")
session.runSql("truncate table cities_latin2")

//@<OUT> Import to table with latin2 character set
util.importTable(__import_data_path + '/cities_pl_latin2.dump', {table:'cities_latin2', characterSet: 'latin2'})
shell.dumpRows(session.runSql('select hex(id), hex(name) from cities_latin2'), "tabbed")

//@<> Import into table a gzip file
session.runSql('TRUNCATE TABLE cities');
util.importTable(__import_data_path + '/world_x_cities.gz', { schema: target_schema, table: 'cities' })
EXPECT_EQ(4079, session.runSql('select count(*) from cities').fetchOne()[0])

//@<> Import into table a zstd file
session.runSql('TRUNCATE TABLE cities');
util.importTable(__import_data_path + '/world_x_cities.zst', { schema: target_schema, table: 'cities' })
EXPECT_EQ(4079, session.runSql('select count(*) from cities').fetchOne()[0])

//@<> Import into table a zip file
EXPECT_THROWS(function () {
    util.importTable(__import_data_path + '/world_x_cities.zip', { schema: target_schema, table: 'cities' });
}, "");


//@<> Create mirror table
session.runSql("DROP TABLE IF EXISTS `t_lob_userdefinedset`");
session.runSql("CREATE TABLE `t_lob_userdefinedset` LIKE `t_lob`");

//@<> User defined transformations using decodeColumns
util.importTable(__import_data_path + '/xtest.t_lob.tsv', {
    schema: target_schema, table: 't_lob_userdefinedset',
    "columns": [
        1,
        2,
        3,
        4,
        "c5",
        "c6",
        "c7",
        "c8",
        "c9",
        "c10",
        "c11",
        "c12"
    ],
    decodeColumns: {
        "c1": "UNHEX(@1)",
        "c2": "UNHEX(@2)",
        "c3": "UNHEX(@3)",
        "c4": "UNHEX(@4)"
    }
});

var all_columns = Array('c1','c2','c3','c4','c5','c6','c7','c8','c9','c10','c11','c12');
EXPECT_EQ(
    compute_crc(target_schema, 't_lob', all_columns),
    compute_crc(target_schema, 't_lob_userdefinedset', all_columns)
);

//@<> Skip columns
session.runSql("CREATE TABLE IF NOT EXISTS `t_lob_skip` ("+
    "`c1` tinyblob,"+
    "`c2` blob"+
  ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");

util.importTable(__import_data_path + '/xtest.t_lob.tsv', {
    schema: target_schema, table: 't_lob_skip',
    "columns": [
        1,
        2,
        3,
        4,
        5,
        6,
        7,
        8,
        9,
        10,
        11,
        12
    ],
    decodeColumns: {
        "c1": "UNHEX(@1)",
        "c2": "UNHEX(@2)"
    }
});

//@<> Skip columns output check
original_output_format = shell.options.resultFormat
shell.options.resultFormat = 'vertical'
session.runSql('SELECT * FROM t_lob_skip;');
shell.options.resultFormat = original_output_format
EXPECT_STDOUT_CONTAINS_MULTILINE(`
vertical
*************************** 1. row ***************************
c1:
c2:
*************************** 2. row ***************************
c1: tinyblob-text readable
c2: blob-text readable
*************************** 3. row ***************************
c1: NULL
c2: NULL
3 rows in set [[*]]
table
`);

//@<> User defined operations
session.runSql("CREATE TABLE IF NOT EXISTS `t_numbers` ("+
    "`a` integer," +
    "`b` integer," +
    "`sum` integer," +
    "`pow` integer," +
    "`mul` integer" +
  ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");

util.importTable(__import_data_path + '/numbers.tsv', {
    schema: target_schema, table: 't_numbers',
    columns: [
        1,
        2,
        3,
        4
    ],
    decodeColumns: {
        "a": "@1",
        "b": "@2",
        "sum": "@1 + @2",
        "pow": "pow(@1, @2)",
        "mul": "@1 * @2"
    }
});

//@<> User defined operations output check
original_output_format = shell.options.resultFormat
shell.options.resultFormat = 'tabbed'
session.runSql('SELECT * FROM t_numbers;');
shell.options.resultFormat = original_output_format
EXPECT_STDOUT_CONTAINS_MULTILINE(`
tabbed
a	b	sum	pow	mul
1	1	2	1	1
2	2	4	4	4
3	3	6	27	9
4	4	8	256	16
5	5	10	3125	25
6	6	12	46656	36
6 rows in set [[*]]
table
`);

//@<> Invalid columns type passed
EXPECT_THROWS(function () {
    util.importTable(__import_data_path + '/numbers.tsv', {
        schema: target_schema, table: 't_numbers',
        columns: [
            1,
            2,
            3,
            {}
        ],
        decodeColumns: {
            "a": "@1",
            "b": "@2",
            "sum": "@1 + @2",
            "pow": "pow(@1, @2)",
            "mul": "@1 * @2"
        }
    });
}, "Util.importTable: Option 'columns' String (column name) or non-negative Integer (user variable binding) expected, but value is Map");

EXPECT_THROWS(function () {
    util.importTable(__import_data_path + '/numbers.tsv', {
        schema: target_schema, table: 't_numbers',
        columns: [
            1,
            2,
            3,
            -6
        ],
        decodeColumns: {
            "a": "@1",
            "b": "@2",
            "sum": "@1 + @2",
            "pow": "pow(@1, @2)",
            "mul": "@1 * @2"
        }
    });
}, "Util.importTable: User variable binding in 'columns' option must be non-negative integer value");

//@<> Unbalanced brackets in transformations
EXPECT_THROWS(function () {
    util.importTable(__import_data_path + '/numbers.tsv', {
        schema: target_schema, table: 't_numbers',
        columns: [
            1,
            2,
            3,
            -6
        ],
        decodeColumns: {
            "a": "@1",
            "b": "@2",
            "sum": "@1 + @2",
            "pow": "pow(@1, @2)",
            "mul": ")@1 * @2"
        }
    });
}, "Util.importTable: Invalid SQL expression in decodeColumns option for column 'mul'");

//@<> Invalid brackets in transformations
EXPECT_THROWS(function () {
    util.importTable(__import_data_path + '/numbers.tsv', {
        schema: target_schema, table: 't_numbers',
        columns: [
            1,
            2,
            3,
            4
        ],
        decodeColumns: {
            "a": "@1",
            "b": "@2",
            "sum": "@1 + @2",
            "pow": "pow(@1, @2)",
            "mul": "@1 * @2\""
        }
    });
}, "Util.importTable: Invalid SQL expression in decodeColumns option for column 'mul'");

EXPECT_THROWS(function () {
    util.importTable(__import_data_path + '/numbers.tsv', {
        schema: target_schema, table: 't_numbers',
        columns: [
            1,
            2,
            3,
            4
        ],
        decodeColumns: {
            "a": "@1",
            "b": "@2",
            "sum": "@1 + @2",
            "pow": "pow(@1, @2)",
            "mul": "(@1 * @2))"
        }
    });
}, "Util.importTable: Invalid SQL expression in decodeColumns option for column 'mul'");

EXPECT_THROWS(function () {
    util.importTable(__import_data_path + '/numbers.tsv', {
        schema: target_schema, table: 't_numbers',
        columns: [
            1,
            2,
            3,
            4
        ],
        decodeColumns: {
            "a": "@1",
            "b": "@2",
            "sum": "@1 + @2",
            "pow": "pow(@1, @2)",
            "mul": "@1 * ((@2)"
        }
    });
}, "Util.importTable: Invalid SQL expression in decodeColumns option for column 'mul'");

EXPECT_THROWS(function () {
    util.importTable(__import_data_path + '/numbers.tsv', {
        schema: target_schema, table: 't_numbers',
        columns: [
            1,
            2,
            3,
            4
        ],
        decodeColumns: {
            "a": "@1",
            "b": "@2",
            "sum": "@1 + @2",
            "pow": "pow(@1, @2)",
            "mul": "@1 * @2\"\\"
        }
    });
}, "Util.importTable: Invalid SQL expression in decodeColumns option for column 'mul'");

//@<> boolean operators in preprocessing transformations
session.runSql("CREATE TABLE IF NOT EXISTS `t_bools` ("+
    "`id` integer not null primary key AUTO_INCREMENT," +
    "`bool_not0` integer," +
    "`bool_not1` integer," +
    "`not0` integer," +
    "`not1` integer," +
    "`a_or_b` integer," +
    "`not_set` integer default -1," +
    "`null_set` integer default -1" +
  ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");

// reuse numbers.tsv as input, but skip it's values
util.importTable(__import_data_path + '/numbers.tsv', {
    schema: target_schema, table: 't_bools',
    columns: [
        1,
        2,
        3,
        4
    ],
    decodeColumns: {
        "id": "NULL",
        "bool_not0": "!0",
        "bool_not1": "!1",
        "not0": "NOT 0",
        "not1": "NOT 1",
        "a_or_b": "(!0) OR (!1)",
        "null_set": "NULL"
    }
});

//@<> boolean operators in preprocessing transformations output check
original_output_format = shell.options.resultFormat
shell.options.resultFormat = 'tabbed'
session.runSql('SELECT * FROM t_bools;');
shell.options.resultFormat = original_output_format
EXPECT_STDOUT_CONTAINS_MULTILINE(`
tabbed
id	bool_not0	bool_not1	not0	not1	a_or_b	not_set	null_set
1	1	0	1	0	1	-1	NULL
2	1	0	1	0	1	-1	NULL
3	1	0	1	0	1	-1	NULL
4	1	0	1	0	1	-1	NULL
5	1	0	1	0	1	-1	NULL
6	1	0	1	0	1	-1	NULL
6 rows in set ([[*]])
table
`);

//@<> Teardown
session.runSql("DROP SCHEMA IF EXISTS " + target_schema);
session.close();

//@<> Throw if session is closed
EXPECT_THROWS(function () {
    util.importTable(__import_data_path + '/world_x_cities.dump', { table: 'cities' });
}, "A classic protocol session is required to perform this operation.");
