//@ Session: validating members
|close: OK|
|createSchema: Missing|
|getCurrentSchema: Missing|
|getDefaultSchema: Missing|
|getSchema: Missing|
|getSchemas: Missing|
|getUri: OK|
|setCurrentSchema: Missing|
|defaultSchema: Missing|
|uri: OK|
|currentSchema: Missing|

//@<OUT> Session: help
Provides facilities to execute queries and retrieve database objects.

The following properties are currently supported.

 - uri Retrieves the URI for the current session.


The following functions are currently supported.

 - close            Closes the internal connection to the MySQL Server held on
                    this session object.
 - commit           Commits all the operations executed after a call to
                    startTransaction().
 - getUri           Retrieves the URI for the current session.
 - help             Provides help about this class and it's members
 - isOpen           Returns true if session is known to be open.
 - query            Executes a query and returns the corresponding
                    ClassicResult object.
 - rollback         Discards all the operations executed after a call to
                    startTransaction().
 - runSql           Executes a query and returns the corresponding
                    ClassicResult object.
 - startTransaction Starts a transaction context on the server.



//@ ClassicSession: accessing Schemas
||Invalid object member getSchemas

//@ ClassicSession: accessing individual schema
||Invalid object member getSchema

//@ ClassicSession: accessing default schema
||Invalid object member getDefaultSchema

//@ ClassicSession: accessing current schema
||Invalid object member getCurrentSchema

//@ ClassicSession: create schema
||Invalid object member createSchema

//@ ClassicSession: set current schema
||Invalid object member setCurrentSchema

//@ ClassicSession: drop schema
||Invalid object member dropSchema

//@Preparation for transaction tests
||

//@ ClassicSession: Transaction handling: rollback
|Inserted Documents: 0|

//@ ClassicSession: Transaction handling: commit
|Inserted Documents: 3|

//@ ClassicSession: date handling
|9999-12-31 23:59:59.999999|

//@# ClassicSession: runSql errors
||Invalid number of arguments in ClassicSession.runSql, expected 1 to 2 but got 0
||Invalid number of arguments in ClassicSession.runSql, expected 1 to 2 but got 3
||ClassicSession.runSql: Argument #1 is expected to be a string
||ClassicSession.runSql: Argument #2 is expected to be an array
||ClassicSession.runSql: Error formatting SQL query: more arguments than escapes while substituting placeholder value at index #2
||ClassicSession.runSql: Insufficient number of values for placeholders in query


//@<OUT> ClassicSession: runSql placeholders
| hello | 1234 |

//@# ClassicSession: query errors
||Invalid number of arguments in ClassicSession.query, expected 1 to 2 but got 0
||Invalid number of arguments in ClassicSession.query, expected 1 to 2 but got 3
||ClassicSession.query: Argument #1 is expected to be a string
||ClassicSession.query: Argument #2 is expected to be an array
||ClassicSession.query: Error formatting SQL query: more arguments than escapes while substituting placeholder value at index #2
||ClassicSession.query: Insufficient number of values for placeholders in query


//@<OUT> ClassicSession: query placeholders
| hello | 1234 |
