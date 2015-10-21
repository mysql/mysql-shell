//@ mysqlx module: exports
|Exported Items: 25|
|getSession: function|
|getNodeSession: function|
|expr: function|
|Varchar: function|
|Char: function|
|Decimal: function|
|<DataTypes.TinyInt>|
|<DataTypes.SmallInt>|
|<DataTypes.MediumInt>|
|<DataTypes.Int>|
|<DataTypes.Integer>|
|<DataTypes.BigInt>|
|<DataTypes.Real>|
|<DataTypes.Float>|
|<DataTypes.Double>|
|<DataTypes.Numeric>|
|<DataTypes.Date>|
|<DataTypes.Time>|
|<DataTypes.Timestamp>|
|<DataTypes.DateTime>|
|<DataTypes.Year>|
|<DataTypes.Bit>|
|<DataTypes.Blob>|
|<DataTypes.Text>|
|<IndexTypes.IndexUnique>|
|<DataTypes.Varchar>|
|<DataTypes.Varchar(10)>|
|<DataTypes.Char>|
|<DataTypes.Char(10)>|
|<DataTypes.Decimal>|
|<DataTypes.Decimal(10)>|
|<DataTypes.Decimal(10,3)>|
|<DataTypes.Numeric>|
|<DataTypes.Numeric(10)>|
|<DataTypes.Numeric(10,3)>|




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

//@ Server Registry, session from data dictionary
|<XSession:|
|Session using right URI|

//@ Server Registry, session from data dictionary removed
||AttributeError: Invalid object member mysqlx_data

//@ Server Registry, session from uri
|<XSession:|
|Session using right URI|

//@ Server Registry, session from uri removed
||AttributeError: Invalid object member mysqlx_uri

//@# mysqlx module: expression errors
||Invalid number of arguments in mysqlx.expr, expected 1 but got 0
||mysqlx.expr: Argument #1 is expected to be a string

//@ mysqlx module: expression
|<Expression>|