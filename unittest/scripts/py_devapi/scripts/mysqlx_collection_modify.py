# Assumptions: validate_crud_functions available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
from mysqlsh import mysqlx

mySession = mysqlx.get_session(__uripwd)

ensure_schema_does_not_exist(mySession, 'js_shell_test')

schema = mySession.create_schema('js_shell_test')

# Creates a test collection and inserts data into it
collection = schema.create_collection('collection1')

result = collection.add({"_id": '5C514FF38144957BE71111C04E0D1246',  "name": 'jack', "age": 17, "gender": 'male'}).execute()
result = collection.add({"_id": '5C514FF38144957BE71111C04E0D1247',  "name": 'adam', "age": 15, "gender": 'male'}).execute()
result = collection.add({"_id": '5C514FF38144957BE71111C04E0D1248',  "name": 'brian', "age": 14, "gender": 'male'}).execute()
result = collection.add({"_id": '5C514FF38144957BE71111C04E0D1249',  "name": 'alma', "age": 13, "gender": 'female'}).execute()
result = collection.add({"_id": '5C514FF38144957BE71111C04E0D1250',  "name": 'carol', "age": 14, "gender": 'female'}).execute()
result = collection.add({"_id": '5C514FF38144957BE71111C04E0D1251',  "name": 'donna', "age": 16, "gender": 'female'}).execute()
result = collection.add({"_id": '5C514FF38144957BE71111C04E0D1252',  "name": 'angel', "age": 14, "gender": 'male'}).execute()

# ------------------------------------------------
# Collection.Modify Unit Testing: Dynamic Behavior
# ------------------------------------------------
#@ CollectionModify: valid operations after modify and set
crud = collection.modify('some_filter')
validate_crud_functions(crud, ['set', 'unset', 'merge', 'patch', 'array_insert', 'array_append', 'array_delete'])
crud = crud.set('name', 'dummy')
validate_crud_functions(crud, ['set', 'unset', 'merge', 'patch', 'array_insert', 'array_append', 'array_delete', 'sort', 'limit', 'bind', 'execute'])

#@ CollectionModify: valid operations after modify and unset empty
crud = collection.modify('some_filter')
validate_crud_functions(crud, ['set', 'unset', 'merge', 'patch', 'array_insert', 'array_append', 'array_delete'])
crud.unset([])
validate_crud_functions(crud, ['set', 'unset', 'merge', 'patch', 'array_insert', 'array_append', 'array_delete'])

#@ CollectionModify: valid operations after modify and unset list
crud = collection.modify('some_filter')
validate_crud_functions(crud, ['set', 'unset', 'merge', 'patch', 'array_insert', 'array_append', 'array_delete'])
crud = crud.unset(['name', 'type'])
validate_crud_functions(crud, ['set', 'unset', 'merge', 'patch', 'array_insert', 'array_append', 'array_delete', 'sort', 'limit', 'bind', 'execute'])

#@ CollectionModify: valid operations after modify and unset multiple params
crud = collection.modify('some_filter')
validate_crud_functions(crud, ['set', 'unset', 'merge', 'patch', 'array_insert', 'array_append', 'array_delete'])
crud = crud.unset('name', 'type')
validate_crud_functions(crud, ['set', 'unset', 'merge', 'patch', 'array_insert', 'array_append', 'array_delete', 'sort', 'limit', 'bind', 'execute'])

#@ CollectionModify: valid operations after modify and merge
crud = collection.modify('some_filter')
validate_crud_functions(crud, ['set', 'unset', 'merge', 'patch', 'array_insert', 'array_append', 'array_delete'])
crud = crud.merge({'att':'value','second':'final'})
validate_crud_functions(crud, ['set', 'unset', 'merge', 'patch', 'array_insert', 'array_append', 'array_delete', 'sort', 'limit', 'bind', 'execute'])

#@ CollectionModify: valid operations after modify and patch
crud = collection.modify('some_filter')
validate_crud_functions(crud, ['set', 'unset', 'merge', 'patch', 'array_insert', 'array_append', 'array_delete'])
crud = crud.merge({'att':'value','second':'final'})
validate_crud_functions(crud, ['set', 'unset', 'merge', 'patch', 'array_insert', 'array_append', 'array_delete', 'sort', 'limit', 'bind', 'execute'])

#@ CollectionModify: valid operations after modify and array_insert
crud = collection.modify('some_filter')
validate_crud_functions(crud, ['set', 'unset', 'merge', 'patch', 'array_insert', 'array_append', 'array_delete'])
crud = crud.array_insert('hobbies[3]', 'run')
validate_crud_functions(crud, ['set', 'unset', 'merge', 'patch', 'array_insert', 'array_append', 'array_delete', 'sort', 'limit', 'bind', 'execute'])

#@ CollectionModify: valid operations after modify and array_append
crud = collection.modify('some_filter')
validate_crud_functions(crud, ['set', 'unset', 'merge', 'patch', 'array_insert', 'array_append', 'array_delete'])
crud = crud.array_append('hobbies','skate')
validate_crud_functions(crud, ['set', 'unset', 'merge', 'patch', 'array_insert', 'array_append', 'array_delete', 'sort', 'limit', 'bind', 'execute'])

#@ CollectionModify: valid operations after modify and array_delete
crud = collection.modify('some_filter')
validate_crud_functions(crud, ['set', 'unset', 'merge', 'patch', 'array_insert', 'array_append', 'array_delete'])
crud = crud.array_delete('hobbies[5]')
validate_crud_functions(crud, ['set', 'unset', 'merge', 'patch', 'array_insert', 'array_append', 'array_delete', 'sort', 'limit', 'bind', 'execute'])

#@ CollectionModify: valid operations after sort
crud = crud.sort(['name'])
validate_crud_functions(crud, ['limit', 'bind', 'execute'])

#@ CollectionModify: valid operations after limit
crud = crud.limit(2)
validate_crud_functions(crud, ['bind', 'execute'])

#@ CollectionModify: valid operations after bind
crud = collection.modify('name = :data').set('age', 15).bind('data', 'angel')
validate_crud_functions(crud, ['bind', 'execute'])

#@ CollectionModify: valid operations after execute
result = crud.execute()
validate_crud_functions(crud, ['bind', 'execute'])

#@ Reusing CRUD with binding
print 'Updated Angel:', result.affected_item_count, '\n'
result=crud.bind('data', 'carol').execute()
print 'Updated Carol:', result.affected_item_count, '\n'


# ----------------------------------------------
# Collection.Modify Unit Testing: Error Conditions
# ----------------------------------------------

#@# CollectionModify: Error conditions on modify
crud = collection.modify()
crud = collection.modify('    ')
crud = collection.modify(5)
crud = collection.modify('test = "2')

#@# CollectionModify: Error conditions on set
crud = collection.modify('some_filter').set()
crud = collection.modify('some_filter').set(45, 'whatever')


#@# CollectionModify: Error conditions on unset
crud = collection.modify('some_filter').unset()
crud = collection.modify('some_filter').unset({})
crud = collection.modify('some_filter').unset({}, '')
crud = collection.modify('some_filter').unset(['name', 1])
crud = collection.modify('some_filter').unset('')

#@# CollectionModify: Error conditions on merge
crud = collection.modify('some_filter').merge()
crud = collection.modify('some_filter').merge('')

#@# CollectionModify: Error conditions on patch
crud = collection.modify('some_filter').patch()
crud = collection.modify('some_filter').patch('')

#@# CollectionModify: Error conditions on array_insert
crud = collection.modify('some_filter').array_insert()
crud = collection.modify('some_filter').array_insert(5, 'another')
crud = collection.modify('some_filter').array_insert('', 'another')
crud = collection.modify('some_filter').array_insert('test', 'another')

#@# CollectionModify: Error conditions on array_append
crud = collection.modify('some_filter').array_append()
crud = collection.modify('some_filter').array_append({},'')
crud = collection.modify('some_filter').array_append('',45)
crud = collection.modify('some_filter').array_append('data', mySession)

#@# CollectionModify: Error conditions on array_delete
crud = collection.modify('some_filter').array_delete()
crud = collection.modify('some_filter').array_delete(5)
crud = collection.modify('some_filter').array_delete('')
crud = collection.modify('some_filter').array_delete('test')

#@# CollectionModify: Error conditions on sort
crud = collection.modify('some_filter').unset('name').sort()
crud = collection.modify('some_filter').unset('name').sort(5)
crud = collection.modify('some_filter').unset('name').sort([])
crud = collection.modify('some_filter').unset('name').sort(['name', 5])
crud = collection.modify('some_filter').unset('name').sort('name', 5)

#@# CollectionModify: Error conditions on limit
crud = collection.modify('some_filter').unset('name').limit()
crud = collection.modify('some_filter').unset('name').limit('')

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

#@<OUT> CollectionModify: Patch initial documents
collection.find('gender="female"')

#@<OUT> CollectionModify: Patch adding fields to multiple documents (WL10856-FR1_1) {VER(>=8.0.4)}
collection.modify('gender="female"').patch({"hobbies":[], "address":"TBD"})
collection.find('gender="female"')

#@<OUT> CollectionModify: Patch updating field on multiple documents (WL10856-FR1_4) {VER(>=8.0.4)}
collection.modify('gender="female"').patch({"address":{"street":"TBD"}})
collection.find('gender="female"')

#@<OUT> CollectionModify: Patch updating field on multiple nested documents (WL10856-FR1_5) {VER(>=8.0.4)}
collection.modify('gender="female"').patch({"address":{"street":"Main"}})
collection.find('gender="female"')

#@<OUT> CollectionModify: Patch adding field on multiple nested documents (WL10856-FR1_2) {VER(>=8.0.4)}
collection.modify('gender="female"').patch({"address":{"number":0}})
collection.find('gender="female"')

#@<OUT> CollectionModify: Patch removing field on multiple nested documents (WL10856-FR1_8) {VER(>=8.0.4)}
collection.modify('gender="female"').patch({"address":{"number":None}})
collection.find('gender="female"')

#@<OUT> CollectionModify: Patch removing field on multiple documents (WL10856-FR1_7) {VER(>=8.0.4)}
collection.modify('gender="female"').patch({"address":None})
collection.find('gender="female"')

#@<OUT> CollectionModify: Patch adding field with multiple calls to patch (WL10856-FR2.1_1) {VER(>=8.0.4)}
collection.modify('gender="female"').patch({"last_name":'doe'}).patch({"address":{}})
collection.find('gender="female"')

#@<OUT> CollectionModify: Patch adding field with multiple calls to patch on nested documents (WL10856-FR2.1_2) {VER(>=8.0.4)}
collection.modify('gender="female"').patch({"address":{"street":'main'}}).patch({"address":{"number":0}})
collection.find('gender="female"')

#@<OUT> CollectionModify: Patch updating fields with multiple calls to patch (WL10856-FR2.1_3, WL10856-FR2.1_4) {VER(>=8.0.4)}
collection.modify('gender="female"').patch({"address":{"street":'riverside'}}).patch({"last_name":'houston'})
collection.find('gender="female"')

#@<OUT> CollectionModify: Patch removing fields with multiple calls to patch (WL10856-FR2.1_5, WL10856-FR2.1_6) {VER(>=8.0.4)}
collection.modify('gender="female"').patch({"address":{"number":None}}).patch({"hobbies":None})
collection.find('gender="female"')

#@<OUT> CollectionModify: Patch adding field to multiple documents using expression (WL10856-ET_13) {VER(>=8.0.4)}
collection.modify('gender="female"').patch({"last_update": mysqlx.expr("curdate()")})
records = collection.find('gender="female"').execute().fetch_all()
last_update = records[0].last_update
records

#@<OUT> CollectionModify: Patch adding field to multiple nested documents using expression (WL10856-ET_14) {VER(>=8.0.4)}
collection.modify('gender="female"').patch({"address": {"street_short": mysqlx.expr("right(address.street, 3)")}})
collection.find('gender="female"')

#@<OUT> CollectionModify: Patch updating field to multiple documents using expression (WL10856-ET_15) {VER(>=8.0.4)}
collection.modify('gender="female"').patch({"name":mysqlx.expr("concat(ucase(left(name,1)), right(name, length(name)-1))")})
collection.find('gender="female"')

#@<OUT> CollectionModify: Patch updating field to multiple nested documents using expression (WL10856-ET_16) {VER(>=8.0.4)}
collection.modify('gender="female"').patch({"address": {"street_short": mysqlx.expr("left(address.street, 3)")}})
collection.find('gender="female"')

#@<OUT> CollectionModify: Patch including _id, ignores _id applies the rest (WL10856-ET_17) {VER(>=8.0.4)}
collection.modify('gender="female"').patch({"_id":'new_id', "city":'Washington'})
collection.find('gender="female"')

#@ CollectionModify: Patch adding field with null value coming from an expression (WL10856-ET_19) {VER(>=8.0.4)}
collection.modify('gender="female"').patch({"another":mysqlx.expr("null")})

#@<OUT> CollectionModify: Patch updating field with null value coming from an expression (WL10856-ET_20) {VER(>=8.0.4)}
collection.modify('gender="female"').patch({"last_update":mysqlx.expr("null")})
collection.find('gender="female"')

#@ CollectionModify: Patch removing the _id field (WL10856-ET_25) {VER(>=8.0.4)}
collection.modify('gender="female"').patch({"_id":None})

# Cleanup
mySession.drop_schema('js_shell_test')
mySession.close()
