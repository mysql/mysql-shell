#@<OUT> mysqlx module: exports
Exported Items: 6
get_session: <class 'builtin_function_or_method'>

expr: <class 'builtin_function_or_method'>

dateValue: <class 'builtin_function_or_method'>

help: <class 'builtin_function_or_method'>

Type: <mysqlx.Type>

LockContention: <mysqlx.LockContention>

#@# mysqlx module: expression errors
||mysqlx.expr: Invalid number of arguments, expected 1 but got 0
||mysqlx.expr: Argument #1 is expected to be a string

#@ mysqlx module: expression
|<Expression>|

#@ mysqlx module: date_value() diffrent parameters
|2025-10-15|
|2017-12-10 10:10:10|
|2017-12-10 10:10:10.500000|
|2017-12-10 10:10:10.599999|

