// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<mysql_port>
var mysql = require('mysql').mysql;

var mySession = mysql.getClassicSession(__uripwd);

ensure_schema_does_not_exist(mySession, 'js_shell_test');

var schema = mySession.createSchema('js_shell_test');
mySession.setCurrentSchema('js_shell_test');

var result = mySession.runSql('create table table1 (name varchar(50))');
var table = mySession.js_shell_test.getTable('table1');

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
print('Valid:', table.existInDatabase());
mySession.dropTable('js_shell_test', 'table1');
print('Invalid:', table.existInDatabase());

// Closes the session
mySession.dropSchema('js_shell_test');
mySession.close();