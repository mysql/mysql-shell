# Assumptions: test schema assigned to db, my_collection collection exists

# Use the collection 'my_collection'
myColl = db.getCollection('my_collection')

# Find a single document that has a field 'name' starts with an 'S'
docs = myColl.find('name like :param').limit(1).bind('param', 'S%').execute()

print docs.fetchOne()

# Get all documents with a field 'name' that starts with an 'S'
docs = myColl.find('name like :param').bind('param','S%').execute()

myDoc = docs.fetchOne()
while myDoc:
	print myDoc
	myDoc = docs.fetchOne()
