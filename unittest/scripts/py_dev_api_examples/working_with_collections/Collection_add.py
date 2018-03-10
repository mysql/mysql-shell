# Create a new collection
myColl = db.create_collection('my_collection')

# Insert a document
myColl.add( {'_id': '1', 'name': 'Sakila', 'age': 15 } ).execute()

# Insert several documents at once
myColl.add( [
{'_id': '5', 'name': 'Susanne', 'age': 24 },
{'_id': '6', 'name': 'Mike', 'age': 39 } ] ).execute()
