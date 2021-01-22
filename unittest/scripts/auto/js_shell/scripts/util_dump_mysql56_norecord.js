//@{__mysql56_uri != null && VER(>=8.0.0)}
// run only vs mysql 8.0, in part because paratrace doesn't support multiple instances of 5.6

// constants
const k_dump_dir = os.path.join(__tmp_dir, "ldtest");
const k_instance_dump = os.path.join(k_dump_dir, "dumpi");
const k_schemas_dump = os.path.join(k_dump_dir, "dumps");
const k_tables_dump = os.path.join(k_dump_dir, "dumpt");
const k_all_schemas = [ "sakila", "all_features", "all_features2", "xtest" ];

// helpers
function cleanup_dump_dir() {
  try {
    testutil.rmdir(k_dump_dir, true);
  } catch {}
}

function row_to_array(row) {
  const result = [];
  for (var i = 0; i < row.length; ++i) {
    result.push(row[i]);
  }
  return result;
}

function fetch(sql) {
  const result = [];
  for (const row of session.runSql(sql).fetchAll()) {
    result.push(row_to_array(row));
  }
  return result;
}

function drop_all_schemas() {
  session.runSql("SET @@session.foreign_key_checks = OFF;");

  for (const schema of fetch("SELECT SCHEMA_NAME FROM INFORMATION_SCHEMA.SCHEMATA WHERE SCHEMA_NAME NOT IN ('mysql', 'sys', 'information_schema', 'performance_schema');")) {
    session.runSql("DROP SCHEMA IF EXISTS !", [ schema[0] ]);
  }

  session.runSql("SET @@session.foreign_key_checks = ON;");
}

function fetch_all_tables() {
  return fetch("select table_schema, table_name from information_schema.tables where table_schema not in ('mysql', 'sys', 'information_schema', 'performance_schema') and table_type = 'BASE TABLE' order by table_schema, table_name");
}

function fetch_all_views() {
  return fetch("select table_schema, table_name from information_schema.tables where table_schema not in ('mysql', 'sys', 'information_schema', 'performance_schema') and table_type = 'VIEW' order by table_schema, table_name");
}

function fetch_all_routines() {
  return fetch("select routine_schema, routine_name from information_schema.routines where routine_schema not in ('sys') order by routine_schema, routine_name");
}

function fetch_all_triggers() {
  return fetch("select trigger_schema, trigger_name from information_schema.triggers where trigger_schema not in ('sys') order by trigger_schema, trigger_name");
}

function fetch_all_events() {
  return fetch("select event_schema, event_name from information_schema.events order by event_schema, event_name");
}

function fetch_all_objects() {
  return [fetch_all_tables(), fetch_all_views(), fetch_all_routines(), fetch_all_triggers(), fetch_all_events()];
}

function checksum(table) {
  const columns = [];
  for (const column of session.runSql("SELECT COLUMN_NAME FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_SCHEMA = ? AND TABLE_NAME = ? ORDER BY ORDINAL_POSITION;", table).fetchAll()) {
    columns.push(column[0]);
  }
  session.runSql("SET @crc = '';");
  session.runSql(`SELECT @crc := MD5(CONCAT_WS(CONVERT('#' USING utf8mb4),@crc,${columns.map(c => '!').join(',')})) FROM !.! ORDER BY CONVERT(! USING utf8mb4) COLLATE utf8mb4_general_ci;`, [...columns, ...table, columns[0]]);
  return session.runSql("SELECT @crc;").fetchOne()[0];
}

function EXPECT_CHECKSUMS(tables) {
  for (const table of tables) {
    EXPECT_EQ(checksums[table], checksum(table), "Checksum of: " + table);
  }
}

//@<> Setup
// prepare dump directory
cleanup_dump_dir();
testutil.mkdir(k_dump_dir);

// prepare test sandbox
testutil.deploySandbox(__mysql_sandbox_port1, "root");
shell.connect(__sandbox_uri1);
session.runSql("SET @@global.local_infile = ON;");

// prepare data to be dumped
shell.connect(__mysql56_uri);

drop_all_schemas();

testutil.importData(__mysql56_uri, os.path.join(__data_path, "sql", "sakila-schema.sql")); // sakila
testutil.importData(__mysql56_uri, os.path.join(__data_path, "sql", "sakila-data.sql"), "sakila"); // sakila
testutil.importData(__mysql56_uri, os.path.join(__data_path, "sql", "misc_features.sql")); // all_features, all_features2
testutil.importData(__mysql56_uri, os.path.join(__data_path, "sql", "fieldtypes_all.sql")); // xtest

const all_objects = fetch_all_objects();
const [all_tables, all_views, all_routines, all_triggers, all_events] = all_objects;

const checksums = {};

for (const table of all_tables) {
  session.runSql("ANALYZE TABLE !.!;", table);
  checksums[table] = checksum(table);
}

// tests
//
// Basic tests to ensure dumpInstance(), dumpSchemas() and dumpTables() work in MySQL 5.6, loadDump() is not expected to work

//@<> dumpInstance should error out unless user dumping is disabled
EXPECT_THROWS(function() { util.dumpInstance(k_dump_dir); }, "Dumping user accounts is currently not supported in MySQL versions before 5.7. Set the 'users' option to false to continue.");

//@<> dumpInstance
// BUG32376447 - use small bytesPerChunk to ensure some tables are chunked, triggering the bug
util.dumpInstance(k_instance_dump, { "users": false, "bytesPerChunk": "128k", "ocimds": true, "compatibility": [ "strip_definers" ] });
EXPECT_STDOUT_CONTAINS("NOTE: Backup lock is not supported in MySQL 5.6 and DDL changes will not be blocked. The dump may fail with an error or not be completely consistent if schema changes are made while dumping.");
// BUG32376447 - check if some tables were chunked
EXPECT_STDOUT_MATCHES(/Data dump for table `\w+`.`\w+` will be written to \d+ files/);
// util.checkForServerUpgrade() does not support 5.6 and it should not be suggested when using the 'ocimds' option
EXPECT_STDOUT_NOT_CONTAINS("checkForServerUpgrade");

//@<> dumpSchemas
util.dumpSchemas(k_all_schemas, k_schemas_dump);

//@<> dumpTables
util.dumpTables("sakila", ["country", "city"], k_tables_dump);

//@<> loading dumps not supported in 5.6
EXPECT_THROWS(function() { util.loadDump(k_instance_dump); }, "Loading dumps is only supported in MySQL 5.7 or newer");

//@<> connect to a newer MySQL server
shell.connect(__sandbox_uri1);

// test if loadDump() is able to load 5.6 dumps into a newer MySQL server

//@<> loadDump tables
drop_all_schemas();

util.loadDump(k_tables_dump, { ignoreVersion: true });

const expected_tables = [ [ "sakila", "city" ], [ "sakila", "country" ] ];
EXPECT_EQ(expected_tables, fetch_all_tables());
EXPECT_CHECKSUMS(expected_tables);

//@<> loadDump schema
drop_all_schemas();

util.loadDump(k_schemas_dump, { ignoreVersion: true });

EXPECT_EQ(all_objects, fetch_all_objects());
EXPECT_CHECKSUMS(all_tables);

//@<> loadDump instance
drop_all_schemas();

util.loadDump(k_instance_dump, { ignoreVersion: true });

EXPECT_EQ(all_objects, fetch_all_objects());
EXPECT_CHECKSUMS(all_tables);

//@<> Cleanup
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
cleanup_dump_dir();
