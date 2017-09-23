// Assumptions: validate_crud_functions available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
var mysqlx = require('mysqlx');

var mySession = mysqlx.getSession(__uripwd);

ensure_schema_does_not_exist(mySession, 'js_shell_test');

var schema = mySession.createSchema('js_shell_test');

// Creates a test collection and inserts data into it
var collection = schema.createCollection('collection1');

var result = collection.add({ name: 'jack', age: 17, gender: 'male' }).execute();
result = collection.add({ name: 'adam', age: 15, gender: 'male' }).execute();
result = collection.add({ name: 'brian', age: 14, gender: 'male' }).execute();
result = collection.add({ name: 'alma', age: 13, gender: 'female' }).execute();
result = collection.add({ name: 'carol', age: 14, gender: 'female' }).execute();
result = collection.add({ name: 'donna', age: 16, gender: 'female' }).execute();
result = collection.add({ name: 'angel', age: 14, gender: 'male' }).execute();

// ----------------------------------------------
// Collection.Find Unit Testing: Dynamic Behavior
// ----------------------------------------------
//@ CollectionFind: valid operations after find
var crud = collection.find();
validate_crud_functions(crud, ['fields', 'groupBy', 'sort', 'limit', 'lockShared', 'lockExclusive', 'bind', 'execute']);

//@ CollectionFind: valid operations after fields
var crud = crud.fields(['name']);
validate_crud_functions(crud, ['groupBy', 'sort', 'limit', 'lockShared', 'lockExclusive', 'bind', 'execute']);

//@ CollectionFind: valid operations after groupBy
var crud = crud.groupBy(['name']);
validate_crud_functions(crud, ['having', 'sort', 'limit', 'lockShared', 'lockExclusive', 'bind', 'execute']);

//@ CollectionFind: valid operations after having
var crud = crud.having('age > 10');
validate_crud_functions(crud, ['sort', 'limit', 'lockShared', 'lockExclusive', 'bind', 'execute']);

//@ CollectionFind: valid operations after sort
var crud = crud.sort(['age']);
validate_crud_functions(crud, ['limit', 'lockShared', 'lockExclusive', 'bind', 'execute']);

//@ CollectionFind: valid operations after limit
var crud = crud.limit(1);
validate_crud_functions(crud, ['skip', 'lockShared', 'lockExclusive', 'bind', 'execute']);

//@ CollectionFind: valid operations after skip
var crud = crud.skip(1);
validate_crud_functions(crud, ['lockShared', 'lockExclusive', 'bind', 'execute']);

//@ CollectionFind: valid operations after lockShared
var crud = crud = collection.find('name = :data').lockShared()
validate_crud_functions(crud, ['bind', 'execute']);

//@ CollectionFind: valid operations after lockExclusive
var crud = crud = collection.find('name = :data').lockExclusive()
validate_crud_functions(crud, ['bind', 'execute']);

//@ CollectionFind: valid operations after bind
var crud = crud = collection.find('name = :data').bind('data', 'adam')
validate_crud_functions(crud, ['bind', 'execute']);

//@ CollectionFind: valid operations after execute
var result = crud.execute();
validate_crud_functions(crud, ['bind', 'execute']);

//@ Reusing CRUD with binding
print(result.fetchOne().name + '\n');
var result = crud.bind('data', 'alma').execute();
print(result.fetchOne().name + '\n');

// ----------------------------------------------
// Collection.Find Unit Testing: Error Conditions
// ----------------------------------------------

//@# CollectionFind: Error conditions on find
crud = collection.find(5);
crud = collection.find('test = "2');

//@# CollectionFind: Error conditions on fields
crud = collection.find().fields();
crud = collection.find().fields(5);
crud = collection.find().fields([]);
crud = collection.find().fields(['name as alias', 5]);
crud = collection.find().fields(mysqlx.expr('concat(field, "whatever")'));
crud = collection.find().fields('name as alias', 5)

//@# CollectionFind: Error conditions on groupBy
crud = collection.find().groupBy();
crud = collection.find().groupBy(5);
crud = collection.find().groupBy([]);
crud = collection.find().groupBy(['name', 5]);
crud = collection.find().groupBy('name', 5)

//@# CollectionFind: Error conditions on having
crud = collection.find().groupBy(['name']).having();
crud = collection.find().groupBy(['name']).having(5);

//@# CollectionFind: Error conditions on sort
crud = collection.find().sort();
crud = collection.find().sort(5);
crud = collection.find().sort([]);
crud = collection.find().sort(['name', 5]);
crud = collection.find().sort('name', 5);

//@# CollectionFind: Error conditions on limit
crud = collection.find().limit();
crud = collection.find().limit('');

//@# CollectionFind: Error conditions on skip
crud = collection.find().limit(1).skip();
crud = collection.find().limit(1).skip('');

//@# CollectionFind: Error conditions on lockShared
crud = collection.find().lockShared(5);

//@# CollectionFind: Error conditions on lockExclusive
crud = collection.find().lockExclusive(5);

//@# CollectionFind: Error conditions on bind
crud = collection.find('name = :data and age > :years').bind();
crud = collection.find('name = :data and age > :years').bind(5, 5);
crud = collection.find('name = :data and age > :years').bind('another', 5);

//@# CollectionFind: Error conditions on execute
crud = collection.find('name = :data and age > :years').execute();
crud = collection.find('name = :data and age > :years').bind('years', 5).execute();

// ---------------------------------------
// Collection.Find Unit Testing: Execution
// ---------------------------------------
//@ Collection.Find All
//! [CollectionFind: All Records]
var records = collection.find().execute().fetchAll();
print("All:", records.length, "\n");
//! [CollectionFind: All Records]

//@ Collection.Find Filtering
//! [CollectionFind: Filtering]
var records = collection.find('gender = "male"').execute().fetchAll();
print("Males:", records.length, "\n");

var records = collection.find('gender = "female"').execute().fetchAll();
print("Females:", records.length, "\n");
//! [CollectionFind: Filtering]

var records = collection.find('age = 13').execute().fetchAll();
print("13 Years:", records.length, "\n");

var records = collection.find('age = 14').execute().fetchAll();
print("14 Years:", records.length, "\n");

var records = collection.find('age < 17').execute().fetchAll();
print("Under 17:", records.length, "\n");

var records = collection.find('name like "a%"').execute().fetchAll();
print("Names With A:", records.length, "\n");

//@ Collection.Find Field Selection
var result = collection.find().fields(['name', 'age']).execute();
var record = result.fetchOne();
var columns = dir(record)
print('1-Metadata Length:', columns.length, '\n');
print('1-Metadata Field:', columns[0], '\n');
print('1-Metadata Field:', columns[1], '\n');

var result = collection.find().fields(['age']).execute();
var record = result.fetchOne();
var columns = dir(record)
print('2-Metadata Length:', columns.length, '\n');
print('2-Metadata Field:', columns[0], '\n');

//@ Collection.Find Sorting
//! [CollectionFind: Sorting]
var records = collection.find().sort('name').execute().fetchAll();
for (index = 0; index < 7; index++) {
  print('Find Asc', index, ':', records[index].name, '\n');
}

var records = collection.find().sort(['name desc']).execute().fetchAll();
for (index = 0; index < 7; index++) {
  print('Find Desc', index, ':', records[index].name, '\n');
}
//! [CollectionFind: Sorting]

//@ Collection.Find Limit and Offset
//! [CollectionFind: Limit and Skip]
var records = collection.find().limit(4).execute().fetchAll();
print('Limit-Skip 0 :', records.length, '\n');

for (index = 1; index < 8; index++) {
  var records = collection.find().limit(4).skip(index).execute().fetchAll();
  print('Limit-Skip', index, ':', records.length, '\n');
}
//! [CollectionFind: Limit and Skip]

//@ Collection.Find Parameter Binding
//! [CollectionFind: Parameter Binding]
var records = collection.find('age = :years and gender = :heorshe').bind('years', 13).bind('heorshe', 'female').execute().fetchAll();
print('Find Binding Length:', records.length, '\n');
print('Find Binding Name:', records[0].name, '\n');
//! [CollectionFind: Parameter Binding]

//@ Collection.Find Field Selection Using Field List
//! [CollectionFind: Field Selection List]
var result = collection.find('name = "jack"').fields(['ucase(name) as FirstName', 'age as Age']).execute();
var record = result.fetchOne();
print('First Name:', record.FirstName, '\n');
print('Age:', record.Age, '\n');
//! [CollectionFind: Field Selection List]

//@ Collection.Find Field Selection Using Field Parameters
//! [CollectionFind: Field Selection Parameters]
var result = collection.find('name = "jack"').fields('ucase(name) as FirstName', 'age as Age').execute();
var record = result.fetchOne();
print('First Name:', record.FirstName, '\n');
print('Age:', record.Age, '\n');
//! [CollectionFind: Field Selection Parameters]

//@ Collection.Find Field Selection Using Projection Expression
//! [CollectionFind: Field Selection Projection]
var result = collection.find('name = "jack"').fields(mysqlx.expr('{"FirstName":ucase(name), "InThreeYears":age + 3}')).execute();
var record = result.fetchOne();
print('First Name:', record.FirstName, '\n');
print('In Three Years:', record.InThreeYears, '\n');
//! [CollectionFind: Field Selection Projection]

// Cleanup
mySession.dropSchema('js_shell_test');
mySession.close();
