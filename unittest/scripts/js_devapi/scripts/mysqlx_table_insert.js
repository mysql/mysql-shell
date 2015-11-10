// Assumptions: validate_crud_functions available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
var mysqlx = require('mysqlx').mysqlx;

var mySession = mysqlx.getNodeSession(__uripwd);

ensure_schema_does_not_exist(mySession, 'js_shell_test');

var schema = mySession.createSchema('js_shell_test');
mySession.setCurrentSchema('js_shell_test');

// Creates a test table
var result = mySession.sql('create table table1 (name varchar(50), age integer, gender varchar(20));').execute();

table = schema.getTable('table1');

// ---------------------------------------------
// Table.insert Unit Testing: Dynamic Behavior
// ---------------------------------------------
//@ TableInsert: valid operations after empty insert
var crud = table.insert();
validate_crud_functions(crud, ['values']);

//@ TableInsert: valid operations after empty insert and values
crud.values('john', 25, 'male');
validate_crud_functions(crud, ['values', 'execute', '__shell_hook__']);

//@ TableInsert: valid operations after empty insert and values 2
crud.values('alma', 23, 'female');
validate_crud_functions(crud, ['values', 'execute', '__shell_hook__']);

//@ TableInsert: valid operations after insert with field list
crud = table.insert(['name', 'age', 'gender']);
validate_crud_functions(crud, ['values']);

//@ TableInsert: valid operations after insert with field list and values
crud.values('john', 25, 'male');
validate_crud_functions(crud, ['values', 'execute', '__shell_hook__']);

//@ TableInsert: valid operations after insert with field list and values 2
crud.values('alma', 23, 'female');
validate_crud_functions(crud, ['values', 'execute', '__shell_hook__']);

//@ TableInsert: valid operations after insert with fields and values
crud = table.insert({ name: 'john', age: 25, gender: 'male' });
validate_crud_functions(crud, ['execute', '__shell_hook__']);

//@ TableInsert: valid operations after execute
result = crud.execute();
validate_crud_functions(crud, ['execute', '__shell_hook__']);

// -------------------------------------------
// Table.insert Unit Testing: Error Conditions
// -------------------------------------------

//@# TableInsert: Error conditions on insert
crud = table.insert(45);
crud = table.insert('name', 28);
crud = table.insert(['name', 28]);

crud = table.insert(['name', 'age', 'gender']).values([5]);
crud = table.insert(['name', 'age', 'gender']).values('carol', mySession);
crud = table.insert(['name', 'id', 'gender']).values('carol', 20, 'female').execute();

// ---------------------------------------
// Table.Find Unit Testing: Execution
// ---------------------------------------
var records;

//@ Table.insert execution
result = table.insert().values('jack', 17, 'male').execute();
print("Affected Rows No Columns:", result.affectedItemCount, "\n");

result = table.insert(['age', 'name', 'gender']).values(21, 'john', 'male').execute();
print("Affected Rows Columns:", result.affectedItemCount, "\n");

var insert = table.insert('name', 'age', 'gender')
insert.values('clark', 22, 'male')
insert.values('mary', 13, 'female')
result = insert.execute()
print("Affected Rows Multiple Values:", result.affectedItemCount, "\n");

result = table.insert({ 'age': 14, 'name': 'jackie', 'gender': 'female' }).execute();
print("Affected Rows Document:", result.affectedItemCount, "\n");

// Cleanup
mySession.dropSchema('js_shell_test');
mySession.close();