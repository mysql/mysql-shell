# Assumptions: ensure_schema_does_not_exist available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<mysql_port>
import mysql

mySession = mysql.getClassicSession(__uripwd)

ensure_schema_does_not_exist(mySession, 'js_shell_test')

schema = mySession.createSchema('js_shell_test')
mySession.setCurrentSchema('js_shell_test')

result = mySession.runSql('create table table1 (name varchar(50))')
result = mySession.runSql('create view view1 (my_name) as select name from table1')

#@ Testing schema name retrieving
print 'getName(): ' + schema.getName()
print 'name: ' + schema.name

#@ Testing schema.getSession
print 'getSession():',schema.getSession()

#@ Testing schema.session
print 'session:', schema.session

#@ Testing schema schema retrieving
print 'getSchema():', schema.getSchema()
print 'schema:', schema.schema

#@ Testing tables, views and collection retrieval
print 'getTables():', mySession.js_shell_test.getTables().table1
print 'tables:', mySession.js_shell_test.tables.table1
print 'getViews():', mySession.js_shell_test.getViews().view1
print 'views:', mySession.js_shell_test.views.view1

#@ Testing specific object retrieval
print 'getTable():', mySession.js_shell_test.getTable('table1')
print '.<table>:', mySession.js_shell_test.table1
print 'getView():', mySession.js_shell_test.getView('view1')
print '.<view>:', mySession.js_shell_test.view1

#@ Testing existence
print 'Valid:', schema.existsInDatabase()
mySession.dropSchema('js_shell_test')
print 'Invalid:', schema.existsInDatabase()

# Closes the session
mySession.close()