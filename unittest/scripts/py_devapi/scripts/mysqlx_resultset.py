# Assumptions: ensure_schema_does_not_exist
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
from mysqlsh import mysqlx

mySession = mysqlx.get_node_session(__uripwd)

ensure_schema_does_not_exist(mySession, 'js_shell_test')

schema = mySession.create_schema('js_shell_test')
mySession.set_current_schema('js_shell_test')
result = mySession.sql('create table js_shell_test.buffer_table (name varchar(50) primary key, age integer, gender varchar(20))').execute()

#@ SqlResult member validation
sqlMembers = dir(result)
print "SqlResult Members:", sqlMembers
validateMember(sqlMembers, 'execution_time')
validateMember(sqlMembers, 'warning_count')
validateMember(sqlMembers, 'warnings')
validateMember(sqlMembers, 'get_execution_time')
validateMember(sqlMembers, 'get_warning_count')
validateMember(sqlMembers, 'get_warnings')
validateMember(sqlMembers, 'column_count')
validateMember(sqlMembers, 'column_names')
validateMember(sqlMembers, 'columns')
validateMember(sqlMembers, 'get_column_count')
validateMember(sqlMembers, 'get_column_names')
validateMember(sqlMembers, 'get_columns')
validateMember(sqlMembers, 'fetch_one')
validateMember(sqlMembers, 'fetch_all')
validateMember(sqlMembers, 'has_data')
validateMember(sqlMembers, 'next_data_set')
validateMember(sqlMembers, 'affected_row_count')
validateMember(sqlMembers, 'auto_increment_value')
validateMember(sqlMembers, 'get_affected_row_count')
validateMember(sqlMembers, 'get_auto_increment_value')


#@ Result member validation
table = schema.get_table('buffer_table')
result = table.insert({'name': 'jack', 'age': 17, 'gender': 'male'}).execute()
result = table.insert({'name': 'adam', 'age': 15, 'gender': 'male'}).execute()
result = table.insert({'name': 'brian', 'age': 14, 'gender': 'male'}).execute()
result = table.insert({'name': 'alma', 'age': 13, 'gender': 'female'}).execute()
result = table.insert({'name': 'carol', 'age': 14, 'gender': 'female'}).execute()
result = table.insert({'name': 'donna', 'age': 16, 'gender': 'female'}).execute()
result = table.insert({'name': 'angel', 'age': 14, 'gender': 'male'}).execute()

table = schema.get_table('buffer_table')
collection = schema.create_collection('buffer_collection')

resultMembers = dir(result)
print "Result Members:", resultMembers
validateMember(resultMembers, 'execution_time')
validateMember(resultMembers, 'warning_count')
validateMember(resultMembers, 'warnings')
validateMember(resultMembers, 'get_execution_time')
validateMember(resultMembers, 'get_warning_count')
validateMember(resultMembers, 'get_warnings')
validateMember(resultMembers, 'affected_item_count')
validateMember(resultMembers, 'auto_increment_value')
validateMember(resultMembers, 'last_document_id')
validateMember(resultMembers, 'get_affected_item_count')
validateMember(resultMembers, 'get_auto_increment_value')
validateMember(resultMembers, 'get_last_document_id')

#@ RowResult member validation
result = table.select().execute()
rowResultMembers = dir(result)
print "RowResult Members:", rowResultMembers
validateMember(rowResultMembers, 'execution_time')
validateMember(rowResultMembers, 'warning_count')
validateMember(rowResultMembers, 'warnings')
validateMember(rowResultMembers, 'get_execution_time')
validateMember(rowResultMembers, 'get_warning_count')
validateMember(rowResultMembers, 'get_warnings')
validateMember(rowResultMembers, 'column_count')
validateMember(rowResultMembers, 'column_names')
validateMember(rowResultMembers, 'columns')
validateMember(rowResultMembers, 'get_column_count')
validateMember(rowResultMembers, 'get_column_names')
validateMember(rowResultMembers, 'get_columns')
validateMember(rowResultMembers, 'fetch_one')
validateMember(rowResultMembers, 'fetch_all')

#@ DocResult member validation
result = collection.find().execute()
docResultMembers = dir(result)
print "DocRowResult Members:", docResultMembers
validateMember(docResultMembers, 'execution_time')
validateMember(docResultMembers, 'warning_count')
validateMember(docResultMembers, 'warnings')
validateMember(docResultMembers, 'get_execution_time')
validateMember(docResultMembers, 'get_warning_count')
validateMember(docResultMembers, 'get_warnings')
validateMember(docResultMembers, 'fetch_one')
validateMember(docResultMembers, 'fetch_all')

#@ Resultset has_data() False
result = mySession.sql('use js_shell_test').execute()
print 'has_data():', result.has_data()

#@ Resultset has_data() True
result = mySession.sql('select * from buffer_table').execute()
print 'has_data():', result.has_data()


#@ Resultset get_columns()
metadata = result.get_columns()

print 'Field Number:', len(metadata)
print 'First Field:', metadata[0].column_name
print 'Second Field:', metadata[1].column_name
print 'Third Field:', metadata[2].column_name


#@ Resultset columns
metadata = result.columns

print 'Field Number:', len(metadata)
print 'First Field:', metadata[0].column_name
print 'Second Field:', metadata[1].column_name
print 'Third Field:', metadata[2].column_name


#@ Resultset buffering on SQL

result1 = mySession.sql('select name, age from js_shell_test.buffer_table where gender = "male" order by name').execute()
result2 = mySession.sql('select name, gender from js_shell_test.buffer_table where age < 15 order by name').execute()

metadata1 = result1.columns
metadata2 = result2.columns

print "Result 1 Field 1:", metadata1[0].column_name
print "Result 1 Field 2:", metadata1[1].column_name

print "Result 2 Field 1:", metadata2[0].column_name
print "Result 2 Field 2:", metadata2[1].column_name


record1 = result1.fetch_one()
record2 = result2.fetch_one()

print "Result 1 Record 1:", record1.name
print "Result 2 Record 1:", record2.name

record1 = result1.fetch_one()
record2 = result2.fetch_one()

print "Result 1 Record 2:", record1.name
print "Result 2 Record 2:", record2.name

record1 = result1.fetch_one()
record2 = result2.fetch_one()

print "Result 1 Record 3:", record1.name
print "Result 2 Record 3:", record2.name

record1 = result1.fetch_one()
record2 = result2.fetch_one()

print "Result 1 Record 4:", record1.name
print "Result 2 Record 4:", record2.name


#@ Resultset buffering on CRUD

result1 = table.select(['name', 'age']).where('gender = :gender').order_by(['name']).bind('gender','male').execute()
result2 = table.select(['name', 'gender']).where('age < :age').order_by(['name']).bind('age',15).execute()

metadata1 = result1.columns
metadata2 = result2.columns

print "Result 1 Field 1:", metadata1[0].column_name
print "Result 1 Field 2:", metadata1[1].column_name

print "Result 2 Field 1:", metadata2[0].column_name
print "Result 2 Field 2:", metadata2[1].column_name


record1 = result1.fetch_one()
record2 = result2.fetch_one()

print "Result 1 Record 1:", record1.name
print "Result 2 Record 1:", record2.name

record1 = result1.fetch_one()
record2 = result2.fetch_one()

print "Result 1 Record 2:", record1.name
print "Result 2 Record 2:", record2.name

record1 = result1.fetch_one()
record2 = result2.fetch_one()

print "Result 1 Record 3:", record1.name
print "Result 2 Record 3:", record2.name

record1 = result1.fetch_one()
record2 = result2.fetch_one()

print "Result 1 Record 4:", record1.name
print "Result 2 Record 4:", record2.name

#@ Resultset table
print table.select(["count(*)"]).execute().fetch_one()[0]

#@ Resultset row members
result = mySession.sql('select name as alias, age, age as length, gender as alias from buffer_table where name = "jack"').execute();
row = result.fetch_one();

all_members = dir(row)

# Remove the python built in members
members = []
for member in all_members:
  if not member.startswith('__'):
    members.append(member)

print "Member Count: %s" % len(members)
validateMember(members, 'length');
validateMember(members, 'get_field');
validateMember(members, 'get_length');
validateMember(members, 'help');
validateMember(members, 'alias');
validateMember(members, 'age');

# Resultset row index access
print "Name with index: %s" % row[0]
print "Age with index: %s" % row[1]
print "Length with index: %s" % row[2]
print "Gender with index: %s" % row[3]

# Resultset row index access
print "Name with get_field: %s" % row.get_field('alias')
print "Age with get_field: %s" % row.get_field('age')
print "Length with get_field: %s" % row.get_field('length')
print "Unable to get gender from alias: %s" % row.get_field('alias')

# Resultset property access
print "Name with property: %s" % row.alias
print "Age with property: %s" % row.age
print "Unable to get length with property: %s" %  row.length

mySession.close()
