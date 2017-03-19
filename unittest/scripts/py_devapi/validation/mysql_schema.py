#@ Schema: validating members
|Member Count: 12|

|name: OK|
|schema: OK|
|session: OK|
|exists_in_database: OK|
|get_name: OK|
|get_schema: OK|
|get_session: OK|
|get_table: OK|
|get_tables: OK|
|help: OK|

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

#@ Testing name shadowing: setup
||

#@ Testing name shadowing: name
|py_db_object_shadow|

#@ Testing name shadowing: getName
|py_db_object_shadow|

#@ Testing name shadowing: schema
||

#@ Testing name shadowing: getSchema
||

#@ Testing name shadowing: session
|<ClassicSession:|

#@ Testing name shadowing: getSession
|<ClassicSession:|

#@ Testing name shadowing: another
|<ClassicTable:another>|

#@ Testing name shadowing: getTable('another')
|<ClassicTable:another>|

#@ Testing name shadowing: getTable('name')
|<ClassicTable:name>|

#@ Testing name shadowing: getTable('schema')
|<ClassicTable:schema>|

#@ Testing name shadowing: getTable('session')
|<ClassicTable:session>|

#@ Testing name shadowing: getTable('getTable')
|<ClassicTable:getTable>|

#@ Testing name shadowing: getTable('get_table')
|<ClassicTable:get_table>|
