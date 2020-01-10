// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
var mysqlx = require('mysqlx');

var mySession = mysqlx.getSession(__uripwd);

ensure_schema_does_not_exist(mySession, 'js_shell_test');

schema = mySession.createSchema('js_shell_test');
mySession.setCurrentSchema('js_shell_test');

var result;
result = mySession.sql('create table table1 (name varchar(50))').execute();
result = mySession.sql('insert into table1 (name) values ("John Doe")').execute();
result = mySession.sql('create view view1 (my_name) as select name from table1;').execute();
var view = mySession.getSchema('js_shell_test').getTable('view1');

//@ Testing view name retrieving
print('getName(): ' + view.getName());
print('name: ' + view.name);

//@ Testing session retrieving
print('getSession():', view.getSession());
print('session:', view.session);

//@ Testing view schema retrieving
print('getSchema():', view.getSchema());
print('schema:', view.schema);

//@ Testing view select
shell.dumpRows(view.select().execute(), "vertical");

//@ Testing view update
testutil.wipeAllOutput();
view.update().set('my_name', 'John Lock').execute();
shell.dumpRows(view.select().execute(), "vertical");

//@ Testing view insert
testutil.wipeAllOutput();
view.insert().values('Jane Doe').execute();
shell.dumpRows(view.select().execute(), "vertical");

//@ Testing view delete
testutil.wipeAllOutput();
view.delete().execute();

//@ Testing existence
testutil.wipeAllOutput();
print('Valid:', view.existsInDatabase());
mySession.sql('drop view view1').execute();
print('Invalid:', view.existsInDatabase());

//@ Testing view check
print('Is View:', view.isView());

// Closes the session
mySession.dropSchema('js_shell_test');
mySession.close();
