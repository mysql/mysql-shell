# Assumptions: ensure_schema_does_not_exist
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<mysql_port>
from mysqlsh import mysql

mySession = mysql.get_classic_session(__uripwd)

#@ Result member validation
mySession.run_sql('drop schema if exists js_shell_test')
mySession.run_sql('create schema js_shell_test')
mySession.run_sql('use js_shell_test')
result = mySession.run_sql('create table js_shell_test.buffer_table (name varchar(50) primary key, age integer, gender varchar(20))')

members = dir(result)
print "SqlResult Members:", members
validateMember(members, 'affected_items_count')
validateMember(members, 'execution_time')
validateMember(members, 'warning_count')
validateMember(members, 'warnings')
validateMember(members, 'warnings_count')
validateMember(members, 'get_affected_items_count')
validateMember(members, 'get_execution_time')
validateMember(members, 'get_warning_count')
validateMember(members, 'get_warnings')
validateMember(members, 'get_warnings_count')
validateMember(members, 'column_count')
validateMember(members, 'column_names')
validateMember(members, 'columns')
validateMember(members, 'info')
validateMember(members, 'get_column_count')
validateMember(members, 'get_column_names')
validateMember(members, 'get_columns')
validateMember(members, 'get_info')
validateMember(members, 'fetch_one')
validateMember(members, 'fetch_one_object')
validateMember(members, 'fetch_all')
validateMember(members, 'has_data')
validateMember(members, 'next_data_set')
validateMember(members, 'next_result')
validateMember(members, 'affected_row_count')
validateMember(members, 'auto_increment_value')
validateMember(members, 'get_affected_row_count')
validateMember(members, 'get_auto_increment_value')

result = mySession.run_sql('insert into buffer_table values("jack", 17, "male")')
result = mySession.run_sql('insert into buffer_table values("adam", 15, "male")')
result = mySession.run_sql('insert into buffer_table values("brian", 14, "male")')
result = mySession.run_sql('insert into buffer_table values("alma", 13, "female")')
result = mySession.run_sql('insert into buffer_table values("carol", 14, "female")')
result = mySession.run_sql('insert into buffer_table values("donna", 16, "female")')
result = mySession.run_sql('insert into buffer_table values("angel", 14, "male")')


#@ Resultset has_data() False
result = mySession.run_sql('use js_shell_test')
print 'has_data():', result.has_data()

#@ Resultset has_data() True
result = mySession.run_sql('select * from buffer_table')
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

#@ Resultset row members
result = mySession.run_sql('select name as alias, age, age as length, gender as alias from buffer_table where name = "jack"');
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

#@ Resultset row as object
result = mySession.run_sql('select name as alias, age, age as length, gender as alias from buffer_table where name = "jack"');
object = result.fetch_one_object();
print "Name with property: %s" % object.alias
print "Age with property: %s" % object["age"]
print object

mySession.close()
