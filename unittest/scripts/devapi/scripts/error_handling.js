// Assumptions: test schema exists, my_collection exists

var mysqlx = require('mysqlx').mysqlx;

var mySession;

try {
        // Connect to server on localhost
        mySession = mysqlx.getSession( {
                host: 'localhost', port: 33060,
                dbUser: 'mike', dbPassword: 's3cr3t!' } );
} catch (err) {
        echo('The database session could not be opened: ' + err.message);

        // Exit with error code
        exit(1);
}

try {
        var myDb = mySession.getSchema('test');

        // Use the collection 'my_collection'
        var myColl = myDb.getCollection('my_collection');

        // Find a document
        var myDoc = myColl.find('name like :param').limit(1)
                .bind('param','S%').execute();

        // Print document
        print(myDoc.first());

        exit(0);
} catch (err) {
        print('The following error occurred: ' + err.message);

        exit(1);
} finally {
        // Close the session in any case
        mySession.close();
}
