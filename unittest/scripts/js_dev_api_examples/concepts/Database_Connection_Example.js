
var mysqlx = require('mysqlx');

// Connect to server on localhost
var mySession = mysqlx.getSession( {
                host: 'localhost', port: 33060,
                dbUser: 'mike', dbPassword: 's3cr3t!' } );

var myDb = mySession.getSchema('test');

// Use the collection 'my_collection'
var myColl = myDb.getCollection('my_collection');

// Specify which document to find with Collection.find() and
// fetch it from the database with .execute()
var myDocs = myColl.find('name like :param').limit(1).
        bind('param', 'S%').execute();

// Print document
print(myDocs.fetchOne());

mySession.close();
