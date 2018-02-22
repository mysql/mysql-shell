//@ db.getTable() with %
||Schema.getTable: The table mysql.% does not exist (RuntimeError)

//@ db.getTable() with _
||Schema.getTable: The table mysql.____ does not exist (RuntimeError)

//@ db.getCollection() with %
||Schema.getCollection: The collection mysql.% does not exist (RuntimeError)

//@ db.getCollection() with _
||Schema.getCollection: The collection mysql.____ does not exist (RuntimeError)

//@ session.getSchema() with %
||Session.getSchema: Unknown database '%' (RuntimeError)

//@ session.getSchema() with _
||Session.getSchema: Unknown database '_____' (RuntimeError)

//@ create tables
|Query OK, 0 rows affected|
|Query OK, 0 rows affected|

//@ create collections
|<Collection:%c>|
|<Collection:d_>|

//@ create schemas
|<Schema:%e>|
|<Schema:f_>|

//@ db.getTable() with % (2)
||Schema.getTable: The table mysql.% does not exist (RuntimeError)

//@ db.getTable() with _ (2)
||Schema.getTable: The table mysql.__ does not exist (RuntimeError)

//@ db.getCollection() with % (2)
||Schema.getCollection: The collection mysql.% does not exist (RuntimeError)

//@ db.getCollection() with _ (2)
||Schema.getCollection: The collection mysql.__ does not exist (RuntimeError)

//@ session.getSchema() with % (2)
||Session.getSchema: Unknown database '%' (RuntimeError)

//@ session.getSchema() with _ (2)
||Session.getSchema: Unknown database '__' (RuntimeError)

//@ db.getTable() existing table with %
|<Table:%a>|
|true|

//@ db.getTable() existing table with _
|<Table:b_>|
|true|

//@ db.getCollection() existing collection with %
|<Collection:%c>|
|true|

//@ db.getCollection() existing collection with _
|<Collection:d_>|
|true|

//@ session.getSchema() existing schema with %
|<Schema:%e>|
|true|

//@ session.getSchema() existing schema with _
|<Schema:f_>|
|true|

//@ delete tables
|Query OK, 0 rows affected|
|Query OK, 0 rows affected|

//@ delete collections
||
||

//@ delete schemas
||
||

//@ check if all has been removed
|false|
|false|
|false|
|false|
|false|
|false|
