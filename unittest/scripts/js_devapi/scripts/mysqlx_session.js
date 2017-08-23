// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
// validateMemer and validateNotMember are defined on the setup script
var mysqlx = require('mysqlx');

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
validateMember(nodeSessionMembers, 'uri');
validateMember(nodeSessionMembers, 'currentSchema');

//@<OUT> NodeSession: help
nodeSession.help();

//@ NodeSession: accessing Schemas
var schemas = nodeSession.getSchemas();
print(getSchemaFromList(schemas, 'mysql'));
print(getSchemaFromList(schemas, 'information_schema'));

//@ NodeSession: accessing individual schema
var schema;
schema = nodeSession.getSchema('mysql');
print(schema.name);
schema = nodeSession.getSchema('information_schema');
print(schema.name);

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
print(result.warnings[0].message);

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

//@# nodeSession: bad params
mysqlx.getNodeSession()
mysqlx.getNodeSession(42)
mysqlx.getNodeSession(["bla"])
mysqlx.getNodeSession(null)

// Cleanup
nodeSession.close();
