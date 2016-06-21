//@ CollectionCreateIndex: valid operations after createIndex
|All expected functions are available|
|No additional functions are available|

//@ CollectionAdd: valid operations after field
|All expected functions are available|
|No additional functions are available|

//@ CollectionAdd: valid operations after execute
|All expected functions are available|
|No additional functions are available|


// -----------------------------------
// Error conditions on chained methods
// -----------------------------------
//@# Error conditions on createIndex
||ArgumentError: Invalid number of arguments in CollectionCreateIndex.createIndex, expected 1 to 2 but got 0
||ArgumentError: CollectionCreateIndex.createIndex: Argument #1 is expected to be a string
||ArgumentError: CollectionCreateIndex.createIndex: Argument #2 is expected to be mysqlx.IndexType.UNIQUE

//@# Error conditions on field
||ArgumentError: Invalid number of arguments in CollectionCreateIndex.field, expected 3 but got 0
||ArgumentError: CollectionCreateIndex.field: Argument #1 is expected to be a string
||ArgumentError: CollectionCreateIndex.field: Argument #2 is expected to be a string
||ArgumentError: CollectionCreateIndex.field: Argument #3 is expected to be a bool


// -----------------------------------
// Execution tests
// -----------------------------------
//@ NonUnique Index: creation with required field
|John Records: 2|

//@ ERROR: insertion of document missing required field
||MySQL Error (5115): Document is missing a required field

//@ ERROR: attempt to create an index with the same name
||MySQL Error (1061): Duplicate key name '_name'

//@ ERROR: Attempt to create unique index when records already duplicate the key field
||MySQL Error (1062): Duplicate entry 'John' for key '_name'

//@ ERROR: Attempt to create unique index when records are missing the key field
||MySQL Error (5117): Collection contains document missing required field

//@ Unique index: creation with required field
||MySQL Error (5116): Document contains a field value that is not unique but required to be

//@ Cleanup
||