// Assumptions: ensure_schema_does_not_exist
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
var mysqlx = require('mysqlx').mysqlx;

var mySession = mysqlx.getNodeSession(__uripwd);

ensure_schema_does_not_exist(mySession, 'js_shell_test');

var schema = mySession.createSchema('js_shell_test');
mySession.setCurrentSchema('js_shell_test');
mySession.sql('create table js_shell_test.buffer_table (name varchar(50), age integer, gender varchar(20))').execute();
var table = schema.getTable('buffer_table');
var result = table.insert({'name': 'jack', 'age': 17, 'gender': 'male'}).execute();
var result = table.insert({'name': 'adam', 'age': 15, 'gender': 'male'}).execute();
var result = table.insert({'name': 'brian', 'age': 14, 'gender': 'male'}).execute();
var result = table.insert({'name': 'alma', 'age': 13, 'gender': 'female'}).execute();
var result = table.insert({'name': 'carol', 'age': 14, 'gender': 'female'}).execute();
var result = table.insert({'name': 'donna', 'age': 16, 'gender': 'female'}).execute();
var result = table.insert({'name': 'angel', 'age': 14, 'gender': 'male'}).execute();

var table = schema.getTable('buffer_table');
var collection = schema.createCollection('buffer_collection');


//@ Resultset hasData false
var result = mySession.sql('use js_shell_test;').execute();
print('hasData:', result.hasData());

//@ Resultset hasData true
var result = mySession.sql('select * from buffer_table;').execute();
print('hasData:', result.hasData());


//@ Resultset getColumns()
var metadata = result.getColumns();

print('Field Number:', metadata.length);
print('First Field:', metadata[0].name);
print('Second Field:', metadata[1].name);
print('Third Field:', metadata[2].name);


//@ Resultset columns
var metadata = result.columns;

print('Field Number:', metadata.length);
print('First Field:', metadata[0].name);
print('Second Field:', metadata[1].name);
print('Third Field:', metadata[2].name);


//@ Resultset buffering on SQL

var result1 = mySession.sql('select name, age from js_shell_test.buffer_table where gender = "male" order by name').execute();
var result2 = mySession.sql('select name, gender from js_shell_test.buffer_table where age < 15 order by name').execute();

var metadata1 = result1.columns;
var metadata2 = result2.columns;

print("Result 1 Field 1:", metadata1[0].name);
print("Result 1 Field 2:", metadata1[1].name);

print("Result 2 Field 1:", metadata2[0].name);
print("Result 2 Field 2:", metadata2[1].name);


var record1 = result1.fetchOne();
var record2 = result2.fetchOne();

print("Result 1 Record 1:", record1.name);
print("Result 2 Record 1:", record2.name);

var record1 = result1.fetchOne();
var record2 = result2.fetchOne();

print("Result 1 Record 2:", record1.name);
print("Result 2 Record 2:", record2.name);

var record1 = result1.fetchOne();
var record2 = result2.fetchOne();

print("Result 1 Record 3:", record1.name);
print("Result 2 Record 3:", record2.name);

var record1 = result1.fetchOne();
var record2 = result2.fetchOne();

print("Result 1 Record 4:", record1.name);
print("Result 2 Record 4:", record2.name);


//@ Resultset buffering on CRUD

var result1 = table.select(['name', 'age']).where('gender = :gender').orderBy(['name']).bind('gender','male').execute();
var result2 = table.select(['name', 'gender']).where('age < :age').orderBy(['name']).bind('age',15).execute();

var metadata1 = result1.columns;
var metadata2 = result2.columns;

print("Result 1 Field 1:", metadata1[0].name);
print("Result 1 Field 2:", metadata1[1].name);

print("Result 2 Field 1:", metadata2[0].name);
print("Result 2 Field 2:", metadata2[1].name);


var record1 = result1.fetchOne();
var record2 = result2.fetchOne();

print("Result 1 Record 1:", record1.name);
print("Result 2 Record 1:", record2.name);

var record1 = result1.fetchOne();
var record2 = result2.fetchOne();

print("Result 1 Record 2:", record1.name);
print("Result 2 Record 2:", record2.name);

var record1 = result1.fetchOne();
var record2 = result2.fetchOne();

print("Result 1 Record 3:", record1.name);
print("Result 2 Record 3:", record2.name);

var record1 = result1.fetchOne();
var record2 = result2.fetchOne();

print("Result 1 Record 4:", record1.name);
print("Result 2 Record 4:", record2.name);

//@ Resultset table
print(table.select(["count(*)"]).execute().fetchOne()[0]);



mySession.close()
