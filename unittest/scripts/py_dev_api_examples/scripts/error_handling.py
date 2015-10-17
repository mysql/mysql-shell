# Assumptions: test schema exists, my_collection exists

import mysqlx

mySession

try:
  # Connect to server on localhost
  mySession = mysqlx.getSession( {
          'host': 'localhost', 'port': 33060,
          'dbUser': 'mike', 'dbPassword': 's3cr3t!' } )

except Exception, err:
  print 'The database session could not be opened: %s' % err.message

try:
  myDb = mySession.getSchema('test')

  # Use the collection 'my_collection'
  myColl = myDb.getCollection('my_collection')

  # Find a document
  myDoc = myColl.find('name like :param').limit(1).bind('param','S%').execute()

  # Print document
  print myDoc.first()
except Exception, err:
  print 'The following error occurred: %s' % err.message
finally:
  # Close the session in any case
  mySession.close()
