//@ Session: validating members
|close: OK|
|createSchema: OK|
|getCurrentSchema: OK|
|getDefaultSchema: OK|
|getSchema: OK|
|getSchemas: OK|
|getUri: OK|
|setCurrentSchema: OK|
|defaultSchema: OK|
|uri: OK|
|currentSchema: OK|

//@<OUT> Session: help
Provides facilities to execute queries and retrieve database objects.

The following properties are currently supported.

 - uri           Retrieves the URI for the current session.
 - defaultSchema Retrieves the ClassicSchema configured as default for the
                 session.
 - currentSchema Retrieves the ClassicSchema that is active as current on the
                 session.


The following functions are currently supported.

 - close            Closes the internal connection to the MySQL Server held on
                    this session object.
 - commit           Commits all the operations executed after a call to
                    startTransaction().
 - createSchema     Creates a schema on the database and returns the
                    corresponding object.
 - dropSchema       Drops the schema with the specified name.
 - dropTable        Drops a table from the specified schema.
 - dropView         Drops a view from the specified schema.
 - getCurrentSchema Retrieves the ClassicSchema that is active as current on
                    the session.
 - getDefaultSchema Retrieves the ClassicSchema configured as default for the
                    session.
 - getSchema        Retrieves a ClassicSchema object from the current session
                    through it's name.
 - getSchemas       Retrieves the Schemas available on the session.
 - getUri           Retrieves the URI for the current session.
 - help             Provides help about this class and it's members
 - isOpen           Returns true if session is known to be open.
 - rollback         Discards all the operations executed after a call to
                    startTransaction().
 - runSql           Executes a query and returns the corresponding
                    ClassicResult object.
 - setCurrentSchema Sets the selected schema for this session's connection.
 - startTransaction Starts a transaction context on the server.


//@ ClassicSession: validate dynamic members for system schemas
|mysql: OK|
|information_schema: OK|

//@ ClassicSession: accessing Schemas
|<ClassicSchema:mysql>|
|<ClassicSchema:information_schema>|

//@ ClassicSession: accessing individual schema
|mysql|
|information_schema|

//@ ClassicSession: accessing unexisting schema
||Unknown database 'unexisting_schema'

//@ ClassicSession: current schema validations: nodefault
|null|
|null|

//@ ClassicSession: create schema success
|<ClassicSchema:node_session_schema>|

//@ ClassicSession: create schema failure
||Can't create database 'node_session_schema'; database exists

//@ Session: create quoted schema
|<ClassicSchema:quoted schema>|

//@ Session: validate dynamic members for created schemas
|node_session_schema: OK|
|quoted schema: OK|

//@ ClassicSession: Transaction handling: rollback
|Inserted Documents: 0|

//@ ClassicSession: Transaction handling: commit
|Inserted Documents: 3|

//@ ClassicSession: current schema validations: nodefault, mysql
|null|
|<ClassicSchema:mysql>|

//@ ClassicSession: current schema validations: nodefault, information_schema
|null|
|<ClassicSchema:information_schema>|

//@ ClassicSession: current schema validations: default
|<ClassicSchema:mysql>|
|<ClassicSchema:mysql>|

//@ ClassicSession: current schema validations: default, information_schema
|<ClassicSchema:mysql>|
|<ClassicSchema:information_schema>|

//$ ClassicSession: date handling
|9999-12-31 23:59:59.999999|

//@# ClassicSession: bad params
||Invalid connection options, expected either a URI or a Dictionary.
||Invalid connection options, expected either a URI or a Dictionary.
||Invalid connection options, expected either a URI or a Dictionary.
||Invalid connection options, expected either a URI or a Dictionary.
