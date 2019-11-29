// Assumptions: validate_crud_functions available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
var mysqlx = require('mysqlx');

var mySession = mysqlx.getSession(__uripwd);

ensure_schema_does_not_exist(mySession, 'js_shell_test');

var schema = mySession.createSchema('js_shell_test');
mySession.setCurrentSchema('js_shell_test');

// Creates a test table
var result = mySession.sql('create table table1 (name varchar(50), age integer, gender varchar(20), primary key (name, age, gender));').execute();
var result = mySession.sql('create view view1 (my_name, my_age, my_gender) as select name, age, gender from table1;').execute();

table = schema.getTable('table1');

// ---------------------------------------------
// Table.insert Unit Testing: Dynamic Behavior
// ---------------------------------------------
//@ TableInsert: valid operations after empty insert
var crud = table.insert();
validate_crud_functions(crud, ['values']);

//@ TableInsert: valid operations after empty insert and values
var crud = crud.values('john', 25, 'male');
validate_crud_functions(crud, ['values', 'execute']);

//@ TableInsert: valid operations after empty insert and values 2
var crud = crud.values('alma', 23, 'female');
validate_crud_functions(crud, ['values', 'execute']);

//@ TableInsert: valid operations after insert with field list
var crud = table.insert(['name', 'age', 'gender']);
validate_crud_functions(crud, ['values']);

//@ TableInsert: valid operations after insert with field list and values
var crud = crud.values('john', 25, 'male');
validate_crud_functions(crud, ['values', 'execute']);

//@ TableInsert: valid operations after insert with field list and values 2
var crud = crud.values('alma', 23, 'female');
validate_crud_functions(crud, ['values', 'execute']);

//@ TableInsert: valid operations after insert with multiple fields
var crud = table.insert('name', 'age', 'gender');
validate_crud_functions(crud, ['values']);

//@ TableInsert: valid operations after insert with multiple fields and values
var crud = crud.values('john', 25, 'male');
validate_crud_functions(crud, ['values', 'execute']);

//@ TableInsert: valid operations after insert with multiple fields and values 2
var crud = crud.values('alma', 23, 'female');
validate_crud_functions(crud, ['values', 'execute']);

//@ TableInsert: valid operations after insert with fields and values
var crud = table.insert({ name: 'john', age: 25, gender: 'male' });
validate_crud_functions(crud, ['execute']);

//@ TableInsert: valid operations after execute
result = crud.execute();
validate_crud_functions(crud, ['execute']);

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
//! [TableInsert: insert()]
result = table.insert().values('jack', 17, 'male').execute();
print("Affected Rows No Columns:", result.affectedItemsCount, "\n");
//! [TableInsert: insert()]

//! [TableInsert: insert(list)]
result = table.insert(['age', 'name', 'gender']).values(21, 'john', 'male').execute();
print("Affected Rows Columns:", result.affectedItemsCount, "\n");
//! [TableInsert: insert(list)]

//! [TableInsert: insert(str...)]
var insert = table.insert('name', 'age', 'gender')
var insert = insert.values('clark', 22, 'male')
var insert = insert.values('mary', 13, 'female')
result = insert.execute()
print("Affected Rows Multiple Values:", result.affectedItemsCount, "\n");
//! [TableInsert: insert(str...)]

//! [TableInsert: insert(JSON)]
result = table.insert({ 'age': 14, 'name': 'jackie', 'gender': 'female' }).execute();
print("Affected Rows Document:", result.affectedItemsCount, "\n");
//! [TableInsert: insert(JSON)]

//@ Table.insert execution on a View
var view = schema.getTable('view1');
var result = view.insert({ 'my_age': 15, 'my_name': 'jhonny', 'my_gender': 'male' }).execute();
print("Affected Rows Through View:", result.affectedItemsCount, "\n");

// Cleanup
mySession.dropSchema('js_shell_test');
mySession.close();
