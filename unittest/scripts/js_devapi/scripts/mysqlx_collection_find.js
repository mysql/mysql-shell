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

// ----------------------------------------------
// Collection.Find Unit Testing: Dynamic Behavior
// ----------------------------------------------
//@ CollectionFind: valid operations after find
var crud = collection.find();
validate_crud_functions(crud, ['fields', 'groupBy', 'sort', 'limit', 'bind', 'execute', '__shell_hook__']);

//@ CollectionFind: valid operations after fields
crud.fields(['name']);
validate_crud_functions(crud, ['groupBy', 'sort', 'limit', 'bind', 'execute', '__shell_hook__']);

//@ CollectionFind: valid operations after groupBy
crud.groupBy(['name']);
validate_crud_functions(crud, ['having', 'sort', 'limit', 'bind', 'execute', '__shell_hook__']);

//@ CollectionFind: valid operations after having
crud.having('age > 10');
validate_crud_functions(crud, ['sort', 'limit', 'bind', 'execute', '__shell_hook__']);

//@ CollectionFind: valid operations after sort
crud.sort(['age']);
validate_crud_functions(crud, ['limit', 'bind', 'execute', '__shell_hook__']);

//@ CollectionFind: valid operations after limit
crud.limit(1);
validate_crud_functions(crud, ['skip', 'bind', 'execute', '__shell_hook__']);

//@ CollectionFind: valid operations after skip
crud.skip(1);
validate_crud_functions(crud, ['bind', 'execute', '__shell_hook__']);

//@ CollectionFind: valid operations after bind
crud = collection.find('name = :data').bind('data', 'adam')
validate_crud_functions(crud, ['bind', 'execute', '__shell_hook__']);

//@ CollectionFind: valid operations after execute
var result = crud.execute();
validate_crud_functions(crud, ['bind', 'execute', '__shell_hook__']);

//@ Reusing CRUD with binding
print(result.next().name + '\n');
result=crud.bind('data', 'alma').execute();
print(result.next().name+ '\n');


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

//@# CollectionFind: Error conditions on groupBy
crud = collection.find().groupBy();
crud = collection.find().groupBy(5);
crud = collection.find().groupBy([]);
crud = collection.find().groupBy(['name', 5]);

//@# CollectionFind: Error conditions on having
crud = collection.find().groupBy(['name']).having();
crud = collection.find().groupBy(['name']).having(5);

//@# CollectionFind: Error conditions on sort
crud = collection.find().sort();
crud = collection.find().sort(5);
crud = collection.find().sort([]);
crud = collection.find().sort(['name', 5]);

//@# CollectionFind: Error conditions on limit
crud = collection.find().limit();
crud = collection.find().limit('');

//@# CollectionFind: Error conditions on skip
crud = collection.find().limit(1).skip();
crud = collection.find().limit(1).skip('');

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
var records;

//@ Collection.Find All
records = collection.find().execute().all();
print("All:", records.length, "\n");

//@ Collection.Find Filtering
records = collection.find('gender = "male"').execute().all();
print("Males:", records.length, "\n");

records = collection.find('gender = "female"').execute().all();
print("Females:", records.length, "\n");

records = collection.find('age = 13').execute().all();
print("13 Years:", records.length, "\n");

records = collection.find('age = 14').execute().all();
print("14 Years:", records.length, "\n");

records = collection.find('age < 17').execute().all();
print("Under 17:", records.length, "\n");

records = collection.find('name like "a%"').execute().all();
print("Names With A:", records.length, "\n");

//@ Collection.Find Field Selection
var columns;
result = collection.find().fields(['name','age']).execute();
record = result.next();
columns = dir(record)
print('1-Metadata Length:', columns.length, '\n');
print('1-Metadata Field:', columns[0], '\n');
print('1-Metadata Field:', columns[1], '\n');

result = collection.find().fields(['age']).execute();
record = result.next();
columns = dir(record)
print('2-Metadata Length:', columns.length, '\n');
print('2-Metadata Field:', columns[0], '\n');

//@ Collection.Find Sorting
records = collection.find().sort(['name']).execute().all();
for(index=0; index < 7; index++){
  print('Find Asc', index, ':', records[index].name, '\n');
}

records = collection.find().sort(['name desc']).execute().all();
for(index=0; index < 7; index++){
  print('Find Desc', index, ':', records[index].name, '\n');
}

//@ Collection.Find Limit and Offset
records = collection.find().limit(4).execute().all();
print('Limit-Skip 0 :', records.length, '\n');

for(index=1; index < 8; index++){
  records = collection.find().limit(4).skip(index).execute().all();
  print('Limit-Skip', index, ':', records.length, '\n');
}

//@ Collection.Find Parameter Binding
records = collection.find('age = :years and gender = :heorshe').bind('years', 13).bind('heorshe','female').execute().all();
print('Find Binding Length:', records.length, '\n');
print('Find Binding Name:', records[0].name, '\n');



// Cleanup
mySession.dropSchema('js_shell_test');
mySession.close();