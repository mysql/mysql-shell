//@ TableSelect: valid operations after select
|All expected functions are available|
|No additional functions are available|

//@ TableSelect: valid operations after where
|All expected functions are available|
|No additional functions are available|

//@ TableSelect: valid operations after groupBy
|All expected functions are available|
|No additional functions are available|

//@ TableSelect: valid operations after having
|All expected functions are available|
|No additional functions are available|

//@ TableSelect: valid operations after orderBy
|All expected functions are available|
|No additional functions are available|

//@ TableSelect: valid operations after limit
|All expected functions are available|
|No additional functions are available|

//@ TableSelect: valid operations after offset
|All expected functions are available|
|No additional functions are available|

//@ TableSelect: valid operations after bind
|All expected functions are available|
|No additional functions are available|

//@ TableSelect: valid operations after execute
|All expected functions are available|
|No additional functions are available|

//@ Reusing CRUD with binding
|adam|
|alma|


//@# TableSelect: Error conditions on select
||TableSelect.select: Argument #1 is expected to be an array
||TableSelect.select: Field selection criteria can not be empty
||TableSelect.select: Element #2 is expected to be a string

//@# TableSelect: Error conditions on where
||Invalid number of arguments in TableSelect.where, expected 1 but got 0
||TableSelect.where: Argument #1 is expected to be a string
||TableSelect.where: Unterminated quoted string starting at 8

//@# TableSelect: Error conditions on groupBy
||Invalid number of arguments in TableSelect.groupBy, expected 1 but got 0
||TableSelect.groupBy: Argument #1 is expected to be an array
||TableSelect.groupBy: Grouping criteria can not be empty
||TableSelect.groupBy: Element #2 is expected to be a string

//@# TableSelect: Error conditions on having
||Invalid number of arguments in TableSelect.having, expected 1 but got 0
||TableSelect.having: Argument #1 is expected to be a string

//@# TableSelect: Error conditions on orderBy
||Invalid number of arguments in TableSelect.orderBy, expected 1 but got 0
||TableSelect.orderBy: Argument #1 is expected to be an array
||TableSelect.orderBy: Order criteria can not be empty
||TableSelect.orderBy: Element #2 is expected to be a string

//@# TableSelect: Error conditions on limit
||Invalid number of arguments in TableSelect.limit, expected 1 but got 0
||TableSelect.limit: Argument #1 is expected to be an unsigned int

//@# TableSelect: Error conditions on offset
||Invalid number of arguments in TableSelect.offset, expected 1 but got 0
||TableSelect.offset: Argument #1 is expected to be an unsigned int

//@# TableSelect: Error conditions on bind
||Invalid number of arguments in TableSelect.bind, expected 2 but got 0
||TableSelect.bind: Argument #1 is expected to be a string
||TableSelect.bind: Unable to bind value for unexisting placeholder: another

//@# TableSelect: Error conditions on execute
||TableSelect.execute: Missing value bindings for the next placeholders: data, years
||TableSelect.execute: Missing value bindings for the next placeholders: data



//@ Table.Select All
|All: 7|

//@ Table.Select Filtering
|Males: 4|
|Females: 3|
|13 Years: 1|
|14 Years: 3|
|Under 17: 6|
|Names With A: 3

//@ Table.Select Field Selection
|1-Metadata Length: 4|
|1-Metadata Field: name|
|1-Metadata Field: age|

|2-Metadata Length: 3|
|2-Metadata Field: age|

//@ Table.Select Sorting
|Select Asc 0 : adam|
|Select Asc 1 : alma|
|Select Asc 2 : angel|
|Select Asc 3 : brian|
|Select Asc 4 : carol|
|Select Asc 5 : donna|
|Select Asc 6 : jack|
|Select Desc 0 : jack|
|Select Desc 1 : donna|
|Select Desc 2 : carol|
|Select Desc 3 : brian|
|Select Desc 4 : angel|
|Select Desc 5 : alma|
|Select Desc 6 : adam|

//@ Table.Select Limit and Offset
|Limit-Offset 0 : 4|
|Limit-Offset 1 : 4|
|Limit-Offset 2 : 4|
|Limit-Offset 3 : 4|
|Limit-Offset 4 : 3|
|Limit-Offset 5 : 2|
|Limit-Offset 6 : 1|
|Limit-Offset 7 : 0|


//@ Table.Select Parameter Binding
|Select Binding Length: 1|
|Select Binding Name: alma|
