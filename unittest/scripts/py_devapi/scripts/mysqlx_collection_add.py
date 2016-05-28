# Assumptions: validate_crud_functions available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
import mysqlx

mySession = mysqlx.getNodeSession(__uripwd)

ensure_schema_does_not_exist(mySession, 'js_shell_test')
schema = mySession.createSchema('js_shell_test')

# Creates a test collection and inserts data into it
collection = schema.createCollection('collection1')

# ---------------------------------------------
# Collection.add Unit Testing: Dynamic Behavior
# ---------------------------------------------
#@ CollectionAdd: valid operations after add with no documents
crud = collection.add([])
validate_crud_functions(crud, ['add', 'execute', '__shell_hook__'])

#@ CollectionAdd: valid operations after add
crud = collection.add({"name":"john", "age":17})
validate_crud_functions(crud, ['add', 'execute', '__shell_hook__'])

#@ CollectionAdd: valid operations after execute
result = crud.execute()
validate_crud_functions(crud, ['add', 'execute', '__shell_hook__'])


# ---------------------------------------------
# Collection.add Unit Testing: Error Conditions
# ---------------------------------------------

#@# CollectionAdd: Error conditions on add
crud = collection.add()
crud = collection.add(45)
crud = collection.add(['invalid data'])
crud = collection.add(mysqlx.expr('5+1'))

# ---------------------------------------
# Collection.Add Unit Testing: Execution
# ---------------------------------------

#@ Collection.add execution
result = collection.add({ "name": 'my first', "passed": 'document', "count": 1 }).execute()
print "Affected Rows Single:", result.affectedItemCount, "\n"
print "lastDocumentId Single:", result.lastDocumentId
print "getLastDocumentId Single:", result.getLastDocumentId()
print "#lastDocumentIds Single:", len(result.lastDocumentIds)
print "#getLastDocumentIds Single:", len(result.getLastDocumentIds())

result = collection.add({ "_id": "sample_document", "name": 'my first', "passed": 'document', "count": 1 }).execute()
print "Affected Rows Single Known ID:", result.affectedItemCount, "\n"
print "lastDocumentId Single Known ID:", result.lastDocumentId
print "getLastDocumentId Single Known ID:", result.getLastDocumentId()
print "#lastDocumentIds Single Known ID:", len(result.lastDocumentIds)
print "#getLastDocumentIds Single Known ID:", len(result.getLastDocumentIds())
print "#lastDocumentIds Single Known ID:", result.lastDocumentIds[0]
print "#getLastDocumentIds Single Known ID:", result.getLastDocumentIds()[0]

result = collection.add([{ "name": 'my second', "passed": 'again', "count": 2 }, { "name": 'my third', "passed": 'once again', "count": 3 }]).execute()
print "Affected Rows Multi:", result.affectedItemCount, "\n"
try:
  print "lastDocumentId Multi:", result.lastDocumentId
except Exception, err:
  print "lastDocumentId Multi:", str(err), "\n"

try:
  print "getLastDocumentId Multi:", result.getLastDocumentId()
except Exception, err:
  print "getLastDocumentId Multi:", str(err), "\n"

print "#lastDocumentIds Multi:", len(result.lastDocumentIds)
print "#getLastDocumentIds Multi:", len(result.getLastDocumentIds())

result = collection.add([{ "_id": "known_00", "name": 'my second', "passed": 'again', "count": 2 }, { "_id": "known_01", "name": 'my third', "passed": 'once again', "count": 3 }]).execute()
print "Affected Rows Multi Known IDs:", result.affectedItemCount, "\n"
try:
  print "lastDocumentId Multi Known IDs:", result.lastDocumentId
except Exception, err:
  print "lastDocumentId Multi Known IDs:", str(err), "\n"

try:
  print "getLastDocumentId Multi Known IDs:", result.getLastDocumentId()
except Exception, err:
  print "getLastDocumentId Multi Known IDs:", str(err), "\n"

print "#lastDocumentIds Multi Known IDs:", len(result.lastDocumentIds)
print "#getLastDocumentIds Multi Known IDs:", len(result.getLastDocumentIds())
print "First lastDocumentIds Multi Known IDs:", result.lastDocumentIds[0]
print "First getLastDocumentIds Multi Known IDs:", result.getLastDocumentIds()[0]
print "Second lastDocumentIds Multi Known IDs:", result.lastDocumentIds[1]
print "Second getLastDocumentIds Multi Known IDs:", result.getLastDocumentIds()[1]

result = collection.add([]).execute()
print "Affected Rows Empty List:", result.affectedItemCount, "\n"
try:
  print "lastDocumentId Empty List:", result.lastDocumentId
except Exception, err:
  print "lastDocumentId Empty List:", str(err), "\n"

try:
  print "getLastDocumentId Empty List:", result.getLastDocumentId()
except Exception, err:
  print "getLastDocumentId Empty List:", str(err), "\n"

print "#lastDocumentIds Empty List:", len(result.lastDocumentIds)
print "#getLastDocumentIds Empty List:", len(result.getLastDocumentIds())


result = collection.add({ "name": 'my fourth', "passed": 'again', "count": 4 }).add({ "name": 'my fifth', "passed": 'once again', "count": 5 }).execute()
print "Affected Rows Chained:", result.affectedItemCount, "\n"

result = collection.add(mysqlx.expr('{"name": "my fifth", "passed": "document", "count": 1}')).execute()
print "Affected Rows Single Expression:", result.affectedItemCount, "\n"

result = collection.add([{ "name": 'my sexth', "passed": 'again', "count": 5 }, mysqlx.expr('{"name": "my senevth", "passed": "yep again", "count": 5}')]).execute()
print "Affected Rows Mixed List:", result.affectedItemCount, "\n"

# Cleanup
mySession.dropSchema('js_shell_test')
mySession.close()