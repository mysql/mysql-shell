#@<OUT> Testing collection help

The following properties are currently supported.

 - name
 - session
 - schema


The following functions are currently supported.

 - add                Inserts one or more documents into a collection.
 - create_index       Creates a non unique/unique index on a collection.
 - drop_index         Drops an index from a collection.
 - exists_in_database
 - find               Retrieves documents from a collection, matching a
                      specified criteria.
 - get_name
 - get_schema
 - get_session
 - help               Provides help about this class and it's members
 - modify             Creates a collection update handler.
 - remove             Creates a document deletion handler.

#@ Validating members
|Member Count: 14|

|name: OK|
|schema: OK|
|session: OK|
|exists_in_database: OK|
|get_name: OK|
|get_schema: OK|
|get_session: OK|
|add: OK|
|create_index: OK|
|drop_index: OK|
|modify: OK|
|remove: OK|
|find: OK|
|help: OK|


#@ Testing collection name retrieving
|get_name(): collection1|
|name: collection1|

#@ Testing session retrieving
|get_session(): <Session:|
|session: <Session:|

#@ Testing collection schema retrieving
|get_schema(): <Schema:js_shell_test>|
|schema: <Schema:js_shell_test>|

#@<OUT> Testing help of drop_index
Drops an index from a collection.

SYNTAX

  <Collection>.drop_index()



#@ Testing dropping index
|None|
|None|
|None|

#@ Testing dropping index using execute
||AttributeError: 'NoneType' object has no attribute 'execute'

#@ Testing existence
|Valid: True|
|Invalid: False|

