# Assumptions: ensure_schema_does_not_exist is available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
# validateMemer and validateNotMember are defined on the setup script
import mysqlx

#@ Session: validating members
mySession = mysqlx.getSession(__uripwd)
all_members = dir(mySession)

# Remove the python built in members
sessionMembers = []
for member in all_members:
  if not member.startswith('__'):
    sessionMembers.append(member)

print sessionMembers

validateMember(sessionMembers, 'close')
validateMember(sessionMembers, 'createSchema')
validateMember(sessionMembers, 'getDefaultSchema')
validateMember(sessionMembers, 'getSchema')
validateMember(sessionMembers, 'getSchemas')
validateMember(sessionMembers, 'getUri')
validateMember(sessionMembers, 'setFetchWarnings')
validateMember(sessionMembers, 'defaultSchema')
validateMember(sessionMembers, 'uri')

#@ Session: validate dynamic members for system schemas
validateNotMember(sessionMembers, 'mysql')
validateNotMember(sessionMembers, 'information_schema')


#@ Session: accessing Schemas
schemas = mySession.getSchemas()
print getSchemaFromList(schemas, 'mysql')
print getSchemaFromList(schemas, 'information_schema')

#@ Session: accessing individual schema
schema = mySession.getSchema('mysql')
print schema.name
schema = mySession.getSchema('information_schema')
print schema.name

#@ Session: accessing unexisting schema
schema = mySession.getSchema('unexisting_schema')

#@ Session: create schema success
ensure_schema_does_not_exist(mySession, 'session_schema')

ss = mySession.createSchema('session_schema')
print ss

#@ Session: create schema failure
sf = mySession.createSchema('session_schema')

#@ Session: create quoted schema
ensure_schema_does_not_exist(mySession, 'quoted schema')
qs = mySession.createSchema('quoted schema')
print qs

#@ Session: validate dynamic members for created schemas
sessionMembers = dir(mySession)
validateNotMember(sessionMembers, 'session_schema');
validateNotMember(sessionMembers, 'quoted schema');

#@ Session: Transaction handling: rollback
collection = ss.createCollection('sample')
mySession.startTransaction()
res1 = collection.add({"name":'john', "age": 15}).execute()
res2 = collection.add({"name":'carol', "age": 16}).execute()
res3 = collection.add({"name":'alma', "age": 17}).execute()
mySession.rollback()

result = collection.find().execute()
print 'Inserted Documents:', len(result.fetchAll())

#@ Session: Transaction handling: commit
mySession.startTransaction()
res1 = collection.add({"name":'john', "age": 15}).execute()
res2 = collection.add({"name":'carol', "age": 16}).execute()
res3 = collection.add({"name":'alma', "age": 17}).execute()
mySession.commit()

result = collection.find().execute()
print 'Inserted Documents:', len(result.fetchAll())


# Cleanup
mySession.dropSchema('session_schema')
mySession.dropSchema('quoted schema')
mySession.close()

#@ NodeSession: validating members
nodeSession = mysqlx.getNodeSession(__uripwd)
all_members = dir(nodeSession)

# Remove the python built in members
nodeSessionMembers = []
for member in all_members:
  if not member.startswith('__'):
    nodeSessionMembers.append(member)

validateMember(nodeSessionMembers, 'close')
validateMember(nodeSessionMembers, 'createSchema')
validateMember(nodeSessionMembers, 'getCurrentSchema')
validateMember(nodeSessionMembers, 'getDefaultSchema')
validateMember(nodeSessionMembers, 'getSchema')
validateMember(nodeSessionMembers, 'getSchemas')
validateMember(nodeSessionMembers, 'getUri')
validateMember(nodeSessionMembers, 'setCurrentSchema')
validateMember(nodeSessionMembers, 'setFetchWarnings')
validateMember(nodeSessionMembers, 'sql')
validateMember(nodeSessionMembers, 'defaultSchema')
validateMember(nodeSessionMembers, 'uri')
validateMember(nodeSessionMembers, 'currentSchema')


#@ NodeSession: accessing Schemas
schemas = nodeSession.getSchemas()
print getSchemaFromList(schemas, 'mysql')
print getSchemaFromList(schemas, 'information_schema')

#@ NodeSession: accessing individual schema
schema = nodeSession.getSchema('mysql')
print schema.name
schema = nodeSession.getSchema('information_schema')
print schema.name

#@ NodeSession: accessing unexisting schema
schema = nodeSession.getSchema('unexisting_schema')

#@ NodeSession: current schema validations: nodefault
dschema = nodeSession.getDefaultSchema()
cschema = nodeSession.getCurrentSchema()
print dschema
print cschema

#@ NodeSession: create schema success
ensure_schema_does_not_exist(nodeSession, 'node_session_schema')

ss = nodeSession.createSchema('node_session_schema')
print ss

#@ NodeSession: create schema failure
sf = nodeSession.createSchema('node_session_schema')

#@ NodeSession: Transaction handling: rollback
collection = ss.createCollection('sample')
nodeSession.startTransaction()
res1 = collection.add({"name":'john', "age": 15}).execute()
res2 = collection.add({"name":'carol', "age": 16}).execute()
res3 = collection.add({"name":'alma', "age": 17}).execute()
nodeSession.rollback()

result = collection.find().execute()
print 'Inserted Documents:', len(result.fetchAll())

#@ NodeSession: Transaction handling: commit
nodeSession.startTransaction()
res1 = collection.add({"name":'john', "age": 15}).execute()
res2 = collection.add({"name":'carol', "age": 16}).execute()
res3 = collection.add({"name":'alma', "age": 17}).execute()
nodeSession.commit()

result = collection.find().execute()
print 'Inserted Documents:', len(result.fetchAll())

nodeSession.dropSchema('node_session_schema')

#@ NodeSession: current schema validations: nodefault, mysql
nodeSession.setCurrentSchema('mysql')
dschema = nodeSession.getDefaultSchema()
cschema = nodeSession.getCurrentSchema()
print dschema
print cschema

#@ NodeSession: current schema validations: nodefault, information_schema
nodeSession.setCurrentSchema('information_schema')
dschema = nodeSession.getDefaultSchema()
cschema = nodeSession.getCurrentSchema()
print dschema
print cschema

#@ NodeSession: current schema validations: default 
nodeSession.close()
nodeSession = mysqlx.getNodeSession(__uripwd + '/mysql')
dschema = nodeSession.getDefaultSchema()
cschema = nodeSession.getCurrentSchema()
print dschema
print cschema

#@ NodeSession: current schema validations: default, information_schema
nodeSession.setCurrentSchema('information_schema')
dschema = nodeSession.getDefaultSchema()
cschema = nodeSession.getCurrentSchema()
print dschema
print cschema

#@ NodeSession: setFetchWarnings(False)
nodeSession.setFetchWarnings(False)
result = nodeSession.sql('drop database if exists unexisting').execute()
print result.warningCount

#@ NodeSession: setFetchWarnings(True)
nodeSession.setFetchWarnings(True)
result = nodeSession.sql('drop database if exists unexisting').execute()
print result.warningCount
print result.warnings[0].Message

#@ NodeSession: quoteName no parameters
print nodeSession.quoteName()

#@ NodeSession: quoteName wrong param type
print nodeSession.quoteName(5)

#@ NodeSession: quoteName with correct parameters
print nodeSession.quoteName('sample')
print nodeSession.quoteName('sam`ple')
print nodeSession.quoteName('`sample`')
print nodeSession.quoteName('`sample')
print nodeSession.quoteName('sample`')

# Cleanup
nodeSession.close()