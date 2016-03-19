# Assumptions: test schema assigned to db, my_collection collection exists

# Gets the collection
myColl = db.getCollection('my_collection')

# Insert a document
res = myColl.add({ 'name': 'Jack', 'age': 15, 'height': 1.76 }).add({ 'name': 'Jim', 'age': 68, 'height': 1.72 }).execute()

# Print the last document id
print "ID for Jim's Document: %s\n" % res.getLastDocumentId()
