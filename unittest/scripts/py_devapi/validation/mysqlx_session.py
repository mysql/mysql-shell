#@ NodeSession: validating members
|close: OK|
|create_schema: OK|
|get_current_schema: OK|
|get_default_schema: OK|
|get_schema: OK|
|get_schemas: OK|
|get_uri: OK|
|set_current_schema: OK|
|set_fetch_warnings: OK|
|sql: OK|
|default_schema: OK|
|uri: OK|
|current_schema: OK|

#@<OUT> NodeSession: help
Enables interaction with an X Protocol enabled MySQL Server, this includes SQL
Execution.

The following properties are currently supported.

 - uri            Retrieves the URI for the current session.
 - default_schema Retrieves the Schema configured as default for the session.
 - current_schema Retrieves the active schema on the session.


The following functions are currently supported.

 - close              Closes the session.
 - commit             Commits all the operations executed after a call to
                      startTransaction().
 - create_schema      Creates a schema on the database and returns the
                      corresponding object.
 - drop_collection    Drops a collection from the specified schema.
 - drop_schema        Drops the schema with the specified name.
 - drop_table         Drops a table from the specified schema.
 - drop_view          Drops a view from the specified schema.
 - get_current_schema Retrieves the active schema on the session.
 - get_default_schema Retrieves the Schema configured as default for the
                      session.
 - get_schema         Retrieves a Schema object from the current session
                      through it's name.
 - get_schemas        Retrieves the Schemas available on the session.
 - get_uri            Retrieves the URI for the current session.
 - help               Provides help about this class and it's members
 - is_open            Verifies if the session is still open.
 - quote_name         Escapes the passed identifier.
 - rollback           Discards all the operations executed after a call to
                      startTransaction().
 - set_current_schema Sets the current schema for this session, and returns the
                      schema object for it.
 - set_fetch_warnings Enables or disables warning generation.
 - sql                Creates a SqlExecute object to allow running the received
                      SQL statement on the target MySQL Server.
 - start_transaction  Starts a transaction context on the server.




#@ NodeSession: accessing Schemas
|<Schema:mysql>|
|<Schema:information_schema>|

#@ NodeSession: accessing individual schema
|mysql|
|information_schema|

#@ NodeSession: accessing unexisting schema
||Unknown database 'unexisting_schema'

#@ NodeSession: current schema validations: nodefault
|None|
|None|

#@ NodeSession: create schema success
|<Schema:node_session_schema>|

#@ NodeSession: create schema failure
||MySQL Error (1007): Can't create database 'node_session_schema'; database exists

#@ NodeSession: Transaction handling: rollback
|Inserted Documents: 0|

#@ NodeSession: Transaction handling: commit
|Inserted Documents: 3|

#@ NodeSession: current schema validations: nodefault, mysql
|None|
|<Schema:mysql>|

#@ NodeSession: current schema validations: nodefault, information_schema
|None|
|<Schema:information_schema>|

#@ NodeSession: current schema validations: default
|<Schema:mysql>|
|<Schema:mysql>|

#@ NodeSession: current schema validations: default, information_schema
|<Schema:mysql>|
|<Schema:information_schema>|

#@ NodeSession: set_fetch_warnings(False)
|0|

#@ NodeSession: set_fetch_warnings(True)
|1|
|Can't drop database 'unexisting'; database doesn't exist|

#@ NodeSession: quote_name no parameters
||ArgumentError: Invalid number of arguments in NodeSession.quote_name, expected 1 but got 0

#@ NodeSession: quote_name wrong param type
||TypeError: Argument #1 is expected to be a string

#@ NodeSession: quote_name with correct parameters
|`sample`|
|`sam``ple`|
|```sample```|
|```sample`|
|`sample```|