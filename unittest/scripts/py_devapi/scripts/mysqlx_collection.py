# Assumptions: validate_crud_functions available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
from mysqlsh import mysqlx

mySession = mysqlx.get_session(__uripwd)

ensure_schema_does_not_exist(mySession, 'js_shell_test')

schema = mySession.create_schema('js_shell_test')

# Creates a test collection and inserts data into it
collection = schema.create_collection('collection1')

#@<OUT> Testing collection help
collection.help()

#@ Validating members
all_members = dir(collection)

# Remove the python built in members
members = []
for member in all_members:
  if not member.startswith('__'):
    members.append(member)

print "Member Count: %s" % len(members)

validateMember(members, 'name')
validateMember(members, 'schema')
validateMember(members, 'session')
validateMember(members, 'exists_in_database')
validateMember(members, 'get_name')
validateMember(members, 'get_schema')
validateMember(members, 'get_session')
validateMember(members, 'add')
validateMember(members, 'create_index')
validateMember(members, 'drop_index')
validateMember(members, 'modify')
validateMember(members, 'remove')
validateMember(members, 'find')
validateMember(members, 'help')

#@ Testing collection name retrieving
print 'get_name(): ' + collection.get_name()
print 'name: ' + collection.name


#@ Testing session retrieving
print 'get_session():', collection.get_session()
print 'session:', collection.session

#@ Testing collection schema retrieving
print 'get_schema():', collection.get_schema()
print 'schema:', collection.schema

#@<OUT> Testing help of drop_index
collection.help("drop_index")

#@ Testing dropping index
collection.create_index('_name').field('name', "TEXT(50)", True).execute()
print collection.drop_index('_name')
print collection.drop_index('_name')
print collection.drop_index('not_an_index')

#@ Testing dropping index using execute
collection.drop_index('_name').execute()

#@ Testing existence
print 'Valid:', collection.exists_in_database()
mySession.get_schema('js_shell_test').drop_collection('collection1')
print 'Invalid:', collection.exists_in_database()

# Closes the session
mySession.drop_schema('js_shell_test')
mySession.close()
