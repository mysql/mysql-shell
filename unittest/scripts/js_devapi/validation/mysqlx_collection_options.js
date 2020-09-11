//@<OUT> __global__
<Schema:js_shell_test>

//@<ERR> create collection with wrong option
Schema.createCollection: Invalid options in create collection options dictionary: vals (ArgumentError)

//@<ERR> create collection with wrong validation type
Schema.createCollection: Option 'validation' is expected to be of type Map, but is String (TypeError)

//@<ERR> create collection with malformed validation object
Schema.createCollection: 'whatever' is not a valid option (MYSQLSH 5021)

//@<ERR> create collection with incorrect validation schema {VER(>= 8.0.19)}
Schema.createCollection: The provided validation schema is invalid (JSON validation schema location #/name failed requirement: 'additionalProperties' at meta schema location '#'). (MYSQLSH 5182)

//@<ERR> create collection unsupported server
Schema.createCollection: The target MySQL server does not support options in the createCollection function (MYSQLSH 5015)
Schema.createCollection: The target MySQL server does not support options in the createCollection function (MYSQLSH 5015)

//@<OUT> create collection with validation block
0
0

//@<ERR> validation negative case
ERROR: 5180: Document is not valid according to the schema assigned to collection. The JSON document location '#' failed requirement 'required' at JSON Schema location '#'.
ERROR: 5180: Document is not valid according to the schema assigned to collection. The JSON document location '#/i' failed requirement 'type' at JSON Schema location '#/properties/i'.

//@ validation positive case
|Query OK, 1 item affected|
|1|

//@<ERR> reuseExistingObject
ERROR: 5180: Document is not valid according to the schema assigned to collection. The JSON document location '#' failed requirement 'required' at JSON Schema location '#'.

//@<ERR> modify collection with empty options
Schema.modifyCollection: Missing values in modify collection options: validation (ArgumentError)

//@<ERR> modify collection with incorrect schema
Schema.modifyCollection: The provided validation schema is invalid (JSON validation schema location #/name failed requirement: 'additionalProperties' at meta schema location '#'). (MYSQLSH 5182)

//@<ERR> modify collection with incorrect validation object
Schema.modifyCollection: 'whatever' is not a valid option (MYSQLSH 5021)

//@ modify collection validation only schema
|Query OK, 1 item affected|
|2|

//@<ERR> schema modification in effect
ERROR: 5180: Document is not valid according to the schema assigned to collection. The JSON document location '#/j' failed requirement 'type' at JSON Schema location '#/properties/j'.

//@ modify collection validation level only
|Query OK, 1 item affected|
|3|

//@<ERR> modify collection validation block fails because of data inconsistencies {VER(< 8.0.19)}
Schema.modifyCollection: The target MySQL server does not support the requested operation. (MYSQLSH 5157)

//@<ERR> modify collection validation block fails because of data inconsistencies {VER(>= 8.0.19)}
Schema.modifyCollection: Document is not valid according to the schema assigned to collection. The JSON document location '#/i' failed requirement 'type' at JSON Schema location '#/properties/i'. (MYSQLSH 5180)

