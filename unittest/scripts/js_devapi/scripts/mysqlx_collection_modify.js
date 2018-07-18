// Assumptions: validate_crud_functions available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
var mysqlx = require('mysqlx');

var mySession = mysqlx.getSession(__uripwd);

ensure_schema_does_not_exist(mySession, 'js_shell_test');

var schema = mySession.createSchema('js_shell_test');

// Creates a test collection and inserts data into it
var collection = schema.createCollection('collection1');

var result = collection.add({ _id: '5C514FF38144957BE71111C04E0D1246', name: 'jack', age: 17, gender: 'male' }).execute();
var result = collection.add({ _id: '5C514FF38144957BE71111C04E0D1247', name: 'adam', age: 15, gender: 'male' }).execute();
var result = collection.add({ _id: '5C514FF38144957BE71111C04E0D1248', name: 'brian', age: 14, gender: 'male' }).execute();
var result = collection.add({ _id: '5C514FF38144957BE71111C04E0D1249', name: 'alma', age: 13, gender: 'female' }).execute();
var result = collection.add({ _id: '5C514FF38144957BE71111C04E0D1250', name: 'carol', age: 14, gender: 'female' }).execute();
var result = collection.add({ _id: '5C514FF38144957BE71111C04E0D1251', name: 'donna', age: 16, gender: 'female' }).execute();
var result = collection.add({ _id: '5C514FF38144957BE71111C04E0D1252', name: 'angel', age: 14, gender: 'male' }).execute();

// ------------------------------------------------
// Collection.Modify Unit Testing: Dynamic Behavior
// ------------------------------------------------
//@ CollectionModify: valid operations after modify and set
var crud = collection.modify('some_filter');
validate_crud_functions(crud, ['set', 'unset', 'merge', 'patch', 'arrayInsert', 'arrayAppend', 'arrayDelete']);
var crud = crud.set('name', 'dummy');
validate_crud_functions(crud, ['set', 'unset', 'merge', 'patch', 'arrayInsert', 'arrayAppend', 'arrayDelete', 'sort', 'limit', 'bind', 'execute']);

//@ CollectionModify: valid operations after modify and unset empty
var crud = collection.modify('some_filter');
validate_crud_functions(crud, ['set', 'unset', 'merge', 'patch', 'arrayInsert', 'arrayAppend', 'arrayDelete']);
var crud = crud.unset([]);
validate_crud_functions(crud, ['set', 'unset', 'merge', 'patch', 'arrayInsert', 'arrayAppend', 'arrayDelete']);

//@ CollectionModify: valid operations after modify and unset list
var crud = collection.modify('some_filter');
validate_crud_functions(crud, ['set', 'unset', 'merge', 'patch', 'arrayInsert', 'arrayAppend', 'arrayDelete']);
var crud = crud.unset(['name', 'type']);
validate_crud_functions(crud, ['set', 'unset', 'merge', 'patch', 'arrayInsert', 'arrayAppend', 'arrayDelete', 'sort', 'limit', 'bind', 'execute']);

//@ CollectionModify: valid operations after modify and unset multiple params
var crud = collection.modify('some_filter');
validate_crud_functions(crud, ['set', 'unset', 'merge', 'patch', 'arrayInsert', 'arrayAppend', 'arrayDelete']);
var crud = crud.unset('name', 'type');
validate_crud_functions(crud, ['set', 'unset', 'merge', 'patch', 'arrayInsert', 'arrayAppend', 'arrayDelete', 'sort', 'limit', 'bind', 'execute']);

//@ CollectionModify: valid operations after modify and merge
var crud = collection.modify('some_filter');
validate_crud_functions(crud, ['set', 'unset', 'merge', 'patch', 'arrayInsert', 'arrayAppend', 'arrayDelete']);
var crud = crud.merge({ 'att': 'value', 'second': 'final' });
validate_crud_functions(crud, ['set', 'unset', 'merge', 'patch', 'arrayInsert', 'arrayAppend', 'arrayDelete', 'sort', 'limit', 'bind', 'execute']);

//@ CollectionModify: valid operations after modify and patch
var crud = collection.modify('some_filter');
validate_crud_functions(crud, ['set', 'unset', 'merge', 'patch', 'arrayInsert', 'arrayAppend', 'arrayDelete']);
var crud = crud.patch({ 'att': 'value', 'second': 'final' });
validate_crud_functions(crud, ['set', 'unset', 'merge', 'patch', 'arrayInsert', 'arrayAppend', 'arrayDelete', 'sort', 'limit', 'bind', 'execute']);

//@ CollectionModify: valid operations after modify and arrayInsert
var crud = collection.modify('some_filter');
validate_crud_functions(crud, ['set', 'unset', 'merge', 'patch', 'arrayInsert', 'arrayAppend', 'arrayDelete']);
var crud = crud.arrayInsert('hobbies[3]', 'run');
validate_crud_functions(crud, ['set', 'unset', 'merge', 'patch', 'arrayInsert', 'arrayAppend', 'arrayDelete', 'sort', 'limit', 'bind', 'execute']);

//@ CollectionModify: valid operations after modify and arrayAppend
var crud = collection.modify('some_filter');
validate_crud_functions(crud, ['set', 'unset', 'merge', 'patch', 'arrayInsert', 'arrayAppend', 'arrayDelete']);
var crud = crud.arrayAppend('hobbies', 'skate');
validate_crud_functions(crud, ['set', 'unset', 'merge', 'patch', 'arrayInsert', 'arrayAppend', 'arrayDelete', 'sort', 'limit', 'bind', 'execute']);

//@ CollectionModify: valid operations after modify and arrayDelete
var crud = collection.modify('some_filter');
validate_crud_functions(crud, ['set', 'unset', 'merge', 'patch', 'arrayInsert', 'arrayAppend', 'arrayDelete']);
var crud = crud.arrayDelete('hobbies[5]')
validate_crud_functions(crud, ['set', 'unset', 'merge', 'patch', 'arrayInsert', 'arrayAppend', 'arrayDelete', 'sort', 'limit', 'bind', 'execute']);

//@ CollectionModify: valid operations after sort
var crud = crud.sort(['name']);
validate_crud_functions(crud, ['limit', 'bind', 'execute']);

//@ CollectionModify: valid operations after limit
var crud = crud.limit(2);
validate_crud_functions(crud, ['bind', 'execute']);

//@ CollectionModify: valid operations after bind
var crud = collection.modify('name = :data').set('age', 15).bind('data', 'angel');
validate_crud_functions(crud, ['bind', 'execute']);

//@ CollectionModify: valid operations after execute
var result = crud.execute();
validate_crud_functions(crud, ['bind', 'execute']);

//@ Reusing CRUD with binding
print('Updated Angel:', result.affectedItemsCount, '\n');
var result = crud.bind('data', 'carol').execute();
print('Updated Carol:', result.affectedItemsCount, '\n');

// ----------------------------------------------
// Collection.Modify Unit Testing: Error Conditions
// ----------------------------------------------

//@# CollectionModify: Error conditions on modify
crud = collection.modify();
crud = collection.modify('   ');
crud = collection.modify(5);
crud = collection.modify('test = "2');

//@# CollectionModify: Error conditions on set
crud = collection.modify('some_filter').set();
crud = collection.modify('some_filter').set(45, 'whatever');

//@# CollectionModify: Error conditions on unset
crud = collection.modify('some_filter').unset();
crud = collection.modify('some_filter').unset({});
crud = collection.modify('some_filter').unset({}, '');
crud = collection.modify('some_filter').unset(['name', 1]);
crud = collection.modify('some_filter').unset('');

//@# CollectionModify: Error conditions on merge
crud = collection.modify('some_filter').merge();
crud = collection.modify('some_filter').merge('');

//@# CollectionModify: Error conditions on patch
crud = collection.modify('some_filter').patch();
crud = collection.modify('some_filter').patch('');

//@# CollectionModify: Error conditions on arrayInsert
crud = collection.modify('some_filter').arrayInsert();
crud = collection.modify('some_filter').arrayInsert(5, 'another');
crud = collection.modify('some_filter').arrayInsert('', 'another');
crud = collection.modify('some_filter').arrayInsert('test', 'another');

//@# CollectionModify: Error conditions on arrayAppend
crud = collection.modify('some_filter').arrayAppend();
crud = collection.modify('some_filter').arrayAppend({}, '');
crud = collection.modify('some_filter').arrayAppend('', 45);
crud = collection.modify('some_filter').arrayAppend('data', mySession);

//@# CollectionModify: Error conditions on arrayDelete
crud = collection.modify('some_filter').arrayDelete();
crud = collection.modify('some_filter').arrayDelete(5);
crud = collection.modify('some_filter').arrayDelete('');
crud = collection.modify('some_filter').arrayDelete('test');

//@# CollectionModify: Error conditions on sort
crud = collection.modify('some_filter').unset('name').sort();
crud = collection.modify('some_filter').unset('name').sort(5);
crud = collection.modify('some_filter').unset('name').sort([]);
crud = collection.modify('some_filter').unset('name').sort(['name', 5]);
crud = collection.modify('some_filter').unset('name').sort('name', 5);

//@# CollectionModify: Error conditions on limit
crud = collection.modify('some_filter').unset('name').limit();
crud = collection.modify('some_filter').unset('name').limit('');

//@# CollectionModify: Error conditions on bind
crud = collection.modify('name = :data and age > :years').set('hobby', 'swim').bind();
crud = collection.modify('name = :data and age > :years').set('hobby', 'swim').bind(5, 5);
crud = collection.modify('name = :data and age > :years').set('hobby', 'swim').bind('another', 5)

//@# CollectionModify: Error conditions on execute
crud = collection.modify('name = :data and age > :years').set('hobby', 'swim').execute();
crud = collection.modify('name = :data and age > :years').set('hobby', 'swim').bind('years', 5).execute();

// ---------------------------------------
// Collection.Modify Unit Testing: Execution
// ---------------------------------------

//@# CollectionModify: Set Execution
var result = collection.modify('name = "brian"').set('alias', 'bri').set('last_name', 'black').set('age', mysqlx.expr('13+1')).execute();
print('Set Affected Rows:', result.affectedItemsCount, '\n');

var result = collection.find('name = "brian"').execute();
var doc = result.fetchOne();
print(dir(doc));

//@# CollectionModify: Set Execution Binding Array
var result = collection.modify('name = "brian"').set('hobbies', mysqlx.expr(':list')).bind('list', ['soccer', 'dance', 'read']).execute();
print('Set Affected Rows:', result.affectedItemsCount, '\n');

var result = collection.find('name = "brian"').execute();
var doc = result.fetchOne();
print(dir(doc));
print(doc.hobbies[0]);
print(doc.hobbies[1]);
print(doc.hobbies[2]);

//@ CollectionModify: Simple Unset Execution
var result = collection.modify('name = "brian"').unset('last_name').execute();
print('Unset Affected Rows:', result.affectedItemsCount, '\n');

var result = collection.find('name = "brian"').execute();
var doc = result.fetchOne();
print(dir(doc));

//@ CollectionModify: List Unset Execution
var result = collection.modify('name = "brian"').unset(['alias', 'age']).execute();
print('Unset Affected Rows:', result.affectedItemsCount, '\n');

var result = collection.find('name = "brian"').execute();
var doc = result.fetchOne();
print(dir(doc));

//@ CollectionModify: Merge Execution
var result = collection.modify('name = "brian"').merge({ last_name: 'black', age: 15, alias: 'bri', girlfriends: ['martha', 'karen'] }).execute();
print('Merge Affected Rows:', result.affectedItemsCount, '\n');

var result = collection.find('name = "brian"').execute();
var doc = result.fetchOne();
print("Brian's last_name:", doc.last_name, '\n');
print("Brian's age:", doc.age, '\n');
print("Brian's alias:", doc.alias, '\n');
print("Brian's first girlfriend:", doc.girlfriends[0], '\n');
print("Brian's second girlfriend:", doc.girlfriends[1], '\n');

//@ CollectionModify: arrayAppend Execution
var result = collection.modify('name = "brian"').arrayAppend('girlfriends', 'cloe').execute();
print('Array Append Affected Rows:', result.affectedItemsCount, '\n');

var result = collection.find('name = "brian"').execute();
var doc = result.fetchOne();
print("Brian's girlfriends:", doc.girlfriends.length);
print("Brian's last:", doc.girlfriends[2]);

//@ CollectionModify: arrayInsert Execution
var result = collection.modify('name = "brian"').arrayInsert('girlfriends[1]', 'samantha').execute();
print('Array Insert Affected Rows:', result.affectedItemsCount, '\n');

var result = collection.find('name = "brian"').execute();
var doc = result.fetchOne();
print("Brian's girlfriends:", doc.girlfriends.length, '\n');
print("Brian's second:", doc.girlfriends[1], '\n');

//@ CollectionModify: arrayDelete Execution
var result = collection.modify('name = "brian"').arrayDelete('girlfriends[2]').execute();
print('Array Delete Affected Rows:', result.affectedItemsCount, '\n');

var result = collection.find('name = "brian"').execute();
var doc = result.fetchOne();
print("Brian's girlfriends:", doc.girlfriends.length, '\n');
print("Brian's third:", doc.girlfriends[2], '\n');

//@ CollectionModify: sorting and limit Execution
var result = collection.modify('age = 15').set('sample', 'in_limit').sort(['name']).limit(2).execute();
print('Affected Rows:', result.affectedItemsCount, '\n');

var result = collection.find('age = 15').sort(['name']).execute();

//@ CollectionModify: sorting and limit Execution - 1
var doc = result.fetchOne();
print(dir(doc));

//@ CollectionModify: sorting and limit Execution - 2
var doc = result.fetchOne();
print(dir(doc));

//@ CollectionModify: sorting and limit Execution - 3
var doc = result.fetchOne();
print(dir(doc));

//@ CollectionModify: sorting and limit Execution - 4
var doc = result.fetchOne();
print(dir(doc));

//@<OUT> CollectionModify: Patch initial documents
collection.find('gender="female"')

//@<OUT> CollectionModify: Patch adding fields to multiple documents (WL10856-FR1_1) {VER(>=8.0.4)}
collection.modify('gender="female"').patch({hobbies:[], address:"TBD"})
collection.find('gender="female"')

//@<OUT> CollectionModify: Patch updating field on multiple documents (WL10856-FR1_4) {VER(>=8.0.4)}
collection.modify('gender="female"').patch({address:{street:"TBD"}})
collection.find('gender="female"')

//@<OUT> CollectionModify: Patch updating field on multiple nested documents (WL10856-FR1_5) {VER(>=8.0.4)}
collection.modify('gender="female"').patch({address:{street:"Main"}})
collection.find('gender="female"')

//@<OUT> CollectionModify: Patch adding field on multiple nested documents (WL10856-FR1_2) {VER(>=8.0.4)}
collection.modify('gender="female"').patch({address:{number:0}})
collection.find('gender="female"')

//@<OUT> CollectionModify: Patch removing field on multiple nested documents (WL10856-FR1_8) {VER(>=8.0.4)}
collection.modify('gender="female"').patch({address:{number:null}})
collection.find('gender="female"')

//@<OUT> CollectionModify: Patch removing field on multiple documents (WL10856-FR1_7) {VER(>=8.0.4)}
collection.modify('gender="female"').patch({address:null})
collection.find('gender="female"')

//@<OUT> CollectionModify: Patch adding field with multiple calls to patch (WL10856-FR2.1_1) {VER(>=8.0.4)}
collection.modify('gender="female"').patch({last_name:'doe'}).patch({address:{}})
collection.find('gender="female"')

//@<OUT> CollectionModify: Patch adding field with multiple calls to patch on nested documents (WL10856-FR2.1_2) {VER(>=8.0.4)}
collection.modify('gender="female"').patch({address:{street:'main'}}).patch({address:{number:0}})
collection.find('gender="female"')

//@<OUT> CollectionModify: Patch updating fields with multiple calls to patch (WL10856-FR2.1_3, WL10856-FR2.1_4) {VER(>=8.0.4)}
collection.modify('gender="female"').patch({address:{street:'riverside'}}).patch({last_name:'houston'})
collection.find('gender="female"')

//@<OUT> CollectionModify: Patch removing fields with multiple calls to patch (WL10856-FR2.1_5, WL10856-FR2.1_6) {VER(>=8.0.4)}
collection.modify('gender="female"').patch({address:{number:null}}).patch({hobbies:null})
collection.find('gender="female"')

//@<OUT> CollectionModify: Patch adding field to multiple documents using expression (WL10856-ET_13) {VER(>=8.0.4)}
collection.modify('gender="female"').patch({last_update: mysqlx.expr("curdate()")})
var records = collection.find('gender="female"').execute().fetchAll()
var last_update = records[0].last_update
println(records)

//@<OUT> CollectionModify: Patch adding field to multiple nested documents using expression (WL10856-ET_14) {VER(>=8.0.4)}
collection.modify('gender="female"').patch({address: {street_short: mysqlx.expr("right(address.street, 3)")}})
collection.find('gender="female"')

//@<OUT> CollectionModify: Patch updating field to multiple documents using expression (WL10856-ET_15) {VER(>=8.0.4)}
collection.modify('gender="female"').patch({name:mysqlx.expr("concat(ucase(left(name,1)), right(name, length(name)-1))")})
collection.find('gender="female"')

//@<OUT> CollectionModify: Patch updating field to multiple nested documents using expression (WL10856-ET_16) {VER(>=8.0.4)}
collection.modify('gender="female"').patch({address: {street_short: mysqlx.expr("left(address.street, 3)")}})
collection.find('gender="female"')

//@<OUT> CollectionModify: Patch including _id, ignores _id applies the rest (WL10856-ET_17) {VER(>=8.0.4)}
collection.modify('gender="female"').patch({_id:'new_id', city:'Washington'})
collection.find('gender="female"')

//@ CollectionModify: Patch adding field with null value coming from an expression (WL10856-ET_19) {VER(>=8.0.4)}
collection.modify('gender="female"').patch({another:mysqlx.expr("null")})

//@<OUT> CollectionModify: Patch updating field with null value coming from an expression (WL10856-ET_20) {VER(>=8.0.4)}
collection.modify('gender="female"').patch({last_update:mysqlx.expr("null")})
collection.find('gender="female"')

//@ CollectionModify: Patch removing the _id field (WL10856-ET_25) {VER(>=8.0.4)}
collection.modify('gender="female"').patch({_id:null})

// Cleanup

mySession.dropSchema('js_shell_test');
mySession.close();
