# Assumptions: validate_crud_functions available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
import mysqlx

mySession = mysqlx.getNodeSession(__uripwd)

ensure_schema_does_not_exist(mySession, 'js_shell_test')

schema = mySession.createSchema('js_shell_test')
mySession.setCurrentSchema('js_shell_test')

# Creates a test table with initial data
result = mySession.sql('create table table1 (name varchar(50), age integer, gender varchar(20))').execute()
result = mySession.sql('create view view1 (my_name, my_age, my_gender) as select name, age, gender from table1;').execute()
table = schema.getTable('table1')

result = table.insert({"name": 'jack', "age": 17, "gender": 'male'}).execute()
result = table.insert({"name": 'adam', "age": 15, "gender": 'male'}).execute()
result = table.insert({"name": 'brian', "age": 14, "gender": 'male'}).execute()
result = table.insert({"name": 'alma', "age": 13, "gender": 'female'}).execute()
result = table.insert({"name": 'carol', "age": 14, "gender": 'female'}).execute()
result = table.insert({"name": 'donna', "age": 16, "gender": 'female'}).execute()
result = table.insert({"name": 'angel', "age": 14, "gender": 'male'}).execute()	

# ----------------------------------------------
# Table.Select Unit Testing: Dynamic Behavior
# ----------------------------------------------
#@ TableSelect: valid operations after select
crud = table.select()
validate_crud_functions(crud, ['where', 'groupBy', 'orderBy', 'limit', 'bind', 'execute', '__shell_hook__'])

#@ TableSelect: valid operations after where
crud = crud.where('age > 13')
validate_crud_functions(crud, ['groupBy', 'orderBy', 'limit', 'bind', 'execute', '__shell_hook__'])

#@ TableSelect: valid operations after groupBy
crud = crud.groupBy(['name'])
validate_crud_functions(crud, ['having', 'orderBy', 'limit', 'bind', 'execute', '__shell_hook__'])

#@ TableSelect: valid operations after having
crud = crud.having('age > 10')
validate_crud_functions(crud, ['orderBy', 'limit', 'bind', 'execute', '__shell_hook__'])

#@ TableSelect: valid operations after orderBy
crud = crud.orderBy(['age'])
validate_crud_functions(crud, ['limit', 'bind', 'execute', '__shell_hook__'])

#@ TableSelect: valid operations after limit
crud = crud.limit(1)
validate_crud_functions(crud, ['offset', 'bind', 'execute', '__shell_hook__'])

#@ TableSelect: valid operations after offset
crud = crud.offset(1)
validate_crud_functions(crud, ['bind', 'execute', '__shell_hook__'])

#@ TableSelect: valid operations after bind
crud = table.select().where('name = :data').bind('data', 'adam')
validate_crud_functions(crud, ['bind', 'execute', '__shell_hook__'])

#@ TableSelect: valid operations after execute
result = crud.execute()
validate_crud_functions(crud, ['bind', 'execute', '__shell_hook__'])

#@ Reusing CRUD with binding
print result.fetchOne().name + '\n'
result=crud.bind('data', 'alma').execute()
print result.fetchOne().name + '\n'


# ----------------------------------------------
# Table.Select Unit Testing: Error Conditions
# ----------------------------------------------

#@# TableSelect: Error conditions on select
crud = table.select(5)
crud = table.select([])
crud = table.select(['name as alias', 5])

#@# TableSelect: Error conditions on where
crud = table.select().where()
crud = table.select().where(5)
crud = table.select().where('name = "whatever')

#@# TableSelect: Error conditions on groupBy
crud = table.select().groupBy()
crud = table.select().groupBy(5)
crud = table.select().groupBy([])
crud = table.select().groupBy(['name', 5])

#@# TableSelect: Error conditions on having
crud = table.select().groupBy(['name']).having()
crud = table.select().groupBy(['name']).having(5)

#@# TableSelect: Error conditions on orderBy
crud = table.select().orderBy()
crud = table.select().orderBy(5)
crud = table.select().orderBy([])
crud = table.select().orderBy(['name', 5])

#@# TableSelect: Error conditions on limit
crud = table.select().limit()
crud = table.select().limit('')

#@# TableSelect: Error conditions on offset
crud = table.select().limit(1).offset()
crud = table.select().limit(1).offset('')

#@# TableSelect: Error conditions on bind
crud = table.select().where('name = :data and age > :years').bind()
crud = table.select().where('name = :data and age > :years').bind(5, 5)
crud = table.select().where('name = :data and age > :years').bind('another', 5)

#@# TableSelect: Error conditions on execute
crud = table.select().where('name = :data and age > :years').execute()
crud = table.select().where('name = :data and age > :years').bind('years', 5).execute()


# ---------------------------------------
# Table.Select Unit Testing: Execution
# ---------------------------------------
records

#@ Table.Select All
records = table.select().execute().fetchAll()
print "All:", len(records), "\n"

#@ Table.Select Filtering
records = table.select().where('gender = "male"').execute().fetchAll()
print "Males:", len(records), "\n"

records = table.select().where('gender = "female"').execute().fetchAll()
print "Females:", len(records), "\n"

records = table.select().where('age = 13').execute().fetchAll()
print "13 Years:", len(records), "\n"

records = table.select().where('age = 14').execute().fetchAll()
print "14 Years:", len(records), "\n"

records = table.select().where('age < 17').execute().fetchAll()
print "Under 17:", len(records), "\n"

records = table.select().where('name like "a%"').execute().fetchAll()
print "Names With A:", len(records), "\n"

records = table.select().where('name LIKE "a%"').execute().fetchAll()
print "Names With A:", len(records), "\n"

records = table.select().where('NOT (age = 14)').execute().fetchAll()
print "Not 14 Years:", len(records), "\n"

#@ Table.Select Field Selection
result = table.select(['name','age']).execute()
record = result.fetchOne()
all_members = dir(record)

# Remove the python built in members
columns = []
for member in all_members:
  if not member.startswith('__'):
    columns.append(member)

# In python, members are returned in alphabetic order
# We print the requested members here (getLength and getField are members too)
print '1-Metadata Length:', len(columns), '\n'
print '1-Metadata Field:', columns[4], '\n'
print '1-Metadata Field:', columns[0], '\n'

result = table.select(['age']).execute()
record = result.fetchOne()

all_members = dir(record)

# Remove the python built in members
columns = []
for member in all_members:
  if not member.startswith('__'):
    columns.append(member)

# In python, members are returned in alphabetic order
# We print the requested members here (getLength and getField are members too)
print '2-Metadata Length:', len(columns), '\n'
print '2-Metadata Field:', columns[0], '\n'

#@ Table.Select Sorting
records = table.select().orderBy(['name']).execute().fetchAll()
for index in xrange(7):
	print 'Select Asc', index, ':', records[index].name, '\n'

records = table.select().orderBy(['name desc']).execute().fetchAll()
for index in xrange(7):
	print 'Select Desc', index, ':', records[index].name, '\n'

#@ Table.Select Limit and Offset
records = table.select().limit(4).execute().fetchAll()
print 'Limit-Offset 0 :', len(records), '\n'

for index in xrange(7):
	records = table.select().limit(4).offset(index + 1).execute().fetchAll()
	print 'Limit-Offset', index + 1, ':', len(records), '\n'

#@ Table.Select Parameter Binding through a View
view = schema.getTable('view1');
records = view.select().where('my_age = :years and my_gender = :heorshe').bind('years', 13).bind('heorshe', 'female').execute().fetchAll()
print 'Select Binding Length:', len(records), '\n'
print 'Select Binding Name:', records[0].my_name, '\n'


# Cleanup
mySession.dropSchema('js_shell_test')
mySession.close()