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

# ----------------------------------------------
# Collection.Find Unit Testing: Dynamic Behavior
# ----------------------------------------------
#@ CollectionFind: valid operations after find
crud = collection.find()
validate_crud_functions(crud, ['fields', 'group_by', 'sort', 'limit', 'bind', 'execute', '__shell_hook__'])

#@ CollectionFind: valid operations after fields
crud = crud.fields(['name'])
validate_crud_functions(crud, ['group_by', 'sort', 'limit', 'bind', 'execute', '__shell_hook__'])

#@ CollectionFind: valid operations after group_by
crud = crud.group_by(['name'])
validate_crud_functions(crud, ['having', 'sort', 'limit', 'bind', 'execute', '__shell_hook__'])

#@ CollectionFind: valid operations after having
crud = crud.having('age > 10')
validate_crud_functions(crud, ['sort', 'limit', 'bind', 'execute', '__shell_hook__'])

#@ CollectionFind: valid operations after sort
crud = crud.sort(['age'])
validate_crud_functions(crud, ['limit', 'bind', 'execute', '__shell_hook__'])

#@ CollectionFind: valid operations after limit
crud = crud.limit(1)
validate_crud_functions(crud, ['skip', 'bind', 'execute', '__shell_hook__'])

#@ CollectionFind: valid operations after skip
crud = crud.skip(1)
validate_crud_functions(crud, ['bind', 'execute', '__shell_hook__'])

#@ CollectionFind: valid operations after bind
crud = collection.find('name = :data').bind('data', 'adam')
validate_crud_functions(crud, ['bind', 'execute', '__shell_hook__'])

#@ CollectionFind: valid operations after execute
result = crud.execute()
validate_crud_functions(crud, ['bind', 'execute', '__shell_hook__'])

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

#@# CollectionFind: Error conditions on group_by
crud = collection.find().group_by()
crud = collection.find().group_by(5)
crud = collection.find().group_by([])
crud = collection.find().group_by(['name', 5])

#@# CollectionFind: Error conditions on having
crud = collection.find().group_by(['name']).having()
crud = collection.find().group_by(['name']).having(5)

#@# CollectionFind: Error conditions on sort
crud = collection.find().sort()
crud = collection.find().sort(5)
crud = collection.find().sort([])
crud = collection.find().sort(['name', 5])

#@# CollectionFind: Error conditions on limit
crud = collection.find().limit()
crud = collection.find().limit('')

#@# CollectionFind: Error conditions on skip
crud = collection.find().limit(1).skip()
crud = collection.find().limit(1).skip('')

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
records = collection.find().execute().fetch_all()
print "All:", len(records), "\n"

#@ Collection.Find Filtering
records = collection.find('gender = "male"').execute().fetch_all()
print "Males:", len(records), "\n"

records = collection.find('gender = "female"').execute().fetch_all()
print "Females:", len(records), "\n"

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
records = collection.find().sort(['name']).execute().fetch_all()
for index in xrange(7):
  print 'Find Asc', index, ':', records[index].name, '\n'

records = collection.find().sort(['name desc']).execute().fetch_all()
for index in xrange(7):
	print 'Find Desc', index, ':', records[index].name, '\n'

#@ Collection.Find Limit and Offset
records = collection.find().limit(4).execute().fetch_all()
print 'Limit-Skip 0 :', len(records), '\n'

for index in xrange(8):
	records = collection.find().limit(4).skip(index + 1).execute().fetch_all()
	print 'Limit-Skip', index + 1, ':', len(records), '\n'

#@ Collection.Find Parameter Binding
records = collection.find('age = :years and gender = :heorshe').bind('years', 13).bind('heorshe', 'female').execute().fetch_all()
print 'Find Binding Length:', len(records), '\n'
print 'Find Binding Name:', records[0].name, '\n'


#@ Collection.Find Field Selection Using Projection Expression
result = collection.find('name = "jack"').fields(mysqlx.expr('{"FirstName":ucase(name), "InThreeYears":age + 3}')).execute();
record = result.fetch_one();
columns = dir(record)
print "%s: %s\n" % (columns[0], record.FirstName)
print "%s: %s\n" % (columns[1], record.InThreeYears)

# Cleanup
mySession.drop_schema('js_shell_test')
mySession.close()