shell.connect(__uripwd);
session.dropSchema('prepared_stmt');
var schema = session.createSchema('prepared_stmt');
var collection = schema.createCollection('test_collection');

collection.add({_id: '001', name: 'george', age: 18}).execute();
collection.add({_id: '002', name: 'james', age: 17}).execute();
collection.add({_id: '003', name: 'luke', age: 18}).execute();


//@ First execution is normal
var crud = collection.remove('_id = "001"');
crud.execute();
collection.find();
collection.add({_id: '001', name: 'george', age: 18}).execute();

//@ Second execution prepares statement and executes it
crud.execute();
collection.find();
collection.add({_id: '001', name: 'george', age: 18}).execute();

//@ Third execution uses prepared statement
crud.execute();
collection.find();
collection.add({_id: '001', name: 'george', age: 18}).execute();

//@ sort() changes statement, back to normal execution
crud.sort('name desc').execute();
collection.find();
collection.add({_id: '001', name: 'george', age: 18}).execute();

//@ second execution after sort(), prepares statement and executes it
crud.execute();
collection.find();
collection.add({_id: '001', name: 'george', age: 18}).execute();

//@ third execution after set(), uses prepared statement
crud.execute();
collection.find();
collection.add({_id: '001', name: 'george', age: 18}).execute();

//@ limit() changes statement, back to normal execution
crud.limit(1).execute();
collection.find();
collection.add({_id: '001', name: 'george', age: 18}).execute();

//@ second execution after limit(), prepares statement and executes it
crud.execute();
collection.find();
collection.add({_id: '001', name: 'george', age: 18}).execute();

//@ third execution after limit(), uses prepared statement
crud.execute();
collection.find();
collection.add({_id: '001', name: 'george', age: 18}).execute();

//@ Prepares statement to test re-usability of bind() and limit()
var crud = collection.remove('_id like :id').limit(1);
crud.bind('id', "001").execute();
collection.find();
collection.add({_id: '001', name: 'george', age: 18}).execute();

//@ Prepares and executes statement
crud.bind('id', "002").execute();
collection.find();
collection.add({_id: '002', name: 'james', age: 17}).execute();

//@ Executes prepared statement with bind()
crud.bind('id', "003").execute();
collection.find();
collection.add({_id: '003', name: 'luke', age: 18}).execute();

//@ Executes prepared statement with limit(1)
crud.limit(1).bind('id', "%").execute();
collection.find();
collection.add({_id: '001', name: 'george', age: 18}).execute();

//@ Executes prepared statement with limit(2)
crud.limit(2).bind('id', "%").execute();
collection.find();

//@<> Finalizing
session.dropSchema('prepared_stmt');
session.close();
