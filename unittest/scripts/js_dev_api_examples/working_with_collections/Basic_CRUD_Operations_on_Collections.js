
// Connecting to MySQL Server and working with a Collection

var mysqlx = require('mysqlx');

// Connect to server
var mySession = mysqlx.getSession( {
host: 'localhost', port: 33060,
dbUser: 'mike', dbPassword: 's3cr3t!'} );

var myDb = mySession.getSchema('test');

// Create a new collection 'my_collection'
var myColl = myDb.createCollection('my_collection');

// Insert documents
myColl.add({_id: '1', name: 'Sakila', age: 15}).execute();
myColl.add({_id: '2', name: 'Susanne', age: 24}).execute();
myColl.add({_id: '3', name: 'Mike', age: 39}).execute();

// Find a document
var docs = myColl.find('name like :param1 AND age < :param2').limit(1).
        bind('param1','S%').bind('param2',20).execute();

// Print document
print(docs.fetchOne());

// Drop the collection
myDb.dropCollection('my_collection');
