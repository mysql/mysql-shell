from mysqlsh import mysqlx

# Connect to server on localhost
mySession = mysqlx.get_session( {
        'host': 'localhost', 'port': 33060,
        'user': 'mike', 'password': 'paSSw0rd' } )

myDb = mySession.get_schema('test')

# Use the collection 'my_collection'
myColl = myDb.get_collection('my_collection')

# Specify which document to find with Collection.find() and
# fetch it from the database with .execute()
myDocs = myColl.find('name like :param').limit(1).bind('param', 'S%').execute()

# Print document
document = myDocs.fetch_one()
print(document)

mySession.close()
