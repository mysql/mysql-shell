var mysqlx = require('mysqlx').mysqlx;

var uri = os.getenv('MYSQL_URI');

var session = mysqlx.getNodeSession(uri);

var schema;
try{
	// Ensures the js_shell_test does not exist
	schema = session.getSchema('js_shell_test');
	schema.drop();
}
catch(err)
{
}

schema = session.createSchema('js_shell_test');
session.setCurrentSchema('js_shell_test');

var result = session.sql('create table table1 (name varchar(50))').execute();
var table = session.js_shell_test.getTable('table1');

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
table.drop();
print('Invalid:', table.existInDatabase());

//@ Closes the session
schema.drop();
session.close();