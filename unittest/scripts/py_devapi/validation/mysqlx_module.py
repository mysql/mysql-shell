#@ mysqlx module: exports
|Exported Items: 5|
|get_session: <type 'builtin_function_or_method'>|
|expr: <type 'builtin_function_or_method'>|
|dateValue: <type 'builtin_function_or_method'>|
|help: <type 'builtin_function_or_method'>|
|Type: <mysqlx.Type>|

#@# mysqlx module: expression errors
||TypeError: expr() takes 1 arguments (0 given)
||mysqlx.expr: Argument #1 is expected to be a string

#@ mysqlx module: expression
|<Expression>|

#@ mysqlx module: date_value() diffrent parameters
|2025-10-15|
|2017-12-10 10:10:10|
|2017-12-10 10:10:10.500000|
|2017-12-10 10:10:10.599999|

