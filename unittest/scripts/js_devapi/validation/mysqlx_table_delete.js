//@ TableDelete: valid operations after delete
|All expected functions are available|
|No additional functions are available|

//@ TableDelete: valid operations after where
|All expected functions are available|
|No additional functions are available|

//@ TableDelete: valid operations after orderBy
|All expected functions are available|
|No additional functions are available|

//@ TableDelete: valid operations after limit
|All expected functions are available|
|No additional functions are available|

//@ TableDelete: valid operations after bind
|All expected functions are available|
|No additional functions are available|

//@ TableDelete: valid operations after execute
|All expected functions are available|
|No additional functions are available|

//@ Reusing CRUD with binding
|Deleted donna: 1|
|Deleted alma: 1|


//@# TableDelete: Error conditions on delete
||TableDelete.delete: Invalid number of arguments, expected 0 but got 1

//@# TableDelete: Error conditions on where
||TableDelete.where: Invalid number of arguments, expected 1 but got 0
||TableDelete.where: Argument #1 is expected to be a string
||TableDelete.where: Unterminated quoted string starting at position 8

//@# TableDelete: Error conditions on orderBy
||TableDelete.orderBy: Invalid number of arguments, expected at least 1 but got 0
||TableDelete.orderBy: Argument #1 is expected to be a string or an array of strings
||TableDelete.orderBy: Order criteria can not be empty
||TableDelete.orderBy: Element #2 is expected to be a string
||TableDelete.orderBy: Argument #2 is expected to be a string

//@# TableDelete: Error conditions on limit
||TableDelete.limit: Invalid number of arguments, expected 1 but got 0
||TableDelete.limit: Argument #1 is expected to be an unsigned int

//@# TableDelete: Error conditions on bind
||TableDelete.bind: Invalid number of arguments, expected 2 but got 0
||TableDelete.bind: Argument #1 is expected to be a string
||TableDelete.bind: Unable to bind value for unexisting placeholder: another

//@# TableDelete: Error conditions on execute
||TableDelete.execute: Missing value bindings for the following placeholders: data, years
||TableDelete.execute: Missing value bindings for the following placeholders: data

//@ TableDelete: delete under condition
|Affected Rows: 1|
|Records Left: 4|

//@ TableDelete: delete with binding
|Affected Rows: 2|
|Records Left: 2|


//@ TableDelete: full delete with a view object
|Affected Rows: 2|
|Records Left: 0|

//@ TableDelete: with limit 0
|Records Left: 5|

//@ TableDelete: with limit 1
|Affected Rows: 2|
|Records Left: 3|

//@ TableDelete: with limit 2
|Affected Rows: 2|
|Records Left: 1|

//@ TableDelete: with limit 3
|Affected Rows: 1|
|Records Left: 0|
