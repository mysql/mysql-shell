//@ CollectionRemove: valid operations after remove
|All expected functions are available|
|No additional functions are available|

//@ CollectionRemove: valid operations after sort
|All expected functions are available|
|No additional functions are available|

//@ CollectionRemove: valid operations after limit
|All expected functions are available|
|No additional functions are available|

//@ CollectionRemove: valid operations after bind
|All expected functions are available|
|No additional functions are available|

//@ CollectionRemove: valid operations after execute
|All expected functions are available|
|No additional functions are available|

//@ Reusing CRUD with binding
|Deleted donna: 1|
|Deleted alma: 1|

//@# CollectionRemove: Error conditions on remove
||Invalid number of arguments, expected 1 but got 0
||Requires a search condition.
||Argument #1 is expected to be a string
||Unterminated quoted string starting at position 8

//@# CollectionRemove: Error conditions sort
||Invalid number of arguments, expected at least 1 but got 0
||Argument #1 is expected to be a string or an array of strings
||Sort criteria can not be empty
||Element #2 is expected to be a string
||Argument #2 is expected to be a string

//@# CollectionRemove: Error conditions on limit
||Invalid number of arguments, expected 1 but got 0
||Argument #1 is expected to be an unsigned int

//@# CollectionRemove: Error conditions on bind
||Invalid number of arguments, expected 2 but got 0
||Argument #1 is expected to be a string
||Unable to bind value for unexisting placeholder: another

//@# CollectionRemove: Error conditions on execute
||Missing value bindings for the following placeholders: data, years
||Missing value bindings for the following placeholders: data

//@ CollectionRemove: remove under condition
|Affected Rows: 1|
|Records Left: 4|

//@ CollectionRemove: remove with binding
|Affected Rows: 2|
|Records Left: 2|


//@ CollectionRemove: full remove
|Affected Rows: 2|
|Records Left: 0|
