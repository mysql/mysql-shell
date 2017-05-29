//@ mysqlx module: exports
|Exported Items: 6|
|getSession: object|
|expr: object|
|dateValue: object|
|help: object|
|Type: <mysqlx.Type>|
|IndexType: <mysqlx.IndexType>|

//@ mysqlx module: getSession through URI
|<Session:|
|Session using right URI|

//@ mysqlx module: getSession through URI and password
|<Session:|
|Session using right URI|

//@ mysqlx module: getSession through data
|<Session:|
|Session using right URI|

//@ mysqlx module: getSession through data and password
|<Session:|
|Session using right URI|

//@# mysqlx module: expression errors
||Invalid number of arguments in mysqlx.expr, expected 1 but got 0
||mysqlx.expr: Argument #1 is expected to be a string

//@ mysqlx module: expression
|<Expression>|
