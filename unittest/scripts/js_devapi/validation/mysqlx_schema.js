
//@ Testing schema name retrieving
|getName(): js_shell_test|
|name: js_shell_test|

//@ Testing schema.getSession
|getSession(): <Session:|

//@ Testing schema.session
|session: <Session:|

//@ Testing schema schema retrieving
|getSchema(): <Schema:js_shell_test>|
|schema: <Schema:js_shell_test>|

//@ Testing tables, views and collection retrieval
|getTables(): <Table:table1>|
|getCollections(): <Collection:collection1>|

//@ Testing specific object retrieval
|Retrieving a table: <Table:table1>|
|.<table>: <Table:table1>|
|Retrieving a view: <Table:view1>|
|.<view>: <Table:view1>|
|getCollection(): <Collection:collection1>|
|.<collection>: <Collection:collection1>|

//@# Testing specific object retrieval: unexisting objects
||The table js_shell_test.unexisting does not exist
||The collection js_shell_test.unexisting does not exist

//@# Testing specific object retrieval: empty name
||An empty name is invalid for a table
||An empty name is invalid for a collection

//@ Retrieving collection as table
|getCollectionAsTable(): <Table:collection1>|

//@ Query collection as table
|getCollectionAsTable().select(): <RowResult>|

//@ Collection creation
|createCollection(): <Collection:my_sample_collection>|

//@ Testing dropping existing schema objects
|<Collection:collection1>|
|undefined|

//@ Testing dropped objects are actually dropped
||The collection js_shell_test.collection1 does not exist

//@ Testing dropping non-existing schema objects
|undefined|

//@ Testing drop functions using execute
||TypeError: Cannot read property 'execute' of undefined

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
|<Session:|

//@ Testing name shadowing: getSession
|<Session:|

//@ Testing name shadowing: another
|<Collection:another>|

//@ Testing name shadowing: getCollection('another')
|<Collection:another>|

//@ Testing name shadowing: getTable('name')
|<Table:name>|

//@ Testing name shadowing: getCollection('schema')
|<Collection:schema>|

//@ Testing name shadowing: getTable('session')
|<Table:session>|

//@ Testing name shadowing: getCollection('getTable')
|<Collection:<<<name_get_table>>>>|

//@ Testing name shadowing: get_table (not a JS function)
|<Table:get_table>|

//@ Testing name shadowing: getTable('get_table')
|<Table:get_table>|

//@ Testing name shadowing: getCollection('getCollection')
|<Collection:<<<name_get_collection>>>>|

//@ Testing name shadowing: get_collection (not a JS function)
|<Table:get_collection>|

//@ Testing name shadowing: getTable('get_collection')
|<Table:get_collection>|

//@ cleanup
||
