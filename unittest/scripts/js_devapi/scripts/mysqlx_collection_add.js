// Assumptions: validate_crud_functions available

var mysqlx = require('mysqlx').mysqlx;

var uri = os.getenv('MYSQL_URI');

var mySession = mysqlx.getNodeSession(uri);

ensure_schema_does_not_exist(mySession, 'js_shell_test');
var schema = mySession.createSchema('js_shell_test');

// Creates a test collection and inserts data into it
var collection = schema.createCollection('collection1');

// ---------------------------------------------
// Collection.add Unit Testing: Dynamic Behavior
// ---------------------------------------------
//@ CollectionAdd: valid operations after add
var crud = collection.add({name:"john", age:17});
validate_crud_functions(crud, ['add', 'execute', '__shell_hook__']);

//@ CollectionAdd: valid operations after execute
var result = crud.execute();
validate_crud_functions(crud, ['add', 'execute', '__shell_hook__']);


// ---------------------------------------------
// Collection.add Unit Testing: Error Conditions
// ---------------------------------------------

//@# CollectionAdd: Error conditions on add
crud = collection.add();
crud = collection.add(45);
crud = collection.add(['invalid data']);

// ---------------------------------------
// Collection.Add Unit Testing: Execution
// ---------------------------------------
var records;

//@ Collection.add execution
result = collection.add({name: 'my first', passed: 'document', count: 1}).execute();
print("Affected Rows Single:", result.affectedItemCount, "\n");

result = collection.add([{name: 'my second', passed: 'again', count: 2}, {name: 'my third', passed: 'once again', count: 3}]).execute();
print("Affected Rows List:", result.affectedItemCount, "\n");

result = collection.add({name: 'my fourth', passed: 'again', count: 4}).add({name: 'my fifth', passed: 'once again', count: 5}).execute();
print("Affected Rows Chained:", result.affectedItemCount, "\n");

// Cleanup
mySession.dropSchema('js_shell_test');
mySession.close();