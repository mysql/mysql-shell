// Assumptions: validate_crud_functions available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
var mysqlx = require('mysqlx').mysqlx;

var mySession = mysqlx.getNodeSession(__uripwd);

ensure_schema_does_not_exist(mySession, 'js_shell_test');
var schema = mySession.createSchema('js_shell_test');

// Creates a test collection and inserts data into it
var collection = schema.createCollection('collection1');

// --------------------------------------
// Create index, dynamic function testing
// --------------------------------------
//@ CollectionCreateIndex: valid operations after createIndex
var create_index = collection.createIndex('_name');
validate_crud_functions(create_index, ['field']);

//@ CollectionAdd: valid operations after field
create_index.field('name', "TEXT(50)", true);
validate_crud_functions(create_index, ['field', 'execute']);

//@ CollectionAdd: valid operations after execute
var result = create_index.execute();
validate_crud_functions(create_index, []);

// -----------------------------------
// Error conditions on chained methods
// -----------------------------------
//@# Error conditions on createIndex
create_index = collection.createIndex();
create_index = collection.createIndex(5);
create_index = collection.createIndex('_sample', 5);
create_index = collection.createIndex('_sample', mysqlx.Type.STRING)
create_index = collection.createIndex('_sample', mysqlx.IndexType.UNIQUE)

//@# Error conditions on field
create_index.field();
create_index.field(6, 6, 6);
create_index.field('other', 6, 6);
create_index.field('other', "INTEGER", 6);
create_index.field('other', "INTEGER", true);
mySession.dropCollection('js_shell_test', 'collection1');

// -----------------------------------
// Execution tests
// -----------------------------------
//@ NonUnique Index: creation with required field
ensure_schema_does_not_exist(mySession, 'js_shell_test');
schema = mySession.createSchema('js_shell_test');

// Creates a test collection and inserts data into it
collection = schema.createCollection('collection1');

var result = collection.createIndex('_name').field('name', "TEXT(50)", true).execute();

var result = collection.add({ name: 'John', last_name: 'Carter', age: 17 }).execute();
var result = collection.add({ name: 'John', last_name: 'Doe', age: 18 }).execute();
var result = collection.find('name="John"').execute();
var records = result.fetchAll();
print('John Records:', records.length);

//@ ERROR: insertion of document missing required field
var result = collection.add({ alias: 'Rock', last_name: 'Doe', age: 19 }).execute();

//@ ERROR: attempt to create an index with the same name
var result = collection.createIndex('_name').field('alias', "TEXT(50)", true).execute();

//@ ERROR: Attempt to create unique index when records already duplicate the key field
var result = collection.dropIndex('_name').execute();
var result = collection.createIndex('_name', mysqlx.IndexType.UNIQUE).field('name', "TEXT(50)", true).execute();

//@ ERROR: Attempt to create unique index when records are missing the key field
var result = collection.createIndex('_alias', mysqlx.IndexType.UNIQUE).field('alias', "TEXT(50)", true).execute();

//@ Unique index: creation with required field
var result = collection.remove().execute();
var result = collection.createIndex('_name', mysqlx.IndexType.UNIQUE).field('name', "TEXT(50)", true).execute();
var result = collection.add({ name: 'John', last_name: 'Carter', age: 17 }).execute();
var result = collection.add({ name: 'John', last_name: 'Doe', age: 18 }).execute();

//@ Cleanup
mySession.close();