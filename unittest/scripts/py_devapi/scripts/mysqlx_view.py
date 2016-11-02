# Assumptions: ensure_schema_does_not_exist is available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
from mysqlsh import mysqlx

mySession = mysqlx.get_node_session(__uripwd)

ensure_schema_does_not_exist(mySession, 'js_shell_test')

schema = mySession.create_schema('js_shell_test')
mySession.set_current_schema('js_shell_test')

result = mySession.sql('create table table1 (name varchar(50))').execute()
result = mySession.sql('create view view1 (my_name) as select name from table1').execute()
view = mySession.get_schema('js_shell_test').get_table('view1')

#@ Testing view name retrieving
print 'get_name(): ' + view.get_name()
print 'name: ' + view.name


#@ Testing session retrieving
print 'get_session():', view.get_session()
print 'session:', view.session

#@ Testing view schema retrieving
print 'get_schema():', view.get_schema()
print 'schema:', view.schema


#@ Testing existence
print 'Valid:', view.exists_in_database()
mySession.drop_view('js_shell_test', 'view1')
print 'Invalid:', view.exists_in_database()

#@ Testing view check
print 'Is View:', view.is_view()

# Closes the session
mySession.drop_schema('js_shell_test')
mySession.close()