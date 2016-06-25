#@ mysqlx module: exports
|Exported Items: 7|
|getSession: <type 'builtin_function_or_method'>|
|getNodeSession: <type 'builtin_function_or_method'>|
|expr: <type 'builtin_function_or_method'>|
|dateValue: <type 'builtin_function_or_method'>|
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

#@ Stored Sessions, session from data dictionary
|<XSession:|
|Session using right URI|

#@ Stored Sessions, session from data dictionary removed
||IndexError: unknown attribute: mysqlx_data

#@ Stored Sessions, session from uri
|<XSession:|
|Session using right URI|

#@ Stored Sessions, session from uri removed
||IndexError: unknown attribute: mysqlx_uri


#@# mysqlx module: expression errors
||Invalid number of arguments in mysqlx.expr, expected 1 but got 0
||mysqlx.expr: Argument #1 is expected to be a string

#@ mysqlx module: expression
|<Expression>|
