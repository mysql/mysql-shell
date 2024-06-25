//@ mysqlx module: exports
|Exported Items: 6|
|getSession: function|
|expr: function|
|dateValue: function|
|help: function|
|Type: <mysqlx.Type>|
|LockContention: <mysqlx.LockContention>|

//@# mysqlx module: expression errors
||Invalid number of arguments, expected 1 but got 0
||Argument #1 is expected to be a string

//@ mysqlx module: expression
|<Expression>|

//@ mysqlx module: dateValue() diffrent parameters
|2025-10-15 00:00:00|
|2017-12-10 10:10:10|
|2017-12-10 10:10:10.500000|
|2017-12-10 10:10:10.599000|

//@ mysqlx module: Bug #26429377
||Invalid number of arguments, expected 3 but got 0 (ArgumentError)

//@ mysqlx module: Bug #26429377 - 4/5 arguments
||Invalid number of arguments, expected 3 but got 4 (ArgumentError)

//@ mysqlx module: Bug #26429426
||Valid day range is 0-31 (ArgumentError)

//@ month validation
||Valid month range is 0-12 (ArgumentError)

//@ year validation
||Valid year range is 0-9999 (ArgumentError)

//@ hour validation
||Valid hour range is 0-23 (ArgumentError)

//@ minute validation
||Valid minute range is 0-59 (ArgumentError)

//@ second validation
||Valid second range is 0-59 (ArgumentError)

//@ usecond validation
||Valid millisecond range is 0-999999 (ArgumentError)
