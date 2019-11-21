//@ mysqlx module: exports
|Exported Items: 6|
|getSession: function|
|expr: function|
|dateValue: function|
|help: function|
|Type: <mysqlx.Type>|
|LockContention: <mysqlx.LockContention>|

//@# mysqlx module: expression errors
||mysqlx.expr: Invalid number of arguments, expected 1 but got 0
||mysqlx.expr: Argument #1 is expected to be a string

//@ mysqlx module: expression
|<Expression>|

//@ mysqlx module: dateValue() diffrent parameters
|2025-10-15 00:00:00|
|2017-12-10 10:10:10|
|2017-12-10 10:10:10.500000|
|2017-12-10 10:10:10.599000|

//@ mysqlx module: Bug #26429377
||mysqlx.dateValue: Invalid number of arguments, expected 3 but got 0 (ArgumentError)

//@ mysqlx module: Bug #26429377 - 4/5 arguments
||mysqlx.dateValue: Invalid number of arguments, expected 3 but got 4 (ArgumentError)

//@ mysqlx module: Bug #26429426
||mysqlx.dateValue: Valid day range is 0-31 (ArgumentError)

//@ month validation
||mysqlx.dateValue: Valid month range is 0-12 (ArgumentError)

//@ year validation
||mysqlx.dateValue: Valid year range is 0-9999 (ArgumentError)

//@ hour validation
||mysqlx.dateValue: Valid hour range is 0-23 (ArgumentError)

//@ minute validation
||mysqlx.dateValue: Valid minute range is 0-59 (ArgumentError)

//@ second validation
||mysqlx.dateValue: Valid second range is 0-59 (ArgumentError)

//@ usecond validation
||mysqlx.dateValue: Valid millisecond range is 0-999999 (ArgumentError)
