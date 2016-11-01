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
||Invalid number of arguments in CollectionCreateIndex.createIndex, expected 1 to 2 but got 0
||CollectionCreateIndex.createIndex: Argument #1 is expected to be a string
||CollectionCreateIndex.createIndex: Argument #2 is expected to be mysqlx.IndexType.UNIQUE

//@# Error conditions on field
||Invalid number of arguments in CollectionCreateIndex.field, expected 3 but got 0
||CollectionCreateIndex.field: Argument #1 is expected to be a string
||CollectionCreateIndex.field: Argument #2 is expected to be a string
||CollectionCreateIndex.field: Argument #3 is expected to be a bool


// -----------------------------------
// Execution tests
// -----------------------------------
//@ NonUnique Index: creation with required field
|John Records: 2|

//@ ERROR: insertion of document missing required field
||Document is missing a required field

//@ ERROR: attempt to create an index with the same name
||Duplicate key name '_name'

//@ ERROR: Attempt to create unique index when records already duplicate the key field
||Duplicate entry 'John' for key '_name'

//@ ERROR: Attempt to create unique index when records are missing the key field
||Collection contains document missing required field

//@ Unique index: creation with required field
||Document contains a field value that is not unique but required to be

//@ Cleanup
||
