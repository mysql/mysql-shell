# Assumptions: ensure_schema_does_not_exist is available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
import mysqlx

mySession = mysqlx.getNodeSession(__uripwd)

ensure_schema_does_not_exist(mySession, 'js_shell_test')

schema = mySession.createSchema('js_shell_test')
mySession.setCurrentSchema('js_shell_test')

result = mySession.sql('create table table1 (name varchar(50))').execute()
result = mySession.sql('create view view1 (my_name) as select name from table1').execute()
view = mySession.getSchema('js_shell_test').getTable('view1')

#@ Testing view name retrieving
print 'getName(): ' + view.getName()
print 'name: ' + view.name


#@ Testing session retrieving
print 'getSession():', view.getSession()
print 'session:', view.session

#@ Testing view schema retrieving
print 'getSchema():', view.getSchema()
print 'schema:', view.schema


#@ Testing existence
print 'Valid:', view.existsInDatabase()
mySession.dropView('js_shell_test', 'view1')
print 'Invalid:', view.existsInDatabase()

#@ Testing view check
print 'Is View:', view.isView()

# Closes the session
mySession.dropSchema('js_shell_test')
mySession.close()