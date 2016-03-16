
import mysqlx

# Connect to server on localhost
mySession = mysqlx.getSession( {
        'host': 'localhost', 'port': 33060,
        'dbUser': 'mike', 'dbPassword': 's3cr3t!' } )

myDb = mySession.getSchema('test')

# Use the collection 'my_collection'
myColl = myDb.getCollection('my_collection')

# Specify which document to find with Collection.find() and
# fetch it from the database with .execute()
myDocs = myColl.find('name like :param').limit(1).bind('param', 'S%').execute()

# Print document
document = myDocs.fetchOne()
print document

mySession.close()
