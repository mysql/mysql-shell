// Assumptions: ensure_schema_does_not_exist
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<mysql_port>
var mysql = require('mysql');

var mySession = mysql.getClassicSession(__uripwd);

ensure_schema_does_not_exist(mySession, 'js_shell_test');

var schema = mySession.createSchema('js_shell_test');
mySession.setCurrentSchema('js_shell_test');

//@ Result member validation
var result = mySession.runSql('create table js_shell_test.buffer_table (name varchar(50) primary key, age integer, gender varchar(20))');
var members = dir(result);
print ("SqlResult Members:" + members);
validateMember(members, 'executionTime');
validateMember(members, 'warningCount');
validateMember(members, 'warnings');
validateMember(members, 'getExecutionTime');
validateMember(members, 'getWarningCount');
validateMember(members, 'getWarnings');
validateMember(members, 'columnCount');
validateMember(members, 'columnNames');
validateMember(members, 'columns');
validateMember(members, 'info');
validateMember(members, 'getColumnCount');
validateMember(members, 'getColumnNames');
validateMember(members, 'getColumns');
validateMember(members, 'getInfo');
validateMember(members, 'fetchOne');
validateMember(members, 'fetchAll');
validateMember(members, 'hasData');
validateMember(members, 'nextDataSet');
validateMember(members, 'affectedRowCount');
validateMember(members, 'autoIncrementValue');
validateMember(members, 'getAffectedRowCount');
validateMember(members, 'getAutoIncrementValue');

var table = schema.getTable('buffer_table');

var result = mySession.runSql('insert into buffer_table values("jack", 17, "male")');
var result = mySession.runSql('insert into buffer_table values("adam", 15, "male")');
var result = mySession.runSql('insert into buffer_table values("brian", 14, "male")');
var result = mySession.runSql('insert into buffer_table values("alma", 13, "female")');
var result = mySession.runSql('insert into buffer_table values("carol", 14, "female")');
var result = mySession.runSql('insert into buffer_table values("donna", 16, "female")');
var result = mySession.runSql('insert into buffer_table values("angel", 14, "male")');

var table = schema.getTable('buffer_table');

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

//@ Resultset row members
var result = mySession.runSql('select name as alias, age, age as length, gender as alias from buffer_table where name = "jack"');
var row = result.fetchOne();
var members = dir(row);
println("Member Count: " + members.length);
validateMember(members, 'length');
validateMember(members, 'getField');
validateMember(members, 'getLength');
validateMember(members, 'help');
validateMember(members, 'alias');
validateMember(members, 'age');

// Resultset row index access
println("Name with index: " +  row[0]);
println("Age with index: " +  row[1]);
println("Length with index: " +  row[2]);
println("Gender with index: " +  row[3]);

// Resultset row index access
println("Name with getField: " +  row.getField('alias'));
println("Age with getField: " +  row.getField('age'));
println("Length with getField: " +  row.getField('length'));
println("Unable to get gender from alias: " +  row.getField('alias'));

// Resultset property access
println("Name with property: " +  row.alias);
println("Age with property: " +  row.age);
println("Unable to get length with property: " +  row.length);

mySession.close()
