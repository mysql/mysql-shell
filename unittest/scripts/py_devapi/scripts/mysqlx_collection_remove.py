# Assumptions: validate_crud_functions available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
import mysqlx

mySession = mysqlx.get_node_session(__uripwd)

ensure_schema_does_not_exist(mySession, 'js_shell_test')

schema = mySession.create_schema('js_shell_test')

# Creates a test collection and inserts data into it
collection = schema.create_collection('collection1')

result = collection.add({"name": 'jack', "age": 17, "gender": 'male'}).execute()
result = collection.add({"name": 'adam', "age": 15, "gender": 'male'}).execute()
result = collection.add({"name": 'brian', "age": 14, "gender": 'male'}).execute()
result = collection.add({"name": 'alma', "age": 13, "gender": 'female'}).execute()
result = collection.add({"name": 'carol', "age": 14, "gender": 'female'}).execute()
result = collection.add({"name": 'donna', "age": 16, "gender": 'female'}).execute()
result = collection.add({"name": 'angel', "age": 14, "gender": 'male'}).execute()

# ------------------------------------------------
# collection.remove Unit Testing: Dynamic Behavior
# ------------------------------------------------
#@ CollectionRemove: valid operations after remove
crud = collection.remove()
validate_crud_functions(crud, ['sort', 'limit', 'bind', 'execute'])

#@ CollectionRemove: valid operations after sort
crud = crud.sort(['name'])
validate_crud_functions(crud, ['limit', 'bind', 'execute'])

#@ CollectionRemove: valid operations after limit
crud = crud.limit(1)
validate_crud_functions(crud, ['bind', 'execute'])

#@ CollectionRemove: valid operations after bind
crud = collection.remove('name = :data').bind('data', 'donna')
validate_crud_functions(crud, ['bind', 'execute'])

#@ CollectionRemove: valid operations after execute
result = crud.execute()
validate_crud_functions(crud, ['bind', 'execute'])

#@ Reusing CRUD with binding
print 'Deleted donna:', result.affected_item_count, '\n'
result=crud.bind('data', 'alma').execute()
print 'Deleted alma:', result.affected_item_count, '\n'


# ----------------------------------------------
# collection.remove Unit Testing: Error Conditions
# ----------------------------------------------

#@# CollectionRemove: Error conditions on remove
crud = collection.remove(5)
crud = collection.remove('test = "2')

#@# CollectionRemove: Error conditions sort
crud = collection.remove().sort()
crud = collection.remove().sort(5)
crud = collection.remove().sort([])
crud = collection.remove().sort(['name', 5])

#@# CollectionRemove: Error conditions on limit
crud = collection.remove().limit()
crud = collection.remove().limit('')

#@# CollectionRemove: Error conditions on bind
crud = collection.remove('name = :data and age > :years').bind()
crud = collection.remove('name = :data and age > :years').bind(5, 5)
crud = collection.remove('name = :data and age > :years').bind('another', 5)

#@# CollectionRemove: Error conditions on execute
crud = collection.remove('name = :data and age > :years').execute()
crud = collection.remove('name = :data and age > :years').bind('years', 5).execute()

# ---------------------------------------
# collection.remove Unit Testing: Execution
# ---------------------------------------

#@ CollectionRemove: remove under condition
result = collection.remove('age = 15').execute()
print 'Affected Rows:', result.affected_item_count, '\n'

try:
  print "last_document_id:", result.last_document_id
except Exception, err:
  print "last_document_id:", str(err), "\n"

try:
  print "get_last_document_id():", result.get_last_document_id()
except Exception, err:
  print "get_last_document_id():", str(err), "\n"

try:
  print "last_document_ids:", result.last_document_ids
except Exception, err:
  print "last_document_ids:", str(err), "\n"

try:
  print "get_last_document_ids():", result.get_last_document_ids()
except Exception, err:
  print "get_last_document_ids():", str(err), "\n"


docs = collection.find().execute().fetch_all()
print 'Records Left:', len(docs), '\n'

#@ CollectionRemove: remove with binding
result = collection.remove('gender = :heorshe').limit(2).bind('heorshe', 'male').execute()
print 'Affected Rows:', result.affected_item_count, '\n'

docs = collection.find().execute().fetch_all()
print 'Records Left:', len(docs), '\n'

#@ CollectionRemove: full remove
result = collection.remove().execute()
print 'Affected Rows:', result.affected_item_count, '\n'

docs = collection.find().execute().fetch_all()
print 'Records Left:', len(docs), '\n'


# Cleanup
mySession.drop_schema('js_shell_test')
mySession.close()