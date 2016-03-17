
# Working with Relational Tables

import mysqlx

# Connect to server using a connection URL
db = mysqlx.getSession( {
  'host': 'localhost', 'port': 33060,
  'dbUser': 'mike', 'dbPassword': 's3cr3t!'}
  ).getSchema('test')

# Accessing an existing table
myTable = db.getTable('my_table')

# Insert SQL Table data
myTable.insert(['name','birthday','age']) \
  .values('Sakila', mysqlx.dateValue(2000, 5, 27), 16).execute()

# Find a row in the SQL Table
myResult = myTable.select(['_id', 'name', 'birthday']) \
  .where('name like :name AND age < :age') \
  .bind('name', 'S%') \
  .bind('age', 20).execute()

# Print result
print myResult.fetchAll()
