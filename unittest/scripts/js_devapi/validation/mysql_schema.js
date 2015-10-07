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
|getTables(): <ClassicTable:table1>|
|tables: <ClassicTable:table1>|
|getViews(): <ClassicView:view1>|
|views: <ClassicView:view1>|

//@ Testing specific object retrieval
|getTable(): <ClassicTable:table1>|
|.<table>: <ClassicTable:table1>|
|getView(): <ClassicView:view1>|
|.<view>: <ClassicView:view1>|

//@ Testing existence
|Valid: true|
|Invalid: false|

