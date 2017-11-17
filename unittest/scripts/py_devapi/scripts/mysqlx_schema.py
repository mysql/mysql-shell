# Assumptions: ensure_schema_does_not_exist available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
from mysqlsh import mysqlx

mySession = mysqlx.get_session(__uripwd)

ensure_schema_does_not_exist(mySession, 'js_shell_test')

mySession.create_schema('js_shell_test')
mySession.set_current_schema('js_shell_test')

mySession.sql('create table table1 (name varchar(50))').execute()
mySession.sql('create view view1 (my_name) as select name from table1').execute()
mySession.get_schema('js_shell_test').create_collection('collection1')

schema = mySession.get_schema('js_shell_test');

# We need to know the lower_case_table_names option to
# properly handle the table shadowing unit tests
lcresult = mySession.sql('select @@lower_case_table_names').execute()
lcrow = lcresult.fetch_one()
lower_case_table_names = lcrow[0]
if lower_case_table_names == 1:
    name_get_table="gettable"
    name_get_collection="getcollection"
else:
    name_get_table="getTable"
    name_get_collection="getCollection"

#@<OUT> Schema: help
schema.help()

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
validateMember(members, 'drop_collection')
validateNotMember(members, 'drop_view')
validateNotMember(members, 'drop_table')

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

#@ Query collection as table
print 'get_collection_as_table().select():', mySchema.get_collection_as_table('collection1').select().execute()

#@ Collection creation
collection = schema.create_collection('my_sample_collection')
print 'create_collection():', collection

#@<OUT> Testing help for drop_collection
print mySchema.help("drop_collection")

#@ Testing dropping existing schema objects
print mySchema.get_collection('collection1')
print mySchema.drop_collection('collection1')

#@ Testing dropped objects are actually dropped
mySchema.get_collection('collection1')

#@ Testing dropping non-existing schema objects
print mySchema.drop_collection('non_existing_collection')

#@ Testing drop functions using execute
mySchema.drop_collection('collection1').execute()

#@ Testing existence
print 'Valid:', schema.exists_in_database()
mySession.drop_schema('js_shell_test')
print 'Invalid:', schema.exists_in_database()

#@ Testing name shadowing: setup
mySession.create_schema('py_db_object_shadow');
mySession.set_current_schema('py_db_object_shadow');

collection_sql = "(`doc` json DEFAULT NULL,`_id` varchar(32) GENERATED ALWAYS AS (json_unquote(json_extract(doc, '$._id'))) STORED) ENGINE=InnoDB DEFAULT CHARSET=utf8"

mySession.sql('create table `name` (name varchar(50));');
mySession.sql('create table `schema` ' + collection_sql);
mySession.sql('create table `session` (name varchar(50));');
mySession.sql('create table `getTable` ' + collection_sql);
mySession.sql('create table `get_table` (name varchar(50));');
mySession.sql('create table `getCollection` ' + collection_sql);
mySession.sql('create table `get_collection` (name varchar(50));');
mySession.sql('create table `another` ' + collection_sql);

schema = mySession.get_schema('py_db_object_shadow');

#@ Testing name shadowing: name
print(schema.name)

#@ Testing name shadowing: getName
print schema.get_name()

#@ Testing name shadowing: schema
print schema.schema

#@ Testing name shadowing: getSchema
print schema.get_schema()

#@ Testing name shadowing: session
print schema.session

#@ Testing name shadowing: getSession
print schema.get_session()

#@ Testing name shadowing: another
print schema.another

#@ Testing name shadowing: get_collection('another')
print schema.get_collection('another')

#@ Testing name shadowing: get_table('name')
print schema.get_table('name')

#@ Testing name shadowing: get_collection('schema')
print schema.get_collection('schema')

#@ Testing name shadowing: get_table('session')
print schema.get_table('session')

#@ Testing name shadowing: get_collection('getTable')
print schema.get_collection('getTable')

#@ Testing name shadowing: getTable (not a python function)
if lower_case_table_names == 1:
    print schema.gettable
else:
    print schema.getTable

#@ Testing name shadowing: get_table('get_table')
print schema.get_table('get_table')

#@ Testing name shadowing: get_collection('getCollection')
print schema.get_collection('getCollection')

#@ Testing name shadowing: getCollection (not a python function)
if lower_case_table_names == 1:
    print schema.getcollection
else:
    print schema.getCollection

#@ Testing name shadowing: get_table('get_collection')
print schema.get_table('get_collection')

mySession.drop_schema('py_db_object_shadow')

# Closes the session
mySession.close()
