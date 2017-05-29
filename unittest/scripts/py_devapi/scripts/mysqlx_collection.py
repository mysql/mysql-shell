# Assumptions: validate_crud_functions available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
from mysqlsh import mysqlx

mySession = mysqlx.get_session(__uripwd)

ensure_schema_does_not_exist(mySession, 'js_shell_test')

schema = mySession.create_schema('js_shell_test')

# Creates a test collection and inserts data into it
collection = schema.create_collection('collection1')


#@ Testing collection name retrieving
print 'get_name(): ' + collection.get_name()
print 'name: ' + collection.name


#@ Testing session retrieving
print 'get_session():', collection.get_session()
print 'session:', collection.session

#@ Testing collection schema retrieving
print 'get_schema():', collection.get_schema()
print 'schema:', collection.schema

#@ Testing existence
print 'Valid:', collection.exists_in_database()
mySession.drop_collection('js_shell_test', 'collection1')
print 'Invalid:', collection.exists_in_database()

# Closes the session
mySession.drop_schema('js_shell_test')
mySession.close()
