// This is unit test file for WL10606 Import JSON documents
const target_port = __mysql_sandbox_port1
const target_xport = __mysql_sandbox_port1 + "0"
const target_schema = 'wl10606';
const uri = "mysql://root:root@localhost:" + target_port;
const xuri = "mysqlx://root:root@localhost:" + target_xport;

function create_table(table_name, column_name) {
  if (!column_name) column_name = 'doc';
  return 'create table `' + target_schema + '`.`' + table_name + '` (`' +
         column_name + '` JSON);';
}

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

testutil.deployRawSandbox(target_port, "root");

//@ Only X Protocol is supported
var classic_session = mysql.getClassicSession(uri);
shell.setSession(classic_session);
EXPECT_THROWS(function() {
  util.importJson(__import_data_path + '/sample.json',
                  {schema : target_schema});
}, "An X Protocol session is required for JSON import.");
classic_session.close();

//@ Setup test
wait_for_server(xuri);
shell.connect(xuri);
var schema_handler = session.createSchema(target_schema);

//@ FR1-01
/// FR1-01  Validate that import VALID collection to a VALID schema, into
/// existing collection|table works - DEV
schema_handler.createCollection("fr1_01_sample");
util.importJson(__import_data_path + '/sample.json', {schema: target_schema, collection: 'fr1_01_sample'});
EXPECT_STDOUT_CONTAINS(
    'Importing from file "' + __import_data_path + '/sample.json' +
    '" to collection `wl10606`.`fr1_01_sample` in MySQL Server at');

util.importJson(__import_data_path + '/sample2.json', {schema: target_schema, collection: 'sample'});
var rc = testutil.callMysqlsh([xuri, '--schema', target_schema, '--import', __import_data_path + '/sample.json', 'sample']);
EXPECT_EQ(0, rc);
EXPECT_STDOUT_CONTAINS(
    'Importing from file "' + __import_data_path + '/sample2.json' +
    '" to collection `wl10606`.`sample` in MySQL Server at');

session.sql(create_table('fr1_01_sample_table'));
util.importJson(__import_data_path + '/sample.json', {schema: target_schema, table: 'fr1_01_sample_table'});
EXPECT_STDOUT_CONTAINS(
    'Importing from file "' + __import_data_path + '/sample.json' +
    '" to table `wl10606`.`fr1_01_sample_table` in MySQL Server at');

var rc = testutil.callMysqlsh([xuri, '--schema', target_schema, '--import', __import_data_path + '/sample.json', 'fr1_01_sample_table', 'doc']);
EXPECT_EQ(0, rc);
EXPECT_STDOUT_CONTAINS(
    'Importing from file "' + __import_data_path + '/sample.json' +
    '" to table `wl10606`.`fr1_01_sample_table` in MySQL Server at');

//@ FR1-02 function call
/// FR1-02  Validate that import VALID collection dump file to a VALID schema
/// without existing collection|table works - DEV
util.importJson(__import_data_path + '/sample.json', {schema: target_schema, collection: 'fr1_02_sample'});
EXPECT_STDOUT_CONTAINS(
    'Importing from file "' + __import_data_path + '/sample.json' +
    '" to collection `wl10606`.`fr1_02_sample` in MySQL Server at');

util.importJson(__import_data_path + '/sample.json', {schema: target_schema, table: 'fr1_02_sample_table'});
EXPECT_STDOUT_CONTAINS(
    'Importing from file "' + __import_data_path + '/sample.json' +
    '" to table `wl10606`.`fr1_02_sample_table` in MySQL Server at');

//@ FR1-02 cli call
var rc = testutil.callMysqlsh([xuri, '--schema='+ target_schema, '--import', __import_data_path + '/sample.json', 'fr1_02_sample_cli']);
EXPECT_EQ(0, rc);
EXPECT_STDOUT_CONTAINS(
    'Importing from file "' + __import_data_path + '/sample.json' +
    '" to collection `wl10606`.`fr1_02_sample_cli` in MySQL Server at');

var rc = testutil.callMysqlsh([xuri, '--schema='+ target_schema, '--import', __import_data_path + '/sample.json', 'fr1_02_sample_cli_table', 'doc']);
EXPECT_EQ(0, rc);
EXPECT_STDOUT_CONTAINS(
    'Importing from file "' + __import_data_path + '/sample.json' +
    '" to table `wl10606`.`fr1_02_sample_cli_table` in MySQL Server at');


//@ FR1-03
/// FR1-03  Validate that import VALID collection dump file into a INVALID schema
/// raise error regarding invalid schema  - DEV
EXPECT_THROWS(function() {
  util.importJson(__import_data_path + '/sample.json', {schema : "non_existing_schema", collection : 'sample'});
}, "Unknown database 'non_existing_schema'");

//@ FR1-04
/// FR1-04  validates that import INVALID collection file with VALID schema raise
/// error regarding invalid collection file, and report the documents that were
/// imported successfully - DEV
EXPECT_THROWS(function() {
  util.importJson(__import_data_path + '/sample_invalid.json', {schema : target_schema});
}, "Util.importJson: Premature end of input stream at offset 2550");
EXPECT_STDOUT_CONTAINS(
    "Importing from file \"" + __import_data_path + '/sample_invalid.json' +
    "\" to collection `wl10606`.`sample_invalid` in MySQL Server at");
EXPECT_STDOUT_CONTAINS("Total successfully imported documents 0 ");

//@ FR1-05
/// FR1-05  Validate that import INVALID collection file into a INVALID schema
/// raise error regarding invalid schema and/or collection file  - DEV
EXPECT_THROWS(function() {
  util.importJson(__import_data_path + '/sample_invalid.json', {schema : "non_existing_schema"});
}, "Unknown database 'non_existing_schema'");

//@ FR1-06  Validate that import works with multiline documents.
util.importJson(__import_data_path + '/sample_pretty.json', {schema: target_schema});
EXPECT_STDOUT_CONTAINS(
    "Importing from file \"" + __import_data_path + '/sample_pretty.json' +
    "\" to collection `wl10606`.`sample_pretty` in MySQL Server at");
EXPECT_STDOUT_CONTAINS("Total successfully imported documents 18 ");

//@ FR2-01a
/// FR2-01  Validate that import VALID json STDIN to a VALID schema, into
/// existing collection|table works - DEV
schema_handler.createCollection("FR2_01a_sample");
var rc = testutil.callMysqlsh([xuri, '--schema='+ target_schema, '--import', '-', 'FR2_01a_sample'], '{"_id": "a"}{"_id": "b"}{"_id": "c"}{"_id": "d"}{"_id": "e"}{"_id": "f"}');
EXPECT_EQ(0, rc);
EXPECT_STDOUT_CONTAINS("Importing from -stdin- to collection `wl10606`.`FR2_01a_sample` in MySQL Server at");
EXPECT_STDOUT_CONTAINS("Total successfully imported documents 6 ");

//@ FR2-01b
session.sql(create_table('FR2_01b_sample_table'));
var rc = testutil.callMysqlsh([xuri, '--schema='+ target_schema, '--import', '-', 'FR2_01b_sample_table', 'doc'], '{"_id": "a"}{"_id": "b"}{"_id": "c"}{"_id": "d"}{"_id": "e"}{"_id": "f"}');
EXPECT_EQ(0, rc);
EXPECT_STDOUT_CONTAINS("Importing from -stdin- to table `wl10606`.`FR2_01b_sample_table` in MySQL Server at");
EXPECT_STDOUT_CONTAINS("Total successfully imported documents 6 ");

//@ FR2-02a
/// FR2-02  Validate that import VALID json STDIN to a VALID schema with no
/// collection|table existing works - DEV
var rc = testutil.callMysqlsh([xuri, '--schema='+ target_schema, '--import', '-', 'fr2_02_sample'], '{"_id": "a"}{"_id": "b"}{"_id": "c"}{"_id": "d"}{"_id": "e"}{"_id": "f"}');
EXPECT_EQ(0, rc);
EXPECT_STDOUT_CONTAINS("Importing from -stdin- to collection `wl10606`.`fr2_02_sample` in MySQL Server at");
EXPECT_STDOUT_CONTAINS("Total successfully imported documents 6 ");

//@ FR2-02b
var rc = testutil.callMysqlsh([xuri, '--schema='+ target_schema, '--import', '-', 'fr2_02_sample_table', 'doc'], '{"_id": "a"}{"_id": "b"}{"_id": "c"}{"_id": "d"}{"_id": "e"}{"_id": "f"}');
EXPECT_EQ(0, rc);
EXPECT_STDOUT_CONTAINS("Importing from -stdin- to table `wl10606`.`fr2_02_sample_table` in MySQL Server at");
EXPECT_STDOUT_CONTAINS("Total successfully imported documents 6 ");

//@ FR2-03 Using shell function call
/// FR2-03  Validate that import VALID json STDIN to an INVALID schema, raise
/// error regarding invalid schema (Shell API call raise error about non-empty path)
EXPECT_THROWS(function() {
  util.importJson("", {schema : "non_existing_schema", collection : 'sample'});
}, "Util.importJson: Path cannot be empty.");

//@<> FR2-03 Using mysqlsh command line arguments
var rc = testutil.callMysqlsh(
    [ xuri, '--schema=non_existing_schema', '--import', '-', 'sample', 'doc' ],
    '{"_id": "a"}{"_id": "b"}{"_id": "c"}{"_id": "d"}{"_id": "e"}{"_id": "f"}');
EXPECT_NE(0, rc);
if (testutil.versionCheck(__version, "==", "8.0.12")) {
  EXPECT_STDOUT_CONTAINS("Access denied for user");
} else {
  EXPECT_STDOUT_CONTAINS("Unknown database 'non_existing_schema'");
}

//@ FR2-04
/// FR2-04  Validate that import INVALID json STDIN to a VALID schema, raise
/// error regarding invalid json STDIN and report the documents that were
/// imported successfully  - DEV
var rc = testutil.callMysqlsh([xuri, '--schema='+ target_schema, '--import', '-', 'fr2_04_sample'], '{"_id": "a"}{"_id": "b"}{"_id": "c"}}{"_id": "d"}{"_id": "e"}{"_id": "f"}');
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Importing from -stdin- to collection `wl10606`.`fr2_04_sample` in MySQL Server at");
EXPECT_STDOUT_CONTAINS("Input does not start with a JSON object at offset 36");
EXPECT_STDOUT_CONTAINS("Total successfully imported documents 0 ");

//@<> FR2-05
/// FR2-05  Validate that import INVALID json STDIN to an INVALID schema, raise
/// error regarding invalid schema adn/or invalid json STDIN  - DEV
var rc = testutil.callMysqlsh(
    [ xuri, '--schema=non_existing_schema', '--import', '-', 'fr2_05_sample' ],
    '{"_id": "a"}{"_id": "b"}{"_id": "c"}}{"_id": "d"}{"_id": "e"}{"_id": "f"}');
EXPECT_NE(0, rc);
if (testutil.versionCheck(__version, "==", "8.0.12")) {
  EXPECT_STDOUT_CONTAINS("Access denied for user");
} else {
  EXPECT_STDOUT_CONTAINS("Unknown database 'non_existing_schema'");
}

/// FR3-01  Validate when import call is interrupted (intr signal - ^C), user get
/// error message along with number of successfully imported (committed)
/// documents.
// Covered in unit test `Interrupt_mysqlsh.json_import`.

//@ FR5-01
/// FR5-01  Validate that import data from MongoDB into JSON strict mode
/// representation and option convertBsonOid set to true, enables ObjectId
/// representation conversion.  - DEV
util.importJson(__import_data_path + '/restaurants_mini.json', {
  schema : target_schema,
  convertBsonOid : true,
  collection : 'oidtest_fr5_01'
});
EXPECT_STDOUT_CONTAINS(
    "Importing from file \"" + __import_data_path + '/restaurants_mini.json' +
    "\" to collection `wl10606`.`oidtest_fr5_01` in MySQL Server at");
EXPECT_STDOUT_CONTAINS("Total successfully imported documents 10 ");

//@<OUT> FR5-01 output
oidcoll = schema_handler.getCollection("oidtest_fr5_01");
oidcoll.find().sort("_id");

//@ FR5-02
/// FR5-02  Validate that import data from MongoDB and not specified option
/// convertBsonOid (default false, disabling the ObjectId representation
/// conversion), will raise error message. ????????  - DEV
EXPECT_THROWS(function() {
  util.importJson(__import_data_path + '/restaurants_mini.json',
                  {schema : target_schema, collection : 'oidtest_fr5_02'})
}, "Data too long for column '_id' at row 1");
EXPECT_STDOUT_CONTAINS(
    "Importing from file \"" + __import_data_path + '/restaurants_mini.json' +
    "\" to collection `wl10606`.`oidtest_fr5_02` in MySQL Server at");
EXPECT_STDOUT_CONTAINS("Total successfully imported documents 0 ");

//@<OUT> FR5-02 output
oidcoll = schema_handler.getCollection("oidtest_fr5_02");
oidcoll.find().sort("_id");

/// FR5-03  Validate that import data from JSON strict mode representation and
/// option convertBsonOid set true, will raise error message. ???????? - DEV
// invalid test

/// FR5-04  Validate that import data from MongoDB and option convertBsonOid set
/// to false, will raise error message. ???????? - DEV
// invalid test

/// FR5-05  Validate that import data from MongoDB and option convertBsonOid set
/// to true and ObjectId is invalid (not 24-hexadecimal string or wrong chars)
/// will raise error message. ????????  - DEV
// invalid test

/// FRA1-01A  Validate that default collection name is the same as the collection
/// dump filename (without extension) - DEV
// This is a mix of basename and split_extension functions call
// Covered in utils_path.basename and utils_path.split_extension unit test

/// FRA1-01B  Validate that path to filename without extension, fully qualified
/// path names and canonical path names are supported - DEV
// This is a mix of basename and split_extension functions call
// Covered in utils_path.basename and utils_path.split_extension unit test

//@ FRA1-02A
/// FRA1-02A  validate that missing schema name (and missing schema in current
/// session) raise an error  - DEV
EXPECT_THROWS(function() {
  util.importJson(__import_data_path + '/sample.json');
}, "Util.importJson: There is no active schema on the current session, the target schema for the import operation must be provided in the options.");

var rc = testutil.callMysqlsh([xuri, '--import', __import_data_path + '/sample.json', 'sample']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Error: --import requires a default schema on the active session.");

//@ FRA1-02B
/// FRA1-02B  validate that if schema isn't provided as parameter, mysqlsh will
/// try to retrieve schema name from current session. - DEV
var rc = testutil.callMysqlsh([
  xuri + '/' + target_schema, '--import', __import_data_path + '/sample.json',
  'fra01_02b_sample'
]);
EXPECT_EQ(0, rc);
EXPECT_STDOUT_CONTAINS(
    'Importing from file "' + __import_data_path + '/sample.json' +
    '" to collection `wl10606`.`fra01_02b_sample` in MySQL Server at');
EXPECT_STDOUT_CONTAINS("Total successfully imported documents 18 ");

//@ FRA1-03
/// FRA1-03 Validate that missing collection name when input is STDIN raise an
/// error. - DEV
EXPECT_THROWS(function() {
  util.importJson('', {schema: target_schema});
}, "Util.importJson: Path cannot be empty.");

var rc = testutil.callMysqlsh([xuri, '--schema=' + target_schema, '--import', '-']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Target collection or table must be set if filename is a STDIN");

//@ FRB1-01
/// FRB1-01 Validate that documents must be imported into a schema, table and
/// column specified by the user  - DEV
util.importJson(__import_data_path + '/test_json_object.json',
                {schema : target_schema, collection: "frb1_01_collection"});
EXPECT_STDOUT_CONTAINS(
    'Importing from file "' + __import_data_path + '/test_json_object.json' +
    '" to collection `wl10606`.`frb1_01_collection` in MySQL Server at');


//@ FRB1-01 target collection correctness
var coll_ = schema_handler.getCollection("frb1_01_collection");
coll_.find().sort("_id");

//@ FRB1-01 target table correctness
util.importJson(__import_data_path + '/test_json_object.json',
                {schema : target_schema, table: "frb1_01_table", tableColumn: "user_column_name"});
EXPECT_STDOUT_CONTAINS(
    'Importing from file "' + __import_data_path + '/test_json_object.json' +
    '" to table `wl10606`.`frb1_01_table` in MySQL Server at');

var coll_ = schema_handler.getTable("frb1_01_table");
coll_.select().orderBy("id");

//@ FRB1-03
/// FRB1-03 Validates that default column name is doc - DEV
util.importJson(__import_data_path + '/test_json_object.json',
                {schema : target_schema, table: "frb1_01_table_default_column"});
EXPECT_STDOUT_CONTAINS(
    'Importing from file "' + __import_data_path + '/test_json_object.json' +
    '" to table `wl10606`.`frb1_01_table_default_column` in MySQL Server at');

var coll_ = schema_handler.getTable("frb1_01_table_default_column");
coll_.select("doc").orderBy("id");

//@ FRB1-04 function call
/// FRB1-04 Validates that missing schema name is an error  - DEV
EXPECT_THROWS(function() {
  util.importJson(__import_data_path + '/sample.json');
}, "Util.importJson: There is no active schema on the current session, the target schema for the import operation must be provided in the options.");

//@ FRB1-04 cli
var rc = testutil.callMysqlsh([xuri, '--import', __import_data_path + '/sample.json']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Error: --import requires a default schema on the active session.");

//@ FRB1-05
/// FRB1-05 Validate that missing table name when input is STDIN is an error
EXPECT_THROWS(function() {
  util.importJson('');
}, "Util.importJson: There is no active schema on the current session, the target schema for the import operation must be provided in the options.");

EXPECT_THROWS(function() {
  util.importJson('', {schema: target_schema});
}, "Util.importJson: Path cannot be empty.");

var rc =
    testutil.callMysqlsh([ xuri, '--schema=' + target_schema, '--import', '-' ],
                         '{"_id": "a"}{"_id": "b"}{"_id": "c"}');
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Target collection or table must be set if filename is a STDIN");


//@<OUT> FRB2-02
/// FRB2-02 Validate that default table structure is id with type INTEGER
/// AUTO_INCREMENT PRIMARY KEY and the target column as JSON  - DEV
util.importJson(__import_data_path + '/sample.json', {schema: target_schema, table: "frb2_02_new_table"});
session.sql("describe "+target_schema+".frb2_02_new_table;");

//@<OUT> FRB3-01a
/// FRB3-01 Validate that Inserting to a table should populate the target column
/// only. Other columns must be left to have their default value.  - DEV
session.sql("create table " + target_schema + ".frb3_01_new_table (ident int AUTO_INCREMENT PRIMARY KEY, json_data JSON, default_value int not null default 42);");
util.importJson(__import_data_path + '/sample.json', {
  schema : target_schema,
  table : "frb3_01_new_table",
  tableColumn : "json_data"
});
session.sql("describe " + target_schema + ".frb3_01_new_table;");

//@<OUT> FRB3-01b 8.0 {VER(>=8.0)}
session.sql("select * from " + target_schema + ".frb3_01_new_table;");

//@<OUT> FRB3-01b 5.7 {VER(<8.0)}
session.sql("select * from " + target_schema + ".frb3_01_new_table;");

/// E1  Validate that help documentation describes new import argument  - DEV
// Covered in util_help_norecord.js

//@ E2
/// E2  Validate that products_big.json file collection|table is processed
/// correctly and in acceptable time - DEV
util.importJson(__import_data_path + '/primer-dataset-id.json', {
  schema : target_schema,
});
EXPECT_STDOUT_CONTAINS(
    'Importing from file "' + __import_data_path + '/primer-dataset-id.json' +
    '" to collection `wl10606`.`primer-dataset-id` in MySQL Server at');
EXPECT_STDOUT_CONTAINS("Total successfully imported documents 25359 ");

//@<OUT> E2 count
session.sql("select count(1) from `" + target_schema + "`.`primer-dataset-id`");

//@ E3
/// E3  Validate if user call mysqlsh user@host/mydb --import c:\bla.js blubb, we
/// will create collection blubb if neither collection nor table exits. - DEV
var rc = testutil.callMysqlsh([
  xuri + '/' + target_schema, '--import', __import_data_path + '/sample.json',
  'blubb'
]);
EXPECT_EQ(0, rc);
EXPECT_STDOUT_CONTAINS(
    'Importing from file "' + __import_data_path + '/sample.json' +
    '" to collection `wl10606`.`blubb` in MySQL Server at');
println(schema_handler.getCollections());
session.sql("describe " + target_schema + ".blubb;");

//@ E4
/// E4  Validate if user call mysqlsh user@host/mydb --import c:\bla.js blubb,
/// and blubb exists and user do not point table column, default column is doc,
/// which is available for collections and for tables. - DEV
util.importJson(__import_data_path + '/sample.json', {
  schema : target_schema,
  table : "blubb_table"
});
var rc = testutil.callMysqlsh([
  xuri + '/' + target_schema, '--import', __import_data_path + '/sample.json',
  'blubb_table'
]);
EXPECT_EQ(0, rc);
EXPECT_STDOUT_CONTAINS(
    'Importing from file "' + __import_data_path + '/sample.json' +
    '" to table `wl10606`.`blubb_table` in MySQL Server at');
println(schema_handler.getTables());

//@<OUT> E4 describe
session.sql("describe " + target_schema + ".blubb_table;");

//@<> E5 Import to VIEW is not supported
session.sql("CREATE VIEW " + target_schema + ".blubb_table_view AS SELECT * FROM " +
            target_schema + ".blubb_table;").execute();

EXPECT_THROWS(
    function() {
      util.importJson(
          __import_data_path + '/sample.json',
          {schema : target_schema, collection : "blubb_table_view"});
    },
    "Table '" + target_schema +
        ".blubb_table_view' exists but is not a collection");

var rc = testutil.callMysqlsh([
  xuri + '/' + target_schema, '--import', __import_data_path + '/sample.json',
  'blubb_table_view'
]);

EXPECT_EQ(1, rc);
EXPECT_STDOUT_CONTAINS(
    '\'' + target_schema +
    '\'.\'blubb_table_view\' is a view. Target must be table or collection.');

//@<> E6 Missing connection options on cli returns error
var rc = testutil.callMysqlsh(['--import', __import_data_path + '/sample.json', 'sample']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Error: --import requires an active session.");

//@<> tableColumn cannot be used with collection
EXPECT_THROWS(
    function() {
      util.importJson(
          __import_data_path + '/sample.json',
          {schema : target_schema, collection : "blubb_table_view", tableColumn: "doc"});
    },
    "tableColumn cannot be used with collection.");

//@ Import document with size greater than mysqlx_max_allowed_packet
session.close()
testutil.stopSandbox(target_port);
testutil.changeSandboxConf(target_port, "max_allowed_packet",        "1M");
testutil.changeSandboxConf(target_port, "mysqlx_max_allowed_packet", "1M");
testutil.startSandbox(target_port);
wait_for_server(xuri);
shell.connect(xuri);

EXPECT_THROWS(function() {
  util.importJson(__import_data_path + '/2MB_doc.json', {
    schema : target_schema,
    collection : "2MB_greater_____"
  });
}, "JSON document is too large. Increase mysqlx_max_allowed_packet value to at least 2097228 bytes.");
EXPECT_STDOUT_CONTAINS(
    'Importing from file "' + __import_data_path + '/2MB_doc.json' +
    '" to collection `wl10606`.`2MB_greater_____` in MySQL Server at');
EXPECT_STDOUT_CONTAINS("Total successfully imported documents 0 ");

//@ Import document with size equal to mysqlx_max_allowed_packet
session.close()
testutil.stopSandbox(target_port);
testutil.changeSandboxConf(target_port, "max_allowed_packet",        "2M"); // 2M == 2097152 bytes
testutil.changeSandboxConf(target_port, "mysqlx_max_allowed_packet", "2M");
testutil.startSandbox(target_port);
wait_for_server(xuri);
shell.connect(xuri);

EXPECT_THROWS(function() {
  util.importJson(__import_data_path + '/2MB_doc.json', {
    schema : target_schema,
    collection : "2MB_equal_______"
  });
}, "JSON document is too large. Increase mysqlx_max_allowed_packet value to at least 2097228 bytes.");
EXPECT_STDOUT_CONTAINS(
    'Importing from file "' + __import_data_path + '/2MB_doc.json' +
    '" to collection `wl10606`.`2MB_equal_______` in MySQL Server at');
EXPECT_STDOUT_CONTAINS("Total successfully imported documents 0 ");

//@ Import 2 megabyte document with recommended mysqlx_max_allowed_packet value
session.close()
testutil.stopSandbox(target_port);
testutil.changeSandboxConf(target_port, "max_allowed_packet",        "2049K"); // max_allowed_packet must be rounded to kilobytes.
testutil.changeSandboxConf(target_port, "mysqlx_max_allowed_packet", "2097228");
testutil.startSandbox(target_port);
wait_for_server(xuri);
shell.connect(xuri);

util.importJson(__import_data_path + '/2MB_doc.json', {
  schema : target_schema,
  collection: "2MB_recommended_"
});
EXPECT_STDOUT_CONTAINS(
    'Importing from file "' + __import_data_path + '/2MB_doc.json' +
    '" to collection `wl10606`.`2MB_recommended_` in MySQL Server at');
EXPECT_STDOUT_CONTAINS("Total successfully imported documents 1 ");

//@ Import document with size less than mysqlx_max_allowed_packet
session.close()
testutil.stopSandbox(target_port);
testutil.changeSandboxConf(target_port, "max_allowed_packet",        "4M");
testutil.changeSandboxConf(target_port, "mysqlx_max_allowed_packet", "4M");
testutil.startSandbox(target_port);
wait_for_server(xuri);
shell.connect(xuri);

util.importJson(__import_data_path + '/2MB_doc.json', {
  schema : target_schema,
  collection: "2MB_less________"
});
EXPECT_STDOUT_CONTAINS(
    'Importing from file "' + __import_data_path + '/2MB_doc.json' +
    '" to collection `wl10606`.`2MB_less________` in MySQL Server at');
EXPECT_STDOUT_CONTAINS("Total successfully imported documents 1 ");

//@<> Import document using invalid options
EXPECT_THROWS(function() {
  util.importJson(__import_data_path + '/2MB_doc.json', {
    schema : target_schema,
    collection: "2MB_less________",
    unexisting: 5
  });
}, "Util.importJson: Invalid options: unexisting");

//@ Teardown
session.close();
testutil.destroySandbox(target_port);
