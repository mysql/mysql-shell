// Assumptions: validate_crud_functions available

var mysqlx = require('mysqlx').mysqlx;

var uri = os.getenv('MYSQL_URI');

var mySession = mysqlx.getNodeSession(uri);

ensure_schema_does_not_exist(mySession, 'js_shell_test');

var schema = mySession.createSchema('js_shell_test');
mySession.setCurrentSchema('js_shell_test');

// Creates a test table with initial data
var result = mySession.sql('create table table1 (name varchar(50), age integer, gender varchar(20));').execute();
table = schema.getTable('table1');

var result = table.insert({name: 'jack', age: 17, gender: 'male'}).execute();
result = table.insert({name: 'adam', age: 15, gender: 'male'}).execute();
result = table.insert({name: 'brian', age: 14, gender: 'male'}).execute();
result = table.insert({name: 'alma', age: 13, gender: 'female'}).execute();
result = table.insert({name: 'carol', age: 14, gender: 'female'}).execute();
result = table.insert({name: 'donna', age: 16, gender: 'female'}).execute();
result = table.insert({name: 'angel', age: 14, gender: 'male'}).execute();	

// ------------------------------------------------
// Table.Modify Unit Testing: Dynamic Behavior
// ------------------------------------------------
//@ TableUpdate: valid operations after update
var crud = table.update();
validate_crud_functions(crud, ['set']);

//@ TableUpdate: valid operations after set
crud.set('name', 'Jack');
validate_crud_functions(crud, ['set', 'where', 'orderBy', 'limit', 'bind', 'execute', '__shell_hook__']);

//@ TableUpdate: valid operations after where
crud.where("age < 100");
validate_crud_functions(crud, ['orderBy', 'limit', 'bind', 'execute', '__shell_hook__']);

//@ TableUpdate: valid operations after orderBy
crud.orderBy(['name']);
validate_crud_functions(crud, ['limit', 'bind', 'execute', '__shell_hook__']);

//@ TableUpdate: valid operations after limit
crud.limit(2);
validate_crud_functions(crud, ['bind', 'execute', '__shell_hook__']);

//@ TableUpdate: valid operations after bind
crud = table.update().set('age', 15).where('name = :data').bind('data', 'angel');
validate_crud_functions(crud, ['bind', 'execute', '__shell_hook__']);

//@ TableUpdate: valid operations after execute
result = crud.execute();
validate_crud_functions(crud, ['bind', 'execute', '__shell_hook__']);

//@ Reusing CRUD with binding
print('Updated Angel:', result.affectedRows, '\n');
result=crud.bind('data', 'carol').execute();
print('Updated Carol:', result.affectedRows, '\n');


// ----------------------------------------------
// Table.Modify Unit Testing: Error Conditions
// ----------------------------------------------

//@# TableUpdate: Error conditions on update
crud = table.update(5);

//@# TableUpdate: Error conditions on set
crud = table.update().set();
crud = table.update().set(45, 'whatever');
crud = table.update().set('name', mySession);


//@# TableUpdate: Error conditions on where
crud = table.update().set('age', 17).where();
crud = table.update().set('age', 17).where(5);
crud = table.update().set('age', 17).where('name = \"2');


//@# TableUpdate: Error conditions on orderBy
crud = table.update().set('age', 17).orderBy();
crud = table.update().set('age', 17).orderBy(5);
crud = table.update().set('age', 17).orderBy([]);
crud = table.update().set('age', 17).orderBy(['name', 5]);

//@# TableUpdate: Error conditions on limit
crud = table.update().set('age', 17).limit();
crud = table.update().set('age', 17).limit('');

//@# TableUpdate: Error conditions on bind
crud = table.update().set('age', 17).where('name = :data and age > :years').bind();
crud = table.update().set('age', 17).where('name = :data and age > :years').bind(5, 5);
crud = table.update().set('age', 17).where('name = :data and age > :years').bind('another', 5);

//@# TableUpdate: Error conditions on execute
crud = table.update().set('age', 17).where('name = :data and age > :years').execute();
crud = table.update().set('age', 17).where('name = :data and age > :years').bind('years', 5).execute();


// ---------------------------------------
// Table.Modify Unit Testing: Execution
// ---------------------------------------
var record;

//@# TableUpdate: simple test
result = result = table.update().set('name', 'aline').where('age = 13').execute();
print('Affected Rows:', result.affectedRows, '\n');

result = table.select().where('name = "aline"').execute();
record = result.next();
print("Updated Record:", record.name, record.age);

//@ TableUpdate: test using expression
result = table.update().set('age', mysqlx.expr('13+10')).where('age = 13').execute();
print('Affected Rows:', result.affectedRows, '\n');

result = table.select().where('age = 23').execute();
record = result.next();
print("Updated Record:", record.name, record.age);

//@ TableUpdate: test using limits
result = table.update().set('age', 16).where('age = 15').limit(2).execute();
print('Affected Rows:', result.affectedRows, '\n');

var records = table.select().where('age = 16').execute().all();
print('With 16 Years:', records.length, '\n');

var records = table.select().where('age = 15').execute().all();
print('With 15 Years:', records.length, '\n');

//@ TableUpdate: test full update
result = table.update().set('gender', 'female').execute();
print('Updated Females:', result.affectedRows, '\n');

records = table.select().where('gender = \"female\"').execute().all();
print('All Females:', records.length, '\n');

// Cleanup
schema.drop();
mySession.close();