# Use the collection 'my_collection'
myColl = db.get_collection('my_collection')

# Find a single document that has a field 'name' starts with an 'S'
docs = myColl.find('name like :param').limit(1).bind('param', 'S%').execute()

print docs.fetch_one()

# Get all documents with a field 'name' that starts with an 'S'
docs = myColl.find('name like :param').bind('param','S%').execute()

myDoc = docs.fetch_one()
while myDoc:
  print myDoc
  myDoc = docs.fetch_one()
	    