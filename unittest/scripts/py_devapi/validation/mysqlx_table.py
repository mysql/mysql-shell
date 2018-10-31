#@ Testing table name retrieving
|get_name(): table1|
|name: table1|

#@ Testing session retrieving
|get_session(): <Session:|
|session: <Session:|

#@ Testing table schema retrieving
|get_schema(): <Schema:py_shell_test>|
|schema: <Schema:py_shell_test>|

#@ Testing existence
|Valid: True|
|Invalid: False|

#@ Testing view check
|Is View: False|

#@ WL12412-TS1_1: Count takes no arguments
||Table.count: Invalid number of arguments, expected 0 but got 1

#@ WL12412-TS1_3: Count returns correct number of records
|Initial Row Count: 0|
|Final Row Count: 3|

#@ WL12412-TS2_2: Count throws error on unexisting table
||RuntimeError: Table.count: Table 'py_shell_test.table_count' doesn't exist
