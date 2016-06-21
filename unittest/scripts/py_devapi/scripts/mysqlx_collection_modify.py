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
# Collection.Modify Unit Testing: Dynamic Behavior
# ------------------------------------------------
#@ CollectionModify: valid operations after modify and set
crud = collection.modify()
validate_crud_functions(crud, ['set', 'unset', 'merge', 'array_insert', 'array_append', 'array_delete'])
crud = crud.set('name', 'dummy')
validate_crud_functions(crud, ['set', 'unset', 'merge', 'array_insert', 'array_append', 'array_delete', 'sort', 'limit', 'bind', 'execute', '__shell_hook__'])

#@ CollectionModify: valid operations after modify and unset empty
crud = collection.modify()
validate_crud_functions(crud, ['set', 'unset', 'merge', 'array_insert', 'array_append', 'array_delete'])
crud.unset([])
validate_crud_functions(crud, ['set', 'unset', 'merge', 'array_insert', 'array_append', 'array_delete'])

#@ CollectionModify: valid operations after modify and unset list
crud = collection.modify()
validate_crud_functions(crud, ['set', 'unset', 'merge', 'array_insert', 'array_append', 'array_delete'])
crud = crud.unset(['name', 'type'])
validate_crud_functions(crud, ['set', 'unset', 'merge', 'array_insert', 'array_append', 'array_delete', 'sort', 'limit', 'bind', 'execute', '__shell_hook__'])

#@ CollectionModify: valid operations after modify and unset multiple params
crud = collection.modify()
validate_crud_functions(crud, ['set', 'unset', 'merge', 'array_insert', 'array_append', 'array_delete'])
crud = crud.unset('name', 'type')
validate_crud_functions(crud, ['set', 'unset', 'merge', 'array_insert', 'array_append', 'array_delete', 'sort', 'limit', 'bind', 'execute', '__shell_hook__'])

#@ CollectionModify: valid operations after modify and merge
crud = collection.modify()
validate_crud_functions(crud, ['set', 'unset', 'merge', 'array_insert', 'array_append', 'array_delete'])
crud = crud.merge({'att':'value','second':'final'})
validate_crud_functions(crud, ['set', 'unset', 'merge', 'array_insert', 'array_append', 'array_delete', 'sort', 'limit', 'bind', 'execute', '__shell_hook__'])

#@ CollectionModify: valid operations after modify and array_insert
crud = collection.modify()
validate_crud_functions(crud, ['set', 'unset', 'merge', 'array_insert', 'array_append', 'array_delete'])
crud = crud.array_insert('hobbies[3]', 'run')
validate_crud_functions(crud, ['set', 'unset', 'merge', 'array_insert', 'array_append', 'array_delete', 'sort', 'limit', 'bind', 'execute', '__shell_hook__'])

#@ CollectionModify: valid operations after modify and array_append
crud = collection.modify()
validate_crud_functions(crud, ['set', 'unset', 'merge', 'array_insert', 'array_append', 'array_delete'])
crud = crud.array_append('hobbies','skate')
validate_crud_functions(crud, ['set', 'unset', 'merge', 'array_insert', 'array_append', 'array_delete', 'sort', 'limit', 'bind', 'execute', '__shell_hook__'])

#@ CollectionModify: valid operations after modify and array_delete
crud = collection.modify()
validate_crud_functions(crud, ['set', 'unset', 'merge', 'array_insert', 'array_append', 'array_delete'])
crud = crud.array_delete('hobbies[5]')
validate_crud_functions(crud, ['set', 'unset', 'merge', 'array_insert', 'array_append', 'array_delete', 'sort', 'limit', 'bind', 'execute', '__shell_hook__'])

#@ CollectionModify: valid operations after sort
crud = crud.sort(['name'])
validate_crud_functions(crud, ['limit', 'bind', 'execute', '__shell_hook__'])

#@ CollectionModify: valid operations after limit
crud = crud.limit(2)
validate_crud_functions(crud, ['bind', 'execute', '__shell_hook__'])

#@ CollectionModify: valid operations after bind
crud = collection.modify('name = :data').set('age', 15).bind('data', 'angel')
validate_crud_functions(crud, ['bind', 'execute', '__shell_hook__'])

#@ CollectionModify: valid operations after execute
result = crud.execute()
validate_crud_functions(crud, ['bind', 'execute', '__shell_hook__'])

#@ Reusing CRUD with binding
print 'Updated Angel:', result.affected_item_count, '\n'
result=crud.bind('data', 'carol').execute()
print 'Updated Carol:', result.affected_item_count, '\n'


# ----------------------------------------------
# Collection.Modify Unit Testing: Error Conditions
# ----------------------------------------------

#@# CollectionModify: Error conditions on modify
crud = collection.modify(5)
crud = collection.modify('test = "2')

#@# CollectionModify: Error conditions on set
crud = collection.modify().set()
crud = collection.modify().set(45, 'whatever')
crud = collection.modify().set('', 5)


#@# CollectionModify: Error conditions on unset
crud = collection.modify().unset()
crud = collection.modify().unset({})
crud = collection.modify().unset({}, '')
crud = collection.modify().unset(['name', 1])
crud = collection.modify().unset('')

#@# CollectionModify: Error conditions on merge
crud = collection.modify().merge()
crud = collection.modify().merge('')

#@# CollectionModify: Error conditions on array_insert
crud = collection.modify().array_insert()
crud = collection.modify().array_insert(5, 'another')
crud = collection.modify().array_insert('', 'another')
crud = collection.modify().array_insert('test', 'another')

#@# CollectionModify: Error conditions on array_append
crud = collection.modify().array_append()
crud = collection.modify().array_append({},'')
crud = collection.modify().array_append('',45)
crud = collection.modify().array_append('data', mySession)

#@# CollectionModify: Error conditions on array_delete
crud = collection.modify().array_delete()
crud = collection.modify().array_delete(5)
crud = collection.modify().array_delete('')
crud = collection.modify().array_delete('test')

#@# CollectionModify: Error conditions on sort
crud = collection.modify().unset('name').sort()
crud = collection.modify().unset('name').sort(5)
crud = collection.modify().unset('name').sort([])
crud = collection.modify().unset('name').sort(['name', 5])

#@# CollectionModify: Error conditions on limit
crud = collection.modify().unset('name').limit()
crud = collection.modify().unset('name').limit('')

#@# CollectionModify: Error conditions on bind
crud = collection.modify('name = :data and age > :years').set('hobby', 'swim').bind()
crud = collection.modify('name = :data and age > :years').set('hobby', 'swim').bind(5, 5)
crud = collection.modify('name = :data and age > :years').set('hobby', 'swim').bind('another', 5)

#@# CollectionModify: Error conditions on execute
crud = collection.modify('name = :data and age > :years').set('hobby', 'swim').execute()
crud = collection.modify('name = :data and age > :years').set('hobby', 'swim').bind('years', 5).execute()


# ---------------------------------------
# Collection.Modify Unit Testing: Execution
# ---------------------------------------

#@# CollectionModify: Set Execution
result = collection.modify('name = "brian"').set('alias', 'bri').set('last_name', 'black').set('age', mysqlx.expr('13+1')).execute()
print 'Set Affected Rows:', result.affected_item_count, '\n'

result = collection.find('name = "brian"').execute()
doc = result.fetch_one()
print dir(doc)

#@# CollectionModify: Set Execution Binding Array
result = collection.modify('name = "brian"').set('hobbies', mysqlx.expr(':list')).bind('list', ['soccer', 'dance', 'reading']).execute()
print 'Set Affected Rows:', result.affected_item_count, '\n'

result = collection.find('name = "brian"').execute()
doc = result.fetch_one()
print dir(doc)
print doc.hobbies[0]
print doc.hobbies[1]
print doc.hobbies[2]

#@ CollectionModify: Simple Unset Execution
result = collection.modify('name = "brian"').unset('last_name').execute()
print 'Unset Affected Rows:', result.affected_item_count, '\n'

result = collection.find('name = "brian"').execute()
doc = result.fetch_one()
print dir(doc)

#@ CollectionModify: List Unset Execution
result = collection.modify('name = "brian"').unset(['alias', 'age']).execute()
print 'Unset Affected Rows:', result.affected_item_count, '\n'

result = collection.find('name = "brian"').execute()
doc = result.fetch_one()
print dir(doc)

#@ CollectionModify: Merge Execution
result = collection.modify('name = "brian"').merge({'last_name':'black', "age":15, 'alias':'bri', 'girlfriends':['martha', 'karen']}).execute()
print 'Merge Affected Rows:', result.affected_item_count, '\n'

result = collection.find('name = "brian"').execute()
doc = result.fetch_one()
print "Brian's last_name:",  doc.last_name, '\n'
print "Brian's age:",  doc.age, '\n'
print "Brian's alias:",  doc.alias, '\n'
print "Brian's first girlfriend:",  doc.girlfriends[0], '\n'
print "Brian's second girlfriend:",  doc.girlfriends[1], '\n'

#@ CollectionModify: array_append Execution
result = collection.modify('name = "brian"').array_append('girlfriends','cloe').execute()
print 'Array Append Affected Rows:', result.affected_item_count, '\n'

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

result = collection.find('name = "brian"').execute()
doc = result.fetch_one()
print "Brian's girlfriends:", len(doc.girlfriends)
print "Brian's last:", doc.girlfriends[2]

#@ CollectionModify: array_insert Execution
result = collection.modify('name = "brian"').array_insert('girlfriends[1]','samantha').execute()
print 'Array Insert Affected Rows:', result.affected_item_count, '\n'

result = collection.find('name = "brian"').execute()
doc = result.fetch_one()
print "Brian's girlfriends:", len(doc.girlfriends), '\n'
print "Brian's second:", doc.girlfriends[1], '\n'

#@ CollectionModify: array_delete Execution
result = collection.modify('name = "brian"').array_delete('girlfriends[2]').execute()
print 'Array Delete Affected Rows:', result.affected_item_count, '\n'

result = collection.find('name = "brian"').execute()
doc = result.fetch_one()
print "Brian's girlfriends:", len(doc.girlfriends), '\n'
print "Brian's third:", doc.girlfriends[2], '\n'

#@ CollectionModify: sorting and limit Execution
result = collection.modify('age = 15').set('sample', 'in_limit').sort(['name']).limit(2).execute()
print 'Affected Rows:', result.affected_item_count, '\n'

result = collection.find('age = 15').sort(['name']).execute()

#@ CollectionModify: sorting and limit Execution - 1
doc = result.fetch_one()
print dir(doc)

#@ CollectionModify: sorting and limit Execution - 2
doc = result.fetch_one()
print dir(doc)

#@ CollectionModify: sorting and limit Execution - 3
doc = result.fetch_one()
print dir(doc)

#@ CollectionModify: sorting and limit Execution - 4
doc = result.fetch_one()
print dir(doc)

# Cleanup
mySession.drop_schema('js_shell_test')
mySession.close()