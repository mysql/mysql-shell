// Assumptions: validate_crud_functions available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
var mysqlx = require('mysqlx');

var mySession = mysqlx.getSession(__uripwd);

ensure_schema_does_not_exist(mySession, 'js_shell_test');

var schema = mySession.createSchema('js_shell_test');
mySession.setCurrentSchema('js_shell_test');

// Creates a test table with initial data
var result = mySession.sql('create table table1 (name varchar(50), age integer, gender varchar(20), primary key (name, age, gender));').execute();
var result = mySession.sql('create view view1 (my_name, my_age, my_gender) as select name, age, gender from table1;').execute();
var table = schema.getTable('table1');

var result = table.insert({ name: 'jack', age: 17, gender: 'male' }).execute();
var result = table.insert({ name: 'adam', age: 15, gender: 'male' }).execute();
var result = table.insert({ name: 'brian', age: 14, gender: 'male' }).execute();
var result = table.insert({ name: 'alma', age: 13, gender: 'female' }).execute();
var result = table.insert({ name: 'carol', age: 14, gender: 'female' }).execute();
var result = table.insert({ name: 'donna', age: 16, gender: 'female' }).execute();
var result = table.insert({ name: 'angel', age: 14, gender: 'male' }).execute();

// ----------------------------------------------
// Table.Select Unit Testing: Dynamic Behavior
// ----------------------------------------------
//@ TableSelect: valid operations after select
var crud = table.select();
validate_crud_functions(crud, ['where', 'groupBy', 'orderBy', 'limit', 'lockShared', 'lockExclusive', 'bind', 'execute']);

//@ TableSelect: valid operations after where
var crud = crud.where('age > 13');
validate_crud_functions(crud, ['groupBy', 'orderBy', 'limit', 'lockShared', 'lockExclusive', 'bind', 'execute']);

//@ TableSelect: valid operations after groupBy
var crud = crud.groupBy(['name']);
validate_crud_functions(crud, ['having', 'orderBy', 'limit', 'lockShared', 'lockExclusive', 'bind', 'execute']);

//@ TableSelect: valid operations after having
var crud = crud.having('age > 10');
validate_crud_functions(crud, ['orderBy', 'limit', 'lockShared', 'lockExclusive', 'bind', 'execute']);

//@ TableSelect: valid operations after orderBy
var crud = crud.orderBy(['age']);
validate_crud_functions(crud, ['limit', 'lockShared', 'lockExclusive', 'bind', 'execute']);

//@ TableSelect: valid operations after limit
var crud = crud.limit(1);
validate_crud_functions(crud, ['offset', 'lockShared', 'lockExclusive', 'bind', 'execute']);

//@ TableSelect: valid operations after offset
var crud = crud.offset(1);
validate_crud_functions(crud, ['lockShared', 'lockExclusive', 'bind', 'execute']);

//@ TableSelect: valid operations after lockShared
var crud = table.select().where('name = :data').lockShared()
validate_crud_functions(crud, ['bind', 'execute']);

//@ TableSelect: valid operations after lockExclusive
var crud = table.select().where('name = :data').lockExclusive()
validate_crud_functions(crud, ['bind', 'execute']);

//@ TableSelect: valid operations after bind
var crud = table.select().where('name = :data').bind('data', 'adam')
validate_crud_functions(crud, ['bind', 'execute']);

//@ TableSelect: valid operations after execute
var result = crud.execute();
validate_crud_functions(crud, ['limit', 'bind', 'execute']);

//@ Reusing CRUD with binding
print(result.fetchOne().name + '\n');
var result = crud.bind('data', 'alma').execute();
print(result.fetchOne().name + '\n');

// ----------------------------------------------
// Table.Select Unit Testing: Error Conditions
// ----------------------------------------------

//@# TableSelect: Error conditions on select
crud = table.select(5);
crud = table.select([]);
crud = table.select(['name as alias', 5]);
crud = table.select('name as alias', 5);

//@# TableSelect: Error conditions on where
crud = table.select().where();
crud = table.select().where(5);
crud = table.select().where('name = "whatever');

//@# TableSelect: Error conditions on groupBy
crud = table.select().groupBy();
crud = table.select().groupBy(5);
crud = table.select().groupBy([]);
crud = table.select().groupBy(['name', 5]);
crud = table.select().groupBy('name', 5);

//@# TableSelect: Error conditions on having
crud = table.select().groupBy(['name']).having();
crud = table.select().groupBy(['name']).having(5);

//@# TableSelect: Error conditions on orderBy
crud = table.select().orderBy();
crud = table.select().orderBy(5);
crud = table.select().orderBy([]);
crud = table.select().orderBy(['name', 5]);
crud = table.select().orderBy('name', 5);

//@# TableSelect: Error conditions on limit
crud = table.select().limit();
crud = table.select().limit('');

//@# TableSelect: Error conditions on offset
crud = table.select().limit(1).offset();
crud = table.select().limit(1).offset('');

//@# TableSelect: Error conditions on lockShared
crud = table.select().lockShared(5,1);
crud = table.select().lockShared(5);

//@# TableSelect: Error conditions on lockExclusive
crud = table.select().lockExclusive(5,1);
crud = table.select().lockExclusive(5);

//@# TableSelect: Error conditions on bind
crud = table.select().where('name = :data and age > :years').bind();
crud = table.select().where('name = :data and age > :years').bind(5, 5);
crud = table.select().where('name = :data and age > :years').bind('another', 5);

//@# TableSelect: Error conditions on execute
crud = table.select().where('name = :data and age > :years').execute();
crud = table.select().where('name = :data and age > :years').bind('years', 5).execute();

// ---------------------------------------
// Table.Select Unit Testing: Execution
// ---------------------------------------
//@ Table.Select All
var records = table.select().execute().fetchAll();
print("All:", records.length, "\n");

//@ Table.Select Filtering
var records = table.select().where('gender = "male"').execute().fetchAll();
print("Males:", records.length, "\n");

var records = table.select().where('gender = "female"').execute().fetchAll();
print("Females:", records.length, "\n");

var records = table.select().where('age = 13').execute().fetchAll();
print("13 Years:", records.length, "\n");

var records = table.select().where('age = 14').execute().fetchAll();
print("14 Years:", records.length, "\n");

var records = table.select().where('age < 17').execute().fetchAll();
print("Under 17:", records.length, "\n");

var records = table.select().where('name like "a%"').execute().fetchAll();
print("Names With A:", records.length, "\n");

var records = table.select().where('name LIKE "a%"').execute().fetchAll();
print("Names With A:", records.length, "\n");

var records = table.select().where('NOT (age = 14)').execute().fetchAll();
print("Not 14 Years:", records.length, "\n");

//@ Table.Select Field Selection
var result = table.select(['name', 'age']).execute();
var record = result.fetchOne();
var columns = dir(record)
print(columns)
print('1-Metadata Length:', columns.length, '\n');
print('1-Metadata Field:', columns[1], '\n');
print('1-Metadata Field:', columns[2], '\n');

var result = table.select(['age']).execute();
var record = result.fetchOne();
var columns = dir(record)
print('2-Metadata Length:', columns.length, '\n');
print('2-Metadata Field:', columns[1], '\n');

//@ Table.Select Sorting
var records = table.select().orderBy(['name']).execute().fetchAll();
for (index = 0; index < 7; index++) {
  print('Select Asc', index, ':', records[index].name, '\n');
}

var records = table.select().orderBy(['name desc']).execute().fetchAll();
for (index = 0; index < 7; index++) {
  print('Select Desc', index, ':', records[index].name, '\n');
}

//@ Table.Select Limit and Offset
var records = table.select().limit(4).execute().fetchAll();
print('Limit-Offset 0 :', records.length, '\n');

for (index = 1; index < 8; index++) {
  var records = table.select().limit(4).offset(index).execute().fetchAll();
  print('Limit-Offset', index, ':', records.length, '\n');
}

//@ Table.Select Parameter Binding through a View
var view = schema.getTable('view1');
var records = view.select().where('my_age = :years and my_gender = :heorshe').bind('years', 13).bind('heorshe', 'female').execute().fetchAll();
print('Select Binding Length:', records.length, '\n');
print('Select Binding Name:', records[0].my_name, '\n');

//@ Table.Select Basic vertical output
shell.options.resultFormat = "vertical"
table.select('name');

//@ Table.Select Check column align in vertical mode
shell.options.resultFormat = "vertical"
table.select('*').where('age > 15');

//@ Table.Select Check row with newline in vertical mode
shell.options.resultFormat = "vertical"
table.insert({ name: 'john\ndoe', age: 13, gender: 'male' }).execute();
table.select('*').where('age = 13');

//@ Table.Select Check switching between table and vertical mode 1
shell.options.resultFormat = "vertical"
table.select('name').where('age > 16');
shell.options.resultFormat = "table"
//@<OUT> Table.Select Check switching between table and vertical mode 2
table.select('name').where('age > 16');

//@ Table.Select Zerofill field as variable
mySession.sql('create table table2 (value int(5) zerofill);').execute();
var table = schema.getTable('table2');
table.insert({ value: '1' }).execute();
table.insert({ value: '12' }).execute();
table.insert({ value: '12345' }).execute();
table.insert({ value: '123456789' }).execute();
var records = table.select().execute().fetchAll();
for (index = 0; index < 4; index++) {
  print('Variable value :', records[index].value, '\n');
}

//@ Table.Select Zerofill field display
mySession.sql('select * from table2');

//@<> WL12813 Collection.Find Collection
var result = mySession.sql('create table wl12813 (`like` varchar(50), `notnested.like` varchar(50));').execute();
var table = schema.getTable('wl12813');
result = table.insert().values('foo', 'bar').execute()
result = table.insert().values('foo', 'non nested bar').execute()
result = table.insert().values('bar', 'foo').execute()
result = table.insert().values('bar', 'non nested foo').execute()

//@ WL12813 Table Test 01
// Table.Select like as column
table.select().where('`like` = "foo"').execute()

//@ WL12813 Table Test 02 [USE:WL12813 Table Test 01]
// Table.Select unescaped like as column
table.select().where('like = "foo"').execute()

//@ WL12813 Table Test 03
// Table.Select escaped column
table.select().where('`notnested.like` = "foo"').execute()

//@ WL12813 Table Test 04
// Table.Select like as column and operator
table.select().where('`like` LIKE "%bar"').execute()

//@ WL12813 Table Test 05 [USE:WL12813 Table Test 04]
// Table.Select unescaped like as column and operator
table.select().where('like LIKE "%bar"').execute()

//@ WL12813 Table Test 06 [USE:WL12813 Table Test 01]
// Table.Select escaped column and like operator
table.select().where('`notnested.like` LIKE "%bar"').execute()

//@ WL12813 Table Test 07 [USE:WL12813 Table Test 04]
// Table.Select like as column, operator and placeholder
table.select().where('`like` LIKE :like').bind('like', '%bar').execute()

//@ WL12813 Table Test 08 [USE:WL12813 Table Test 04]
// Table.Select unescaped like as column, operator and placeholder
table.select().where('like LIKE :like').bind('like', '%bar').execute()

//@ WL12813 Table Test 09 [USE:WL12813 Table Test 01]
// Table.Select escaped column, with like as operator and placeholder
table.select().where('`notnested.like` LIKE :like').bind('like', '%bar').execute()

//@<> WL12767 Table.select {VER(>=8.0.17)}
var result = mySession.sql('create table wl12767 (`overlaps` JSON, `list` JSON,`Name` varchar(50));').execute()
var table = schema.getTable('wl12767')
result = table.insert().values('{"one":1, "two":2, "three":3}', '{"one":1,"two":2, "three":3}', "one").execute()
result = table.insert().values('{"one":1, "two":2, "three":3}', '{"four":4,"five":5, "six":6}', "two").execute()
result = table.insert().values('{"one":1, "three":3, "five":5}', '{"two":2,"four":4, "six":6}', "three").execute()
result = table.insert().values('{"one":1, "three":3, "five":5}', '{"three":3,"six":9, "nine":9}', "four").execute()
result = table.insert().values('{"one":1, "three":3, "five":5}', '{"three":6,"six":12, "nine":18}', "five").execute()
result = table.insert().values('{"one":[1,2,3]}', '{"one":[3,4,5]}',"six").execute()
result = table.insert().values('{"one":[1,2,3]}', '{"one":[1,2,3]}',"seven").execute()

//@ WL12767-TS1_1-01 {VER(>=8.0.17)}
// WL12767-TS6_1
table.select(["name"]).where("`overlaps` overlaps `list`").execute()

//@ WL12767-TS1_1-02 [USE:WL12767-TS1_1-01] {VER(>=8.0.17)}
// WL12767-TS3_1
table.select(["name"]).where("overlaps overlaps list").execute()

//@ WL12767-TS1_1-03 [USE:WL12767-TS1_1-01] {VER(>=8.0.17)}
table.select(["name"]).where("`list` overlaps `overlaps`").execute()

//@ WL12767-TS1_1-04 [USE:WL12767-TS1_1-01] {VER(>=8.0.17)}
// WL12767-TS3_1
table.select(["name"]).where("list overlaps overlaps").execute()

//@ WL12767-TS1_1-05 {VER(>=8.0.17)}
table.select(["name"]).where("`overlaps` not overlaps `list`").execute()

//@ WL12767-TS1_1-06 [USE:WL12767-TS1_1-05] {VER(>=8.0.17)}
// WL12767-TS2_1
// WL12767-TS3_1
table.select(["name"]).where("overlaps not OVERLAPS list").execute()

//@ WL12767-TS1_1-07 [USE:WL12767-TS1_1-05] {VER(>=8.0.17)}
table.select(["name"]).where("`list` not overlaps `overlaps`").execute()

//@ WL12767-TS1_1-08 [USE:WL12767-TS1_1-05] {VER(>=8.0.17)}
// WL12767-TS2_1
// WL12767-TS3_1
table.select(["name"]).where("list not OvErLaPs overlaps").execute()

//@ WL12767-TS5_1 {VER(>=8.0.17)}
table.select(["name"]).where("name OvErLaPs overlaps").execute()

//@<> BUG29794340 X DEVAPI: SUPPORT FOR JSON UNQUOTING EXTRACTION OPERATOR (->>)
shell.options['resultFormat'] = "tabbed"
mySession.sql("drop table if exists bug29794340").execute();
mySession.sql("create table bug29794340 (id integer primary key, doc json)").execute();
var n = schema.getTable("bug29794340");
n.insert("id", "doc").
  values(1, '{ "name": "Bob" }').
  values(2, '{ "name": "Jake" }').
  values(3, '{ "name": "Mark" }').
  execute();

//@<OUT> BUG29794340: Right arrow operator
n.select("doc->'$.name' as names");

//@<OUT> BUG29794340: Two head right arrow operator
n.select("doc->>'$.name' as names");

//@ BUG29794340: Expected token type QUOTE
n.select("doc->>>'$.name' as names");

// Cleanup
mySession.dropSchema('js_shell_test');
mySession.close();
