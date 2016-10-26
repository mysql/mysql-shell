#@ mysqlx module: exports
|Exported Items: 7|
|get_session: <type 'builtin_function_or_method'>|
|get_node_session: <type 'builtin_function_or_method'>|
|expr: <type 'builtin_function_or_method'>|
|dateValue: <type 'builtin_function_or_method'>|
|help: <type 'builtin_function_or_method'>|
|Type: <mysqlx.Type>|
|IndexType: <mysqlx.IndexType>|


#@ mysqlx module: get_session through URI
|<XSession:|
|Session using right URI|

#@ mysqlx module: get_session through URI and password
|<XSession:|
|Session using right URI|

#@ mysqlx module: get_session through data
|<XSession:|
|Session using right URI|

#@ mysqlx module: get_session through data and password
|<XSession:|
|Session using right URI|


#@ mysqlx module: get_node_session through URI
|<NodeSession:|
|Session using right URI|

#@ mysqlx module: get_node_session through URI and password
|<NodeSession:|
|Session using right URI|

#@ mysqlx module: get_node_session through data
|<NodeSession:|
|Session using right URI|

#@ mysqlx module: get_node_session through data and password
|<NodeSession:|
|Session using right URI|

#@# mysqlx module: expression errors
||Invalid number of arguments in mysqlx.expr, expected 1 but got 0
||mysqlx.expr: Argument #1 is expected to be a string

#@ mysqlx module: expression
|<Expression>|
