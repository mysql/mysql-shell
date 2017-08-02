// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
// validateMemer and validateNotMember are defined on the setup script
var mysqlx = require('mysqlx');

//@ Session: validating members
var mySession = mysqlx.getSession(__uripwd);
var mySessionMembers = dir(mySession);
validateMember(mySessionMembers, 'close');
validateMember(mySessionMembers, 'createSchema');
validateMember(mySessionMembers, 'getCurrentSchema');
validateMember(mySessionMembers, 'getDefaultSchema');
validateMember(mySessionMembers, 'getSchema');
validateMember(mySessionMembers, 'getSchemas');
validateMember(mySessionMembers, 'getUri');
validateMember(mySessionMembers, 'setCurrentSchema');
validateMember(mySessionMembers, 'setFetchWarnings');
validateMember(mySessionMembers, 'sql');
validateMember(mySessionMembers, 'defaultSchema');
validateMember(mySessionMembers, 'uri');
validateMember(mySessionMembers, 'currentSchema');

//@<OUT> Session: help
mySession.help();

//@ Session: accessing Schemas
var schemas = mySession.getSchemas();
print(getSchemaFromList(schemas, 'mysql'));
print(getSchemaFromList(schemas, 'information_schema'));

//@ Session: accessing individual schema
var schema;
schema = mySession.getSchema('mysql');
print(schema.name);
schema = mySession.getSchema('information_schema');
print(schema.name);

//@ Session: accessing unexisting schema
schema = mySession.getSchema('unexisting_schema');

//@ Session: current schema validations: nodefault
var dschema;
var cschema;
dschema = mySession.getDefaultSchema();
cschema = mySession.getCurrentSchema();
print(dschema);
print(cschema);

//@ Session: create schema success
ensure_schema_does_not_exist(mySession, 'node_session_schema');

ss = mySession.createSchema('node_session_schema');
print(ss)

//@ Session: create schema failure
var sf = mySession.createSchema('node_session_schema');

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

mySession.dropSchema('node_session_schema');

//@ Session: current schema validations: nodefault, mysql
mySession.setCurrentSchema('mysql');
dschema = mySession.getDefaultSchema();
cschema = mySession.getCurrentSchema();
print(dschema);
print(cschema);

//@ Session: current schema validations: nodefault, information_schema
mySession.setCurrentSchema('information_schema');
dschema = mySession.getDefaultSchema();
cschema = mySession.getCurrentSchema();
print(dschema);
print(cschema);

//@ Session: current schema validations: default
mySession.close()
mySession = mysqlx.getSession(__uripwd + '/mysql');
dschema = mySession.getDefaultSchema();
cschema = mySession.getCurrentSchema();
print(dschema);
print(cschema);

//@ Session: current schema validations: default, information_schema
mySession.setCurrentSchema('information_schema');
dschema = mySession.getDefaultSchema();
cschema = mySession.getCurrentSchema();
print(dschema);
print(cschema);

//@ Session: setFetchWarnings(false)
mySession.setFetchWarnings(false);
var result = mySession.sql('drop database if exists unexisting').execute();
print(result.warningCount);

//@ Session: setFetchWarnings(true)
mySession.setFetchWarnings(true);
var result = mySession.sql('drop database if exists unexisting').execute();
print(result.warningCount);
print(result.warnings[0].message);

//@ Session: quoteName no parameters
print(mySession.quoteName());

//@ Session: quoteName wrong param type
print(mySession.quoteName(5));

//@ Session: quoteName with correct parameters
print(mySession.quoteName('sample'));
print(mySession.quoteName('sam`ple'));
print(mySession.quoteName('`sample`'));
print(mySession.quoteName('`sample'));
print(mySession.quoteName('sample`'));

//@# Session: bad params
mysqlx.getSession()
mysqlx.getSession(42)
mysqlx.getSession(["bla"])
mysqlx.getSession(null)

// Cleanup
mySession.close();
