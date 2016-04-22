# Assumptions: ensure_schema_does_not_exist
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
import mysqlx

mySession = mysqlx.getNodeSession(__uripwd)

ensure_schema_does_not_exist(mySession, 'js_shell_test')

schema = mySession.createSchema('js_shell_test')
mySession.setCurrentSchema('js_shell_test')
result = mySession.sql('create table js_shell_test.buffer_table (name varchar(50), age integer, gender varchar(20))').execute()

#@ SqlResult member validation
sqlMembers = dir(result)
print "SqlResult Members:", sqlMembers
validateMember(sqlMembers, 'executionTime')
validateMember(sqlMembers, 'warningCount')
validateMember(sqlMembers, 'warnings')
validateMember(sqlMembers, 'getExecutionTime')
validateMember(sqlMembers, 'getWarningCount')
validateMember(sqlMembers, 'getWarnings')
validateMember(sqlMembers, 'columnCount')
validateMember(sqlMembers, 'columnNames')
validateMember(sqlMembers, 'columns')
validateMember(sqlMembers, 'getColumnCount')
validateMember(sqlMembers, 'getColumnNames')
validateMember(sqlMembers, 'getColumns')
validateMember(sqlMembers, 'fetchOne')
validateMember(sqlMembers, 'fetchAll')
validateMember(sqlMembers, 'hasData')
validateMember(sqlMembers, 'nextDataSet')
validateMember(sqlMembers, 'affectedRowCount')
validateMember(sqlMembers, 'autoIncrementValue')
validateMember(sqlMembers, 'getAffectedRowCount')
validateMember(sqlMembers, 'getAutoIncrementValue')


#@ Result member validation
table = schema.getTable('buffer_table')
result = table.insert({'name': 'jack', 'age': 17, 'gender': 'male'}).execute()
result = table.insert({'name': 'adam', 'age': 15, 'gender': 'male'}).execute()
result = table.insert({'name': 'brian', 'age': 14, 'gender': 'male'}).execute()
result = table.insert({'name': 'alma', 'age': 13, 'gender': 'female'}).execute()
result = table.insert({'name': 'carol', 'age': 14, 'gender': 'female'}).execute()
result = table.insert({'name': 'donna', 'age': 16, 'gender': 'female'}).execute()
result = table.insert({'name': 'angel', 'age': 14, 'gender': 'male'}).execute()

table = schema.getTable('buffer_table')
collection = schema.createCollection('buffer_collection')

resultMembers = dir(result)
print "Result Members:", resultMembers
validateMember(resultMembers, 'executionTime')
validateMember(resultMembers, 'warningCount')
validateMember(resultMembers, 'warnings')
validateMember(resultMembers, 'getExecutionTime')
validateMember(resultMembers, 'getWarningCount')
validateMember(resultMembers, 'getWarnings')
validateMember(resultMembers, 'affectedItemCount')
validateMember(resultMembers, 'autoIncrementValue')
validateMember(resultMembers, 'lastDocumentId')
validateMember(resultMembers, 'getAffectedItemCount')
validateMember(resultMembers, 'getAutoIncrementValue')
validateMember(resultMembers, 'getLastDocumentId')

#@ RowResult member validation
result = table.select().execute()
rowResultMembers = dir(result)
print "RowResult Members:", rowResultMembers
validateMember(rowResultMembers, 'executionTime')
validateMember(rowResultMembers, 'warningCount')
validateMember(rowResultMembers, 'warnings')
validateMember(rowResultMembers, 'getExecutionTime')
validateMember(rowResultMembers, 'getWarningCount')
validateMember(rowResultMembers, 'getWarnings')
validateMember(rowResultMembers, 'columnCount')
validateMember(rowResultMembers, 'columnNames')
validateMember(rowResultMembers, 'columns')
validateMember(rowResultMembers, 'getColumnCount')
validateMember(rowResultMembers, 'getColumnNames')
validateMember(rowResultMembers, 'getColumns')
validateMember(rowResultMembers, 'fetchOne')
validateMember(rowResultMembers, 'fetchAll')

#@ DocResult member validation
result = collection.find().execute()
docResultMembers = dir(result)
print "DocRowResult Members:", docResultMembers
validateMember(docResultMembers, 'executionTime')
validateMember(docResultMembers, 'warningCount')
validateMember(docResultMembers, 'warnings')
validateMember(docResultMembers, 'getExecutionTime')
validateMember(docResultMembers, 'getWarningCount')
validateMember(docResultMembers, 'getWarnings')
validateMember(docResultMembers, 'fetchOne')
validateMember(docResultMembers, 'fetchAll')

#@ Resultset hasData() False
result = mySession.sql('use js_shell_test').execute()
print 'hasData():', result.hasData()

#@ Resultset hasData() True
result = mySession.sql('select * from buffer_table').execute()
print 'hasData():', result.hasData()


#@ Resultset getColumns()
metadata = result.getColumns()

print 'Field Number:', len(metadata)
print 'First Field:', metadata[0].columnName
print 'Second Field:', metadata[1].columnName
print 'Third Field:', metadata[2].columnName


#@ Resultset columns
metadata = result.columns

print 'Field Number:', len(metadata)
print 'First Field:', metadata[0].columnName
print 'Second Field:', metadata[1].columnName
print 'Third Field:', metadata[2].columnName


#@ Resultset buffering on SQL

result1 = mySession.sql('select name, age from js_shell_test.buffer_table where gender = "male" order by name').execute()
result2 = mySession.sql('select name, gender from js_shell_test.buffer_table where age < 15 order by name').execute()

metadata1 = result1.columns
metadata2 = result2.columns

print "Result 1 Field 1:", metadata1[0].columnName
print "Result 1 Field 2:", metadata1[1].columnName

print "Result 2 Field 1:", metadata2[0].columnName
print "Result 2 Field 2:", metadata2[1].columnName


record1 = result1.fetchOne()
record2 = result2.fetchOne()

print "Result 1 Record 1:", record1.name
print "Result 2 Record 1:", record2.name

record1 = result1.fetchOne()
record2 = result2.fetchOne()

print "Result 1 Record 2:", record1.name
print "Result 2 Record 2:", record2.name

record1 = result1.fetchOne()
record2 = result2.fetchOne()

print "Result 1 Record 3:", record1.name
print "Result 2 Record 3:", record2.name

record1 = result1.fetchOne()
record2 = result2.fetchOne()

print "Result 1 Record 4:", record1.name
print "Result 2 Record 4:", record2.name


#@ Resultset buffering on CRUD

result1 = table.select(['name', 'age']).where('gender = :gender').orderBy(['name']).bind('gender','male').execute()
result2 = table.select(['name', 'gender']).where('age < :age').orderBy(['name']).bind('age',15).execute()

metadata1 = result1.columns
metadata2 = result2.columns

print "Result 1 Field 1:", metadata1[0].columnName
print "Result 1 Field 2:", metadata1[1].columnName

print "Result 2 Field 1:", metadata2[0].columnName
print "Result 2 Field 2:", metadata2[1].columnName


record1 = result1.fetchOne()
record2 = result2.fetchOne()

print "Result 1 Record 1:", record1.name
print "Result 2 Record 1:", record2.name

record1 = result1.fetchOne()
record2 = result2.fetchOne()

print "Result 1 Record 2:", record1.name
print "Result 2 Record 2:", record2.name

record1 = result1.fetchOne()
record2 = result2.fetchOne()

print "Result 1 Record 3:", record1.name
print "Result 2 Record 3:", record2.name

record1 = result1.fetchOne()
record2 = result2.fetchOne()

print "Result 1 Record 4:", record1.name
print "Result 2 Record 4:", record2.name

#@ Resultset table
print table.select(["count(*)"]).execute().fetchOne()[0]

mySession.close()
