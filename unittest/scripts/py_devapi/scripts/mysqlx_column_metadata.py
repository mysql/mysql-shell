# Assumptions: validate_crud_functions available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
from __future__ import print_function
from mysqlsh import mysqlx

mySession = mysqlx.get_session(__uripwd)

ensure_schema_does_not_exist(mySession, 'py_shell_test')

schema = mySession.create_schema('py_shell_test')
mySession.set_current_schema('py_shell_test')

server_57 = mySession.sql("select @@version like '5.7%'").execute().fetch_one()[0]

def formatCollation(collation):
  if collation == "binary":
    return collation
  # print collation with normalized output to work in both 5.7 and 8.0
  # default utf8mb4 collation in 5.7 is utf8mb4_general_ci
  # but in 8.0 it's utf8mb4_0900_ai_ci
  if (server_57):
    return collation+"//utf8mb4_0900_ai_ci"
  else:
    return "utf8mb4_general_ci//"+collation

# Metadata Validation On Numeric Types
result = mySession.sql('create table table1 (one bit, two tinyint primary key, utwo tinyint unsigned, three smallint, uthree smallint unsigned, four mediumint, ufour mediumint unsigned, five int, ufive int unsigned, six float, usix float unsigned, csix float(5,3), seven decimal, useven decimal unsigned, cseven decimal(4,2), eight double, ueight double unsigned, ceight double(8,3))').execute()
table = schema.get_table('table1')

result = table.insert().values(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18).execute()

result = table.select().execute()
row = result.fetch_one()
columns = result.get_columns()
column_index = 0

#@ Metadata on Bit Column
column = columns[column_index]
field = row[column_index]
column_index += 1
print('Field Type: ', type(field))
print('Schema Name:', column.get_schema_name())
print('Table Name:', column.get_table_name())
print('Table Label:', column.get_table_label())
print('Column Name:', column.get_column_name())
print('Column Label:', column.get_column_label())
print('Type:', column.get_type())
print('Length:', column.get_length())
print('Fractional Digits:', column.get_fractional_digits())
print('Is Number Signed:', column.is_number_signed())
print('Collation Name:', column.get_collation_name())
print('Charset Name:', column.get_character_set_name())
print('Is ZeroFill:', column.is_zero_fill())

#@ Metadata on TinyInt Column
column = columns[column_index]
field = row[column_index]
column_index += 1
print('Field Type: ', type(field))
print('Schema Name:', column.get_schema_name())
print('Table Name:', column.get_table_name())
print('Table Label:', column.get_table_label())
print('Column Name:', column.get_column_name())
print('Column Label:', column.get_column_label())
print('Type:', column.get_type())
print('Length:', column.get_length())
print('Fractional Digits:', column.get_fractional_digits())
print('Is Number Signed:', column.is_number_signed())
print('Collation Name:', column.get_collation_name())
print('Charset Name:', column.get_character_set_name())
print('Is ZeroFill:', column.is_zero_fill())

#@ Metadata on Unsigned TinyInt Column
column = columns[column_index]
field = row[column_index]
column_index += 1
print('Field Type: ', type(field))
print('Schema Name:', column.get_schema_name())
print('Table Name:', column.get_table_name())
print('Table Label:', column.get_table_label())
print('Column Name:', column.get_column_name())
print('Column Label:', column.get_column_label())
print('Type:', column.get_type())
print('Length:', column.get_length())
print('Fractional Digits:', column.get_fractional_digits())
print('Is Number Signed:', column.is_number_signed())
print('Collation Name:', column.get_collation_name())
print('Charset Name:', column.get_character_set_name())
print('Is ZeroFill:', column.is_zero_fill())

#@ Metadata on SmallInt Column
column = columns[column_index]
field = row[column_index]
column_index += 1
print('Field Type: ', type(field))
print('Schema Name:', column.get_schema_name())
print('Table Name:', column.get_table_name())
print('Table Label:', column.get_table_label())
print('Column Name:', column.get_column_name())
print('Column Label:', column.get_column_label())
print('Type:', column.get_type())
print('Length:', column.get_length())
print('Fractional Digits:', column.get_fractional_digits())
print('Is Number Signed:', column.is_number_signed())
print('Collation Name:', column.get_collation_name())
print('Charset Name:', column.get_character_set_name())
print('Is ZeroFill:', column.is_zero_fill())

#@ Metadata on Unsigned SmallInt Column
column = columns[column_index]
field = row[column_index]
column_index += 1
print('Field Type: ', type(field))
print('Schema Name:', column.get_schema_name())
print('Table Name:', column.get_table_name())
print('Table Label:', column.get_table_label())
print('Column Name:', column.get_column_name())
print('Column Label:', column.get_column_label())
print('Type:', column.get_type())
print('Length:', column.get_length())
print('Fractional Digits:', column.get_fractional_digits())
print('Is Number Signed:', column.is_number_signed())
print('Collation Name:', column.get_collation_name())
print('Charset Name:', column.get_character_set_name())
print('Is ZeroFill:', column.is_zero_fill())

#@ Metadata on MediumInt Column
column = columns[column_index]
field = row[column_index]
column_index += 1
print('Field Type: ', type(field))
print('Schema Name:', column.get_schema_name())
print('Table Name:', column.get_table_name())
print('Table Label:', column.get_table_label())
print('Column Name:', column.get_column_name())
print('Column Label:', column.get_column_label())
print('Type:', column.get_type())
print('Length:', column.get_length())
print('Fractional Digits:', column.get_fractional_digits())
print('Is Number Signed:', column.is_number_signed())
print('Collation Name:', column.get_collation_name())
print('Charset Name:', column.get_character_set_name())
print('Is ZeroFill:', column.is_zero_fill())

#@ Metadata on Unsigned MediumInt Column
column = columns[column_index]
field = row[column_index]
column_index += 1
print('Field Type: ', type(field))
print('Schema Name:', column.get_schema_name())
print('Table Name:', column.get_table_name())
print('Table Label:', column.get_table_label())
print('Column Name:', column.get_column_name())
print('Column Label:', column.get_column_label())
print('Type:', column.get_type())
print('Length:', column.get_length())
print('Fractional Digits:', column.get_fractional_digits())
print('Is Number Signed:', column.is_number_signed())
print('Collation Name:', column.get_collation_name())
print('Charset Name:', column.get_character_set_name())
print('Is ZeroFill:', column.is_zero_fill())

#@ Metadata on Int Column
column = columns[column_index]
field = row[column_index]
column_index += 1
print('Field Type: ', type(field))
print('Schema Name:', column.get_schema_name())
print('Table Name:', column.get_table_name())
print('Table Label:', column.get_table_label())
print('Column Name:', column.get_column_name())
print('Column Label:', column.get_column_label())
print('Type:', column.get_type())
print('Length:', column.get_length())
print('Fractional Digits:', column.get_fractional_digits())
print('Is Number Signed:', column.is_number_signed())
print('Collation Name:', column.get_collation_name())
print('Charset Name:', column.get_character_set_name())
print('Is ZeroFill:', column.is_zero_fill())

#@ Metadata on Unsigned Int Column
column = columns[column_index]
field = row[column_index]
column_index += 1
print('Field Type: ', type(field))
print('Schema Name:', column.get_schema_name())
print('Table Name:', column.get_table_name())
print('Table Label:', column.get_table_label())
print('Column Name:', column.get_column_name())
print('Column Label:', column.get_column_label())
print('Type:', column.get_type())
print('Length:', column.get_length())
print('Fractional Digits:', column.get_fractional_digits())
print('Is Number Signed:', column.is_number_signed())
print('Collation Name:', column.get_collation_name())
print('Charset Name:', column.get_character_set_name())
print('Is ZeroFill:', column.is_zero_fill())

#@ Metadata on Float Column
column = columns[column_index]
field = row[column_index]
column_index += 1
print('Field Type: ', type(field))
print('Schema Name:', column.get_schema_name())
print('Table Name:', column.get_table_name())
print('Table Label:', column.get_table_label())
print('Column Name:', column.get_column_name())
print('Column Label:', column.get_column_label())
print('Type:', column.get_type())
print('Length:', column.get_length())
print('Fractional Digits:', column.get_fractional_digits())
print('Is Number Signed:', column.is_number_signed())
print('Collation Name:', column.get_collation_name())
print('Charset Name:', column.get_character_set_name())
print('Is ZeroFill:', column.is_zero_fill())

#@ Metadata on Unsigned Float Column
column = columns[column_index]
field = row[column_index]
column_index += 1
print('Field Type: ', type(field))
print('Schema Name:', column.get_schema_name())
print('Table Name:', column.get_table_name())
print('Table Label:', column.get_table_label())
print('Column Name:', column.get_column_name())
print('Column Label:', column.get_column_label())
print('Type:', column.get_type())
print('Length:', column.get_length())
print('Fractional Digits:', column.get_fractional_digits())
print('Is Number Signed:', column.is_number_signed())
print('Collation Name:', column.get_collation_name())
print('Charset Name:', column.get_character_set_name())
print('Is ZeroFill:', column.is_zero_fill())

#@ Metadata on Float Column with length and fractional digits
column = columns[column_index]
field = row[column_index]
column_index += 1
print('Field Type: ', type(field))
print('Schema Name:', column.get_schema_name())
print('Table Name:', column.get_table_name())
print('Table Label:', column.get_table_label())
print('Column Name:', column.get_column_name())
print('Column Label:', column.get_column_label())
print('Type:', column.get_type())
print('Length:', column.get_length())
print('Fractional Digits:', column.get_fractional_digits())
print('Is Number Signed:', column.is_number_signed())
print('Collation Name:', column.get_collation_name())
print('Charset Name:', column.get_character_set_name())
print('Is ZeroFill:', column.is_zero_fill())

#@ Metadata on Decimal Column
column = columns[column_index]
field = row[column_index]
column_index += 1
print('Field Type: ', type(field))
print('Schema Name:', column.get_schema_name())
print('Table Name:', column.get_table_name())
print('Table Label:', column.get_table_label())
print('Column Name:', column.get_column_name())
print('Column Label:', column.get_column_label())
print('Type:', column.get_type())
print('Length:', column.get_length())
print('Fractional Digits:', column.get_fractional_digits())
print('Is Number Signed:', column.is_number_signed())
print('Collation Name:', column.get_collation_name())
print('Charset Name:', column.get_character_set_name())
print('Is ZeroFill:', column.is_zero_fill())

#@ Metadata on Unsigned Decimal Column
column = columns[column_index]
field = row[column_index]
column_index += 1
print('Field Type: ', type(field))
print('Schema Name:', column.get_schema_name())
print('Table Name:', column.get_table_name())
print('Table Label:', column.get_table_label())
print('Column Name:', column.get_column_name())
print('Column Label:', column.get_column_label())
print('Type:', column.get_type())
print('Length:', column.get_length())
print('Fractional Digits:', column.get_fractional_digits())
print('Is Number Signed:', column.is_number_signed())
print('Collation Name:', column.get_collation_name())
print('Charset Name:', column.get_character_set_name())
print('Is ZeroFill:', column.is_zero_fill())

#@ Metadata on Decimal Column with length and fractional digits
column = columns[column_index]
field = row[column_index]
column_index += 1
print('Field Type: ', type(field))
print('Schema Name:', column.get_schema_name())
print('Table Name:', column.get_table_name())
print('Table Label:', column.get_table_label())
print('Column Name:', column.get_column_name())
print('Column Label:', column.get_column_label())
print('Type:', column.get_type())
print('Length:', column.get_length())
print('Fractional Digits:', column.get_fractional_digits())
print('Is Number Signed:', column.is_number_signed())
print('Collation Name:', column.get_collation_name())
print('Charset Name:', column.get_character_set_name())
print('Is ZeroFill:', column.is_zero_fill())

#@ Metadata on Double Column
column = columns[column_index]
field = row[column_index]
column_index += 1
print('Field Type: ', type(field))
print('Schema Name:', column.get_schema_name())
print('Table Name:', column.get_table_name())
print('Table Label:', column.get_table_label())
print('Column Name:', column.get_column_name())
print('Column Label:', column.get_column_label())
print('Type:', column.get_type())
print('Length:', column.get_length())
print('Fractional Digits:', column.get_fractional_digits())
print('Is Number Signed:', column.is_number_signed())
print('Collation Name:', column.get_collation_name())
print('Charset Name:', column.get_character_set_name())
print('Is ZeroFill:', column.is_zero_fill())

#@ Metadata on Unsigned Double Column
column = columns[column_index]
field = row[column_index]
column_index += 1
print('Field Type: ', type(field))
print('Schema Name:', column.get_schema_name())
print('Table Name:', column.get_table_name())
print('Table Label:', column.get_table_label())
print('Column Name:', column.get_column_name())
print('Column Label:', column.get_column_label())
print('Type:', column.get_type())
print('Length:', column.get_length())
print('Fractional Digits:', column.get_fractional_digits())
print('Is Number Signed:', column.is_number_signed())
print('Collation Name:', column.get_collation_name())
print('Charset Name:', column.get_character_set_name())
print('Is ZeroFill:', column.is_zero_fill())

#@ Metadata on Double Column with length and fractional digits
column = columns[column_index]
field = row[column_index]
column_index += 1
print('Field Type: ', type(field))
print('Schema Name:', column.get_schema_name())
print('Table Name:', column.get_table_name())
print('Table Label:', column.get_table_label())
print('Column Name:', column.get_column_name())
print('Column Label:', column.get_column_label())
print('Type:', column.get_type())
print('Length:', column.get_length())
print('Fractional Digits:', column.get_fractional_digits())
print('Is Number Signed:', column.is_number_signed())
print('Collation Name:', column.get_collation_name())
print('Charset Name:', column.get_character_set_name())
print('Is ZeroFill:', column.is_zero_fill())

# Metadata Validation On Other Types
result = mySession.sql("create table table2 (one json, two char(5) primary key, three varchar(20), four text, five time, six date, seven timestamp, eight set('1','2','3'), nine enum ('a','b','c'), ten varbinary(15), eleven blob)").execute()
table = schema.get_table('table2')

result = table.insert().values('{"name":"John", "Age":23}', 'test', 'sample', 'a_text', mysqlx.expr('NOW()'), mysqlx.expr('CURDATE()'), mysqlx.expr('NOW()'), '2','c', '¡¡¡¡', '£€¢').execute()

result = table.select().execute()
columns = result.get_columns()
column_index = 0

#@ Metadata on Json Column
column = columns[column_index]
field = row[column_index]
column_index += 1
print('Field Type: ', type(field))
print('Schema Name:', column.get_schema_name())
print('Table Name:', column.get_table_name())
print('Table Label:', column.get_table_label())
print('Column Name:', column.get_column_name())
print('Column Label:', column.get_column_label())
print('Type:', column.get_type())
print('Length:', column.get_length())
print('Fractional Digits:', column.get_fractional_digits())
print('Is Number Signed:', column.is_number_signed())
print('Collation Name:', column.get_collation_name())
print('Charset Name:', column.get_character_set_name())
print('Is ZeroFill:', column.is_zero_fill())

#@ Metadata on Char Column
column = columns[column_index]
field = row[column_index]
column_index += 1
print('Field Type: ', type(field))
print('Schema Name:', column.get_schema_name())
print('Table Name:', column.get_table_name())
print('Table Label:', column.get_table_label())
print('Column Name:', column.get_column_name())
print('Column Label:', column.get_column_label())
print('Type:', column.get_type())
print('Length:', column.get_length())
print('Fractional Digits:', column.get_fractional_digits())
print('Is Number Signed:', column.is_number_signed())
print('Collation Name:', formatCollation(column.get_collation_name()))
print('Charset Name:', column.get_character_set_name())
print('Is ZeroFill:', column.is_zero_fill())

#@ Metadata on Varchar Column
column = columns[column_index]
field = row[column_index]
column_index += 1
print('Field Type: ', type(field))
print('Schema Name:', column.get_schema_name())
print('Table Name:', column.get_table_name())
print('Table Label:', column.get_table_label())
print('Column Name:', column.get_column_name())
print('Column Label:', column.get_column_label())
print('Type:', column.get_type())
print('Length:', column.get_length())
print('Fractional Digits:', column.get_fractional_digits())
print('Is Number Signed:', column.is_number_signed())
print('Collation Name:', formatCollation(column.get_collation_name()))
print('Charset Name:', column.get_character_set_name())
print('Is ZeroFill:', column.is_zero_fill())

#@ Metadata on Text Column
column = columns[column_index]
field = row[column_index]
column_index += 1
print('Field Type: ', type(field))
print('Schema Name:', column.get_schema_name())
print('Table Name:', column.get_table_name())
print('Table Label:', column.get_table_label())
print('Column Name:', column.get_column_name())
print('Column Label:', column.get_column_label())
print('Type:', column.get_type())
print('Length:', column.get_length())
print('Fractional Digits:', column.get_fractional_digits())
print('Is Number Signed:', column.is_number_signed())
print('Collation Name:', formatCollation(column.get_collation_name()))
print('Charset Name:', column.get_character_set_name())
print('Is ZeroFill:', column.is_zero_fill())

#@ Metadata on Time Column
column = columns[column_index]
field = row[column_index]
column_index += 1
print('Field Type: ', type(field))
print('Schema Name:', column.get_schema_name())
print('Table Name:', column.get_table_name())
print('Table Label:', column.get_table_label())
print('Column Name:', column.get_column_name())
print('Column Label:', column.get_column_label())
print('Type:', column.get_type())
print('Length:', column.get_length())
print('Fractional Digits:', column.get_fractional_digits())
print('Is Number Signed:', column.is_number_signed())
print('Collation Name:', column.get_collation_name())
print('Charset Name:', column.get_character_set_name())
print('Is ZeroFill:', column.is_zero_fill())

#@ Metadata on Date Column
column = columns[column_index]
field = row[column_index]
column_index += 1
print('Field Type: ', type(field))
print('Schema Name:', column.get_schema_name())
print('Table Name:', column.get_table_name())
print('Table Label:', column.get_table_label())
print('Column Name:', column.get_column_name())
print('Column Label:', column.get_column_label())
print('Type:', column.get_type())
print('Length:', column.get_length())
print('Fractional Digits:', column.get_fractional_digits())
print('Is Number Signed:', column.is_number_signed())
print('Collation Name:', column.get_collation_name())
print('Charset Name:', column.get_character_set_name())
print('Is ZeroFill:', column.is_zero_fill())

#@ Metadata on DateTime Column
column = columns[column_index]
field = row[column_index]
column_index += 1
print('Field Type: ', type(field))
print('Schema Name:', column.get_schema_name())
print('Table Name:', column.get_table_name())
print('Table Label:', column.get_table_label())
print('Column Name:', column.get_column_name())
print('Column Label:', column.get_column_label())
print('Type:', column.get_type())
print('Length:', column.get_length())
print('Fractional Digits:', column.get_fractional_digits())
print('Is Number Signed:', column.is_number_signed())
print('Collation Name:', column.get_collation_name())
print('Charset Name:', column.get_character_set_name())
print('Is ZeroFill:', column.is_zero_fill())

#@ Metadata on Set Column
column = columns[column_index]
field = row[column_index]
column_index += 1
print('Field Type: ', type(field))
print('Schema Name:', column.get_schema_name())
print('Table Name:', column.get_table_name())
print('Table Label:', column.get_table_label())
print('Column Name:', column.get_column_name())
print('Column Label:', column.get_column_label())
print('Type:', column.get_type())
print('Length:', column.get_length())
print('Fractional Digits:', column.get_fractional_digits())
print('Is Number Signed:', column.is_number_signed())
print('Collation Name:', formatCollation(column.get_collation_name()))
print('Charset Name:', column.get_character_set_name())
print('Is ZeroFill:', column.is_zero_fill())

#@ Metadata on Enum Column
column = columns[column_index]
field = row[column_index]
column_index += 1
print('Field Type: ', type(field))
print('Schema Name:', column.get_schema_name())
print('Table Name:', column.get_table_name())
print('Table Label:', column.get_table_label())
print('Column Name:', column.get_column_name())
print('Column Label:', column.get_column_label())
print('Type:', column.get_type())
print('Length:', column.get_length())
print('Fractional Digits:', column.get_fractional_digits())
print('Is Number Signed:', column.is_number_signed())
print('Collation Name:', formatCollation(column.get_collation_name()))
print('Charset Name:', column.get_character_set_name())
print('Is ZeroFill:', column.is_zero_fill())

#@ Metadata on VarBinary Column
column = columns[column_index]
field = row[column_index]
column_index += 1
print('Field Type: ', type(field))
print('Schema Name:', column.get_schema_name())
print('Table Name:', column.get_table_name())
print('Table Label:', column.get_table_label())
print('Column Name:', column.get_column_name())
print('Column Label:', column.get_column_label())
print('Type:', column.get_type())
print('Length:', column.get_length())
print('Fractional Digits:', column.get_fractional_digits())
print('Is Number Signed:', column.is_number_signed())
print('Collation Name:', formatCollation(column.get_collation_name()))
print('Charset Name:', column.get_character_set_name())
print('Is ZeroFill:', column.is_zero_fill())

#@ Metadata on Blob Column
column = columns[column_index]
field = row[column_index]
column_index += 1
print('Field Type: ', type(field))
print('Schema Name:', column.get_schema_name())
print('Table Name:', column.get_table_name())
print('Table Label:', column.get_table_label())
print('Column Name:', column.get_column_name())
print('Column Label:', column.get_column_label())
print('Type:', column.get_type())
print('Length:', column.get_length())
print('Fractional Digits:', column.get_fractional_digits())
print('Is Number Signed:', column.is_number_signed())
print('Collation Name:', formatCollation(column.get_collation_name()))
print('Charset Name:', column.get_character_set_name())
print('Is ZeroFill:', column.is_zero_fill())

#@ Aggregated column type
result = mySession.run_sql("select count(*) from mysql.user")
columns = result.get_columns()
print("Count(*) Type:", columns[0].get_type().data)

mySession.close()
