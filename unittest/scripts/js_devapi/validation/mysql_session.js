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
|schemas: OK|
|uri: OK|
|currentSchema: OK|
		

//@ ClassicSession: accessing Schemas
|<ClassicSchema:mysql>|
|<ClassicSchema:information_schema>|

//@ ClassicSession: accessing individual schema
|mysql|
|information_schema|

//@ ClassicSession: accessing schema through dynamic attributes
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
||MySQL Error (1007): Can't create database 'node_session_schema'; database exists

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
