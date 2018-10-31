//@ Testing table name retrieving
|getName(): table1|
|name: table1|

//@ Testing session retrieving
|getSession(): <Session:|
|session: <Session:|

//@ Testing table schema retrieving
|getSchema(): <Schema:js_shell_test>|
|schema: <Schema:js_shell_test>|

//@ Testing existence
|Valid: true|
|Invalid: false|

//@ Testing view check
|Is View: false|

//@ WL12412-TS1_1: Count takes no arguments
||Table.count: Invalid number of arguments, expected 0 but got 1 (ArgumentError)

//@ WL12412-TS1_3: Count returns correct number of records
|Initial Row Count: 0|
|Final Row Count: 3|

//@ WL12412-TS2_2: Count throws error on unexisting table
||Table.count: Table 'js_shell_test.table_count' doesn't exist (RuntimeError)


