// Assumptions: validate_crud_functions available

var mysqlx = require('mysqlx').mysqlx;

var uri = os.getenv('MYSQL_URI');

var session = mysqlx.getNodeSession(uri);

var schema;
try{
	// Ensures the js_shell_test does not exist
	schema = session.getSchema('js_shell_test');
	schema.drop();
}
catch(err)
{
}

schema = session.createSchema('js_shell_test');

// Creates a test collection and inserts data into it
var collection = schema.createCollection('collection1');

// ---------------------------------------------
// Collection.add Unit Testing: Dynamic Behavior
// ---------------------------------------------
//@ CollectionFind: valid operations after add
var crud = collection.add({name:"john", age:17});
validate_crud_functions(crud, ['add', 'execute', '__shell_hook__']);

//@ CollectionFind: valid operations after execute
var result = crud.execute();
validate_crud_functions(crud, ['execute', '__shell_hook__']);


// ---------------------------------------------
// Collection.add Unit Testing: Error Conditions
// ---------------------------------------------

//@# CollectionAdd: Error conditions on add
crud = collection.add();
crud = collection.add(45);
crud = collection.add(['invalid data']);

// ---------------------------------------
// Collection.Find Unit Testing: Execution
// ---------------------------------------
var records;

//@ Collection.add execution
result = collection.add({name: 'my first', passed: 'document', count: 1}).execute();
print("Affected Rows Single:", result.affectedRows, "\n");

result = collection.add([{name: 'my second', passed: 'again', count: 2}, {name: 'my third', passed: 'once again', count: 3}]).execute();
print("Affected Rows List:", result.affectedRows, "\n");

result = collection.add({name: 'my fourth', passed: 'again', count: 4}).add({name: 'my fifth', passed: 'once again', count: 5}).execute();
print("Affected Rows Chained:", result.affectedRows, "\n");

//@ Closes the session
schema.drop();
session.close();

