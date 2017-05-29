# Assumptions: validate_crud_functions available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
from mysqlsh import mysqlx

mySession = mysqlx.get_session(__uripwd)

ensure_schema_does_not_exist(mySession, 'js_shell_test')
schema = mySession.create_schema('js_shell_test')

# Creates a test collection and inserts data into it
collection = schema.create_collection('collection1')

# ---------------------------------------------
# Collection.add Unit Testing: Dynamic Behavior
# ---------------------------------------------
#@ CollectionAdd: valid operations after add with no documents
crud = collection.add([])
validate_crud_functions(crud, ['add', 'execute'])

#@ CollectionAdd: valid operations after add
crud = collection.add({"name":"john", "age":17, "account": None})
validate_crud_functions(crud, ['add', 'execute'])

#@ CollectionAdd: valid operations after execute
result = crud.execute()
validate_crud_functions(crud, ['add', 'execute'])


# ---------------------------------------------
# Collection.add Unit Testing: Error Conditions
# ---------------------------------------------

#@# CollectionAdd: Error conditions on add
crud = collection.add()
crud = collection.add(45)
crud = collection.add(['invalid data'])
crud = collection.add(mysqlx.expr('5+1'))
crud = collection.add({'_id':45, 'name': 'sample'});
crud = collection.add([{'name': 'sample'}, 'error']);
crud = collection.add({'name': 'sample'}, 'error');


# ---------------------------------------
# Collection.Add Unit Testing: Execution
# ---------------------------------------

#@ Collection.add execution
result = collection.add({ "name": 'my first', "Passed": 'document', "count": 1 }).execute()
print "Affected Rows Single:", result.affected_item_count, "\n"
print "last_document_id Single:", result.last_document_id
print "get_last_document_id Single:", result.get_last_document_id()
print "#last_document_ids Single:", len(result.last_document_ids)
print "#get_last_document_ids Single:", len(result.get_last_document_ids())

result = collection.add({ "_id": "sample_document", "name": 'my first', "passed": 'document', "count": 1 }).execute()
print "Affected Rows Single Known ID:", result.affected_item_count, "\n"
print "last_document_id Single Known ID:", result.last_document_id
print "get_last_document_id Single Known ID:", result.get_last_document_id()
print "#last_document_ids Single Known ID:", len(result.last_document_ids)
print "#get_last_document_ids Single Known ID:", len(result.get_last_document_ids())
print "#last_document_ids Single Known ID:", result.last_document_ids[0]
print "#get_last_document_ids Single Known ID:", result.get_last_document_ids()[0]

result = collection.add([{ "name": 'my second', "passed": 'again', "count": 2 }, { "name": 'my third', "passed": 'once again', "count": 3 }]).execute()
print "Affected Rows Multi:", result.affected_item_count, "\n"
try:
  print "last_document_id Multi:", result.last_document_id
except Exception, err:
  print "last_document_id Multi:", str(err), "\n"

try:
  print "get_last_document_id Multi:", result.get_last_document_id()
except Exception, err:
  print "get_last_document_id Multi:", str(err), "\n"

print "#last_document_ids Multi:", len(result.last_document_ids)
print "#get_last_document_ids Multi:", len(result.get_last_document_ids())

result = collection.add([{ "_id": "known_00", "name": 'my second', "passed": 'again', "count": 2 }, { "_id": "known_01", "name": 'my third', "passed": 'once again', "count": 3 }]).execute()
print "Affected Rows Multi Known IDs:", result.affected_item_count, "\n"
try:
  print "last_document_id Multi Known IDs:", result.last_document_id
except Exception, err:
  print "last_document_id Multi Known IDs:", str(err), "\n"

try:
  print "get_last_document_id Multi Known IDs:", result.get_last_document_id()
except Exception, err:
  print "get_last_document_id Multi Known IDs:", str(err), "\n"

print "#last_document_ids Multi Known IDs:", len(result.last_document_ids)
print "#get_last_document_ids Multi Known IDs:", len(result.get_last_document_ids())
print "First last_document_ids Multi Known IDs:", result.last_document_ids[0]
print "First get_last_document_ids Multi Known IDs:", result.get_last_document_ids()[0]
print "Second last_document_ids Multi Known IDs:", result.last_document_ids[1]
print "Second get_last_document_ids Multi Known IDs:", result.get_last_document_ids()[1]

result = collection.add([]).execute()
print "Affected Rows Empty List:", result.affected_item_count, "\n"
try:
  print "last_document_id Empty List:", result.last_document_id
except Exception, err:
  print "last_document_id Empty List:", str(err), "\n"

try:
  print "get_last_document_id Empty List:", result.get_last_document_id()
except Exception, err:
  print "get_last_document_id Empty List:", str(err), "\n"

print "#last_document_ids Empty List:", len(result.last_document_ids)
print "#get_last_document_ids Empty List:", len(result.get_last_document_ids())

//! [CollectionAdd: Chained Calls]
result = collection.add({ "name": 'my fourth', "passed": 'again', "count": 4 }).add({ "name": 'my fifth', "passed": 'once again', "count": 5 }).execute()
print "Affected Rows Chained:", result.affected_item_count, "\n"
//! [CollectionAdd: Chained Calls]

//! [CollectionAdd: Using an Expression]
result = collection.add(mysqlx.expr('{"name": "my fifth", "passed": "document", "count": 1}')).execute()
print "Affected Rows Single Expression:", result.affected_item_count, "\n"
//! [CollectionAdd: Using an Expression]

//! [CollectionAdd: Document List]
result = collection.add([{ "name": 'my sexth', "passed": 'again', "count": 5 }, mysqlx.expr('{"name": "my senevth", "passed": "yep again", "count": 5}')]).execute()
print "Affected Rows Mixed List:", result.affected_item_count, "\n"
//! [CollectionAdd: Document List]

//! [CollectionAdd: Multiple Parameters]
result = collection.add({ "name": 'my eigth', "passed": 'yep', "count": 6 }, mysqlx.expr('{"name": "my nineth", "passed": "yep again", "count": 6}')).execute()
print "Affected Rows Multiple Params:", result.affected_item_count, "\n"
//! [CollectionAdd: Multiple Parameters]

# Cleanup
mySession.drop_schema('js_shell_test')
mySession.close()
