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
validate_crud_functions(crud, ['add', 'execute', '__shell_hook__']);

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
crud = collection.add({_id:45, name: 'sample'});

// ---------------------------------------
// Collection.Add Unit Testing: Execution
// ---------------------------------------
var records;

//@ Collection.add execution
var result = collection.add({ name: 'my first', Passed: 'document', count: 1 }).execute();
print("Affected Rows Single:", result.affectedItemCount, "\n");
print("lastDocumentId Single:", result.lastDocumentId);
print("getLastDocumentId Single:", result.getLastDocumentId());
print("#lastDocumentIds Single:", result.lastDocumentIds.length);
print("#getLastDocumentIds Single:", result.getLastDocumentIds().length);

var result = collection.add({ _id: "sample_document", name: 'my first', passed: 'document', count: 1 }).execute();
print("Affected Rows Single Known ID:", result.affectedItemCount, "\n");
print("lastDocumentId Single Known ID:", result.lastDocumentId);
print("getLastDocumentId Single Known ID:", result.getLastDocumentId());
print("#lastDocumentIds Single Known ID:", result.lastDocumentIds.length);
print("#getLastDocumentIds Single Known ID:", result.getLastDocumentIds().length);
print("#lastDocumentIds Single Known ID:", result.lastDocumentIds[0]);
print("#getLastDocumentIds Single Known ID:", result.getLastDocumentIds()[0]);

var result = collection.add([{ name: 'my second', passed: 'again', count: 2 }, { name: 'my third', passed: 'once again', count: 3 }]).execute();
print("Affected Rows Multi:", result.affectedItemCount, "\n");
try
{
  print("lastDocumentId Multi:", result.lastDocumentId);
}
catch(err)
{
  print("lastDocumentId Multi:", err.message, "\n");
}
try
{
  print("getLastDocumentId Multi:", result.getLastDocumentId());
}
catch(err)
{
  print("getLastDocumentId Multi:", err.message, "\n");
}

print("#lastDocumentIds Multi:", result.lastDocumentIds.length);
print("#getLastDocumentIds Multi:", result.getLastDocumentIds().length);

var result = collection.add([{ _id: "known_00", name: 'my second', passed: 'again', count: 2 }, { _id: "known_01", name: 'my third', passed: 'once again', count: 3 }]).execute();
print("Affected Rows Multi Known IDs:", result.affectedItemCount, "\n");
try
{
  print("lastDocumentId Multi Known IDs:", result.lastDocumentId);
}
catch(err)
{
  print("lastDocumentId Multi Known IDs:", err.message, "\n");
}
try
{
  print("getLastDocumentId Multi Known IDs:", result.getLastDocumentId());
}
catch(err)
{
  print("getLastDocumentId Multi Known IDs:", err.message, "\n");
}

print("#lastDocumentIds Multi Known IDs:", result.lastDocumentIds.length);
print("#getLastDocumentIds Multi Known IDs:", result.getLastDocumentIds().length);
print("First lastDocumentIds Multi Known IDs:", result.lastDocumentIds[0]);
print("First getLastDocumentIds Multi Known IDs:", result.getLastDocumentIds()[0]);
print("Second lastDocumentIds Multi Known IDs:", result.lastDocumentIds[1]);
print("Second getLastDocumentIds Multi Known IDs:", result.getLastDocumentIds()[1]);

var result = collection.add([]).execute();
print("Affected Rows Empty List:", result.affectedItemCount, "\n");
try
{
  print("lastDocumentId Empty List:", result.lastDocumentId);
}
catch(err)
{
  print("lastDocumentId Empty List:", err.message, "\n");
}
try
{
  print("getLastDocumentId Empty List:", result.getLastDocumentId());
}
catch(err)
{
  print("getLastDocumentId Empty List:", err.message, "\n");
}

print("#lastDocumentIds Empty List:", result.lastDocumentIds.length);
print("#getLastDocumentIds Empty List:", result.getLastDocumentIds().length);


var result = collection.add({ name: 'my fourth', passed: 'again', count: 4 }).add({ name: 'my fifth', passed: 'once again', count: 5 }).execute();
print("Affected Rows Chained:", result.affectedItemCount, "\n");

var result = collection.add(mysqlx.expr('{"name": "my fifth", "passed": "document", "count": 1}')).execute()
print("Affected Rows Single Expression:", result.affectedItemCount, "\n")

var result = collection.add([{ "name": 'my sexth', "passed": 'again', "count": 5 }, mysqlx.expr('{"name": "my senevth", "passed": "yep again", "count": 5}')]).execute()
print("Affected Rows Mixed List:", result.affectedItemCount, "\n")

// Cleanup
mySession.dropSchema('js_shell_test');
mySession.close();