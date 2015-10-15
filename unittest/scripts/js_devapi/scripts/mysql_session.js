// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<mysql_port>
// validateMemer and validateNotMember are defined on the setup script
var mysql = require('mysql').mysql;

//@ Session: validating members
var classicSession = mysql.getClassicSession(__uripwd);
var sessionMembers = dir(classicSession);

validateMember(sessionMembers, 'close');
validateMember(sessionMembers, 'createSchema');
validateMember(sessionMembers, 'getCurrentSchema');
validateMember(sessionMembers, 'getDefaultSchema');
validateMember(sessionMembers, 'getSchema');
validateMember(sessionMembers, 'getSchemas');
validateMember(sessionMembers, 'getUri');
validateMember(sessionMembers, 'setCurrentSchema');
validateMember(sessionMembers, 'runSql');
validateMember(sessionMembers, 'defaultSchema');
validateMember(sessionMembers, 'schemas');
validateMember(sessionMembers, 'uri');
validateMember(sessionMembers, 'currentSchema');

//@ ClassicSession: accessing Schemas
var schemas = classicSession.getSchemas();
print(schemas.mysql);
print(schemas.information_schema);

//@ ClassicSession: accessing individual schema
var schema;
schema = classicSession.getSchema('mysql');
print(schema.name);
schema = classicSession.getSchema('information_schema');
print(schema.name);

//@ ClassicSession: accessing schema through dynamic attributes
print(classicSession.mysql.name)
print(classicSession.information_schema.name)

//@ ClassicSession: accessing unexisting schema
schema = classicSession.getSchema('unexisting_schema');

//@ ClassicSession: current schema validations: nodefault
var dschema;
var cschema;
dschema = classicSession.getDefaultSchema();
cschema = classicSession.getCurrentSchema();
print(dschema);
print(cschema);

//@ ClassicSession: create schema success
ensure_schema_does_not_exist(classicSession, 'node_session_schema');

ss = classicSession.createSchema('node_session_schema');
print(ss)

//@ ClassicSession: create schema failure
var sf = classicSession.createSchema('node_session_schema');

//@ Session: create quoted schema
ensure_schema_does_not_exist(classicSession, 'quoted schema');
var qs = classicSession.createSchema('quoted schema');
print(qs);

//@ Session: validate dynamic members for created schemas
sessionMembers = dir(classicSession)
validateMember(sessionMembers, 'node_session_schema');
validateNotMember(sessionMembers, 'quoted schema');

//@ ClassicSession: Transaction handling: rollback
classicSession.setCurrentSchema('node_session_schema');

var result = classicSession.runSql('create table sample (name varchar(50))');
classicSession.startTransaction();
var res1 = classicSession.runSql('insert into sample values ("john")');
var res2 = classicSession.runSql('insert into sample values ("carol")');
var res3 = classicSession.runSql('insert into sample values ("jack")');
classicSession.rollback();

result = classicSession.runSql('select * from sample');
print('Inserted Documents:', result.fetchAll().length);

//@ ClassicSession: Transaction handling: commit
classicSession.startTransaction();
var res1 = classicSession.runSql('insert into sample values ("john")');
var res2 = classicSession.runSql('insert into sample values ("carol")');
var res3 = classicSession.runSql('insert into sample values ("jack")');
classicSession.commit();

result = classicSession.runSql('select * from sample');
print('Inserted Documents:', result.fetchAll().length);

classicSession.dropSchema('node_session_schema');
classicSession.dropSchema('quoted schema');

//@ ClassicSession: current schema validations: nodefault, mysql
classicSession.setCurrentSchema('mysql');
dschema = classicSession.getDefaultSchema();
cschema = classicSession.getCurrentSchema();
print(dschema);
print(cschema);

//@ ClassicSession: current schema validations: nodefault, information_schema
classicSession.setCurrentSchema('information_schema');
dschema = classicSession.getDefaultSchema();
cschema = classicSession.getCurrentSchema();
print(dschema);
print(cschema);

//@ ClassicSession: current schema validations: default 
classicSession.close()
classicSession = mysql.getClassicSession(__uripwd + '/mysql');
dschema = classicSession.getDefaultSchema();
cschema = classicSession.getCurrentSchema();
print(dschema);
print(cschema);

//@ ClassicSession: current schema validations: default, information_schema
classicSession.setCurrentSchema('information_schema');
dschema = classicSession.getDefaultSchema();
cschema = classicSession.getCurrentSchema();
print(dschema);
print(cschema);

// Cleanup
classicSession.close();