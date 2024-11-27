// Assumptions: ensure_schema_does_not_exist available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
var mysqlx = require('mysqlx');

var mySession = mysqlx.getSession(__uripwd);

ensure_schema_does_not_exist(mySession, 'js_shell_test');

mySession.createSchema('js_shell_test');
mySession.setCurrentSchema('js_shell_test');

var result;
result = mySession.sql('create table table1 (name varchar(50));').execute();
result = mySession.sql('create view view1 (my_name) as select name from table1;').execute();
result = mySession.getSchema('js_shell_test').createCollection('collection1');


var schema = mySession.getSchema('js_shell_test');

// We need to know the lower_case_table_names option to
// properly handle the table shadowing unit tests
var lcresult = mySession.sql('select @@lower_case_table_names').execute();
var lcrow = lcresult.fetchOne();
if (lcrow[0] == 1) {
    var name_get_table="gettable";
    var name_get_collection="getcollection";
} else {
    var name_get_table="getTable";
    var name_get_collection="getCollection";
}

//@<> Schema: validating members

validateMembers(schema, [
    'name',
    'schema',
    'session',
    'existsInDatabase',
    'getName',
    'getSchema',
    'getSession',
    'getTable',
    'getTables',
    'getCollection',
    'getCollections',
    'createCollection',
    'getCollectionAsTable',
    'modifyCollection',
    'help',
    'dropCollection',
    'table1',
    'view1',
    'collection1'])


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

//@ Query collection as table
print('getCollectionAsTable().select():', mySchema.getCollectionAsTable('collection1').select().execute());

//@ Collection creation
var collection = schema.createCollection('my_sample_collection');
print('createCollection():', collection);

//@<> Testing dropping existing schema objects
print(mySchema.getCollection('collection1'));
EXPECT_OUTPUT_CONTAINS("<Collection:collection1>");
EXPECT_EQ('undefined', String(mySchema.dropCollection('collection1')));

//@ Testing dropped objects are actually dropped
mySchema.getCollection('collection1');

//@<> Testing dropping non-existing schema objects
EXPECT_EQ('undefined', String(mySchema.dropCollection('non_existing_collection')));

//@ Testing drop functions using execute
mySchema.dropCollection('collection1').execute();

//@ Testing existence
print('Valid:', schema.existsInDatabase());
mySession.dropSchema('js_shell_test');
print('Invalid:', schema.existsInDatabase());

//@ Testing name shadowing: setup
mySession.createSchema('js_db_object_shadow');
mySession.setCurrentSchema('js_db_object_shadow');

collection_sql = "(`doc` json DEFAULT NULL,`_id` varchar(32) GENERATED ALWAYS AS (json_unquote(json_extract(doc, '$._id'))) STORED) ENGINE=InnoDB DEFAULT CHARSET=utf8"

mySession.sql('create table `name` (name varchar(50));');
mySession.sql('create table `schema` ' + collection_sql);
mySession.sql('create table `session` (name varchar(50));');
mySession.sql('create table `getTable` ' + collection_sql);
mySession.sql('create table `get_table` (name varchar(50));');
mySession.sql('create table `getCollection` ' + collection_sql);
mySession.sql('create table `get_collection` (name varchar(50));');
mySession.sql('create table `another` ' + collection_sql);

var schema = mySession.getSchema('js_db_object_shadow');

//@ Testing name shadowing: name
print(schema.name)

//@ Testing name shadowing: getName
print(schema.getName())

//@ Testing name shadowing: schema
print(schema.schema)

//@ Testing name shadowing: getSchema
print(schema.getSchema())

//@ Testing name shadowing: session
print(schema.session)

//@ Testing name shadowing: getSession
print(schema.getSession())

//@ Testing name shadowing: another
print(schema.another)

//@ Testing name shadowing: getCollection('another')
print(schema.getCollection('another'))

//@ Testing name shadowing: getTable('name')
print(schema.getTable('name'))

//@ Testing name shadowing: getCollection('schema')
print(schema.getCollection('schema'))

//@ Testing name shadowing: getTable('session')
print(schema.getTable('session'))

//@ Testing name shadowing: getCollection('getTable')
print(schema.getCollection('getTable'))

//@ Testing name shadowing: get_table (not a JS function)
print(schema.get_table)

//@ Testing name shadowing: getTable('get_table')
print(schema.getTable('get_table'))

//@ Testing name shadowing: getCollection('getCollection')
print(schema.getCollection('getCollection'))

//@ Testing name shadowing: get_collection (not a JS function)
print(schema.get_collection)

//@ Testing name shadowing: getTable('get_collection')
print(schema.getTable('get_collection'))

//@ cleanup
mySession.dropSchema('js_db_object_shadow')

// Closes the session
mySession.close();
