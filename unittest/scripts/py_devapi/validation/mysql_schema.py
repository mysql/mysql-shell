#@ Testing schema name retrieving
|getName(): js_shell_test|
|name: js_shell_test|

#@ Testing schema.getSession
|getSession(): <ClassicSession:|

#@ Testing schema.session
|session: <ClassicSession:|

#@ Testing schema schema retrieving
|getSchema(): None|
|schema: None|

#@ Testing tables, views and collection retrieval
|getTables(): <ClassicTable:table1>|
|getViews(): <ClassicView:view1>|

#@ Testing specific object retrieval
|getTable(): <ClassicTable:table1>|
|.<table>: <ClassicTable:table1>|
|getView(): <ClassicView:view1>|
|.<view>: <ClassicView:view1>|

#@ Testing existence
|Valid: True|
|Invalid: False|

