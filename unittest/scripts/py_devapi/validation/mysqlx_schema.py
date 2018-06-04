#@ Schema: validating members
|Member Count: 18|

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
|help: OK|
|drop_collection: OK|
|drop_view: OK|
|drop_table: OK|

|table1: OK|
|view1: OK|
|collection1: OK|

#@ Testing schema name retrieving
|get_name(): js_shell_test|
|name: js_shell_test|

#@ Testing schema.get_session
|get_session(): <Session:|

#@ Testing schema.session
|session: <Session:|

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

#@ Query collection as table
|get_collection_as_table().select(): <RowResult>|

#@ Collection creation
|create_collection(): <Collection:my_sample_collection>|

#@ Testing dropping existing schema objects
|<Collection:collection1>|
|None|

#@ Testing dropped objects are actually dropped
||The collection js_shell_test.collection1 does not exist

#@ Testing dropping non-existing schema objects
|None|

#@ Testing drop functions using execute
||AttributeError: 'NoneType' object has no attribute 'execute'

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
|<Session:|

#@ Testing name shadowing: getSession
|<Session:|

#@ Testing name shadowing: another
|<Collection:another>|

#@ Testing name shadowing: get_collection('another')
|<Collection:another>|

#@ Testing name shadowing: get_table('name')
|<Table:name>|

#@ Testing name shadowing: get_collection('schema')
|<Collection:schema>|

#@ Testing name shadowing: get_table('session')
|<Table:session>|

#@ Testing name shadowing: get_collection('getTable')
|<Collection:<<<name_get_table>>>>|

#@ Testing name shadowing: getTable (not a python function)
|<Collection:<<<name_get_table>>>>|

#@ Testing name shadowing: get_table('get_table')
|<Table:get_table>|

#@ Testing name shadowing: get_collection('getCollection')
|<Collection:<<<name_get_collection>>>>|

#@ Testing name shadowing: getCollection (not a python function)
|<Collection:<<<name_get_collection>>>>|

#@ Testing name shadowing: get_table('get_collection')
|<Table:get_collection>|
