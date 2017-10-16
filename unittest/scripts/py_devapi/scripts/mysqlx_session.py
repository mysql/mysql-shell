# Assumptions: ensure_schema_does_not_exist is available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
# validateMemer and validateNotMember are defined on the setup script
from mysqlsh import mysqlx

#@ Session: validating members
mySession = mysqlx.get_session(__uripwd)
all_members = dir(mySession)

# Remove the python built in members
mySessionMembers = []
for member in all_members:
  if not member.startswith('__'):
    mySessionMembers.append(member)

validateMember(mySessionMembers, 'close')
validateMember(mySessionMembers, 'create_schema')
validateMember(mySessionMembers, 'get_current_schema')
validateMember(mySessionMembers, 'get_default_schema')
validateMember(mySessionMembers, 'get_schema')
validateMember(mySessionMembers, 'get_schemas')
validateMember(mySessionMembers, 'get_uri')
validateMember(mySessionMembers, 'set_current_schema')
validateMember(mySessionMembers, 'set_fetch_warnings')
validateMember(mySessionMembers, 'sql')
validateMember(mySessionMembers, 'default_schema')
validateMember(mySessionMembers, 'uri')
validateMember(mySessionMembers, 'current_schema')

#@<OUT> Session: help
mySession.help()

#@<OUT> Session: dir
dir(mySession)

#@ Session: accessing Schemas
schemas = mySession.get_schemas()
print getSchemaFromList(schemas, 'mysql')
print getSchemaFromList(schemas, 'information_schema')

#@ Session: accessing individual schema
schema = mySession.get_schema('mysql')
print schema.name
schema = mySession.get_schema('information_schema')
print schema.name

#@ Session: accessing unexisting schema
schema = mySession.get_schema('unexisting_schema')

#@ Session: current schema validations: nodefault
dschema = mySession.get_default_schema()
cschema = mySession.get_current_schema()
print dschema
print cschema

#@ Session: create schema success
ensure_schema_does_not_exist(mySession, 'node_session_schema')

ss = mySession.create_schema('node_session_schema')
print ss

#@ Session: create schema failure
sf = mySession.create_schema('node_session_schema')

#@ Session: Transaction handling: rollback
collection = ss.create_collection('sample')
mySession.start_transaction()
res1 = collection.add({"name":'john', "age": 15}).execute()
res2 = collection.add({"name":'carol', "age": 16}).execute()
res3 = collection.add({"name":'alma', "age": 17}).execute()
mySession.rollback()

result = collection.find().execute()
print 'Inserted Documents:', len(result.fetch_all())

#@ Session: Transaction handling: commit
mySession.start_transaction()
res1 = collection.add({"name":'john', "age": 15}).execute()
res2 = collection.add({"name":'carol', "age": 16}).execute()
res3 = collection.add({"name":'alma', "age": 17}).execute()
mySession.commit()

result = collection.find().execute()
print 'Inserted Documents:', len(result.fetch_all())

#@ Session: test for drop schema functions
mySession.drop_collection('node_session_schema', 'coll')
mySession.drop_table('node_session_schema', 'table')
mySession.drop_view('node_session_schema', 'view')

#@ Session: Testing dropping existing schema
print mySession.drop_schema('node_session_schema')

#@ Session: Testing if the schema is actually dropped
mySession.get_schema('node_session_schema')

#@<OUT> Session: Testing drop_schema help
print mySession.help('drop_schema')

#@ Session: Testing dropping non-existing schema
print mySession.drop_schema('non_existing')

#@ Session: current schema validations: nodefault, mysql
mySession.set_current_schema('mysql')
dschema = mySession.get_default_schema()
cschema = mySession.get_current_schema()
print dschema
print cschema

#@ Session: current schema validations: nodefault, information_schema
mySession.set_current_schema('information_schema')
dschema = mySession.get_default_schema()
cschema = mySession.get_current_schema()
print dschema
print cschema

#@ Session: current schema validations: default
mySession.close()
mySession = mysqlx.get_session(__uripwd + '/mysql')
dschema = mySession.get_default_schema()
cschema = mySession.get_current_schema()
print dschema
print cschema

#@ Session: current schema validations: default, information_schema
mySession.set_current_schema('information_schema')
dschema = mySession.get_default_schema()
cschema = mySession.get_current_schema()
print dschema
print cschema

#@ Session: set_fetch_warnings(False)
mySession.set_fetch_warnings(False)
result = mySession.sql('drop database if exists unexisting').execute()
print result.warning_count

#@ Session: set_fetch_warnings(True)
mySession.set_fetch_warnings(True)
result = mySession.sql('drop database if exists unexisting').execute()
print result.warning_count
print result.warnings[0].message

#@ Session: quote_name no parameters
print mySession.quote_name()

#@ Session: quote_name wrong param type
print mySession.quote_name(5)

#@ Session: quote_name with correct parameters
print mySession.quote_name('sample')
print mySession.quote_name('sam`ple')
print mySession.quote_name('`sample`')
print mySession.quote_name('`sample')
print mySession.quote_name('sample`')

#@# Session: bad params
mysqlx.get_session()
mysqlx.get_session(42)
mysqlx.get_session(["bla"])
mysqlx.get_session(None)

# Cleanup
mySession.close()
