shell.connect(__uripwd);
session.dropSchema('prepared_stmt');
var schema = session.createSchema('prepared_stmt');
var collection = schema.createCollection('test_collection');

collection.add({_id: '001', name: 'george', age: 18}).execute();
collection.add({_id: '002', name: 'james', age: 17}).execute();
collection.add({_id: '003', name: 'luke', age: 18}).execute();


//@ First execution is normal
var crud = collection.find();
crud.execute();

//@ Second execution prepares statement and executes it
crud.execute()

//@ Third execution uses prepared statement
crud.execute()

//@ fields() changes statement, back to normal execution
crud.fields('name', 'age').execute();

//@ second execution after fields(), prepares statement and executes it
crud.execute()

//@ third execution after fields(), uses prepared statement
crud.execute()

//@ sort() changes statement, back to normal execution
crud.sort('age asc').execute();

//@ second execution after sort(), prepares statement and executes it
crud.execute()

//@ third execution after sort(), uses prepared statement
crud.execute()

//@ limit() changes statement, back to normal execution
crud.limit(2).execute();

//@ second execution after limit(), prepares statement and executes it
crud.execute()

//@ third execution after limit(), uses prepared statement
crud.execute()

//@ offset() does not change the statement, uses prepared one
crud.offset(1).execute();

//@ lockExclusive() changes statement, back to normal execution
crud.lockExclusive().execute();

//@ second execution after lockExclusive(), prepares statement and executes it
crud.execute()

//@ third execution after lockExclusive(), uses prepared statement
crud.execute()

//@ creates statement to test lockShared()
var crud = collection.find();
crud.execute();

//@ prepares statement to test lockShared()
crud.execute();

//@ lockShared() changes statement, back to normal execution
crud.lockShared().execute();

//@ second execution after lockShared(), prepares statement and executes it
crud.execute()

//@ third execution after lockShared(), uses prepared statement
crud.execute()

//@ creates statement with aggregate function to test having()
var crud = collection.find().fields('age', 'count(age)').groupBy('age')
crud.execute();

//@ prepares statement with aggregate function to test having()
crud.execute();

//@ having() changes statement, back to normal execution
crud.having('count(age) > 1').execute();

//@ second execution after having(), prepares statement and executes it
crud.execute()

//@ third execution after having(), uses prepared statement
crud.execute()


//@ creates statement to test no changes when reusing bind(), limit() and offset()
var crud = collection.find('name like :ph').limit(3)
crud.bind('ph', '%').execute();

//@ prepares statement to test no changes when reusing bind(), limit() and offset()
crud.bind('ph', '%').execute();

//@ Reusing statement with bind() using g%
crud.bind('ph', 'g%').execute();

//@ Reusing statement with bind() using j%
crud.bind('ph', 'j%').execute();

//@ Reusing statement with bind() using l%
crud.bind('ph', 'l%').execute();

//@ Reusing statement with new limit()
crud.limit(1).bind('ph', '%').execute();

//@ Reusing statement with new offset()
crud.offset(1).bind('ph', '%').execute();

//@ Reusing statement with new limit() and offset()
crud.limit(2).offset(1).bind('ph', '%').execute();


//@<> Finalizing
session.dropSchema('prepared_stmt');
session.close();
