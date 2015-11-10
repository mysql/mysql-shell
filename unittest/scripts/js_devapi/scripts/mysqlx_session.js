// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
// validateMemer and validateNotMember are defined on the setup script
var mysqlx = require('mysqlx').mysqlx;

//@ Session: validating members
var mySession = mysqlx.getSession(__uripwd);
var sessionMembers = dir(mySession);

validateMember(sessionMembers, 'close');
validateMember(sessionMembers, 'createSchema');
validateMember(sessionMembers, 'getDefaultSchema');
validateMember(sessionMembers, 'getSchema');
validateMember(sessionMembers, 'getSchemas');
validateMember(sessionMembers, 'getUri');
validateMember(sessionMembers, 'setFetchWarnings');
validateMember(sessionMembers, 'defaultSchema');
validateMember(sessionMembers, 'schemas');
validateMember(sessionMembers, 'uri');

//@ Session: accessing Schemas
var schemas = mySession.getSchemas();
print(schemas.mysql);
print(schemas.information_schema);

//@ Session: accessing individual schema
var schema;
schema = mySession.getSchema('mysql');
print(schema.name);
schema = mySession.getSchema('information_schema');
print(schema.name);

//@ Session: accessing schema through dynamic attributes
print(mySession.mysql.name)
print(mySession.information_schema.name)

//@ Session: accessing unexisting schema
schema = mySession.getSchema('unexisting_schema');

//@ Session: create schema success
ensure_schema_does_not_exist(mySession, 'session_schema');

var ss = mySession.createSchema('session_schema');
print(ss);

//@ Session: create schema failure
var sf = mySession.createSchema('session_schema');

//@ Session: create quoted schema
ensure_schema_does_not_exist(mySession, 'quoted schema');
var qs = mySession.createSchema('quoted schema');
print(qs);

//@ Session: validate dynamic members for created schemas
sessionMembers = dir(mySession)
validateMember(sessionMembers, 'session_schema');
validateNotMember(sessionMembers, 'quoted schema');

//@ Session: Transaction handling: rollback
var collection = ss.createCollection('sample');
mySession.startTransaction();
var res1 = collection.add({ name: 'john', age: 15 }).execute();
var res2 = collection.add({ name: 'carol', age: 16 }).execute();
var res3 = collection.add({ name: 'alma', age: 17 }).execute();
mySession.rollback();

var result = collection.find().execute();
print('Inserted Documents:', result.fetchAll().length);

//@ Session: Transaction handling: commit
mySession.startTransaction();
var res1 = collection.add({ name: 'john', age: 15 }).execute();
var res2 = collection.add({ name: 'carol', age: 16 }).execute();
var res3 = collection.add({ name: 'alma', age: 17 }).execute();
mySession.commit();

var result = collection.find().execute();
print('Inserted Documents:', result.fetchAll().length);

// Cleanup
mySession.dropSchema('session_schema');
mySession.dropSchema('quoted schema');
mySession.close();

//@ NodeSession: validating members
var nodeSession = mysqlx.getNodeSession(__uripwd);
var nodeSessionMembers = dir(nodeSession);
validateMember(nodeSessionMembers, 'close');
validateMember(nodeSessionMembers, 'createSchema');
validateMember(nodeSessionMembers, 'getCurrentSchema');
validateMember(nodeSessionMembers, 'getDefaultSchema');
validateMember(nodeSessionMembers, 'getSchema');
validateMember(nodeSessionMembers, 'getSchemas');
validateMember(nodeSessionMembers, 'getUri');
validateMember(nodeSessionMembers, 'setCurrentSchema');
validateMember(nodeSessionMembers, 'setFetchWarnings');
validateMember(nodeSessionMembers, 'sql');
validateMember(nodeSessionMembers, 'defaultSchema');
validateMember(nodeSessionMembers, 'schemas');
validateMember(nodeSessionMembers, 'uri');
validateMember(nodeSessionMembers, 'currentSchema');

//@ NodeSession: accessing Schemas
var schemas = nodeSession.getSchemas();
print(schemas.mysql);
print(schemas.information_schema);

//@ NodeSession: accessing individual schema
var schema;
schema = nodeSession.getSchema('mysql');
print(schema.name);
schema = nodeSession.getSchema('information_schema');
print(schema.name);

//@ NodeSession: accessing schema through dynamic attributes
print(nodeSession.mysql.name)
print(nodeSession.information_schema.name)

//@ NodeSession: accessing unexisting schema
schema = nodeSession.getSchema('unexisting_schema');

//@ NodeSession: current schema validations: nodefault
var dschema;
var cschema;
dschema = nodeSession.getDefaultSchema();
cschema = nodeSession.getCurrentSchema();
print(dschema);
print(cschema);

//@ NodeSession: create schema success
ensure_schema_does_not_exist(nodeSession, 'node_session_schema');

ss = nodeSession.createSchema('node_session_schema');
print(ss)

//@ NodeSession: create schema failure
var sf = nodeSession.createSchema('node_session_schema');

//@ NodeSession: Transaction handling: rollback
var collection = ss.createCollection('sample');
nodeSession.startTransaction();
var res1 = collection.add({ name: 'john', age: 15 }).execute();
var res2 = collection.add({ name: 'carol', age: 16 }).execute();
var res3 = collection.add({ name: 'alma', age: 17 }).execute();
nodeSession.rollback();

var result = collection.find().execute();
print('Inserted Documents:', result.fetchAll().length);

//@ NodeSession: Transaction handling: commit
nodeSession.startTransaction();
var res1 = collection.add({ name: 'john', age: 15 }).execute();
var res2 = collection.add({ name: 'carol', age: 16 }).execute();
var res3 = collection.add({ name: 'alma', age: 17 }).execute();
nodeSession.commit();

var result = collection.find().execute();
print('Inserted Documents:', result.fetchAll().length);

nodeSession.dropSchema('node_session_schema');

//@ NodeSession: current schema validations: nodefault, mysql
nodeSession.setCurrentSchema('mysql');
dschema = nodeSession.getDefaultSchema();
cschema = nodeSession.getCurrentSchema();
print(dschema);
print(cschema);

//@ NodeSession: current schema validations: nodefault, information_schema
nodeSession.setCurrentSchema('information_schema');
dschema = nodeSession.getDefaultSchema();
cschema = nodeSession.getCurrentSchema();
print(dschema);
print(cschema);

//@ NodeSession: current schema validations: default
nodeSession.close()
nodeSession = mysqlx.getNodeSession(__uripwd + '/mysql');
dschema = nodeSession.getDefaultSchema();
cschema = nodeSession.getCurrentSchema();
print(dschema);
print(cschema);

//@ NodeSession: current schema validations: default, information_schema
nodeSession.setCurrentSchema('information_schema');
dschema = nodeSession.getDefaultSchema();
cschema = nodeSession.getCurrentSchema();
print(dschema);
print(cschema);

//@ NodeSession: setFetchWarnings(false)
nodeSession.setFetchWarnings(false);
var result = nodeSession.sql('drop database if exists unexisting').execute();
print(result.warningCount);

//@ NodeSession: setFetchWarnings(true)
nodeSession.setFetchWarnings(true);
var result = nodeSession.sql('drop database if exists unexisting').execute();
print(result.warningCount);
print(result.warnings[0].Message);

//@ NodeSession: quoteName no parameters
print(nodeSession.quoteName());

//@ NodeSession: quoteName wrong param type
print(nodeSession.quoteName(5));

//@ NodeSession: quoteName with correct parameters
print(nodeSession.quoteName('sample'));
print(nodeSession.quoteName('sam`ple'));
print(nodeSession.quoteName('`sample`'));
print(nodeSession.quoteName('`sample'));
print(nodeSession.quoteName('sample`'));

// Cleanup
nodeSession.close();