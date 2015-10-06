// Assumptions: ensure_schema_does_not_exist is available
var mysqlx = require('mysqlx').mysqlx;

var uri = os.getenv('MYSQL_URI');

var mySession = mysqlx.getNodeSession(uri);

ensure_schema_does_not_exist(mySession, 'js_shell_test');

schema = mySession.createSchema('js_shell_test');
mySession.setCurrentSchema('js_shell_test');

var result;
result = mySession.sql('create table table1 (name varchar(50))').execute();
result = mySession.sql('create view view1 (my_name) as select name from table1;').execute();
var view = mySession.js_shell_test.getView('view1');

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
mySession.dropView('js_shell_test', 'view1');
print('Invalid:', view.existInDatabase());

// Closes the session
mySession.dropSchema('js_shell_test');
mySession.close();