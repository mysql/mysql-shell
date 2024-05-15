// Assumptions: ensure_schema_does_not_exist
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<mysql_port>
var mysql = require('mysql');

var mySession = mysql.getClassicSession(__uripwd);

mySession.runSql('drop schema if exists js_shell_test');
mySession.runSql('create schema js_shell_test');
mySession.runSql('use js_shell_test');

//@<> Result member validation
var result = mySession.runSql('create table js_shell_test.buffer_table (name varchar(50) primary key, age integer, gender varchar(20))');

validateMembers(result, [
'affectedItemsCount',
'executionTime',
'statementId',
'warnings',
'warningsCount',
'getAffectedItemsCount',
'getExecutionTime',
'getStatementId',
'getWarnings',
'getWarningsCount',
'columnCount',
'columnNames',
'columns',
'info',
'getColumnCount',
'getColumnNames',
'getColumns',
'getInfo',
'fetchOne',
'fetchOneObject',
'fetchAll',
'hasData',
'nextResult',
'autoIncrementValue',
'getAutoIncrementValue'])

var result = mySession.runSql('insert into buffer_table values("jack", 17, "male")');
var result = mySession.runSql('insert into buffer_table values("adam", 15, "male")');
var result = mySession.runSql('insert into buffer_table values("brian", 14, "male")');
var result = mySession.runSql('insert into buffer_table values("alma", 13, "female")');
var result = mySession.runSql('insert into buffer_table values("carol", 14, "female")');
var result = mySession.runSql('insert into buffer_table values("donna", 16, "female")');
var result = mySession.runSql('insert into buffer_table values("angel", 14, "male")');

//@ Resultset hasData false
var result = mySession.runSql('use js_shell_test');
print('hasData:', result.hasData());

//@ Resultset hasData true
var result = mySession.runSql('select * from buffer_table');
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

//@<> Resultset row members
var result = mySession.runSql('select name as alias, age, age as length, gender as alias from buffer_table where name = "jack"');
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

//@ Resultset row as objects
var result = mySession.runSql('select name as alias, age, age as length, gender as alias from buffer_table where name = "jack"');
var object = result.fetchOneObject();
println("Alias with property: " +  object.alias);
println("Age as item: " +  object['age']);
println()
println(object)

//@ 0 dates from MySQL must be converted to None, since datetime don't like them
// Regression test for Bug #33621406
var result = mySession.runSql("set sql_mode=''")
var result = mySession.runSql("select cast('0000-00-00' as date), cast('0000-00-00 00:00:00' as datetime), cast('00:00:00' as time)")
var row = result.fetchOne()
println(row[0])
println(row[1])
println(row[2])

var result = mySession.runSql("select cast('2000-01-01' as date), cast('2000-01-01 01:02:03' as datetime), cast('2000-01-01 00:00:00' as datetime), cast('01:02:03' as time)")
var row = result.fetchOne()
println(row[0])
println(row[1])
println(row[2])
println(row[3])

mySession.runSql('drop schema if exists js_shell_test');
mySession.close()