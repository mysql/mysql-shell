//@ Schema: validating members
|Member Count: 12|

|name: OK|
|schema: OK|
|session: OK|
|existsInDatabase: OK|
|getName: OK|
|getSchema: OK|
|getSession: OK|
|getTable: OK|
|getTables: OK|
|help: OK|

|table1: OK|
|view1: OK|

//@ Testing schema name retrieving
|getName(): js_shell_test|
|name: js_shell_test|

//@ Testing schema.getSession
|getSession(): <ClassicSession:|

//@ Testing schema.session
|session: <ClassicSession:|

//@ Testing schema schema retrieving
|getSchema(): null|
|schema: null|

//@ Testing tables, views and collection retrieval
|getTables(): <ClassicTable:|

//@ Testing specific object retrieval
|Retrieving a table: <ClassicTable:table1>|
|.<table>: <ClassicTable:table1>|
|Retrieving a view: <ClassicTable:view1>|
|.<view>: <ClassicTable:view1>|

//@ Testing existence
|Valid: true|
|Invalid: false|

//@ Testing name shadowing: setup
||

//@ Testing name shadowing: name
|js_db_object_shadow|

//@ Testing name shadowing: getName
|js_db_object_shadow|

//@ Testing name shadowing: schema
||

//@ Testing name shadowing: getSchema
||

//@ Testing name shadowing: session
|<ClassicSession:|

//@ Testing name shadowing: getSession
|<ClassicSession:|

//@ Testing name shadowing: another
|<ClassicTable:another>|

//@ Testing name shadowing: getTable('another')
|<ClassicTable:another>|

//@ Testing name shadowing: getTable('name')
|<ClassicTable:name>|

//@ Testing name shadowing: getTable('schema')
|<ClassicTable:schema>|

//@ Testing name shadowing: getTable('session')
|<ClassicTable:session>|

//@ Testing name shadowing: getTable('getTable')
|<ClassicTable:<<<name_get_table>>>>|
