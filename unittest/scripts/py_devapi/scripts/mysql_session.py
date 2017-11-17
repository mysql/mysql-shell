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
validateMember(sessionMembers, 'query')
validateMember(sessionMembers, 'run_sql')
validateMember(sessionMembers, 'default_schema')
validateMember(sessionMembers, 'uri')
validateMember(sessionMembers, 'current_schema')

#@<OUT> Session: help
classicSession.help()

#@ ClassicSession: accessing Schemas
schemas = classicSession.get_schemas();

#@ ClassicSession: accessing individual schema
schema = classicSession.get_schema('mysql');

#@ ClassicSession: accessing default schema
dschema = classicSession.get_default_schema();

#@ ClassicSession: accessing current schema
cschema = classicSession.get_current_schema();

#@ ClassicSession: create schema
sf = classicSession.create_schema('classic_session_schema');

#@ ClassicSession: set current schema
classicSession.set_current_schema('classic_session_schema');

#@ ClassicSession: drop schema
classicSession.drop_schema('node_session_schema');

#@Preparation for transaction tests
result = classicSession.run_sql('drop schema if exists classic_session_schema');
result = classicSession.run_sql('create schema classic_session_schema');
result = classicSession.run_sql('use classic_session_schema');

#@ ClassicSession: Transaction handling: rollback
result = classicSession.run_sql('create table sample (name varchar(50) primary key)');
classicSession.start_transaction();
res1 = classicSession.run_sql('insert into sample values ("john")');
res2 = classicSession.run_sql('insert into sample values ("carol")');
res3 = classicSession.run_sql('insert into sample values ("jack")');
classicSession.rollback();

result = classicSession.run_sql('select * from sample');
print 'Inserted Documents:', len(result.fetch_all());

#@ ClassicSession: Transaction handling: commit
classicSession.start_transaction();
res1 = classicSession.run_sql('insert into sample values ("john")');
res2 = classicSession.run_sql('insert into sample values ("carol")');
res3 = classicSession.run_sql('insert into sample values ("jack")');
classicSession.commit();

result = classicSession.run_sql('select * from sample');
print 'Inserted Documents:', len(result.fetch_all());

#@ ClassicSession: date handling
classicSession.run_sql("select cast('9999-12-31 23:59:59.999999' as datetime(6))");

#@# ClassicSession: run_sql errors
classicSession.run_sql();
classicSession.run_sql(1, 2, 3);
classicSession.run_sql(1);
classicSession.run_sql('select ?', 5);
classicSession.run_sql('select ?, ?', [1, 2, 3]);
classicSession.run_sql('select ?, ?', [1]);

#@<OUT> ClassicSession: run_sql placeholders
classicSession.run_sql("select ?, ?", ['hello', 1234]);

#@# ClassicSession: query errors
classicSession.query();
classicSession.query(1, 2, 3);
classicSession.query(1);
classicSession.query('select ?', 5);
classicSession.query('select ?, ?', [1, 2, 3]);
classicSession.query('select ?, ?', [1]);

#@<OUT> ClassicSession: query placeholders
classicSession.query("select ?, ?", ['hello', 1234]);

# Cleanup
classicSession.close();
