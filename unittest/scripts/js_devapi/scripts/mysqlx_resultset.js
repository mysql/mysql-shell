// Assumptions: ensure_schema_does_not_exist
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
var mysqlx = require('mysqlx');

var mySession = mysqlx.getSession(__uripwd);

ensure_schema_does_not_exist(mySession, 'js_shell_test');

var schema = mySession.createSchema('js_shell_test');
mySession.setCurrentSchema('js_shell_test');
var result = mySession.sql('create table js_shell_test.buffer_table (name varchar(50) primary key, age integer, gender varchar(20))').execute();

//@<> SqlResult member validation
validateMembers(result, [
    'executionTime',
    'warningCount',
    'warnings',
    'warningsCount',
    'getExecutionTime',
    'getWarningCount',
    'getWarnings',
    'getWarningsCount',
    'columnCount',
    'columnNames',
    'columns',
    'getColumnCount',
    'getColumnNames',
    'getColumns',
    'fetchOne',
    'fetchOneObject',
    'fetchAll',
    'help',
    'hasData',
    'nextDataSet',
    'nextResult',
    'affectedRowCount',
    'autoIncrementValue',
    'affectedItemsCount',
    'getAffectedRowCount',
    'getAffectedItemsCount',
    'getAutoIncrementValue'])

//@<> Result member validation
var table = schema.getTable('buffer_table');
var result = table.insert({ 'name': 'jack', 'age': 17, 'gender': 'male' }).execute();
var result = table.insert({ 'name': 'adam', 'age': 15, 'gender': 'male' }).execute();
var result = table.insert({ 'name': 'brian', 'age': 14, 'gender': 'male' }).execute();
var result = table.insert({ 'name': 'alma', 'age': 13, 'gender': 'female' }).execute();
var result = table.insert({ 'name': 'carol', 'age': 14, 'gender': 'female' }).execute();
var result = table.insert({ 'name': 'donna', 'age': 16, 'gender': 'female' }).execute();
var result = table.insert({ 'name': 'angel', 'age': 14, 'gender': 'male' }).execute();

var table = schema.getTable('buffer_table');
var collection = schema.createCollection('buffer_collection');

validateMembers(result, [
    'executionTime',
    'warningCount',
    'warnings',
    'warningsCount',
    'getExecutionTime',
    'getWarningCount',
    'getWarnings',
    'getWarningsCount',
    'affectedItemCount',
    'affectedItemsCount',
    'autoIncrementValue',
    'generatedIds',
    'help',
    'getAffectedItemCount',
    'getAffectedItemsCount',
    'getAutoIncrementValue',
    'getGeneratedIds'])

//@<> RowResult member validation
var result = table.select().execute();
validateMembers(result, [
    'affectedItemsCount',
    'executionTime',
    'warningCount',
    'warnings',
    'warningsCount',
    'getAffectedItemsCount',
    'getExecutionTime',
    'getWarningCount',
    'getWarnings',
    'getWarningsCount',
    'columnCount',
    'columnNames',
    'columns',
    'getColumnCount',
    'getColumnNames',
    'getColumns',
    'help',
    'fetchOne',
    'fetchOneObject',
    'fetchAll'])

//@<> DocResult member validation
var result = collection.find().execute();

validateMembers(result, [
    'affectedItemsCount',
    'executionTime',
    'warningCount',
    'warnings',
    'warningsCount',
    'getAffectedItemsCount',
    'getExecutionTime',
    'getWarningCount',
    'getWarnings',
    'getWarningsCount',
    'help',
    'fetchOne',
    'fetchAll'])


//@ Resultset hasData false
var result = mySession.sql('use js_shell_test;').execute();
print('hasData:', result.hasData());

//@ Resultset hasData true
var result = mySession.sql('select * from buffer_table;').execute();
print('hasData:', result.hasData());

//@ Resultset getColumns()
var metadata = result.getColumns();

print('Field Number:', metadata.length);
print('First Field:', metadata[0].columnName);
print('Second Field:', metadata[1].columnName);
print('Third Field:', metadata[2].columnName);

//@ Resultset columns
var metadata = result.columns;

print('Field Number:', metadata.length);
print('First Field:', metadata[0].columnName);
print('Second Field:', metadata[1].columnName);
print('Third Field:', metadata[2].columnName);

//@ Resultset buffering on SQL

var result1 = mySession.sql('select name, age from js_shell_test.buffer_table where gender = "male" order by name').execute();
var result2 = mySession.sql('select name, gender from js_shell_test.buffer_table where age < 15 order by name').execute();

var metadata1 = result1.columns;
var metadata2 = result2.columns;

print("Result 1 Field 1:", metadata1[0].columnName);
print("Result 1 Field 2:", metadata1[1].columnName);

print("Result 2 Field 1:", metadata2[0].columnName);
print("Result 2 Field 2:", metadata2[1].columnName);

var object1 = result1.fetchOneObject();
var object2 = result2.fetchOneObject();

print("Result 1 Record 1:", object1.name);
print("Result 2 Record 1:", object2["name"]);

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

//@ Rows as objects in SQL
println(object1)
println(object2)

//@ Resultset buffering on CRUD

var result1 = table.select(['name', 'age']).where('gender = :gender').orderBy(['name']).bind('gender', 'male').execute();
var result2 = table.select(['name', 'gender']).where('age < :age').orderBy(['name']).bind('age', 15).execute();

var metadata1 = result1.columns;
var metadata2 = result2.columns;

print("Result 1 Field 1:", metadata1[0].columnName);
print("Result 1 Field 2:", metadata1[1].columnName);

print("Result 2 Field 1:", metadata2[0].columnName);
print("Result 2 Field 2:", metadata2[1].columnName);

var object1 = result1.fetchOneObject();
var object2 = result2.fetchOneObject();

print("Result 1 Record 1:", object1.name);
print("Result 2 Record 1:", object2["name"]);

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

//@ Rows as objects in CRUD
println(object1)
println(object2)


//@ Resultset table
print(table.select(["count(*)"]).execute().fetchOne()[0]);

//@<> Resultset row members
var result = mySession.sql('select name as alias, age, age as length, gender as alias from buffer_table where name = "jack"').execute();
var row = result.fetchOne();

validateMembers(row, [
    'length',
    'getField',
    'getLength',
    'help',
    'alias',
    'age'])

//@ Resultset row index access
println("Name with index: " +  row[0]);
println("Age with index: " +  row[1]);
println("Length with index: " +  row[2]);
println("Gender with index: " +  row[3]);

//@ Resultset row getField access
println("Name with getField: " +  row.getField('alias'));
println("Age with getField: " +  row.getField('age'));
println("Length with getField: " +  row.getField('length'));
println("Unable to get gender from alias: " +  row.getField('alias'));

//@ Resultset property access
println("Name with property: " +  row.alias);
println("Age with property: " +  row.age);
println("Unable to get length with property: " +  row.length);

//@<> BUG#30825330 crash when SQL statement is executed after a stored procedure
mySession.sql('CREATE PROCEDURE my_proc() BEGIN SELECT name FROM js_shell_test.buffer_table; END;').execute();
var res = mySession.sql('CALL my_proc();').execute();
var res2 = mySession.sql('SELECT 1;').execute();

//@<> cleanup
mySession.close()
