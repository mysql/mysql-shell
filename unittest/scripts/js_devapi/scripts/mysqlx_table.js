// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
var mysqlx = require('mysqlx');

var mySession = mysqlx.getSession(__uripwd);

ensure_schema_does_not_exist(mySession, 'js_shell_test');

var schema = mySession.createSchema('js_shell_test');
mySession.setCurrentSchema('js_shell_test');

var result = mySession.sql('create table table1 (name varchar(50))').execute();
var table = mySession.getSchema('js_shell_test').getTable('table1');

//@ Testing table name retrieving
print('getName(): ' + table.getName());
print('name: ' + table.name);

//@ Testing session retrieving
print('getSession():', table.getSession());
print('session:', table.session);

//@ Testing table schema retrieving
print('getSchema():', table.getSchema());
print('schema:', table.schema);

//@ Testing existence
print('Valid:', table.existsInDatabase());
mySession.sql('drop table table1').execute();
print('Invalid:', table.existsInDatabase());

//@ Testing view check
print('Is View:', table.isView());

//@<> WL12412: Initialize Count Tests
var result = mySession.sql('create table table_count (name varchar(50))').execute();
var table = schema.getTable('table_count');

//@ WL12412-TS1_1: Count takes no arguments
table.count(1);

//@ WL12412-TS1_3: Count returns correct number of records
var count = table.count();
println ("Initial Row Count: " + count);

table.insert().values("First");
table.insert().values("Second");
table.insert().values("Third");

var count = table.count();
println ("Final Row Count: " + count);

//@ WL12412-TS2_2: Count throws error on unexisting table
mySession.sql('drop table js_shell_test.table_count');
var count = table.count();

// Closes the session
mySession.dropSchema('js_shell_test');
mySession.close();
