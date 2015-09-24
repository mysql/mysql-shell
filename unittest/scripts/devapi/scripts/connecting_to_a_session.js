// Assumptions: test schema exists
// Passing the paramaters in the { param: value } format
var dictSession = mysqlx.getSession( {
        host: 'localhost', port: 33060,
        dbUser: 'mike', dbPassword: 's3cr3t!' } );
        
var db1 = dictSession.getSchema('test');

// Passing the paramaters in the URL format
var uriSession = mysqlx.getSession('mike:s3cr3t!@localhost:33060');

var db2 = uriSession.getSchema('test');