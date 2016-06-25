# app code, requiring an explicit call to .execute()
db.get_collection('my_coll').add({'name': 'mike'}).execute();

# interactive shell command with direct access
# to collections from the db object
db.my_coll.add({'name': 'mike'});
