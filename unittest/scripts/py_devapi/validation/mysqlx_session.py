#@ Session: validating members
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

#@<OUT> Session: help
Document Store functionality can be used through this object, in addition to
SQL.

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




#@ Session: accessing Schemas
|<Schema:mysql>|
|<Schema:information_schema>|

#@ Session: accessing individual schema
|mysql|
|information_schema|

#@ Session: accessing unexisting schema
||Unknown database 'unexisting_schema'

#@ Session: current schema validations: nodefault
|None|
|None|

#@ Session: create schema success
|<Schema:node_session_schema>|

#@ Session: create schema failure
||MySQL Error (1007): Can't create database 'node_session_schema'; database exists

#@ Session: Transaction handling: rollback
|Inserted Documents: 0|

#@ Session: Transaction handling: commit
|Inserted Documents: 3|

#@ Session: current schema validations: nodefault, mysql
|None|
|<Schema:mysql>|

#@ Session: current schema validations: nodefault, information_schema
|None|
|<Schema:information_schema>|

#@ Session: current schema validations: default
|<Schema:mysql>|
|<Schema:mysql>|

#@ Session: current schema validations: default, information_schema
|<Schema:mysql>|
|<Schema:information_schema>|

#@ Session: set_fetch_warnings(False)
|0|

#@ Session: set_fetch_warnings(True)
|1|
|Can't drop database 'unexisting'; database doesn't exist|

#@ Session: quote_name no parameters
||ArgumentError: Invalid number of arguments in Session.quote_name, expected 1 but got 0

#@ Session: quote_name wrong param type
||TypeError: Argument #1 is expected to be a string

#@ Session: quote_name with correct parameters
|`sample`|
|`sam``ple`|
|```sample```|
|```sample`|
|`sample```|

#@# nodeSession: bad params
||Invalid connection options, expected either a URI or a Dictionary.
||Invalid connection options, expected either a URI or a Dictionary.
||Invalid connection options, expected either a URI or a Dictionary.
||Invalid connection options, expected either a URI or a Dictionary.
