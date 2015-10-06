//@ Session: validating members
|close: OK|
|createSchema: OK|
|getDefaultSchema: OK|
|getSchema: OK|
|getSchemas: OK|
|getUri: OK|
|setFetchWarnings: OK|
|defaultSchema: OK|
|schemas: OK|
|uri: OK|
		
//@ Session: accessing Schemas
|<Schema:mysql>|
|<Schema:information_schema>|

//@ Session: accessing individual schema
|mysql|
|information_schema|

//@ Session: accessing schema through dynamic attributes
|mysql|
|information_schema|

//@ Session: accessing unexisting schema
||Unknown database 'unexisting_schema'

//@ Session: create schema success
|<Schema:session_schema>|

//@ Session: create schema failure
||MySQL Error (1007): Can't create database 'session_schema'; database exists

//@ Session: create quoted schema
|<Schema:quoted schema>|

//@ Session: Transaction handling: rollback
|Inserted Documents: 0|

//@ Session: Transaction handling: commit
|Inserted Documents: 3|

//@ NodeSession: validating members
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
|schemas: OK|
|uri: OK|
|currentSchema: OK|		

//@ NodeSession: accessing Schemas
|<Schema:mysql>|
|<Schema:information_schema>|

//@ NodeSession: accessing individual schema
|mysql|
|information_schema|

//@ NodeSession: accessing schema through dynamic attributes
|mysql|
|information_schema|

//@ NodeSession: accessing unexisting schema
||Unknown database 'unexisting_schema'

//@ NodeSession: current schema validations: nodefault
|null|
|null|

//@ NodeSession: create schema success
|<Schema:node_session_schema>|

//@ NodeSession: create schema failure
||MySQL Error (1007): Can't create database 'node_session_schema'; database exists

//@ NodeSession: Transaction handling: rollback
|Inserted Documents: 0|

//@ NodeSession: Transaction handling: commit
|Inserted Documents: 3|

//@ NodeSession: current schema validations: nodefault, mysql
|null|
|<Schema:mysql>|

//@ NodeSession: current schema validations: nodefault, information_schema
|null|
|<Schema:information_schema>|

//@ NodeSession: current schema validations: default
|<Schema:mysql>|
|<Schema:mysql>|

//@ NodeSession: current schema validations: default, information_schema
|<Schema:mysql>|
|<Schema:information_schema>|

//@ NodeSession: setFetchWarnings(false)
|0|

//@ NodeSession: setFetchWarnings(true)
|1|
|Can't drop database 'unexisting'; database doesn't exist|

//@ NodeSession: quoteName no parameters
||ArgumentError: Invalid number of arguments in NodeSession.quoteName, expected 1 but got 0

//@ NodeSession: quoteName wrong param type
||TypeError: Argument #1 is expected to be a string

//@ NodeSession: quoteName with correct parameters
|`sample`|
|`sam``ple`|
|```sample```|
|```sample`|
|`sample```|