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
||MySQL Error (1007): Can't create database 'node_session_schema'; database exists

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
