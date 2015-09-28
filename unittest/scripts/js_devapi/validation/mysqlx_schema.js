//@ Testing schema name retrieving
|getName(): js_shell_test|
|name: js_shell_test|

//@ Testing schema.getSession
|getSession(): <NodeSession:|

//@ Testing schema.session
|session: <NodeSession:|

//@ Testing schema schema retrieving
|getSchema(): null|
|schema: null|

//@ Testing tables, views and collection retrieval
|getTables(): <Table:table1>|
|tables: <Table:table1>|
|getViews(): <View:view1>|
|views: <View:view1>|
|getCollections(): <Collection:collection1>|
|collections: <Collection:collection1>|

//@ Testing specific object retrieval
|getTable(): <Table:table1>|
|.<table>: <Table:table1>|
|getView(): <View:view1>|
|.<view>: <View:view1>|
|getCollection(): <Collection:collection1>|
|.<collection>: <Collection:collection1>|

//@ Retrieving collection as table
|getCollectionAsTable(): <Table:collection1>|

//@ Collection creation
|createCollection(): <Collection:my_sample_collection>|

//@ Testing existence
|Valid: true|
|Invalid: false|

