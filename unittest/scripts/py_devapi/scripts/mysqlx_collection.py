# Assumptions: validate_crud_functions available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
from __future__ import print_function
from mysqlsh import mysqlx

mySession = mysqlx.get_session(__uripwd)

ensure_schema_does_not_exist(mySession, 'py_shell_test')

schema = mySession.create_schema('py_shell_test')

# Creates a test collection and inserts data into it
collection = schema.create_collection('collection1')

#@<> Validating members
validate_members(collection, [
  'name',
  'schema',
  'session',
  'add',
  'add_or_replace_one',
  'create_index',
  'count',
  'drop_index',
  'exists_in_database',
  'find',
  'get_name',
  'get_one',
  'get_schema',
  'get_session',
  'help',
  'modify',
  'remove',
  'remove_one',
  'replace_one'])

#@ Testing collection name retrieving
print('get_name(): ' + collection.get_name())
print('name: ' + collection.name)

#@ Testing session retrieving
print('get_session():', collection.get_session())
print('session:', collection.session)

#@ Testing collection schema retrieving
print('get_schema():', collection.get_schema())
print('schema:', collection.schema)

#@ Testing dropping index {VER(>=8.0.11)}
collection.create_index('_name', {'fields': [{'field': '$.name', 'type': 'TEXT(50)'}]});
print(collection.drop_index('_name'))
print(collection.drop_index('_name'))
print(collection.drop_index('not_an_index'))

#@ Testing dropping index using execute {VER(>=8.0.11)}
collection.drop_index('_name').execute()

#@ Testing existence
print('Valid:', collection.exists_in_database())
mySession.get_schema('py_shell_test').drop_collection('collection1')
print('Invalid:', collection.exists_in_database())

#================= add_or_replace_one ======================
#@ add_or_replace_one parameter error conditions
col = schema.create_collection('add_or_replace_one');
col.add_or_replace_one();

# WL10849-EX3
col.add_or_replace_one(1,1);

# WL10849-EX4
col.add_or_replace_one("identifier",1);

# WL10849-FR6.1.1
#@ add_or_replace_one: adding new document 1
col.add_or_replace_one('document_001', {'name':'basic'});

#@ add_or_replace_one: adding new document 2
col.add_or_replace_one('document_002', {'name':'basic'});

#@<OUT> Verify added documents
col.find();

# WL10849-FR6.3.1
#@ add_or_replace_one: replacing an existing document {VER(>=8.0.3)}
col.add_or_replace_one('document_001', {'name':'complex', 'state':'updated'});

#@<OUT> add_or_replace_one: Verify replaced document {VER(>=8.0.3)}
col.find();

# Bug#32753547
#@ add_or_replace_one: replacing an existing document, wrong _id {VER(>=8.0.3)}
col.add_or_replace_one('document_001', {'_id':'ignored_id', 'name':'medium'});

# WL10849-FR6.2.1
#@ add_or_replace_one: adding with key {VER(>=8.0.3)}
result = col.create_index('_name', {'fields': [{'field': '$.name', 'type': 'TEXT(50)'}], 'unique':True});
col.add_or_replace_one('document_003', {'name':'high'});

# WL10849-FR6.2.2
#@ add_or_replace_one: error adding with key (BUG#27013165) {VER(>=8.0.3)}
col.add_or_replace_one('document_004', {'name':'basic'});

# WL10849-FR6.5.1
#@ add_or_replace_one: error replacing with key {VER(>=8.0.3)}
col.add_or_replace_one('document_001', {'name':'basic'});

# WL10849-FR6.4.1
#@ add_or_replace_one: replacing document matching id and key {VER(>=8.0.3)}
col.add_or_replace_one('document_001', {'name':'medium', 'sample':True});

# WL10849-EX2
#@ add_or_replace_one: attempt on dropped collection
schema.drop_collection('add_or_replace_one')
col.add_or_replace_one('document_001', {'name':'medium', 'sample':True});

#================= get_one ======================
#@ get_one: parameter error conditions
col = schema.create_collection('get_one');
col.get_one();
col.get_one(45);

#@<OUT> get_one: returns expected document
col.add({'_id': 'document_001', 'name': 'test'});
doc = col.get_one('document_001');
doc

#@ get_one: returns NULL if no match found
doc = col.get_one('document_000');
print(doc);

#@ get_one: attempt on dropped collection
schema.drop_collection('get_one')
doc = col.get_one('document_001');

#================= remove_one ======================
#@<OUT> remove_one: initialization
col = schema.create_collection('remove_one');
col.add({'_id': 'document_001', 'name': 'test'});
col.add({'_id': 'document_002', 'name': 'test'});
col.find();

# WL10849-FR12.2
#@ remove_one: parameter error conditions
col.remove_one();
col.remove_one(45);

# WL10849-FR12.1
#@ remove_one: removes the expected document
col.remove_one('document_001');

# WL10849-FR13.1
#@ remove_one: suceeds with 0 affected rows
col.remove_one('document_001');

# WL10849-FR13.2
#@<OUT> remove_one: final verification
col.find();

# WL10849-EX6
#@ remove_one: attempt on dropped collection
schema.drop_collection('remove_one')
col.remove_one('document_001');

#================= replace_one ======================
#@<OUT> replace_one: initialization
col = schema.create_collection('replace_one');
col.add({'_id': 'document_001', 'name':'simple'});
col.add({'_id': 'document_002', 'name':'simple'});
col.find();

#@ replace_one parameter error conditions
col.replace_one();

# WL10849-FR2.5
col.replace_one(1,1);

# WL10849-FR2.6
col.replace_one("identifier",1);

# WL10849-FR1.1
#@ replace_one: replacing an existing document {VER(>=8.0.3)}
col.replace_one('document_001', {'name':'complex', 'state':'updated'});

# WL10849-FR1.2
#@<OUT> replace_one: Verify replaced document {VER(>=8.0.3)}
col.find();

# WL10849-FR2.2
#@ replace_one: replacing unexisting document {VER(>=8.0.3)}
col.replace_one('document_003', {'name':'complex', 'state':'updated'});

# Bug#32753547
#@ replace_one: replacing an existing document, wrong _id {VER(>=8.0.3)}
col.replace_one('document_001', {'_id':'ignored_id', 'name':'medium'});

#@ replace_one: error replacing with key {VER(>=8.0.3)}
result = col.create_index('_name', {'fields': [{'field': '$.name', 'type': 'TEXT(50)'}], 'unique':True});
col.replace_one('document_001', {'name':'simple'});

#@ replace_one: replacing document matching id and key {VER(>=8.0.3)}
col.replace_one('document_001', {'name':'medium', 'sample':True});

#@<OUT> Verify replaced document with id and key {VER(>=8.0.3)}
col.find();

# WL10849-EX1.1
#@ replace_one: attempt on dropped collection {VER(>=8.0.3)}
schema.drop_collection('replace_one')
col.replace_one('document_001', {'name':'medium', 'sample':True});

#@<> WL12412: Count Test Initialization
collection = schema.create_collection('count_collection')

#@ WL12412-TS1_1: Count takes no arguments
collection.count(1)

#@ WL12412-TS1_2: Count returns correct number of documents
count = collection.count()
print("Initial Document Count: %d" % count)

collection.add({"_id" : "001", "sample": "document1"})
collection.add({"_id" : "002", "sample": "document2"})
collection.add({"_id" : "003", "sample": "document3"})

count = collection.count()
print("Final Document Count: %d" % count)

#@ WL12412-TS2_1: Count throws error on unexisting collection
schema.drop_collection('count_collection')
collection.count()

#@<OUT> BUG32377134 Add empty list of document crash MySQL Shell
collection = schema.create_collection('batch_posts')
collection.add([{"_id": "001"}, {"_id": "002"}])
collection.add([{"_id": "003"}])
collection.add({"_id": "004"})
collection.add([])
collection.find().sort(["_id"])

# Closes the session
mySession.drop_schema('py_shell_test')
mySession.close()
