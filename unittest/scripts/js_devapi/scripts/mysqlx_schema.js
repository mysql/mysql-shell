var mysqlx = require('mysqlx').mysqlx;

var  uri = os.getenv('MYSQL_URI');

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
session.setCurrentSchema('js_shell_test');

var result;
result = session.sql('create table table1 (name varchar(50));').execute();
result = session.sql('create view view1 (my_name) as select name from table1;').execute();
result = session.js_shell_test.createCollection('collection1');


//@ Testing schema name retrieving
print('getName(): ' + schema.getName());
print('name: ' + schema.name);

//@ Testing schema.getSession
print('getSession():',schema.getSession());

//@ Testing schema.session
print('session:', schema.session);

//@ Testing schema schema retrieving
print('getSchema():', schema.getSchema());
print('schema:', schema.schema);

//@ Testing tables, views and collection retrieval
print('getTables():', session.js_shell_test.getTables().table1);
print('tables:', session.js_shell_test.tables.table1);
print('getViews():', session.js_shell_test.getViews().view1);
print('views:', session.js_shell_test.views.view1);
print('getCollections():', session.js_shell_test.getCollections().collection1);
print('collections:', session.js_shell_test.collections.collection1);

//@ Testing specific object retrieval
print('getTable():', session.js_shell_test.getTable('table1'));
print('.<table>:', session.js_shell_test.table1);
print('getView():', session.js_shell_test.getView('view1'));
print('.<view>:', session.js_shell_test.view1);
print('getCollection():', session.js_shell_test.getCollection('collection1'));
print('.<collection>:', session.js_shell_test.collection1);

//@ Retrieving collection as table
print('getCollectionAsTable():', session.js_shell_test.getCollectionAsTable('collection1'));

//@ Collection creation
var collection = schema.createCollection('my_sample_collection');
print('createCollection():', collection);

//@ Testing existence
print('Valid:', schema.existInDatabase());
schema.drop();
print('Invalid:', schema.existInDatabase());

//@ Closes the session
session.close();











