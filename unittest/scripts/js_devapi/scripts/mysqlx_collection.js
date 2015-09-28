// Assumptions: validate_crud_functions available

var mysqlx = require('mysqlx').mysqlx;

var uri = os.getenv('MYSQL_URI');

var session = mysqlx.getNodeSession(uri);

var schema;
try{
	// Ensures the js_shell_test does not exist
	schema = session.getSchema('js_shell_test');
	schema.drop();
}
catch(err)
{
}

schema = session.createSchema('js_shell_test');

// Creates a test collection and inserts data into it
var collection = schema.createCollection('collection1');


//@ Testing collection name retrieving
print('getName(): ' + collection.getName());
print('name: ' + collection.name);


//@ Testing session retrieving
print('getSession():', collection.getSession());
print('session:', collection.session);

//@ Testing collection schema retrieving
print('getSchema():', collection.getSchema());
print('schema:', collection.schema);

//@ Testing existence
print('Valid:', collection.existInDatabase());
collection.drop();
print('Invalid:', collection.existInDatabase());

//@ Closes the session
schema.drop();
session.close();
