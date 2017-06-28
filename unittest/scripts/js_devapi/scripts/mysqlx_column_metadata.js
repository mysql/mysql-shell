// Assumptions: validate_crud_functions available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
var mysqlx = require('mysqlx');

var mySession = mysqlx.getNodeSession(__uripwd)

ensure_schema_does_not_exist(mySession, 'py_shell_test')

var schema = mySession.createSchema('py_shell_test')
mySession.setCurrentSchema('py_shell_test')

var server_57 = mySession.sql("select @@version like '5.7%'").execute().fetchOne()[0];

function formatCollation(collation) {
  // print collation with normalized output to work in both 5.7 and 8.0
  // default utf8mb4 collation in 5.7 is utf8mb4_general_ci
  // but in 8.0 it's utf8mb4_0900_ai_ci
  if (server_57) {
    return collation+"//utf8mb4_0900_ai_ci";
  } else {
    return "utf8mb4_general_ci//"+collation;
  }
}

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
println('Schema Name:', column.getSchemaName());
println('Table Name:', column.getTableName());
println('Table Label:', column.getTableLabel());
println('Column Name:', column.getColumnName());
println('Column Label:', column.getColumnLabel());
println('Type:', column.getType());
println('Length:', column.getLength());
println('Fractional Digits:', column.getFractionalDigits());
println('Is Number Signed:', column.isNumberSigned());
println('Collation Name:', column.getCollationName());
println('Charset Name:', column.getCharacterSetName());
println('Is Padded:', column.isPadded());
println('Is ZeroFill:', column.isZeroFill());

//@ Metadata on TinyInt Column
column = columns[column_index]
column_index += 1
println('Schema Name:', column.getSchemaName());
println('Table Name:', column.getTableName());
println('Table Label:', column.getTableLabel());
println('Column Name:', column.getColumnName());
println('Column Label:', column.getColumnLabel());
println('Type:', column.getType());
println('Length:', column.getLength());
println('Fractional Digits:', column.getFractionalDigits());
println('Is Number Signed:', column.isNumberSigned());
println('Collation Name:', column.getCollationName());
println('Charset Name:', column.getCharacterSetName());
println('Is Padded:', column.isPadded());
println('Is ZeroFill:', column.isZeroFill());

//@ Metadata on Unsigned TinyInt Column
column = columns[column_index]
column_index += 1
println('Schema Name:', column.getSchemaName());
println('Table Name:', column.getTableName());
println('Table Label:', column.getTableLabel());
println('Column Name:', column.getColumnName());
println('Column Label:', column.getColumnLabel());
println('Type:', column.getType());
println('Length:', column.getLength());
println('Fractional Digits:', column.getFractionalDigits());
println('Is Number Signed:', column.isNumberSigned());
println('Collation Name:', column.getCollationName());
println('Charset Name:', column.getCharacterSetName());
println('Is Padded:', column.isPadded());
println('Is ZeroFill:', column.isZeroFill());

//@ Metadata on SmallInt Column
column = columns[column_index]
column_index += 1
println('Schema Name:', column.getSchemaName());
println('Table Name:', column.getTableName());
println('Table Label:', column.getTableLabel());
println('Column Name:', column.getColumnName());
println('Column Label:', column.getColumnLabel());
println('Type:', column.getType());
println('Length:', column.getLength());
println('Fractional Digits:', column.getFractionalDigits());
println('Is Number Signed:', column.isNumberSigned());
println('Collation Name:', column.getCollationName());
println('Charset Name:', column.getCharacterSetName());
println('Is Padded:', column.isPadded());
println('Is ZeroFill:', column.isZeroFill());

//@ Metadata on Unsigned SmallInt Column
column = columns[column_index]
column_index += 1
println('Schema Name:', column.getSchemaName());
println('Table Name:', column.getTableName());
println('Table Label:', column.getTableLabel());
println('Column Name:', column.getColumnName());
println('Column Label:', column.getColumnLabel());
println('Type:', column.getType());
println('Length:', column.getLength());
println('Fractional Digits:', column.getFractionalDigits());
println('Is Number Signed:', column.isNumberSigned());
println('Collation Name:', column.getCollationName());
println('Charset Name:', column.getCharacterSetName());
println('Is Padded:', column.isPadded());
println('Is ZeroFill:', column.isZeroFill());

//@ Metadata on MediumInt Column
column = columns[column_index]
column_index += 1
println('Schema Name:', column.getSchemaName());
println('Table Name:', column.getTableName());
println('Table Label:', column.getTableLabel());
println('Column Name:', column.getColumnName());
println('Column Label:', column.getColumnLabel());
println('Type:', column.getType());
println('Length:', column.getLength());
println('Fractional Digits:', column.getFractionalDigits());
println('Is Number Signed:', column.isNumberSigned());
println('Collation Name:', column.getCollationName());
println('Charset Name:', column.getCharacterSetName());
println('Is Padded:', column.isPadded());
println('Is ZeroFill:', column.isZeroFill());

//@ Metadata on Unsigned MediumInt Column
column = columns[column_index]
column_index += 1
println('Schema Name:', column.getSchemaName());
println('Table Name:', column.getTableName());
println('Table Label:', column.getTableLabel());
println('Column Name:', column.getColumnName());
println('Column Label:', column.getColumnLabel());
println('Type:', column.getType());
println('Length:', column.getLength());
println('Fractional Digits:', column.getFractionalDigits());
println('Is Number Signed:', column.isNumberSigned());
println('Collation Name:', column.getCollationName());
println('Charset Name:', column.getCharacterSetName());
println('Is Padded:', column.isPadded());
println('Is ZeroFill:', column.isZeroFill());

//@ Metadata on Int Column
column = columns[column_index]
column_index += 1
println('Schema Name:', column.getSchemaName());
println('Table Name:', column.getTableName());
println('Table Label:', column.getTableLabel());
println('Column Name:', column.getColumnName());
println('Column Label:', column.getColumnLabel());
println('Type:', column.getType());
println('Length:', column.getLength());
println('Fractional Digits:', column.getFractionalDigits());
println('Is Number Signed:', column.isNumberSigned());
println('Collation Name:', column.getCollationName());
println('Charset Name:', column.getCharacterSetName());
println('Is Padded:', column.isPadded());
println('Is ZeroFill:', column.isZeroFill());

//@ Metadata on Unsigned Int Column
column = columns[column_index]
column_index += 1
println('Schema Name:', column.getSchemaName());
println('Table Name:', column.getTableName());
println('Table Label:', column.getTableLabel());
println('Column Name:', column.getColumnName());
println('Column Label:', column.getColumnLabel());
println('Type:', column.getType());
println('Length:', column.getLength());
println('Fractional Digits:', column.getFractionalDigits());
println('Is Number Signed:', column.isNumberSigned());
println('Collation Name:', column.getCollationName());
println('Charset Name:', column.getCharacterSetName());
println('Is Padded:', column.isPadded());
println('Is ZeroFill:', column.isZeroFill());

//@ Metadata on Float Column
column = columns[column_index]
column_index += 1
println('Schema Name:', column.getSchemaName());
println('Table Name:', column.getTableName());
println('Table Label:', column.getTableLabel());
println('Column Name:', column.getColumnName());
println('Column Label:', column.getColumnLabel());
println('Type:', column.getType());
println('Length:', column.getLength());
println('Fractional Digits:', column.getFractionalDigits());
println('Is Number Signed:', column.isNumberSigned());
println('Collation Name:', column.getCollationName());
println('Charset Name:', column.getCharacterSetName());
println('Is Padded:', column.isPadded());
println('Is ZeroFill:', column.isZeroFill());

//@ Metadata on Unsigned Float Column
column = columns[column_index]
column_index += 1
println('Schema Name:', column.getSchemaName());
println('Table Name:', column.getTableName());
println('Table Label:', column.getTableLabel());
println('Column Name:', column.getColumnName());
println('Column Label:', column.getColumnLabel());
println('Type:', column.getType());
println('Length:', column.getLength());
println('Fractional Digits:', column.getFractionalDigits());
println('Is Number Signed:', column.isNumberSigned());
println('Collation Name:', column.getCollationName());
println('Charset Name:', column.getCharacterSetName());
println('Is Padded:', column.isPadded());
println('Is ZeroFill:', column.isZeroFill());

//@ Metadata on Float Column with length and fractional digits
column = columns[column_index]
column_index += 1
println('Schema Name:', column.getSchemaName());
println('Table Name:', column.getTableName());
println('Table Label:', column.getTableLabel());
println('Column Name:', column.getColumnName());
println('Column Label:', column.getColumnLabel());
println('Type:', column.getType());
println('Length:', column.getLength());
println('Fractional Digits:', column.getFractionalDigits());
println('Is Number Signed:', column.isNumberSigned());
println('Collation Name:', column.getCollationName());
println('Charset Name:', column.getCharacterSetName());
println('Is Padded:', column.isPadded());
println('Is ZeroFill:', column.isZeroFill());

//@ Metadata on Decimal Column
column = columns[column_index]
column_index += 1
println('Schema Name:', column.getSchemaName());
println('Table Name:', column.getTableName());
println('Table Label:', column.getTableLabel());
println('Column Name:', column.getColumnName());
println('Column Label:', column.getColumnLabel());
println('Type:', column.getType());
println('Length:', column.getLength());
println('Fractional Digits:', column.getFractionalDigits());
println('Is Number Signed:', column.isNumberSigned());
println('Collation Name:', column.getCollationName());
println('Charset Name:', column.getCharacterSetName());
println('Is Padded:', column.isPadded());
println('Is ZeroFill:', column.isZeroFill());

//@ Metadata on Unsigned Decimal Column
column = columns[column_index]
column_index += 1
println('Schema Name:', column.getSchemaName());
println('Table Name:', column.getTableName());
println('Table Label:', column.getTableLabel());
println('Column Name:', column.getColumnName());
println('Column Label:', column.getColumnLabel());
println('Type:', column.getType());
println('Length:', column.getLength());
println('Fractional Digits:', column.getFractionalDigits());
println('Is Number Signed:', column.isNumberSigned());
println('Collation Name:', column.getCollationName());
println('Charset Name:', column.getCharacterSetName());
println('Is Padded:', column.isPadded());
println('Is ZeroFill:', column.isZeroFill());

//@ Metadata on Decimal Column with length and fractional digits
column = columns[column_index]
column_index += 1
println('Schema Name:', column.getSchemaName());
println('Table Name:', column.getTableName());
println('Table Label:', column.getTableLabel());
println('Column Name:', column.getColumnName());
println('Column Label:', column.getColumnLabel());
println('Type:', column.getType());
println('Length:', column.getLength());
println('Fractional Digits:', column.getFractionalDigits());
println('Is Number Signed:', column.isNumberSigned());
println('Collation Name:', column.getCollationName());
println('Charset Name:', column.getCharacterSetName());
println('Is Padded:', column.isPadded());
println('Is ZeroFill:', column.isZeroFill());

//@ Metadata on Double Column
column = columns[column_index]
column_index += 1
println('Schema Name:', column.getSchemaName());
println('Table Name:', column.getTableName());
println('Table Label:', column.getTableLabel());
println('Column Name:', column.getColumnName());
println('Column Label:', column.getColumnLabel());
println('Type:', column.getType());
println('Length:', column.getLength());
println('Fractional Digits:', column.getFractionalDigits());
println('Is Number Signed:', column.isNumberSigned());
println('Collation Name:', column.getCollationName());
println('Charset Name:', column.getCharacterSetName());
println('Is Padded:', column.isPadded());
println('Is ZeroFill:', column.isZeroFill());

//@ Metadata on Unsigned Double Column
column = columns[column_index]
column_index += 1
println('Schema Name:', column.getSchemaName());
println('Table Name:', column.getTableName());
println('Table Label:', column.getTableLabel());
println('Column Name:', column.getColumnName());
println('Column Label:', column.getColumnLabel());
println('Type:', column.getType());
println('Length:', column.getLength());
println('Fractional Digits:', column.getFractionalDigits());
println('Is Number Signed:', column.isNumberSigned());
println('Collation Name:', column.getCollationName());
println('Charset Name:', column.getCharacterSetName());
println('Is Padded:', column.isPadded());
println('Is ZeroFill:', column.isZeroFill());

//@ Metadata on Double Column with length and fractional digits
column = columns[column_index]
column_index += 1
println('Schema Name:', column.getSchemaName());
println('Table Name:', column.getTableName());
println('Table Label:', column.getTableLabel());
println('Column Name:', column.getColumnName());
println('Column Label:', column.getColumnLabel());
println('Type:', column.getType());
println('Length:', column.getLength());
println('Fractional Digits:', column.getFractionalDigits());
println('Is Number Signed:', column.isNumberSigned());
println('Collation Name:', column.getCollationName());
println('Charset Name:', column.getCharacterSetName());
println('Is Padded:', column.isPadded());
println('Is ZeroFill:', column.isZeroFill());

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
println('Schema Name:', column.getSchemaName());
println('Table Name:', column.getTableName());
println('Table Label:', column.getTableLabel());
println('Column Name:', column.getColumnName());
println('Column Label:', column.getColumnLabel());
println('Type:', column.getType());
println('Length:', column.getLength());
println('Fractional Digits:', column.getFractionalDigits());
println('Is Number Signed:', column.isNumberSigned());
println('Collation Name:', column.getCollationName());
println('Charset Name:', column.getCharacterSetName());
println('Is Padded:', column.isPadded());
println('Is ZeroFill:', column.isZeroFill());

//@ Metadata on Char Column
column = columns[column_index]
column_index += 1
println('Schema Name:', column.getSchemaName());
println('Table Name:', column.getTableName());
println('Table Label:', column.getTableLabel());
println('Column Name:', column.getColumnName());
println('Column Label:', column.getColumnLabel());
println('Type:', column.getType());
println('Length:', column.getLength());
println('Fractional Digits:', column.getFractionalDigits());
println('Is Number Signed:', column.isNumberSigned());
println('Collation Name:', formatCollation(column.getCollationName()));
println('Charset Name:', column.getCharacterSetName());
println('Is Padded:', column.isPadded());
println('Is ZeroFill:', column.isZeroFill());

//@ Metadata on Varchar Column
column = columns[column_index]
column_index += 1
println('Schema Name:', column.getSchemaName());
println('Table Name:', column.getTableName());
println('Table Label:', column.getTableLabel());
println('Column Name:', column.getColumnName());
println('Column Label:', column.getColumnLabel());
println('Type:', column.getType());
println('Length:', column.getLength());
println('Fractional Digits:', column.getFractionalDigits());
println('Is Number Signed:', column.isNumberSigned());
println('Collation Name:', formatCollation(column.getCollationName()));
println('Charset Name:', column.getCharacterSetName());
println('Is Padded:', column.isPadded());
println('Is ZeroFill:', column.isZeroFill());

//@ Metadata on Text Column
column = columns[column_index]
column_index += 1
println('Schema Name:', column.getSchemaName());
println('Table Name:', column.getTableName());
println('Table Label:', column.getTableLabel());
println('Column Name:', column.getColumnName());
println('Column Label:', column.getColumnLabel());
println('Type:', column.getType());
println('Length:', column.getLength());
println('Fractional Digits:', column.getFractionalDigits());
println('Is Number Signed:', column.isNumberSigned());
println('Collation Name:', formatCollation(column.getCollationName()));
println('Charset Name:', column.getCharacterSetName());
println('Is Padded:', column.isPadded());
println('Is ZeroFill:', column.isZeroFill());

//@ Metadata on Time Column
column = columns[column_index]
column_index += 1
println('Schema Name:', column.getSchemaName());
println('Table Name:', column.getTableName());
println('Table Label:', column.getTableLabel());
println('Column Name:', column.getColumnName());
println('Column Label:', column.getColumnLabel());
println('Type:', column.getType());
println('Length:', column.getLength());
println('Fractional Digits:', column.getFractionalDigits());
println('Is Number Signed:', column.isNumberSigned());
println('Collation Name:', column.getCollationName());
println('Charset Name:', column.getCharacterSetName());
println('Is Padded:', column.isPadded());
println('Is ZeroFill:', column.isZeroFill());

//@ Metadata on Date Column
column = columns[column_index]
column_index += 1
println('Schema Name:', column.getSchemaName());
println('Table Name:', column.getTableName());
println('Table Label:', column.getTableLabel());
println('Column Name:', column.getColumnName());
println('Column Label:', column.getColumnLabel());
println('Type:', column.getType());
println('Length:', column.getLength());
println('Fractional Digits:', column.getFractionalDigits());
println('Is Number Signed:', column.isNumberSigned());
println('Collation Name:', column.getCollationName());
println('Charset Name:', column.getCharacterSetName());
println('Is Padded:', column.isPadded());
println('Is ZeroFill:', column.isZeroFill());

//@ Metadata on Timestamp Column
column = columns[column_index]
column_index += 1
println('Schema Name:', column.getSchemaName());
println('Table Name:', column.getTableName());
println('Table Label:', column.getTableLabel());
println('Column Name:', column.getColumnName());
println('Column Label:', column.getColumnLabel());
println('Type:', column.getType());
println('Length:', column.getLength());
println('Fractional Digits:', column.getFractionalDigits());
println('Is Number Signed:', column.isNumberSigned());
println('Collation Name:', column.getCollationName());
println('Charset Name:', column.getCharacterSetName());
println('Is Padded:', column.isPadded());
println('Is ZeroFill:', column.isZeroFill());

//@ Metadata on Set Column
column = columns[column_index]
column_index += 1
println('Schema Name:', column.getSchemaName());
println('Table Name:', column.getTableName());
println('Table Label:', column.getTableLabel());
println('Column Name:', column.getColumnName());
println('Column Label:', column.getColumnLabel());
println('Type:', column.getType());
println('Length:', column.getLength());
println('Fractional Digits:', column.getFractionalDigits());
println('Is Number Signed:', column.isNumberSigned());
println('Collation Name:', formatCollation(column.getCollationName()));
println('Charset Name:', column.getCharacterSetName());
println('Is Padded:', column.isPadded());
println('Is ZeroFill:', column.isZeroFill());

//@ Metadata on Enum Column
column = columns[column_index]
column_index += 1
println('Schema Name:', column.getSchemaName());
println('Table Name:', column.getTableName());
println('Table Label:', column.getTableLabel());
println('Column Name:', column.getColumnName());
println('Column Label:', column.getColumnLabel());
println('Type:', column.getType());
println('Length:', column.getLength());
println('Fractional Digits:', column.getFractionalDigits());
println('Is Number Signed:', column.isNumberSigned());
println('Collation Name:', formatCollation(column.getCollationName()));
println('Charset Name:', column.getCharacterSetName());
println('Is Padded:', column.isPadded());
println('Is ZeroFill:', column.isZeroFill());

mySession.close();
