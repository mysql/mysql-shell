# Assumptions: validate_crud_functions available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
from mysqlsh import mysqlx

mySession = mysqlx.get_node_session(__uripwd)

ensure_schema_does_not_exist(mySession, 'py_shell_test')

schema = mySession.create_schema('py_shell_test')
mySession.set_current_schema('py_shell_test')

# Metadata Validation On Numeric Types
result = mySession.sql('create table table1 (one bit, two tinyint primary key, utwo tinyint unsigned, three smallint, uthree smallint unsigned, four mediumint, ufour mediumint unsigned, five int, ufive int unsigned, six float, usix float unsigned, csix float(5,3), seven decimal, useven decimal unsigned, cseven decimal(4,2), eight double, ueight double unsigned, ceight double(8,3))').execute()
table = schema.get_table('table1')

result = table.insert().values(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18).execute()

result = table.select().execute()
columns = result.get_columns()
column_index = 0

#@ Metadata on Bit Column
column = columns[column_index]
column_index += 1
print 'Schema Name:', column.get_schema_name()
print 'Table Name:', column.get_table_name()
print 'Table Label:', column.get_table_label()
print 'Column Name:', column.get_column_name()
print 'Column Label:', column.get_column_label()
print 'Type:', column.get_type()
print 'Length:', column.get_length()
print 'Fractional Digits:', column.get_fractional_digits()
print 'Is Number Signed:', column.is_number_signed()
print 'Collation Name:', column.get_collation_name()
print 'Charset Name:', column.get_character_set_name()
print 'Is Padded:', column.is_padded()

#@ Metadata on TinyInt Column
column = columns[column_index]
column_index += 1
print 'Schema Name:', column.get_schema_name()
print 'Table Name:', column.get_table_name()
print 'Table Label:', column.get_table_label()
print 'Column Name:', column.get_column_name()
print 'Column Label:', column.get_column_label()
print 'Type:', column.get_type()
print 'Length:', column.get_length()
print 'Fractional Digits:', column.get_fractional_digits()
print 'Is Number Signed:', column.is_number_signed()
print 'Collation Name:', column.get_collation_name()
print 'Charset Name:', column.get_character_set_name()
print 'Is Padded:', column.is_padded()

#@ Metadata on Unsigned TinyInt Column
column = columns[column_index]
column_index += 1
print 'Schema Name:', column.get_schema_name()
print 'Table Name:', column.get_table_name()
print 'Table Label:', column.get_table_label()
print 'Column Name:', column.get_column_name()
print 'Column Label:', column.get_column_label()
print 'Type:', column.get_type()
print 'Length:', column.get_length()
print 'Fractional Digits:', column.get_fractional_digits()
print 'Is Number Signed:', column.is_number_signed()
print 'Collation Name:', column.get_collation_name()
print 'Charset Name:', column.get_character_set_name()
print 'Is Padded:', column.is_padded()

#@ Metadata on SmallInt Column
column = columns[column_index]
column_index += 1
print 'Schema Name:', column.get_schema_name()
print 'Table Name:', column.get_table_name()
print 'Table Label:', column.get_table_label()
print 'Column Name:', column.get_column_name()
print 'Column Label:', column.get_column_label()
print 'Type:', column.get_type()
print 'Length:', column.get_length()
print 'Fractional Digits:', column.get_fractional_digits()
print 'Is Number Signed:', column.is_number_signed()
print 'Collation Name:', column.get_collation_name()
print 'Charset Name:', column.get_character_set_name()
print 'Is Padded:', column.is_padded()

#@ Metadata on Unsigned SmallInt Column
column = columns[column_index]
column_index += 1
print 'Schema Name:', column.get_schema_name()
print 'Table Name:', column.get_table_name()
print 'Table Label:', column.get_table_label()
print 'Column Name:', column.get_column_name()
print 'Column Label:', column.get_column_label()
print 'Type:', column.get_type()
print 'Length:', column.get_length()
print 'Fractional Digits:', column.get_fractional_digits()
print 'Is Number Signed:', column.is_number_signed()
print 'Collation Name:', column.get_collation_name()
print 'Charset Name:', column.get_character_set_name()
print 'Is Padded:', column.is_padded()

#@ Metadata on MediumInt Column
column = columns[column_index]
column_index += 1
print 'Schema Name:', column.get_schema_name()
print 'Table Name:', column.get_table_name()
print 'Table Label:', column.get_table_label()
print 'Column Name:', column.get_column_name()
print 'Column Label:', column.get_column_label()
print 'Type:', column.get_type()
print 'Length:', column.get_length()
print 'Fractional Digits:', column.get_fractional_digits()
print 'Is Number Signed:', column.is_number_signed()
print 'Collation Name:', column.get_collation_name()
print 'Charset Name:', column.get_character_set_name()
print 'Is Padded:', column.is_padded()

#@ Metadata on Unsigned MediumInt Column
column = columns[column_index]
column_index += 1
print 'Schema Name:', column.get_schema_name()
print 'Table Name:', column.get_table_name()
print 'Table Label:', column.get_table_label()
print 'Column Name:', column.get_column_name()
print 'Column Label:', column.get_column_label()
print 'Type:', column.get_type()
print 'Length:', column.get_length()
print 'Fractional Digits:', column.get_fractional_digits()
print 'Is Number Signed:', column.is_number_signed()
print 'Collation Name:', column.get_collation_name()
print 'Charset Name:', column.get_character_set_name()
print 'Is Padded:', column.is_padded()

#@ Metadata on Int Column
column = columns[column_index]
column_index += 1
print 'Schema Name:', column.get_schema_name()
print 'Table Name:', column.get_table_name()
print 'Table Label:', column.get_table_label()
print 'Column Name:', column.get_column_name()
print 'Column Label:', column.get_column_label()
print 'Type:', column.get_type()
print 'Length:', column.get_length()
print 'Fractional Digits:', column.get_fractional_digits()
print 'Is Number Signed:', column.is_number_signed()
print 'Collation Name:', column.get_collation_name()
print 'Charset Name:', column.get_character_set_name()
print 'Is Padded:', column.is_padded()

#@ Metadata on Unsigned Int Column
column = columns[column_index]
column_index += 1
print 'Schema Name:', column.get_schema_name()
print 'Table Name:', column.get_table_name()
print 'Table Label:', column.get_table_label()
print 'Column Name:', column.get_column_name()
print 'Column Label:', column.get_column_label()
print 'Type:', column.get_type()
print 'Length:', column.get_length()
print 'Fractional Digits:', column.get_fractional_digits()
print 'Is Number Signed:', column.is_number_signed()
print 'Collation Name:', column.get_collation_name()
print 'Charset Name:', column.get_character_set_name()
print 'Is Padded:', column.is_padded()

#@ Metadata on Float Column
column = columns[column_index]
column_index += 1
print 'Schema Name:', column.get_schema_name()
print 'Table Name:', column.get_table_name()
print 'Table Label:', column.get_table_label()
print 'Column Name:', column.get_column_name()
print 'Column Label:', column.get_column_label()
print 'Type:', column.get_type()
print 'Length:', column.get_length()
print 'Fractional Digits:', column.get_fractional_digits()
print 'Is Number Signed:', column.is_number_signed()
print 'Collation Name:', column.get_collation_name()
print 'Charset Name:', column.get_character_set_name()
print 'Is Padded:', column.is_padded()

#@ Metadata on Unsigned Float Column
column = columns[column_index]
column_index += 1
print 'Schema Name:', column.get_schema_name()
print 'Table Name:', column.get_table_name()
print 'Table Label:', column.get_table_label()
print 'Column Name:', column.get_column_name()
print 'Column Label:', column.get_column_label()
print 'Type:', column.get_type()
print 'Length:', column.get_length()
print 'Fractional Digits:', column.get_fractional_digits()
print 'Is Number Signed:', column.is_number_signed()
print 'Collation Name:', column.get_collation_name()
print 'Charset Name:', column.get_character_set_name()
print 'Is Padded:', column.is_padded()

#@ Metadata on Float Column with length and fractional digits
column = columns[column_index]
column_index += 1
print 'Schema Name:', column.get_schema_name()
print 'Table Name:', column.get_table_name()
print 'Table Label:', column.get_table_label()
print 'Column Name:', column.get_column_name()
print 'Column Label:', column.get_column_label()
print 'Type:', column.get_type()
print 'Length:', column.get_length()
print 'Fractional Digits:', column.get_fractional_digits()
print 'Is Number Signed:', column.is_number_signed()
print 'Collation Name:', column.get_collation_name()
print 'Charset Name:', column.get_character_set_name()
print 'Is Padded:', column.is_padded()

#@ Metadata on Decimal Column
column = columns[column_index]
column_index += 1
print 'Schema Name:', column.get_schema_name()
print 'Table Name:', column.get_table_name()
print 'Table Label:', column.get_table_label()
print 'Column Name:', column.get_column_name()
print 'Column Label:', column.get_column_label()
print 'Type:', column.get_type()
print 'Length:', column.get_length()
print 'Fractional Digits:', column.get_fractional_digits()
print 'Is Number Signed:', column.is_number_signed()
print 'Collation Name:', column.get_collation_name()
print 'Charset Name:', column.get_character_set_name()
print 'Is Padded:', column.is_padded()

#@ Metadata on Unsigned Decimal Column
column = columns[column_index]
column_index += 1
print 'Schema Name:', column.get_schema_name()
print 'Table Name:', column.get_table_name()
print 'Table Label:', column.get_table_label()
print 'Column Name:', column.get_column_name()
print 'Column Label:', column.get_column_label()
print 'Type:', column.get_type()
print 'Length:', column.get_length()
print 'Fractional Digits:', column.get_fractional_digits()
print 'Is Number Signed:', column.is_number_signed()
print 'Collation Name:', column.get_collation_name()
print 'Charset Name:', column.get_character_set_name()
print 'Is Padded:', column.is_padded()

#@ Metadata on Decimal Column with length and fractional digits
column = columns[column_index]
column_index += 1
print 'Schema Name:', column.get_schema_name()
print 'Table Name:', column.get_table_name()
print 'Table Label:', column.get_table_label()
print 'Column Name:', column.get_column_name()
print 'Column Label:', column.get_column_label()
print 'Type:', column.get_type()
print 'Length:', column.get_length()
print 'Fractional Digits:', column.get_fractional_digits()
print 'Is Number Signed:', column.is_number_signed()
print 'Collation Name:', column.get_collation_name()
print 'Charset Name:', column.get_character_set_name()
print 'Is Padded:', column.is_padded()

#@ Metadata on Double Column
column = columns[column_index]
column_index += 1
print 'Schema Name:', column.get_schema_name()
print 'Table Name:', column.get_table_name()
print 'Table Label:', column.get_table_label()
print 'Column Name:', column.get_column_name()
print 'Column Label:', column.get_column_label()
print 'Type:', column.get_type()
print 'Length:', column.get_length()
print 'Fractional Digits:', column.get_fractional_digits()
print 'Is Number Signed:', column.is_number_signed()
print 'Collation Name:', column.get_collation_name()
print 'Charset Name:', column.get_character_set_name()
print 'Is Padded:', column.is_padded()

#@ Metadata on Unsigned Double Column
column = columns[column_index]
column_index += 1
print 'Schema Name:', column.get_schema_name()
print 'Table Name:', column.get_table_name()
print 'Table Label:', column.get_table_label()
print 'Column Name:', column.get_column_name()
print 'Column Label:', column.get_column_label()
print 'Type:', column.get_type()
print 'Length:', column.get_length()
print 'Fractional Digits:', column.get_fractional_digits()
print 'Is Number Signed:', column.is_number_signed()
print 'Collation Name:', column.get_collation_name()
print 'Charset Name:', column.get_character_set_name()
print 'Is Padded:', column.is_padded()

#@ Metadata on Double Column with length and fractional digits
column = columns[column_index]
column_index += 1
print 'Schema Name:', column.get_schema_name()
print 'Table Name:', column.get_table_name()
print 'Table Label:', column.get_table_label()
print 'Column Name:', column.get_column_name()
print 'Column Label:', column.get_column_label()
print 'Type:', column.get_type()
print 'Length:', column.get_length()
print 'Fractional Digits:', column.get_fractional_digits()
print 'Is Number Signed:', column.is_number_signed()
print 'Collation Name:', column.get_collation_name()
print 'Charset Name:', column.get_character_set_name()
print 'Is Padded:', column.is_padded()

# Metadata Validation On Other Types
result = mySession.sql("create table table2 (one json, two char(5) primary key, three varchar(20), four text, five time, six date, seven timestamp, eight set('1','2','3'), nine enum ('a','b','c'))").execute()
table = schema.get_table('table2')

result = table.insert().values('{"name":"John", "Age":23}', 'test', 'sample', 'a_text', mysqlx.expr('NOW()'), mysqlx.expr('CURDATE()'), mysqlx.expr('NOW()'), '2','c').execute()

result = table.select().execute()
columns = result.get_columns()
column_index = 0

#@ Metadata on Json Column
column = columns[column_index]
column_index += 1
print 'Schema Name:', column.get_schema_name()
print 'Table Name:', column.get_table_name()
print 'Table Label:', column.get_table_label()
print 'Column Name:', column.get_column_name()
print 'Column Label:', column.get_column_label()
print 'Type:', column.get_type()
print 'Length:', column.get_length()
print 'Fractional Digits:', column.get_fractional_digits()
print 'Is Number Signed:', column.is_number_signed()
print 'Collation Name:', column.get_collation_name()
print 'Charset Name:', column.get_character_set_name()
print 'Is Padded:', column.is_padded()

#@ Metadata on Char Column
column = columns[column_index]
column_index += 1
print 'Schema Name:', column.get_schema_name()
print 'Table Name:', column.get_table_name()
print 'Table Label:', column.get_table_label()
print 'Column Name:', column.get_column_name()
print 'Column Label:', column.get_column_label()
print 'Type:', column.get_type()
print 'Length:', column.get_length()
print 'Fractional Digits:', column.get_fractional_digits()
print 'Is Number Signed:', column.is_number_signed()
print 'Collation Name:', column.get_collation_name()
print 'Charset Name:', column.get_character_set_name()
print 'Is Padded:', column.is_padded()

#@ Metadata on Varchar Column
column = columns[column_index]
column_index += 1
print 'Schema Name:', column.get_schema_name()
print 'Table Name:', column.get_table_name()
print 'Table Label:', column.get_table_label()
print 'Column Name:', column.get_column_name()
print 'Column Label:', column.get_column_label()
print 'Type:', column.get_type()
print 'Length:', column.get_length()
print 'Fractional Digits:', column.get_fractional_digits()
print 'Is Number Signed:', column.is_number_signed()
print 'Collation Name:', column.get_collation_name()
print 'Charset Name:', column.get_character_set_name()
print 'Is Padded:', column.is_padded()

#@ Metadata on Text Column
column = columns[column_index]
column_index += 1
print 'Schema Name:', column.get_schema_name()
print 'Table Name:', column.get_table_name()
print 'Table Label:', column.get_table_label()
print 'Column Name:', column.get_column_name()
print 'Column Label:', column.get_column_label()
print 'Type:', column.get_type()
print 'Length:', column.get_length()
print 'Fractional Digits:', column.get_fractional_digits()
print 'Is Number Signed:', column.is_number_signed()
print 'Collation Name:', column.get_collation_name()
print 'Charset Name:', column.get_character_set_name()
print 'Is Padded:', column.is_padded()

#@ Metadata on Time Column
column = columns[column_index]
column_index += 1
print 'Schema Name:', column.get_schema_name()
print 'Table Name:', column.get_table_name()
print 'Table Label:', column.get_table_label()
print 'Column Name:', column.get_column_name()
print 'Column Label:', column.get_column_label()
print 'Type:', column.get_type()
print 'Length:', column.get_length()
print 'Fractional Digits:', column.get_fractional_digits()
print 'Is Number Signed:', column.is_number_signed()
print 'Collation Name:', column.get_collation_name()
print 'Charset Name:', column.get_character_set_name()
print 'Is Padded:', column.is_padded()

#@ Metadata on Date Column
column = columns[column_index]
column_index += 1
print 'Schema Name:', column.get_schema_name()
print 'Table Name:', column.get_table_name()
print 'Table Label:', column.get_table_label()
print 'Column Name:', column.get_column_name()
print 'Column Label:', column.get_column_label()
print 'Type:', column.get_type()
print 'Length:', column.get_length()
print 'Fractional Digits:', column.get_fractional_digits()
print 'Is Number Signed:', column.is_number_signed()
print 'Collation Name:', column.get_collation_name()
print 'Charset Name:', column.get_character_set_name()
print 'Is Padded:', column.is_padded()

#@ Metadata on Timestamp Column
column = columns[column_index]
column_index += 1
print 'Schema Name:', column.get_schema_name()
print 'Table Name:', column.get_table_name()
print 'Table Label:', column.get_table_label()
print 'Column Name:', column.get_column_name()
print 'Column Label:', column.get_column_label()
print 'Type:', column.get_type()
print 'Length:', column.get_length()
print 'Fractional Digits:', column.get_fractional_digits()
print 'Is Number Signed:', column.is_number_signed()
print 'Collation Name:', column.get_collation_name()
print 'Charset Name:', column.get_character_set_name()
print 'Is Padded:', column.is_padded()

#@ Metadata on Set Column
column = columns[column_index]
column_index += 1
print 'Schema Name:', column.get_schema_name()
print 'Table Name:', column.get_table_name()
print 'Table Label:', column.get_table_label()
print 'Column Name:', column.get_column_name()
print 'Column Label:', column.get_column_label()
print 'Type:', column.get_type()
print 'Length:', column.get_length()
print 'Fractional Digits:', column.get_fractional_digits()
print 'Is Number Signed:', column.is_number_signed()
print 'Collation Name:', column.get_collation_name()
print 'Charset Name:', column.get_character_set_name()
print 'Is Padded:', column.is_padded()

#@ Metadata on Enum Column
column = columns[column_index]
column_index += 1
print 'Schema Name:', column.get_schema_name()
print 'Table Name:', column.get_table_name()
print 'Table Label:', column.get_table_label()
print 'Column Name:', column.get_column_name()
print 'Column Label:', column.get_column_label()
print 'Type:', column.get_type()
print 'Length:', column.get_length()
print 'Fractional Digits:', column.get_fractional_digits()
print 'Is Number Signed:', column.is_number_signed()
print 'Collation Name:', column.get_collation_name()
print 'Charset Name:', column.get_character_set_name()
print 'Is Padded:', column.is_padded()

mySession.close()
