// Assumptions: validate_crud_functions available

var mysqlx = require('mysqlx').mysqlx;

var uri = os.getenv('MYSQL_URI');

var mySession = mysqlx.getNodeSession(uri);

ensure_schema_does_not_exist(mySession, 'js_shell_test');

var schema = mySession.createSchema('js_shell_test');

// Creates a test collection and inserts data into it
var collection = schema.createCollection('collection1');

var result = collection.add({name: 'jack', age: 17, gender: 'male'}).execute();
result = collection.add({name: 'adam', age: 15, gender: 'male'}).execute();
result = collection.add({name: 'brian', age: 14, gender: 'male'}).execute();
result = collection.add({name: 'alma', age: 13, gender: 'female'}).execute();
result = collection.add({name: 'carol', age: 14, gender: 'female'}).execute();
result = collection.add({name: 'donna', age: 16, gender: 'female'}).execute();
result = collection.add({name: 'angel', age: 14, gender: 'male'}).execute();

// ------------------------------------------------
// collection.remove Unit Testing: Dynamic Behavior
// ------------------------------------------------
//@ CollectionRemove: valid operations after remove
var crud = collection.remove();
validate_crud_functions(crud, ['sort', 'limit', 'bind', 'execute', '__shell_hook__']);

//@ CollectionRemove: valid operations after sort
crud.sort(['name']);
validate_crud_functions(crud, ['limit', 'bind', 'execute', '__shell_hook__']);

//@ CollectionRemove: valid operations after limit
crud.limit(1);
validate_crud_functions(crud, ['bind', 'execute', '__shell_hook__']);

//@ CollectionRemove: valid operations after bind
crud = collection.remove('name = :data').bind('data', 'donna');
validate_crud_functions(crud, ['bind', 'execute', '__shell_hook__']);

//@ CollectionRemove: valid operations after execute
result = crud.execute();
validate_crud_functions(crud, ['bind', 'execute', '__shell_hook__']);

//@ Reusing CRUD with binding
print('Deleted donna:', result.affectedItemCount, '\n');
result=crud.bind('data', 'alma').execute();
print('Deleted alma:', result.affectedItemCount, '\n');


// ----------------------------------------------
// collection.remove Unit Testing: Error Conditions
// ----------------------------------------------

//@# CollectionRemove: Error conditions on remove
crud = collection.remove(5);
crud = collection.remove('test = "2');

//@# CollectionRemove: Error conditions sort
crud = collection.remove().sort();
crud = collection.remove().sort(5);
crud = collection.remove().sort([]);
crud = collection.remove().sort(['name', 5]);

//@# CollectionRemove: Error conditions on limit
crud = collection.remove().limit();
crud = collection.remove().limit('');

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
var docs;
result = collection.remove('age = 15').execute();
print('Affected Rows:', result.affectedItemCount, '\n');

docs = collection.find().execute().fetchAll();
print('Records Left:', docs.length, '\n');

//@ CollectionRemove: remove with binding
result = collection.remove('gender = :heorshe').limit(2).bind('heorshe', 'male').execute();
print('Affected Rows:', result.affectedItemCount, '\n');

docs = collection.find().execute().fetchAll();
print('Records Left:', docs.length, '\n');

//@ CollectionRemove: full remove
result = collection.remove().execute();
print('Affected Rows:', result.affectedItemCount, '\n');

docs = collection.find().execute().fetchAll();
print('Records Left:', docs.length, '\n');


// Cleanup
mySession.dropSchema('js_shell_test');
mySession.close();