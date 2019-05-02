# Assumptions: validate_crud_functions available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
from mysqlsh import mysqlx

mySession = mysqlx.get_session(__uripwd)

ensure_schema_does_not_exist(mySession, 'js_shell_test')

schema = mySession.create_schema('js_shell_test')

# Creates a test collection and inserts data into it
collection = schema.create_collection('collection1')

result = collection.add({"_id": "3C514FF38144B714E7119BCF48B4CA01", "name": 'jack', "age": 17, "gender": 'male'}).execute()
result = collection.add({"_id": "3C514FF38144B714E7119BCF48B4CA02", "name": 'adam', "age": 15, "gender": 'male'}).execute()
result = collection.add({"_id": "3C514FF38144B714E7119BCF48B4CA03", "name": 'brian', "age": 14, "gender": 'male'}).execute()
result = collection.add({"_id": "3C514FF38144B714E7119BCF48B4CA04", "name": 'alma', "age": 13, "gender": 'female'}).execute()
result = collection.add({"_id": "3C514FF38144B714E7119BCF48B4CA05", "name": 'carol', "age": 14, "gender": 'female'}).execute()
result = collection.add({"_id": "3C514FF38144B714E7119BCF48B4CA06", "name": 'donna', "age": 16, "gender": 'female'}).execute()
result = collection.add({"_id": "3C514FF38144B714E7119BCF48B4CA07", "name": 'angel', "age": 14, "gender": 'male'}).execute()

# ----------------------------------------------
# Collection.Find Unit Testing: Dynamic Behavior
# ----------------------------------------------
#@ CollectionFind: valid operations after find
crud = collection.find()
validate_crud_functions(crud, ['fields', 'group_by', 'sort', 'limit', 'lock_shared', 'lock_exclusive', 'bind', 'execute'])

#@ CollectionFind: valid operations after fields
crud = crud.fields(['name'])
validate_crud_functions(crud, ['group_by', 'sort', 'limit', 'lock_shared', 'lock_exclusive', 'bind', 'execute'])

#@ CollectionFind: valid operations after group_by
crud = crud.group_by(['name'])
validate_crud_functions(crud, ['having', 'sort', 'limit', 'lock_shared', 'lock_exclusive', 'bind', 'execute'])

#@ CollectionFind: valid operations after having
crud = crud.having('age > 10')
validate_crud_functions(crud, ['sort', 'limit', 'lock_shared', 'lock_exclusive', 'bind', 'execute'])

#@ CollectionFind: valid operations after sort
crud = crud.sort(['age'])
validate_crud_functions(crud, ['limit', 'lock_shared', 'lock_exclusive', 'bind', 'execute'])

#@ CollectionFind: valid operations after limit
crud = crud.limit(1)
validate_crud_functions(crud, ['skip', 'offset', 'lock_shared', 'lock_exclusive', 'bind', 'execute'])

#@ CollectionFind: valid operations after offset
crud = crud.offset(1)
validate_crud_functions(crud, ['lock_shared', 'lock_exclusive', 'bind', 'execute'])

#@ CollectionFind: valid operations after skip
crud = collection.find().limit(10).skip(1)
validate_crud_functions(crud, ['lock_shared', 'lock_exclusive', 'bind', 'execute'])

#@ CollectionFind: valid operations after lock_shared
crud = collection.find('name = :data').lock_shared()
validate_crud_functions(crud, ['bind', 'execute'])

#@ CollectionFind: valid operations after lock_exclusive
crud = collection.find('name = :data').lock_exclusive()
validate_crud_functions(crud, ['bind', 'execute'])

#@ CollectionFind: valid operations after bind
crud = collection.find('name = :data').bind('data', 'adam')
validate_crud_functions(crud, ['bind', 'execute'])

#@ CollectionFind: valid operations after execute
result = crud.execute()
validate_crud_functions(crud, ['limit', 'bind', 'execute'])

#@ CollectionFind: valid operations after execute with limit
result = crud.limit(1).bind('data', 'adam').execute()
validate_crud_functions(crud, ['limit', 'offset', 'skip', 'bind', 'execute'])

#@ Reusing CRUD with binding
print result.fetch_one().name + '\n'
result=crud.bind('data', 'alma').execute()
print result.fetch_one().name + '\n'


# ----------------------------------------------
# Collection.Find Unit Testing: Error Conditions
# ----------------------------------------------

#@# CollectionFind: Error conditions on find
crud = collection.find(5)
crud = collection.find('test = "2')

#@# CollectionFind: Error conditions on fields
crud = collection.find().fields()
crud = collection.find().fields(5)
crud = collection.find().fields([])
crud = collection.find().fields(['name as alias', 5])
crud = collection.find().fields(mysqlx.expr('concat(field, "whatever")'));
crud = collection.find().fields('name as alias', 5)

#@# CollectionFind: Error conditions on group_by
crud = collection.find().group_by()
crud = collection.find().group_by(5)
crud = collection.find().group_by([])
crud = collection.find().group_by(['name', 5])
crud = collection.find().group_by('name', 5)

#@# CollectionFind: Error conditions on having
crud = collection.find().group_by(['name']).having()
crud = collection.find().group_by(['name']).having(5)

#@# CollectionFind: Error conditions on sort
crud = collection.find().sort()
crud = collection.find().sort(5)
crud = collection.find().sort([])
crud = collection.find().sort(['name', 5])
crud = collection.find().sort('name', 5)

#@# CollectionFind: Error conditions on limit
crud = collection.find().limit()
crud = collection.find().limit('')

#@# CollectionFind: Error conditions on offset
crud = collection.find().limit(1).offset()
crud = collection.find().limit(1).offset('')

#@# CollectionFind: Error conditions on skip
crud = collection.find().limit(1).skip()
crud = collection.find().limit(1).skip('')

#@# CollectionFind: Error conditions on lock_shared
crud = collection.find().lock_shared(5,1)
crud = collection.find().lock_shared(5)

#@# CollectionFind: Error conditions on lock_exclusive
crud = collection.find().lock_exclusive(5,1)
crud = collection.find().lock_exclusive(5)

#@# CollectionFind: Error conditions on bind
crud = collection.find('name = :data and age > :years').bind()
crud = collection.find('name = :data and age > :years').bind(5, 5)
crud = collection.find('name = :data and age > :years').bind('another', 5)

#@# CollectionFind: Error conditions on execute
crud = collection.find('name = :data and age > :years').execute()
crud = collection.find('name = :data and age > :years').bind('years', 5).execute()


# ---------------------------------------
# Collection.Find Unit Testing: Execution
# ---------------------------------------

#@ Collection.Find All
//! [CollectionFind: All Records]
records = collection.find().execute().fetch_all()
print "All:", len(records), "\n"
//! [CollectionFind: All Records]

#@ Collection.Find Filtering
//! [CollectionFind: Filtering]
records = collection.find('gender = "male"').execute().fetch_all()
print "Males:", len(records), "\n"

records = collection.find('gender = "female"').execute().fetch_all()
print "Females:", len(records), "\n"
//! [CollectionFind: Filtering]

records = collection.find('age = 13').execute().fetch_all()
print "13 Years:", len(records), "\n"

records = collection.find('age = 14').execute().fetch_all()
print "14 Years:", len(records), "\n"

records = collection.find('age < 17').execute().fetch_all()
print "Under 17:", len(records), "\n"

records = collection.find('name like "a%"').execute().fetch_all()
print "Names With A:", len(records), "\n"

#@ Collection.Find Field Selection
result = collection.find().fields(['name','age']).execute()
record = result.fetch_one()

# Since a DbDoc in python is a dictionary we can iterate over its members
# using keys()
columns = record.keys()

# Keys come in alphabetic order
print '1-Metadata Length:', len(columns), '\n'
print '1-Metadata Field:', columns[1], '\n'
print '1-Metadata Field:', columns[0], '\n'

result = collection.find().fields(['age']).execute()
record = result.fetch_one()
all_members = dir(record)

# Since a DbDoc in python is a dictionary we can iterate over its members
# using keys()
columns = record.keys()

print '2-Metadata Length:', len(columns), '\n'
print '2-Metadata Field:', columns[0], '\n'

#@ Collection.Find Sorting
//! [CollectionFind: Sorting]
records = collection.find().sort(['name']).execute().fetch_all()
for index in xrange(7):
  print 'Find Asc', index, ':', records[index].name, '\n'

records = collection.find().sort(['name desc']).execute().fetch_all()
for index in xrange(7):
	print 'Find Desc', index, ':', records[index].name, '\n'
//! [CollectionFind: Sorting]

#@ Collection.Find Limit and Offset
//! [CollectionFind: Limit and Offset]
records = collection.find().limit(4).execute().fetch_all()
print 'Limit-Offset 0 :', len(records), '\n'

for index in xrange(8):
	records = collection.find().limit(4).offset(index + 1).execute().fetch_all()
	print 'Limit-Offset', index + 1, ':', len(records), '\n'
//! [CollectionFind: Limit and Offset]

#@ Collection.Find Parameter Binding
//! [CollectionFind: Parameter Binding]
records = collection.find('age = :years and gender = :heorshe').bind('years', 13).bind('heorshe', 'female').execute().fetch_all()
print 'Find Binding Length:', len(records), '\n'
print 'Find Binding Name:', records[0].name, '\n'
//! [CollectionFind: Parameter Binding]

#@ Collection.Find Field Selection Using Field List
//! [CollectionFind: Field Selection List]
result = collection.find('name = "jack"').fields(['ucase(name) as FirstName', 'age as Age']).execute();
record = result.fetch_one();
print "First Name: %s\n" % record.FirstName
print "Age: %s\n" % record.Age
//! [CollectionFind: Field Selection List]

#@ Collection.Find Field Selection Using Field Parameters
//! [CollectionFind: Field Selection Parameters]
result = collection.find('name = "jack"').fields('ucase(name) as FirstName', 'age as Age').execute();
record = result.fetch_one();
print "First Name: %s\n" % record.FirstName
print "Age: %s\n" % record.Age
//! [CollectionFind: Field Selection Parameters]

#@ Collection.Find Field Selection Using Projection Expression
//! [CollectionFind: Field Selection Projection]
result = collection.find('name = "jack"').fields(mysqlx.expr('{"FirstName":ucase(name), "InThreeYears":age + 3}')).execute();
record = result.fetch_one();
print "First Name: %s\n" % record.FirstName
print "In Three Years: %s\n" % record.InThreeYears
//! [CollectionFind: Field Selection Projection]

#@<> WL12813 Collection.Find Collection
collection = schema.create_collection('wl12813')
result = collection.add({ "_id": '4C514FF38144B714E7119BCF48B4CA01', "like": 'foo', "nested": { "like": 'bar' } }).execute()
result = collection.add({ "_id": '4C514FF38144B714E7119BCF48B4CA02', "like": 'foo', "nested": { "like": 'nested bar' } }).execute()
result = collection.add({ "_id": '4C514FF38144B714E7119BCF48B4CA03', "like": 'top foo', "nested": { "like": 'bar' } }).execute()
result = collection.add({ "_id": '4C514FF38144B714E7119BCF48B4CA04', "like": 'top foo', "nested": { "like": 'nested bar' } }).execute()
result = collection.add({ "_id": '4C514FF38144B714E7119BCF48B4CA05', "like": 'bar', "nested": { "like": 'foo' } }).execute()
result = collection.add({ "_id": '4C514FF38144B714E7119BCF48B4CA06', "like": 'bar', "nested": { "like": 'nested foo' } }).execute()
result = collection.add({ "_id": '4C514FF38144B714E7119BCF48B4CA07', "like": 'top bar', "nested": { "like": 'foo' } }).execute()
result = collection.add({ "_id": '4C514FF38144B714E7119BCF48B4CA08', "like": 'top bar', "nested": { "like": 'nested foo' } }).execute()

#@ WL12813 Collection Test 01
# Collection.Find foo, like as column
collection.find('`like` = "foo"').execute()

#@ WL12813 Collection Test 02 [USE:WL12813 Collection Test 01]
# Collection.Find foo, unescaped like as column
collection.find('like = "foo"').execute()

#@ WL12813 Collection Test 03
# Collection.Find foo, nested like as column
collection.find('nested.`like` = "foo"').execute()

#@ WL12813 Collection Test 04
# Collection.Find % bar, like as column and operand
collection.find('`like` LIKE "%bar"').execute()

#@ WL12813 Collection Test 05 [USE:WL12813 Collection Test 04]
# Collection.Find % bar, like as unescaped column and operand
collection.find('like LIKE "%bar"').execute()

#@ WL12813 Collection Test 06
# Collection.Find % bar, nested like as column
collection.find('nested.`like` LIKE "%bar"').execute()

#@ WL12813 Collection Test 07 [USE:WL12813 Collection Test 04]
# Collection.Find like as column, operand and placeholder
collection.find('`like` LIKE :like').bind('like', '%bar').execute()

#@ WL12813 Collection Test 08 [USE:WL12813 Collection Test 04]
# Collection.Find unescaped like as column, operand and placeholder
collection.find('like LIKE :like').bind('like', '%bar').execute()

#@ WL12813 Collection Test 09 [USE:WL12813 Collection Test 06]
# Collection.Find, nested like as column, operand and placeholder
collection.find('nested.`like` LIKE :like').bind('like', '%bar').execute()

#@<> WL12767 Collection.Find Collection
collection = schema.create_collection('wl12767')
result = collection.add({ "_id": '4C514FF38144B714E7119BCF48B4CA01', "name":"one", "list": [1,2,3], "overlaps": [4,5,6] }).execute()
result = collection.add({ "_id": '4C514FF38144B714E7119BCF48B4CA02', "name":"two", "list": [1,2,3], "overlaps": [3,4,5] }).execute()
result = collection.add({ "_id": '4C514FF38144B714E7119BCF48B4CA03', "name":"three", "list": [1,2,3], "overlaps": [1,2,3,4,5,6] }).execute()
result = collection.add({ "_id": '4C514FF38144B714E7119BCF48B4CA04', "name":"four", "list": [1,2,3,4,5,6], "overlaps": [4,5,6] }).execute()
result = collection.add({ "_id": '4C514FF38144B714E7119BCF48B4CA05', "name":"five", "list": [1,2,3], "overlaps": [1,3,5] }).execute()
result = collection.add({ "_id": '4C514FF38144B714E7119BCF48B4CA06', "name":"six", "list": [1,2,3], "overlaps": [7,8,9] }).execute()
result = collection.add({ "_id": '4C514FF38144B714E7119BCF48B4CA07', "name":"seven", "list": [1,3,5], "overlaps": [2,4,6] }).execute()

#@ WL12767-TS1_1-01
# WL12767-TS6_1
collection.find("`overlaps` overlaps `list`").fields(["name"]).execute()

#@ WL12767-TS1_1-02 [USE:WL12767-TS1_1-01] {VER(>=8.0.17)}
# WL12767-TS3_1
collection.find("overlaps overlaps list").fields(["name"]).execute()

#@ WL12767-TS1_1-03 [USE:WL12767-TS1_1-01] {VER(>=8.0.17)}
collection.find("`list` overlaps `overlaps`").fields(["name"]).execute()

#@ WL12767-TS1_1-04 [USE:WL12767-TS1_1-01] {VER(>=8.0.17)}
# WL12767-TS3_1
collection.find("list overlaps overlaps").fields(["name"]).execute()

#@ WL12767-TS1_1-05 {VER(>=8.0.17)}
collection.find("`overlaps` not overlaps `list`").fields(["name"]).execute()

#@ WL12767-TS1_1-06 [USE:WL12767-TS1_1-05] {VER(>=8.0.17)}
# WL12767-TS2_1
# WL12767-TS3_1
collection.find("overlaps not OVERLAPS list").fields(["name"]).execute()

#@ WL12767-TS1_1-07 [USE:WL12767-TS1_1-05] {VER(>=8.0.17)}
collection.find("`list` not overlaps `overlaps`").fields(["name"]).execute()

#@ WL12767-TS1_1-08 [USE:WL12767-TS1_1-05] {VER(>=8.0.17)}
# WL12767-TS2_1
# WL12767-TS3_1
collection.find("list not OvErLaPs overlaps").fields(["name"]).execute()

# Cleanup
mySession.drop_schema('js_shell_test')
mySession.close()
