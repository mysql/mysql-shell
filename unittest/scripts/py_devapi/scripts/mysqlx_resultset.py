# Assumptions: ensure_schema_does_not_exist
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
from __future__ import print_function
from mysqlsh import mysqlx

mySession = mysqlx.get_session(__uripwd)

ensure_schema_does_not_exist(mySession, 'js_shell_test')

schema = mySession.create_schema('js_shell_test')
mySession.set_current_schema('js_shell_test')
result = mySession.sql('create table js_shell_test.buffer_table (name varchar(50) primary key, age integer, gender varchar(20))').execute()

#@<> SqlResult member validation
validate_members(result, [
  'execution_time',
  'warning_count',
  'warnings_count',
  'warnings',
  'get_execution_time',
  'get_warning_count',
  'get_warnings',
  'column_count',
  'column_names',
  'columns',
  'get_column_count',
  'get_column_names',
  'get_columns',
  'fetch_one',
  'fetch_one_object',
  'fetch_all',
  'has_data',
  'help',
  'next_data_set',
  'affected_row_count',
  'affected_items_count',
  'auto_increment_value',
  'get_affected_row_count',
  'get_affected_items_count',
  'get_auto_increment_value',
  'get_warnings_count',
  'next_result'])


#@<> Result member validation
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

validate_members(result, [
  'execution_time',
  'warning_count',
  'warnings_count',
  'warnings',
  'get_affected_items_count',
  'get_execution_time',
  'get_warning_count',
  'get_warnings',
  'get_warnings_count',
  'help',
  'affected_item_count',
  'affected_items_count',
  'auto_increment_value',
  'generated_ids',
  'get_affected_item_count',
  'get_auto_increment_value',
  'get_generated_ids'])

#@<> RowResult member validation
result = table.select().execute()

validate_members(result, [
  'affected_items_count',
  'execution_time',
  'warning_count',
  'warnings_count',
  'warnings',
  'fetch_one_object',
  'get_affected_items_count',
  'get_execution_time',
  'get_warning_count',
  'get_warnings',
  'get_warnings_count',
  'help',
  'column_count',
  'column_names',
  'columns',
  'get_column_count',
  'get_column_names',
  'get_columns',
  'fetch_one',
  'fetch_all'])

#@<> DocResult member validation
result = collection.find().execute()

validate_members(result, [
  'affected_items_count',
  'execution_time',
  'warning_count',
  'warnings',
  'warnings_count',
  'get_affected_items_count',
  'get_execution_time',
  'get_warning_count',
  'get_warnings',
  'get_warnings_count',
  'help',
  'fetch_one',
  'fetch_all'])

#@ Resultset has_data() False
result = mySession.sql('use js_shell_test').execute()
print('has_data():', result.has_data())

#@ Resultset has_data() True
result = mySession.sql('select * from buffer_table').execute()
print('has_data():', result.has_data())


#@ Resultset get_columns()
metadata = result.get_columns()

print('Field Number:', len(metadata))
print('First Field:', metadata[0].column_name)
print('Second Field:', metadata[1].column_name)
print('Third Field:', metadata[2].column_name)


#@ Resultset columns
metadata = result.columns

print('Field Number:', len(metadata))
print('First Field:', metadata[0].column_name)
print('Second Field:', metadata[1].column_name)
print('Third Field:', metadata[2].column_name)


#@ Resultset buffering on SQL

result1 = mySession.sql('select name, age from js_shell_test.buffer_table where gender = "male" order by name').execute()
result2 = mySession.sql('select name, gender from js_shell_test.buffer_table where age < 15 order by name').execute()

metadata1 = result1.columns
metadata2 = result2.columns

print("Result 1 Field 1:", metadata1[0].column_name)
print("Result 1 Field 2:", metadata1[1].column_name)

print("Result 2 Field 1:", metadata2[0].column_name)
print("Result 2 Field 2:", metadata2[1].column_name)


object1 = result1.fetch_one_object()
object2 = result2.fetch_one_object()

print("Result 1 Record 1:", object1.name)
print("Result 2 Record 1:", object2["name"])

record1 = result1.fetch_one()
record2 = result2.fetch_one()

print("Result 1 Record 2:", record1.name)
print("Result 2 Record 2:", record2.name)

record1 = result1.fetch_one()
record2 = result2.fetch_one()

print("Result 1 Record 3:", record1.name)
print("Result 2 Record 3:", record2.name)

record1 = result1.fetch_one()
record2 = result2.fetch_one()

print("Result 1 Record 4:", record1.name)
print("Result 2 Record 4:", record2.name)

#@ Row as object on SQL
print(object1)
print(object2)

#@ Resultset buffering on CRUD
result1 = table.select(['name', 'age']).where('gender = :gender').order_by(['name']).bind('gender','male').execute()
result2 = table.select(['name', 'gender']).where('age < :age').order_by(['name']).bind('age',15).execute()

metadata1 = result1.columns
metadata2 = result2.columns

print("Result 1 Field 1:", metadata1[0].column_name)
print("Result 1 Field 2:", metadata1[1].column_name)

print("Result 2 Field 1:", metadata2[0].column_name)
print("Result 2 Field 2:", metadata2[1].column_name)


object1 = result1.fetch_one_object()
object2 = result2.fetch_one_object()

print("Result 1 Record 1:", object1.name)
print("Result 2 Record 1:", object2["name"])

record1 = result1.fetch_one()
record2 = result2.fetch_one()

print("Result 1 Record 2:", record1.name)
print("Result 2 Record 2:", record2.name)

record1 = result1.fetch_one()
record2 = result2.fetch_one()

print("Result 1 Record 3:", record1.name)
print("Result 2 Record 3:", record2.name)

record1 = result1.fetch_one()
record2 = result2.fetch_one()

print("Result 1 Record 4:", record1.name)
print("Result 2 Record 4:", record2.name)

#@ Row as object on CRUD
print(object1)
print(object2)

#@ Resultset table
print(table.select(["count(*)"]).execute().fetch_one()[0])

#@<> Resultset row members
result = mySession.sql('select name as alias, age, age as length, gender as alias from buffer_table where name = "jack"').execute();
row = result.fetch_one();

validate_members(row, [
  'length',
  'get_field',
  'get_length',
  'help',
  'alias',
  'age'])

#@ Resultset row index access
print("Name with index: %s" % row[0])
print("Age with index: %s" % row[1])
print("Length with index: %s" % row[2])
print("Gender with index: %s" % row[3])

#@ Resultset row get_field access
print("Name with get_field: %s" % row.get_field('alias'))
print("Age with get_field: %s" % row.get_field('age'))
print("Length with get_field: %s" % row.get_field('length'))
print("Unable to get gender from alias: %s" % row.get_field('alias'))

#@ Resultset property access
print("Name with property: %s" % row.alias)
print("Age with property: %s" % row.age)
print("Unable to get length with property: %s" %  row.length)

mySession.close()
