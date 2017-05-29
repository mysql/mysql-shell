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
mySession.dropTable('js_shell_test', 'table1');
print('Invalid:', table.existsInDatabase());

//@ Testing view check
print('Is View:', table.isView());

// Closes the session
mySession.dropSchema('js_shell_test');
mySession.close();