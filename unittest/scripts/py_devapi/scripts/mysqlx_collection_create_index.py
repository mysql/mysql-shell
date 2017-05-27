# Assumptions: validate_crud_functions available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
from mysqlsh import mysqlx

mySession = mysqlx.get_node_session(__uripwd)

ensure_schema_does_not_exist(mySession, 'js_shell_test')
schema = mySession.create_schema('js_shell_test')

# Creates a test collection and inserts data into it
collection = schema.create_collection('collection1')


# --------------------------------------
# Create index, dynamic function testing
# --------------------------------------
#@ CollectionCreateIndex: valid operations after create_index
create_index = collection.create_index('_name')
validate_crud_functions(create_index, ['field'])

#@ CollectionAdd: valid operations after field
create_index.field('name', "TEXT(50)", True)
validate_crud_functions(create_index, ['field', 'execute'])

#@ CollectionAdd: valid operations after execute
result = create_index.execute()
validate_crud_functions(create_index, [])


# -----------------------------------
# Error conditions on chained methods
# -----------------------------------
#@# Error conditions on create_index
create_index = collection.create_index()
create_index = collection.create_index(5)
create_index = collection.create_index('_sample', 5)
create_index = collection.create_index('_sample', mysqlx.Type.STRING)
create_index = collection.create_index('_sample', mysqlx.IndexType.UNIQUE)

#@# Error conditions on field
create_index.field()
create_index.field(6,6,6)
create_index.field('other', 6, 6)
create_index.field('other', "INTEGER", "sample")
create_index.field('other', "INTEGER", True)
mySession.drop_collection('js_shell_test', 'collection1')


# -----------------------------------
# Execution tests
# -----------------------------------
#@ NonUnique Index: creation with required field
ensure_schema_does_not_exist(mySession, 'js_shell_test')
schema = mySession.create_schema('js_shell_test')

# Creates a test collection and inserts data into it
collection = schema.create_collection('collection1')

result = collection.create_index('_name').field('name', "TEXT(50)", True).execute()

result = collection.add({'name':'John', 'last_name':'Carter', 'age':17}).execute()
result = collection.add({'name':'John', 'last_name':'Doe', 'age':18}).execute()
result = collection.find('name="John"').execute()
records = result.fetch_all()
print 'John Records:', len(records)

#@ ERROR: insertion of document missing required field
result = collection.add({'alias':'Rock', 'last_name':'Doe', 'age':19}).execute()

#@ ERROR: attempt to create an index with the same name
result = collection.create_index('_name').field('alias', "TEXT(50)", True).execute()

#@ ERROR: Attempt to create unique index when records already duplicate the key field
result = collection.drop_index('_name').execute()
result = collection.create_index('_name', mysqlx.IndexType.UNIQUE).field('name', "TEXT(50)", True).execute()

#@ ERROR: Attempt to create unique index when records are missing the key field
result = collection.create_index('_alias', mysqlx.IndexType.UNIQUE).field('alias', "TEXT(50)", True).execute()

#@ Unique index: creation with required field
result = collection.remove('1').execute()
result = collection.create_index('_name', mysqlx.IndexType.UNIQUE).field('name', "TEXT(50)", True).execute()
result = collection.add({'name':'John', 'last_name':'Carter', 'age':17}).execute()
result = collection.add({'name':'John', 'last_name':'Doe', 'age':18}).execute()

#@ Cleanup
mySession.close()



