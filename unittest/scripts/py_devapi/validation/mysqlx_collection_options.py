#@ create collection with wrong option
||ValueError: Schema.create_collection: Invalid options in create collection options dictionary: vals


#@ create collection with wrong validation type
||TypeError: Schema.create_collection: Option 'validation' is expected to be of type Map, but is String


#@ create collection with malformed validation object
||mysqlsh.DBError: MySQL Error (5021): Schema.create_collection: 'whatever' is not a valid option


#@ create collection with incorrect validation schema
||mysqlsh.DBError: MySQL Error (5182): Schema.create_collection: The provided validation schema is invalid (JSON validation schema location #/name failed requirement: 'additionalProperties' at meta schema location '#').


#@ create collection unsupported server
||mysqlsh.DBError: MySQL Error (5015): Schema.create_collection: The target MySQL server does not support options in the create_collection function
||mysqlsh.DBError: MySQL Error (5015): Schema.create_collection: The target MySQL server does not support options in the create_collection function

#@<OUT> create collection with validation block
0
0

#@<ERR> validation negative case
ERROR: 5180: Document is not valid according to the schema assigned to collection. The JSON document location '#' failed requirement 'required' at JSON Schema location '#'.
ERROR: 5180: Document is not valid according to the schema assigned to collection. The JSON document location '#/i' failed requirement 'type' at JSON Schema location '#/properties/i'.

#@ validation positive case
|Query OK, 1 item affected|
|1|

#@<ERR> reuseExistingObject
ERROR: 5180: Document is not valid according to the schema assigned to collection. The JSON document location '#' failed requirement 'required' at JSON Schema location '#'.

#@ modify collection with empty options
||ValueError: Schema.modify_collection: Missing values in modify collection options: validation


#@ modify collection with incorrect schema
||mysqlsh.DBError: MySQL Error (5182): Schema.modify_collection: The provided validation schema is invalid (JSON validation schema location #/name failed requirement: 'additionalProperties' at meta schema location '#').


#@ modify collection with incorrect validation object
||mysqlsh.DBError: MySQL Error (5021): Schema.modify_collection: 'whatever' is not a valid option

#@ modify collection validation only schema
|Query OK, 1 item affected|
|2|

#@<ERR> schema modification in effect
ERROR: 5180: Document is not valid according to the schema assigned to collection. The JSON document location '#/j' failed requirement 'type' at JSON Schema location '#/properties/j'.

#@ modify collection validation level only
|Query OK, 1 item affected|
|3|

#@ modify collection validation block fails because of data inconsistencies {VER(< 8.0.19)}
||mysqlsh.DBError: MySQL Error (5157): Schema.modify_collection: The target MySQL server does not support the requested operation.


#@ modify collection validation block fails because of data inconsistencies {VER(>= 8.0.19)}
||mysqlsh.DBError: MySQL Error (5180): Schema.modify_collection: Document is not valid according to the schema assigned to collection. The JSON document location '#/i' failed requirement 'type' at JSON Schema location '#/properties/i'.


