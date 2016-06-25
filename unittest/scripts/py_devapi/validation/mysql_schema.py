#@ Schema: validating members
|Member Count: 11|

|name: OK|
|schema: OK|
|session: OK|
|exists_in_database: OK|
|get_name: OK|
|get_schema: OK|
|get_session: OK|
|get_table: OK|
|get_tables: OK|

|table1: OK|
|view1: OK|

#@ Testing schema name retrieving
|get_name(): js_shell_test|
|name: js_shell_test|

#@ Testing schema.get_session
|get_session(): <ClassicSession:|

#@ Testing schema.session
|session: <ClassicSession:|

#@ Testing schema schema retrieving
|get_schema(): None|
|schema: None|

#@ Testing tables, views and collection retrieval
|get_tables(): <ClassicTable:|

#@ Testing specific object retrieval
|Retrieving a table: <ClassicTable:table1>|
|.<table>: <ClassicTable:table1>|
|Retrieving a view: <ClassicTable:view1>|
|.<view>: <ClassicTable:view1>|

#@ Testing existence
|Valid: True|
|Invalid: False|

