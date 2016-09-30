// Assumptions: validate_crud_functions available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
var mysqlx = require('mysqlx');

var mySession = mysqlx.getNodeSession(__uripwd)

ensure_schema_does_not_exist(mySession, 'py_shell_test')

var schema = mySession.createSchema('py_shell_test')
mySession.setCurrentSchema('py_shell_test')

// Metadata Validation On Numeric Types
var result = mySession.sql('create table table1 (one bit, two tinyint primary key, utwo tinyint unsigned, three smallint, uthree smallint unsigned, four mediumint, ufour mediumint unsigned, five int, ufive int unsigned, six float, usix float unsigned, csix float(5,3), seven decimal, useven decimal unsigned, cseven decimal(4,2), eight double, ueight double unsigned, ceight double(8,3))').execute();
var table = schema.getTable('table1')

result = table.insert().values(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18).execute();

result = table.select().execute();
var columns = result.getColumns();
var column_index = 0

//@ Metadata on Bit Column
var column = columns[column_index]
column_index += 1
print ('Schema Name:', column.getSchemaName());
print ('Table Name:', column.getTableName());
print ('Table Label:', column.getTableLabel());
print ('Column Name:', column.getColumnName());
print ('Column Label:', column.getColumnLabel());
print ('Type:', column.getType());
print ('Length:', column.getLength());
print ('Fractional Digits:', column.getFractionalDigits());
print ('Is Number Signed:', column.isNumberSigned());
print ('Collation Name:', column.getCollationName());
print ('Charset Name:', column.getCharacterSetName());
print ('Is Padded:', column.isPadded());

//@ Metadata on TinyInt Column
column = columns[column_index]
column_index += 1
print ('Schema Name:', column.getSchemaName());
print ('Table Name:', column.getTableName());
print ('Table Label:', column.getTableLabel());
print ('Column Name:', column.getColumnName());
print ('Column Label:', column.getColumnLabel());
print ('Type:', column.getType());
print ('Length:', column.getLength());
print ('Fractional Digits:', column.getFractionalDigits());
print ('Is Number Signed:', column.isNumberSigned());
print ('Collation Name:', column.getCollationName());
print ('Charset Name:', column.getCharacterSetName());
print ('Is Padded:', column.isPadded());

//@ Metadata on Unsigned TinyInt Column
column = columns[column_index]
column_index += 1
print ('Schema Name:', column.getSchemaName());
print ('Table Name:', column.getTableName());
print ('Table Label:', column.getTableLabel());
print ('Column Name:', column.getColumnName());
print ('Column Label:', column.getColumnLabel());
print ('Type:', column.getType());
print ('Length:', column.getLength());
print ('Fractional Digits:', column.getFractionalDigits());
print ('Is Number Signed:', column.isNumberSigned());
print ('Collation Name:', column.getCollationName());
print ('Charset Name:', column.getCharacterSetName());
print ('Is Padded:', column.isPadded());

//@ Metadata on SmallInt Column
column = columns[column_index]
column_index += 1
print ('Schema Name:', column.getSchemaName());
print ('Table Name:', column.getTableName());
print ('Table Label:', column.getTableLabel());
print ('Column Name:', column.getColumnName());
print ('Column Label:', column.getColumnLabel());
print ('Type:', column.getType());
print ('Length:', column.getLength());
print ('Fractional Digits:', column.getFractionalDigits());
print ('Is Number Signed:', column.isNumberSigned());
print ('Collation Name:', column.getCollationName());
print ('Charset Name:', column.getCharacterSetName());
print ('Is Padded:', column.isPadded());

//@ Metadata on Unsigned SmallInt Column
column = columns[column_index]
column_index += 1
print ('Schema Name:', column.getSchemaName());
print ('Table Name:', column.getTableName());
print ('Table Label:', column.getTableLabel());
print ('Column Name:', column.getColumnName());
print ('Column Label:', column.getColumnLabel());
print ('Type:', column.getType());
print ('Length:', column.getLength());
print ('Fractional Digits:', column.getFractionalDigits());
print ('Is Number Signed:', column.isNumberSigned());
print ('Collation Name:', column.getCollationName());
print ('Charset Name:', column.getCharacterSetName());
print ('Is Padded:', column.isPadded());

//@ Metadata on MediumInt Column
column = columns[column_index]
column_index += 1
print ('Schema Name:', column.getSchemaName());
print ('Table Name:', column.getTableName());
print ('Table Label:', column.getTableLabel());
print ('Column Name:', column.getColumnName());
print ('Column Label:', column.getColumnLabel());
print ('Type:', column.getType());
print ('Length:', column.getLength());
print ('Fractional Digits:', column.getFractionalDigits());
print ('Is Number Signed:', column.isNumberSigned());
print ('Collation Name:', column.getCollationName());
print ('Charset Name:', column.getCharacterSetName());
print ('Is Padded:', column.isPadded());

//@ Metadata on Unsigned MediumInt Column
column = columns[column_index]
column_index += 1
print ('Schema Name:', column.getSchemaName());
print ('Table Name:', column.getTableName());
print ('Table Label:', column.getTableLabel());
print ('Column Name:', column.getColumnName());
print ('Column Label:', column.getColumnLabel());
print ('Type:', column.getType());
print ('Length:', column.getLength());
print ('Fractional Digits:', column.getFractionalDigits());
print ('Is Number Signed:', column.isNumberSigned());
print ('Collation Name:', column.getCollationName());
print ('Charset Name:', column.getCharacterSetName());
print ('Is Padded:', column.isPadded());

//@ Metadata on Int Column
column = columns[column_index]
column_index += 1
print ('Schema Name:', column.getSchemaName());
print ('Table Name:', column.getTableName());
print ('Table Label:', column.getTableLabel());
print ('Column Name:', column.getColumnName());
print ('Column Label:', column.getColumnLabel());
print ('Type:', column.getType());
print ('Length:', column.getLength());
print ('Fractional Digits:', column.getFractionalDigits());
print ('Is Number Signed:', column.isNumberSigned());
print ('Collation Name:', column.getCollationName());
print ('Charset Name:', column.getCharacterSetName());
print ('Is Padded:', column.isPadded());

//@ Metadata on Unsigned Int Column
column = columns[column_index]
column_index += 1
print ('Schema Name:', column.getSchemaName());
print ('Table Name:', column.getTableName());
print ('Table Label:', column.getTableLabel());
print ('Column Name:', column.getColumnName());
print ('Column Label:', column.getColumnLabel());
print ('Type:', column.getType());
print ('Length:', column.getLength());
print ('Fractional Digits:', column.getFractionalDigits());
print ('Is Number Signed:', column.isNumberSigned());
print ('Collation Name:', column.getCollationName());
print ('Charset Name:', column.getCharacterSetName());
print ('Is Padded:', column.isPadded());

//@ Metadata on Float Column
column = columns[column_index]
column_index += 1
print ('Schema Name:', column.getSchemaName());
print ('Table Name:', column.getTableName());
print ('Table Label:', column.getTableLabel());
print ('Column Name:', column.getColumnName());
print ('Column Label:', column.getColumnLabel());
print ('Type:', column.getType());
print ('Length:', column.getLength());
print ('Fractional Digits:', column.getFractionalDigits());
print ('Is Number Signed:', column.isNumberSigned());
print ('Collation Name:', column.getCollationName());
print ('Charset Name:', column.getCharacterSetName());
print ('Is Padded:', column.isPadded());

//@ Metadata on Unsigned Float Column
column = columns[column_index]
column_index += 1
print ('Schema Name:', column.getSchemaName());
print ('Table Name:', column.getTableName());
print ('Table Label:', column.getTableLabel());
print ('Column Name:', column.getColumnName());
print ('Column Label:', column.getColumnLabel());
print ('Type:', column.getType());
print ('Length:', column.getLength());
print ('Fractional Digits:', column.getFractionalDigits());
print ('Is Number Signed:', column.isNumberSigned());
print ('Collation Name:', column.getCollationName());
print ('Charset Name:', column.getCharacterSetName());
print ('Is Padded:', column.isPadded());

//@ Metadata on Float Column with length and fractional digits
column = columns[column_index]
column_index += 1
print ('Schema Name:', column.getSchemaName());
print ('Table Name:', column.getTableName());
print ('Table Label:', column.getTableLabel());
print ('Column Name:', column.getColumnName());
print ('Column Label:', column.getColumnLabel());
print ('Type:', column.getType());
print ('Length:', column.getLength());
print ('Fractional Digits:', column.getFractionalDigits());
print ('Is Number Signed:', column.isNumberSigned());
print ('Collation Name:', column.getCollationName());
print ('Charset Name:', column.getCharacterSetName());
print ('Is Padded:', column.isPadded());

//@ Metadata on Decimal Column
column = columns[column_index]
column_index += 1
print ('Schema Name:', column.getSchemaName());
print ('Table Name:', column.getTableName());
print ('Table Label:', column.getTableLabel());
print ('Column Name:', column.getColumnName());
print ('Column Label:', column.getColumnLabel());
print ('Type:', column.getType());
print ('Length:', column.getLength());
print ('Fractional Digits:', column.getFractionalDigits());
print ('Is Number Signed:', column.isNumberSigned());
print ('Collation Name:', column.getCollationName());
print ('Charset Name:', column.getCharacterSetName());
print ('Is Padded:', column.isPadded());

//@ Metadata on Unsigned Decimal Column
column = columns[column_index]
column_index += 1
print ('Schema Name:', column.getSchemaName());
print ('Table Name:', column.getTableName());
print ('Table Label:', column.getTableLabel());
print ('Column Name:', column.getColumnName());
print ('Column Label:', column.getColumnLabel());
print ('Type:', column.getType());
print ('Length:', column.getLength());
print ('Fractional Digits:', column.getFractionalDigits());
print ('Is Number Signed:', column.isNumberSigned());
print ('Collation Name:', column.getCollationName());
print ('Charset Name:', column.getCharacterSetName());
print ('Is Padded:', column.isPadded());

//@ Metadata on Decimal Column with length and fractional digits
column = columns[column_index]
column_index += 1
print ('Schema Name:', column.getSchemaName());
print ('Table Name:', column.getTableName());
print ('Table Label:', column.getTableLabel());
print ('Column Name:', column.getColumnName());
print ('Column Label:', column.getColumnLabel());
print ('Type:', column.getType());
print ('Length:', column.getLength());
print ('Fractional Digits:', column.getFractionalDigits());
print ('Is Number Signed:', column.isNumberSigned());
print ('Collation Name:', column.getCollationName());
print ('Charset Name:', column.getCharacterSetName());
print ('Is Padded:', column.isPadded());

//@ Metadata on Double Column
column = columns[column_index]
column_index += 1
print ('Schema Name:', column.getSchemaName());
print ('Table Name:', column.getTableName());
print ('Table Label:', column.getTableLabel());
print ('Column Name:', column.getColumnName());
print ('Column Label:', column.getColumnLabel());
print ('Type:', column.getType());
print ('Length:', column.getLength());
print ('Fractional Digits:', column.getFractionalDigits());
print ('Is Number Signed:', column.isNumberSigned());
print ('Collation Name:', column.getCollationName());
print ('Charset Name:', column.getCharacterSetName());
print ('Is Padded:', column.isPadded());

//@ Metadata on Unsigned Double Column
column = columns[column_index]
column_index += 1
print ('Schema Name:', column.getSchemaName());
print ('Table Name:', column.getTableName());
print ('Table Label:', column.getTableLabel());
print ('Column Name:', column.getColumnName());
print ('Column Label:', column.getColumnLabel());
print ('Type:', column.getType());
print ('Length:', column.getLength());
print ('Fractional Digits:', column.getFractionalDigits());
print ('Is Number Signed:', column.isNumberSigned());
print ('Collation Name:', column.getCollationName());
print ('Charset Name:', column.getCharacterSetName());
print ('Is Padded:', column.isPadded());

//@ Metadata on Double Column with length and fractional digits
column = columns[column_index]
column_index += 1
print ('Schema Name:', column.getSchemaName());
print ('Table Name:', column.getTableName());
print ('Table Label:', column.getTableLabel());
print ('Column Name:', column.getColumnName());
print ('Column Label:', column.getColumnLabel());
print ('Type:', column.getType());
print ('Length:', column.getLength());
print ('Fractional Digits:', column.getFractionalDigits());
print ('Is Number Signed:', column.isNumberSigned());
print ('Collation Name:', column.getCollationName());
print ('Charset Name:', column.getCharacterSetName());
print ('Is Padded:', column.isPadded());

// Metadata Validation On Other Types
result = mySession.sql("create table table2 (one json, two char(5) primary key, three varchar(20), four text, five time, six date, seven timestamp, eight set('1','2','3'), nine enum ('a','b','c'))").execute();
table = schema.getTable('table2')

result = table.insert().values('{"name":"John", "Age":23}', 'test', 'sample', 'a_text', mysqlx.expr('NOW()'), mysqlx.expr('CURDATE()'), mysqlx.expr('NOW()'), '2','c').execute();

result = table.select().execute();
columns = result.getColumns();
column_index = 0

//@ Metadata on Json Column
column = columns[column_index]
column_index += 1
print ('Schema Name:', column.getSchemaName());
print ('Table Name:', column.getTableName());
print ('Table Label:', column.getTableLabel());
print ('Column Name:', column.getColumnName());
print ('Column Label:', column.getColumnLabel());
print ('Type:', column.getType());
print ('Length:', column.getLength());
print ('Fractional Digits:', column.getFractionalDigits());
print ('Is Number Signed:', column.isNumberSigned());
print ('Collation Name:', column.getCollationName());
print ('Charset Name:', column.getCharacterSetName());
print ('Is Padded:', column.isPadded());

//@ Metadata on Char Column
column = columns[column_index]
column_index += 1
print ('Schema Name:', column.getSchemaName());
print ('Table Name:', column.getTableName());
print ('Table Label:', column.getTableLabel());
print ('Column Name:', column.getColumnName());
print ('Column Label:', column.getColumnLabel());
print ('Type:', column.getType());
print ('Length:', column.getLength());
print ('Fractional Digits:', column.getFractionalDigits());
print ('Is Number Signed:', column.isNumberSigned());
print ('Collation Name:', column.getCollationName());
print ('Charset Name:', column.getCharacterSetName());
print ('Is Padded:', column.isPadded());

//@ Metadata on Varchar Column
column = columns[column_index]
column_index += 1
print ('Schema Name:', column.getSchemaName());
print ('Table Name:', column.getTableName());
print ('Table Label:', column.getTableLabel());
print ('Column Name:', column.getColumnName());
print ('Column Label:', column.getColumnLabel());
print ('Type:', column.getType());
print ('Length:', column.getLength());
print ('Fractional Digits:', column.getFractionalDigits());
print ('Is Number Signed:', column.isNumberSigned());
print ('Collation Name:', column.getCollationName());
print ('Charset Name:', column.getCharacterSetName());
print ('Is Padded:', column.isPadded());

//@ Metadata on Text Column
column = columns[column_index]
column_index += 1
print ('Schema Name:', column.getSchemaName());
print ('Table Name:', column.getTableName());
print ('Table Label:', column.getTableLabel());
print ('Column Name:', column.getColumnName());
print ('Column Label:', column.getColumnLabel());
print ('Type:', column.getType());
print ('Length:', column.getLength());
print ('Fractional Digits:', column.getFractionalDigits());
print ('Is Number Signed:', column.isNumberSigned());
print ('Collation Name:', column.getCollationName());
print ('Charset Name:', column.getCharacterSetName());
print ('Is Padded:', column.isPadded());

//@ Metadata on Time Column
column = columns[column_index]
column_index += 1
print ('Schema Name:', column.getSchemaName());
print ('Table Name:', column.getTableName());
print ('Table Label:', column.getTableLabel());
print ('Column Name:', column.getColumnName());
print ('Column Label:', column.getColumnLabel());
print ('Type:', column.getType());
print ('Length:', column.getLength());
print ('Fractional Digits:', column.getFractionalDigits());
print ('Is Number Signed:', column.isNumberSigned());
print ('Collation Name:', column.getCollationName());
print ('Charset Name:', column.getCharacterSetName());
print ('Is Padded:', column.isPadded());

//@ Metadata on Date Column
column = columns[column_index]
column_index += 1
print ('Schema Name:', column.getSchemaName());
print ('Table Name:', column.getTableName());
print ('Table Label:', column.getTableLabel());
print ('Column Name:', column.getColumnName());
print ('Column Label:', column.getColumnLabel());
print ('Type:', column.getType());
print ('Length:', column.getLength());
print ('Fractional Digits:', column.getFractionalDigits());
print ('Is Number Signed:', column.isNumberSigned());
print ('Collation Name:', column.getCollationName());
print ('Charset Name:', column.getCharacterSetName());
print ('Is Padded:', column.isPadded());

//@ Metadata on Timestamp Column
column = columns[column_index]
column_index += 1
print ('Schema Name:', column.getSchemaName());
print ('Table Name:', column.getTableName());
print ('Table Label:', column.getTableLabel());
print ('Column Name:', column.getColumnName());
print ('Column Label:', column.getColumnLabel());
print ('Type:', column.getType());
print ('Length:', column.getLength());
print ('Fractional Digits:', column.getFractionalDigits());
print ('Is Number Signed:', column.isNumberSigned());
print ('Collation Name:', column.getCollationName());
print ('Charset Name:', column.getCharacterSetName());
print ('Is Padded:', column.isPadded());

//@ Metadata on Set Column
column = columns[column_index]
column_index += 1
print ('Schema Name:', column.getSchemaName());
print ('Table Name:', column.getTableName());
print ('Table Label:', column.getTableLabel());
print ('Column Name:', column.getColumnName());
print ('Column Label:', column.getColumnLabel());
print ('Type:', column.getType());
print ('Length:', column.getLength());
print ('Fractional Digits:', column.getFractionalDigits());
print ('Is Number Signed:', column.isNumberSigned());
print ('Collation Name:', column.getCollationName());
print ('Charset Name:', column.getCharacterSetName());
print ('Is Padded:', column.isPadded());

//@ Metadata on Enum Column
column = columns[column_index]
column_index += 1
print ('Schema Name:', column.getSchemaName());
print ('Table Name:', column.getTableName());
print ('Table Label:', column.getTableLabel());
print ('Column Name:', column.getColumnName());
print ('Column Label:', column.getColumnLabel());
print ('Type:', column.getType());
print ('Length:', column.getLength());
print ('Fractional Digits:', column.getFractionalDigits());
print ('Is Number Signed:', column.isNumberSigned());
print ('Collation Name:', column.getCollationName());
print ('Charset Name:', column.getCharacterSetName());
print ('Is Padded:', column.isPadded());
