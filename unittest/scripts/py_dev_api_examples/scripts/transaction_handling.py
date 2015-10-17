# Assumptions: test schema exists, my_collection collection not exists
import mysqlx

# Connect to server
session = mysqlx.getSession( {
  'host': 'localhost', 'port': 33060,
  'dbUser': 'mike', 'dbPassword': 's3cr3t!' } )

# Get the Schema test
db = session.getSchema('test')

# Create a new collection
myColl = db.createCollection('my_collection')

# Start a transaction
session.startTransaction()
try:
  myColl.add({'name': 'Jack', 'age': 15, 'height': 1.76, 'weight': 69.4}).execute()
  myColl.add({'name': 'Susanne', 'age': 24, 'height': 1.65}).execute()
  myColl.add({'name': 'Mike', 'age': 39, 'height': 1.9, 'weight': 74.3}).execute()

  # Commit the transaction if everything went well
  session.commit()

  print 'Data inserted successfully.'
except Exception, err:
  # Rollback the transaction in case of an error
  session.rollback()

  # Printing the error message
  print 'Data could not be inserted: %s' % err.message
