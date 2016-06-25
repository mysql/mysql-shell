# Assumptions: ensure_schema_does_not_exist is available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
# validateMemer and validateNotMember are defined on the setup script
import mysqlx

#@ Session: validating members
mySession = mysqlx.get_session(__uripwd)
all_members = dir(mySession)

# Remove the python built in members
sessionMembers = []
for member in all_members:
  if not member.startswith('__'):
    sessionMembers.append(member)

print sessionMembers

validateMember(sessionMembers, 'close')
validateMember(sessionMembers, 'create_schema')
validateMember(sessionMembers, 'get_default_schema')
validateMember(sessionMembers, 'get_schema')
validateMember(sessionMembers, 'get_schemas')
validateMember(sessionMembers, 'get_uri')
validateMember(sessionMembers, 'set_fetch_warnings')
validateMember(sessionMembers, 'default_schema')
validateMember(sessionMembers, 'uri')

#@ Session: validate dynamic members for system schemas
validateNotMember(sessionMembers, 'mysql')
validateNotMember(sessionMembers, 'information_schema')


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

#@ Session: create schema success
ensure_schema_does_not_exist(mySession, 'session_schema')

ss = mySession.create_schema('session_schema')
print ss

#@ Session: create schema failure
sf = mySession.create_schema('session_schema')

#@ Session: create quoted schema
ensure_schema_does_not_exist(mySession, 'quoted schema')
qs = mySession.create_schema('quoted schema')
print qs

#@ Session: validate dynamic members for created schemas
sessionMembers = dir(mySession)
validateNotMember(sessionMembers, 'session_schema');
validateNotMember(sessionMembers, 'quoted schema');

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


# Cleanup
mySession.drop_schema('session_schema')
mySession.drop_schema('quoted schema')
mySession.close()

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

# Cleanup
nodeSession.close()