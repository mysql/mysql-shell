# Assumptions: ensure_schema_does_not_exist available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
from mysqlsh import mysqlx

mySession = mysqlx.get_node_session(__uripwd)

ensure_schema_does_not_exist(mySession, 'js_shell_test')

mySession.create_schema('js_shell_test')
mySession.set_current_schema('js_shell_test')

mySession.sql('create table table1 (name varchar(50))').execute()
mySession.sql('create view view1 (my_name) as select name from table1').execute()
mySession.get_schema('js_shell_test').create_collection('collection1')

schema = mySession.get_schema('js_shell_test');

#@ Schema: validating members
all_members = dir(schema)

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
validateMember(members, 'get_table')
validateMember(members, 'get_tables')
validateMember(members, 'get_collection')
validateMember(members, 'get_collections')
validateMember(members, 'create_collection')
validateMember(members, 'get_collection_as_table')
validateMember(members, 'help')

# Dynamic Properties
validateMember(members, 'table1')
validateMember(members, 'view1')
validateMember(members, 'collection1')

#@ Testing schema name retrieving
print 'get_name(): ' + schema.get_name()
print 'name: ' + schema.name

#@ Testing schema.get_session
print 'get_session():',schema.get_session()

#@ Testing schema.session
print 'session:', schema.session

#@ Testing schema schema retrieving
print 'get_schema():', schema.get_schema()
print 'schema:', schema.schema

#@ Testing tables, views and collection retrieval
mySchema = mySession.get_schema('js_shell_test')
print 'get_tables():', mySchema.get_tables()[0]
print 'get_collections():', mySchema.get_collections()[0]

#@ Testing specific object retrieval
print 'Retrieving a Table:', mySchema.get_table('table1')
print '.<table>:', mySchema.table1
print 'Retrieving a View:', mySchema.get_table('view1')
print '.<view>:', mySchema.view1
print 'get_collection():', mySchema.get_collection('collection1')
print '.<collection>:', mySchema.collection1

#@# Testing specific object retrieval: unexisting objects
mySchema.get_table('unexisting')
mySchema.get_collection('unexisting')

#@# Testing specific object retrieval: empty name
mySchema.get_table('')
mySchema.get_collection('')

#@ Retrieving collection as table
print 'get_collection_as_table():', mySchema.get_collection_as_table('collection1')

#@ Collection creation
collection = schema.create_collection('my_sample_collection')
print 'create_collection():', collection

#@ Testing existence
print 'Valid:', schema.exists_in_database()
mySession.drop_schema('js_shell_test')
print 'Invalid:', schema.exists_in_database()

# Closes the session
mySession.close()
