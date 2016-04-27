#@ Schema: validating members
|Member Count: 16|

|name: OK|
|schema: OK|
|session: OK|
|existsInDatabase: OK|
|getName: OK|
|getSchema: OK|
|getSession: OK|
|getTable: OK|
|getTables: OK|
|getCollection: OK|
|getCollections: OK|
|createCollection: OK|
|getCollectionAsTable: OK|

|table1: OK|
|view1: OK|
|collection1: OK|

#@ Testing schema name retrieving
|getName(): js_shell_test|
|name: js_shell_test|

#@ Testing schema.getSession
|getSession(): <NodeSession:|

#@ Testing schema.session
|session: <NodeSession:|

#@ Testing schema schema retrieving
|getSchema(): None|
|schema: None|

#@ Testing tables, views and collection retrieval
|getTables(): <Table:|
|getCollections(): <Collection:collection1>|

#@ Testing specific object retrieval
|Retrieving a Table: <Table:table1>|
|.<table>: <Table:table1>|
|Retrieving a View: <Table:view1>|
|.<view>: <Table:view1>|
|getCollection(): <Collection:collection1>|
|.<collection>: <Collection:collection1>|

#@# Testing specific object retrieval: unexisting objects
||The table js_shell_test.unexisting does not exist 
||The collection js_shell_test.unexisting does not exist 

#@# Testing specific object retrieval: empty name
||An empty name is invalid for a table
||An empty name is invalid for a collection

#@ Retrieving collection as table
|getCollectionAsTable(): <Table:collection1>|

#@ Collection creation
|createCollection(): <Collection:my_sample_collection>|

#@ Testing existence
|Valid: True|
|Invalid: False|

