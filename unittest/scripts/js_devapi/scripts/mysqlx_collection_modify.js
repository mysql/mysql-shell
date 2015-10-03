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

// ------------------------------------------------
// Collection.Modify Unit Testing: Dynamic Behavior
// ------------------------------------------------
//@ CollectionModify: valid operations after modify and set
var crud = collection.modify();
validate_crud_functions(crud, ['set', 'unset', 'merge', 'arrayInsert', 'arrayAppend', 'arrayDelete']);
crud.set('name', 'dummy');
validate_crud_functions(crud, ['set', 'unset', 'merge', 'arrayInsert', 'arrayAppend', 'arrayDelete', 'sort', 'limit', 'bind', 'execute', '__shell_hook__']);

//@ CollectionModify: valid operations after modify and unset empty
crud = collection.modify();
validate_crud_functions(crud, ['set', 'unset', 'merge', 'arrayInsert', 'arrayAppend', 'arrayDelete']);
crud.unset([]);
validate_crud_functions(crud, ['set', 'unset', 'merge', 'arrayInsert', 'arrayAppend', 'arrayDelete']);

//@ CollectionModify: valid operations after modify and unset list
crud = collection.modify();
validate_crud_functions(crud, ['set', 'unset', 'merge', 'arrayInsert', 'arrayAppend', 'arrayDelete']);
crud.unset(['name', 'type']);
validate_crud_functions(crud, ['set', 'unset', 'merge', 'arrayInsert', 'arrayAppend', 'arrayDelete', 'sort', 'limit', 'bind', 'execute', '__shell_hook__']);

//@ CollectionModify: valid operations after modify and unset multiple params
crud = collection.modify();
validate_crud_functions(crud, ['set', 'unset', 'merge', 'arrayInsert', 'arrayAppend', 'arrayDelete']);
crud.unset('name', 'type');
validate_crud_functions(crud, ['set', 'unset', 'merge', 'arrayInsert', 'arrayAppend', 'arrayDelete', 'sort', 'limit', 'bind', 'execute', '__shell_hook__']);

//@ CollectionModify: valid operations after modify and merge
crud = collection.modify();
validate_crud_functions(crud, ['set', 'unset', 'merge', 'arrayInsert', 'arrayAppend', 'arrayDelete']);
crud.merge({'att':'value','second':'final'});
validate_crud_functions(crud, ['set', 'unset', 'merge', 'arrayInsert', 'arrayAppend', 'arrayDelete', 'sort', 'limit', 'bind', 'execute', '__shell_hook__']);

//@ CollectionModify: valid operations after modify and arrayInsert
crud = collection.modify();
validate_crud_functions(crud, ['set', 'unset', 'merge', 'arrayInsert', 'arrayAppend', 'arrayDelete']);
crud.arrayInsert('hobbies[3]', 'run');
validate_crud_functions(crud, ['set', 'unset', 'merge', 'arrayInsert', 'arrayAppend', 'arrayDelete', 'sort', 'limit', 'bind', 'execute', '__shell_hook__']);

//@ CollectionModify: valid operations after modify and arrayAppend
crud = collection.modify();
validate_crud_functions(crud, ['set', 'unset', 'merge', 'arrayInsert', 'arrayAppend', 'arrayDelete']);
crud.arrayAppend('hobbies','skate');
validate_crud_functions(crud, ['set', 'unset', 'merge', 'arrayInsert', 'arrayAppend', 'arrayDelete', 'sort', 'limit', 'bind', 'execute', '__shell_hook__']);

//@ CollectionModify: valid operations after modify and arrayDelete
crud = collection.modify();
validate_crud_functions(crud, ['set', 'unset', 'merge', 'arrayInsert', 'arrayAppend', 'arrayDelete']);
crud.arrayDelete('hobbies[5]')
validate_crud_functions(crud, ['set', 'unset', 'merge', 'arrayInsert', 'arrayAppend', 'arrayDelete', 'sort', 'limit', 'bind', 'execute', '__shell_hook__']);

//@ CollectionModify: valid operations after sort
crud.sort(['name']);
validate_crud_functions(crud, ['limit', 'bind', 'execute', '__shell_hook__']);

//@ CollectionModify: valid operations after limit
crud.limit(2);
validate_crud_functions(crud, ['bind', 'execute', '__shell_hook__']);

//@ CollectionModify: valid operations after bind
crud = collection.modify('name = :data').set('age', 15).bind('data', 'angel');
validate_crud_functions(crud, ['bind', 'execute', '__shell_hook__']);

//@ CollectionModify: valid operations after execute
result = crud.execute();
validate_crud_functions(crud, ['bind', 'execute', '__shell_hook__']);

//@ Reusing CRUD with binding
print('Updated Angel:', result.affectedRows, '\n');
result=crud.bind('data', 'carol').execute();
print('Updated Carol:', result.affectedRows, '\n');


// ----------------------------------------------
// Collection.Modify Unit Testing: Error Conditions
// ----------------------------------------------

//@# CollectionModify: Error conditions on modify
crud = collection.modify(5);
crud = collection.modify('test = "2');

//@# CollectionModify: Error conditions on set
crud = collection.modify().set();
crud = collection.modify().set(45, 'whatever');
crud = collection.modify().set('', 5);


//@# CollectionModify: Error conditions on unset
crud = collection.modify().unset();
crud = collection.modify().unset({});
crud = collection.modify().unset({}, '');
crud = collection.modify().unset(['name', 1]);
crud = collection.modify().unset('');

//@# CollectionModify: Error conditions on merge
crud = collection.modify().merge();
crud = collection.modify().merge('');

//@# CollectionModify: Error conditions on arrayInsert
crud = collection.modify().arrayInsert();
crud = collection.modify().arrayInsert(5, 'another');
crud = collection.modify().arrayInsert('', 'another');
crud = collection.modify().arrayInsert('test', 'another');

//@# CollectionModify: Error conditions on arrayAppend
crud = collection.modify().arrayAppend();
crud = collection.modify().arrayAppend({},'');
crud = collection.modify().arrayAppend('',45);
crud = collection.modify().arrayAppend('data', mySession);

//@# CollectionModify: Error conditions on arrayDelete
crud = collection.modify().arrayDelete();
crud = collection.modify().arrayDelete(5);
crud = collection.modify().arrayDelete('');
crud = collection.modify().arrayDelete('test');

//@# CollectionModify: Error conditions on sort
crud = collection.modify().unset('name').sort();
crud = collection.modify().unset('name').sort(5);
crud = collection.modify().unset('name').sort([]);
crud = collection.modify().unset('name').sort(['name', 5]);

//@# CollectionModify: Error conditions on limit
crud = collection.modify().unset('name').limit();
crud = collection.modify().unset('name').limit('');

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
var doc;

//@# CollectionModify: Set Execution
result = collection.modify('name = "brian"').set('alias', 'bri').set('last_name', 'black').set('age', mysqlx.expr('13+1')).execute();
print('Set Affected Rows:', result.affectedRows, '\n');

result = collection.find('name = "brian"').execute();
doc = result.next();
print(dir(doc));

//@ CollectionModify: Simple Unset Execution
result = collection.modify('name = "brian"').unset('last_name').execute();
print('Unset Affected Rows:', result.affectedRows, '\n');

result = collection.find('name = "brian"').execute();
doc = result.next();
print(dir(doc));

//@ CollectionModify: List Unset Execution
result = collection.modify('name = "brian"').unset(['alias', 'age']).execute();
print('Unset Affected Rows:', result.affectedRows, '\n');

result = collection.find('name = "brian"').execute();
doc = result.next();
print(dir(doc));

//@ CollectionModify: Merge Execution
result = collection.modify('name = "brian"').merge({last_name:'black', age:15, alias:'bri', girlfriends:['martha', 'karen']}).execute();
print('Merge Affected Rows:', result.affectedRows, '\n');

result = collection.find('name = "brian"').execute();
doc = result.next();
print("Brian's last_name:",  doc.last_name, '\n');
print("Brian's age:",  doc.age, '\n');
print("Brian's alias:",  doc.alias, '\n');
print("Brian's first girlfriend:",  doc.girlfriends[0], '\n');
print("Brian's second girlfriend:",  doc.girlfriends[1], '\n');

//@ CollectionModify: arrayAppend Execution
result = collection.modify('name = "brian"').arrayAppend('girlfriends','cloe').execute();
print('Array Append Affected Rows:', result.affectedRows, '\n');

result = collection.find('name = "brian"').execute();
doc = result.next();
print("Brian's girlfriends:", doc.girlfriends.length);
print("Brian's last:", doc.girlfriends[2]);

//@ CollectionModify: arrayInsert Execution
result = collection.modify('name = "brian"').arrayInsert('girlfriends[1]','samantha').execute();
print('Array Insert Affected Rows:', result.affectedRows, '\n');

result = collection.find('name = "brian"').execute();
doc = result.next();
print("Brian's girlfriends:", doc.girlfriends.length, '\n');
print("Brian's second:", doc.girlfriends[1], '\n');

//@ CollectionModify: arrayDelete Execution
result = collection.modify('name = "brian"').arrayDelete('girlfriends[2]').execute();
print('Array Delete Affected Rows:', result.affectedRows, '\n');

result = collection.find('name = "brian"').execute();
doc = result.next();
print("Brian's girlfriends:", doc.girlfriends.length, '\n');
print("Brian's third:", doc.girlfriends[2], '\n');

//@ CollectionModify: sorting and limit Execution
result = collection.modify('age = 15').set('sample', 'in_limit').sort(['name']).limit(2).execute();
print('Affected Rows:', result.affectedRows, '\n');

result = collection.find('age = 15').sort(['name']).execute();

//@ CollectionModify: sorting and limit Execution - 1
doc = result.next();
print(dir(doc));

//@ CollectionModify: sorting and limit Execution - 2
doc = result.next();
print(dir(doc));

//@ CollectionModify: sorting and limit Execution - 3
doc = result.next();
print(dir(doc));

//@ CollectionModify: sorting and limit Execution - 4
doc = result.next();
print(dir(doc));

// Cleanup
mySession.dropSchema('js_shell_test');
mySession.close();