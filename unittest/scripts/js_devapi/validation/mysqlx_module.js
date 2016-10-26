//@ mysqlx module: exports
|Exported Items: 6|
|getNodeSession: object|
|expr: object|
|dateValue: object|
|help: object|
|Type: <mysqlx.Type>|
|IndexType: <mysqlx.IndexType>|

//@ mysqlx module: getNodeSession through URI
|<NodeSession:|
|Session using right URI|

//@ mysqlx module: getNodeSession through URI and password
|<NodeSession:|
|Session using right URI|

//@ mysqlx module: getNodeSession through data
|<NodeSession:|
|Session using right URI|

//@ mysqlx module: getNodeSession through data and password
|<NodeSession:|
|Session using right URI|

//@# mysqlx module: expression errors
||Invalid number of arguments in mysqlx.expr, expected 1 but got 0
||mysqlx.expr: Argument #1 is expected to be a string

//@ mysqlx module: expression
|<Expression>|
