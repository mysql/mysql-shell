#@<ERR> create collection with wrong option
Traceback (most recent call last):
  File "<string>", line 2, in <module>
SystemError: ArgumentError: Schema.create_collection: Invalid options in create collection options dictionary: vals


#@<ERR> create collection with wrong validation type
Traceback (most recent call last):
  File "<string>", line 2, in <module>
SystemError: TypeError: Schema.create_collection: Option 'validation' is expected to be of type Map, but is String


#@<ERR> create collection with malformed validation object
Traceback (most recent call last):
  File "<string>", line 1, in <module>
SystemError: RuntimeError: Schema.create_collection: 'whatever' is not a valid option


#@<ERR> create collection with incorrect validation schema
Traceback (most recent call last):
  File "<string>", line 1, in <module>
SystemError: RuntimeError: Schema.create_collection: The provided validation schema is invalid (JSON validation schema location #/name failed requirement: 'additionalProperties' at meta schema location '#').


#@<ERR> create collection unsupported server
Traceback (most recent call last):
  File "<string>", line 17, in <module>
SystemError: RuntimeError: Schema.create_collection: The target MySQL server does not support options in the create_collection function

Traceback (most recent call last):
  File "<string>", line 1, in <module>
SystemError: RuntimeError: Schema.create_collection: The target MySQL server does not support options in the create_collection function

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

#@<ERR> modify collection with empty options
Traceback (most recent call last):
  File "<string>", line 1, in <module>
SystemError: ArgumentError: Schema.modify_collection: Missing values in modify collection options: validation


#@<ERR> modify collection with incorrect schema
Traceback (most recent call last):
  File "<string>", line 1, in <module>
SystemError: RuntimeError: Schema.modify_collection: The provided validation schema is invalid (JSON validation schema location #/name failed requirement: 'additionalProperties' at meta schema location '#').


#@<ERR> modify collection with incorrect validation object
Traceback (most recent call last):
  File "<string>", line 1, in <module>
SystemError: RuntimeError: Schema.modify_collection: 'whatever' is not a valid option

#@ modify collection validation only schema
|Query OK, 1 item affected|
|2|

#@<ERR> schema modification in effect
ERROR: 5180: Document is not valid according to the schema assigned to collection. The JSON document location '#/j' failed requirement 'type' at JSON Schema location '#/properties/j'.

#@ modify collection validation level only
|Query OK, 1 item affected|
|3|

#@<ERR> modify collection validation block fails because of data inconsistencies {VER(< 8.0.19)}
Traceback (most recent call last):
  File "<string>", line 14, in <module>
SystemError: RuntimeError: Schema.modify_collection: The target MySQL server does not support the requested operation.


#@<ERR> modify collection validation block fails because of data inconsistencies {VER(>= 8.0.19)}
Traceback (most recent call last):
  File "<string>", line 14, in <module>
SystemError: RuntimeError: Schema.modify_collection: Document is not valid according to the schema assigned to collection. The JSON document location '#/i' failed requirement 'type' at JSON Schema location '#/properties/i'.


