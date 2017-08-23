# Assumptions: ensure_schema_does_not_exist is available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
# validateMemer and validateNotMember are defined on the setup script
from mysqlsh import mysqlx

#@ NodeSession: validating members
nodeSession = mysqlx.get_node_session(__uripwd)
all_members = dir(nodeSession)

# Remove the python built in members
nodeSessionMembers = []
for member in all_members:
  if not member.startswith('__'):
    nodeSessionMembers.append(member)

validateMember(nodeSessionMembers, 'close')
validateMember(nodeSessionMembers, 'create_schema')
validateMember(nodeSessionMembers, 'get_current_schema')
validateMember(nodeSessionMembers, 'get_default_schema')
validateMember(nodeSessionMembers, 'get_schema')
validateMember(nodeSessionMembers, 'get_schemas')
validateMember(nodeSessionMembers, 'get_uri')
validateMember(nodeSessionMembers, 'set_current_schema')
validateMember(nodeSessionMembers, 'set_fetch_warnings')
validateMember(nodeSessionMembers, 'sql')
validateMember(nodeSessionMembers, 'default_schema')
validateMember(nodeSessionMembers, 'uri')
validateMember(nodeSessionMembers, 'current_schema')

#@<OUT> NodeSession: help
nodeSession.help()

#@ NodeSession: accessing Schemas
schemas = nodeSession.get_schemas()
print getSchemaFromList(schemas, 'mysql')
print getSchemaFromList(schemas, 'information_schema')

#@ NodeSession: accessing individual schema
schema = nodeSession.get_schema('mysql')
print schema.name
schema = nodeSession.get_schema('information_schema')
print schema.name

#@ NodeSession: accessing unexisting schema
schema = nodeSession.get_schema('unexisting_schema')

#@ NodeSession: current schema validations: nodefault
dschema = nodeSession.get_default_schema()
cschema = nodeSession.get_current_schema()
print dschema
print cschema

#@ NodeSession: create schema success
ensure_schema_does_not_exist(nodeSession, 'node_session_schema')

ss = nodeSession.create_schema('node_session_schema')
print ss

#@ NodeSession: create schema failure
sf = nodeSession.create_schema('node_session_schema')

#@ NodeSession: Transaction handling: rollback
collection = ss.create_collection('sample')
nodeSession.start_transaction()
res1 = collection.add({"name":'john', "age": 15}).execute()
res2 = collection.add({"name":'carol', "age": 16}).execute()
res3 = collection.add({"name":'alma', "age": 17}).execute()
nodeSession.rollback()

result = collection.find().execute()
print 'Inserted Documents:', len(result.fetch_all())

#@ NodeSession: Transaction handling: commit
nodeSession.start_transaction()
res1 = collection.add({"name":'john', "age": 15}).execute()
res2 = collection.add({"name":'carol', "age": 16}).execute()
res3 = collection.add({"name":'alma', "age": 17}).execute()
nodeSession.commit()

result = collection.find().execute()
print 'Inserted Documents:', len(result.fetch_all())

nodeSession.drop_schema('node_session_schema')

#@ NodeSession: current schema validations: nodefault, mysql
nodeSession.set_current_schema('mysql')
dschema = nodeSession.get_default_schema()
cschema = nodeSession.get_current_schema()
print dschema
print cschema

#@ NodeSession: current schema validations: nodefault, information_schema
nodeSession.set_current_schema('information_schema')
dschema = nodeSession.get_default_schema()
cschema = nodeSession.get_current_schema()
print dschema
print cschema

#@ NodeSession: current schema validations: default
nodeSession.close()
nodeSession = mysqlx.get_node_session(__uripwd + '/mysql')
dschema = nodeSession.get_default_schema()
cschema = nodeSession.get_current_schema()
print dschema
print cschema

#@ NodeSession: current schema validations: default, information_schema
nodeSession.set_current_schema('information_schema')
dschema = nodeSession.get_default_schema()
cschema = nodeSession.get_current_schema()
print dschema
print cschema

#@ NodeSession: set_fetch_warnings(False)
nodeSession.set_fetch_warnings(False)
result = nodeSession.sql('drop database if exists unexisting').execute()
print result.warning_count

#@ NodeSession: set_fetch_warnings(True)
nodeSession.set_fetch_warnings(True)
result = nodeSession.sql('drop database if exists unexisting').execute()
print result.warning_count
print result.warnings[0].message

#@ NodeSession: quote_name no parameters
print nodeSession.quote_name()

#@ NodeSession: quote_name wrong param type
print nodeSession.quote_name(5)

#@ NodeSession: quote_name with correct parameters
print nodeSession.quote_name('sample')
print nodeSession.quote_name('sam`ple')
print nodeSession.quote_name('`sample`')
print nodeSession.quote_name('`sample')
print nodeSession.quote_name('sample`')

#@# nodeSession: bad params
mysqlx.get_node_session()
mysqlx.get_node_session(42)
mysqlx.get_node_session(["bla"])
mysqlx.get_node_session(None)

# Cleanup
nodeSession.close()
