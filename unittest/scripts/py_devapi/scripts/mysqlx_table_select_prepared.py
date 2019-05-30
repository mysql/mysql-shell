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
crud = table.select();
crud.execute();

#@ Second execution prepares statement and executes it
crud.execute()

#@ Third execution uses prepared statement
crud.execute()

#@ where() changes statement, back to normal execution
crud.where('age >= 18').execute();

#@ second execution after where(), prepares statement and executes it
crud.execute()

#@ third execution after where(), uses prepared statement
crud.execute()

#@ order_by() changes statement, back to normal execution
crud.order_by('name desc').execute();

#@ second execution after order_by(), prepares statement and executes it
crud.execute()

#@ third execution after order_by(), uses prepared statement
crud.execute()

#@ limit() changes statement, back to normal execution
crud.limit(1).execute();

#@ second execution after limit(), prepares statement and executes it
crud.execute()

#@ third execution after limit(), uses prepared statement
crud.execute()

#@ offset() does not change the statement, uses prepared one
crud.offset(1).execute();

#@ lock_exclusive() changes statement, back to normal execution
crud.lock_exclusive().execute();

#@ second execution after lock_exclusive(), prepares statement and executes it
crud.execute()

#@ third execution after lock_exclusive(), uses prepared statement
crud.execute()

#@ creates statement to test lock_shared()
crud = table.select();
crud.execute();

#@ prepares statement to test lock_shared()
crud.execute();

#@ lock_shared() changes statement, back to normal execution
crud.lock_shared().execute();

#@ second execution after lock_shared(), prepares statement and executes it
crud.execute()

#@ third execution after lock_shared(), uses prepared statement
crud.execute()

#@ creates statement with aggregate function to test having()
crud = table.select('age', 'count(age) as number').group_by('age')
crud.execute();

#@ prepares statement with aggregate function to test having()
crud.execute();

#@ having() changes statement, back to normal execution
crud.having('number > 1').execute();

#@ second execution after having(), prepares statement and executes it
crud.execute()

#@ third execution after having(), uses prepared statement
crud.execute()


#@ creates statement to test no changes when reusing bind(), limit() and offset()
crud = table.select().where('name like :ph').limit(3)
crud.bind('ph', '%').execute();

#@ prepares statement to test no changes when reusing bind(), limit() and offset()
crud.bind('ph', '%').execute();

#@ Reusing statement with bind() using g%
crud.bind('ph', 'g%').execute();

#@ Reusing statement with bind() using j%
crud.bind('ph', 'j%').execute();

#@ Reusing statement with bind() using l%
crud.bind('ph', 'l%').execute();

#@ Reusing statement with new limit()
crud.limit(1).bind('ph', '%').execute();

#@ Reusing statement with new offset()
crud.offset(1).bind('ph', '%').execute();

#@ Reusing statement with new limit() and offset()
crud.limit(2).offset(1).bind('ph', '%').execute();

#@<> Finalizing
session.drop_schema('prepared_stmt');
session.close();
