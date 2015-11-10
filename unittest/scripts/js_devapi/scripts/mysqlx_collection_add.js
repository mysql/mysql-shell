// Assumptions: validate_crud_functions available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
var mysqlx = require('mysqlx').mysqlx;

var mySession = mysqlx.getNodeSession(__uripwd);

ensure_schema_does_not_exist(mySession, 'js_shell_test');
var schema = mySession.createSchema('js_shell_test');

// Creates a test collection and inserts data into it
var collection = schema.createCollection('collection1');

// ---------------------------------------------
// Collection.add Unit Testing: Dynamic Behavior
// ---------------------------------------------
//@ CollectionAdd: valid operations after add with no documents
var crud = collection.add([]);
validate_crud_functions(crud, ['add']);

//@ CollectionAdd: valid operations after add
var crud = collection.add({ name: "john", age: 17 });
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
crud = collection.add(mysqlx.expr('5+1'));

// ---------------------------------------
// Collection.Add Unit Testing: Execution
// ---------------------------------------
var records;

//@ Collection.add execution
var result = collection.add({ name: 'my first', passed: 'document', count: 1 }).execute();
print("Affected Rows Single:", result.affectedItemCount, "\n");

var result = collection.add([{ name: 'my second', passed: 'again', count: 2 }, { name: 'my third', passed: 'once again', count: 3 }]).execute();
print("Affected Rows List:", result.affectedItemCount, "\n");

var result = collection.add({ name: 'my fourth', passed: 'again', count: 4 }).add({ name: 'my fifth', passed: 'once again', count: 5 }).execute();
print("Affected Rows Chained:", result.affectedItemCount, "\n");

var result = collection.add(mysqlx.expr('{"name": "my fifth", "passed": "document", "count": 1}')).execute()
print("Affected Rows Single Expression:", result.affectedItemCount, "\n")

var result = collection.add([{ "name": 'my sexth', "passed": 'again', "count": 5 }, mysqlx.expr('{"name": "my senevth", "passed": "yep again", "count": 5}')]).execute()
print("Affected Rows Mixed List:", result.affectedItemCount, "\n")

// Cleanup
mySession.dropSchema('js_shell_test');
mySession.close();