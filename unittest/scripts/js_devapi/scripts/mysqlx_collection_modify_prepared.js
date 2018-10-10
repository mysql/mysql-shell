shell.connect(__uripwd);
session.dropSchema('prepared_stmt');
var schema = session.createSchema('prepared_stmt');
var collection = schema.createCollection('test_collection');

collection.add({_id: '001', name: 'george', age: 18}).execute();
collection.add({_id: '002', name: 'james', age: 17}).execute();
collection.add({_id: '003', name: 'luke', age: 18}).execute();


//@ First execution is normal
var crud = collection.modify('1').set('grade', 1);
crud.execute();
collection.find();
collection.modify('1').unset('grade');

//@ Second execution prepares statement and executes it
crud.execute()
collection.find();
collection.modify('1').unset('grade');

//@ Third execution uses prepared statement
crud.execute()
collection.find();
collection.modify('1').unset('grade');

//@ set() changes statement, back to normal execution
crud.set('group', 'A').execute();
collection.find();
collection.modify('1').unset('group', 'grade');

//@ second execution after set(), prepares statement and executes it
crud.execute()
collection.find();
collection.modify('1').unset('group', 'grade');

//@ third execution after set(), uses prepared statement
crud.execute()
collection.find();
collection.modify('1').unset('group', 'grade');

//@ unset() changes statement, back to normal execution
crud.unset('age').execute();
collection.find();
collection.modify('1').unset('group', 'grade').set('age', 18);

//@ second execution after unset(), prepares statement and executes it
crud.execute()
collection.find();
collection.modify('1').unset('group', 'grade').set('age', 18);

//@ third execution after unset(), uses prepared statement
crud.execute()
collection.find();
collection.modify('1').unset('group', 'grade').set('age', 18);

//@ patch() changes statement, back to normal execution
crud.patch({grades: ['A', 'B', 'C', 'D', 'E', 'F']}).execute();
collection.find();
collection.modify('1').unset('group', 'grade', 'grades').set('age', 18);

//@ second execution after patch(), prepares statement and executes it
crud.execute()
collection.find();
collection.modify('1').unset('group', 'grade', 'grades').set('age', 18);

//@ third execution after patch(), uses prepared statement
crud.execute()
collection.find();
collection.modify('1').unset('group', 'grade', 'grades').set('age', 18);


//@ arrayInsert() changes statement, back to normal execution
crud.arrayInsert("grades[1]", "A+").execute();
collection.find();
collection.modify('1').unset('group', 'grade', 'grades').set('age', 18);

//@ second execution after arrayInsert(), prepares statement and executes it
crud.execute()
collection.find();
collection.modify('1').unset('group', 'grade', 'grades').set('age', 18);

//@ third execution after arrayInsert(), uses prepared statement
crud.execute()
collection.find();
collection.modify('1').unset('group', 'grade', 'grades').set('age', 18);

//@ arrayAppend() changes statement, back to normal execution
crud.arrayAppend("grades", "G").execute();
collection.find();
collection.modify('1').unset('group', 'grade', 'grades').set('age', 18);

//@ second execution after arrayAppend(), prepares statement and executes it
crud.execute()
collection.find();
collection.modify('1').unset('group', 'grade', 'grades').set('age', 18);

//@ third execution after arrayAppend(), uses prepared statement
crud.execute()
collection.find();
collection.modify('1').unset('group', 'grade', 'grades').set('age', 18);


//@ sort() changes statement, back to normal execution
crud.sort('name desc').execute();
collection.find();
collection.modify('1').unset('group', 'grade', 'grades').set('age', 18);

//@ second execution after sort(), prepares statement and executes it
crud.execute()
collection.find();
collection.modify('1').unset('group', 'grade', 'grades').set('age', 18);

//@ third execution after sort(), uses prepared statement
crud.execute()
collection.find();
collection.modify('1').unset('group', 'grade', 'grades').set('age', 18);

//@ limit() changes statement, back to normal execution
crud.limit(2).execute();
collection.find();
collection.modify('1').unset('group', 'grade', 'grades').set('age', 18);

//@ second execution after limit(), prepares statement and executes it
crud.execute()
collection.find();
collection.modify('1').unset('group', 'grade', 'grades').set('age', 18);

//@ third execution after limit(), uses prepared statement
crud.execute()
collection.find();
collection.modify('1').unset('group', 'grade', 'grades').set('age', 18);

//@ prepares statement to test no changes when reusing bind() and limit()
var crud = collection.modify('name like :name').set('age', 19).limit(1);
crud.bind('name', 'g%').execute();
collection.find();

//@ Reusing statement with bind() using j%
crud.bind('name', 'j%').execute();
collection.find();

//@ Reusing statement with new limit()
crud.limit(2).bind('name', '%').execute();
collection.find();

//@<> Finalizing
session.dropSchema('prepared_stmt');
session.close();
