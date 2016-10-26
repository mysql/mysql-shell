# Working with Relational Tables
import mysqlx

# Connect to server using a connection URL
mySession = mysqlx.get_node_session( {
  'host': 'localhost', 'port': 33060,
  'dbUser': 'mike', 'dbPassword': 's3cr3t!'} )

myDb = mySession.get_schema('test')

# Accessing an existing table
myTable = myDb.get_table('my_table')

# Insert SQL Table data
myTable.insert(['name','birthday','age']) \
  .values('Sakila', mysqlx.date_value(2000, 5, 27), 16).execute()

# Find a row in the SQL Table
myResult = myTable.select(['_id', 'name', 'birthday']) \
  .where('name like :name AND age < :age') \
  .bind('name', 'S%') \
  .bind('age', 20).execute()

# Print result
print myResult.fetch_all()
