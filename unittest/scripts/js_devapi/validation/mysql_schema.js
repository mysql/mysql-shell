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
