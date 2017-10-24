// Assumptions: validate_crud_functions available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
var mysqlx = require('mysqlx');

var mySession = mysqlx.getSession(__uripwd);

ensure_schema_does_not_exist(mySession, 'js_shell_test');

schema = mySession.createSchema('js_shell_test');

// Creates a test collection and inserts data into it
var collection = schema.createCollection('collection1');

//@ Validating collection members
var members = dir(collection);
validateMember(members, 'name');
validateMember(members, 'session');
validateMember(members, 'schema');
validateMember(members, 'add');
validateMember(members, 'addOrReplaceOne');
validateMember(members, 'createIndex');
validateMember(members, 'dropIndex');
validateMember(members, 'existsInDatabase');
validateMember(members, 'find');
validateMember(members, 'getName');
validateMember(members, 'getOne');
validateMember(members, 'getSchema');
validateMember(members, 'getSession');
validateMember(members, 'help');
validateMember(members, 'modify');
validateMember(members, 'remove');
validateMember(members, 'removeOne');
validateMember(members, 'replaceOne');

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

//================= addOrReplaceOne ======================
//@ addOrReplaceOne parameter error conditions
var col = schema.createCollection('add_or_replace_one');
col.addOrReplaceOne();

// WL10849-EX3
col.addOrReplaceOne(1,1);

// WL10849-EX4
col.addOrReplaceOne("identifier",1);

// WL10849-FR6.1.1
//@ addOrReplaceOne: adding new document 1
col.addOrReplaceOne('document_001', {name:'basic'});

//@ addOrReplaceOne: adding new document 2
col.addOrReplaceOne('document_002', {name:'basic'});

//@<OUT> Verify added documents
col.find();

// WL10849-FR6.3.1
//@ addOrReplaceOne: replacing an existing document
col.addOrReplaceOne('document_001', {name:'complex', state:'updated'});

//@<OUT> addOrReplaceOne: Verify replaced document
col.find();

// WL10849-FR8.1
//@ addOrReplaceOne: replacing an existing document, ignoring new _id
col.addOrReplaceOne('document_001', {_id:'ignored_id', name:'medium'});

//@<OUT> addOrReplaceOne: Verify replaced document with ignored _id
col.find();

// WL10849-FR6.2.1
//@ addOrReplaceOne: adding with key
var result = col.createIndex('_name', mysqlx.IndexType.UNIQUE).field('name', "TEXT(50)", true).execute();
col.addOrReplaceOne('document_003', {name:'high'});

// WL10849-FR6.2.2
//@ addOrReplaceOne: error adding with key (BUG#27013165)
col.addOrReplaceOne('document_004', {name:'basic'});

// WL10849-FR6.5.1
//@ addOrReplaceOne: error replacing with key
col.addOrReplaceOne('document_001', {name:'basic'});

// WL10849-FR6.4.1
//@ addOrReplaceOne: replacing document matching id and key
col.addOrReplaceOne('document_001', {name:'medium', sample:true});

// WL10849-EX2
//@ addOrReplaceOne: attempt on dropped collection
schema.dropCollection('add_or_replace_one')
col.addOrReplaceOne('document_001', {name:'medium', sample:true});

//================= getOne ======================
//@ getOne: parameter error conditions
var col = schema.createCollection('get_one');
col.getOne();
col.getOne(45);

//@<OUT> getOne: returns expected document
col.add({_id: 'document_001', name: 'test'});
var doc = col.getOne('document_001');
print(doc);

//@ getOne: returns NULL if no match found
var doc = col.getOne('document_000');
print(doc);

//@ getOne: attempt on dropped collection
schema.dropCollection('get_one')
var doc = col.getOne('document_001');

//================= removeOne ======================
//@<OUT> removeOne: initialization
var col = schema.createCollection('remove_one');
col.add({_id: 'document_001', name: 'test'});
col.add({_id: 'document_002', name: 'test'});
col.find();

// WL10849-FR12.2
//@ removeOne: parameter error conditions
col.removeOne();
col.removeOne(45);

// WL10849-FR12.1
//@ removeOne: removes the expected document
col.removeOne('document_001');

// WL10849-FR13.1
//@ removeOne: suceeds with 0 affected rows
col.removeOne('document_001');

// WL10849-FR13.2
//@<OUT> removeOne: final verification
col.find();

// WL10849-EX6
//@ removeOne: attempt on dropped collection
schema.dropCollection('remove_one')
col.removeOne('document_001');

//================= replaceOne ======================
//@<OUT> replaceOne: initialization
var col = schema.createCollection('replace_one');
col.add({_id: 'document_001', name:'simple'});
col.add({_id: 'document_002', name:'simple'});
col.find();

//@ replaceOne parameter error conditions
col.replaceOne();

// WL10849-FR2.5
col.replaceOne(1,1);

// WL10849-FR2.6
col.replaceOne("identifier",1);

// WL10849-FR1.1
//@ replaceOne: replacing an existing document
col.replaceOne('document_001', {name:'complex', state:'updated'});

// WL10849-FR1.2
//@<OUT> replaceOne: Verify replaced document
col.find();

// WL10849-FR2.2
//@ replaceOne: replacing unexisting document
col.replaceOne('document_003', {name:'complex', state:'updated'});

// WL10849-FR5.1
//@ replaceOne: replacing an existing document, ignoring new _id
col.replaceOne('document_001', {_id:'ignored_id', name:'medium'});

//@<OUT> replaceOne: Verify replaced document with ignored _id
col.find();

//@ replaceOne: error replacing with key
var result = col.createIndex('_name', mysqlx.IndexType.UNIQUE).field('name', "TEXT(50)", true).execute();
col.replaceOne('document_001', {name:'simple'});

//@ replaceOne: replacing document matching id and key
col.replaceOne('document_001', {name:'medium', sample:true});

//@<OUT> Verify replaced document with id and key
col.find();

// WL10849-EX1.1
//@ replaceOne: attempt on dropped collection
schema.dropCollection('replace_one')
col.replaceOne('document_001', {name:'medium', sample:true});

// Closes the session
mySession.dropSchema('js_shell_test');
mySession.close();
