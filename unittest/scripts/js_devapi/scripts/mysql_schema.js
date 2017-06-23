// Assumptions: ensure_schema_does_not_exist available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<mysql_port>
var mysql = require('mysql');

var mySession = mysql.getClassicSession(__uripwd);

ensure_schema_does_not_exist(mySession, 'js_shell_test');

mySession.createSchema('js_shell_test');
mySession.setCurrentSchema('js_shell_test');

var result;
result = mySession.runSql('create table table1 (name varchar(50));');
result = mySession.runSql('create view view1 (my_name) as select name from table1;');

var schema = mySession.getSchema('js_shell_test');

// We need to know the lower_case_table_names option to
// properly handle the table shadowing unit tests
var lcresult = mySession.runSql('select @@lower_case_table_names')
var lcrow = lcresult.fetchOne();
if (lcrow[0] == 1) {
    var name_get_table="gettable";
} else {
    var name_get_table="getTable";
}

//@ Schema: validating members
var members = dir(schema);

print("Member Count:", members.length);

validateMember(members, 'name');
validateMember(members, 'schema');
validateMember(members, 'session');
validateMember(members, 'existsInDatabase');
validateMember(members, 'getName');
validateMember(members, 'getSchema');
validateMember(members, 'getSession');
validateMember(members, 'getTable');
validateMember(members, 'getTables');
validateMember(members, 'help');

//Dynamic Properties
validateMember(members, 'table1');
validateMember(members, 'view1');

//@ Testing schema name retrieving
print('getName(): ' + schema.getName());
print('name: ' + schema.name);

//@ Testing schema.getSession
print('getSession():', schema.getSession());

//@ Testing schema.session
print('session:', schema.session);

//@ Testing schema schema retrieving
print('getSchema():', schema.getSchema());
print('schema:', schema.schema);

//@ Testing tables, views and collection retrieval
var mySchema = mySession.getSchema('js_shell_test');
print('getTables():', mySchema.getTables()[0]);

//@ Testing specific object retrieval
print('Retrieving a table:', mySchema.getTable('table1'));
print('.<table>:', mySchema.table1);
print('Retrieving a view:', mySchema.getTable('view1'));
print('.<view>:', mySchema.view1);

//@ Testing existence
print('Valid:', schema.existsInDatabase());
mySession.dropSchema('js_shell_test');
print('Invalid:', schema.existsInDatabase());

//@ Testing name shadowing: setup
mySession.createSchema('js_db_object_shadow');
mySession.setCurrentSchema('js_db_object_shadow');
result = mySession.runSql('create table `name` (name varchar(50));');
result = mySession.runSql('create table `schema` (name varchar(50));');
result = mySession.runSql('create table `session` (name varchar(50));');
result = mySession.runSql('create table `getTable` (name varchar(50));');
result = mySession.runSql('create table `another` (name varchar(50));');

var schema = mySession.getSchema('js_db_object_shadow');

//@ Testing name shadowing: name
print(schema.name)

//@ Testing name shadowing: getName
print(schema.getName())

//@ Testing name shadowing: schema
print(schema.schema)

//@ Testing name shadowing: getSchema
print(schema.getSchema())

//@ Testing name shadowing: session
print(schema.session)

//@ Testing name shadowing: getSession
print(schema.getSession())

//@ Testing name shadowing: another
print(schema.another)

//@ Testing name shadowing: getTable('another')
print(schema.getTable('another'))

//@ Testing name shadowing: getTable('name')
print(schema.getTable('name'))

//@ Testing name shadowing: getTable('schema')
print(schema.getTable('schema'))

//@ Testing name shadowing: getTable('session')
print(schema.getTable('session'))

//@ Testing name shadowing: getTable('getTable')
print(schema.getTable('getTable'))

mySession.dropSchema('js_db_object_shadow');

// Closes the session
mySession.close();
