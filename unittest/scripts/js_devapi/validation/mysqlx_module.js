//@ mysqlx module: exports
|Exported Items: 3|
|getSession: function|
|getNodeSession: function|
|expr: function|


//@ mysqlx module: getSession through URI
|<XSession:|
|Session using right URI|

//@ mysqlx module: getSession through URI and password
|<XSession:|
|Session using right URI|

//@ mysqlx module: getSession through data
|<XSession:|
|Session using right URI|

//@ mysqlx module: getSession through data and password
|<XSession:|
|Session using right URI|


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