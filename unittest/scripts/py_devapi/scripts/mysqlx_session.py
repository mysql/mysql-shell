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
validateMember(mySessionMembers, 'set_savepoint')
validateMember(mySessionMembers, 'release_savepoint')
validateMember(mySessionMembers, 'rollback_to')

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
res1 = collection.add({"_id": "4C514FF38144B714E7119BCF48B4CA01", "name":'john', "age": 15}).execute()
res2 = collection.add({"_id": "4C514FF38144B714E7119BCF48B4CA02", "name":'carol', "age": 16}).execute()
res3 = collection.add({"_id": "4C514FF38144B714E7119BCF48B4CA03", "name":'alma', "age": 17}).execute()
mySession.rollback()

result = collection.find().execute()
print 'Inserted Documents:', len(result.fetch_all())

#@ Session: Transaction handling: commit
mySession.start_transaction()
res1 = collection.add({"_id": "4C514FF38144B714E7119BCF48B4CA04", "name":'john', "age": 15}).execute()
res2 = collection.add({"_id": "4C514FF38144B714E7119BCF48B4CA05", "name":'carol', "age": 16}).execute()
res3 = collection.add({"_id": "4C514FF38144B714E7119BCF48B4CA06", "name":'alma', "age": 17}).execute()
mySession.commit()

result = collection.find().execute()
print 'Inserted Documents:', len(result.fetch_all())




#@ Transaction Savepoints Initialization
mySession.drop_schema('testSP')
schema = mySession.create_schema('testSP')
coll = schema.create_collection('collSP')

#@# Savepoint Error Conditions (WL10859-ET1_2)
sp = mySession.set_savepoint(None)
sp = mySession.set_savepoint('mysql3306', 'extraParam')

#@ Create a savepoint without specifying a name (WL10869-SR1_1)
mySession.start_transaction()
coll.add({ '_id': '5C514FF38144B714E7119BCF48B4CA98', 'name' : 'test1'})
sp = mySession.set_savepoint()
print "Autogenerated Savepoint: %s" % sp
coll.add({ '_id': '5C514FF38144B714E7119BCF48B4CA99', 'name' : 'test2'})

#@<OUT> WL10869-SR1_1: Documents
coll.find()

#@<OUT> Rollback a savepoint using a savepoint with auto generated name (WL10869-SR1_2)
mySession.rollback_to(sp)
coll.find()

#@ Release a savepoint using a savepoint with auto generated name (WL10869-SR1_3)
sp = mySession.set_savepoint()
print "Autogenerated Savepoint: %s" % sp
mySession.release_savepoint(sp)

#@ Create multiple savepoints with auto generated name and verify are unique in the session (WL10869-SR1_4)
sp = mySession.set_savepoint()
print "Autogenerated Savepoint: %s" % sp
coll.add({ '_id': '5C514FF38144B714E7119BCF48B4CBA0', 'name' : 'test3'})

#@<OUT> WL10869-SR1_4: Documents
coll.find()

#@ Create a savepoint specifying a name (WL10869-SR1_5)
sp = mySession.set_savepoint('mySavedPoint')
print "Savepoint: %s" % sp
coll.add({ '_id': '5C514FF38144B714E7119BCF48B4CBA1', 'name' : 'test4'})

#@<OUT> WL10869-SR1_5: Documents
coll.find()

#@<OUT> Rollback a savepoint using a savepoint created with a custom name (WL10869-SR1_6)
mySession.rollback_to(sp)
coll.find()

#@ Release a savepoint using a savepoint with a custom name (WL10869-SR1_6)
sp = mySession.set_savepoint('anotherSP')
print "Savepoint: %s" % sp
mySession.release_savepoint(sp)
sp = mySession.rollback()

#@<OUT> Create a savepoint several times with the same name, the savepoint must be overwritten (WL10869-ET1_3)
mySession.start_transaction()
sp = mySession.set_savepoint('anotherSP')
coll.add({ '_id': '5C514FF38144B714E7119BCF48B4CBA1', 'name' : 'test4'})
sp = mySession.set_savepoint('anotherSP')
coll.add({ '_id': '5C514FF38144B714E7119BCF48B4CBA2', 'name' : 'test5'})
sp = mySession.set_savepoint('anotherSP')
coll.add({ '_id': '5C514FF38144B714E7119BCF48B4CBA3', 'name' : 'test6'})
mySession.rollback_to(sp)
coll.find()

#@ Releasing the savepoint
mySession.release_savepoint(sp)

#@ Release the same savepoint multiple times, an error must be thrown (WL10869-ET1_4)
mySession.release_savepoint(sp)

#@ Rollback a non existing savepoint, exception must be thrown (WL10869-ET1_7)
mySession.rollback_to('unexisting')

#@ Final rollback
mySession.rollback()
coll.find()

#@<OUT> Rollback and Release a savepoint after a transaction commit, error must be thrown
mySession.start_transaction()
sp = mySession.set_savepoint()
coll.add({ '_id': '5C514FF38144B714E7119BCF48B4CBA4', 'name' : 'test7'})
mySession.commit()
coll.find()

#@ rollback_to after commit (WL10869-ET1_8)
mySession.rollback_to(sp)

#@ release after commit (WL10869-ET1_8)
mySession.release_savepoint(sp)

#@<OUT> Rollback and Release a savepoint after a transaction rollback, error must be thrown
mySession.start_transaction()
sp = mySession.set_savepoint()
coll.add({ '_id': '5C514FF38144B714E7119BCF48B4CBA5', 'name' : 'test8'})
mySession.rollback()
coll.find()

#@ rollback_to after rollback (WL10869-ET1_9)
mySession.rollback_to(sp)

#@ release after rollback (WL10869-ET1_9)
-
mySession.release_savepoint(sp)




#@ Session: test for drop schema functions
mySession.drop_collection('node_session_schema', 'coll')
mySession.drop_table('node_session_schema', 'table')
mySession.drop_view('node_session_schema', 'view')

#@ Session: Testing dropping existing schema
print mySession.drop_schema('node_session_schema')

#@ Session: Testing if the schema is actually dropped
mySession.get_schema('node_session_schema')

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
