//@<OUT> Schema: help
**  View Support  **

MySQL Views are stored queries that when executed produce a result set.

MySQL supports the concept of Updatable Views: in specific conditions are met,
Views can be used not only to retrieve data from them but also to update, add
and delete records.

For the purpose of this API, Views behave similar to a Table, and so they are
threated as Tables.

The following properties are currently supported.

 - name
 - session
 - schema
 - table1
 - view1
 - collection1


The following functions are currently supported.

 - createCollection     Creates in the current schema a new collection with the
                        specified name and retrieves an object representing the
                        new collection created.
 - dropCollection       Drops the specified collection.
 - existsInDatabase
 - getCollection        Returns the Collection of the given name for this
                        schema.
 - getCollectionAsTable Returns a Table object representing a Collection on the
                        database.
 - getCollections       Returns a list of Collections for this Schema.
 - getName
 - getSchema
 - getSession
 - getTable             Returns the Table of the given name for this schema.
 - getTables            Returns a list of Tables for this Schema.
 - help                 Provides help about this class and it's members

//@ Schema: validating members
|Member Count: 18|

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
|help: OK|
|dropCollection: OK|
|dropView: OK|
|dropTable: OK|

|table1: OK|
|view1: OK|
|collection1: OK|

//@ Testing schema name retrieving
|getName(): js_shell_test|
|name: js_shell_test|

//@ Testing schema.getSession
|getSession(): <Session:|

//@ Testing schema.session
|session: <Session:|

//@ Testing schema schema retrieving
|getSchema(): null|
|schema: null|

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

//@<OUT> Testing help for dropCollection
Drops the specified collection.

SYNTAX

  <Schema>.dropCollection()

RETURNS

 nothing.


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
