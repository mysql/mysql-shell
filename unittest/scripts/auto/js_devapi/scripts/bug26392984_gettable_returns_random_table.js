// connect and select schema
shell.connect(__uripwd + '/mysql');

//@ db.getTable() with %
db.getTable('%');

//@ db.getTable() with _
db.getTable('____');  // matches func

//@ db.getCollection() with %
db.getCollection('%');

//@ db.getCollection() with _
db.getCollection('____');

//@ session.getSchema() with %
session.getSchema('%');

//@ session.getSchema() with _
session.getSchema('_____');  // matches mysql

//@ create tables
session.sql("CREATE TABLE `%a` (c CHAR(1))").execute();
session.sql("CREATE TABLE `b_` (c CHAR(1))").execute();

//@ create collections
db.createCollection('%c');
db.createCollection('d_');

//@ create schemas
session.createSchema('%e');
session.createSchema('f_');

//@ db.getTable() with % (2)
db.getTable('%');

//@ db.getTable() with _ (2)
db.getTable('__');  // matches b_

//@ db.getCollection() with % (2)
db.getCollection('%');

//@ db.getCollection() with _ (2)
db.getCollection('__');  // matches d_

//@ session.getSchema() with % (2)
session.getSchema('%');

//@ session.getSchema() with _ (2)
session.getSchema('__');  // matches f_

//@ db.getTable() existing table with %
a = db.getTable('%a');
a.existsInDatabase();

//@ db.getTable() existing table with _
b = db.getTable('b_');
b.existsInDatabase();

//@ db.getCollection() existing collection with %
c = db.getCollection('%c');
c.existsInDatabase();

//@ db.getCollection() existing collection with _
d = db.getCollection('d_');
d.existsInDatabase();

//@ session.getSchema() existing schema with %
e = session.getSchema('%e');
e.existsInDatabase();

//@ session.getSchema() existing schema with _
f = session.getSchema('f_');
f.existsInDatabase();

//@ delete tables
session.sql("DROP TABLE `%a`").execute();
session.sql("DROP TABLE `b_`").execute();

//@ delete collections
db.dropCollection('%c');
db.dropCollection('d_');

//@ delete schemas
session.dropSchema('%e');
session.dropSchema('f_');

//@ check if all has been removed
a.existsInDatabase();
b.existsInDatabase();
c.existsInDatabase();
d.existsInDatabase();
e.existsInDatabase();
f.existsInDatabase();

// cleanup
session.close();
