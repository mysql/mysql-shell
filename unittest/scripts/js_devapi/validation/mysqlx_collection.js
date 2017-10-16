
//@<OUT> Testing collection help
The following properties are currently supported.

 - name
 - session
 - schema


The following functions are currently supported.

 - add              Inserts one or more documents into a collection.
 - createIndex      Creates a non unique/unique index on a collection.
 - dropIndex        Drops an index from a collection.
 - existsInDatabase
 - find             Retrieves documents from a collection, matching a specified
                    criteria.
 - getName
 - getSchema
 - getSession
 - help             Provides help about this class and it's members
 - modify           Creates a collection update handler.
 - remove           Creates a document deletion handler.

//@ Validating members
|Member Count: 14|

|name: OK|
|schema: OK|
|session: OK|
|existsInDatabase: OK|
|getName: OK|
|getSchema: OK|
|getSession: OK|
|add: OK|
|createIndex: OK|
|dropIndex: OK|
|modify: OK|
|remove: OK|
|find: OK|
|help: OK|

//@ Testing collection name retrieving
|getName(): collection1|
|name: collection1|

//@ Testing session retrieving
|getSession(): <Session:|
|session: <Session:|

//@ Testing collection schema retrieving
|getSchema(): <Schema:js_shell_test>|
|schema: <Schema:js_shell_test>|

//@<OUT> Testing help of dropIndex

Drops an index from a collection.

SYNTAX

  <Collection>.dropIndex()


//@ Testing dropping index
|undefined|
|undefined|
|undefined|

//@ Testing dropping index using execute
||TypeError: Cannot read property 'execute' of undefined

//@ Testing existence
|Valid: true|
|Invalid: false|

