// Assumptions: validate_crud_functions available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
var mysqlx = require('mysqlx');

var mySession = mysqlx.getSession(__uripwd);

ensure_schema_does_not_exist(mySession, 'js_shell_test');

schema = mySession.createSchema('js_shell_test');

// Creates a test collection and inserts data into it
var collection = schema.createCollection('collection1');

//#@<OUT> Testing collection help
collection.help()

//@ Validating members
var members = dir(collection);

print("Member Count:", members.length);

validateMember(members, 'name');
validateMember(members, 'schema');
validateMember(members, 'session');
validateMember(members, 'existsInDatabase');
validateMember(members, 'getName');
validateMember(members, 'getSchema');
validateMember(members, 'getSession');
validateMember(members, 'add');
validateMember(members, 'createIndex');
validateMember(members, 'dropIndex');
validateMember(members, 'modify');
validateMember(members, 'remove');
validateMember(members, 'find');
validateMember(members, 'help');


//@ Testing collection name retrieving
print('getName(): ' + collection.getName());
print('name: ' + collection.name);

//@ Testing session retrieving
print('getSession():', collection.getSession());
print('session:', collection.session);

//@ Testing collection schema retrieving
print('getSchema():', collection.getSchema());
print('schema:', collection.schema);

//@<OUT> Testing help of dropIndex
collection.help("dropIndex")

//@ Testing dropping index
collection.createIndex('_name').field('name', "TEXT(50)", true).execute();
print (collection.dropIndex('_name'));
print (collection.dropIndex('_name'));
print (collection.dropIndex('not_an_index'));

//@ Testing dropping index using execute
collection.dropIndex('_name').execute()

//@ Testing existence
print('Valid:', collection.existsInDatabase());
mySession.getSchema('js_shell_test').dropCollection('collection1');
print('Invalid:', collection.existsInDatabase());

// Closes the session
mySession.dropSchema('js_shell_test');
mySession.close();
