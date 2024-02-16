// Assumptions: validate_crud_functions available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
var mysqlx = require('mysqlx');

var mySession = mysqlx.getSession(__uripwd);

ensure_schema_does_not_exist(mySession, 'js_shell_test');

var schema = mySession.createSchema('js_shell_test');

// Creates a test collection and inserts data into it
var collection = schema.createCollection('collection1');

var result = collection.add({ _id: '4C514FF38144B714E7119BCF48B4CA01', name: 'jack', age: 17, gender: 'male' }).execute();
result = collection.add({ _id: '4C514FF38144B714E7119BCF48B4CA02', name: 'adam', age: 15, gender: 'male' }).execute();
result = collection.add({ _id: '4C514FF38144B714E7119BCF48B4CA03', name: 'brian', age: 14, gender: 'male' }).execute();
result = collection.add({ _id: '4C514FF38144B714E7119BCF48B4CA04', name: 'alma', age: 13, gender: 'female' }).execute();
result = collection.add({ _id: '4C514FF38144B714E7119BCF48B4CA05', name: 'carol', age: 14, gender: 'female' }).execute();
result = collection.add({ _id: '4C514FF38144B714E7119BCF48B4CA06', name: 'donna', age: 16, gender: 'female' }).execute();
result = collection.add({ _id: '4C514FF38144B714E7119BCF48B4CA07', name: 'angel', age: 14, gender: 'male' }).execute();

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
validate_crud_functions(crud, ['offset', 'lockShared', 'lockExclusive', 'bind', 'execute']);

//@ CollectionFind: valid operations after offset
var crud = crud.offset(1);
validate_crud_functions(crud, ['lockShared', 'lockExclusive', 'bind', 'execute']);

//@ CollectionFind: valid operations after lockShared
var crud = collection.find('name = :data').lockShared()
validate_crud_functions(crud, ['bind', 'execute']);

//@ CollectionFind: valid operations after lockExclusive
var crud = collection.find('name = :data').lockExclusive()
validate_crud_functions(crud, ['bind', 'execute']);

//@ CollectionFind: valid operations after bind
var crud = collection.find('name = :data').bind('data', 'adam')
validate_crud_functions(crud, ['bind', 'execute']);

//@ CollectionFind: valid operations after execute
var result = crud.execute();
validate_crud_functions(crud, ['limit', 'bind', 'execute']);

//@ CollectionFind: valid operations after execute with limit
var result = crud.limit(1).bind('data', 'adam').execute();
validate_crud_functions(crud, ['limit', 'offset', 'bind', 'execute'])

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

//@# CollectionFind: Error conditions on offset
crud = collection.find().limit(1).offset();
crud = collection.find().limit(1).offset('');

//@# CollectionFind: Error conditions on lockShared
crud = collection.find().lockShared(5,1);
crud = collection.find().lockShared(5);

//@# CollectionFind: Error conditions on lockExclusive
crud = collection.find().lockExclusive(5,1);
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
//! [CollectionFind: Limit and Offset]
var records = collection.find().limit(4).execute().fetchAll();
print('Limit-Offset 0 :', records.length, '\n');

for (index = 1; index < 8; index++) {
  var records = collection.find().limit(4).offset(index).execute().fetchAll();
  print('Limit-Offset', index, ':', records.length, '\n');
}
//! [CollectionFind: Limit and Offset]

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

//@<> WL12813 Collection.Find Collection
var collection = schema.createCollection('wl12813');
result = collection.add({ _id: '4C514FF38144B714E7119BCF48B4CA01', like: 'foo', nested: { like: 'bar' } }).execute();
result = collection.add({ _id: '4C514FF38144B714E7119BCF48B4CA02', like: 'foo', nested: { like: 'nested bar' } }).execute();
result = collection.add({ _id: '4C514FF38144B714E7119BCF48B4CA03', like: 'top foo', nested: { like: 'bar' } }).execute();
result = collection.add({ _id: '4C514FF38144B714E7119BCF48B4CA04', like: 'top foo', nested: { like: 'nested bar' } }).execute();
result = collection.add({ _id: '4C514FF38144B714E7119BCF48B4CA05', like: 'bar', nested: { like: 'foo' } }).execute();
result = collection.add({ _id: '4C514FF38144B714E7119BCF48B4CA06', like: 'bar', nested: { like: 'nested foo' } }).execute();
result = collection.add({ _id: '4C514FF38144B714E7119BCF48B4CA07', like: 'top bar', nested: { like: 'foo' } }).execute();
result = collection.add({ _id: '4C514FF38144B714E7119BCF48B4CA08', like: 'top bar', nested: { like: 'nested foo' } }).execute();

//@ WL12813 Collection Test 01
// Collection.Find foo, like as column
collection.find('`like` = "foo"').execute()

//@ WL12813 Collection Test 02 [USE:WL12813 Collection Test 01]
// Collection.Find foo, unescaped like as column
collection.find('like = "foo"').execute()

//@ WL12813 Collection Test 03
// Collection.Find foo, nested like as column
collection.find('nested.`like` = "foo"').execute()

//@ WL12813 Collection Test 04
// Collection.Find % bar, like as column and operand
collection.find('`like` LIKE "%bar"').execute()

//@ WL12813 Collection Test 05 [USE:WL12813 Collection Test 04]
// Collection.Find % bar, like as unescaped column and operand
collection.find('like LIKE "%bar"').execute()

//@ WL12813 Collection Test 06
// Collection.Find % bar, nested like as column
collection.find('nested.`like` LIKE "%bar"').execute()

//@ WL12813 Collection Test 07 [USE:WL12813 Collection Test 04]
// Collection.Find like as column, operand and placeholder
collection.find('`like` LIKE :like').bind('like', '%bar').execute()

//@ WL12813 Collection Test 08 [USE:WL12813 Collection Test 04]
// Collection.Find unescaped like as column, operand and placeholder
collection.find('like LIKE :like').bind('like', '%bar').execute()

//@ WL12813 Collection Test 09 [USE:WL12813 Collection Test 06]
// Collection.Find, nested like as column, operand and placeholder
collection.find('nested.`like` LIKE :like').bind('like', '%bar').execute()

//@<> WL12767 Collection.Find Collection
collection = schema.createCollection('wl12767')
result = collection.add({ "_id": '4C514FF38144B714E7119BCF48B4CA01', "name":"one", "list": [1,2,3], "overlaps": [4,5,6] }).execute()
result = collection.add({ "_id": '4C514FF38144B714E7119BCF48B4CA02', "name":"two", "list": [1,2,3], "overlaps": [3,4,5] }).execute()
result = collection.add({ "_id": '4C514FF38144B714E7119BCF48B4CA03', "name":"three", "list": [1,2,3], "overlaps": [1,2,3,4,5,6] }).execute()
result = collection.add({ "_id": '4C514FF38144B714E7119BCF48B4CA04', "name":"four", "list": [1,2,3,4,5,6], "overlaps": [4,5,6] }).execute()
result = collection.add({ "_id": '4C514FF38144B714E7119BCF48B4CA05', "name":"five", "list": [1,2,3], "overlaps": [1,3,5] }).execute()
result = collection.add({ "_id": '4C514FF38144B714E7119BCF48B4CA06', "name":"six", "list": [1,2,3], "overlaps": [7,8,9] }).execute()
result = collection.add({ "_id": '4C514FF38144B714E7119BCF48B4CA07', "name":"seven", "list": [1,3,5], "overlaps": [2,4,6] }).execute()

//@ WL12767-TS1_1-01
// WL12767-TS6_1
collection.find("`overlaps` overlaps `list`").fields(["name"]).execute()

//@ WL12767-TS1_1-02 [USE:WL12767-TS1_1-01] {VER(>=8.0.17)}
// WL12767-TS3_1
collection.find("overlaps overlaps list").fields(["name"]).execute()

//@ WL12767-TS1_1-03 [USE:WL12767-TS1_1-01] {VER(>=8.0.17)}
collection.find("`list` overlaps `overlaps`").fields(["name"]).execute()

//@ WL12767-TS1_1-04 [USE:WL12767-TS1_1-01] {VER(>=8.0.17)}
// WL12767-TS3_1
collection.find("list overlaps overlaps").fields(["name"]).execute()

//@ WL12767-TS1_1-05 {VER(>=8.0.17)}
collection.find("`overlaps` not overlaps `list`").fields(["name"]).execute()

//@ WL12767-TS1_1-06 [USE:WL12767-TS1_1-05] {VER(>=8.0.17)}
// WL12767-TS2_1
// WL12767-TS3_1
collection.find("overlaps not OVERLAPS list").fields(["name"]).execute()

//@ WL12767-TS1_1-07 [USE:WL12767-TS1_1-05] {VER(>=8.0.17)}
collection.find("`list` not overlaps `overlaps`").fields(["name"]).execute()

//@ WL12767-TS1_1-08 [USE:WL12767-TS1_1-05] {VER(>=8.0.17)}
// WL12767-TS2_1
// WL12767-TS3_1
collection.find("list not OvErLaPs overlaps").fields(["name"]).execute()

//@ BUG29807711 VALID CAST EXPRESSIONS RESULT IN UNEXPECTED ERROR
var collection = schema.createCollection('bug29807711')
var result = collection.add({ "_id": '4C514FF38144B714E7119BCF48B4CA01', "name": 'foo', "age": 42 }).execute()
var result = collection.add({ "_id": '4C514FF38144B714E7119BCF48B4CA02', "name": 'bar', "age": 23 }).execute()

collection.find("CAST('42' AS UNSIGNED INTEGER) = age").execute()

// Cleanup
mySession.dropSchema('js_shell_test');
mySession.close();
