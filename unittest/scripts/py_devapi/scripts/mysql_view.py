# Assumptions: ensure_schema_does_not_exist is available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<mysql_port>
import mysql

mySession = mysql.getClassicSession(__uripwd)

ensure_schema_does_not_exist(mySession, 'js_shell_test')

schema = mySession.createSchema('js_shell_test')
mySession.setCurrentSchema('js_shell_test')

result
result = mySession.runSql('create table table1 (name varchar(50))')
result = mySession.runSql('create view view1 (my_name) as select name from table1')
view = mySession.js_shell_test.getView('view1')

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

# Closes the session
mySession.dropSchema('js_shell_test')
mySession.close()