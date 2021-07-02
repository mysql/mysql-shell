//@{__mysql56_uri != null && VER(>=8.0.0)}
// run only vs mysql 8.0, in part because paratrace doesn't support multiple instances of 5.6

// constants
const k_dump_dir = os.path.join(__tmp_dir, "ldtest");
const k_users_dump = os.path.join(k_dump_dir, "dumpu");
const k_instance_dump = os.path.join(k_dump_dir, "dumpi");
const k_schemas_dump = os.path.join(k_dump_dir, "dumps");
const k_tables_dump = os.path.join(k_dump_dir, "dumpt");
const k_all_schemas = [ "sakila", "all_features", "all_features2", "xtest", "test" ];

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

// BUG#32925914 - create an InnoDB table that will use fixed row format
session.runSql("CREATE SCHEMA test;");
session.runSql("CREATE TABLE test.fixed_row_format (a int) ENGINE=InnoDB;");

const all_objects = fetch_all_objects();
const [all_tables, all_views, all_routines, all_triggers, all_events] = all_objects;

const checksums = {};

for (const table of all_tables) {
  session.runSql("ANALYZE TABLE !.!;", table);
  checksums[table] = checksum(table);
}

// prepare accounts
// remove any anonymous accounts
for (const row of session.runSql("SELECT Host FROM mysql.user WHERE User = ''").fetchAll()) {
  session.runSql("DROP USER ''@?;", [ row[0] ]);
}

// remove any leftovers
function drop_user_if_exists(user, host = '%') {
  if (session.runSql("SELECT User FROM mysql.user WHERE User = ? AND Host = ?", [ user, host ]).fetchOne()) {
    session.runSql("DROP USER ?@?;", [ user, host ]);
  }
}

drop_user_if_exists("almost_superuser");
drop_user_if_exists("regular_user");

// account with all privileges minus SUPER
session.runSql("CREATE USER almost_superuser IDENTIFIED BY 'pwd';");
session.runSql("GRANT ALL PRIVILEGES ON *.* TO almost_superuser REQUIRE NONE WITH MAX_USER_CONNECTIONS 100 GRANT OPTION;");
session.runSql("REVOKE SUPER ON *.* FROM almost_superuser;");

// URI to connect as this user
let co = shell.parseUri(__mysql56_uri);
co.user = "almost_superuser";
co.password = "pwd";
const almost_superuser_uri = shell.unparseUri(co);

// account with some random privileges
session.runSql("CREATE USER regular_user IDENTIFIED BY 'pwdpwd';");
session.runSql("GRANT SELECT ON *.* TO regular_user;");
session.runSql("GRANT INSERT ON sakila.* TO regular_user REQUIRE SSL WITH MAX_UPDATES_PER_HOUR 5;");
session.runSql("GRANT update ON `sakila`.`actor` TO regular_user WITH GRANT OPTION;");

// make sure changes are provisioned
session.runSql("FLUSH PRIVILEGES;");

// tests
//
// Basic tests to ensure dumpInstance(), dumpSchemas() and dumpTables() work in MySQL 5.6, loadDump() is not expected to work

//@<> BUG#32883314 - SUPER is required to dump users
shell.connect(almost_superuser_uri);
EXPECT_THROWS(function() { util.dumpInstance(k_users_dump); }, "Util.dumpInstance: User 'almost_superuser'@'%' is missing the following global privilege(s): SUPER.");
shell.connect(__mysql56_uri);

//@<> BUG#32883314 - dumpInstance should be able to dump users
EXPECT_NO_THROWS(function() { util.dumpInstance(k_users_dump); }, "It should be possible to dump user information.");

//@<> BUG#32925914 set ROW_FORMAT=FIXED
// a warning will be reported saying:
//   Warning (code 1478): InnoDB: assuming ROW_FORMAT=COMPACT.
// but SHOW CREATE TABLE will still output FIXED
session.runSql("ALTER TABLE test.fixed_row_format ROW_FORMAT=FIXED;");

//@<> dumpInstance
// BUG32376447 - use small bytesPerChunk to ensure some tables are chunked, triggering the bug
WIPE_SHELL_LOG();
util.dumpInstance(k_instance_dump, { "users": false, "bytesPerChunk": "128k", "ocimds": true, "compatibility": [ "ignore_missing_pks", "strip_definers" ] });
EXPECT_STDOUT_CONTAINS("NOTE: Backup lock is not supported in MySQL 5.6 and DDL changes will not be blocked. The dump may fail with an error or not be completely consistent if schema changes are made while dumping.");
// BUG32376447 - check if some tables were chunked
EXPECT_SHELL_LOG_MATCHES(/Data dump for table `\w+`.`\w+` will be written to \d+ files/);
// util.checkForServerUpgrade() does not support 5.6 and it should not be suggested when using the 'ocimds' option
EXPECT_STDOUT_NOT_CONTAINS("checkForServerUpgrade");
// BUG#32925914 - FIXED row format will be removed even though 'force_innodb' is not specified, because table uses InnoDB engine
EXPECT_STDOUT_CONTAINS("NOTE: Table 'test'.'fixed_row_format' had unsupported ROW_FORMAT=FIXED option removed")

//@<> BUG#32925914 remove ROW_FORMAT=FIXED, so the remaining tests can work without ocimds/force_innodb options
session.runSql("ALTER TABLE test.fixed_row_format ROW_FORMAT=DEFAULT;");

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

//@<> BUG32376447 - loadDump instance with users
drop_all_schemas();

util.loadDump(k_users_dump, { loadUsers: true, ignoreVersion: true });

EXPECT_STDOUT_CONTAINS("Executing user accounts SQL...");
EXPECT_EQ(all_objects, fetch_all_objects());
EXPECT_CHECKSUMS(all_tables);

//@<> Cleanup
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
cleanup_dump_dir();
