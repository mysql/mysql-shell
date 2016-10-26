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