# Assumptions: ensure_schema_does_not_exist is available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
from mysqlsh import mysqlx

mySession = mysqlx.get_session(__uripwd)

ensure_schema_does_not_exist(mySession, 'js_shell_test')

schema = mySession.create_schema('js_shell_test')
mySession.set_current_schema('js_shell_test')

result = mySession.sql('create table table1 (name varchar(50))').execute()
table = mySession.get_schema('js_shell_test').get_table('table1')

#@ Testing table name retrieving
print 'get_name(): ' + table.get_name()
print 'name: ' + table.name


#@ Testing session retrieving
print 'get_session():', table.get_session()
print 'session:', table.session

#@ Testing table schema retrieving
print 'get_schema():', table.get_schema()
print 'schema:', table.schema


#@ Testing existence
print 'Valid:', table.exists_in_database()
schema.drop_table('table1')
print 'Invalid:', table.exists_in_database()

#@ Testing view check
print 'Is View:', table.is_view()

# Closes the session
mySession.drop_schema('js_shell_test')
mySession.close()
