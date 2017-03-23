# Assumptions: ensure_schema_does_not_exist is available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<mysql_port>
# validateMemer and validateNotMember are defined on the setup script
from mysqlsh import mysql

#@ Session: validating members
classicSession = mysql.get_classic_session(__uripwd)
all_members = dir(classicSession)

# Remove the python built in members
sessionMembers = []
for member in all_members:
  if not member.startswith('__'):
    sessionMembers.append(member)


validateMember(sessionMembers, 'close')
validateMember(sessionMembers, 'create_schema')
validateMember(sessionMembers, 'get_current_schema')
validateMember(sessionMembers, 'get_default_schema')
validateMember(sessionMembers, 'get_schema')
validateMember(sessionMembers, 'get_schemas')
validateMember(sessionMembers, 'get_uri')
validateMember(sessionMembers, 'set_current_schema')
validateMember(sessionMembers, 'run_sql')
validateMember(sessionMembers, 'default_schema')
validateMember(sessionMembers, 'uri')
validateMember(sessionMembers, 'current_schema')

#@ ClassicSession: validate dynamic members for system schemas
sessionMembers = dir(classicSession)
validateNotMember(sessionMembers, 'mysql')
validateNotMember(sessionMembers, 'information_schema')

#@ ClassicSession: accessing Schemas
schemas = classicSession.get_schemas()
print getSchemaFromList(schemas, 'mysql')
print getSchemaFromList(schemas, 'information_schema')

#@ ClassicSession: accessing individual schema
schema = classicSession.get_schema('mysql')
print schema.name
schema = classicSession.get_schema('information_schema')
print schema.name

#@ ClassicSession: accessing unexisting schema
schema = classicSession.get_schema('unexisting_schema')

#@ ClassicSession: current schema validations: nodefault
dschema = classicSession.get_default_schema()
cschema = classicSession.get_current_schema()
print dschema
print cschema

#@ ClassicSession: create schema success
ensure_schema_does_not_exist(classicSession, 'node_session_schema')

ss = classicSession.create_schema('node_session_schema')
print ss

#@ ClassicSession: create schema failure
sf = classicSession.create_schema('node_session_schema')

#@ Session: create quoted schema
ensure_schema_does_not_exist(classicSession, 'quoted schema');
qs = classicSession.create_schema('quoted schema');
print(qs);

#@ Session: validate dynamic members for created schemas
sessionMembers = dir(classicSession)
validateNotMember(sessionMembers, 'node_session_schema');
validateNotMember(sessionMembers, 'quoted schema');

#@ ClassicSession: Transaction handling: rollback
classicSession.set_current_schema('node_session_schema')

result = classicSession.run_sql('create table sample (name varchar(50) primary key)')
classicSession.start_transaction()
res1 = classicSession.run_sql('insert into sample values ("john")')
res2 = classicSession.run_sql('insert into sample values ("carol")')
res3 = classicSession.run_sql('insert into sample values ("jack")')
classicSession.rollback()

result = classicSession.run_sql('select * from sample')
print 'Inserted Documents:', len(result.fetch_all())

#@ ClassicSession: Transaction handling: commit
classicSession.start_transaction()
res1 = classicSession.run_sql('insert into sample values ("john")')
res2 = classicSession.run_sql('insert into sample values ("carol")')
res3 = classicSession.run_sql('insert into sample values ("jack")')
classicSession.commit()

result = classicSession.run_sql('select * from sample')
print 'Inserted Documents:', len(result.fetch_all())

classicSession.drop_schema('node_session_schema')
classicSession.drop_schema('quoted schema')

#@ ClassicSession: current schema validations: nodefault, mysql
classicSession.set_current_schema('mysql')
dschema = classicSession.get_default_schema()
cschema = classicSession.get_current_schema()
print dschema
print cschema

#@ ClassicSession: current schema validations: nodefault, information_schema
classicSession.set_current_schema('information_schema')
dschema = classicSession.get_default_schema()
cschema = classicSession.get_current_schema()
print dschema
print cschema

#@ ClassicSession: current schema validations: default
classicSession.close()
classicSession = mysql.get_classic_session(__uripwd + '/mysql')
dschema = classicSession.get_default_schema()
cschema = classicSession.get_current_schema()
print dschema
print cschema

#@ ClassicSession: current schema validations: default, information_schema
classicSession.set_current_schema('information_schema')
dschema = classicSession.get_default_schema()
cschema = classicSession.get_current_schema()
print dschema
print cschema

#@ ClassicSession: date handling
classicSession.run_sql("select cast('9999-12-31 23:59:59.999999' as datetime(6))")

# Cleanup
classicSession.close()
