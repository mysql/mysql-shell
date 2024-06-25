#@ create collection with wrong option
||ValueError: Invalid options in create collection options dictionary: vals


#@ create collection with wrong validation type
||TypeError: Option 'validation' is expected to be of type Map, but is String


#@ create collection with malformed validation object
||MySQL Error (5021): 'whatever' is not a valid option


#@ create collection with incorrect validation schema
||MySQL Error (5182): The provided validation schema is invalid (JSON validation schema location #/name failed requirement: 'additionalProperties' at meta schema location '#').


#@ create collection unsupported server
||MySQL Error (5015): The target MySQL server does not support options in the create_collection function
||MySQL Error (5015): The target MySQL server does not support options in the create_collection function

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
||ValueError: Missing values in modify collection options: validation


#@ modify collection with incorrect schema
||MySQL Error (5182): The provided validation schema is invalid (JSON validation schema location #/name failed requirement: 'additionalProperties' at meta schema location '#').


#@ modify collection with incorrect validation object
||MySQL Error (5021): 'whatever' is not a valid option

#@ modify collection validation only schema
|Query OK, 1 item affected|
|2|

#@<ERR> schema modification in effect
ERROR: 5180: Document is not valid according to the schema assigned to collection. The JSON document location '#/j' failed requirement 'type' at JSON Schema location '#/properties/j'.

#@ modify collection validation level only
|Query OK, 1 item affected|
|3|

#@ modify collection validation block fails because of data inconsistencies {VER(< 8.0.19)}
||MySQL Error (5157): The target MySQL server does not support the requested operation.


#@ modify collection validation block fails because of data inconsistencies {VER(>= 8.0.19)}
||MySQL Error (5180): Document is not valid according to the schema assigned to collection. The JSON document location '#/i' failed requirement 'type' at JSON Schema location '#/properties/i'.


