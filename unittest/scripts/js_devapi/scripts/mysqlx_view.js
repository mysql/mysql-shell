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

var result;
result = session.sql('create table table1 (name varchar(50))').execute();
result = session.sql('create view view1 (my_name) as select name from table1;').execute();
var view = session.js_shell_test.getView('view1');

//@ Testing view name retrieving
print('getName(): ' + view.getName());
print('name: ' + view.name);


//@ Testing session retrieving
print('getSession():', view.getSession());
print('session:', view.session);

//@ Testing view schema retrieving
print('getSchema():', view.getSchema());
print('schema:', view.schema);


//@ Testing existence
print('Valid:', view.existInDatabase());
view.drop();
print('Invalid:', view.existInDatabase());

//@ Closes the session
schema.drop();
session.close();