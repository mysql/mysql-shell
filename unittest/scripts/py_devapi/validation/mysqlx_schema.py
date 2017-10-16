#@<OUT> Schema: help
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

 - create_collection       Creates in the current schema a new collection with
                           the specified name and retrieves an object
                           representing the new collection created.
 - drop_collection         Drops the specified collection.
 - drop_table              Drops the specified table.
 - drop_view               Drops the specified view
 - exists_in_database
 - get_collection          Returns the Collection of the given name for this
                           schema.
 - get_collection_as_table Returns a Table object representing a Collection on
                           the database.
 - get_collections         Returns a list of Collections for this Schema.
 - get_name
 - get_schema
 - get_session
 - get_table               Returns the Table of the given name for this schema.
 - get_tables              Returns a list of Tables for this Schema.
 - help                    Provides help about this class and it's members

#@ Schema: validating members
|Member Count: 20|

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

#@<OUT> Testing help for drop_collection

Drops the specified collection.

SYNTAX

  <Schema>.drop_collection()

RETURNS

 nothing.

#@<OUT> Testing help for drop_view

Drops the specified view

SYNTAX

  <Schema>.drop_view()

RETURNS

 nothing.

#@<OUT> Testing help for drop_table

Drops the specified table.

SYNTAX

  <Schema>.drop_table()

RETURNS

 nothing.

#@ Testing dropping existing schema objects
|<Table:table1>|
|None|
|<Table:view1>|
|None|
|<Collection:collection1>|
|None|

#@ Testing dropped objects are actually dropped
||The table js_shell_test.table1 does not exist
||The table js_shell_test.view1 does not exist
||The collection js_shell_test.collection1 does not exist

#@ Testing dropping non-existing schema objects
|None|
|None|
|None|

#@ Testing drop functions using execute
||AttributeError: 'NoneType' object has no attribute 'execute'
||AttributeError: 'NoneType' object has no attribute 'execute'
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

