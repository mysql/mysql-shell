//@<> MY-225 X SHELL RETURNS UNDEFINED RESULT FOR SUM() AND AVG() OVER MYSQLX PROTOCOL
shell.connect(__uripwd);
session.dropSchema('MYS_225');
var schema = session.createSchema('MYS_225');
session.runSql('CREATE TABLE MYS_225.sample (Value INT NOT NULL, valuedecimal FLOAT NOT NULL)');
var table = schema.getTable('sample');
table.insert().values(1,1.1).values(2,2.2).values(3,3.3).values(4,4.4).values(5,5.5).values(6,6.6).values(7,7.7).values(8,8.8).values(9,9.9).values(10,10.10);

shell.dumpRows(table.select('sum(value)', 'sum(valuedecimal)').execute(), 'vertical');
EXPECT_OUTPUT_CONTAINS('sum(`value`): 55');
EXPECT_OUTPUT_CONTAINS('sum(`valuedecimal`): 59.60000002384186');

shell.dumpRows(table.select('avg(value)', 'avg(valuedecimal)').execute(), 'vertical');
EXPECT_OUTPUT_CONTAINS('avg(`value`): 5.5000');
EXPECT_OUTPUT_CONTAINS('avg(`valuedecimal`): 5.960000002384186');
testutil.wipeAllOutput();

// Repeats the test in using a classic session
shell.connect(__mysqluripwd);
shell.dumpRows(session.runSql('select sum(value), sum(valuedecimal) FROM MYS_225.sample'), 'vertical');
EXPECT_OUTPUT_CONTAINS('sum(value): 55');
EXPECT_OUTPUT_CONTAINS('sum(valuedecimal): 59.60000002384186');

shell.dumpRows(session.runSql('select avg(value), avg(valuedecimal) FROM MYS_225.sample'), 'vertical');
EXPECT_OUTPUT_CONTAINS('avg(value): 5.5000');
EXPECT_OUTPUT_CONTAINS('avg(valuedecimal): 5.960000002384186');
testutil.wipeAllOutput();

session.runSql('DROP SCHEMA MYS_225')
session.close();

//@<> MY-286 TYPE DATETIME RETURNS BOOLEAN INSTEAD OF DATE
shell.connect(__uripwd);
var schema = session.dropSchema('MYS_286');
var schema = session.createSchema('MYS_286');
session.runSql('CREATE TABLE MYS_286.sample (date datetime)');
var table = schema.getTable('sample');
table.insert().values('2016-03-14 12:36:37');

shell.dumpRows(table.select().execute(), 'vertical');
EXPECT_OUTPUT_CONTAINS('2016-03-14 12:36:37');
testutil.wipeAllOutput();

// Repeats the test in using a classic session
shell.connect(__mysqluripwd);
shell.dumpRows(session.runSql('select * from MYS_286.sample'), 'vertical');
EXPECT_OUTPUT_CONTAINS('2016-03-14 12:36:37');

session.runSql('DROP SCHEMA MYS_286')
session.close();

//@<> MY-290 JS File with errors generate a ? response with batch mode
testutil.callMysqlsh([__uripwd, "--js", "--file", __test_data_path + "js/js_err.js"]);
if (testutil.versionCheck(__version, ">=", "8.0.0")) {
    EXPECT_OUTPUT_CONTAINS("Session.runSql: Unknown database 'unexisting'");
} else {
    EXPECT_OUTPUT_CONTAINS("Session.runSql: Table 'unexisting.whatever' doesn't exist");
}
testutil.wipeAllOutput();

testutil.callMysqlsh([__uripwd, "--interactive=full", "--js", "--file", __test_data_path + "js/js_err.js"]);
if (testutil.versionCheck(__version, ">=", "8.0.0")) {
    EXPECT_OUTPUT_CONTAINS("Session.runSql: Unknown database 'unexisting'");
} else {
    EXPECT_OUTPUT_CONTAINS("Session.runSql: Table 'unexisting.whatever' doesn't exist");
}
testutil.wipeAllOutput();

//@<> MY-309 session.createSchema() does not accept schemaName with dashes ("-")
shell.connect(__uripwd);
EXPECT_NO_THROWS(function() {session.createSchema('MYS-309')});
EXPECT_NO_THROWS(function() {session.dropSchema('MYS-309')});
session.close();

//@<> MY-348 In a world prior to our time - SQL DATE handling broken - wrong results
shell.connect(__uripwd);
var schema = session.dropSchema('MYS_348');
var schema = session.createSchema('MYS_348');
session.runSql('CREATE TABLE MYS_348.sample (my_column date)');
var table = schema.getTable('sample');
table.insert().values('1000-02-01 00:00:00');

var row = table.select().execute().fetchOne();

EXPECT_EQ(1000, row.my_column.getFullYear());
testutil.wipeAllOutput();

session.runSql('DROP SCHEMA MYS_348')
session.close();


//@<> MY-438 TRUE OR FALSE NOT RECOGNIZED AS AVAILABLE BOOL CONSTANTS
shell.connect(__uripwd);
var schema = session.dropSchema('MYS_438');
var schema = session.createSchema('MYS_438');
session.runSql('CREATE TABLE MYS_438.sample (my_column bool)');
var table = schema.getTable('sample');
table.insert().values(true).values(false);

shell.dumpRows(table.select().execute(), 'vertical');
EXPECT_OUTPUT_CONTAINS('my_column: 1');
EXPECT_OUTPUT_CONTAINS('my_column: 0');
testutil.wipeAllOutput();
session.runSql('DROP SCHEMA MYS_438')
session.close();


//@<> MY-492 MYSQLSH SHOWS TIME COLUMNS AS BOOLEAN VALUES
shell.connect(__uripwd);
var schema = session.dropSchema('MYS_492');
var schema = session.createSchema('MYS_492');
session.runSql('CREATE TABLE MYS_492.sample (my_column time)');
var table = schema.getTable('sample');
table.insert().values('10:05:30');

shell.dumpRows(table.select().execute(), 'vertical');
EXPECT_OUTPUT_CONTAINS('my_column: 10:05:30');
testutil.wipeAllOutput();
session.runSql('DROP SCHEMA MYS_492')
session.close();


//@ MY-495 STATUS BEHAVES DIFFERENTLY WHEN CONNECTION IS LOST
shell.connect(__uripwd);
\status
EXPECT_OUTPUT_CONTAINS("MySQL Shell version");
EXPECT_THROWS(function() {session.runSql('kill connection_id()')}, "Session.runSql: Query execution was interrupted");

\status
EXPECT_OUTPUT_CONTAINS("MySQL Shell version");

session.close();


//@<> MY-496 MySQL Shell prints Undefined on JSON column (Classic Session)
shell.connect(__uripwd);
var schema = session.dropSchema('MYS_496');
var schema = session.createSchema('MYS_496');
var collection = schema.createCollection('sample');
collection.add({_id:1, name:'John Doe'});

shell.connect(__mysqluripwd);
shell.dumpRows(session.runSql("select * from MYS_496.sample"));
if (testutil.versionCheck(__version, ">=", "8.0.0")) {
    EXPECT_OUTPUT_CONTAINS('| {"_id": 1, "name": "John Doe"} | 1   | {"type": "object"} |');
} else {
    EXPECT_OUTPUT_CONTAINS('| {"_id": 1, "name": "John Doe"} | 1   |');
}
session.runSql('DROP SCHEMA MYS_496')
session.close();


//@<> MY-518 Make it easier to use the session data on shell prompt
// Testing Global X Session
shell.connect(__uripwd);
EXPECT_EQ(true, session.isOpen());
session.close();
EXPECT_EQ(false, session.isOpen());

// Testing Global Classic Session
shell.connect(__mysqluripwd);
EXPECT_EQ(true, session.isOpen());
session.close();
EXPECT_EQ(false, session.isOpen());

// Testing API X Session
var xsession = mysqlx.getSession(__uripwd);
EXPECT_EQ(true, xsession.isOpen());
xsession.close();
EXPECT_EQ(false, xsession.isOpen());

// Testing API Classic Session
var csession = mysql.getSession(__mysqluripwd);
EXPECT_EQ(true, csession.isOpen());
csession.close();
EXPECT_EQ(false, csession.isOpen());

// Testing parseUri
EXPECT_EQ({"host": "localhost", "password": "password", "port": 3307, "schema": "sample", "user": "root"}, shell.parseUri('root:password@localhost:3307/sample'));

// Testing parseUri with sockets
EXPECT_EQ({"password": "password", "schema": "sample", "socket": "/my/socket/file", "user": "root"}, shell.parseUri('root:password@(/my/socket/file)/sample'));


//@<> MY-583 URI PARSING DOES NOT DECODE PCT BEFORE PASSING TO OTHER SYSTEMS
shell.connect(__uripwd);
session.runSql("drop user if exists sample_pct@localhost");
session.runSql("create user sample_pct@localhost identified by 'pwd'");
session.runSql("grant all on *.* to sample_pct@localhost");
session.close()
shell.connect('sample%5f%70%63%74:%70%77%64@localhost:' + __port);
EXPECT_OUTPUT_CONTAINS("Creating a session to 'sample_pct@localhost:" + __port + "'");

shell.dumpRows(session.runSql('show databases'), 'vertical')
EXPECT_OUTPUT_CONTAINS('Database: mysql');

session.close();
shell.connect(__uripwd);
session.runSql("drop user sample_pct@localhost");
session.close();

//@<> MY-697 UNEXPECTED BEHAVIOR WHILE EXECUTING SQL STATEMENTS CONTAINING COMMENTS
shell.connect(__uripwd);
shell.dumpRows(session.runSql('select 1 as /*This is an inline comment*/my_value'), 'vertical')
EXPECT_OUTPUT_CONTAINS('my_value: 1');
session.close();
