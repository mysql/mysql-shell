#@ Session: validating members
|close: OK|
|create_schema: OK|
|get_current_schema: OK|
|get_default_schema: OK|
|get_schema: OK|
|get_schemas: OK|
|get_uri: OK|
|set_current_schema: OK|
|default_schema: OK|
|uri: OK|
|current_schema: OK|

#@<OUT> Session: help
Provides facilities to execute queries and retrieve database objects.

The following properties are currently supported.

 - uri            Retrieves the URI for the current session.
 - default_schema Retrieves the ClassicSchema configured as default for the
                  session.
 - current_schema Retrieves the ClassicSchema that is active as current on the
                  session.


The following functions are currently supported.

 - close              Closes the internal connection to the MySQL Server held
                      on this session object.
 - commit             Commits all the operations executed after a call to
                      startTransaction().
 - create_schema      Creates a schema on the database and returns the
                      corresponding object.
 - drop_schema        Drops the schema with the specified name.
 - drop_table         Drops a table from the specified schema.
 - drop_view          Drops a view from the specified schema.
 - get_current_schema Retrieves the ClassicSchema that is active as current on
                      the session.
 - get_default_schema Retrieves the ClassicSchema configured as default for the
                      session.
 - get_schema         Retrieves a ClassicSchema object from the current session
                      through it's name.
 - get_schemas        Retrieves the Schemas available on the session.
 - get_uri            Retrieves the URI for the current session.
 - help               Provides help about this class and it's members
 - is_open            Returns true if session is known to be open.
 - query              Executes a query and returns the corresponding
                      ClassicResult object.
 - rollback           Discards all the operations executed after a call to
                      startTransaction().
 - run_sql            Executes a query and returns the corresponding
                      ClassicResult object.
 - set_current_schema Sets the selected schema for this session's connection.
 - start_transaction  Starts a transaction context on the server.


#@ ClassicSession: validate dynamic members for system schemas
|mysql: OK|
|information_schema: OK|

#@ ClassicSession: accessing Schemas
|<ClassicSchema:mysql>|
|<ClassicSchema:information_schema>|

#@ ClassicSession: accessing individual schema
|mysql|
|information_schema|

#@ ClassicSession: accessing unexisting schema
||Unknown database 'unexisting_schema'

#@ ClassicSession: current schema validations: nodefault
|None|
|None|

#@ ClassicSession: create schema success
|<ClassicSchema:node_session_schema>|

#@ ClassicSession: create schema failure
||MySQL Error (1007): ClassicSession.create_schema: Can't create database 'node_session_schema'; database exists

#@ Session: create quoted schema
|<ClassicSchema:quoted schema>|

#@ Session: validate dynamic members for created schemas
|node_session_schema: OK|
|quoted schema: OK|

#@ ClassicSession: Transaction handling: rollback
|Inserted Documents: 0|

#@ ClassicSession: Transaction handling: commit
|Inserted Documents: 3|

#@ ClassicSession: current schema validations: nodefault, mysql
|None|
|<ClassicSchema:mysql>|

#@ ClassicSession: current schema validations: nodefault, information_schema
|None|
|<ClassicSchema:information_schema>|

#@ ClassicSession: current schema validations: default
|<ClassicSchema:mysql>|
|<ClassicSchema:mysql>|

#@ ClassicSession: current schema validations: default, information_schema
|<ClassicSchema:mysql>|
|<ClassicSchema:information_schema>|

#@ ClassicSession: date handling
|9999-12-31 23:59:59.999999|

#@ ClassicSession: placeholders handling
|+-------+------+|
|| hello | 1234 ||
|+-------+------+|
|| hello | 1234 ||
|+-------+------+|
|+-------+------+|
|| hello | 1234 ||
|+-------+------+|
|| hello | 1234 ||
|+-------+------+|

#@# ClassicSession: bad params
||Invalid connection options, expected either a URI or a Dictionary.
||Invalid connection options, expected either a URI or a Dictionary.
||Invalid connection options, expected either a URI or a Dictionary.
||Invalid connection options, expected either a URI or a Dictionary.
