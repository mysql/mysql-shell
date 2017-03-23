// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<mysql_port>
// validateMemer and validateNotMember are defined on the setup script
var mysql = require('mysql');

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
validateMember(sessionMembers, 'uri');
validateMember(sessionMembers, 'currentSchema');

//@ ClassicSession: validate dynamic members for system schemas
var sessionMembers = dir(classicSession)
validateNotMember(sessionMembers, 'mysql');
validateNotMember(sessionMembers, 'information_schema');


//@ ClassicSession: accessing Schemas
var schemas = classicSession.getSchemas();
print(getSchemaFromList(schemas, 'mysql'));
print(getSchemaFromList(schemas, 'information_schema'));

//@ ClassicSession: accessing individual schema
var schema = classicSession.getSchema('mysql');
print(schema.name);
var schema = classicSession.getSchema('information_schema');
print(schema.name);

//@ ClassicSession: accessing unexisting schema
var schema = classicSession.getSchema('unexisting_schema');

//@ ClassicSession: current schema validations: nodefault
var dschema = classicSession.getDefaultSchema();
var cschema = classicSession.getCurrentSchema();
print(dschema);
print(cschema);

//@ ClassicSession: create schema success
ensure_schema_does_not_exist(classicSession, 'node_session_schema');

var ss = classicSession.createSchema('node_session_schema');
print(ss)

//@ ClassicSession: create schema failure
var sf = classicSession.createSchema('node_session_schema');

//@ Session: create quoted schema
ensure_schema_does_not_exist(classicSession, 'quoted schema');
var qs = classicSession.createSchema('quoted schema');
print(qs);

//@ Session: validate dynamic members for created schemas
var sessionMembers = dir(classicSession)
validateNotMember(sessionMembers, 'node_session_schema');
validateNotMember(sessionMembers, 'quoted schema');

//@ ClassicSession: Transaction handling: rollback
classicSession.setCurrentSchema('node_session_schema');

var result = classicSession.runSql('create table sample (name varchar(50) primary key)');
classicSession.startTransaction();
var res1 = classicSession.runSql('insert into sample values ("john")');
var res2 = classicSession.runSql('insert into sample values ("carol")');
var res3 = classicSession.runSql('insert into sample values ("jack")');
classicSession.rollback();

var result = classicSession.runSql('select * from sample');
print('Inserted Documents:', result.fetchAll().length);

//@ ClassicSession: Transaction handling: commit
classicSession.startTransaction();
var res1 = classicSession.runSql('insert into sample values ("john")');
var res2 = classicSession.runSql('insert into sample values ("carol")');
var res3 = classicSession.runSql('insert into sample values ("jack")');
classicSession.commit();

var result = classicSession.runSql('select * from sample');
print('Inserted Documents:', result.fetchAll().length);

classicSession.dropSchema('node_session_schema');
classicSession.dropSchema('quoted schema');

//@ ClassicSession: current schema validations: nodefault, mysql
classicSession.setCurrentSchema('mysql');
var dschema = classicSession.getDefaultSchema();
var cschema = classicSession.getCurrentSchema();
print(dschema);
print(cschema);

//@ ClassicSession: current schema validations: nodefault, information_schema
classicSession.setCurrentSchema('information_schema');
var dschema = classicSession.getDefaultSchema();
var cschema = classicSession.getCurrentSchema();
print(dschema);
print(cschema);

//@ ClassicSession: current schema validations: default
classicSession.close()
classicSession = mysql.getClassicSession(__uripwd + '/mysql');
var dschema = classicSession.getDefaultSchema();
var cschema = classicSession.getCurrentSchema();
print(dschema);
print(cschema);

//@ ClassicSession: current schema validations: default, information_schema
classicSession.setCurrentSchema('information_schema');
var dschema = classicSession.getDefaultSchema();
var cschema = classicSession.getCurrentSchema();
print(dschema);
print(cschema);

//$ ClassicSession: date handling
classicSession.runSql("select cast('9999-12-31 23:59:59.999999' as datetime(6))");

// Cleanup
classicSession.close();
