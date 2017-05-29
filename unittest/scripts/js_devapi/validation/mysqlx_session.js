//@ Session: validating members
|close: OK|
|createSchema: OK|
|getCurrentSchema: OK|
|getDefaultSchema: OK|
|getSchema: OK|
|getSchemas: OK|
|getUri: OK|
|setCurrentSchema: OK|
|setFetchWarnings: OK|
|sql: OK|
|defaultSchema: OK|
|uri: OK|
|currentSchema: OK|

//@<OUT> Session: help
Document Store functionality can be used through this object, in addition to
SQL.

The following properties are currently supported.

 - uri           Retrieves the URI for the current session.
 - defaultSchema Retrieves the Schema configured as default for the session.
 - currentSchema Retrieves the active schema on the session.


The following functions are currently supported.

 - close            Closes the session.
 - commit           Commits all the operations executed after a call to
                    startTransaction().
 - createSchema     Creates a schema on the database and returns the
                    corresponding object.
 - dropCollection   Drops a collection from the specified schema.
 - dropSchema       Drops the schema with the specified name.
 - dropTable        Drops a table from the specified schema.
 - dropView         Drops a view from the specified schema.
 - getCurrentSchema Retrieves the active schema on the session.
 - getDefaultSchema Retrieves the Schema configured as default for the session.
 - getSchema        Retrieves a Schema object from the current session through
                    it's name.
 - getSchemas       Retrieves the Schemas available on the session.
 - getUri           Retrieves the URI for the current session.
 - help             Provides help about this class and it's members
 - isOpen           Verifies if the session is still open.
 - quoteName        Escapes the passed identifier.
 - rollback         Discards all the operations executed after a call to
                    startTransaction().
 - setCurrentSchema Sets the current schema for this session, and returns the
                    schema object for it.
 - setFetchWarnings Enables or disables warning generation.
 - sql              Creates a SqlExecute object to allow running the received
                    SQL statement on the target MySQL Server.
 - startTransaction Starts a transaction context on the server.


//@ Session: accessing Schemas
|<Schema:mysql>|
|<Schema:information_schema>|

//@ Session: accessing individual schema
|mysql|
|information_schema|

//@ Session: accessing unexisting schema
||Unknown database 'unexisting_schema'

//@ Session: current schema validations: nodefault
|null|
|null|

//@ Session: create schema success
|<Schema:node_session_schema>|

//@ Session: create schema failure
||Can't create database 'node_session_schema'; database exists

//@ Session: Transaction handling: rollback
|Inserted Documents: 0|

//@ Session: Transaction handling: commit
|Inserted Documents: 3|

//@ Session: current schema validations: nodefault, mysql
|null|
|<Schema:mysql>|

//@ Session: current schema validations: nodefault, information_schema
|null|
|<Schema:information_schema>|

//@ Session: current schema validations: default
|<Schema:mysql>|
|<Schema:mysql>|

//@ Session: current schema validations: default, information_schema
|<Schema:mysql>|
|<Schema:information_schema>|

//@ Session: setFetchWarnings(false)
|0|

//@ Session: setFetchWarnings(true)
|1|
|Can't drop database 'unexisting'; database doesn't exist|

//@ Session: quoteName no parameters
||Invalid number of arguments in Session.quoteName, expected 1 but got 0

//@ Session: quoteName wrong param type
||Argument #1 is expected to be a string

//@ Session: quoteName with correct parameters
|`sample`|
|`sam``ple`|
|```sample```|
|```sample`|
|`sample```|

//@# nodeSession: bad params
||Invalid connection options, expected either a URI or a Dictionary.
||Invalid connection options, expected either a URI or a Dictionary.
||Invalid connection options, expected either a URI or a Dictionary.
||Invalid connection options, expected either a URI or a Dictionary.
