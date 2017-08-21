#@ TableSelect: valid operations after select
|All expected functions are available|
|No additional functions are available|

#@ TableSelect: valid operations after where
|All expected functions are available|
|No additional functions are available|

#@ TableSelect: valid operations after group_by
|All expected functions are available|
|No additional functions are available|

#@ TableSelect: valid operations after having
|All expected functions are available|
|No additional functions are available|

#@ TableSelect: valid operations after order_by
|All expected functions are available|
|No additional functions are available|

#@ TableSelect: valid operations after limit
|All expected functions are available|
|No additional functions are available|

#@ TableSelect: valid operations after offset
|All expected functions are available|
|No additional functions are available|

#@ TableSelect: valid operations after bind
|All expected functions are available|
|No additional functions are available|

#@ TableSelect: valid operations after execute
|All expected functions are available|
|No additional functions are available|

#@ Reusing CRUD with binding
|adam|
|alma|


#@# TableSelect: Error conditions on select
||TableSelect.select: Argument #1 is expected to be a string or an array of strings
||TableSelect.select: Field selection criteria can not be empty
||TableSelect.select: Element #2 is expected to be a string
||TableSelect.select: Argument #2 is expected to be a string

#@# TableSelect: Error conditions on where
||Invalid number of arguments in TableSelect.where, expected 1 but got 0
||TableSelect.where: Argument #1 is expected to be a string
||TableSelect.where: Unterminated quoted string starting at position 8

#@# TableSelect: Error conditions on group_by
||Invalid number of arguments in TableSelect.group_by, expected at least 1 but got 0
||TableSelect.group_by: Argument #1 is expected to be a string or an array of strings
||TableSelect.group_by: Grouping criteria can not be empty
||TableSelect.group_by: Element #2 is expected to be a string
||TableSelect.group_by: Argument #2 is expected to be a string

#@# TableSelect: Error conditions on having
||Invalid number of arguments in TableSelect.having, expected 1 but got 0
||TableSelect.having: Argument #1 is expected to be a string

#@# TableSelect: Error conditions on order_by
||Invalid number of arguments in TableSelect.order_by, expected at least 1 but got 0
||TableSelect.order_by: Argument #1 is expected to be a string or an array of strings
||TableSelect.order_by: Order criteria can not be empty
||TableSelect.order_by: Element #2 is expected to be a string
||TableSelect.order_by: Argument #2 is expected to be a string

#@# TableSelect: Error conditions on limit
||Invalid number of arguments in TableSelect.limit, expected 1 but got 0
||TableSelect.limit: Argument #1 is expected to be an unsigned int

#@# TableSelect: Error conditions on offset
||Invalid number of arguments in TableSelect.offset, expected 1 but got 0
||TableSelect.offset: Argument #1 is expected to be an unsigned int

#@# TableSelect: Error conditions on bind
||Invalid number of arguments in TableSelect.bind, expected 2 but got 0
||TableSelect.bind: Argument #1 is expected to be a string
||TableSelect.bind: Unable to bind value for unexisting placeholder: another

#@# TableSelect: Error conditions on execute
||TableSelect.execute: Missing value bindings for the following placeholders: data, years
||TableSelect.execute: Missing value bindings for the following placeholders: data



#@ Table.Select All
|All: 7|

#@ Table.Select Filtering
|Males: 4|
|Females: 3|
|13 Years: 1|
|14 Years: 3|
|Under 17: 6|
|Names With A: 3|
|Names With A: 3|
|Not 14 Years: 4|

#@ Table.Select Field Selection
|1-Metadata Length: 6|
|1-Metadata Field: name|
|1-Metadata Field: age|

|2-Metadata Length: 5|
|2-Metadata Field: age|

#@ Table.Select Sorting
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

#@ Table.Select Limit and Offset
|Limit-Offset 0 : 4|
|Limit-Offset 1 : 4|
|Limit-Offset 2 : 4|
|Limit-Offset 3 : 4|
|Limit-Offset 4 : 3|
|Limit-Offset 5 : 2|
|Limit-Offset 6 : 1|
|Limit-Offset 7 : 0|


#@ Table.Select Parameter Binding through a View
|Select Binding Length: 1|
|Select Binding Name: alma|

#@ Table.Select Basic vertical output
|*************************** 1. row ***************************|
|name: adam|
|*************************** 2. row ***************************|
|name: alma|
|*************************** 3. row ***************************|
|name: angel|
|*************************** 4. row ***************************|
|name: brian|
|*************************** 5. row ***************************|
|name: carol|
|*************************** 6. row ***************************|
|name: donna|
|*************************** 7. row ***************************|
|name: jack|

#@ Table.Select Check column align in vertical mode
|*************************** 1. row ***************************|
|  name: donna|
|   age: 16|
|gender: female|
|*************************** 2. row ***************************|
|  name: jack|
|   age: 17|
|gender: male|

#@ Table.Select Check row with newline in vertical mode
|*************************** 1. row ***************************|
|  name: alma|
|   age: 13|
|gender: female|
|*************************** 2. row ***************************|
|  name: john|
|doe|
|   age: 13|
|gender: male|

#@ Table.Select Check switching between table and vertical mode
|*************************** 1. row ***************************|
|name: jack|
|+------+|
|| name ||
|+------+|
|| jack ||
|+------+|

#@ Table.Select Zerofill field as variable
|Variable value : 1|
|Variable value : 12|
|Variable value : 12345|
|Variable value : 123456789|

#@ Table.Select Zerofill field display
|+-----------+|
|value|
|+-----------+|
|00001|
|00012|
|12345|
|123456789|
|+-----------+|

