// Assumptions: validate_crud_functions available

var mysqlx = require('mysqlx').mysqlx;

var uri = os.getenv('MYSQL_URI');

var mySession = mysqlx.getNodeSession(uri);

ensure_schema_does_not_exist(mySession, 'js_shell_test');

schema = mySession.createSchema('js_shell_test');

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
mySession.dropCollection('js_shell_test', 'collection1');
print('Invalid:', collection.existInDatabase());

// Closes the session
mySession.dropSchema('js_shell_test');
mySession.close();
