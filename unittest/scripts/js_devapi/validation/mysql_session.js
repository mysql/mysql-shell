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
