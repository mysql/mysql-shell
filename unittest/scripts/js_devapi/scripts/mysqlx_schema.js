// Assumptions: ensure_schema_does_not_exist available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
var mysqlx = require('mysqlx');

var mySession = mysqlx.getNodeSession(__uripwd);

ensure_schema_does_not_exist(mySession, 'js_shell_test');

mySession.createSchema('js_shell_test');
mySession.setCurrentSchema('js_shell_test');

var result;
result = mySession.sql('create table table1 (name varchar(50));').execute();
result = mySession.sql('create view view1 (my_name) as select name from table1;').execute();
result = mySession.getSchema('js_shell_test').createCollection('collection1');


var schema = mySession.getSchema('js_shell_test');

//@ Schema: validating members
var members = dir(schema);

print("Member Count:", members.length);

validateMember(members, 'name');
validateMember(members, 'schema');
validateMember(members, 'session');
validateMember(members, 'existsInDatabase');
validateMember(members, 'getName');
validateMember(members, 'getSchema');
validateMember(members, 'getSession');
validateMember(members, 'getTable');
validateMember(members, 'getTables');
validateMember(members, 'getCollection');
validateMember(members, 'getCollections');
validateMember(members, 'createCollection');
validateMember(members, 'getCollectionAsTable');

//Dynamic Properties
validateMember(members, 'table1');
validateMember(members, 'view1');
validateMember(members, 'collection1');


//@ Testing schema name retrieving
print('getName(): ' + schema.getName());
print('name: ' + schema.name);

//@ Testing schema.getSession
print('getSession():',schema.getSession());

//@ Testing schema.session
print('session:', schema.session);

//@ Testing schema schema retrieving
print('getSchema():', schema.getSchema());
print('schema:', schema.schema);

//@ Testing tables, views and collection retrieval
var mySchema = mySession.getSchema('js_shell_test');
print('getTables():', mySchema.getTables()[0]);
print('getCollections():', mySchema.getCollections()[0]);

//@ Testing specific object retrieval
print('Retrieving a table:', mySchema.getTable('table1'));
print('.<table>:', mySchema.table1);
print('Retrieving a view:', mySchema.getTable('view1'));
print('.<view>:', mySchema.view1);
print('getCollection():', mySchema.getCollection('collection1'));
print('.<collection>:', mySchema.collection1);

//@# Testing specific object retrieval: unexisting objects
mySchema.getTable('unexisting');
mySchema.getCollection('unexisting');

//@# Testing specific object retrieval: empty name
mySchema.getTable('');
mySchema.getCollection('');

//@ Retrieving collection as table
print('getCollectionAsTable():', mySchema.getCollectionAsTable('collection1'));

//@ Collection creation
var collection = schema.createCollection('my_sample_collection');
print('createCollection():', collection);

//@ Testing existence
print('Valid:', schema.existsInDatabase());
mySession.dropSchema('js_shell_test');
print('Invalid:', schema.existsInDatabase());

// Closes the session
mySession.close();