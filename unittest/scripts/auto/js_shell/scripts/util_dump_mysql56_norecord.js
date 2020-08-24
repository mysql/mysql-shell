//@{__mysql56_uri != null && VER(>=8.0.0)}
// run only vs mysql 8.0, in part because paratrace doesn't support multiple instances of 5.6

// Basic tests to ensure dumpInstance, dumpSchemas and dumpTables work in MySQL 5.6
// loadDump() is not expected to work

//@<> Setup
try {testutil.rmdir(__tmp_dir+"/ldtest", true);} catch {}
testutil.mkdir(__tmp_dir+"/ldtest");

shell.connect(__mysql56_uri);

session.runSql("drop schema if exists sakila");

testutil.importData(__mysql56_uri, __data_path+"/sql/sakila-schema.sql"); // sakila
testutil.importData(__mysql56_uri, __data_path+"/sql/sakila-data.sql", "sakila"); // sakila
testutil.importData(__mysql56_uri, __data_path+"/sql/misc_features.sql"); // all_features, all_features2

var all_tables = session.runSql("select table_schema, table_name from information_schema.tables where table_schema not in ('mysql', 'sys', 'information_schema', 'performance_schema') order by table_schema, table_name").fetchAll();
var all_routines = session.runSql("select routine_schema, routine_name from information_schema.routines order by routine_schema, routine_name").fetchAll();
var all_triggers = session.runSql("select trigger_schema, trigger_name from information_schema.triggers order by trigger_schema, trigger_name").fetchAll();
var all_events = session.runSql("select event_schema, event_name from information_schema.events order by event_schema, event_name").fetchAll();

//@<> dumpInstance should error out unless user dumping is disabled
EXPECT_THROWS(function(){util.dumpInstance(__tmp_dir+"/ldtest")}, "Dumping user accounts is currently not supported in MySQL versions before 5.7. Set the 'users' option to false to continue.");

//@<> dumpInstance
util.dumpInstance(__tmp_dir+"/ldtest/dumpi", {users:false});

//@<> dumpSchemas
util.dumpSchemas(["sakila", "all_features", "all_features2"], __tmp_dir+"/ldtest/dumps");

//@<> dumpTables
util.dumpTables("sakila", ["country", "city"], __tmp_dir+"/ldtest/dumpt");

//@<> loading dumps not supported in 5.6
EXPECT_THROWS(function(){util.loadDump(__tmp_dir+"/ldtest/dumpi");}, "Loading dumps is only supported in MySQL 5.7 or newer");

//@ loadDump tables in newer MySQL version
testutil.deploySandbox(__mysql_sandbox_port1, "root");
shell.connect(__sandbox_uri1);

session.runSql("set global local_infile=1");

session.runSql("create schema sakila");
util.loadDump(__tmp_dir+"/ldtest/dumpt", {schema:"sakila", ignoreVersion:1});
session.runSql("select table_schema, table_name from information_schema.tables where table_schema not in ('mysql', 'sys', 'information_schema', 'performance_schema') order by table_schema, table_name");

//@ loadDump schema
session.runSql("drop schema sakila");
util.loadDump(__tmp_dir+"/ldtest/dumps", {ignoreVersion:1});

EXPECT_EQ(all_tables, session.runSql("select table_schema, table_name from information_schema.tables where table_schema not in ('mysql', 'sys', 'information_schema', 'performance_schema') order by table_schema, table_name").fetchAll());
EXPECT_EQ(all_routines, session.runSql("select routine_schema, routine_name from information_schema.routines where routine_schema not in ('sys') order by routine_schema, routine_name").fetchAll());
EXPECT_EQ(all_triggers, session.runSql("select trigger_schema, trigger_name from information_schema.triggers where trigger_schema not in ('sys') order by trigger_schema, trigger_name").fetchAll());
EXPECT_EQ(all_events, session.runSql("select event_schema, event_name from information_schema.events order by event_schema, event_name").fetchAll());

session.runSql("select * from sakila.country limit 10")


//@ loadDump instance
session.runSql("set foreign_key_checks=0");
session.runSql("drop schema sakila");
session.runSql("drop schema all_features");
session.runSql("drop schema all_features2");
session.runSql("set foreign_key_checks=1");

util.loadDump(__tmp_dir+"/ldtest/dumpi", {ignoreVersion:1});

EXPECT_EQ(all_tables, session.runSql("select table_schema, table_name from information_schema.tables where table_schema not in ('mysql', 'sys', 'information_schema', 'performance_schema') order by table_schema, table_name").fetchAll());
EXPECT_EQ(all_routines, session.runSql("select routine_schema, routine_name from information_schema.routines where routine_schema not in ('sys') order by routine_schema, routine_name").fetchAll());
EXPECT_EQ(all_triggers, session.runSql("select trigger_schema, trigger_name from information_schema.triggers where trigger_schema not in ('sys') order by trigger_schema, trigger_name").fetchAll());
EXPECT_EQ(all_events, session.runSql("select event_schema, event_name from information_schema.events order by event_schema, event_name").fetchAll());

session.runSql("select * from sakila.country limit 10")


//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
try {testutil.rmdir(__tmp_dir+"/ldtest", true);} catch {}
