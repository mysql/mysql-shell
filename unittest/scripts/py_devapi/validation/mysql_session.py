#@ Session: validating members
|close: OK|
|create_schema: Missing|
|get_current_schema: Missing|
|get_default_schema: Missing|
|get_schema: Missing|
|get_schemas: Missing|
|get_uri: OK|
|set_current_schema: Missing|
|default_schema: Missing|
|uri: OK|
|current_schema: Missing|

#@<OUT> Session: help
Provides facilities to execute queries and retrieve database objects.

The following properties are currently supported.

 - uri Retrieves the URI for the current session.


The following functions are currently supported.

 - close             Closes the internal connection to the MySQL Server held on
                     this session object.
 - commit            Commits all the operations executed after a call to
                     startTransaction().
 - get_uri           Retrieves the URI for the current session.
 - help              Provides help about this class and it's members
 - is_open           Returns true if session is known to be open.
 - query             Executes a query and returns the corresponding
                     ClassicResult object.
 - rollback          Discards all the operations executed after a call to
                     startTransaction().
 - run_sql           Executes a query and returns the corresponding
                     ClassicResult object.
 - start_transaction Starts a transaction context on the server.


#@ ClassicSession: accessing Schemas
||unknown attribute: get_schemas

#@ ClassicSession: accessing individual schema
||unknown attribute: get_schema

#@ ClassicSession: accessing default schema
||unknown attribute: get_default_schema

#@ ClassicSession: accessing current schema
||unknown attribute: get_current_schema

#@ ClassicSession: create schema
||unknown attribute: create_schema

#@ ClassicSession: set current schema
||unknown attribute: set_current_schema

#@ ClassicSession: drop schema
||unknown attribute: drop_schema

#@Preparation for transaction tests
||

#@ ClassicSession: Transaction handling: rollback
|Inserted Documents: 0|

#@ ClassicSession: Transaction handling: commit
|Inserted Documents: 3|

#@ ClassicSession: date handling
|9999-12-31 23:59:59.999999|

#@# ClassicSession: run_sql errors
||Invalid number of arguments in ClassicSession.run_sql, expected 1 to 2 but got 0
||Invalid number of arguments in ClassicSession.run_sql, expected 1 to 2 but got 3
||ClassicSession.run_sql: Argument #1 is expected to be a string
||ClassicSession.run_sql: Argument #2 is expected to be an array
||ClassicSession.run_sql: Error formatting SQL query: more arguments than escapes while substituting placeholder value at index #2
||ClassicSession.run_sql: Insufficient number of values for placeholders in query


#@<OUT> ClassicSession: run_sql placeholders
| hello | 1234 |

#@# ClassicSession: query errors
||Invalid number of arguments in ClassicSession.query, expected 1 to 2 but got 0
||Invalid number of arguments in ClassicSession.query, expected 1 to 2 but got 3
||ClassicSession.query: Argument #1 is expected to be a string
||ClassicSession.query: Argument #2 is expected to be an array
||ClassicSession.query: Error formatting SQL query: more arguments than escapes while substituting placeholder value at index #2
||ClassicSession.query: Insufficient number of values for placeholders in query


#@<OUT> ClassicSession: query placeholders
| hello | 1234 |
