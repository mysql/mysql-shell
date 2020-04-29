#@ create collection with wrong option
||SystemError: ArgumentError: Schema.create_collection: Invalid options in create collection options dictionary: vals


#@ create collection with wrong validation type
||SystemError: TypeError: Schema.create_collection: Option 'validation' is expected to be of type Map, but is String


#@ create collection with malformed validation object
||SystemError: RuntimeError: Schema.create_collection: 'whatever' is not a valid option


#@ create collection with incorrect validation schema
||SystemError: RuntimeError: Schema.create_collection: The provided validation schema is invalid (JSON validation schema location #/name failed requirement: 'additionalProperties' at meta schema location '#').


#@ create collection unsupported server
||SystemError: RuntimeError: Schema.create_collection: The target MySQL server does not support options in the create_collection function
||SystemError: RuntimeError: Schema.create_collection: The target MySQL server does not support options in the create_collection function

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
||SystemError: ArgumentError: Schema.modify_collection: Missing values in modify collection options: validation


#@ modify collection with incorrect schema
||SystemError: RuntimeError: Schema.modify_collection: The provided validation schema is invalid (JSON validation schema location #/name failed requirement: 'additionalProperties' at meta schema location '#').


#@ modify collection with incorrect validation object
||SystemError: RuntimeError: Schema.modify_collection: 'whatever' is not a valid option

#@ modify collection validation only schema
|Query OK, 1 item affected|
|2|

#@<ERR> schema modification in effect
ERROR: 5180: Document is not valid according to the schema assigned to collection. The JSON document location '#/j' failed requirement 'type' at JSON Schema location '#/properties/j'.

#@ modify collection validation level only
|Query OK, 1 item affected|
|3|

#@ modify collection validation block fails because of data inconsistencies {VER(< 8.0.19)}
||SystemError: RuntimeError: Schema.modify_collection: The target MySQL server does not support the requested operation.


#@ modify collection validation block fails because of data inconsistencies {VER(>= 8.0.19)}
||SystemError: RuntimeError: Schema.modify_collection: Document is not valid according to the schema assigned to collection. The JSON document location '#/i' failed requirement 'type' at JSON Schema location '#/properties/i'.


