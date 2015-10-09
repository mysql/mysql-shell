# Assumptions: ensure_schema_does_not_exist
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<mysql_port>
import mysql

mySession = mysql.getClassicSession(__uripwd)

ensure_schema_does_not_exist(mySession, 'js_shell_test')

schema = mySession.createSchema('js_shell_test')
mySession.setCurrentSchema('js_shell_test')
mySession.runSql('create table js_shell_test.buffer_table (name varchar(50), age integer, gender varchar(20))')
table = schema.getTable('buffer_table')

result = mySession.runSql('insert into buffer_table values("jack", 17, "male")')
result = mySession.runSql('insert into buffer_table values("adam", 15, "male")')
result = mySession.runSql('insert into buffer_table values("brian", 14, "male")')
result = mySession.runSql('insert into buffer_table values("alma", 13, "female")')
result = mySession.runSql('insert into buffer_table values("carol", 14, "female")')
result = mySession.runSql('insert into buffer_table values("donna", 16, "female")')
result = mySession.runSql('insert into buffer_table values("angel", 14, "male")')


table = schema.getTable('buffer_table')

#@ Resultset hasData() False
result = mySession.runSql('use js_shell_test')
print 'hasData():', result.hasData()

#@ Resultset hasData() True
result = mySession.runSql('select * from buffer_table')
print 'hasData():', result.hasData()


#@ Resultset getColumns()
metadata = result.getColumns()

print 'Field Number:', len(metadata)
print 'First Field:', metadata[0].name
print 'Second Field:', metadata[1].name
print 'Third Field:', metadata[2].name


#@ Resultset columns
metadata = result.columns

print 'Field Number:', len(metadata)
print 'First Field:', metadata[0].name
print 'Second Field:', metadata[1].name
print 'Third Field:', metadata[2].name

mySession.close()
