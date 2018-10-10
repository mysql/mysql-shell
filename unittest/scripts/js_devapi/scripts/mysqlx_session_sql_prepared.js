shell.connect(__uripwd);
session.dropSchema('prepared_stmt');
var schema = session.createSchema('prepared_stmt');
session.sql("use prepared_stmt");
session.sql("create table test_table (id integer, name varchar(20), age integer)");
var table = schema.getTable('test_table');

table.insert().values(1, "george", 18).execute();
table.insert().values(2, "james", 17).execute();
table.insert().values(3, "luke", 18).execute();

//@ First execution is normal
var sql = session.sql('select * from test_table where name like ?');
sql.bind('g%').execute();

//@ Second execution prepares statement and executes it
sql.bind('j%').execute();

//@ Third execution uses prepared statement
sql.bind('l%').execute();

//@<> Finalizing
session.dropSchema('prepared_stmt');
session.close();
