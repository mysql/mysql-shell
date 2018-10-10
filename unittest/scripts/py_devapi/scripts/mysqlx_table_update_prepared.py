shell.connect(__uripwd);
session.drop_schema('prepared_stmt');
schema = session.create_schema('prepared_stmt');
session.sql("use prepared_stmt");
session.sql("create table test_table (id integer, name varchar(20), age integer)");
table = schema.get_table('test_table');

table.insert().values(1, "george", 18).execute();
table.insert().values(2, "james", 17).execute();
table.insert().values(3, "luke", 18).execute();

#@ First execution is normal
crud = table.update().set('age', mysqlx.expr('age + 1'));
crud.execute();
table.select();

#@ Second execution prepares statement and executes it
crud.execute()
table.select();

#@ Third execution uses prepared statement
crud.execute()
table.select();

#@ set() changes statement, back to normal execution
crud.set('name', mysqlx.expr('concat(ucase(left(name, 1)), substring(name, 2))')).execute();
table.select();

#@ second execution after set(), prepares statement and executes it
crud.execute()
table.select();

#@ third execution after set(), uses prepared statement
crud.execute()
table.select();

#@ where() changes statement, back to normal execution
crud.where('name = "james"').execute();
table.select();

#@ second execution after where(), prepares statement and executes it
crud.execute();
table.select();

#@ third execution after where(), uses prepared statement
crud.execute();
table.select();

#@ order_by() changes statement, back to normal execution
crud.order_by('name desc').execute();
table.select();

#@ second execution after order_by(), prepares statement and executes it
crud.execute();
table.select();

#@ third execution after order_by(), uses prepared statement
crud.execute();
table.select();

#@ limit() changes statement, back to normal execution
crud.limit(1).execute();
table.select();

#@ second execution after limit(), prepares statement and executes it
crud.execute();
table.select();

#@ third execution after limit(), uses prepared statement
crud.execute();
table.select();

#@ creates statement to test no changes when reusing bind(), limit() and offset()
crud = table.update().set('age', mysqlx.expr('age+1')).where('name like :ph').limit(3)
crud.bind('ph', '%').execute();
table.select();

#@ prepares statement to test no changes when reusing bind(), limit() and offset()
crud.bind('ph', '%').execute();
table.select();

#@ Reusing statement with bind() using g%
crud.bind('ph', 'g%').execute();
table.select();

#@ Reusing statement with bind() using j%
crud.bind('ph', 'j%').execute();
table.select();

#@ Reusing statement with bind() using l%
crud.bind('ph', 'l%').execute();
table.select();

#@ Reusing statement with new limit()
crud.limit(1).bind('ph', '%').execute();
table.select();

#@ Reusing statement with new limit() and offset()
crud.limit(2).bind('ph', '%').execute();
table.select();

#@<> Finalizing
session.drop_schema('prepared_stmt');
session.close();
