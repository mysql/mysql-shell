var mysqlx = require('mysqlx').mysqlx;

//@ mysqlx type constants
print('Type.Bit:', mysqlx.Type.Bit);
print('Type.TinyInt:', mysqlx.Type.TinyInt);
print('Type.SmallInt:', mysqlx.Type.SmallInt);
print('Type.MediumInt:', mysqlx.Type.MediumInt);
print('Type.Int:', mysqlx.Type.Int);
print('Type.BigInt:', mysqlx.Type.BigInt);
print('Type.Float:', mysqlx.Type.Float);
print('Type.Decimal:', mysqlx.Type.Decimal);
print('Type.Double:', mysqlx.Type.Double);
print('Type.Json:', mysqlx.Type.Json);
print('Type.String:', mysqlx.Type.String);
print('Type.Bytes:', mysqlx.Type.Bytes);
print('Type.Time:', mysqlx.Type.Time);
print('Type.Date:', mysqlx.Type.Date);
print('Type.DateTime:', mysqlx.Type.DateTime);
print('Type.Timestamp:', mysqlx.Type.Timestamp);
print('Type.Set:', mysqlx.Type.Set);
print('Type.Enum:', mysqlx.Type.Enum);
print('Type.Geometry:', mysqlx.Type.Geometry);
print('Type.Set:', mysqlx.Type.Set);

//@ mysqlx index type constants
print('IndexType.Unique:', mysqlx.IndexType.Unique);
