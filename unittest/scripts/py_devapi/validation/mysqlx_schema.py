#@ Schema: validating members
|Member Count: 16|

|name: OK|
|schema: OK|
|session: OK|
|exists_in_database: OK|
|get_name: OK|
|get_schema: OK|
|get_session: OK|
|get_table: OK|
|get_tables: OK|
|get_collection: OK|
|get_collections: OK|
|create_collection: OK|
|get_collection_as_table: OK|

|table1: OK|
|view1: OK|
|collection1: OK|

#@ Testing schema name retrieving
|get_name(): js_shell_test|
|name: js_shell_test|

#@ Testing schema.get_session
|get_session(): <NodeSession:|

#@ Testing schema.session
|session: <NodeSession:|

#@ Testing schema schema retrieving
|get_schema(): None|
|schema: None|

#@ Testing tables, views and collection retrieval
|get_tables(): <Table:|
|get_collections(): <Collection:collection1>|

#@ Testing specific object retrieval
|Retrieving a Table: <Table:table1>|
|.<table>: <Table:table1>|
|Retrieving a View: <Table:view1>|
|.<view>: <Table:view1>|
|get_collection(): <Collection:collection1>|
|.<collection>: <Collection:collection1>|

#@# Testing specific object retrieval: unexisting objects
||The table js_shell_test.unexisting does not exist 
||The collection js_shell_test.unexisting does not exist 

#@# Testing specific object retrieval: empty name
||An empty name is invalid for a table
||An empty name is invalid for a collection

#@ Retrieving collection as table
|get_collection_as_table(): <Table:collection1>|

#@ Collection creation
|create_collection(): <Collection:my_sample_collection>|

#@ Testing existence
|Valid: True|
|Invalid: False|

