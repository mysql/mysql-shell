// Assumptions: validate_crud_functions available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
var mysqlx = require('mysqlx');

var mySession = mysqlx.getSession(__uripwd);

ensure_schema_does_not_exist(mySession, 'js_shell_test');

var schema = mySession.createSchema('js_shell_test');

// Creates a test collection and inserts data into it
var collection = schema.createCollection('collection1');

var result = collection.add({ _id: '3C514FF38144B714E7119BCF48B4CA01', name: 'jack', age: 17, gender: 'male' }).execute();
var result = collection.add({ _id: '3C514FF38144B714E7119BCF48B4CA02', name: 'adam', age: 15, gender: 'male' }).execute();
var result = collection.add({ _id: '3C514FF38144B714E7119BCF48B4CA03', name: 'brian', age: 14, gender: 'male' }).execute();
var result = collection.add({ _id: '3C514FF38144B714E7119BCF48B4CA04', name: 'alma', age: 13, gender: 'female' }).execute();
var result = collection.add({ _id: '3C514FF38144B714E7119BCF48B4CA05', name: 'carol', age: 14, gender: 'female' }).execute();
var result = collection.add({ _id: '3C514FF38144B714E7119BCF48B4CA06', name: 'donna', age: 16, gender: 'female' }).execute();
var result = collection.add({ _id: '3C514FF38144B714E7119BCF48B4CA07', name: 'angel', age: 14, gender: 'male' }).execute();

// ------------------------------------------------
// collection.remove Unit Testing: Dynamic Behavior
// ------------------------------------------------
//@ CollectionRemove: valid operations after remove
var crud = collection.remove('some_filter');
validate_crud_functions(crud, ['sort', 'limit', 'bind', 'execute']);

//@ CollectionRemove: valid operations after sort
var crud = crud.sort(['name']);
validate_crud_functions(crud, ['limit', 'bind', 'execute']);

//@ CollectionRemove: valid operations after limit
var crud = crud.limit(1);
validate_crud_functions(crud, ['bind', 'execute']);

//@ CollectionRemove: valid operations after bind
var crud = collection.remove('name = :data').bind('data', 'donna');
validate_crud_functions(crud, ['bind', 'execute']);

//@ CollectionRemove: valid operations after execute
var result = crud.execute();
validate_crud_functions(crud, ['bind', 'execute']);

//@ Reusing CRUD with binding
print('Deleted donna:', result.affectedItemCount, '\n');
var result = crud.bind('data', 'alma').execute();
print('Deleted alma:', result.affectedItemCount, '\n');

// ----------------------------------------------
// collection.remove Unit Testing: Error Conditions
// ----------------------------------------------

//@# CollectionRemove: Error conditions on remove
crud = collection.remove();
crud = collection.remove('    ');
crud = collection.remove(5);
crud = collection.remove('test = "2');

//@# CollectionRemove: Error conditions sort
crud = collection.remove('some_filter').sort();
crud = collection.remove('some_filter').sort(5);
crud = collection.remove('some_filter').sort([]);
crud = collection.remove('some_filter').sort(['name', 5]);
crud = collection.remove('some_filter').sort('name', 5);

//@# CollectionRemove: Error conditions on limit
crud = collection.remove('some_filter').limit();
crud = collection.remove('some_filter').limit('');

//@# CollectionRemove: Error conditions on bind
crud = collection.remove('name = :data and age > :years').bind();
crud = collection.remove('name = :data and age > :years').bind(5, 5);
crud = collection.remove('name = :data and age > :years').bind('another', 5);

//@# CollectionRemove: Error conditions on execute
crud = collection.remove('name = :data and age > :years').execute();
crud = collection.remove('name = :data and age > :years').bind('years', 5).execute()

// ---------------------------------------
// collection.remove Unit Testing: Execution
// ---------------------------------------

//@ CollectionRemove: remove under condition
//! [CollectionRemove: remove under condition]
var result = collection.remove('age = 15').execute();
print('Affected Rows:', result.affectedItemCount, '\n');

var docs = collection.find().execute().fetchAll();
print('Records Left:', docs.length, '\n');
//! [CollectionRemove: remove under condition]

//@ CollectionRemove: remove with binding
//! [CollectionRemove: remove with binding]
var result = collection.remove('gender = :heorshe').limit(2).bind('heorshe', 'male').execute();
print('Affected Rows:', result.affectedItemCount, '\n');

var docs = collection.find().execute().fetchAll();
print('Records Left:', docs.length, '\n');
//! [CollectionRemove: remove with binding]

//@ CollectionRemove: full remove
//! [CollectionRemove: full remove]
var result = collection.remove('1').execute();
print('Affected Rows:', result.affectedItemCount, '\n');

var docs = collection.find().execute().fetchAll();
print('Records Left:', docs.length, '\n');
//! [CollectionRemove: full remove]

// Cleanup
mySession.dropSchema('js_shell_test');
mySession.close();
