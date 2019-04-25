# Gets the collection
myColl = db.get_collection('my_collection')

# Insert a document
res = myColl.add({'name': 'Jack', 'age': 15, 'height': 1.76 }).add({'name': 'Jim', 'age': 68, 'height': 1.72 }).execute()

# Print the last document id
print("ID for Jim's Document: %s\n" % res.get_generated_ids()[1])
