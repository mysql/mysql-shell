# Assumptions: ensure_schema_does_not_exist is available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
from __future__ import print_function
from mysqlsh import mysqlx

mySession = mysqlx.get_session(__uripwd)

ensure_schema_does_not_exist(mySession, 'py_shell_test')

schema = mySession.create_schema('py_shell_test')
mySession.set_current_schema('py_shell_test')

result = mySession.sql('create table table1 (name varchar(50))').execute()
table = mySession.get_schema('py_shell_test').get_table('table1')

#@ Testing table name retrieving
print('get_name(): ' + table.get_name())
print('name: ' + table.name)


#@ Testing session retrieving
print('get_session():', table.get_session())
print('session:', table.session)

#@ Testing table schema retrieving
print('get_schema():', table.get_schema())
print('schema:', table.schema)


#@ Testing existence
print('Valid:', table.exists_in_database())
mySession.sql('drop table table1').execute()
print('Invalid:', table.exists_in_database())

#@ Testing view check
print('Is View:', table.is_view())

#@<> WL12412: Initialize Count Tests
result = mySession.sql('create table table_count (name varchar(50))').execute();
table = schema.get_table('table_count');

#@ WL12412-TS1_1: Count takes no arguments
table.count(1);

#@ WL12412-TS1_3: Count returns correct number of records
count = table.count();
print("Initial Row Count: %d" % count)

table.insert().values("First")
table.insert().values("Second")
table.insert().values("Third")

count = table.count()
print("Final Row Count: %d" % count)

#@ WL12412-TS2_2: Count throws error on unexisting table
mySession.sql('drop table py_shell_test.table_count');
count = table.count()

# Closes the session
mySession.drop_schema('py_shell_test')
mySession.close()
